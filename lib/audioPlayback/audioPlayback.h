#ifndef audioPlayback_h
#define audioPlayback_h

#include "Arduino.h"
#include "regionConfig.h"
#include "audioSlices.h"

struct playbackDef {
  //-------callback-------//
  void (*callback)();
  //---------tone---------//
  unsigned int duration;        // 0 = forever, otherwise milliseconds
  unsigned short freq1;
  unsigned short freq2;
  unsigned short freq3;
  unsigned short freq4;
  //---------mp3----------//
  String filepath;
  unsigned long offsetBytes;    // 0 = start
  unsigned long samplesToPlay;  // 0 = until end
  //--------repeat--------//
  short repeatIndex;            // any preceding index in the queue
  unsigned repeatTimes;         // 0 = forever, otherwise exact number of times
};

struct playbackQueue {
  byte iterations;              // 0 = entire queue forever, otherwise exact number of times
  short arraySize;
  playbackDef defs[128];
  bool stopping;
  short lastSample;
  bool debug;
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

    enum infotones {
      low_short,
      low_long,
      high_short,
      high_long
    };

    audioPlayback(RegionConfig region = region_northAmerica, byte targetCore = 0);
    ~audioPlayback();

    void changeRegion(RegionConfig region);
    void queueCallback(void (*callback)());
    void queueGap(unsigned spaceTime);
    void queueMP3(String filepath, byte iterations = 1, unsigned gapMS = 0);
    void queueTone(tones tone, unsigned iterations = 0);
    void queueTone(unsigned toneTime, unsigned short freq1, unsigned short freq2, unsigned short freq3, unsigned short freq4);
    void queueDTMF(String digits, unsigned toneTime = 40, unsigned spaceTime = 40);
    void queueSlice(sliceConfig slice);
    void queueNumber(String digits, unsigned spaceTime = 0);
    void queueInfoTones(infotones first, infotones second, infotones third);

    void playTone(tones tone, byte iterations = 0);
    void playMP3(String filepath, byte iterations = 1, unsigned gapMS = 0, unsigned long offsetBytes = 0, unsigned long samplesToPlay = 0);
    void playDTMF(String digits, unsigned toneTime = 40, unsigned spaceTime = 40);

    void play(byte iterations = 1, unsigned gapMS = 0, bool showDebug = false);
    void await(bool (*cancelIf)() = NULL);
    void stop();

  private:
    playbackQueue *pq;
    void queueDTMF(char digit, unsigned toneTime);
    void queueMP3Slice(String filepath, unsigned long offsetBytes = 0, unsigned long samplesToPlay = 0);
    void queueToneConfig(ToneConfig tc, unsigned iterations);
    void queuePlaybackDef(playbackDef def);
};

#endif