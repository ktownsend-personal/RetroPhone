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
  byte iterations;
  unsigned int gapTime;
  String filepath;
  unsigned long offset;
  unsigned long segment;
};

void tone_task(void *arg);
void mp3_task(void *arg);
void antipop(short lastSample);

class audioGenerator
{
  public:
    enum tones {
      dialtone,
      ringing,
      busytone,
      howler,
      zip,
      err
    };

    audioGenerator(RegionConfig region = region_northAmerica, byte targetCore = 0);
    ~audioGenerator();
    void changeRegion(RegionConfig region);
    void playTone(tones tone, byte iterations = 0);
    void playMP3(String filepath, byte iterations = 1, unsigned gapTime = 0, unsigned long offset = 0, unsigned long segment = 0);
    void playDTMF(String digits, unsigned toneTime = 40, unsigned spaceTime = 40);
    void stop();

  private:
    void addSegmentSilent(toneDef* def, unsigned spaceTime);
    void addSegmentDTMF(toneDef* def, char digit, unsigned toneTime);
    void addSegmentsTone(toneDef* def, tones tone);
    void addSegmentsToneConfig(toneDef* def, ToneConfig tc);
};

#endif