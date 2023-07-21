#include "mp3Handler.h"
#include "SPIFFS.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "DACOutput.h"
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_STDIO
#include "minimp3.h"

const int BUFFER_SIZE = 1024;

/* NOTE: if we want to use *real* I2S instead of onboard DAC:
    // refer to original example at https://github.com/atomic14/esp32-play-mp3-demo
    - add #include "I2SOutput.h"
    - change "output = new DACOutput()" to new I2SOutput(i2s_port_t, i2s_pin_config_t) 
    - set direction and level on the SD pin for your DAC if needed
*/

TaskHandle_t x_handle = NULL;

void mp3_task(void *arg){
  static mp3dec_t mp3d;                         // this gets populated by mp3 decoder on each read
  mp3dec_frame_info_t info;                     // this gets populated by mp3 decoder on each read
  unsigned char buf[BUFFER_SIZE];
  int nbuf = 0;
  SPIFFS spiffs = SPIFFS("/fs");                // open the file system
  String* filepath = (String *)arg;             // convert void arg to the right type
  FILE *fp = fopen(filepath->c_str(), "r");     // open the file
  Output *output = new DACOutput();             // this seems to be using I2S to write to onboard DAC... I need to dig deeper to understand it
  int to_read = BUFFER_SIZE;                    // first read is full buffer size; subsequent reads vary by how much is decoded
  bool is_output_started = false;

  do {
    if(ulTaskNotifyTake(pdTRUE, 0)) break;      // we've been told to stop
    vTaskDelay(pdMS_TO_TICKS(1));               // feed the watchdog
    nbuf += fread(buf + nbuf, 1, to_read, fp);  // read from file into buffer
    if (nbuf == 0) break;                       // end of file
    short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    int samples = mp3dec_decode_frame(&mp3d, buf, nbuf, pcm, &info);  // decode mp3 data from buffer
    nbuf -= info.frame_bytes;                   // adjust buffer count by how much was decoded
    to_read = info.frame_bytes;                 // adjust how much to read from file on next cycle
    memmove(buf, buf + info.frame_bytes, nbuf); // shift the remaining data to the front of the buffer
    if(samples){
      if (!is_output_started) {                 // start output stream when we start getting samples from decoder
        output->start(info.hz);
        is_output_started = true;
      }
      if (info.channels == 1) {                 // if mono make stereo by copying left channel
        for (int i = samples - 1; i >= 0; i--) {
          pcm[i * 2] = pcm[i];
          pcm[i * 2 - 1] = pcm[i];
        }
      }
      output->write(pcm, samples);              // write the decoded samples to the output
    }
  } while(info.frame_bytes);                    // we technically break out before this happens

  //cleanup everything and terminate
  fclose(fp);
  output->stop();
  fp = NULL;
  spiffs.~SPIFFS();
  output->~Output();
  output = NULL;
  filepath = NULL;
  x_handle = NULL;
  Serial.print("Stopping mp3 task (self)");
  vTaskDelete(NULL);
}

// plays the file on core 0 as a task
void mp3_start(String filepath){
  static String f = filepath;
  xTaskCreatePinnedToCore(mp3_task, "test2", 32768, (void*)&f, 1, &x_handle, 1);
}

// gracefully notifies the task to stop and cleanup
void mp3_stop(){
  if(x_handle == NULL) return;
  xTaskNotifyGive(x_handle); // tell the task to stop
}