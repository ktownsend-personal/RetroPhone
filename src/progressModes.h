#ifndef progressModes_h
#define progressModes_h

#include <Arduino.h>

enum modes {
  call_idle,
  call_ready,
  call_pulse_dialing,
  call_tone_dialing,
  call_connecting,
  call_busy,
  call_fail,
  call_ringing,
  call_connected,
  call_disconnected,
  call_timeout, //TODO: should we split this to two modes for message vs. howler?
  call_abandoned,
  call_incoming
};

String modeNames[13] = {  // best way I found to translate modes to names for output
  "idle", 
  "ready", 
  "pulse dialing", 
  "tone dialing",
  "connecting", 
  "busy",
  "fail",
  "ringing",
  "connected",
  "disconnected",
  "timeout",
  "abandoned",
  "incoming"
};

modes mode = call_idle;
modes lastDebounceValue = call_idle;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 80; // 80ms minimum to ignore pulse dialing on snoopy phone, but 70ms was ok for my sony phone
unsigned long timeoutStarted = 0;
const unsigned long timeoutMax = 1000 * 30;
const unsigned long abandonMax = 1000 * 60;  // based on same timeoutStarted origin as timeoutMax, so ensure higher than timeoutMax 

modes modeDetect();
bool modeBouncing(modes);
void modeStop(modes);
void modeStart(modes);
void modeRun(modes);
void modeGo(modes);
void timeoutStart();
void timeoutCheck();
void ringCountCallback(int);
void dialingStartedCallback(bool isTone);
void digitReceivedCallback(char);
void startDTMF();
void detectDTMF();
#endif