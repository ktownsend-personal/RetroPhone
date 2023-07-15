#ifndef mozziHandler_h
#define mozziHandler_h

#include "Arduino.h"
#include <regionConfig.h>

class mozziHandler
{
  public:
    enum tones {
      dialtone,
      ringing,
      busytone,
      howler,
      zip
    };

    enum samples {
      dialAgain
    };

    mozziHandler(RegionConfig region);
    void changeRegion(RegionConfig region);
    void playTone(tones tone, byte repeat = 0);
    void playSample(samples sample, byte repeat, unsigned gapTime);
    void run();
    void stop();

  private:
    void messageDialAgain(byte repeat, unsigned gapTime);
    void toneStart(ToneConfig tc, byte repeat = 0);
    void toneStop();
};

#endif