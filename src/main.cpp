#include <Arduino.h>
#include <progressModes.h>
#include <toneHandler.h>
#include <ringHandler.h>
#include <hookHandler.h>
// #include <PhoneDTMF.h>

#define PIN_LED 2
#define PIN_BTN 12
#define PIN_SHK 13
#define PIN_RM 32
#define PIN_FR 33
#define CH_FR 0
#define PIN_AUDIO_OUT 14  //TODO: move our output to a DAC pin and put SLIC output (our input) on 14 for DTMF decoding in software with PhoneDTMF
#define CH_AUDIO_OUT 1
#define RING_FREQ 20

auto ringer = ringHandler(PIN_RM, PIN_FR, CH_FR, RING_FREQ);
auto toner = toneHandler(PIN_AUDIO_OUT, CH_AUDIO_OUT);
auto hooker = hookHandler(PIN_SHK, dialingStartedCallback);
//TODO:  auto dtmf = PhoneDTMF();

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLDOWN);
  pinMode(PIN_SHK, INPUT_PULLDOWN);
  pinMode(PIN_RM, OUTPUT);
  Serial.begin(115200);
  ringer.setCounterCallback(ringCountCallback);
  hooker.setDigitCallback(digitReceivedCallback);
  //TODO: dtmf.begin(TODO: ADC PIN); // use pin 14 for this after I move the SLIC input to a DAC pin and connect SLIC output to 14
}

void loop() {
  modes newmode = modeDetect();         // check inputs for new mode
  modeRun(mode);                        // always run the current mode first
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
    // Serial.print(","); // visual cue each time value bounces
    return true;
  }
  // true = bouncing, false = stable
  return (millis() - lastDebounceTime) < debounceDelay;
}

modes modeDetect() {
  int SHK = digitalRead(PIN_SHK);
  int BTN = digitalRead(PIN_BTN);

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
  Serial.printf("...stop %s\n", modeNames[oldmode]);
  switch(oldmode){
    case call_incoming:
      ringer.stop();
      break;
    case call_ready:
      toner.play(toner.silent);
      break;
    case call_timeout:
      toner.play(toner.silent);
      break;
  }
}

void modeStart(modes newmode) {
  Serial.printf("%s...", modeNames[newmode]);

  digitalWrite(PIN_LED, digitalRead(PIN_SHK)); // basic off-hook status; might change to different cue patterns for different modes unless I devote more output pins for different signal LED's

  switch(newmode){
    case call_incoming:
      ringer.start();
      break;
    case call_ready:
      toner.play(toner.dialtone);
      timeoutStart();
      hooker.start();
      break;
    case call_dialing:
      timeoutStart();
      break;
    case call_timeout:
      toner.play(toner.howler);
      break;
  }
}

void modeRun(modes mode){
  switch(mode){
    case call_incoming:
      ringer.run();
      break;
    case call_ready:
      hooker.run();
      timeoutCheck();
      break;
    case call_dialing:
      hooker.run();
      timeoutCheck();
      break;
    case call_timeout:
      timeoutCheck();
      break;
  }
  toner.run();
}

void modeGo(modes newmode){
  modeStop(mode);
  modeStart(newmode);
  mode = newmode;
}

//TODO: move timeout to a class

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

String digits;

void dialingStartedCallback(){
  digits = "";
  modeGo(call_dialing);
}

// we can use this as callback for both pulse and tone dialing handlers
void digitReceivedCallback(char digit){
  timeoutStart(); // reset timeout after user input
  Serial.print(digit); // debug output show digits as they are received
  //TODO: accumulate digits and decide when appropriate to "connect" (i.e., another node, simulation or special behavior [config website?])
  digits += digit;
  if(digits.length() > 3){ // fake decision to connect
    modeGo(call_connecting);
    Serial.print(digits);
  }
}
