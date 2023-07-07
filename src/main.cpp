#include <Arduino.h>
#include <progressModes.h>
#include <ringHandler.h>
#include <hookHandler.h>
#include <mozziHandler.h>
// #include <PhoneDTMF.h>

#define PIN_LED 2
#define PIN_BTN 12
#define PIN_SHK 13
#define PIN_RM 32
#define PIN_FR 33
#define CH_FR 0
#define RING_FREQ 20

auto ringer = ringHandler(PIN_RM, PIN_FR, CH_FR, RING_FREQ);
auto hooker = hookHandler(PIN_SHK, dialingStartedCallback);
auto mozzi = mozziHandler(region_unitedKingdom);
//TODO:  auto dtmf = PhoneDTMF();

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLDOWN);
  pinMode(PIN_SHK, INPUT_PULLUP);
  pinMode(PIN_RM, OUTPUT);
  Serial.begin(115200);
  ringer.setCounterCallback(ringCountCallback);
  hooker.setDigitCallback(digitReceivedCallback);
  //TODO: dtmf.begin(TODO: ADC PIN); // use pin 14 for this (probably)
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

  switch(mode) {
    case call_idle:
      if(SHK) return call_ready;
      if(BTN) return call_incoming;
    case call_incoming:
      if(SHK) return call_connected;
      if(!BTN) return call_idle;
    default:
      if(!SHK) return call_idle;
  }
  return mode; // if we get here the mode didn't change
}

void modeStop(modes oldmode) {
  //Serial.printf("...stop %s\n", modeNames[oldmode]);
  Serial.println(" /");
  switch(oldmode){
    case call_incoming:
      ringer.stop();
      break;
    case call_ready:
    case call_timeout:
    case call_abandoned:
      mozzi.play(mozzi.silent);
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
      mozzi.play(mozzi.dialtone);
      timeoutStart();
      hooker.start();
      break;
    case call_dialing:
      timeoutStart();
      break;
    case call_timeout:
      mozzi.play(mozzi.howler);
      break;
    case call_abandoned:
      mozzi.play(mozzi.dialAgain);  //TODO: this should really be when dialing first times out, played twice then play mozzi.offHookToneStart(); call_abandoned should be silence
      break;
  }
}

void modeRun(modes mode){
  switch(mode){
    case call_incoming:
      ringer.run();
      break;
    case call_ready:
    case call_dialing:
      hooker.run();
      timeoutCheck();
      break;
    case call_timeout:
      timeoutCheck();
      break;
  }
  mozzi.run(); //TODO: should this be done conditionally when we know audio should be playing?
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

String digits;

void dialingStartedCallback(){
  digits = "";
  modeGo(call_dialing);
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
