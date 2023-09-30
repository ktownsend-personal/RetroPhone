#include "Arduino.h"
#include "audioPlayback.h"
#include "audioSlices.h"
#include "regionConfig.h"
#include "spiffs.h"
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

#define AUDIO_RATE 32768 // rate to use for tone generation; 32768 is the default rate for ESP32
#define CHUNK 50 // samples per loop for tone generation; minimum 33 (1ms) for smooth audio on core 0 but other loads on same core may affect, like when we add wifi
                 // math: 33/32768Hz=1.0007ms; for comparison, mp3 decoder chunk sizes: 576/22050Hz=26.122ms, 1152/44100Hz=26.122ms

//NOTE: I2S buffer overrun: 
//      Adding to buffer times out and skips whatever didn't fit unless portMAX_DELAY given to i2s_write and then code blocks until all samples are written.
//      This project uses portMAX_DELAY to avoid overrun.

//NOTE: I2S buffer underrun: 
//      I2S repeats the buffer indefinitely until new samples arrive unless i2s_config.tx_desc_auto_clear is true, then it drops level to -128 (0v output).
//      This project uses i2s_config.tx_desc_auto_clear = false and stuffs the buffer with zeros when done feeding samples to avoid popping from instant transitions to/from 0v.

//SOLVED: popping between howler segments was resolved by setting phase to 0 on the oscillators before starting the segment so the silence segments aren't flatlining whatever the last sample was
//SOLVED: the extra blip of dialtone when hanging up appears to be happening in the SLIC and is only noticeable because we are playing SLIC output on external speaker

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
void audioPlayback::play(byte iterations, unsigned gapMS, bool showDebug){
  stop();                           // stop active playback, if any
  while(x_handle != NULL){          // wait for active task to stop (necessary due to both tasks trying to share static queue variable; new task simply ends if old task still active)
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  if(gapMS > 0) queueGap(gapMS);

  // clone the queue for the task to use because memory is fickle
  static playbackQueue queue;
  queue.iterations = iterations;
  queue.arraySize = pq->arraySize;
  queue.stopping = false;
  queue.lastSample = 0;
  queue.debug = showDebug;
  if(showDebug) Serial.printf("\n\tsequence{len:%d,times:%d}", queue.arraySize, queue.iterations);
  for(int i = 0; i < pq->arraySize; i++){
    auto def = pq->defs[i];
    queue.defs[i] = def;
    if(showDebug) {
      Serial.printf("\n\tdefs[%d]: ", i);
      if(def.callback != NULL) {
        Serial.print("callback");
      } else if(def.repeatTimes != 1) {
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
  if(showDebug) Serial.println();

  pq->arraySize = 0; // reset source queue for next sequence config

  xTaskCreatePinnedToCore(playback_task, "playback", 32768, &queue, 1, &x_handle, core);
}

// wait until current playback is finished
void audioPlayback::await(bool (*cancelIf)()){
  // wait until task stops
  while(x_handle != NULL){
    if(cancelIf != NULL && cancelIf()) break;
    vTaskDelay(pdMS_TO_TICKS(10));
  }
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

void audioPlayback::queueCallback(void (*callback)()){
  //NOTE: if using this in the middle of playback to change mode, pass true for 2nd arg of modeGo() to avoid stopping playback
  auto def = playbackDef{callback, 0, 0, 0, 0, 0, "", 0, 0, 0, 1};
  queuePlaybackDef(def);
}

// add silence gap
void audioPlayback::queueGap(unsigned spaceTime){
  queueTone(spaceTime, 0, 0, 0, 0);
}

// add multifreq tone
void audioPlayback::queueTone(unsigned toneTime, unsigned short freq1, unsigned short freq2, unsigned short freq3, unsigned short freq4){
  auto def = playbackDef{NULL, toneTime, freq1, freq2, freq3, freq4, "", 0, 0, 0, 1};
  queuePlaybackDef(def);
}

// add mp3 file
void audioPlayback::queueMP3(String filepath, byte iterations, unsigned gapMS){

  auto repeatIndex = pq->arraySize;           // if we have 0 or > 1 iterations we need to add a repeat segment after adding the tone segments

  auto def = playbackDef{NULL, 0, 0, 0, 0, 0, filepath, 0, 0, 0, 1};
  queuePlaybackDef(def);

  if(iterations != 1) {
    if(gapMS > 0) queueGap(gapMS);
    auto def = playbackDef{NULL, 0, 0, 0, 0, 0, "", 0, 0, repeatIndex, iterations};
    queuePlaybackDef(def);
  }
}

// add mp3 slice
void audioPlayback::queueMP3Slice(String filepath, unsigned long offsetBytes, unsigned long samplesToPlay){
  auto def = playbackDef{NULL, 0, 0, 0, 0, 0, filepath, offsetBytes, samplesToPlay, 0, 1};
  queuePlaybackDef(def);
}

// add DTMF digits
void audioPlayback::queueDTMF(String digits, unsigned toneTime, unsigned spaceTime){

  /* TODO: saw this about dBm levels here: https://people.ece.ubc.ca/edc/4550.fall2017/lec2.pdf
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
void audioPlayback::queueTone(tones tone, unsigned iterations){
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
void audioPlayback::queueToneConfig(ToneConfig tc, unsigned iterations) {

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
    auto def = playbackDef{NULL, 0, 0, 0, 0, 0, "", 0, 0, repeatIndex, iterations};
    queuePlaybackDef(def);
  }
}

void audioPlayback::queueSlice(sliceConfig slice){
  queueMP3Slice(slice.filename, slice.byteOffset, slice.samplesToPlay);
}

void audioPlayback::queueNumber(String digits, unsigned spaceTime){
  for(auto digit : digits){
    switch(digit){
      case '0': queueSlice(SLICES.zero);   break;
      case '1': queueSlice(SLICES.one);    break;
      case '2': queueSlice(SLICES.two);    break;
      case '3': queueSlice(SLICES.three);  break;
      case '4': queueSlice(SLICES.four);   break;
      case '5': queueSlice(SLICES.five);   break;
      case '6': queueSlice(SLICES.six);    break;
      case '7': queueSlice(SLICES.seven);  break;
      case '8': queueSlice(SLICES.eight);  break;
      case '9': queueSlice(SLICES.nine);   break;
    }
    if(spaceTime > 0) queueGap(spaceTime);
  }
}

void audioPlayback::queueInfoTones(infotones first, infotones second, infotones third){

  const int short_tone = 276;
  const int long_tone = 380;
  const int low_tone[] = {914, 1371, 1777};
  const int high_tone[] = {985, 1428, 0};    // third tone is always low, but just in case someone tries to specify high tone we will use 0Hz

  infotones tones[] = {first, second, third};
  for(int x = 0; x < 3; x++){
    switch(tones[x]){
      case low_short:
        queueTone(short_tone, low_tone[x], 0, 0, 0);
        break;
      case low_long:
        queueTone(long_tone, low_tone[x], 0, 0, 0);
        break;
      case high_short:
        queueTone(short_tone, high_tone[x], 0, 0, 0);
        break;
      case high_long:
        queueTone(long_tone, high_tone[x], 0, 0, 0);
        break;
    }
  }

  // NOTE:  The spec allows 100ms max between tones and message, but recommends as close to 0ms as possible...however we
  //        need 125ms minimum to avoid challenges with mp3 changing playback speed before buffer finishes playing tone.
  // TODO:  Figure out a way to sync playback speed change with buffer completing previous audio so we can reduce the gap.
  queueGap(125);

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
  queueMP3Slice(filepath, offsetBytes, samplesToPlay);
  play(iterations, gapMS);
}

void playback_task(void *arg){
  auto queue = (playbackQueue*)arg;
  auto debug = queue->debug;
  
  if(currentPlaybackRate == 0){                                 // initialize output at first playback
    if(debug) Serial.println("Initializing audio");
    currentPlaybackRate = AUDIO_RATE;
    output->start(AUDIO_RATE);                                  // start with rate we use for tones, but it can change when playing MP3's
    antipop(-128, 0, 500);                                      // trying to soften the I2S/DAC startup pop
    antipop(0, 0, 1024*4);                                      // stuff buffer with silence so it stays at level 0 instead of 0v during I2S/DAC startup
  }

  do {                                                          // loop iterations
    for(byte idx = 0; idx < queue->arraySize; idx++){           // loop the queue
      if(queue->stopping) break;                                // abort if task is stopping
      if(debug) Serial.print(".");
      auto def = queue->defs[idx];
      if(def.callback != NULL) {
        def.callback();                                         // do the callback
      } else if(def.repeatTimes != 1) {                         // check repeater
        queue->defs[idx].repeatTimes -= 1;                      // decrement remaining repeats before changing idx
        idx = def.repeatIndex - 1;                              // go back to starting def of repeat sequence
        if(debug) Serial.printf("Repeat %d times from index %d\n", def.repeatTimes, def.repeatIndex);
      } else if(def.filepath != "") {
        playback_mp3(queue, def);                               // play the mp3 segment
      } else if(def.duration + def.freq1 > 0) {
        playback_tone(queue, def);                              // play the tone segment
      }
    }
  } while(queue->iterations-- != 1 && !queue->stopping);        // stop on last iteration (if iterations started at 0 then loop until notified to stop)

  antipop(0, 0, 1024*4);                                        // stuff buffer with silence so I2S doesn't repeat remnants
  if(x_handle == xTaskGetCurrentTaskHandle()) x_handle = NULL;  // clear our own task handle if not already cleared or replaced
  vTaskDelete(NULL);                                            // self-delete the task when done
}

void playback_tone(playbackQueue *pq, playbackDef tone){

  auto gap = (0 == tone.freq1 + tone.freq2 + tone.freq3 + tone.freq4);
  byte volshift = 0;
  auto debug = pq->debug;
  if(debug) Serial.printf("%s for %dms\n", gap?"gap":"tone", tone.duration);

  if(!gap){
    // starting at phase 0 makes the tones predictable and avoids sharp transition at start
    t1.setFreq(tone.freq1); t1.setPhase(0);
    t2.setFreq(tone.freq2); t2.setPhase(0);
    t3.setFreq(tone.freq3); t3.setPhase(0);
    t4.setFreq(tone.freq4); t4.setPhase(0);

    // NOTE: be careful setting this when there is still audio in the buffer because the change is immediate
    // NOTE: don't set playback rate if it didn't change because that will cause a skip in playback
    // NOTE: don't change playback rate for silence because it will make previous audio in buffer change speed; silence doesn't care what the rate is as long as we calculate the duration based on current speed
    // TODO: would it be useful to add a delay equal to the buffer play time when changing playback rate? Would need to do this for both tone and mp3 playback. The extra audio gap might be undesirable in some circumstances...need to experiment.
    if(currentPlaybackRate != AUDIO_RATE) {
      if(debug) Serial.printf("changing audio rate from %d to %d\n", currentPlaybackRate, AUDIO_RATE);
      output->set_frequency(AUDIO_RATE);
      currentPlaybackRate = AUDIO_RATE;
    }

    // dynamic volshift based on how many frequencies are getting mixed
    if(tone.freq1 > 0) volshift = 1;
    if(tone.freq2 > 0) volshift = 3;
    if(tone.freq3 > 0) volshift = 3;
    if(tone.freq4 > 0) volshift = 3;
  }

  unsigned long samples = CHUNK;
  if(tone.duration > 0) samples = currentPlaybackRate * tone.duration * 0.001; // calculate number of samples to achieve duration

  while(samples > 0 && !pq->stopping) {                       // generate the samples in chunks
    auto toGenerate = (samples > CHUNK) ? CHUNK : samples;    // determine chunk size
    if(tone.duration != 0) samples -= toGenerate;             // only decrement chunk size if not an infinite segment
    short pcm[toGenerate*2] = {0};                            // this holds the samples we want to give to I2S; all elements initialized to 0 for gap scenario
    if(!gap){
      for(int x = 0; x < toGenerate; x++){                    // fill frame buffer with samples
        auto sum = t1.next()+t2.next()+t3.next()+t4.next();   // sum the next sample from all oscillators together
        auto sample = (sum >> volshift) << 8;                 // bitshift to upper 8 bits (onboard DAC only reads upper 8 bits)
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
  auto debug = pq->debug;
  if(debug) Serial.printf("playing mp3 %s\n", mp3.filepath.c_str());

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
  unsigned long cycles_buffered = 0;

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
        if(debug) Serial.println("beginning mp3 audio");
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

      //NOTE: be careful setting this when there is still audio in the buffer; even if rate is the same you will get a skip in the audio
      //NOTE: doing this after writing bytes to buffer to see if we can shorten the timing since setting frequency is immediate
      //NOTE: we used to do this check inside if(!is_output_started){}, which is logically correct but we are strugging with timing vs. previous audio still playing
      if(currentPlaybackRate != info.hz) {
        if(debug) Serial.printf("changing audio rate from %d to %d\n", currentPlaybackRate, info.hz);
        output->set_frequency(info.hz);
        currentPlaybackRate = info.hz;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1));                   // feed the watchdog
  } while(info.frame_bytes && !pq->stopping && !lastIteration);

  if(debug) Serial.println("ending mp3 audio");

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
