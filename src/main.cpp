#include <Arduino.h>
#include <progressModes.h>
#include <ringHandler.h>
#include <hookHandler.h>
#include <mozziHandler.h>

#define PIN_LED 2
#define PIN_BTN 12
#define PIN_SHK 13
#define PIN_RM 32
#define PIN_FR 33
#define CH_FR 0
#define RING_FREQ 20

#define ENABLE_DTMF false // DTMF and Mozzi don't play nice together, so I want to control them easily
#if ENABLE_DTMF
  #include <PhoneDTMF.h>
  #define PIN_DTMF 14
  auto dtmf = PhoneDTMF(300); // 70 seems to work well
#endif

String digits; // this is where we accumulate dialed digits
auto ringer = ringHandler(PIN_RM, PIN_FR, CH_FR, RING_FREQ);
auto hooker = hookHandler(PIN_SHK, dialingStartedCallback);
auto mozzi = mozziHandler(region_northAmerica);

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLDOWN);
  pinMode(PIN_SHK, INPUT_PULLUP);
  pinMode(PIN_RM, OUTPUT);
  Serial.begin(115200);
  ringer.setCounterCallback(ringCountCallback);
  hooker.setDigitCallback(digitReceivedCallback);

  #if ENABLE_DTMF
    pinMode(PIN_DTMF, INPUT);
    dtmf.begin(PIN_DTMF, 4000); // 4000 seems to work well once dialtone stops
  #endif
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
      if(!BTN) return call_idle;
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
  Serial.printf("%s...", modeNames[newmode]);

  digitalWrite(PIN_LED, digitalRead(PIN_SHK)); // basic off-hook status; might expand later with addressable RGB

  switch(newmode){
    case call_incoming:
      ringer.start();
      break;
    case call_ready:
      #if !ENABLE_DTMF // DTMF and Mozzi don't play nice together AT ALL
      mozzi.play(mozzi.dialtone);
      #endif
      timeoutStart();
      hooker.start();
      break;
    case call_pulse_dialing:
    case call_tone_dialing:
      timeoutStart();
      break;
    case call_timeout:
      mozzi.play(mozzi.howler);
      break;
    case call_abandoned:
      mozzi.play(mozzi.dialAgain);  //TODO: this should really be when dialing first times out, played twice then play mozzi.offHookToneStart(); call_abandoned should be silence
      break;
    case call_connecting:
      //TODO: this is just simulating ring or busy based on first digit of dialed number (even=ring); implement call negotiation later
      if((digits[0] - '0') % 2== 0)
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
      detectDTMF();
      hooker.run();
      timeoutCheck();
      break;
    case call_pulse_dialing:
      hooker.run();
      timeoutCheck();
      break;
    case call_tone_dialing:
      detectDTMF();
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

//-------------- dialing stuff ---------------------//

void dialingStartedCallback(){
  digits = "";
  modeGo(call_pulse_dialing);
}

// we can use this as callback for both pulse and tone dialing handlers
void digitReceivedCallback(char digit){
  timeoutStart();           // reset timeout after user input
  Serial.print(digit);      // debug output show digits as they are received
  digits += digit;          // accumulate the digit

  //TODO: decide when appropriate to "connect" (i.e., another node, simulation or special behavior [config website?])
  if(digits.length() > 3){  // fake decision to connect
    modeGo(call_connecting);
    Serial.print(digits);
  }
}

void detectDTMF(){
  #if ENABLE_DTMF
  char button = dtmf.tone2char(dtmf.detect());
  if(button > 0) {
    Serial.print(button);
    if(mode == call_ready)
      modeGo(call_tone_dialing);
    else
      timeoutStart();
  }
  #endif
}
