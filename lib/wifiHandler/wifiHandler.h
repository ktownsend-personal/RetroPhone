#pragma once

#include "Arduino.h"
#include "WiFiManager.h"  // https://github.com/tzapu/WiFiManager

class wifiHandler {
  public:
    void init();
    void run();
    void showNetworks();
    void showStatus();

  private:
    bool connected;
};