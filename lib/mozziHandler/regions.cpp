#include <regions.h>

RegionConfig::RegionConfig(regions region){
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
      howl.freqs     = new int[5]{4, 1400, 2060, 2450, 2600};
      howl.cadence   = new int[3]{2, 100, 100};
      howl.gain      = 2; // override gain for howler
      zip.freqs      = new int[2]{1, 440};
      zip.cadence    = new int[3]{2, 100, 100};
      break;
    default:
      ringer.freq    = 20;
      ringer.cadence = new int[3]{2, 2000, 4000};
      dial.freqs     = new int[3]{2, 350, 440};
      dial.cadence   = new int[1]{0};
      ring.freqs     = new int[3]{2, 440, 480};
      ring.cadence   = new int[3]{2, 2000, 4000};
      busy.freqs     = new int[3]{2, 480, 620};
      busy.cadence   = new int[3]{2, 500, 500};
      howl.freqs     = new int[5]{4, 1400, 2060, 2450, 2600};
      howl.cadence   = new int[3]{2, 100, 100};
      howl.gain      = 2; // override gain for howler
      zip.freqs      = new int[2]{1, 440};
      zip.cadence    = new int[3]{2, 100, 100};
      break;
  }
}
