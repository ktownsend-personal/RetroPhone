#include "ringHandler.h"

void ringHandler::init(unsigned pinRM, unsigned pinFR, unsigned channelFR) {
  RMpin = pinRM;
  FRch = channelFR;
  pinMode(pinRM, OUTPUT);
  ledcSetup(channelFR, 20, 8); // this freq will get replaced by start(), so it's fairly arbitrary
  ledcAttachPin(pinFR, channelFR);
}

void ringHandler::setCounterCallback(void (*callback)(const int)){
  counterCallback = callback;
}

void ringHandler::start(int freq, int* cadence){
  RING_FREQ = freq;
  ringCadence = cadence;
  cadenceCount = cadence[0];
  cadenceIndex = 0;
  cadenceSince = 0;
  ringCount = 0;
  run();
}

void ringHandler::run(){
  if(cadenceIndex > 0 && (millis() - cadenceSince) < ringCadence[cadenceIndex]) return;
  cadenceSince = millis();
  cadenceIndex++;
  if(cadenceIndex > cadenceCount) cadenceIndex = 1;
  if(cadenceIndex == 1) {
    ringCount++;
    counterCallback(ringCount);
  }
  if(cadenceIndex % 2 == 1) on(); else off();
}

void ringHandler::stop(){
  off();
}

void ringHandler::on(){
  digitalWrite(RMpin, HIGH);
  ledcWriteTone(FRch, RING_FREQ);
}

void ringHandler::off(){
  ledcWriteTone(FRch, 0);
  digitalWrite(RMpin, LOW);
}
