#ifndef regions_h
#define regions_h

#include <Arduino.h>

enum regions {
  region_northAmerica,
  region_unitedKingdom
};

class RegionConfig {
  public:
    RegionConfig(regions region);
    int* dialFreq;
    int* dialCadence;
    int* busyFreq;
    int* busyCadence;
    int* ringFreq;
    int* ringCadence;
    int* howlFreq;
    int* howlCadence;
};

#endif