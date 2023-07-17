#ifndef dtmfmod_h
#define dtmfmod_h

#include "Arduino.h"

class dtmfModule {
  public:
    dtmfModule(byte pinQ1, byte pinQ2, byte pinQ3, byte pinQ4, byte pinSTQ, void (*callback)(bool));
    void setDigitCallback(void (*callback)(char));
    void start();
    void run();

    // make private after done testing
    char tone2char(byte tone);

  private:
    byte Q1;
    byte Q2;
    byte Q3;
    byte Q4;
    byte STQ;
    bool newDTMF;
    byte currentDTMF;
    void (*dialingStartedCallback)(bool);
    void (*digitReceivedCallback)(char);
};

#endif