#include "Arduino.h"
#include "mozziHandler.h"
#include "regionConfig.h"
#include "MozziGuts.h"
#include "Oscil.h"                            // oscillator template
#include "Sample.h"                           // sample template
#include "tables/sin2048_int8.h"              // sine table for oscillator
#include "../samples/DialAgain/DialAgain1.h"  // audio sampled from https://www.thisisarecording.com/Joyce-Gordon.html
#include "../samples/DialAgain/DialAgain2.h"  // and converted to .h files with https://sensorium.github.io/Mozzi/doc/html/char2mozzi_8py.html
#include "../samples/DialAgain/DialAgain3.h"  

//TODO: now that we have mp3Handler working, see if we can play tones with it and if so we should merge the two handlers and drop Mozzi

/* NOTE:
    The Mozzi library did not implement mozziStop(), so it hangs onto the I2S to internal DAC forever and that was crashing if we used I2S to internal DAC from mp3Handler.
    I added these two lines to mozziStop() in MozziGuts_impl_ESP32.hpp:
        i2s_stop((i2s_port_t)i2s_num);
        i2s_driver_uninstall((i2s_port_t)i2s_num);
    I also refactored this mozziHandler to ensure we call mozziStart() and mozziStop() at the points of audio playing. This fixed the crash.
*/

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 64  // Hz, powers of 2 are most reliable

RegionConfig mozziRegion(region_northAmerica); // regional tone defs; default to North America until told otherwise
byte tone_iterations = 0;                      // active tone iterations; 0 = forever
int tone_gain = 1;                             // active tone gain multiplier
bool tone_cadence_on = false;                  // active tone cadence on/off
int *tone_cadence_timings;                     // active tone cadence timings; NULL to stop
byte tone_cadence_index = 0;                   // active tone cadence timing index
unsigned long tone_cadence_until;              // active tone cadence timing expiration
bool sample_playing = false;                   // whether a sound sample is being played or not
byte sample_index = 0;                         // active sample segment index
byte sample_iterations = 1;                    // active sample iterations; 0 = forever
unsigned sample_gap = 0;                       // active sample gap between iterations
unsigned long sample_gap_until = 0;            // active sample when to start next iteration after gap

// audio osc generators
// Oscil <table_size, update_rate> oscilName (wavetable)
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> tone1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> tone2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> tone3(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> tone4(SIN2048_DATA);

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

mozziHandler::mozziHandler(RegionConfig region)
{
  mozziRegion = region;

  // init mozzi synth oscillators, starting with 0 Hz (no output)
  tone1.setFreq(0);
  tone2.setFreq(0);
  tone3.setFreq(0);
  tone4.setFreq(0);

  sample1.setFreq((float) DialAgain1_SAMPLERATE / (float) DialAgain1_NUM_CELLS); // play at the speed it was recorded
  sample2.setFreq((float) DialAgain2_SAMPLERATE / (float) DialAgain2_NUM_CELLS); // play at the speed it was recorded
  sample3.setFreq((float) DialAgain3_SAMPLERATE / (float) DialAgain3_NUM_CELLS); // play at the speed it was recorded
}

void mozziHandler::changeRegion(RegionConfig region){
  mozziRegion = region;
}

void mozziHandler::run() {
  if(!isStarted) return;  // short-circuit when stopped
  
  audioHook(); // handle mozzi operations periodically

  // skip cadence logic until it's transitioning to minimize overhead accessing pointers
  if (tone_cadence_timings != NULL && tone_cadence_index > -1 && millis() > tone_cadence_until) {
    tone_cadence_index++;
    if (tone_cadence_index > tone_cadence_timings[0]) {
      // restart cadence cycle
      tone_cadence_index = 1;
      // manage iterations of full cadence cycle; 0 = forever
      if(tone_iterations == 1) {
        tone_cadence_on = false;
        return;
      }else if(tone_iterations > 1) {
        tone_iterations--;
      }
    }
    tone_cadence_until = millis() + tone_cadence_timings[tone_cadence_index];
    tone_cadence_on = tone_cadence_index % 2 == 1;
  }

  // rotate active sample as each one completes
  if (sample_playing) {
    if (sample_index == 0 && sample_gap_until < millis()){
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
      // manage iterations; 0 = forever
      if(sample_iterations == 1) {
        sample_playing = false;
      } else {
        sample_iterations--;
        sample_index = 0;
        sample_gap_until = millis() + sample_gap;
      }
    }
  }
}

void mozziHandler::stop(){
  if(!isStarted) return;

  // force Mozzi to release I2S internal DAC when not playing audio (see NOTE at top of this file about code I had to add to Mozzi for this)
  stopMozzi();
  isStarted = false;

  tone1.setFreq(0);
  tone2.setFreq(0);
  tone3.setFreq(0);
  tone4.setFreq(0);
  tone_cadence_timings = NULL;
  tone_cadence_on = false;
  sample_playing = false;
}

void mozziHandler::start(){
  if(isStarted) return;
  startMozzi(CONTROL_RATE);
  isStarted = true;
}

void mozziHandler::playTone(tones tone, byte iterations){
  stop();   // ensure anything currently playing is stopped first
  start();  // restart Mozzi on I2S internal DAC

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
      return toneStart(mozziRegion.zip, iterations);
    case err:
      return toneStart(mozziRegion.err, iterations);
  }
}

