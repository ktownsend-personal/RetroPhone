#ifndef mozziHandler_h
#define mozziHandler_h

#include "Arduino.h"
#include <regions.h>

class mozziHandler
{
  public:
    enum tones {
      silent,
      dialtone,
      ringing,
      busytone,
      howler,
      dialAgain
    };

    mozziHandler(regions region);
    void play(tones tone);
    void run();

  private:
    regions _region;  // control which call progress sounds to generate based on region
    void messageDialAgain();
    void toneStart(ToneConfig tc);
    void toneStop();
};

#endif