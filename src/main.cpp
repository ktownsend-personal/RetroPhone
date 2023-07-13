#include <Arduino.h>
#include <progressModes.h>
#include <ringHandler.h>
#include <hookHandler.h>
#include <mozziHandler.h>
#include <dtmfHandler.h>
#include <dtmfModule.h>

#define PIN_LED 2           // using onboard LED until I get my addressable RGB
#define PIN_BTN 12          // external button to initiate ringing
#define PIN_AUDIO_IN 14     // software DTMF and live audio digitization if we are able to make that work 
#define PIN_AUDIO_OUT_L 25  // Mozzi defaults to this
#define PIN_AUDIO_OUT_R 26  // Mozzi defaults to this

// SLIC module
#define PIN_SHK 13          // SLIC SHK, off-hook
#define PIN_RM 32           // SLIC RM, ring mode enable
#define PIN_FR 33           // SLIC FR, ring toggle, use PWM 50%
#define CH_FR 0             // SLIC FR, PWM channel
#define RING_FREQ 20        // SLIC FR, PWM frequency

// DTMF module
#define PIN_Q1 36           // DTMF bit 1
#define PIN_Q2 39           // DTMF bit 2
#define PIN_Q3 34           // DTMF bit 3
#define PIN_Q4 35           // DTMF bit 4
#define PIN_STQ 27          // DTMF active and ready to read

bool ENABLE_DTMF = false; // DTMF and Mozzi don't play nice together; true disables dialtone, false distables DTMF

String digits; // this is where we accumulate dialed digits
auto ringer = ringHandler(PIN_RM, PIN_FR, CH_FR);
auto hooker = hookHandler(PIN_SHK, dialingStartedCallback);
auto dtmfer = dtmfHandler(PIN_AUDIO_IN, dialingStartedCallback);
auto dtmfmod = dtmfModule(PIN_Q1, PIN_Q2, PIN_Q3, PIN_Q4, PIN_STQ, dialingStartedCallback);
auto mozzi = mozziHandler(region_northAmerica);

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLDOWN);
  pinMode(PIN_SHK, INPUT_PULLUP);
  Serial.begin(115200);
  ringer.setCounterCallback(ringCountCallback);
  hooker.setDigitCallback(digitReceivedCallback);
  dtmfer.setDigitCallback(digitReceivedCallback);
  dtmfmod.setDigitCallback(digitReceivedCallback);
}

void loop() {
  modeRun(mode);                        // always run the current mode before modeDetect() because mode can change during modeRun()
  modes newmode = modeDetect();         // check inputs for new mode
  if(modeBouncing(newmode)) return;     // wait for bounce to settle
  if(newmode != mode) modeGo(newmode);  // switch to new mode
}

bool modeBouncing(modes newmode) {
  // kill ringer faster by skipping debounce when going off-hook; debounce continues fine after mode change
  if(mode == call_incoming && newmode == call_connected) return false;
  // reset debounce timer on each observation of change
  if(newmode != lastDebounceValue){
    lastDebounceTime = millis();
    lastDebounceValue = newmode;
    return true;
  }
  // true = bouncing, false = stable
  return (millis() - lastDebounceTime) < debounceDelay;
}

modes modeDetect() {
  int SHK = digitalRead(PIN_SHK);
  int BTN = digitalRead(PIN_BTN);

  //NOTE: every case must have a break to prevent fallthrough if return conditions not met
  switch(mode) {
    case call_idle:
      if(SHK) return call_ready;
      if(BTN) return call_incoming;
      break;
    case call_incoming:
      if(SHK) return call_connected;
      break;
    default:
      if(!SHK) return call_idle;
      break;
  }
  return mode; // if we get here the mode didn't change
}

void modeStop(modes oldmode) {
  Serial.println(" /");
  switch(oldmode){
    case call_incoming:
      ringer.stop();
      break;
    default:
      mozzi.stop();
      break;
  }
}

