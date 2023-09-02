#include "pulseHandler.h"

pulseHandler::pulseHandler(unsigned pinSHK, void (*callback)(bool)) {
  PIN = pinSHK;
  dialingStartedCallback = callback;
}

void pulseHandler::setDigitCallback(void (*callback)(char)) {
  digitReceivedCallback = callback;
}

void pulseHandler::setMaybeCallback(void (*callback)()){
  maybeDialingStartedCallback = callback;
}

void pulseHandler::start(){
  maybe = false;
  dialing = false;
  pulsing = false;
  pulses = 0;
  edge = 0;
  lastDebounceTime = millis();
}

void pulseHandler::run(){
  bool newSHK = digitalRead(PIN);

  // debounce because SHK from SLIC is very noisy
  if(newSHK != lastDebounceValue) {
    if(!newSHK && !maybe) {
      maybeDialingStartedCallback();   // used for stopping dialtone faster
      maybe = true;
    }
    lastDebounceValue = newSHK;
    lastDebounceTime = millis();
    return;
  }
  if((millis() - lastDebounceTime) < pulseGapMin) return;

  unsigned gap = edge ? millis() - edge : 0;
  if(newSHK && SHK && gap <= pulseGapMax) return; // SHK high and unchanged, skip until long gap between digits
  if(!newSHK && !SHK) return;                     // SHK low and unchanged, skip always
  SHK = newSHK;

  // falling edge
  if(!SHK && !pulsing) {
    pulsing = true;
    edge = millis();
    return;
  }

  // rising edge
  if(SHK && pulsing){
    pulsing = false;
    pulses++;
    edge = millis();
    if(!dialing){ 
      // NOTE: callback on rising edge because falling edge occurs when user hangs up and causes a brief "pulse dialing" mode change
      dialing = true;
      dialingStartedCallback(false);
    }
    return;
  }

  // gap between digits
  if(SHK && !pulsing && gap > pulseGapMax){
    char digit = String(pulses % 10)[0];
    pulsing = false;
    pulses = 0;
    edge = 0;
    digitReceivedCallback(digit);
    return;
  }
}
