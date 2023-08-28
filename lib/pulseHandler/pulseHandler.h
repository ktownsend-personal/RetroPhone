#ifndef pulser_h
#define pulser_h

#include "Arduino.h"

class pulseHandler {
  public:
    pulseHandler(unsigned pinSHK, void (*dialingStartedCallback)(bool));
    void setDigitCallback(void (*digitReceivedCallback)(char));
    void start();
    void run();

  private:
    unsigned PIN;
    bool SHK;
    bool dialing;
    bool pulsing;
    byte pulses;
    bool lastDebounceValue;
    unsigned long lastDebounceTime;
    unsigned long edge;
    const unsigned pulseGapMin = 10;
    const unsigned pulseGapMax = 80;  //probably should be same as SHK debounceDelay in progressModes.h
    void (*dialingStartedCallback)(bool);
    void (*digitReceivedCallback)(char);
};

#endif