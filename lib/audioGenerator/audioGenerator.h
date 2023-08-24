#ifndef audioGenerator_h
#define audioGenerator_h

#include "Arduino.h"
#include "regionConfig.h"

struct toneSegment {
  unsigned short freq1;
  unsigned short freq2;
  unsigned short freq3;
  unsigned short freq4;
  unsigned int duration;
};

struct toneDef {
  byte iterations;
  byte segmentCount;
  toneSegment *segments[20];
};

struct mp3Def {
  String filepath;
  byte iterations;
  unsigned int gapMS;
  unsigned long offsetBytes;
  unsigned long samplesToPlay;
};

void tone_task(void *arg);
void mp3_task(void *arg);
void antipop(short start, short finish);

class audioGenerator
{
  public:
    enum tones {
      dialtone,
      ringback,
      busytone,
      reorder,
      howler,
      zip,
      err
    };

    audioGenerator(RegionConfig region = region_northAmerica, byte targetCore = 0);
    ~audioGenerator();
    void changeRegion(RegionConfig region);
    void playTone(tones tone, byte iterations = 0);
    void playMP3(String filepath, byte iterations = 1, unsigned gapMS = 0, unsigned long offsetBytes = 0, unsigned long samplesToPlay = 0);
    void playDTMF(String digits, unsigned toneTime = 40, unsigned spaceTime = 40);
    void stop();

  private:
    void addSegmentSilent(toneDef* def, unsigned spaceTime);
    void addSegmentDTMF(toneDef* def, char digit, unsigned toneTime);
    void addSegmentsTone(toneDef* def, tones tone);
    void addSegmentsToneConfig(toneDef* def, ToneConfig tc);
};

#endif