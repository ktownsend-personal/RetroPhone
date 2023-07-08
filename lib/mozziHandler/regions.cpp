#include <regions.h>

RegionConfig::RegionConfig(regions region){
  switch(region){
    case region_unitedKingdom:
      dial.freqs   = new int[3]{2, 350, 450};
      dial.cadence = new int[1]{0};
      ring.freqs   = new int[3]{2, 400, 450};
      ring.cadence = new int[5]{4, 400, 200, 400, 2000};
      busy.freqs   = new int[3]{1, 400};
      busy.cadence = new int[3]{2, 375, 375};
      howl.freqs   = new int[5]{4, 1400, 2060, 2450, 2600};
      howl.cadence = new int[3]{2, 100, 100};
      howl.gain    = 2; // override gain for howler
      break;
    default:
      dial.freqs   = new int[3]{2, 350, 440};
      dial.cadence = new int[1]{0};
      ring.freqs   = new int[3]{2, 440, 480};
      ring.cadence = new int[3]{2, 2000, 4000};
      busy.freqs   = new int[3]{2, 480, 620};
      busy.cadence = new int[3]{2, 500, 500};
      howl.freqs   = new int[5]{4, 1400, 2060, 2450, 2600};
      howl.cadence = new int[3]{2, 100, 100};
      howl.gain    = 2; // override gain for howler
      break;
  }
}
