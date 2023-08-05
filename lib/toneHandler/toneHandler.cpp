#include "toneHandler.h"

toneHandler::toneHandler(unsigned pin, unsigned channel) {
  OUT_CHANNEL = channel;
  ledcSetup(channel, 10000, 8);
  ledcAttachPin(pin, channel);
}

void toneHandler::play(tones tone) {
  if(tone == currentTone) return;
  currentTone = tone;
  cadenceIndex = -1;
  cadenceSince = 0;
  run();
}

void toneHandler::run() {
  switch(currentTone){
    case silent:
      off();
      break;
    case dialtone:  // 350+440 Hz
      on(350);
      break;
    case ringback:   // 440+480 Hz @ 2000/4000 ms
      doCadence(440, 2, 2000, 4000);
      break;
    case howler:    // 1400+2060+2450+2600 Hz @ 100/100 ms
      doCadence(1400, 2, 100, 100);
      break;
    case busytone:  // 480+620 Hz @ 500/500 ms
      doCadence(480, 2, 500, 500);
      break;
  }
}

void toneHandler::doCadence(int freq, int cadenceLength, ...){
  // extract desired value from varadic args (passing an array of variable length seems impossible in C++)
  int ms = 0;
  va_list cadence;
  va_start(cadence, cadenceLength);
  for(int i=0;i<=cadenceIndex;i++)
    ms = va_arg(cadence, int);
  va_end(cadence);

  if(cadenceIndex >= 0 && (millis() - cadenceSince) < ms) return;
  cadenceSince = millis();
  cadenceIndex++;
  if(cadenceIndex >= cadenceLength) cadenceIndex = 0;
  if(cadenceIndex % 2 == 0) on(freq); else off();
}

void toneHandler::on(int freq) {
  if(toneFreq == freq) return;
  toneFreq = freq;
  ledcWriteTone(OUT_CHANNEL, freq);
}

void toneHandler::off() {
  if(toneFreq == 0) return;
  toneFreq = 0;
  ledcWrite(OUT_CHANNEL, 0);
}