#include "Arduino.h"
#include "audioGenerator.h"
#include "regionConfig.h"
#include "Oscil.h"                // oscillator template class from Mozzi
#include "tables/sin2048_int8.h"  // sine table for oscillator from Mozzi
#include "SPIFFS.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "DACOutput.h"

// must define these before including minimp3.h
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_STDIO
#include "minimp3.h"

#define CHUNK 576 // this works out to 17.58ms of samples based on AUDIO_RATE of 32768; copied what mp3 decoder chunks at but there might be a better value

//QUESTION: what does I2S do if we are feeding samples faster than they are played? It times out and skips whatever didn't fit.
//QUESTION: what does I2S do if we stop feeding samples? Does it repeat the current buffer indefinitely?

//TODO: what is causing the clicking when the task exits? or is it when DAC finishes samples? Are transitions too abrupt? Do we need constant zeros feeding DAC? Look at signal on scope.
//SOLVED: what is causing the extra blip of dialtone when hanging up? It's the SLIC briefly interrupting audio on the output (we are listening to SLIC out, not ESP32 out)
//TODO: implement playback-sequencing, such as different mp3 segments and/or tones + mp3 (how can we have different config structs in a list? C++ seems difficult)
//TODO: implement a way to notify main loop that a sequence is done playing
//        maybe an anonymous callback function provided when starting playback so we can do specific things as needed, but threadsafe? Maybe just a flag the loop can watch?
//        maybe enhance wait(ms) in main.cpp to be more of a defer(ms, callback) that blocks until mode change or audio completion (should mode change abort callback?)

