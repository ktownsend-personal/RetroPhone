#ifndef main_h
#define main_h

#include "Arduino.h"
#include "progressModes.h"

modes mode = call_idle;                       // current mode
modes previousMode = call_idle;               // previous mode
modes deferredMode;                           // mode to activate when deferredMode expires unless mode changes first
unsigned long deferModeUntil = 0;             // milliseconds until deferred mode should be activated; cleared if mode changes first
void (*deferAction)();                        // action to run when deferredAction expires unless mode changes first
unsigned long deferActionUntil = 0;           // milliseconds until deferred action should be executed; cleared if mode changes first
modes lastDebounceValue = call_idle;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 80;       // 80ms minimum to ignore pulse dialing on snoopy phone, but 70ms was ok for my sony phone
unsigned long timeoutStarted = 0;
const unsigned long timeout1Max = 1000 * 15;  // how long to wait for user action
const unsigned long timeout2Max = 1000 * 40;  // cumulative with timeout1Max; enough time to play message twice and then eventually give up and play howler
const unsigned long abandonMax = 1000 * 120;  // cumulative with timeout2Max; when to give up on user and stop howler

void settingsInit();
void existsDTMF_module();
bool testDTMF_module(int toneTime, int gapTime, bool showSend = false, bool ignoreSHK = false);
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
void maybeDialingStartedCallback();
void dialingStartedCallback(bool isTone);
void digitReceivedCallback(char);
void configureByNumber(String starcode);
void callFailed();

#endif