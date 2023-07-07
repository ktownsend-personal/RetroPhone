#ifndef hooker_h
#define hooker_h

#include <Arduino.h>

class hookHandler {
  public:
    hookHandler(unsigned pinSHK, void (*callback)());
    void setDigitCallback(void (*callback)(char));
    void start();
    void run();

  private:
    unsigned PIN_SHK;
    bool SHK;
    bool dialingStarting;
    bool pulseInProgress;
    unsigned long pulseTime;
    const unsigned pulseGapMin = 10;
    const unsigned pulseGapMax = 70;
    unsigned pulseCount;
    void (*dialingStartedCallback)();
    void (*digitReceivedCallback)(char);
};

#endif