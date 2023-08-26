#ifndef audioPlayback_h
#define audioPlayback_h

#include "Arduino.h"
#include "regionConfig.h"
#include "audioSlices.h"

struct playbackDef {
  //---------tone---------//
  unsigned int duration;
  unsigned short freq1;
  unsigned short freq2;
  unsigned short freq3;
  unsigned short freq4;
  //---------mp3----------//
  String filepath;
  unsigned long offsetBytes;
  unsigned long samplesToPlay;
  //--------repeat--------//
  byte repeatIndex;
  byte repeatTimes;
};

struct playbackQueue {
  byte iterations;
  short arraySize;
  playbackDef defs[128];
  bool stopping;
  short lastSample;
};

void playback_task(void *arg);
void playback_tone(playbackQueue *pq, playbackDef tone);
void playback_mp3(playbackQueue *pq, playbackDef mp3);
void antipop(short start, short finish, short len = 256);

class audioPlayback
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

    audioPlayback(RegionConfig region = region_northAmerica, byte targetCore = 0);
    ~audioPlayback();

    void changeRegion(RegionConfig region);

    void queueGap(unsigned spaceTime);
    void queueMP3(String filepath, unsigned long offsetBytes = 0, unsigned long samplesToPlay = 0);
    void queueTone(tones tone, byte iterations = 0);
    void queueTone(unsigned toneTime, unsigned short freq1, unsigned short freq2, unsigned short freq3, unsigned short freq4);
    void queueDTMF(String digits, unsigned toneTime = 40, unsigned spaceTime = 40);
    void queueSlice(sliceConfig slice);
    void queueNumber(String digits, unsigned spaceTime = 0);

    void playTone(tones tone, byte iterations = 0);
    void playMP3(String filepath, byte iterations = 1, unsigned gapMS = 0, unsigned long offsetBytes = 0, unsigned long samplesToPlay = 0);
    void playDTMF(String digits, unsigned toneTime = 40, unsigned spaceTime = 40);

    void play(byte iterations = 1, unsigned gapMS = 0);
    void stop();

  private:
    playbackQueue *pq;
    void queueDTMF(char digit, unsigned toneTime);
    void queueToneConfig(ToneConfig tc, byte iterations);
    void queuePlaybackDef(playbackDef def);
};

#endif