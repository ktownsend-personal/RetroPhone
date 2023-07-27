#include "dtmfHandler.h"
#include "PhoneDTMF.h"

auto dtmf = PhoneDTMF(300);

dtmfHandler::dtmfHandler(unsigned pinDTMF, void (*callback)(bool)) {
  PIN = pinDTMF;
  dialingStartedCallback = callback;
  pinMode(PIN, INPUT);
  dtmf.begin(PIN);
}

void dtmfHandler::setDigitCallback(void (*callback)(char)) {
  digitReceivedCallback = callback;
}

void dtmfHandler::start(){
  newDTMF = true;
  currentDTMF = 0;

  // just some info to see when debugging
  // Serial.printf("DTMF: controller sample frequency limit: %d\n", dtmf.getSampleFrequence());
  // Serial.printf("DTMF: sampling frequency: %d\n", dtmf.getRealFrequence());
  // Serial.printf("DTMF: analog center: %d\n", dtmf.getAnalogCenter());
  // Serial.printf("DTMF: base magnitude: %d\n", dtmf.getBaseMagnitude());
  // Serial.printf("DTMF: estimated measurement time: %d\n", dtmf.getMeasurementTime());
}

void dtmfHandler::run(){
  auto tone = dtmf.detect();

  if(tone > 0 && dtmf.tone2char(tone) == 0) return; // ignore bad reads
  
  if(tone > 0) {
    
    if(newDTMF) {
      dialingStartedCallback(true);
      newDTMF = false;
    }
    if(tone != currentDTMF) {
      currentDTMF = tone;

      // measuring space time
      // leadingEdgeTime = millis();
      // Serial.printf("<space=%dms>", leadingEdgeTime - trailingEdgeTime);
    }
  } else if(currentDTMF > 0) {
    digitReceivedCallback(dtmf.tone2char(currentDTMF));
    currentDTMF = 0;

    // measuring tone time
    // trailingEdgeTime = millis();
    // Serial.printf("<tone=%dms>", trailingEdgeTime - leadingEdgeTime);
  }
}
