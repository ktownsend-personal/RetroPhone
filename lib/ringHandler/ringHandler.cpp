#include <ringHandler.h>

ringHandler::ringHandler(unsigned pinRM, unsigned pinFR, unsigned channelFR, unsigned freq) {
  PIN_RM = pinRM;
  CH_FR = channelFR;
  RING_FREQ = freq;
  pinMode(pinRM, OUTPUT);
  ledcSetup(channelFR, freq, 8);
  ledcAttachPin(pinFR, channelFR);
}

void ringHandler::setCounterCallback(void (*callback)(const int)){
  counterCallback = callback;
}

void ringHandler::start(){
  ringCount = 0;
  cadenceIndex = -1;
  cadenceSince = 0;
  run();
}

void ringHandler::run(){
  if(cadenceIndex >= 0 && (millis() - cadenceSince) < cadence[cadenceIndex]) return;
  cadenceSince = millis();
  cadenceIndex++;
  if(cadenceIndex >= cadenceLength) cadenceIndex = 0;
  if(cadenceIndex % 2 == 0) on(); else off();
}

void ringHandler::stop(){
  off();
}

void ringHandler::on(){
  digitalWrite(PIN_RM, HIGH);
  ledcWriteTone(CH_FR, RING_FREQ);
  ringCount++;
  counterCallback(ringCount);
}

void ringHandler::off(){
  ledcWriteTone(CH_FR, 0);
  digitalWrite(PIN_RM, LOW);
}
