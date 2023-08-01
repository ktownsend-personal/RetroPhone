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

// Internal DAC pins; just defining so I remember these are used by the audio libraries
#define PIN_AUDIO_OUT_L 25  // internal DAC pin
#define PIN_AUDIO_OUT_R 26  // internal DAC pin

// SLIC module
#define PIN_SHK 13          // SLIC SHK, off-hook
#define PIN_RM 32           // SLIC RM, ring mode enable
#define PIN_FR 33           // SLIC FR, ring toggle, use PWM 50%
#define CH_FR 0             // SLIC FR, PWM channel

// DTMF module
#define PIN_Q1 36           // DTMF bit 1
#define PIN_Q2 39           // DTMF bit 2
#define PIN_Q3 34           // DTMF bit 3
#define PIN_Q4 35           // DTMF bit 4
#define PIN_STQ 27          // DTMF value ready to read

bool softwareDTMF = false;  // software DTMF decoding is finicky, so I want to switch between hardware and software DTMF decoders easily

String digits; // this is where we accumulate dialed digits
auto prefs   = Preferences();
auto ringer  = ringHandler(PIN_RM, PIN_FR, CH_FR);
auto hooker  = hookHandler(PIN_SHK, dialingStartedCallback);
auto dtmfer  = dtmfHandler(PIN_AUDIO_IN, dialingStartedCallback);
auto dtmfmod = dtmfModule(PIN_Q1, PIN_Q2, PIN_Q3, PIN_Q4, PIN_STQ, dialingStartedCallback);
auto region  = RegionConfig(region_northAmerica);
auto player  = audioGenerator(region, 0); // core 0 starts audio plyaback faster than core 1 (the main loop is on core 1, wifi & bluetooth are core 0)
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

// this will wait the number of milliseconds specified, or until user hangs up
// returns true if user hung up, otherwise false
bool wait(unsigned int milliseconds){
  auto until = millis() + milliseconds;
  while(millis() < until) if(!digitalRead(PIN_SHK)) return true;
  return false;
}

// this will defer an action or abort if mode changes first
void deferActionUnlessModeChange(unsigned int milliseconds,  void (*action)()){
  deferAction = action;
  deferActionUntil = millis() + milliseconds;
}

void deferActionCheckElapsed(){
  if(deferActionUntil > 0 && deferActionUntil < millis()) {
    deferActionUntil = 0;
    deferAction();
  }
}

void existsDTMF_module(){
  // DTMF module present?
  bool dtmfHardwareExists = testDTMF_module(40, 40, true, true);
  if(!dtmfHardwareExists && !softwareDTMF) {
    softwareDTMF = true;
    Serial.println("Hardware DTMF module not detected. Auto-switching to software DTMF.");
  };
  // delay a bit to allow DTMF "D" to finish so we don't detect as dialing it if user has phone off the hook
  wait(50);
}

// test DTMF module response at given tone and space times
bool testDTMF_module(int toneTime, int spaceTime, bool showSend, bool ignoreSHK){
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
    if(!ignoreSHK && !digitalRead(PIN_SHK)) return false; // abort if user hangs up
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
  Serial.printf("\tUsing %s DTMF detection. Dial *20 for hardware, *21 for software.\n", dtmfmode ? "software" : "hardware");
  softwareDTMF = dtmfmode;
  regions r = (regions)prefs.getInt("region", (int)region_northAmerica);
  region = RegionConfig(r);
  player.changeRegion(region);
  Serial.printf("\tUsing %s sounds and ring style. Dial *11 for North America, *12 for United Kingdom.\n", region.label.c_str());
  prefs.end();
}

