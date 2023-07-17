#ifndef progressModes_h
#define progressModes_h

#include "Arduino.h"

enum modes {
  call_idle,          // waiting for incoming or lift handset
  call_ready,         // play dialtone; listen for dialing to start
  call_pulse_dialing, // listen for pulse dialing
  call_tone_dialing,  // listen for tone dialing
  call_connecting,    // negotiate path to remote
  call_busy,          // play busy tone
  call_fail,          // play "number not in service" message
  call_ringing,       // play ringing tone
  call_connected,     // establish two-way live audio between phones
  call_disconnected,  // detect end of call and start timeout
  call_timeout1,      // play "cannot be completed as dialed" message
  call_timeout2,      // play howler
  call_abandoned,     // silence; we gave up alerting user (maybe go into low power mode?)
  call_incoming,      // physical ringer
  call_operator,      // simulate calling the operator
  system_config       // configuration mode from dialing star-codes
};

String modeNames[16] = {  // best way I found to translate modes to names for output
  "phone idle", 
  "ready to dial", 
  "pulse dialing", 
  "tone dialing",
  "call connecting", 
  "destination busy",
  "call failed",
  "destination ringing",
  "call connected",
  "call disconnected",
  "timeout1",
  "timeout2",
  "call abandoned",
  "incoming call",
  "calling operator",
  "configuring"
};

modes mode = call_idle;
modes deferredMode;
unsigned long deferModeUntil = 0;
modes lastDebounceValue = call_idle;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 80;       // 80ms minimum to ignore pulse dialing on snoopy phone, but 70ms was ok for my sony phone
unsigned long timeoutStarted = 0;
const unsigned long timeout1Max = 1000 * 30;  // how long to wait for user action
const unsigned long timeout2Max = 1000 * 60;  // cumulative with timeout1Max; enough time to play message twice and then eventually give up and play howler
const unsigned long abandonMax = 1000 * 120;  // cumulative with timeout2Max; when to give up on user and stop howler

void settingsInit();
void testDTMF(String digits, int toneTime, int gapTime);
bool modeBouncing(modes);
void modeDefer(modes deferMode, unsigned delay);
void modeDeferCheck();
modes modeDetect();
void modeStop(modes);
void modeStart(modes);
void modeRun(modes);
void modeGo(modes);
void timeoutStart();
void timeoutCheck();
void ringCountCallback(int);
void dialingStartedCallback(bool isTone);
void digitReceivedCallback(char);
void configureByNumber(String starcode);
#endif