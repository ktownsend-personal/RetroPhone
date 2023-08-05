#include "regionConfig.h"

RegionConfig::RegionConfig(regions regionToUse){

  // common settings for all regions
  region         = regionToUse;
  howler.freqs     = new int[5]{4, 1400, 2060, 2450, 2600};
  howler.cadence   = new int[3]{2, 100, 100};
  howler.gain      = 2; // override gain for howler
  zip.freqs      = new int[2]{1, 440};
  zip.cadence    = new int[3]{2, 200, 100};
  err.freqs      = new int[5]{4, 1400, 2060, 2450, 2600};
  err.cadence    = new int[3]{2, 150, 100};
  err.gain       = 2;

  switch(regionToUse){
    case region_unitedKingdom:
      label             = "United Kingdom";
      ringer.freq       = 25;
      ringer.cadence    = new int[5]{4, 400, 200, 400, 2000};
      dialtone.freqs    = new int[3]{2, 350, 450};
      dialtone.cadence  = new int[1]{0};
      ringback.freqs    = new int[3]{2, 400, 450};
      ringback.cadence  = new int[5]{4, 400, 200, 400, 2000};
      busytone.freqs    = new int[2]{1, 400};
      busytone.cadence  = new int[3]{2, 375, 375};
      reorder.freqs     = new int[2]{1, 400};
      reorder.cadence   = new int[5]{4, 400, 350, 225, 525};
      break;
    case region_northAmerica:
      label             = "North America";
      ringer.freq       = 20;
      ringer.cadence    = new int[3]{2, 2000, 4000};
      dialtone.freqs    = new int[3]{2, 350, 440};
      dialtone.cadence  = new int[1]{0};
      ringback.freqs    = new int[3]{2, 440, 480};
      ringback.cadence  = new int[3]{2, 2000, 4000};
      busytone.freqs    = new int[3]{2, 480, 620};
      busytone.cadence  = new int[3]{2, 500, 500};
      reorder.freqs     = new int[3]{2, 480, 620};
      reorder.cadence   = new int[3]{2, 250, 250};
      break;
  }
}
