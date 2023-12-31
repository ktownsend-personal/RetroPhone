#include "Arduino.h"
#include "main.h"
#include "config.h"
#include "progressModes.h"
#include "regionConfig.h"
#include "prefsHandler.h"
#include "ringHandler.h"
#include "pulseHandler.h"
#include "dtmfHandler.h"
#include "dtmfModule.h"
#include "audioPlayback.h"
#include "audioSlices.h"
#include "statusHandler.h"
#include "wifiHandler.h"
// #include "streamTest.h"

String digits; // this is where we accumulate dialed digits
auto region  = RegionConfig(region_northAmerica);
auto prefs   = prefsHandler();
auto ringer  = ringHandler();
auto pulser  = pulseHandler(PIN_SHK, dialingStartedCallback);
auto dtmfer  = dtmfHandler(PIN_AUDIO_IN, dialingStartedCallback);
auto dtmfmod = dtmfModule(PIN_Q1, PIN_Q2, PIN_Q3, PIN_Q4, PIN_STQ, dialingStartedCallback);
auto player  = audioPlayback(region, 0); // core 0 better; core 1 delays mp3 playback start too much when splicing and requires larger tone chunk size to avoid underflowing buffer (the main loop is on core 1, wifi & bluetooth are core 0)
auto status  = statusHandler<PIN_RGB>(100);
auto comms   = wifiHandler();
// auto stream  = streamTest();

bool onHook() {return !digitalRead(PIN_SHK);}
bool ringButton() {return digitalRead(PIN_BTN);}

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(PIN_MUTE, OUTPUT);
  pinMode(PIN_LED,  OUTPUT);
  pinMode(PIN_BTN,  INPUT_PULLDOWN);
  pinMode(PIN_SHK,  INPUT_PULLUP);

  inputMuting(true); // start muted

  splash();
  settingsInit();
  if(!softwareDTMF && detectMT870) moduleDTMF_exists();
  Serial.println();

  ringer.init(PIN_RM, PIN_FR, CH_FR);
  ringer.setCounterCallback(ringCountCallback);
  pulser.setMaybeCallback(maybeDialingStartedCallback);
  pulser.setDigitCallback(digitReceivedCallback);
  dtmfer.setDigitCallback(digitReceivedCallback);
  dtmfmod.setDigitCallback(digitReceivedCallback);
  comms.init();           // WIFI auto-connect & config portal
  Serial.println();

  // stream.init();
  // player.setStreamCallback([](short* sBuffer, size_t bytesIn)->void{stream.sendAudio(sBuffer, bytesIn);});

  modeStart(mode, mode);  // activate initial call progress mode
}

