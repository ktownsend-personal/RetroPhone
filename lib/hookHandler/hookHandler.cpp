#include <hookHandler.h>

hookHandler::hookHandler(unsigned pinSHK, void (*callback)(bool)) {
  PIN = pinSHK;
  dialingStartedCallback = callback;
}

void hookHandler::setDigitCallback(void (*callback)(char)) {
  digitReceivedCallback = callback;
}

void hookHandler::start(){
  dialingStarting = false;
  pulseInProgress = false;
  pulseTime = 0;
  pulseCount = 0;
}

void hookHandler::run(){
  bool newSHK = digitalRead(PIN);
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
  SHK = newSHK;             // track change
  if(SHK == LOW) {
    // possible dialing start, but could be just hanging up so defer callback until SHK goes high again
    if(pulseTime == 0) dialingStarting = true; 
    // on low state start tracking pulse time
    pulseTime = millis();
    pulseInProgress = true;
    return;
  }
  if(pulseInProgress && gap > pulseGapMin){
    // accumulate pulse count
    pulseCount++;
    pulseInProgress = false;
    // notify when dialing started so controller can change mode
    if(dialingStarting){
      dialingStarting = false;
      dialingStartedCallback(false);
    }
  }
}
