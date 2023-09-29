#pragma once

#include "Arduino.h"
#include "progressModes.h"

modes mode = call_idle;                       // current mode
modes deferredMode;                           // mode to activate when deferredMode expires unless mode changes first
unsigned long deferModeUntil = 0;             // milliseconds until deferred mode should be activated; cleared if mode changes first
void (*deferAction)();                        // action to run when deferredAction expires unless mode changes first
unsigned long deferActionUntil = 0;           // milliseconds until deferred action should be executed; cleared if mode changes first
modes lastDebounceValue = call_idle;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 80;       // 80ms minimum to ignore pulse dialing on snoopy phone, but 70ms was ok for my sony phone
const unsigned long timeoutMax = 1000 * 15;   // how long to wait for user action

void splash();
void settingsInit();
void moduleDTMF_exists();
bool moduleDTMF_test(String digits, int toneTime, int gapTime, bool showSend = false, bool ignoreSHK = false);
modes modeDetect();
bool modeBouncing(modes);
void modeDefer(modes deferMode, unsigned delay);
void modeDeferCheck();
void modeGo(modes newmode, bool keepPlaybackAlive = false);
void modeStop(modes oldmode, bool keepPlaybackAlive = false);
void modeStart(modes newmode, modes oldmode);
void modeRun(modes mode);
void timeoutStart();
void ringCountCallback(int);
void maybeDialingStartedCallback();
void dialingStartedCallback(bool isTone);
void digitReceivedCallback(char);
void configureByNumber(String starcode);
void playCallFailed();
void playTimeoutMessage(String mp3);
void terminal();
bool testMp3slice(String starcode);
bool testMp3file(String starcode);
