#ifndef toneHandler_h
#define toneHandler_h

#include "Arduino.h"

class toneHandler {
  public:
    enum tones {
      silent,
      dialtone,
      ringing,
      busytone,
      howler
    };

    toneHandler(unsigned pin, unsigned channel);
    void play(tones tone);
    void run();

  private:
    unsigned OUT_CHANNEL;
    tones currentTone = silent;
    int cadenceIndex = -1;
    unsigned long cadenceSince = 0;
    int toneFreq = 0;
    void doCadence(int freq, int cadenceLength, ...);
    void on(int freq);
    void off();
};

#endif