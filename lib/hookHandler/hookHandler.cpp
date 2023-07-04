#include <hookHandler.h>

hookHandler::hookHandler(unsigned pinSHK, void (*callback)()) {
  PIN_SHK = pinSHK;
  dialingStartedCallback = callback;
}

void hookHandler::setDigitCallback(void (*callback)(char)) {
  digitReceivedCallback = callback;
}

void hookHandler::start(){
  pulseInProgress = false;
  pulseTime = 0;
  pulseCount = 0;
}

void hookHandler::run(){
  bool newSHK = digitalRead(PIN_SHK);
  unsigned gap = millis() - pulseTime;
  if(newSHK && pulseInProgress && gap > pulseGapMax) {
    // accumulate digit and reset pulse count
    char digit = String(pulseCount % 10)[0];
    pulseCount = 0;
    pulseInProgress = false;
    digitReceivedCallback(digit);
    return;
  }
  if(newSHK == SHK) return; // abort if unchanged 
  SHK = newSHK; // track change
  if(SHK == LOW) {
    // callback when dialing starts the very first time
    if(pulseTime == 0) dialingStartedCallback();
    // on low state start tracking pulse time
    pulseTime = millis();
    pulseInProgress = true;
    return;
  }
  if(pulseInProgress && gap > pulseGapMin){
    // accumulate pulse count
    pulseCount++;
    pulseInProgress = false;
  }
}
