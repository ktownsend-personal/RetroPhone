#include "Arduino.h"
#include "audioPlayback.h"
#include "audioSlices.h"
#include "regionConfig.h"
#include "SPIFFS.h"
#include "DACOutput.h"
#include "Oscil.h"                // oscillator template class (copied from Mozzi)
#include "sin2048_int8.h"         // sine table for oscillator (copied from Mozzi)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// must define these before including minimp3.h
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_STDIO
#include "minimp3.h"

#define AUDIO_RATE 32768 // default rate for ESP32
#define CHUNK 576 // for tone generation; 17.58ms of samples based on AUDIO_RATE of 32768; copied what mp3 decoder chunks at but there might be a better value (mp3 decoder uses 576 for 22050Hz or 1152 for 44100Hz, which is 22.122ms)

//NOTE: I2S buffer overrun: 
//      Adding to buffer times out and skips whatever didn't fit unless portMAX_DELAY given to i2s_write and then code blocks until all samples are written.
//      This project uses portMAX_DELAY to avoid overrun.

//NOTE: I2S buffer underrun: 
//      I2S repeats the buffer indefinitely until new samples arrive unless i2s_config.tx_desc_auto_clear is true, then it drops level to -128 (0v output).
//      This project uses i2s_config.tx_desc_auto_clear = false and stuffs the buffer with zeros when done feeding samples to avoid popping from instant transitions to/from 0v.

//SOLVED: popping between howler segments was resolved by setting phase to 0 on the oscillators before starting the segment so the silence segments aren't flatlining whatever the last sample was
//SOLVED: the extra blip of dialtone when hanging up appears to be happening in the SLIC and is only noticeable because we are playing SLIC output on external speaker

//TODO: implement a way to notify main loop that a sequence is done playing
//        maybe an anonymous callback function provided when starting playback so we can do specific things as needed, but threadsafe? Maybe just a flag the loop can watch?
//        maybe enhance wait(ms) in main.cpp to be more of a defer(ms, callback) that blocks until mode change or audio completion (should mode change abort callback?)

