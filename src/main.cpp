#include "Arduino.h"
#include "progressModes.h"
#include "ringHandler.h"
#include "hookHandler.h"
#include "audioGenerator.h"
#include "dtmfHandler.h"
#include "dtmfModule.h"
#include "Preferences.h"
#include "statusHandler.h"

#define PIN_LED 2           // using onboard LED until I get my addressable RGB
#define PIN_RGB 21          // addressable RGB for status
#define PIN_BTN 12          // external button to initiate ringing
#define PIN_AUDIO_IN 14     // software DTMF and maybe live audio digitization

// Internal DAC pins; just defining so I remember Mozzi and mp3Handler use these
#define PIN_AUDIO_OUT_L 25  // internal DAC pin
#define PIN_AUDIO_OUT_R 26  // internal DAC pin

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
#define PIN_STQ 27          // DTMF value ready to read

bool softwareDTMF = true; // software DTMF decoding and Mozzi don't play nice together; true disables dialtone, false distables softwware DTMF

String digits; // this is where we accumulate dialed digits
auto prefs   = Preferences();
auto ringer  = ringHandler(PIN_RM, PIN_FR, CH_FR);
auto hooker  = hookHandler(PIN_SHK, dialingStartedCallback);
auto dtmfer  = dtmfHandler(PIN_AUDIO_IN, dialingStartedCallback);
auto dtmfmod = dtmfModule(PIN_Q1, PIN_Q2, PIN_Q3, PIN_Q4, PIN_STQ, dialingStartedCallback);
auto region  = RegionConfig(region_northAmerica);
auto player  = audioGenerator(region);
auto status  = statusHandler<PIN_RGB>(100);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("RetroPhone, by Keith Townsend");

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLDOWN);
  pinMode(PIN_SHK, INPUT_PULLUP);

  settingsInit();
  if(!softwareDTMF) existsDTMF_module();

  ringer.setCounterCallback(ringCountCallback);
  hooker.setDigitCallback(digitReceivedCallback);
  dtmfer.setDigitCallback(digitReceivedCallback);
  dtmfmod.setDigitCallback(digitReceivedCallback);

  modeStart(mode); // set initial mode
}

void existsDTMF_module(){
  // DTMF module present?
  bool dtmfHardwareExists = testDTMF_module(40, 40, true);
  if(!dtmfHardwareExists && !softwareDTMF) {
    softwareDTMF = true;
    Serial.println("Hardware DTMF module not detected. Auto-switching to software DTMF.");
  };
  // delay a bit to allow DTMF "D" to finish so we don't detect as dialing it if user has phone off the hook
  auto until = millis() + 50;
  while(millis() < until);
}

// test DTMF module response at given tone and space times
bool testDTMF_module(int toneTime, int spaceTime, bool showSend){
  // testing DTMF module by playing tones ourselves and checking pin responses
  // The module may give a read during the space or even overlap into the next digit, so we must 
  // detect reads independently of playing tones and run the loop extra iterations after finished.
  // My testing shows min toneTime 33 and min spaceTime 19, but min combo time between 54 and 66 depending on combination.
  const String digits = "1234567890*#ABCD";
  String reads;
  bool checked = false;
  if(showSend) Serial.printf("Testing DTMF module: %s", digits.c_str());
  auto until = millis() + ((toneTime+spaceTime)*digits.length()) + 500;
  player.playDTMF(digits, toneTime, spaceTime);
  while(millis() < until){
    bool reading = digitalRead(PIN_STQ);
    if(checked && !reading) checked = false;
    if(!checked && reading) {
      checked = true;
      byte tone = 0x00 | (digitalRead(PIN_Q1) << 0) | (digitalRead(PIN_Q2) << 1) | (digitalRead(PIN_Q3) << 2) | (digitalRead(PIN_Q4) << 3);
      reads += dtmfmod.tone2char(tone);
    }
  }
  int unread = digits.length() - reads.length();
  int misread = 0;
  String result;
  // this misread check assumes all source digits are unique
  for(int x = 0; x < digits.length(); x++){
    int countFound = 0;
    for(int y = 0; y < reads.length(); y++){
      if(digits[x] == reads[y]) countFound++;
    };
    result += countFound == 0 ? '_' : digits[x];
    if(countFound > 1) misread += countFound - 1;
  };
  bool pass = (unread + misread == 0);
  if(showSend && !pass) Serial.println();
  if(!showSend || !pass) Serial.printf("                     %s", result.c_str());
  Serial.printf(" --> %sED: ", pass ? "PASS" : "FAIL");
  if(unread > 0) Serial.printf("%d unread, ", unread);
  if(misread > 0) Serial.printf("%d misread, ", misread);
  Serial.printf("%d/%d ms\n", toneTime, spaceTime);
  return reads.length() > 0;  // simply returning whether we detected the module
}

void settingsInit(){
  delay(50);  // this delay resolved a timing issue accessing Preferences.h, but not sure what the timing issue actually is (filesystem not ready yet, or some other activity?)
  prefs.begin("phone", false);
  bool dtmfmode = prefs.getBool("dtmf", false);
  Serial.printf("Using %s DTMF detection. Dial *20 for hardware, *21 for software.\n", dtmfmode ? "software" : "hardware");
  softwareDTMF = dtmfmode;
  regions r = (regions)prefs.getInt("region", (int)region_northAmerica);
  Serial.printf("Using %s sounds and ring style. Dial *11 for North America, *12 for United Kingdom.\n", r == region_northAmerica ? "North America" : "United Kingdom");
  region = RegionConfig(r);
  player.changeRegion(region);
  prefs.end();
}

