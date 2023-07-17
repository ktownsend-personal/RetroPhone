#ifndef mozziHandler_h
#define mozziHandler_h

#include "Arduino.h"
#include "regionConfig.h"

class mozziHandler
{
  public:
    enum tones {
      dialtone,
      ringing,
      busytone,
      howler,
      zip,
      err
    };

    enum samples {
      dialAgain
    };

    mozziHandler(RegionConfig region);
    void changeRegion(RegionConfig region);
    void playTone(tones tone, byte iterations = 0);
    void playSample(samples sample, byte iterations, unsigned gapTime);
    void playDTMF(char digit, int length);
    void run();
    void stop();

  private:
    void messageDialAgain(byte iterations, unsigned gapTime);
    void toneStart(ToneConfig tc, byte iterations = 0);
    void toneStop();
};

#endif