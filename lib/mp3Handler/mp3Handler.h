#ifndef mp3Handler_h
#define mp3Handler_h

#include "Arduino.h"

void mp3_task(void *arg);
void mp3_start(String filepath);
void mp3_stop();
void mp3_tone_start();

#endif