const int BUFFER_SIZE = 1024;
byte core;
TaskHandle_t x_handle = NULL;
Output *output = new DACOutput();                           // this uses I2S to write to onboard DAC; we could use external DAC if we use I2SOutput.h
RegionConfig mozziRegion(region_northAmerica);              // regional tone defs; default to North America until told otherwise
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> t1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> t2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> t3(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> t4(SIN2048_DATA);

/* NOTE: if we want to use *real* I2S instead of onboard DAC:
    // refer to original example at https://github.com/atomic14/esp32-play-mp3-demo
    - add #include "I2SOutput.h"
    - change "output = new DACOutput()" to new I2SOutput(i2s_port_t, i2s_pin_config_t) 
    - you will need to define i2s_port_t and i2s_pin_config_t values as found in original example
    - set direction and level on the SD pin for your DAC if needed
*/

// plays tones of up to 4 frequencies, DTMF tones and MP3 files in flash file system
audioGenerator::audioGenerator(RegionConfig region, byte targetCore)
{
  mozziRegion = region;
  core = targetCore;
  output->start(AUDIO_RATE); // start with Mozzi's AUDIO_RATE macro, but it can change when playing MP3's
}

audioGenerator::~audioGenerator(){
  if(!output) return;
  output->stop();
  output->~Output();
  output = NULL;
}

// change the active region for generated sounds
void audioGenerator::changeRegion(RegionConfig region){
  mozziRegion = region;
}

// gracefully notifies the active audio generation task to stop and cleanup
void audioGenerator::stop(){
  if(x_handle == NULL) return;
  xTaskNotifyGive(x_handle); // tell the task to stop
  x_handle = NULL;           // clear the handle
}

// plays the tone type as a task
void audioGenerator::playTone(tones tone, byte iterations){
  if(x_handle != NULL) stop();

  static toneDef td;
  td.iterations = iterations;
  td.segmentCount = 0;
  addSegmentsTone(&td, tone);

  xTaskCreatePinnedToCore(tone_task, "tone", 32768, (void*)&td, 1, &x_handle, core);
}

// plays the DTMF digits as a task
void audioGenerator::playDTMF(String digits, unsigned toneTime, unsigned spaceTime){
  if(x_handle != NULL) stop();

  static toneDef td;
  td.iterations = 1;
  td.segmentCount = 0;
  for(auto digit : digits){
    addSegmentDTMF(&td, digit, toneTime);
    if(spaceTime > 0) addSegmentSilent(&td, spaceTime);
  }

  xTaskCreatePinnedToCore(tone_task, "dtmf", 32768, (void*)&td, 1, &x_handle, core);
}

// plays the MP3 file as a task
void audioGenerator::playMP3(String filepath, byte iterations, unsigned gapTime, unsigned long offset, unsigned long segment){
  if(x_handle != NULL) stop();

  static mp3Def d;
  d.filepath = filepath;
  d.iterations = iterations;
  d.gapTime = gapTime;
  d.offset = offset;
  d.segment = segment;

  xTaskCreatePinnedToCore(mp3_task, "mp3", 32768, (void*)&d, 1, &x_handle, core);
}

// add appropriate segment for silence at the right index and increment segment count
void audioGenerator::addSegmentSilent(toneDef* def, unsigned spaceTime){
  auto index = def->segmentCount++;
  if(!def->segments[index]) def->segments[index] = new toneSegment();
  def->segments[index]->duration = spaceTime;
  def->segments[index]->freq1 = 0;
  def->segments[index]->freq2 = 0;
  def->segments[index]->freq3 = 0;
  def->segments[index]->freq4 = 0;
}

// add appropriate segment for the specified DTMF digit and time
void audioGenerator::addSegmentDTMF(toneDef* def, char digit, unsigned toneTime){

  // DTMF keypad frequencies (Hz)
  //     1209 1336 1477 1633
  // 697   1    2    3    A
  // 770   4    5    6    B
  // 852   7    8    9    C
  // 941   *    0    #    D

  auto index = def->segmentCount++;
  if(!def->segments[index]) def->segments[index] = new toneSegment();
  def->segments[index]->duration = toneTime;
  def->segments[index]->freq3 = 0;
  def->segments[index]->freq4 = 0;

  switch(digit){
    case '1':
      def->segments[index]->freq1 = 697;
      def->segments[index]->freq2 = 1209;
      break;
    case '2':
      def->segments[index]->freq1 = 697;
      def->segments[index]->freq2 = 1336;
      break;
    case '3':
      def->segments[index]->freq1 = 697;
      def->segments[index]->freq2 = 1477;
      break;
    case 'A':
      def->segments[index]->freq1 = 697;
      def->segments[index]->freq2 = 1633;
      break;
    case '4':
      def->segments[index]->freq1 = 770;
      def->segments[index]->freq2 = 1209;
      break;
    case '5':
      def->segments[index]->freq1 = 770;
      def->segments[index]->freq2 = 1336;
      break;
    case '6':
      def->segments[index]->freq1 = 770;
      def->segments[index]->freq2 = 1477;
      break;
    case 'B':
      def->segments[index]->freq1 = 770;
      def->segments[index]->freq2 = 1633;
      break;
    case '7':
      def->segments[index]->freq1 = 852;
      def->segments[index]->freq2 = 1209;
      break;
    case '8':
      def->segments[index]->freq1 = 852;
      def->segments[index]->freq2 = 1336;
      break;
    case '9':
      def->segments[index]->freq1 = 852;
      def->segments[index]->freq2 = 1477;
      break;
    case 'C':
      def->segments[index]->freq1 = 852;
      def->segments[index]->freq2 = 1633;
      break;
    case '*':
      def->segments[index]->freq1 = 941;
      def->segments[index]->freq2 = 1209;
      break;
    case '0':
      def->segments[index]->freq1 = 941;
      def->segments[index]->freq2 = 1336;
      break;
    case '#':
      def->segments[index]->freq1 = 941;
      def->segments[index]->freq2 = 1477;
      break;
    case 'D':
      def->segments[index]->freq1 = 941;
      def->segments[index]->freq2 = 1633;
      break;
  }
}

// add appropriate segments for the specified tone type and time
void audioGenerator::addSegmentsTone(toneDef* def, tones tone){
  switch(tone){
    case dialtone:
      return addSegmentsToneConfig(def, mozziRegion.dial);
    case ringing:
      return addSegmentsToneConfig(def, mozziRegion.ring);
    case busytone:
      return addSegmentsToneConfig(def, mozziRegion.busy);
    case howler:
      return addSegmentsToneConfig(def, mozziRegion.howl);
    case zip: 
      return addSegmentsToneConfig(def, mozziRegion.zip);
    case err:
      return addSegmentsToneConfig(def, mozziRegion.err);
  }
}

// convert ToneConfig to segments
void audioGenerator::addSegmentsToneConfig(toneDef* def, ToneConfig tc) {

  // NOTE: first index of tc.cadence[] and tc.freqs[] is the count of elements following
  // NOTE: when cadence count is zero then freqs are played forever (no cadence)
  // NOTE: any cadence with duration zero is played forever

  auto cadenceCount = tc.cadence[0];
  for(int x = 0; x <= cadenceCount; x++){
    if(x == 0 && cadenceCount != 0) continue;                     // only do 0th index when cadence count is zero
    if(x > 0 && (x % 2) == 0){       
      addSegmentSilent(def, tc.cadence[x]);
      continue;
    }
    auto fcount = tc.freqs[0];
    auto index = def->segmentCount++;
    if(!def->segments[index]) def->segments[index] = new toneSegment();
    def->segments[index]->duration = tc.cadence[x];               // if no cadence then duration is 0 (forever); when x==0 then cadence[x]==0 otherwise we don't get here for x==0
    def->segments[index]->freq1 = (fcount > 0) ? tc.freqs[1] : 0; // apply frequency 1 if defined
    def->segments[index]->freq2 = (fcount > 1) ? tc.freqs[2] : 0; // apply frequency 2 if defined
    def->segments[index]->freq3 = (fcount > 2) ? tc.freqs[3] : 0; // apply frequency 3 if defined
    def->segments[index]->freq4 = (fcount > 3) ? tc.freqs[4] : 0; // apply frequency 4 if defined
    if(cadenceCount == 0) return;                                 // done adding segments if no cadence
  }
}

void tone_task(void *arg){
  toneDef* d = (toneDef*)arg;                                     // convert void arg to the right type
  bool is_output_started = false;
  bool is_output_ending = false;
  short lastSample;

  do {                                                            // run the iterations
    auto segmentCount = d->segmentCount;                          // segments remaining to run
    do {                                                          // run the segments
      auto segmentIndex = d->segmentCount - segmentCount;         // calculate current segment index
      auto segment = d->segments[segmentIndex];                   // current segment
      unsigned long samples = CHUNK;
      if(segment->duration > 0) samples = (unsigned long)AUDIO_RATE / (float)((float)1000/(float)segment->duration); // calculate number of samples to achieve duration
      t1.setFreq(segment->freq1);
      t2.setFreq(segment->freq2);
      t3.setFreq(segment->freq3);
      t4.setFreq(segment->freq4);
      // starting phase at 0 each time makes the tones and gaps predictable and ensures gaps use 0 level instead of where the last tone left off
      t1.setPhase(0);
      t2.setPhase(0);
      t3.setPhase(0);
      t4.setPhase(0);
      while(samples > 0 && !is_output_ending) {                   // generate the samples in chunks
        short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];                 // this holds the samples we want to give to I2S
        auto toGenerate = (samples > CHUNK) ? CHUNK : samples;    // determine chunk size
        if(segment->duration != 0) samples -= toGenerate;         // only decrement chunk size if not an infinite segment
        for(int x = 0; x < toGenerate; x++){                      // fill frame buffer with samples
          auto sample = MonoOutput::from8Bit(((t1.next() + t2.next() + t3.next() + t4.next()) >> 3)); // generate and sum a single sample from each oscillator
          pcm[x*2] = sample.l() << 8;                             // left channel at every even index, shifted to upper 8 bits of 16 bit value (onboard DAC only reads upper 8 bits)
          pcm[x*2+1] = pcm[x*2];                                  // right channel at every odd index, copy from left channel
        }
        if (!is_output_started) {                                 // start output stream when we start getting samples from oscillators
          is_output_started = true;
          output->set_frequency(AUDIO_RATE);                      // using Mozzi macro AUDIO_RATE = 32768 because it works; not sure what range is valid
          antipopStart();                                         // soften transition from min to zero
        }
        output->write(pcm, toGenerate);                           // write the tone samples to the output
        vTaskDelay(pdMS_TO_TICKS(10));                            // feed the watchdog
        is_output_ending |= ulTaskNotifyTake(pdTRUE, 0);          // check if told to stop
        lastSample = pcm[(toGenerate-1)*2];                       // track last sample to feed antipopFinish() when we finish
      };
    } while(--segmentCount > 0 && !is_output_ending);             // finished segments if decremented count is 0
  } while(d->iterations-- != 1 && !is_output_ending);             // finished iterations if we just did iteration 1 (0=forever)

  // testing to see if this helps with clicks at end of audio (replicate for start if we figure it out)
  // REF: this thread is closest I've found to others having the issue, but not solved: https://github.com/earlephilhower/ESP8266Audio/issues/406
  // REF: this looks interesting... try gradually going to max value instead of min value: https://github.com/earlephilhower/ESP8266Audio/issues/367#issuecomment-1180460464
  // short silence[4096*2] = { };
  // for(int i = 0; i < 4096*2; silence[i++] = 128 << 8);
  // output->write(silence, 4096);
  antipopFinish(lastSample);

  //cleanup and terminate
  if(x_handle == xTaskGetCurrentTaskHandle()) x_handle = NULL; // clear our own task handle if not already cleared or replaced
  vTaskDelete(NULL);
}