void loop() {
  modeRun(mode);                        // always run the current mode before modeDetect() because mode can change during modeRun()
  modeDeferCheck();                     // check if deferred mode ready to activate
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

void modeDefer(modes deferMode, unsigned delay) {
  deferredMode = deferMode;
  deferModeUntil = millis() + delay;
}

void modeDeferCheck() {
  if(deferModeUntil > 0 && deferModeUntil < millis()) 
    modeGo(deferredMode);
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
  Serial.println(); // end the mode line
  deferModeUntil = 0;
  switch(oldmode){
    case call_incoming:
      ringer.stop();
      break;
    default:
      player.stop();
      break;
  }
}

void modeStart(modes newmode) {
  Serial.printf("%s...", modeNames[newmode].c_str());

  digitalWrite(PIN_LED, digitalRead(PIN_SHK)); // basic off-hook status; might expand later with addressable RGB
  status.show(newmode);

  switch(newmode){
    case call_incoming:
      ringer.start(region.ringer.freq, region.ringer.cadence);
      break;
    case call_ready:
      digits = "";
      timeoutStart();
      hooker.start();
      if(softwareDTMF) {
        dtmfer.start();
        // software-DTMF blocks too much to play audio with Mozzi
      } else {
        dtmfmod.start();
        // mozzi.playTone(mozzi.dialtone);
      }
      player.playTone(player.dialtone);
      break;
    case call_pulse_dialing:
    case call_tone_dialing:
      timeoutStart();
      break;
    case call_timeout1:
      player.playMP3("/fs/timeout-bell-f1.mp3", 2, 2000);
      break;
    case call_timeout2:
      player.playTone(player.howler);
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
      player.playTone(player.ringing);
      break;
    case call_busy:
      player.playTone(player.busytone);
      break;
    case call_operator:
      //TODO: handle operator simulation
      break;
    case system_config:
      configureByNumber(digits);
      break;
  }
}

void modeRun(modes mode){
  // mozzi.run();  // this must always run because mozzi needs a lot of cycles to transition to silence
  status.run(); // update LED visualization

  switch(mode){
    case call_incoming:
      ringer.run();
      break;
    case call_ready:
      if(softwareDTMF) {
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
      if(softwareDTMF) {
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
  }else if(digits.length() == 4 && digits.substring(0, 2) == "22"){
    // pulse dialing can't dial *, so we are using "22" as trigger and normalizing it to * for system_config mode
    digits = String("*") + digits.substring(2);
    modeGo(system_config);
  }else if(digits.length() == 7){
    modeGo(call_connecting);
  }
}

void configureByNumber(String starcode){
  bool success = false;
  bool skipack = false;
  bool changed = false;
  if(starcode[0] != '*') return;
  switch(starcode[1]){
    case '1': // change region
      switch(starcode[2]){
        case '1': // region North America
          region = RegionConfig(region_northAmerica);
          player.changeRegion(region);
          prefs.begin("phone", false);
          prefs.putInt("region", region_northAmerica);
          prefs.end();
          Serial.print("region changed to North America");
          success = true;
          break;
        case '2': // region United Kingdom
          region = RegionConfig(region_unitedKingdom);
          player.changeRegion(region);
          prefs.begin("phone", false);
          prefs.putInt("region", region_unitedKingdom);
          prefs.end();
          Serial.print("region changed to United Kingdom");
          success = true;
          break;
      }
      break;
    case '2': // select hardware or software DTMF
      switch(starcode[2]){
        case '0':
          changed = softwareDTMF;
          softwareDTMF = false;
          success = true;
          break;
        case '1':
          changed = !softwareDTMF;
          softwareDTMF = true;
          success = true;
          break;
        case '2':
          // play "D" to clear DTMF module's LEDs
          player.playDTMF("D", 75, 0);
          delay(100);
          success = true;
          break;
      }
      if(changed){
        prefs.begin("phone", false);
        prefs.putBool("dtmf", softwareDTMF);
        prefs.end();
      }
      if(success) Serial.printf("DTMF using %s decoder", softwareDTMF ? "software" : "hardware");
      break;
    case '4': // mp3 test
      Serial.println("Testing MP3 playback; hang up when done...");
      switch(starcode[2]){
        case '1':
          player.playMP3("/fs/circuits-bell-f1.mp3");
          break;
        case '2':
          player.playMP3("/fs/complete2-bell-f1.mp3");
          break;
        case '3':
          player.playMP3("/fs/discoornis-bell-f1.mp3");
          break;
        case '4':
          player.playMP3("/fs/timeout-bell-f1.mp3");
          break;
      }
      return; // make sure we don't play ack tone because it would interfere with the playback
    case '3': // DTMF module speed test; last digit is max iterations, or zero to go until nothing reads successfully
      skipack = true; // must skip normal ack so the D tone at end of the loop can clear the module's LED's
      int start = 39;
      int floor = (starcode[2] == '0') ? 0 : 39 - (starcode[2]-'0');
      Serial.printf("testing DTMF module speed for maximum %d iterations...\n", start-floor);
      for(int duration = start; duration > floor; duration--)
        if(!testDTMF_module(duration, duration, false)) break;
      break;
  }

  if(!skipack) {
    if(success) {
      player.playTone(player.zip, 1);
    } else {
      player.playTone(player.err, 2);
      Serial.print("unrecognized star-code");
    }
  }

  modeDefer(call_ready, 1000);
}