void splash(){
  // used this website to generate text art: https://patorjk.com/software/taag/#p=testall&f=Trek&t=RetroPhone%20

  // Serial.println(" -------------------------------------------------- ");
  // Serial.println("|          ╦═╗┌─┐┌┬┐┬─┐┌─┐╔═╗┬ ┬┌─┐┌┐┌┌─┐          |");
  // Serial.println("|          ╠╦╝├┤  │ ├┬┘│ │╠═╝├─┤│ ││││├┤           |");
  // Serial.println("|          ╩╚═└─┘ ┴ ┴└─└─┘╩  ┴ ┴└─┘┘└┘└─┘          |");
  // Serial.println("|               2023, Keith Townsend               |");
  // Serial.println("| https://github.com/ktownsend-personal/RetroPhone |");
  // Serial.println(" -------------------------------------------------- ");

  Serial.println(R"(╭────────────────────────────────────────────────────────────────────────────╮)");
  Serial.println(R"(│ __________        __               __________.__                           │)");
  Serial.println(R"(│ \______   \ _____/  |________  ____\______   \  |__   ____   ____   ____   │)");
  Serial.println(R"(│  |       _// __ \   __\_  __ \/  _ \|     ___/  |  \ /  _ \ /    \_/ __ \  │)");
  Serial.println(R"(│  |    |   \  ___/|  |  |  | \(  <_> )    |   |   Y  (  <_> )   |  \  ___/  │)");
  Serial.println(R"(│  |____|_  /\___  >__|  |__|   \____/|____|   |___|  /\____/|___|  /\___  > │)");
  Serial.println(R"(│         \/     \/                                 \/            \/     \/  │)");
  Serial.println(R"(│                            2023, Keith Townsend                            │)");
  Serial.println(R"(│              https://github.com/ktownsend-personal/RetroPhone              │)");
  Serial.println(R"(╰────────────────────────────────────────────────────────────────────────────╯)");
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

void moduleDTMF_exists(){
  // DTMF module present?
  bool dtmfHardwareExists = moduleDTMF_test("1248D", 50, 50, true, true); // playing just the tones that light each Q bit LED individually, and D to clear them
  if(!dtmfHardwareExists && !softwareDTMF) {
    softwareDTMF = true;
    Serial.println("Hardware DTMF module not detected. Auto-switching to software DTMF.");
  };
  player.await(); // wait for playback to finish so we don't detect as dialing it if user has phone off the hook
}

// test DTMF module response at given tone and space times
bool moduleDTMF_test(String digits, int toneTime, int spaceTime, bool showSend, bool ignoreSHK){
  // testing DTMF module by playing tones ourselves and checking pin responses
  // The module may give a read during the space or even overlap into the next digit, so we must 
  // detect reads independently of playing tones and run the loop extra iterations after finished.
  // My testing shows min toneTime 33 and min spaceTime 19, but min combo time between 54 and 66 depending on combination.
  String reads;
  bool checked = false;
  if(showSend) Serial.printf("> Testing DTMF module: %s", digits.c_str());
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
    if(!ignoreSHK && onHook()) return false; // abort if user hangs up
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
  if(!showSend || !pass) Serial.printf("\t%s", result.c_str());
  Serial.printf(" @ %d/%d ms --> ", toneTime, spaceTime);
  if(unread > 0) Serial.printf("%d unread, ", unread);
  if(misread > 0) Serial.printf("%d misread, ", misread);
  Serial.printf("%sED\n", pass ? "PASS" : "FAIL");
  return reads.length() > 0;  // simply returning whether we detected the module
}

void settingsInit(){
  delay(50);  // this delay resolved a timing issue accessing Preferences.h, but not sure what the timing issue actually is (filesystem not ready yet, or some other activity?)
  softwareDTMF = prefs.getDTMF();
  Serial.printf("> Using %s DTMF detection. Dial *20 for hardware, *21 for software.\n", softwareDTMF ? "software" : "hardware");
  regions r = (regions)prefs.getRegion();
  region = RegionConfig(r);
  player.changeRegion(region);
  Serial.printf("> Using %s sounds and ring style. Dial *11 for North America, *12 for United Kingdom.\n", region.label.c_str());
}

void loop() {
  comms.run();                              // process non-blocking WiFi config portal if not yet connected
  modeRun(mode);                            // always run the current mode before modeDetect() because mode can change during modeRun()
  modeDeferCheck();                         // check if deferred mode ready to activate
  deferActionCheckElapsed();                // check if a deferred action is waiting to run
  modes newmode = modeDetect();             // check inputs for new mode
  if(modeBouncing(newmode)) return;         // wait for bounce to settle
  if(newmode != mode) modeGo(newmode);      // switch to new mode
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
  //NOTE: every case must have a break to prevent fallthrough if return conditions not met
  switch(mode) {
    case call_idle:
      if(!onHook())     return call_ready;
      if(ringButton())  return call_incoming;
      break;
    case call_incoming:
      if(!onHook())     return call_connected;
      break;
    default:
      if(onHook())      return call_idle;
      break;
  }
  return mode; // if we get here the mode didn't change
}

void modeStop(modes oldmode, bool keepPlaybackAlive) {
  Serial.println();     // end the active mode line
  deferModeUntil = 0;   // cancel scheduled mode change
  deferActionUntil = 0; // cancel scheduled action
  switch(oldmode){
    case call_incoming: 
      return ringer.stop();
    default:
      inputMuting(true);
      return keepPlaybackAlive ? void() : player.stop();
  }
}

void modeStart(modes newmode, modes oldmode) {
  Serial.printf("> %s...", modeNames[newmode].c_str());

  digitalWrite(PIN_LED, !onHook()); // basic off-hook status
  status.show(newmode);             // addressable LED for mode status

  switch(newmode){
    case call_ready:
      digits = "";
      timeoutStart();
      inputMuting(false); // need to hear DTMF tones from SLIC
      pulser.start();
      player.playTone(player.dialtone);
      return (softwareDTMF) ? dtmfer.start() : dtmfmod.start();
    case call_connecting:
      Serial.print(digits);
      //TODO: implement real connection negotiation; this fake simulation is based on first digit
      switch(digits[0] % 3){
        case 0: return modeGo(call_ringing);
        case 1: return modeGo(call_busy);
        case 2: return modeGo(call_fail);
      }
      return;
    case call_timeout:
      switch(oldmode){
        case call_ready:          return playTimeoutMessage("/fs/timeout-bell-f1.mp3");
        case call_pulse_dialing:  return playTimeoutMessage("/fs/complete2-bell-f1.mp3");
        case call_tone_dialing:   return playTimeoutMessage("/fs/complete2-bell-f1.mp3");
      }
      return;
    case call_incoming:       return ringer.start(region.ringer.freq, region.ringer.cadence);
    case call_pulse_dialing:  return timeoutStart();
    case call_tone_dialing:   
      inputMuting(false); // need to hear DTMF tones from SLIC
      return timeoutStart();
    case call_ringing:        return player.playTone(player.ringback);
    case call_busy:           return player.playTone(player.busytone);
    case call_fail:           return playCallFailed();
    case system_config:       return configureByNumber(digits);
    case call_operator:       return; //TODO: handle operator simulation
    case call_abandoned:      return;
  }
}

void modeRun(modes mode){
  status.run(); // update LED visualization

  switch(mode){
    case call_incoming: 
      return ringer.run();
    case call_ready:
      pulser.run();
      return (softwareDTMF) ? dtmfer.run() : dtmfmod.run();
    case call_pulse_dialing:
      return pulser.run();
    case call_tone_dialing:
      return (softwareDTMF) ? dtmfer.run() : dtmfmod.run();
  }
}

void modeGo(modes newmode, bool keepPlaybackAlive){
  if(mode == newmode) return;
  auto oldmode = mode;
  modeStop(oldmode, keepPlaybackAlive);
  mode = newmode;
  modeStart(newmode, oldmode);
}

void timeoutStart(){
  modeDefer(call_timeout, timeoutMax);
}

// callback from ringer class so we can show ring counts in the serial output
void ringCountCallback(int rings){
  Serial.printf("%d.", rings);
}

void maybeDialingStartedCallback(){
  player.stop(); // kill dialtone as fast as possible to avoid hearing it when dialing a 1 first with pulse dialing
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

  auto rest = []()-> void {
    modeDefer(call_ready, 600);
  };

  auto ack = [rest]()-> void {
    player.playTone(player.zip, 1);
    rest();
  };

  auto err = [rest]()-> void {
    player.playTone(player.err, 2);
    Serial.print("unrecognized star-code");
    rest();
  };
  
  if(starcode[0] != '*') return;
  switch(starcode[1]){

    case '1': // change region
      switch(starcode[2]){
        case '0': // show current region
          Serial.printf("region is %s", region.label.c_str());
          return ack();
        case '1': // region North America
          region = RegionConfig(region_northAmerica);
          player.changeRegion(region);
          prefs.setRegion(region_northAmerica);
          Serial.printf("region set to %s", region.label.c_str());
          return ack();
        case '2': // region United Kingdom
          region = RegionConfig(region_unitedKingdom);
          player.changeRegion(region);
          prefs.setRegion(region_unitedKingdom);
          Serial.printf("region set to %s", region.label.c_str());
          return ack();
      }
      break;

    case '2': { // select hardware or software DTMF

      auto change = [ack](bool value){
        if(value != softwareDTMF){
          softwareDTMF = value;
          prefs.setDTMF(softwareDTMF);
        }
        Serial.printf("DTMF using %s decoder", softwareDTMF ? "software" : "hardware");
        return ack();
      };

      switch(starcode[2]){
        case '0': return change(false);
        case '1': return change(true);
        case '2':
          player.playDTMF("D", 75, 0);
          Serial.print("cleared DTMF module LEDs");
          return rest();
      }
      break;
    }

    case '3': {       // DTMF module speed test; last digit is max iterations, or zero to go until nothing reads successfully
      const String digits = "1234567890*#ABCD";
      int start = 39;
      byte num = starcode[2] - '0';
      int floor = (num == 0) ? 0 : (start - num);
      Serial.printf("testing DTMF module speed for maximum %d iterations...\n", start-floor);
      for(int duration = start; duration > floor; duration--)
        if(!moduleDTMF_test(digits, duration, duration)) break;
      return rest();
    }

    case '4': return testMp3file(starcode) ? ack() : err();
    case '5': return testMp3slice(starcode) ? ack() : err();

    case '6':
      switch(starcode[2]){
        case '0':
          comms.showStatus();
          return ack();
        case '1':
          comms.showNetworks();
          return ack();
      }
      break;

    case '7':
      switch(starcode[2]){
        case '1': return terminal(); // ack tones handled by function, so skipping it here
      }
      break;
  }

  err();  // if we get here the starcode wasn't handled
}

void playCallFailed(){
  player.queueGap(1000);
  player.queueInfoTones(player.low_short, player.low_short, player.low_long); // number changed or disconnected
  player.queueSlice(SLICES.the_number_you_have_dialed);
  player.queueNumber(digits);
  player.queueSlice(SLICES.is_not_in_service);
  player.queueSlice(SLICES.please_check_the_number_and_dial_again);
  player.queueSlice(SLICES.this_is_a_recording);
  player.queueGap(2000);
  player.queueTone(player.reorder);
  player.play();
}

void playTimeoutMessage(String mp3){
  player.queueMP3(mp3, 2, 2000);
  player.queueGap(5000);
  player.queueCallback([]()->void{Serial.print("attention!");});
  player.queueTone(player.howler, 300); // 300 iterations * 200ms cadence = 60 seconds
  player.queueCallback([]()->void{modeGo(call_abandoned);});
  player.play();
}

void terminal(){
  Serial.println("starting console mode; enter 'help' for list of commands");
  String command;
  bool reenter = false;
  while(command != "exit"){
    command = ""; // reset command
    Serial.print("\nEnter command: ");
    player.queueSlice(reenter ? SLICES.re_enter : SLICES.enter_service_code);
    player.play();
    while(true){
      reenter = false;
      command = Serial.readStringUntil('\n');
      if(command == "") continue; // loop until we get a command
      if(command == "exit") {
        Serial.println("exiting serial input mode");
      } else if(command == "help") {
        Serial.println("available commands:");
        Serial.println("\thelp");
        Serial.println("\texit");
        Serial.println("\tnetwork status");
        Serial.println("\tnetwork list");
        Serial.println("\tregion na");
        Serial.println("\tregion uk");
        Serial.println("\tdtmf software");
        Serial.println("\tdtmf hardware");
        Serial.println("\tdtmf module speed test");
      } else if (command == "network status") {
        comms.showStatus();
      } else if (command == "network list") {
        comms.showNetworks();
      } else if (command == "region na") {
        region = RegionConfig(region_northAmerica);
        player.changeRegion(region);
        prefs.setRegion(region_northAmerica);
        Serial.printf("region set to %s\n", region.label.c_str());
      } else if (command == "region uk") {
        region = RegionConfig(region_unitedKingdom);
        player.changeRegion(region);
        prefs.setRegion(region_unitedKingdom);
        Serial.printf("region set to %s\n", region.label.c_str());
      } else if (command == "dtmf software") {
        softwareDTMF = true;
        prefs.setDTMF(true);
        Serial.println("using software DTMF decoder");
      } else if (command == "dtmf hardware") {
        softwareDTMF = false;
        prefs.setDTMF(false);
        Serial.println("using hardware DTMF decoder");
      } else if (command == "dtmf module speed test") {
        const String digits = "1234567890*#ABCD";
        int start = 39;
        Serial.println("testing DTMF module speed until all digits fail...");
        for(int duration = start; duration > 0; duration--)
          if(!moduleDTMF_test(digits, duration, duration, false, true)) break;
      } else {
        Serial.println("unrecognized command");
        player.playTone(player.err, 2);
        reenter = true;
        delay(600);
        break;
      }
      player.playTone(player.zip, 1);
      delay(600);
      break;
    }
  }
}

bool testMp3slice(String starcode){

  auto sliceit = [](sliceConfig slice)-> bool {
    Serial.printf("testing MP3 slice [%s]...", slice.label.c_str());
    player.queueSlice(slice);
    player.play();
    player.await(onHook);
    return true;
  };

  byte option = (starcode[2]-'0') * 10 + (starcode[3]-'0');

  switch(option){
    case 0:  return sliceit(SLICES.zero);
    case 1:  return sliceit(SLICES.one);
    case 2:  return sliceit(SLICES.two);
    case 3:  return sliceit(SLICES.three);
    case 4:  return sliceit(SLICES.four);
    case 5:  return sliceit(SLICES.five);
    case 6:  return sliceit(SLICES.six);
    case 7:  return sliceit(SLICES.seven);
    case 8:  return sliceit(SLICES.eight);
    case 9:  return sliceit(SLICES.nine);
    case 10: return sliceit(SLICES.zero_alt);
    case 11: return sliceit(SLICES.please_note);
    case 12: return sliceit(SLICES.this_is_a_recording);
    case 13: return sliceit(SLICES.has_been_changed);
    case 14: return sliceit(SLICES.the_number_you_have_dialed);
    case 15: return sliceit(SLICES.is_not_in_service);
    case 16: return sliceit(SLICES.please_check_the_number_and_dial_again);
    case 17: return sliceit(SLICES.the_number_dialed);
    case 18: return sliceit(SLICES.the_new_number_is);
    case 19: return sliceit(SLICES.enter_function);
    case 20: return sliceit(SLICES.please_enter);
    case 21: return sliceit(SLICES.area_code);
    case 22: return sliceit(SLICES.new_number);
    case 23: return sliceit(SLICES.invalid);
    case 24: return sliceit(SLICES.not_available);
    case 25: return sliceit(SLICES.enter_service_code);
    case 26: return sliceit(SLICES.deleted);
    case 27: return sliceit(SLICES.category);
    case 28: return sliceit(SLICES.date);
    case 29: return sliceit(SLICES.re_enter);
    case 30: return sliceit(SLICES.thank_you);
    case 31: return sliceit(SLICES.or_dial_directory_assistance);
    default: return false;
  }
}

bool testMp3file(String starcode){

  auto play = [](String filepath)-> bool {
    Serial.printf("testing MP3 playback [%s]...", filepath.c_str());
    player.playMP3(filepath);
    player.await(onHook);
    return true;
  };

  switch(starcode[2]){
    case '1': return play("/fs/circuits-xxx-anna.mp3");
    case '2': return play("/fs/complete2-bell-f1.mp3");
    case '3': return play("/fs/discoornis-bell-f1.mp3");
    case '4': return play("/fs/timeout-bell-f1.mp3");
    case '5': return play("/fs/svcoroption-xxx-anna.mp3");
    case '6': return play("/fs/anna-1DSS-default-vocab.mp3");
    case '7': return play("/fs/feature5-xxx-anna.mp3");
    case '8': return play("/fs/feature6-xxx-anna.mp3");
    default: return false;
  }
}

void inputMuting(bool muted){
  digitalWrite(PIN_MUTE, !muted); // TODO: remove the "not" if I have this backwards based on high vs. low mute signal
}