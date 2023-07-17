#ifndef dtmfer_h
#define dtmfer_h

#include "Arduino.h"

class dtmfHandler {
  public:
    dtmfHandler(unsigned pinDTMF, void (*callback)(bool));
    void setDigitCallback(void (*callback)(char));
    void start();
    void run();

  private:
    int PIN;
    bool newDTMF;
    uint8_t currentDTMF;
    unsigned long leadingEdgeTime;
    unsigned long trailingEdgeTime;
    void (*dialingStartedCallback)(bool);
    void (*digitReceivedCallback)(char);
};

#endif