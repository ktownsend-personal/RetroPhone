#include "prefsHandler.h"

void prefsHandler::setRegion(regions region) {
  prefs.begin("phone", false);
  prefs.putInt("region", region);
  prefs.end();
}

regions prefsHandler::getRegion() {
  prefs.begin("phone", false);
  regions r = (regions)prefs.getInt("region", (int)region_northAmerica);
  prefs.end();
  return r;
}

void prefsHandler::setDTMF(bool softwareDTMF) {
  prefs.begin("phone", false);
  prefs.putBool("dtmf", softwareDTMF);
  prefs.end();
}

bool prefsHandler::getDTMF() {
  prefs.begin("phone", false);
  bool dtmfmode = prefs.getBool("dtmf", false);
  prefs.end();
  return dtmfmode;
}