void mozziHandler::playSample(samples sample, byte iterations, unsigned gapTime) {
  stop(); // ensure anything currently playing is stopped first
  start();  // restart Mozzi on I2S internal DAC

  switch(sample){
    case dialAgain:
      return messageDialAgain(iterations, gapTime);
  }
}

void mozziHandler::playDTMF(char digit, int length, int gap) {

  // DTMF keypad frequencies (Hz)
  //     1209 1336 1477 1633
  // 697   1    2    3    A
  // 770   4    5    6    B
  // 852   7    8    9    C
  // 941   *    0    #    D

  auto dtmf_tone = ToneConfig();
  int cadence[3]{2, length, gap};
  dtmf_tone.cadence = cadence;
  int freqs[3]{2, 0, 0};
  dtmf_tone.freqs = freqs;
  switch(digit){
    case '1':
      freqs[1] = 697;
      freqs[2] = 1209;
      break;
    case '2':
      freqs[1] = 697;
      freqs[2] = 1336;
      break;
    case '3':
      freqs[1] = 697;
      freqs[2] = 1477;
      break;
    case 'A':
      freqs[1] = 697;
      freqs[2] = 1633;
      break;
    case '4':
      freqs[1] = 770;
      freqs[2] = 1209;
      break;
    case '5':
      freqs[1] = 770;
      freqs[2] = 1336;
      break;
    case '6':
      freqs[1] = 770;
      freqs[2] = 1477;
      break;
    case 'B':
      freqs[1] = 770;
      freqs[2] = 1633;
      break;
    case '7':
      freqs[1] = 852;
      freqs[2] = 1209;
      break;
    case '8':
      freqs[1] = 852;
      freqs[2] = 1336;
      break;
    case '9':
      freqs[1] = 852;
      freqs[2] = 1477;
      break;
    case 'C':
      freqs[1] = 852;
      freqs[2] = 1633;
      break;
    case '*':
      freqs[1] = 941;
      freqs[2] = 1209;
      break;
    case '0':
      freqs[1] = 941;
      freqs[2] = 1336;
      break;
    case '#':
      freqs[1] = 941;
      freqs[2] = 1477;
      break;
    case 'D':
      freqs[1] = 941;
      freqs[2] = 1633;
      break;
  }

  start();  // restart Mozzi on I2S internal DAC
  toneStart(dtmf_tone, 1);
}

void mozziHandler::messageDialAgain(byte iterations, unsigned gapTime) {
  sample_iterations = iterations;
  sample_gap = gapTime;
  sample_playing = true;
  sample_index = 1;
  sample1.start();
}

void mozziHandler::toneStart(ToneConfig tc, byte iterations){
  byte freqCount = tc.freqs[0];
  if(freqCount >= 1) tone1.setFreq(tc.freqs[1]);
  if(freqCount >= 2) tone2.setFreq(tc.freqs[2]);
  if(freqCount >= 3) tone3.setFreq(tc.freqs[3]);
  if(freqCount >= 4) tone4.setFreq(tc.freqs[4]);

  tone_iterations = iterations;
  tone_cadence_timings = tc.cadence;               // copy to variable for speed in loop (dereferencing pointers crashes randomly)
  tone_gain = tc.gain;                             // copy to variable for speed in loop
  tone_cadence_on = true;                          // first cadence is always on
  if(tc.cadence[0] < 1){                           // first index is the count of cadence timings (required to be 0 when no cadence)
    tone_cadence_index = -1;                       // -1 skips cadence handling logic in run()
  } else {
    tone_cadence_index = 1;                        // first cadence index (position 0 is the count, so start at 1)
    tone_cadence_until = millis() + tc.cadence[1]; // pre-calculating expiration avoids having to do the math on every cycle
  };
}
