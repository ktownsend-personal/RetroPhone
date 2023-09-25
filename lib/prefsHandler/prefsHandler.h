#pragma once

#include "Arduino.h"
#include "regionConfig.h"
#include "Preferences.h"

class prefsHandler {
  public:
    void setRegion(regions region);
    regions getRegion();
    void setDTMF(bool softwareDTMF);
    bool getDTMF();
  
  private:
    Preferences prefs;

};