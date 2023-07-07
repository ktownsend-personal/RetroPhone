#include <regions.h>

RegionConfig::RegionConfig(regions region){
  switch(region){
    case region_unitedKingdom:
      dialFreq    = new int[3]{2, 350, 450};
      dialCadence = new int[1]{0};
      ringFreq    = new int[3]{2, 400, 450};
      ringCadence = new int[5]{4, 400, 200, 400, 2000};
      busyFreq    = new int[3]{1, 400};
      busyCadence = new int[3]{2, 375, 375};
      howlFreq    = new int[5]{4, 1400, 2060, 2450, 2600};
      howlCadence = new int[3]{2, 100, 100};
      break;
    default:
      dialFreq    = new int[3]{2, 350, 440};
      dialCadence = new int[1]{0};
      ringFreq    = new int[3]{2, 440, 480};
      ringCadence = new int[3]{2, 2000, 4000};
      busyFreq    = new int[3]{2, 480, 620};
      busyCadence = new int[3]{2, 500, 500};
      howlFreq    = new int[5]{4, 1400, 2060, 2450, 2600};
      howlCadence = new int[3]{2, 100, 100};
      break;
  }
}
