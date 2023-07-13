#ifndef mozziHandler_h
#define mozziHandler_h

#include "Arduino.h"
#include <regions.h>

class mozziHandler
{
  public:
    enum tones {
      dialtone,
      ringing,
      busytone,
      howler
    };

    enum samples {
      dialAgain
    };

    mozziHandler(regions region);
    void changeRegion(regions region);
    RegionConfig currentRegion();
    void playTone(tones tone);
    void playSample(samples sample, byte repeat, unsigned gapTime);
    void run();
    void stop();

  private:
    regions _region;  // control which call progress sounds to generate based on region
    void messageDialAgain(byte repeat, unsigned gapTime);
    void toneStart(ToneConfig tc);
    void toneStop();
};

#endif