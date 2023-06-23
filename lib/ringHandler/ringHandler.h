#ifndef ringer_h
#define ringer_h

#include <Arduino.h>

class ringHandler {
  public:
    ringHandler(unsigned pinRM, unsigned pinFR, unsigned channelFR, unsigned freq);
    void setCounterCallback(void (*callback)(const int));
    void start();
    void run();
    void stop();
    int ringCount = 0;

  private:
    unsigned PIN_RM;
    unsigned CH_FR;
    unsigned RING_FREQ;
    unsigned cadence[2] = {2000, 4000};
    int cadenceLength = sizeof(cadence) / sizeof(cadence[0]); // this is how you get array length
    int cadenceIndex = 0;
    unsigned long cadenceSince = 0;
    void on();
    void off();
    void (*counterCallback)(const int);
};

#endif