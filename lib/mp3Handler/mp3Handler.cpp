#include "mp3Handler.h"
#include "spiffs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "DACOutput.h"
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_STDIO
#include "minimp3.h"

const int BUFFER_SIZE = 1024;

void play_task(void *param)
{
  Output *output = new DACOutput();
  // create the file system
  SPIFFS spiffs("/fs");
  // setup for the mp3 decoded
  short *pcm = (short *)malloc(sizeof(short) * MINIMP3_MAX_SAMPLES_PER_FRAME);
  uint8_t *input_buf = (uint8_t *)malloc(BUFFER_SIZE);
  if (!pcm) ESP_LOGE("main", "Failed to allocate pcm memory");
  if (!input_buf) ESP_LOGE("main", "Failed to allocate input_buf memory");
  // files to play
  String files[] = {
    "timeout-bell-f1.mp3",
    "circuits-bell-f1.mp3", 
    "complete2-bell-f1.mp3", 
    "discoornis-bell-f1.mp3", 
  };
  //TODO: why is only the first file playing audibly, and the rest seem to play but are silent? Always first audible regardless of order. 
  //TODO: try removing array and using a switch on iteration to open the files
  for(auto file : files){
    // mp3 decoder state
    mp3dec_t mp3d = {};
    mp3dec_init(&mp3d);
    mp3dec_frame_info_t info = {};
    // keep track of how much data we have buffered, need to read and decoded
    int to_read = BUFFER_SIZE;
    int buffered = 0;
    int decoded = 0;
    bool is_output_started = false;
    // this assumes that you have uploaded the mp3 file to the SPIFFS
    auto filename = String("/fs/") + file;
    FILE *fp = fopen(filename.c_str(), "r");
    if (!fp) {
      ESP_LOGE("main", "Failed to open file %s", filename.c_str());
      continue;
    };
    while (1) {
      size_t n = fread(input_buf + buffered, 1, to_read, fp); // read in the data that is needed to top up the buffer
      vTaskDelay(pdMS_TO_TICKS(1));                           // feed the watchdog
      buffered += n;
      if (buffered == 0) {
        // we've reached the end of the file and processed all the buffered data
        output->stop();
        is_output_started = false;
        break;
      }
      int samples = mp3dec_decode_frame(&mp3d, input_buf, buffered, pcm, &info);  // decode the next frame
      buffered -= info.frame_bytes;                                               // we've processed this may bytes from teh buffered data
      memmove(input_buf, input_buf + info.frame_bytes, buffered);                 // shift the remaining data to the front of the buffer
      to_read = info.frame_bytes;                                                 // we need to top up the buffer from the file
      if (samples > 0) {
        // if we haven't started the output yet we can do it now as we now know the sample rate and number of channels
        if (!is_output_started) {
          output->start(info.hz);
          is_output_started = true;
          Serial.printf("%s: %dHz, %d bitrate, %d channels, %d frame bytes\n", filename.c_str(), info.hz, info.bitrate_kbps, info.channels, info.frame_bytes);
        }
        // if we've decoded a frame of mono samples convert it to stereo by duplicating the left channel
        // we can do this in place as our samples buffer has enough space
        if (info.channels == 1) {
          for (int i = samples - 1; i >= 0; i--) {
            pcm[i * 2] = pcm[i];
            pcm[i * 2 - 1] = pcm[i];
          }
        }
        output->write(pcm, samples);  // write the decoded samples to the output
        decoded += samples;           // keep track of how many samples we've decoded
      }
    }
    fclose(fp);
  }
  Serial.println("Done testing MP3 playback.");
  vTaskSuspend(NULL); // stop the work //TODO: deallocate memory and terminate instead of suspending
}

void testmp3()
{
  xTaskCreatePinnedToCore(play_task, "task", 32768, NULL, 1, NULL, 0);
}