void modeStart(modes newmode) {
  Serial.printf("%s...", modeNames[newmode].c_str());

  digitalWrite(PIN_LED, digitalRead(PIN_SHK)); // basic off-hook status; might expand later with addressable RGB

  switch(newmode){
    case call_incoming:
      //TODO: we may need to move region into a dedicated library instead of inside mozziHandler since it's no longer just mozzi settings
      ringer.start(mozzi.currentRegion().ringer.freq, mozzi.currentRegion().ringer.cadence);
      break;
    case call_ready:
      digits = "";
      timeoutStart();
      hooker.start();
      // software-DTMF blocks too much to play audio, but hardware-DTMF ok
      if(ENABLE_DTMF) {
        dtmfer.start();
      } else {
        dtmfmod.start();
        mozzi.playTone(mozzi.dialtone);
      }
      break;
    case call_pulse_dialing:
    case call_tone_dialing:
      timeoutStart();
      break;
    case call_timeout1:
      mozzi.playSample(mozzi.dialAgain, 2, 2000);
      break;
    case call_timeout2:
      mozzi.playTone(mozzi.howler);
      break;
    case call_abandoned:
      break;
    case call_connecting:
      Serial.print(digits);
      //TODO: implement real connection negotiation; this fake simulation rings if first digit even, or busy if odd
      if(digits[0] % 2== 0)
        modeGo(call_ringing);
      else
        modeGo(call_busy);
      break;
    case call_ringing:
      mozzi.playTone(mozzi.ringing);
      break;
    case call_busy:
      mozzi.playTone(mozzi.busytone);
      break;
    case call_operator:
      //TODO: handle operator simulation
      break;
    case system_config:
      configureByNumber(digits);
      modeGo(call_ready);
      break;
  }
}

void modeRun(modes mode){
  mozzi.run(); // this must always run because mozzi needs a lot of cycles to transition to silence

  switch(mode){
    case call_incoming:
      ringer.run();
      break;
    case call_ready:
      if(ENABLE_DTMF) {
        dtmfer.run();   // software-DTMF blocks too much to play audio, so if we want dialtone we have to disable this
      } else {
        dtmfmod.run();  // hardware DTMF module ok to run with dialtone
      }
      hooker.run();
      timeoutCheck();
      break;
    case call_pulse_dialing:
      hooker.run();
      timeoutCheck();
      break;
    case call_tone_dialing:
      // software-DTMF blocks too much to play audio, but hardware-DTMF ok
      if(ENABLE_DTMF) {
        dtmfer.run();
      } else {
        dtmfmod.run();
      }
      timeoutCheck();
      break;
    case call_timeout1:
    case call_timeout2:
      timeoutCheck();
      break;
  }
}

void modeGo(modes newmode){
  modeStop(mode);
  mode = newmode;
  modeStart(newmode);
}

void timeoutStart(){
  timeoutStarted = millis();
}

void timeoutCheck(){
  unsigned int since = (millis() - timeoutStarted);
  if(since > abandonMax) return (mode == call_abandoned) ? void() : modeGo(call_abandoned); // check by longest max first
  if(since > timeout2Max) return (mode == call_timeout2) ? void() : modeGo(call_timeout2);  
  if(since > timeout1Max) return (mode == call_timeout1) ? void() : modeGo(call_timeout1);
}

// callback from ringer class so we can show ring counts in the serial output
void ringCountCallback(int rings){
  Serial.printf("%d.", rings);
}

// callback from both tone and DTMF dailing handlers to signal us when dialing has started so we can reset dialed digits and switch off dialtone
void dialingStartedCallback(bool isTone){
  if(isTone)
    modeGo(call_tone_dialing);
  else
    modeGo(call_pulse_dialing);
}

// callback from both tone and DTMF dialing handlers to signal when a digit is dialed so we can accumulate it and decide what to do
void digitReceivedCallback(char digit){
  timeoutStart();           // reset timeout after user input
  Serial.print(digit);      // debug output show digits as they are received
  digits += digit;          // accumulate the digit

  if(digits.length() == 1 && digits[0] == '0'){
    modeGo(call_operator);
  }else if(digits.length() == 3 && digits[0] == '*'){
    modeGo(system_config);
  }else if(digits.length() == 4 && digits.substring(0, 1) == "22"){
    // pulse dialing can't dial *, so we are using "22" as trigger and normalizing it to * for system_config mode
    digits = '*' + digits.substring(2, 3);
    modeGo(system_config);
  }else if(digits.length() == 7){
    modeGo(call_connecting);
  }
}

void configureByNumber(String starcode){
  
  //TODO: ack & error beeps

  if(starcode[0] != '*') return;
  switch(starcode[1]){
    case '1':
      switch(starcode[2]){
        case '1': 
          mozzi.changeRegion(region_northAmerica);
          Serial.print("region changed to North America");
          break;
        case '2': 
          mozzi.changeRegion(region_unitedKingdom);
          Serial.print("region changed to United Kingdom");
          break;
      }
      break;
    case '2':
      ENABLE_DTMF = !!(starcode[2] - '0');
      Serial.printf("DTMF using %s decoder", ENABLE_DTMF ? "software" : "hardware");
      break;
  }
}