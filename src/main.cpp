#include <Arduino.h>
#include <progressModes.h>
#include <ringHandler.h>
#include <hookHandler.h>
#include <dtmfHandler.h>
#include <mozziHandler.h>

#define PIN_LED 2
#define PIN_BTN 12
#define PIN_DTMF 14
#define PIN_SHK 13
#define PIN_RM 32
#define PIN_FR 33
#define CH_FR 0
#define RING_FREQ 20

const bool ENABLE_DTMF = false; // DTMF and Mozzi don't play nice together; true disables dialtone, false distables DTMF

String digits; // this is where we accumulate dialed digits
auto ringer = ringHandler(PIN_RM, PIN_FR, CH_FR, RING_FREQ);
auto hooker = hookHandler(PIN_SHK, dialingStartedCallback);
auto dtmfer = dtmfHandler(PIN_DTMF, dialingStartedCallback);
auto mozzi = mozziHandler(region_northAmerica);

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLDOWN);
  pinMode(PIN_SHK, INPUT_PULLUP);
  Serial.begin(115200);
  ringer.setCounterCallback(ringCountCallback);
  hooker.setDigitCallback(digitReceivedCallback);
  dtmfer.setDigitCallback(digitReceivedCallback);
}

void loop() {
  modeRun(mode);                        // always run the current mode before modeDetect() because mode can change during modeRun()
  modes newmode = modeDetect();         // check inputs for new mode
  if(modeBouncing(newmode)) return;     // wait for bounce to settle
  if(newmode != mode) modeGo(newmode);  // switch to new mode
}

//TODO: move this to a class; can we do "any" type arg in C++ like we can in golang? Look at template/auto
bool modeBouncing(modes newmode) {
  // kill ringer faster by skipping debounce when going off-hook; debounce continues fine after mode change
  if(mode == call_incoming && newmode == call_connected) return false;
  // reset debounce timer on each observation of change
  if(newmode != lastDebounceValue){
    lastDebounceTime = millis();
    lastDebounceValue = newmode;
    return true;
  }
  // true = bouncing, false = stable
  return (millis() - lastDebounceTime) < debounceDelay;
}

modes modeDetect() {
  int SHK = digitalRead(PIN_SHK);
  int BTN = digitalRead(PIN_BTN);

  //NOTE: every case must have a break to prevent fallthrough if return conditions not met
  switch(mode) {
    case call_idle:
      if(SHK) return call_ready;
      if(BTN) return call_incoming;
      break;
    case call_incoming:
      if(SHK) return call_connected;
      // if(!BTN) return call_idle; // enabling this will stop ring when button is released; disable this to ring until answered without having to hold button down
      break;
    default:
      if(!SHK) return call_idle;
      break;
  }
  return mode; // if we get here the mode didn't change
}

void modeStop(modes oldmode) {
  Serial.println(" /");
  switch(oldmode){
    case call_incoming:
      ringer.stop();
      break;
    default:
      mozzi.play(mozzi.silent);
      break;
  }
}

void modeStart(modes newmode) {
  Serial.printf("%s...", modeNames[newmode].c_str());

  digitalWrite(PIN_LED, digitalRead(PIN_SHK)); // basic off-hook status; might expand later with addressable RGB

  switch(newmode){
    case call_incoming:
      ringer.start();
      break;
    case call_ready:
      timeoutStart();
      if(!ENABLE_DTMF) mozzi.play(mozzi.dialtone); // DTMF blocks too much to play audio
      hooker.start();
      dtmfer.start();
      break;
    case call_pulse_dialing:
    case call_tone_dialing:
      timeoutStart();
      break;
    case call_timeout:
      mozzi.play(mozzi.howler);
      break;
    case call_abandoned:
      mozzi.play(mozzi.dialAgain);  //TODO: this should really be when dialing times out (played twice then play mozzi.howler) disconnected timeout should skip and just do howler; call_abandoned should be silence
      break;
    case call_connecting:
      //TODO: implement real connection negotiation; this fake simulation rings if first digit even, or busy if odd
      if(digits[0] % 2== 0)
        modeGo(call_ringing);
      else
        modeGo(call_busy);
      break;
    case call_ringing:
      mozzi.play(mozzi.ringing);
      break;
    case call_busy:
      mozzi.play(mozzi.busytone);
      break;
  }
}

void modeRun(modes mode){
  mozzi.run(); // this must always run because mozzi needs a lot of cycles to transition to silence

  switch(mode){
    case call_incoming:
      ringer.run();
      break;
    case call_ready:
      if (ENABLE_DTMF) dtmfer.run(); // DTMF blocks too much to play audio, so if we want dialtone we have to disable this
      hooker.run();
      timeoutCheck();
      break;
    case call_pulse_dialing:
      hooker.run();
      timeoutCheck();
      break;
    case call_tone_dialing:
      dtmfer.run();
      timeoutCheck();
      break;
    case call_timeout:
      timeoutCheck();
      break;
  }
}

void modeGo(modes newmode){
  modeStop(mode);
  modeStart(newmode);
  mode = newmode;
}

//TODO: move timeout to a class?

void timeoutStart(){
  timeoutStarted = millis();
}

void timeoutCheck(){
  unsigned int since = (millis() - timeoutStarted);
  if(since > abandonMax && mode == call_timeout) return modeGo(call_abandoned); // check longer max first
  if(since > timeoutMax && mode != call_timeout) return modeGo(call_timeout);
}

// callback from ringer class so we can show ring counts in the serial output
void ringCountCallback(int rings){
  Serial.printf("%d.", rings);
}

//-------------- dialing stuff to refactor later ---------------------//

void dialingStartedCallback(bool isTone){
  digits = "";
  if(isTone)
    modeGo(call_tone_dialing);
  else
    modeGo(call_pulse_dialing);
}

// we can use this as callback for both pulse and tone dialing handlers
void digitReceivedCallback(char digit){
  timeoutStart();           // reset timeout after user input
  Serial.print(digit);      // debug output show digits as they are received
  digits += digit;          // accumulate the digit

  //TODO: decide when appropriate to "connect" (i.e., another node, simulation or special behavior [config website?])
  if(digits.length() > 6){  // fake decision to connect
    modeGo(call_connecting);
    Serial.print(digits);
  }
}