void mp3_task(void *arg){
  mp3Def* d = (mp3Def*)arg;                           // convert void arg to the right type
  SPIFFS spiffs = SPIFFS("/fs");                      // open the file system
  FILE *fp;                                           // file handle
  bool is_output_ending = false;
  bool decrement = (d->iterations != 0);
  short lastSample;

  do {
    fp = fopen(d->filepath.c_str(), "rb");            // open the file
    if(!fp) {
      Serial.printf("%s not found", d->filepath.c_str());
      break;
    }
    static mp3dec_t mp3d;                             // this gets populated by mp3 decoder on each read
    mp3dec_frame_info_t info;                         // this gets populated by mp3 decoder on each read
    unsigned char buf[BUFFER_SIZE];
    int nbuf = 0;
    int to_read = BUFFER_SIZE;                        // first read is full buffer size; subsequent reads vary by how much is decoded
    bool is_output_started = false;
    unsigned long samples_cutoff = 0;
    unsigned long samples_played = 0;
    unsigned long bytes_played = 0;

    do {
      is_output_ending |= ulTaskNotifyTake(pdTRUE, 0);// flag if we've been told to stop
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
          output->set_frequency(info.hz);
          is_output_started = true;
          antipopStart();
          // auto filepos = ftell(fp);
          // fseek(fp, 0, SEEK_END);       // move to end of file
          // auto filesize = ftell(fp);    // get position from end of file
          // fseek(fp, filepos, SEEK_SET); // return to where we were
          // Serial.printf("%d samples, %d Hz, %d frame bytes, audio from byte %d to %d\n", samples, info.hz, info.frame_bytes, bytepos, filesize);
          if(d->offset > 0) {                         // jump ahead if appropriate
            if(d->segment > 0) samples_cutoff = d->segment; // put a cap on how many samples to play
            int result = fseek(fp, d->offset, SEEK_SET); // seek to offset position for playing a segment
            // Serial.printf("file size %d, seeking to %d returned %d; before=%d, after=%d\n", filesize, d->offset, result, filepos, ftell(fp));
            continue;
          }
        }
        if(samples_cutoff > 0 && (samples_played + samples) > samples_cutoff) {
          samples = (samples_cutoff - samples_played);// only send what we need if we reached end of specified segment
          is_output_ending = true;                    // flag to stop after this iteration
        }
        if (info.channels == 1) {                     // if mono make stereo by copying left channel
          for (int i = samples - 1; i >= 0; i--) {
            pcm[i * 2] = pcm[i];
            pcm[i * 2 - 1] = pcm[i];
          }
        }
        output->write(pcm, samples);                  // write the decoded samples to the output
        samples_played += samples;                    // track how many samples have been output so we know when we hit our cutoff
        bytes_played += info.frame_bytes;             // tracking how many bytes of audio we play for manual slicing calculations
        lastSample = pcm[(samples-1)*2];              // track last sample to feed antipopFinish() when we finish
      }
      vTaskDelay(pdMS_TO_TICKS(1));                   // feed the watchdog
    } while(info.frame_bytes && !is_output_ending);

    // Serial.printf("%d samples played at %dHz (%f seconds) from %d file bytes\n", samples_played, info.hz, (double)samples_played / (double)info.hz, bytes_played);

    antipopFinish(lastSample);

    fclose(fp);
    fp = NULL;
    if(d->iterations != 1) vTaskDelay(pdMS_TO_TICKS(d->gapTime));     // wait specfied gapTime if not last iteration
  } while((!decrement || d->iterations-- != 1) && !is_output_ending); // iteration 1 is final, or 0 is infinite

  //cleanup and terminate
  if(fp) fclose(fp);
  fp = NULL;
  spiffs.~SPIFFS();
  if(x_handle == xTaskGetCurrentTaskHandle()) x_handle = NULL; // clear our own task handle if not already cleared or replaced
  vTaskDelete(NULL);
}

