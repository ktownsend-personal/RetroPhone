#ifndef regionConfig_h
#define regionConfig_h

#include "Arduino.h"

enum regions {
  region_northAmerica,
  region_unitedKingdom
};

struct RingConfig {
  int freq;       // expects Hz, typically 20 or 25
  int* cadence;   // expects array with cadence count as first item and then the intervals (milliseconds, sequenced as alternating on and off)
};

struct ToneConfig {
  int* freqs;     // expects array with freq count as first item and then the frequencies to mix (Hz, up to 4)
  int* cadence;   // expects array with cadence count as first item and then the intervals (milliseconds, sequenced as alternating on and off)
};

class RegionConfig {
  public:
    RegionConfig(regions regionToUse = region_northAmerica);
    regions region;
    String label;
    RingConfig ringer;
    ToneConfig dialtone;
    ToneConfig busytone;
    ToneConfig ringback;
    ToneConfig howler;
    ToneConfig zip;
    ToneConfig err;
    ToneConfig reorder; // fast-busy
};

#endif