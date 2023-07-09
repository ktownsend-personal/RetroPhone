#include <dtmfHandler.h>
#include <PhoneDTMF.h>
#include <bitset>
#include <driver/adc.h>

auto dtmf = PhoneDTMF(128);

dtmfHandler::dtmfHandler(unsigned pinDTMF, void (*callback)(bool)) {
  PIN = pinDTMF;
  dialingStartedCallback = callback;
  pinMode(PIN, INPUT);
  dtmf.begin(PIN, 6000);
}

void dtmfHandler::setDigitCallback(void (*callback)(char)) {
  digitReceivedCallback = callback;
}

void dtmfHandler::start(){
  newDTMF = true;
  currentDTMF = 0;

  Serial.printf("DTMF: controller sample frequency limit: %d\n", dtmf.getSampleFrequence());
  Serial.printf("DTMF: sampling frequency: %d\n", dtmf.getRealFrequence());
  Serial.printf("DTMF: analog center: %d\n", dtmf.getAnalogCenter());
  Serial.printf("DTMF: base magnitude: %d\n", dtmf.getBaseMagnitude());
  Serial.printf("DTMF: estimated measurement time: %d\n", dtmf.getMeasurementTime());
}

void dtmfHandler::run(){
  auto tone = dtmf.detect();
  
  std::bitset<8> b(tone);
  if(tone != 0 && b.count() != 2) return; // ignore when not 2 bits active because that's not a good read (one bit per frequency detected out of 8 possible)

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
