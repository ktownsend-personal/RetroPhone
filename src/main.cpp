#include <Arduino.h>
#include <progressModes.h>
#include <toneHandler.h>
#include <ringHandler.h>
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
//TODO:  auto dtmf = PhoneDTMF();

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLDOWN);
  pinMode(PIN_SHK, INPUT_PULLDOWN);
  pinMode(PIN_RM, OUTPUT);
  Serial.begin(115200);
  ringer.setCounterCallback(showRingCount);
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
    Serial.print(","); // visual cue each time value bounces
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
    case call_ready:
    case call_connected:
    case call_timeout:
    case call_abandoned:
      if(!SHK) return call_idle;
      break;
  }
  return mode; // if we get here the mode didn't change
}

void modeStop(modes oldmode) {
  Serial.printf("not %s\n", modeNames[oldmode]);
  switch(oldmode){
    case call_idle:
    case call_connected:
    case call_abandoned:
      break;
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
    case call_idle:
    case call_connected:
    case call_abandoned:
      break;
    case call_incoming:
      ringer.start();
      break;
    case call_ready:
      toner.play(toner.dialtone);
      timeoutStart();
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
      dialingCheck(mode);
      timeoutCheck();
      break;
    case call_dialing:
      dialingCheck(mode);
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
void showRingCount(int rings){
  Serial.printf("%d.", rings);
}

void dialingCheck(modes mode){
  //TODO: detect start and type of dialing in ready mode
  //TODO: monitor only detected dialing type and track values in dialing mode
  //TODO: this could probably be done as a class to encapsulate it
  //TODO: resest timeout check when we detect each digit

  /*
    - count pulses during debounce: lastSHK, pulseCount, pulseGapMin, pulseGapMax (min to ignore spurious, max to detect gaps between numbers)
    - capture digit and reset pulseCount if gap > max: digitAccumulator, digitCount (length of digitAccumulator, but incremented manually so we can avoid sizeof function on string, if that matters...might be fine to use sizeof)

    - after doing this in the loop, try doing it with interrupt and see how that goes (especially if interrupt works well for reducing ringing overlap in handset)
  */
}