void antipopStart(){
  short len = 256;
  short ramp[len*2];
  short start = -128;
  short range = 128;
  float step = range/(float)len;
  for(int i = 0; i < len; i++){
    short sample = start+(short)((i*step));
    ramp[i*2] = sample << 8;
    ramp[i*2+1] = sample << 8;
  }
  output->write(ramp, len);
  // Serial.printf("start=%d, range=%d, step=%f, samples=%d, ramp=[%d..%d]\n", start, range, step, len, ramp[0]>>8, ramp[(len-1)*2]>>8);
}

void antipopFinish(short lastSample){
  short len = 256;
  short ramp[len*2];
  short start = lastSample >> 8;
  short range = 128 + start;
  float step = range/(float)len;
  for(int i = 0; i < len; i++){
    short sample = start-(short)((i*step));
    ramp[i*2] = sample << 8;
    ramp[i*2+1] = sample << 8;
  }
  output->write(ramp, len);
  // Serial.printf("lastSample=%d, range=%d, step=%f, samples=%d, ramp=[%d..%d]\n", start, range, step, len, ramp[0]>>8, ramp[(len-1)*2]>>8);
  // for(int i = 0; i < len; i++){
  //   Serial.printf("%d,",ramp[i*2]>>8);
  // }
  // Serial.println("]");
}