#include <Arduino.h>
#include <mozziHandler.h>
#include <regions.h>
#include <MozziGuts.h>
#include <Oscil.h>                // oscillator template
#include <Sample.h>               // sample template
#include <tables/sin2048_int8.h>  // sine table for oscillator
#include <DialAgain1.h>           // audio sampled from https://www.thisisarecording.com/Joyce-Gordon.html
#include <DialAgain2.h>           // and converted to .h files with https://sensorium.github.io/Mozzi/doc/html/char2mozzi_8py.html
#include <DialAgain3.h>

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 64  // Hz, powers of 2 are most reliable

bool _audioOn = false;                        // whether audio output is muted or not (on/off cadence)
bool tonePlaying = false;                     // whether a call progress tone is being played or not
bool samplePlaying = false;                   // whether a sound sample is being played or not
int *dtmf_freq;                               // tone frequencies to add together
int *dtmf_cadence;                            // tone cadence (on/off/on/off durations in mS)
byte dtmf_cadence_step = 0;                   // which part of the call progress tone pattern is in progress
unsigned long dtmf_cadence_timer = millis();  // used for on/off timing intervals of dtmf tones
RegionConfig dtmfRegion(region_northAmerica); // regional tone defs

// audio osc generators
// Oscil <table_size, update_rate> oscilName (wavetable)
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> tone1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> tone2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> tone3(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> tone4(SIN2048_DATA);

//TODO: refactor to allow more recordings with any number of samples; probably move to separate class to manage samples, or add to regions.h if using different recording per region
// use: Sample <table_size, update_rate> SampleName (wavetable)
Sample <DialAgain1_NUM_CELLS, AUDIO_RATE> aSample1(DialAgain1_DATA);
Sample <DialAgain2_NUM_CELLS, AUDIO_RATE> aSample2(DialAgain2_DATA);
Sample <DialAgain3_NUM_CELLS, AUDIO_RATE> aSample3(DialAgain3_DATA);
byte curSample = 0;

// mozzi function to update controls
void updateControl() {
  // put any mozzi changing controls in here
}

// mozzi function to generate the audio sample to send to the audio output pin
AudioOutput_t updateAudio() {

  if (tonePlaying && _audioOn) {
    // create an audio signal by summing 4 oscillators and scaling the resulting signal to fit into an 8 bit format
    return MonoOutput::from8Bit((tone1.next() + tone2.next() + tone3.next() + tone4.next()) >> 3);
    //TODO: >>3 gives a nice audio level in general, but howler probably should be >>2 for higher volume; >>1 causes howler to distort badly.
  }

  if (samplePlaying) {
    switch(curSample){
      case 1:
        return MonoOutput::from8Bit(aSample1.next());
      case 2:
        return MonoOutput::from8Bit(aSample2.next());
      case 3:
        return MonoOutput::from8Bit(aSample3.next());
    }
  }

  return 0;
}

mozziHandler::mozziHandler(regions region)
{
  dtmfRegion = RegionConfig(region);
  _audioOn = false;
  tonePlaying = false;
  samplePlaying = false;

  // init mozzi synth oscillators, starting with 0 Hz (no output)
  startMozzi(CONTROL_RATE);
  tone1.setFreq(0);
  tone2.setFreq(0);
  tone3.setFreq(0);
  tone4.setFreq(0);

  aSample1.setFreq((float) DialAgain1_SAMPLERATE / (float) DialAgain1_NUM_CELLS); // play at the speed it was recorded
  aSample2.setFreq((float) DialAgain2_SAMPLERATE / (float) DialAgain2_NUM_CELLS); // play at the speed it was recorded
  aSample3.setFreq((float) DialAgain3_SAMPLERATE / (float) DialAgain3_NUM_CELLS); // play at the speed it was recorded
}

void mozziHandler::run() {
  audioHook(); // handle mozzi operations periodically

  if (tonePlaying) {
    int cadenceCount = dtmf_cadence[0];
    if (cadenceCount < 1) {
      _audioOn = true;
    } else {
      _audioOn = dtmf_cadence_step % 2 == 0;
      if ((millis() - dtmf_cadence_timer) > dtmf_cadence[dtmf_cadence_step+1]) {
        dtmf_cadence_timer = millis();
        dtmf_cadence_step++;
        if (dtmf_cadence_step >= cadenceCount) dtmf_cadence_step = 0;
      }
    }
  }

  if (samplePlaying) {
    if (curSample == 1 && !aSample1.isPlaying()) {
      curSample = 2;
      aSample2.start();
    }
    if (curSample == 2 && !aSample2.isPlaying()) {
      curSample = 3;
      aSample3.start();
    }
    if (curSample == 3 && !aSample3.isPlaying()) {
      curSample = 1;
      aSample1.start();
    }
  }
}

void mozziHandler::play(tones tone){
  switch(tone){
    case dialtone:
      return toneStart(dtmfRegion.dialFreq, dtmfRegion.dialCadence);
    case ringing:
      return toneStart(dtmfRegion.ringFreq, dtmfRegion.ringCadence);
    case busytone:
      return toneStart(dtmfRegion.busyFreq, dtmfRegion.busyCadence);
    case howler:
      return toneStart(dtmfRegion.howlFreq, dtmfRegion.howlCadence);
    case dialAgain:
      return messageDialAgain();
    default:
      return toneStop();
  }
}

void mozziHandler::messageDialAgain() {
  _audioOn = true;
  samplePlaying = true;
  curSample = 1;
  aSample1.start();
}

void mozziHandler::toneStart(int *freqs, int *cadences){
  byte freqCount = freqs[0];
  if(freqCount >= 1) tone1.setFreq(freqs[1]);
  if(freqCount >= 2) tone2.setFreq(freqs[2]);
  if(freqCount >= 3) tone3.setFreq(freqs[3]);
  if(freqCount >= 4) tone4.setFreq(freqs[4]);

  dtmf_cadence = cadences;
  dtmf_cadence_step = 0;
  dtmf_cadence_timer = millis();
  _audioOn = true;
  tonePlaying = true;
}

void mozziHandler::toneStop()  {
  _audioOn = false;
  tonePlaying = false;
  samplePlaying = false;
  tone1.setFreq(0);
  tone2.setFreq(0);
  tone3.setFreq(0);
  tone4.setFreq(0);
}