void loop() {
  modeRun(mode);                        // always run the current mode before modeDetect() because mode can change during modeRun()
  modeDeferCheck();                     // check if deferred mode ready to activate
  deferActionCheckElapsed();            // check if a deferred action is waiting to run
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
  if(deferModeUntil > 0 && deferModeUntil < millis()) {
    deferModeUntil = 0;
    modeGo(deferredMode);
  }
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
  deferActionUntil = 0;
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
      } else {
        dtmfmod.start();
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
  status.run(); // update LED visualization

  switch(mode){
    case call_incoming:
      ringer.run();
      break;
    case call_ready:
      if(softwareDTMF) {
        dtmfer.run();   // software-DTMF is finicky with the loop because of how much time it takes to get samples
      } else {
        dtmfmod.run();  // hardware DTMF module works great
      }
      hooker.run();
      timeoutCheck();
      break;
    case call_pulse_dialing:
      hooker.run();
      timeoutCheck();
      break;
    case call_tone_dialing:
      if(softwareDTMF) {
        dtmfer.run();   // software-DTMF is finicky with the loop because of how much time it takes to get samples
      } else {
        dtmfmod.run();  // hardware DTMF module works great
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

  auto dlen = digits.length();

  // convert 22 to * for pulse dialing alternate star-code
  if(dlen > 1 && digits.substring(0, 2) == "22") digits = String("*") + digits.substring(2);

  auto d0 = digits[0];
  auto d1 = digits[1];

  if(dlen == 1 && d0 == '0') return modeGo(call_operator);

  if(d0 == '*'){
    if(d1 == '5' && dlen < 4) return; // need 4 digits for menu 5
    if(dlen < 3) return;              // need 3 digits for all other menus
    return modeGo(system_config);
  }
  
  if(dlen == 7) return modeGo(call_connecting);
}

void configureByNumber(String starcode){
  bool success = false;
  bool skipack = false;
  bool changed = false;
  if(starcode[0] != '*') return;
  switch(starcode[1]){

    case '1': // change region
      switch(starcode[2]){
        case '0': // show current region
          Serial.printf("region is %s", region.label.c_str());
          success = true;
          break;
        case '1': // region North America
          region = RegionConfig(region_northAmerica);
          player.changeRegion(region);
          prefs.begin("phone", false);
          prefs.putInt("region", region_northAmerica);
          prefs.end();
          Serial.printf("region set to %s", region.label.c_str());
          success = true;
          break;
        case '2': // region United Kingdom
          region = RegionConfig(region_unitedKingdom);
          player.changeRegion(region);
          prefs.begin("phone", false);
          prefs.putInt("region", region_unitedKingdom);
          prefs.end();
          Serial.printf("region set to %s", region.label.c_str());
          success = true;
          break;
      }
      break;

    case '2': // select hardware or software DTMF
      switch(starcode[2]){
        case '0':
          changed = softwareDTMF;   // check this before changing
          softwareDTMF = false;
          success = true;
          break;
        case '1':
          changed = !softwareDTMF;  // check this before changing
          softwareDTMF = true;
          success = true;
          break;
        case '2':
          player.playDTMF("D", 75, 0);
          if (wait(500)) return; // wait or abort if user hangs up
          Serial.print("cleared DTMF module LEDs");
          return modeGo(call_ready); // skip ack tone
      }
      if(changed){
        prefs.begin("phone", false);
        prefs.putBool("dtmf", softwareDTMF);
        prefs.end();
      }
      if(success) Serial.printf("DTMF using %s decoder", softwareDTMF ? "software" : "hardware");
      break;

    case '3': {       // DTMF module speed test; last digit is max iterations, or zero to go until nothing reads successfully
      skipack = true; // must skip normal ack so the D tone at end of the loop can clear the module's LED's
      int start = 39;
      byte num = starcode[2] - '0';
      int floor = (num == 0) ? 0 : (start - num);
      Serial.printf("testing DTMF module speed for maximum %d iterations...\n", start-floor);
      for(int duration = start; duration > floor; duration--)
        if(!testDTMF_module(duration, duration)) break;
      break;
    }

    case '4': {
      //NOTE: managing the success tone and mode switch directly so we can get the timing right
      auto play = [](String filepath, unsigned milliseconds)-> void {
        Serial.printf("testing MP3 playback [%s]...", filepath.c_str());
        player.playMP3(filepath);
        if (wait(milliseconds + 800)) return; // attempting to delay exactly the right amount of time before switching mode, but abort if user hangs up
        player.playTone(player.zip, 1);
        modeDefer(call_ready, 500);
      };
      switch(starcode[2]){
        case '1': return play("/fs/circuits-xxx-anna.mp3",        7706);
        case '2': return play("/fs/complete2-bell-f1.mp3",        8856);
        case '3': return play("/fs/discoornis-bell-f1.mp3",       10109);
        case '4': return play("/fs/timeout-bell-f1.mp3",          7367);
        case '5': return play("/fs/svcoroption-xxx-anna.mp3",     12434);
        case '6': return play("/fs/anna-1DSS-default-vocab.mp3",  26776);
        case '7': return play("/fs/feature5-xxx-anna.mp3",        1881);
        case '8': return play("/fs/feature6-xxx-anna.mp3",        1802);
      }
    }

    case '5': {
      //NOTE: managing the success tone and mode switch directly so we can get the timing right
      byte option = (starcode[2]-'0') * 10 + (starcode[3]-'0');
      auto slice = [option](unsigned offset, unsigned samples, String label)-> void {
        Serial.printf("testing MP3 slice [%s]...", label.c_str());
        player.playMP3("/fs/anna-1DSS-default-vocab.mp3", 1, 0, offset, samples);
        if (wait(samples/44.1 + 500)) return; // attempting to delay exactly the right amount of time before switching mode, but abort if user hangs up
        player.playTone(player.zip, 1);
        modeDefer(call_ready, 1000);
      };
      switch(option){
        case 0:  return slice(4670,   23770, "0");
        case 1:  return slice(8944,   18787, "1");
        case 2:  return slice(12520,  18566, "2");
        case 3:  return slice(16088,  19580, "3");
        case 4:  return slice(19768,  17684, "4");
        case 5:  return slice(23015,  27651, "5");
        case 6:  return slice(28291,  23682, "6");
        case 7:  return slice(32749,  18610, "7");
        case 8:  return slice(36172,  18213, "8");
        case 9:  return slice(39468,  25049, "9");
        case 10: return slice(44038,  23814, "0 (alternate)");
        case 11: return slice(48391,  32810, "please note");
        case 12: return slice(54373,  48334, "this is a recording");
        case 13: return slice(63104,  52082, "has been changed");
        case 14: return slice(72565,  68002, "the number you have dialed");
        case 15: return slice(84929,  57065, "is not in service");
        case 16: return slice(95336,  98343, "please check the number and dial again");
        case 17: return slice(113184, 48334, "the number dialed");
        case 18: return slice(121971, 57771, "the new number is");
        case 19: return slice(132475, 31840, "enter function");
        case 20: return slice(138263, 29547, "please enter");
        case 21: return slice(143635, 33913, "area code");
        case 22: return slice(149801, 27783, "new number");
        case 23: return slice(154852, 27563, "invalid");
        case 24: return slice(159864, 32414, "not available");
        case 25: return slice(165757, 57418, "enter service code");
        case 26: return slice(176196, 21344, "deleted");
        case 27: return slice(180077, 28489, "category");
        case 28: return slice(185256, 17993, "date");
        case 29: return slice(188527, 25754, "re-enter");
        case 30: return slice(193210, 20551, "thank you");
        case 31: return slice(196946, 75499, "or dial directory assistance");
      }
    }

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
