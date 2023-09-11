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
  call_ringing,       // play ringback tone
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
  "available for call", 
  "ready to dial", 
  "pulse dialing", 
  "tone dialing",
  "call connecting", 
  "destination busy",
  "call failed",
  "destination ringing",
  "call connected",
  "call disconnected",
  "helpful message",
  "attention, off-hook",
  "call abandoned",
  "incoming call",
  "calling operator",
  "configuring"
};

#endif