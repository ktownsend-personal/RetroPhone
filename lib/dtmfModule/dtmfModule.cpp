#include <dtmfModule.h>

dtmfModule::dtmfModule(byte pinQ1, byte pinQ2, byte pinQ3, byte pinQ4, byte pinSTQ, void (*callback)(bool)) {
  Q1 = pinQ1;
  Q2 = pinQ2;
  Q3 = pinQ3;
  Q4 = pinQ4;
  STQ = pinSTQ;
  dialingStartedCallback = callback;
  pinMode(Q1, INPUT);
  pinMode(Q2, INPUT);
  pinMode(Q3, INPUT);
  pinMode(Q4, INPUT);
  pinMode(STQ, INPUT);
}

void dtmfModule::setDigitCallback(void (*callback)(char)) {
  digitReceivedCallback = callback;
}

void dtmfModule::start(){
  newDTMF = true;
  currentDTMF = 0;
}

void dtmfModule::run(){
  if(digitalRead(STQ)) {
    if(newDTMF) {
      dialingStartedCallback(true);
      newDTMF = false;
    }
    byte tone = 0x00 | (digitalRead(Q1) << 0) | (digitalRead(Q2) << 1) | (digitalRead(Q3) << 2) | (digitalRead(Q4) << 3);
    if(tone != currentDTMF) {
      currentDTMF = tone;

      // measuring space time
      // leadingEdgeTime = millis();
      // Serial.printf("<space=%dms>", leadingEdgeTime - trailingEdgeTime);
    }
  } else if(currentDTMF > 0) {
    byte digit = tone2char(currentDTMF);
    if(digit != ' ') digitReceivedCallback(tone2char(currentDTMF));
    currentDTMF = 0;

    // measuring tone time
    // trailingEdgeTime = millis();
    // Serial.printf("<tone=%dms>", trailingEdgeTime - leadingEdgeTime);
  }
}

char dtmfModule::tone2char(byte tone){
  switch(tone){
    case 0x01: return '1';
    case 0x02: return '2';
    case 0x03: return '3';
    case 0x04: return '4';
    case 0x05: return '5';
    case 0x06: return '6';
    case 0x07: return '7';
    case 0x08: return '8';
    case 0x09: return '9';
    case 0x0A: return '0';
    case 0x0B: return '*';
    case 0x0C: return '#';
    default: return ' '; // space means invalid value
  }
}