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

RegionConfig dtmfRegion(region_northAmerica); // regional tone defs
int tone_gain = 1;                            // active tone gain multiplier
bool tone_cadence_on = false;                 // active tone cadence on/off
int *tone_cadence_timings;                    // active tone cadence timings; NULL to stop
byte tone_cadence_index = 0;                  // active tone cadence timing index
unsigned long tone_cadence_until;             // active tone cadence timing expiration
bool sample_playing = false;                  // whether a sound sample is being played or not
byte sample_index = 0;

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
    //TODO: >>3 gives a nice audio level in general, but howler probably should be >>2 for higher volume; >>1 causes howler to distort badly.
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
  dtmfRegion = RegionConfig(region);

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

void mozziHandler::run() {
  audioHook(); // handle mozzi operations periodically

  // skip cadence logic until it's transitioning to minimize overhead accessing pointers
  if (tone_cadence_timings != NULL && tone_cadence_index > -1 && millis() > tone_cadence_until) {
    tone_cadence_index++;
    if (tone_cadence_index > tone_cadence_timings[0]) tone_cadence_index = 1;
    tone_cadence_until = millis() + tone_cadence_timings[tone_cadence_index];
    tone_cadence_on = tone_cadence_index % 2 == 1;
  }

  // rotate active sample as each one completes
  //TODO: we will need to add a configurable count for repeats and gap duration in between
  if (sample_playing) {
    if (sample_index == 1 && !sample1.isPlaying()) {
      sample_index = 2;
      sample2.start();
    }
    if (sample_index == 2 && !sample2.isPlaying()) {
      sample_index = 3;
      sample3.start();
    }
    if (sample_index == 3 && !sample3.isPlaying()) {
      sample_index = 1;
      sample1.start();
    }
  }
}

void mozziHandler::play(tones tone){
  switch(tone){
    case dialtone:
      return toneStart(dtmfRegion.dial);
    case ringing:
      return toneStart(dtmfRegion.ring);
    case busytone:
      return toneStart(dtmfRegion.busy);
    case howler:
      return toneStart(dtmfRegion.howl);
    case dialAgain:
      return messageDialAgain();
    default:
      return toneStop();
  }
}

void mozziHandler::messageDialAgain() {
  sample_playing = true;
  sample_index = 1;
  sample1.start();
}

void mozziHandler::toneStart(ToneConfig tc){
  byte freqCount = tc.freqs[0];
  if(freqCount >= 1) tone1.setFreq(tc.freqs[1]);
  if(freqCount >= 2) tone2.setFreq(tc.freqs[2]);
  if(freqCount >= 3) tone3.setFreq(tc.freqs[3]);
  if(freqCount >= 4) tone4.setFreq(tc.freqs[4]);

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
