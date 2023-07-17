#include "regionConfig.h"

RegionConfig::RegionConfig(regions region){

  // common settings for all regions
  howl.freqs     = new int[5]{4, 1400, 2060, 2450, 2600};
  howl.cadence   = new int[3]{2, 100, 100};
  howl.gain      = 2; // override gain for howler
  zip.freqs      = new int[2]{1, 440};
  zip.cadence    = new int[3]{2, 200, 100};
  err.freqs      = new int[5]{4, 1400, 2060, 2450, 2600};
  err.cadence    = new int[3]{2, 150, 100};
  err.gain       = 2;

  switch(region){
    case region_unitedKingdom:
      ringer.freq    = 25;
      ringer.cadence = new int[5]{4, 400, 200, 400, 2000};
      dial.freqs     = new int[3]{2, 350, 450};
      dial.cadence   = new int[1]{0};
      ring.freqs     = new int[3]{2, 400, 450};
      ring.cadence   = new int[5]{4, 400, 200, 400, 2000};
      busy.freqs     = new int[2]{1, 400};
      busy.cadence   = new int[3]{2, 375, 375};
      break;
    case region_northAmerica:
      ringer.freq    = 20;
      ringer.cadence = new int[3]{2, 2000, 4000};
      dial.freqs     = new int[3]{2, 350, 440};
      dial.cadence   = new int[1]{0};
      ring.freqs     = new int[3]{2, 440, 480};
      ring.cadence   = new int[3]{2, 2000, 4000};
      busy.freqs     = new int[3]{2, 480, 620};
      busy.cadence   = new int[3]{2, 500, 500};
      break;
  }
}