const int BUFFER_SIZE = 1024;
byte core;
TaskHandle_t x_handle = NULL;
Output *output = new DACOutput();                               // this uses I2S to write to onboard DAC; we could use external DAC if we use I2SOutput.h
RegionConfig currentRegion(region_northAmerica);                // regional tone defs; default to North America until told otherwise
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> t1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> t2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> t3(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> t4(SIN2048_DATA);
unsigned long currentPlaybackRate;

/* NOTE: if we want to use *real* I2S instead of onboard DAC:
    // refer to original example at https://github.com/atomic14/esp32-play-mp3-demo
    - add #include "I2SOutput.h"
    - change "output = new DACOutput()" to new I2SOutput(i2s_port_t, i2s_pin_config_t) 
    - you will need to define i2s_port_t and i2s_pin_config_t values as found in original example's config.h
    - set direction and level on the SD pin for your DAC if needed
*/

// plays tones of up to 4 frequencies, DTMF tones and MP3 files in flash file system
audioPlayback::audioPlayback(RegionConfig region, byte targetCore)
{
  currentRegion = region;
  core = targetCore;
  pq = new playbackQueue();
  // output->start(AUDIO_RATE); // start with rate we use for tones, but it can change when playing MP3's
}

// cleanup I2S
audioPlayback::~audioPlayback(){
  if(!output) return;
  output->~Output();
  output = NULL;
}

// change the active region for generated sounds
void audioPlayback::changeRegion(RegionConfig region){
  currentRegion = region;
}

// starts playback of the queue for the given number of iterations (0 = infinite)
void audioPlayback::play(byte iterations, unsigned gapMS){
  stop();                           // stop active playback, if any
  while(x_handle != NULL){          // wait for active task to stop (necessary due to both tasks trying to share static queue variable; new task simply ends if old task still active)
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  bool showDebug = false;
  if(gapMS > 0) queueGap(gapMS);

  // clone the queue for the task to use because memory is fickle
  static playbackQueue queue;
  queue.iterations = iterations;
  queue.arraySize = pq->arraySize;
  queue.stopping = false;
  queue.lastSample = 0;
  if(showDebug) Serial.printf("\n\tsequence{len:%d,times:%d}", queue.arraySize, queue.iterations);
  for(int i = 0; i < pq->arraySize; i++){
    auto def = pq->defs[i];
    queue.defs[i] = def;
    if(showDebug) {
      Serial.printf("\n\tdefs[%d]: ", i);
      if(def.repeatTimes != 1) {
        auto times = def.repeatTimes == 0 ? String("forever") : String(def.repeatTimes) + String(" times");
        Serial.printf("repeat{idx:%d,%s}", def.repeatIndex, times.c_str());
      } else if (def.filepath != "") {
        Serial.printf("mp3{%s,offset:%d,samples:%d}", def.filepath.c_str(), def.offsetBytes, def.samplesToPlay);
      } else if (def.filepath == "") {
        auto duration = def.duration == 0 ? String("forever") : String(def.duration) + String("ms");
        Serial.printf("tone{%s,%dHz,%dHz,%dHz,%dHz}", duration.c_str(), def.freq1, def.freq2, def.freq3, def.freq4);
      }
    }
  }

  pq->arraySize = 0; // reset source queue for next sequence config

  xTaskCreatePinnedToCore(playback_task, "playback", 32768, &queue, 1, &x_handle, core);
}

// gracefully notifies the active audio generation task to stop and cleanup
void audioPlayback::stop(){
  if(x_handle == NULL) return;
  xTaskNotifyGive(x_handle);  // tell the task to stop
}

// add playback def to the queue
void audioPlayback::queuePlaybackDef(playbackDef def){
  auto index = pq->arraySize++;
  pq->defs[index]= def;
}

// add silence gap
void audioPlayback::queueGap(unsigned spaceTime){
  queueTone(spaceTime, 0, 0, 0, 0);
}

// add multifreq tone
void audioPlayback::queueTone(unsigned toneTime, unsigned short freq1, unsigned short freq2, unsigned short freq3, unsigned short freq4){
  auto def = playbackDef{toneTime, freq1, freq2, freq3, freq4, "", 0, 0, 0, 1};
  queuePlaybackDef(def);
}

// add mp3
void audioPlayback::queueMP3(String filepath, unsigned long offsetBytes, unsigned long samplesToPlay){
  auto def = playbackDef{0, 0, 0, 0, 0, filepath, offsetBytes, samplesToPlay, 0, 1};
  queuePlaybackDef(def);
}

// add DTMF digits
void audioPlayback::queueDTMF(String digits, unsigned toneTime, unsigned spaceTime){

  /*
    TODO: saw this about dBm levels here: https://people.ece.ubc.ca/edc/4550.fall2017/lec2.pdf
    • DTMF high-frequency tones are sent at a level of -4 to -9 dBm and lower-frequency tones are sent at a level about 2 dB lower
    • dial tone is a continuous tone having frequencies of 350 and 440 Hz at a level of −13 dBm
    • ringback tone is defined as comprising frequencies of 440 and 480 Hz at a level of −19 dBm and a cadence of 2 seconds ON and 4 seconds OFF
    • busy tone is defined as having frequency components of 480 and 620 Hz at a level of −24 dBm and a cadence of half a second ON and half a second OFF
    • reorder tone, also called “fast busy” tone, contains the same frequency components as busy tone at a similar level but with a cadence of 0.25 of a second on and 0.25 of a second off
  */

  for(auto digit : digits){
    queueDTMF(digit, toneTime);
    if(spaceTime > 0) queueGap(spaceTime);
  }
}

// add DTMF digit
void audioPlayback::queueDTMF(char digit, unsigned toneTime){

  // DTMF keypad frequencies (Hz)
  //     1209 1336 1477 1633
  // 697   1    2    3    A
  // 770   4    5    6    B
  // 852   7    8    9    C
  // 941   *    0    #    D

  switch(digit){
    case '1': return queueTone(toneTime, 697, 1209, 0, 0);
    case '2': return queueTone(toneTime, 697, 1336, 0, 0);
    case '3': return queueTone(toneTime, 697, 1477, 0, 0);
    case 'A': return queueTone(toneTime, 697, 1633, 0, 0);
    case '4': return queueTone(toneTime, 770, 1209, 0, 0);
    case '5': return queueTone(toneTime, 770, 1336, 0, 0);
    case '6': return queueTone(toneTime, 770, 1477, 0, 0);
    case 'B': return queueTone(toneTime, 770, 1633, 0, 0);
    case '7': return queueTone(toneTime, 852, 1209, 0, 0);
    case '8': return queueTone(toneTime, 852, 1336, 0, 0);
    case '9': return queueTone(toneTime, 852, 1477, 0, 0);
    case 'C': return queueTone(toneTime, 852, 1633, 0, 0);
    case '*': return queueTone(toneTime, 941, 1209, 0, 0);
    case '0': return queueTone(toneTime, 941, 1336, 0, 0);
    case '#': return queueTone(toneTime, 941, 1477, 0, 0);
    case 'D': return queueTone(toneTime, 941, 1633, 0, 0);
  }
}

// add appropriate segments for the specified tone type and time
void audioPlayback::queueTone(tones tone, byte iterations){
  switch(tone){
    case dialtone:  return queueToneConfig(currentRegion.dialtone, iterations);
    case ringback:  return queueToneConfig(currentRegion.ringback, iterations);
    case busytone:  return queueToneConfig(currentRegion.busytone, iterations);
    case reorder:   return queueToneConfig(currentRegion.reorder,  iterations);
    case howler:    return queueToneConfig(currentRegion.howler,   iterations);
    case zip:       return queueToneConfig(currentRegion.zip,      iterations);
    case err:       return queueToneConfig(currentRegion.err,      iterations);
  }
}

// convert ToneConfig to segments
void audioPlayback::queueToneConfig(ToneConfig tc, byte iterations) {

  // NOTE: first index of tc.cadence[] and tc.freqs[] is the count of elements following
  // NOTE: when cadence count is zero then freqs are played forever (no cadence)
  // NOTE: any cadence with duration zero is played forever

  auto repeatIndex = pq->arraySize;           // if we have 0 or > 1 iterations we need to add a repeat segment after adding the tone segments

  auto cadenceCount = tc.cadence[0];
  for(int x = 0; x <= cadenceCount; x++){
    if(x == 0 && cadenceCount != 0) continue; // only do 0th index when cadence count is zero
    if(x > 0 && (x % 2) == 0){       
      queueGap(tc.cadence[x]);
      continue;
    }
    auto fcount = tc.freqs[0];
    queueTone(
      tc.cadence[x],                          // if no cadence then duration is 0 (forever); when x==0 then cadence[x]==0 otherwise we don't get here for x==0
      (fcount > 0) ? tc.freqs[1] : 0,         // zero if not defined
      (fcount > 1) ? tc.freqs[2] : 0,         // zero if not defined
      (fcount > 2) ? tc.freqs[3] : 0,         // zero if not defined
      (fcount > 3) ? tc.freqs[4] : 0          // zero if not defined
    );
    if(cadenceCount == 0) return;             // done adding segments if no cadence
  }

  if(iterations != 1) {
    auto def = playbackDef{0, 0, 0, 0, 0, "", 0, 0, repeatIndex, iterations};
    queuePlaybackDef(def);
  }
}

void audioPlayback::queueSlice(sliceConfig slice){
  queueMP3(slice.filename, slice.byteOffset, slice.samplesToPlay);
}

void audioPlayback::queueNumber(String digits, unsigned spaceTime){

  sliceConfigs slices;
  
  for(auto digit : digits){
    switch(digit){
      case '0': queueSlice(slices.zero);   break;
      case '1': queueSlice(slices.one);    break;
      case '2': queueSlice(slices.two);    break;
      case '3': queueSlice(slices.three);  break;
      case '4': queueSlice(slices.four);   break;
      case '5': queueSlice(slices.five);   break;
      case '6': queueSlice(slices.six);    break;
      case '7': queueSlice(slices.seven);  break;
      case '8': queueSlice(slices.eight);  break;
      case '9': queueSlice(slices.nine);   break;
    }
    if(spaceTime > 0) queueGap(spaceTime);
  }
}

// plays the tone type as a task
void audioPlayback::playTone(tones tone, byte iterations){
  queueTone(tone, iterations);
  play();
}

// plays the DTMF digits as a task
void audioPlayback::playDTMF(String digits, unsigned toneTime, unsigned spaceTime){
  queueDTMF(digits, toneTime, spaceTime);
  play(1);
}

// plays the MP3 file as a task
void audioPlayback::playMP3(String filepath, byte iterations, unsigned gapMS, unsigned long offsetBytes, unsigned long samplesToPlay){
  queueMP3(filepath, offsetBytes, samplesToPlay);
  play(iterations, gapMS);
}

void playback_task(void *arg){
  auto queue = (playbackQueue*)arg;
  
  if(currentPlaybackRate == 0){                                 // initialize output at first playback
    currentPlaybackRate = AUDIO_RATE;
    output->start(AUDIO_RATE);                                  // start with rate we use for tones, but it can change when playing MP3's
    antipop(-128, 0, 500);                                      // trying to soften the I2S/DAC startup pop
    antipop(0, 0, 1024*4);                                      // stuff buffer with silence so it stays at level 0 instead of 0v during I2S/DAC startup
  }

  do {                                                          // loop iterations
    for(byte idx = 0; idx < queue->arraySize; idx++){           // loop the queue
      auto def = queue->defs[idx];
      if(def.repeatTimes != 1 && !queue->stopping) {            // check repeater
        idx = def.repeatIndex - 1;                              // go back to starting def of repeat sequence
        continue;
      }
      if(def.filepath == "") playback_tone(queue, def);         // play the tone segment
      if(def.filepath != "") playback_mp3(queue, def);          // play the mp3 segment
    }
  } while(queue->iterations-- != 1 && !queue->stopping);        // stop on last iteration (if iterations started at 0 then loop until notified to stop)

  antipop(0, 0, 1024*4);                                        // stuff buffer with silence so I2S doesn't repeat remnants
  if(x_handle == xTaskGetCurrentTaskHandle()) x_handle = NULL;  // clear our own task handle if not already cleared or replaced
  vTaskDelete(NULL);                                            // self-delete the task when done
}

void playback_tone(playbackQueue *pq, playbackDef tone){

  auto gap = (0 == tone.freq1 + tone.freq2 + tone.freq3 + tone.freq4);

  if(!gap){
    // starting at phase 0 makes the tones predictable and avoids sharp transition at start
    t1.setFreq(tone.freq1); t1.setPhase(0);
    t2.setFreq(tone.freq2); t2.setPhase(0);
    t3.setFreq(tone.freq3); t3.setPhase(0);
    t4.setFreq(tone.freq4); t4.setPhase(0);
  }

  //NOTE: be careful setting this when there is still audio in the buffer; even if rate is the same you will get a skip in the audio
  if(currentPlaybackRate != AUDIO_RATE) {
    output->set_frequency(AUDIO_RATE);
    currentPlaybackRate = AUDIO_RATE;
  }

  unsigned long samples = CHUNK;
  if(tone.duration > 0) samples = (unsigned long)AUDIO_RATE / (float)((float)1000/(float)tone.duration); // calculate number of samples to achieve duration

  while(samples > 0 && !pq->stopping) {                       // generate the samples in chunks
    auto toGenerate = (samples > CHUNK) ? CHUNK : samples;    // determine chunk size
    if(tone.duration != 0) samples -= toGenerate;             // only decrement chunk size if not an infinite segment
    short pcm[toGenerate*2] = {0};                            // this holds the samples we want to give to I2S; all elements initialized to 0 for gap scenario
    if(!gap){
      for(int x = 0; x < toGenerate; x++){                    // fill frame buffer with samples
        auto sum = t1.next()+t2.next()+t3.next()+t4.next();   // sum the next sample from all oscillators together
        auto sample = (sum >> 3) << 8;                        // bitshift 3 to reduce amplitude (because we summed 4 oscillators), then bitshift 8 to upper 8 bits (onboard DAC only reads upper 8 bits)
        pcm[x*2]   = sample;                                  // left channel at even index
        pcm[x*2+1] = sample;                                  // right channel at odd index
      }
    }
    output->write(pcm, toGenerate);                           // write the tone samples to the output
    pq->lastSample = pcm[(toGenerate-1)*2];                   // track last sample to feed antipopFinish() when we finish
    vTaskDelay(pdMS_TO_TICKS(1));                             // feed the watchdog
    pq->stopping |= ulTaskNotifyTake(pdTRUE, 0);              // check if told to stop
  };
  //TODO: can we soften the tail of a tone back to zero? Antipop ramp caused audible artifact. Maybe try fading volume over the last samples loop after the buffer is filled?
}

void playback_mp3(playbackQueue *pq, playbackDef mp3){

  SPIFFS spiffs = SPIFFS("/fs");                    // open the file system
  auto fp = fopen(mp3.filepath.c_str(), "rb");      // open the file
  if(!fp) {
    spiffs.~SPIFFS();
    Serial.printf("%s not found", mp3.filepath.c_str());
    return;
  }

  fseek(fp, mp3.offsetBytes, SEEK_SET);             // jump ahead if appropriate

  static mp3dec_t mp3d;                             // this gets populated by mp3 decoder on each read
  mp3dec_frame_info_t info;                         // this gets populated by mp3 decoder on each read
  unsigned char buf[BUFFER_SIZE];
  int nbuf = 0;
  int to_read = BUFFER_SIZE;                        // first read is full buffer size; subsequent reads vary by how much is decoded
  bool is_output_started = false;
  bool lastIteration = false;
  unsigned long samples_cutoff = 0;
  unsigned long samples_played = 0;
  unsigned long bytes_played = 0;

  do {
    pq->stopping |= ulTaskNotifyTake(pdTRUE, 0);    // flag if we've been told to stop
    auto bytepos = ftell(fp);                       // the byte we read from; used later to identify where music started
    nbuf += fread(buf + nbuf, 1, to_read, fp);      // read from file into buffer
    if (nbuf == 0) break;                           // end of file
    short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];       // this will receive the decoded data for output
    int samples = mp3dec_decode_frame(&mp3d, buf, nbuf, pcm, &info);  // decode mp3 data from buffer
    nbuf -= info.frame_bytes;                       // adjust buffer count by how much was decoded
    to_read = info.frame_bytes;                     // adjust how much to read from file on next cycle
    memmove(buf, buf + info.frame_bytes, nbuf);     // shift the remaining data to the front of the buffer
    if(samples){
      if (!is_output_started) {                     // start output stream when we start getting samples from decoder
        is_output_started = true;
        if(mp3.samplesToPlay > 0) samples_cutoff = mp3.samplesToPlay; // put a cap on how many samples to play
        //NOTE: be careful setting this when there is still audio in the buffer; even if rate is the same you will get a skip in the audio
        if(currentPlaybackRate != info.hz) {
          output->set_frequency(info.hz);
          currentPlaybackRate = info.hz;
        }
      }
      if(samples_cutoff > 0 && (samples_played + samples) > samples_cutoff) {
        samples = (samples_cutoff - samples_played);// only send what we need if we reached end of specified segment
        lastIteration = true;                       // flag to stop after this iteration
      }
      if (info.channels == 1) {                     // if mono make stereo by copying left channel
        for (int i = samples - 1; i >= 0; i--) {
          pcm[i*2  ] = pcm[i];
          pcm[i*2-1] = pcm[i];
        }
      }
      output->write(pcm, samples);                  // write the decoded samples to the output
      samples_played += samples;                    // track how many samples have been output so we know when we hit our cutoff
      bytes_played += info.frame_bytes;             // tracking how many bytes of audio we play for manual slicing calculations
      pq->lastSample = pcm[(samples-1)*2];          // track last sample to feed antipopFinish() when we finish
    }
    vTaskDelay(pdMS_TO_TICKS(1));                   // feed the watchdog
  } while(info.frame_bytes && !pq->stopping && !lastIteration);

  //cleanup
  if(mp3.samplesToPlay > 0) antipop(0, 0, 1024*4);  // filling buffer with silence resolves stuttering on slices by giving time for overhead of seeking to next slice
  if(fp) fclose(fp);
  fp = NULL;
  spiffs.~SPIFFS();
}

// use this to create a ramp to soften transitions, or stuff buffer with zeros when done feeding audio to ensure it idles silently
void antipop(short start, short finish, short len){
  short ramp[len*2];
  float step = (start - finish)/(float)len;
  for(int i = 0; i < len; i++){
    short sample = (start-(short)((i*step))) << 8;
    ramp[i*2  ] = sample;
    ramp[i*2+1] = sample;
  }
  output->write(ramp, len);
}
