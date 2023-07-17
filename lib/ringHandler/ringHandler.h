#ifndef ringer_h
#define ringer_h

#include "Arduino.h"

class ringHandler {
  public:
    ringHandler(unsigned pinRM, unsigned pinFR, unsigned channelFR);
    void setCounterCallback(void (*callback)(const int));
    void start(int freq, int* cadence);
    void run();
    void stop();
    int ringCount = 0;

  private:
    unsigned PIN_RM;
    unsigned CH_FR;
    unsigned RING_FREQ;
    int* ringCadence; // expects cadence array, first element count of timings in the array, then the timings
    int cadenceCount = 0;
    int cadenceIndex = 0;
    unsigned long cadenceSince = 0;
    void on();
    void off();
    void (*counterCallback)(const int);
};

#endif