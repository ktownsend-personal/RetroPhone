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

RegionConfig mozziRegion(region_northAmerica); // regional tone defs
byte tone_repeat_times = 0;                    // active tone times to repeat
int tone_gain = 1;                             // active tone gain multiplier
bool tone_cadence_on = false;                  // active tone cadence on/off
int *tone_cadence_timings;                     // active tone cadence timings; NULL to stop
byte tone_cadence_index = 0;                   // active tone cadence timing index
unsigned long tone_cadence_until;              // active tone cadence timing expiration
bool sample_playing = false;                   // whether a sound sample is being played or not
byte sample_index = 0;                         // active sample segment index
byte sample_repeat_times = 1;                  // active sample times to play
byte sample_repeat_count = 0;                  // active sample number of times played
unsigned sample_repeat_gap = 0;                // active sample gap between repeat plays
unsigned long sample_repeat_next_start = 0;    // active sample time to play next after gap

// audio osc generators
// Oscil <table_size, update_rate> oscilName (wavetable)
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> tone1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> tone2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> tone3(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> tone4(SIN2048_DATA);

//TODO: refactor to allow more recordings with any number of samples; probably move to separate class to manage samples, or add to regions.h if using different recording per region

// use: Sample <table_size, update_rate> SampleName (wavetable)
Sample <DialAgain1_NUM_CELLS, AUDIO_RATE> sample1(DialAgain1_DATA);
Sample <DialAgain2_NUM_CELLS, AUDIO_RATE> sample2(DialAgain2_DATA);
Sample <DialAgain3_NUM_CELLS, AUDIO_RATE> sample3(DialAgain3_DATA);

// mozzi function to update controls
void updateControl() {
  // put any mozzi changing controls in here
}

// mozzi function to generate the audio sample to send to the audio output pin
AudioOutput_t updateAudio() {

  if (tone_cadence_on) {
    // create an audio signal by summing 4 oscillators and scaling the resulting signal to fit into an 8 bit format
    return MonoOutput::from8Bit(((tone1.next() + tone2.next() + tone3.next() + tone4.next()) >> 3) * tone_gain);
  }

  if (sample_playing) {
    switch(sample_index){
      case 1:
        return MonoOutput::from8Bit(sample1.next());
      case 2:
        return MonoOutput::from8Bit(sample2.next());
      case 3:
        return MonoOutput::from8Bit(sample3.next());
    }
  }

  return 0;
}

mozziHandler::mozziHandler(regions region)
{
  mozziRegion = RegionConfig(region);

  // init mozzi synth oscillators, starting with 0 Hz (no output)
  startMozzi(CONTROL_RATE);
  tone1.setFreq(0);
  tone2.setFreq(0);
  tone3.setFreq(0);
  tone4.setFreq(0);

  sample1.setFreq((float) DialAgain1_SAMPLERATE / (float) DialAgain1_NUM_CELLS); // play at the speed it was recorded
  sample2.setFreq((float) DialAgain2_SAMPLERATE / (float) DialAgain2_NUM_CELLS); // play at the speed it was recorded
  sample3.setFreq((float) DialAgain3_SAMPLERATE / (float) DialAgain3_NUM_CELLS); // play at the speed it was recorded
}

void mozziHandler::changeRegion(regions region){
  mozziRegion = RegionConfig(region);
}

RegionConfig mozziHandler::currentRegion(){
  return mozziRegion;
}

void mozziHandler::run() {
  audioHook(); // handle mozzi operations periodically

  // skip cadence logic until it's transitioning to minimize overhead accessing pointers
  if (tone_cadence_timings != NULL && tone_cadence_index > -1 && millis() > tone_cadence_until) {
    tone_cadence_index++;
    if (tone_cadence_index > tone_cadence_timings[0]) {
      // restart cadence cycle
      tone_cadence_index = 1;
      // manage repeats; 0 = forever, 1 or more is exact repeats
      if(tone_repeat_times == 1) {
        tone_cadence_on = false;
        return;
      }else if(tone_repeat_times > 1) {
        tone_repeat_times--;
      }
    }
    tone_cadence_until = millis() + tone_cadence_timings[tone_cadence_index];
    tone_cadence_on = tone_cadence_index % 2 == 1;
  }

  // rotate active sample as each one completes
  if (sample_playing) {
    if (sample_index == 0 && sample_repeat_next_start < millis()){
      sample_index = 1;
      sample1.start();
    }
    if (sample_index == 1 && !sample1.isPlaying()) {
      sample_index = 2;
      sample2.start();
    }
    if (sample_index == 2 && !sample2.isPlaying()) {
      sample_index = 3;
      sample3.start();
    }
    if (sample_index == 3 && !sample3.isPlaying()) {
      sample_index = 0;
      sample_repeat_count++;
      if(sample_repeat_times > sample_repeat_count) 
        sample_repeat_next_start = millis() + sample_repeat_gap;
      else
        sample_playing = false;
    }
  }
}

void mozziHandler::stop(){
  toneStop();
}

void mozziHandler::playTone(tones tone, byte repeat){
  toneStop(); // ensure anything currently playing is stopped first
  switch(tone){
    case dialtone:
      return toneStart(mozziRegion.dial);
    case ringing:
      return toneStart(mozziRegion.ring);
    case busytone:
      return toneStart(mozziRegion.busy);
    case howler:
      return toneStart(mozziRegion.howl);
    case zip: 
      return toneStart(mozziRegion.zip, repeat);
  }
}

void mozziHandler::playSample(samples sample, byte repeat, unsigned gapTime) {
  toneStop(); // ensure anything currently playing is stopped first
  switch(sample){
    case dialAgain:
      return messageDialAgain(repeat, gapTime);
  }
}

void mozziHandler::messageDialAgain(byte repeat, unsigned gapTime) {
  sample_repeat_times = repeat;
  sample_repeat_gap = gapTime;
  sample_repeat_count = 0;
  sample_playing = true;
  sample_index = 1;
  sample1.start();
}

void mozziHandler::toneStart(ToneConfig tc, byte repeat){
  byte freqCount = tc.freqs[0];
  if(freqCount >= 1) tone1.setFreq(tc.freqs[1]);
  if(freqCount >= 2) tone2.setFreq(tc.freqs[2]);
  if(freqCount >= 3) tone3.setFreq(tc.freqs[3]);
  if(freqCount >= 4) tone4.setFreq(tc.freqs[4]);

  tone_repeat_times = repeat;
  tone_cadence_timings = tc.cadence;  // copy to variable for speed in loop (dereferencing pointers crashes randomly)
  tone_gain = tc.gain;                // copy to variable for speed in loop
  tone_cadence_on = true;             // first cadence is always on
  if(tc.cadence[0] < 1){              // first index is the count of cadence timings (required to be 0 when no cadence)
    tone_cadence_index = -1;          // -1 skips cadence handling logic in run()
  } else {
    tone_cadence_index = 1;           // first cadence index (position 0 is the count, so start at 1)
    tone_cadence_until = millis() + tc.cadence[1]; // pre-calculating expiration avoids having to do the math on every cycle
  };
}

void mozziHandler::toneStop()  {
  tone_cadence_timings = NULL;
  tone_cadence_on = false;
  sample_playing = false;
  tone1.setFreq(0);
  tone2.setFreq(0);
  tone3.setFreq(0);
  tone4.setFreq(0);
}
