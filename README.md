# RetroPhone

>*Work-in-progress*

Free for anyone to copy from. I live by the motto "inspire, not require" as much as I can.

This hobby project is to make a few old phones interactive for my retro room so that visitors can experience old-school landline phones without having to buy phone service.

There is a lot of info in this README, but also some useful stuff in the [docs folder](docs/README.md).

<p float="left">
  <a href="docs/modules and chips.jpg"><img src="docs/modules and chips.jpg" width="49%" /></a>
  <a href="docs/prototype on breadboard.jpg"><img src="docs/prototype on breadboard.jpg" width="49%" /></a>
</p>

## Goals
* [ ] Accurately replicate a real phone experience
  * [x] physical ringing
  * [x] call progress tones
  * [x] rotary dialing (pulse dialing)
  * [X] tone dialing (DTMF)
  * [ ] automated phone service messages
* [ ] Dial a few phone numbers and get a simulated response from the "other end"
* [ ] Call one of the other phones on display and talk to whoever answers

## Optional Goals
* wifi for signaling and comms between phones if ESP32 can handle DAC/ADC simultaneously without noticeable audio problems
* having other devices elsewhere on the Internet and dial them 
  * DIY without using VOIP because if I wanted VOIP [I can buy that for $40](https://a.co/d/4o4eVzs)
* special mode for my old candlestick phone to simulate just picking it up and talking to an operator, possibly with toggling the hook to attract the operator and using a voice assistant to respond to spoken commands
  * if I get that voice assistant idea working, can also use it to respond to dialing zero to get an operator
* status web page, possibly with configurable options
  * could have it run just when on-hook, or maybe only if a special maintenance number is dialed and terminate when phone is hung up again

## Inspiration & Motivation
* [Telephone Central Office Simulator](https://youtu.be/qM0ZhSyA6Jw) (video) and [related GitHub repo](https://github.com/GadgetReboot/misc_file_bin/tree/master/2022_11/Telephone_Central_Office_wip) from **GadgetReboot**
  >I have a great appreciation for GadgetReboot's willingness to collaborate on our similar projects. He graciously filled in a lot of gaps I had on the hardware side, and I still have much to learn. My military electronics training is about 25 years stale now and I never had the benefit of practical engineering of electronic circuits... all theory and no substance once could say.
* I got my first ESP32 many months ago and it's been sitting patiently on my workbench waiting for me to get around to it. I never even did a flashing LED demo...leave it to me to start out with something complicated!
* I've been wanting to get back into tinkering with electronics for many, many years. My software career has been great, but I still have that longing to build stuff with real parts. I've loved both computer programming and electronics since I first discovered them as a child...ESP32 is a great way to do both!
* I like old phones and I've started to grow a small collection. Why not make them do something?
* This project has a special connection to a childhood memory. When I was around 12 years old I experimented with the house phone line by putting a speaker on it and hearing a dialtone. I was fascinated, and learned how to dial it by making and breaking the connection rapidly to call a friend and discovered he could hear me if I shouted into the speaker. I took a few old phones apart to study their innards and learned a few more things about them in the process. My parents never knew about that, of course! 

## Currently Functional
* physical ringing for incoming call, currently triggered by a button
* real call progress tones using [Mozzi](https://sensorium.github.io/Mozzi/); based on work done by [GadgetReboot](https://youtu.be/qM0ZhSyA6Jw)
* stable call progress transitions
* switchable regions for North America and United Kingdom with accurate progress tones and ringing
* tone and pulse dialing
  * hardware DTMF decoding with [MT8870 module](https://microcontrollerslab.com/mt8870-dtmf-decoder-module-pinout-interfacing-with-arduino-features/)
  * software DTMF decoding with [PhoneDTMF](https://github.com/Adrianotiger/phoneDTMF) (has severe limitation...see challenges)
  * automatic presence detection of hardware DTMF module at startup to force software detection if not present
  * software pulse dialing decoder
* configuration by dialing star-codes (or 22 instead of * for pulse dialing)
  * settings are saved to onboard flash and survive reboot and flashing updates
  * ack & err tone feedback
  * region: dial *11 North America, *12 United Kingdom
  * DTMF decoder: dial *20 harware, *21 software
  * DTMF module timing test: *3n where n is the max number of iterations [1 to 9] or [0] for no limit; test ends at max iterations or entire 16 digit sequence doens't read
* plays audio sample for "call cannot be completed as dialed" when appropriate; based on work done by [GadgetReboot](https://youtu.be/qM0ZhSyA6Jw)
  * full sequence of not finishing dialing is realistic: dialtone, "try again" message twice, howler sound for a while, then silence because it gave up on you
  * able to play mp3 files from internal flash so we can fit more recorded messages and manage them more easily
* dialing any 7-digit number will play ring or busy sound depending on first digit even or odd (temporary demo until far enough along to establish calls between two devices)
* RGB LED for status colors & patterns representing all of the call states
* mp3 playback for system messages and potentially other scenarios
  * also able to play short slices from the middle of an mp3 file for a future sequencing feature, such as reading out the number you dialed during a system message

## Next Steps
* call progress recorded messages
  * recently implemented playing mp3 files from onboard flash, but don't have the messages playing at the right times yet and don't have them equalized
* add filter to block 20Hz ring signal from SLIC's audio out line (mostly to keep it off my external speaker when it's ringing)
* trunk line via wifi, or wired if wifi affects audio quality
  * could switch to PiZeroW or something if simultaneious ADC/DAC/Wifi is too much for ESP32
  * might be an option to design these modules for a backplane in a single housing and just use wiring between the modules for analog audio (think of it like a local switching office)
* documentation
  * create a schematic for my circuit
  * describe the code

## Challenges
* Timing of stopping the ringer when handset taken off-hook is difficult to get "immediate", so there is a brief amount of clicking on the handset when first picked up. 
  * I was able to tweak my debounce logic to skip the debounce period when specifically transitioning from incoming to connected (triggered by going off-hook), which has it down to about 100 milliseconds of ringing overlap.
  * It seems like the SLIC takes a moment to detect off-hook when the RM pin is high, possibly related to the higher voltage while ringing. I noticed if I leave RM low while ringing the overlap disappears, however we need the RM pin high to actuate a physical bell. 
  * This overlap might be normal, even with a real phone system...just rare for anyone to notice because they don't usually have the earpiece to their ear already when going off-hook while ringing. If I find a phone on a real phone system I will try to check this.
* The SLIC's audio output pin has the 20Hz ringing signal while ringing (at audio level, not high voltage), likely requiring a filter or an isolation mechanism (relay, solid-state relay, other options?)
* software-DTMF with [PhoneDTMF library](https://github.com/Adrianotiger/phoneDTMF) requires too much sample time and it murders mozzi so it requires not using dialtone, and I had repeating digit issues during steady tone presses
  * While trying a new strategy to generate call progress tones I found that PhoneDTMF doesn't murder the new dialtone. It doesn't hear the DTMF tone sometimes, but seems to get it most of the time. The new techique uses I2S to the local DAC like Mozzi, but runs as a FreeRTOS task feeding a buffer with multiple samples at a time instead of a single sample during each main loop iteration. 
* Using Preferences.h to save settings was hanging on the prefs.begin() call at startup. I couldn't find anyone else online having this issue, but I eventually figured out it was a timing issue of some sort. A short delay before calling prefs.begin() was needed. A 50ms delay seems sufficient. I occasionally saw hangs when reading values during startup(), so if you see that try increasing the delay. 

## Notable
* SHK pin from SLIC bounces a lot, so it requires debounce logic. GadgetReboot noticed it bounces longer when powered 3.3V vs. 5V
* RM and FR pins on the SLIC are both necessary for rining. The RM pin sets the higher ringing voltage, and the FR pin flip flops the polarity on high and low cycles. Both are definitely needed, although the electronic ringer on my Sony slimline works fine with just FR toggled, the physical bell on the Snoopy phone requires the RM to have enough power to physically move the armature. The FR pin should be toggled at 50% duty cycle at 20Hz with cadence 2s-on/4s-off for US ring.
* When using PhoneDTMF library, we seem to need 300 sample count and 6000 frequency to avoid detection dropouts while a button is pressed (which causes repeated numbers)
  * I tried 50ms debounce and still got gaps, and since 50ms tone and 50ms space are the standard that I saw someplace we probably shouldn't debounce longer
  * Update: I have since found a bug in my coordination of PhoneDTMF that resolved the debounce issue without needing a debounce.
* phones only use frequencies 300Hz to 3400Hz, but somehow we actually hear lower tones due to [this phenomenon](https://blogs.scientificamerican.com/roots-of-unity/your-telephone-is-lying-to-you-about-sounds/)

## Thoughts
* would interrupts be useful in this project so we can put the device to sleep when idle, but still wake up for incoming wifi call or off-hook pin?
* special response for 911 dialing to clearly say it's not a real phone and cannot be used for emergencies
* digitizing settings can be fairly low because a real phone system caps the upper frequency at 3400 Hz
* could use an IO pin to control power to the hardware DTMF decoder and level shifter to reduce power consumption, but only if I want to run on a battery

## Tidbits
* `for (const auto &line : lines){}` enumerate a local array (not pointer to array) [ref](https://luckyresistor.me/2019/07/12/write-less-code-using-the-auto-keyword/)
* `Serial.printf("..%s..", stringvar.c_str())` to print `String` type with printf because `%s` epxects C style string not `String` object and will garble the value or even crash the app
* passing arrays to functions, [good explanation](https://stackoverflow.com/a/19894884/8228356)
  in a nutshell, arg is simple pointer and you pass array directly when calling, but really a pointer to first array value is passed and in the func you can access array values by index like normal...but you can't know the length and must pass that value separately

## Hardware
* [KS0835F SLIC module](https://www.youtube.com/redirect?event=video_description&redir_token=QUFFLUhqbEtxcHQ2MnVEQ3c2ZXVjNHRtZW82Tk1JSS1UUXxBQ3Jtc0ttV0g1ZlFleXBXV0JRbVJTbldEbW12X2JVQ0ZJcEJ0NG44ck94cUtmeEowY2xuNi1QSEQwbzFzYmo1cDdGLTFWNHR4QmpVbS0yNlRvdWFYeEN4b3JUcnFYZnN3SWkwUGRmSmI4UDNFSDE3R1Rlb0Iycw&q=https%3A%2F%2Fs.click.aliexpress.com%2Fe%2F_DFeMKoP&v=qM0ZhSyA6Jw) (AG1171/AG1170 clone)
* [ESP32 Freenove WROVER CAM board](https://www.amazon.com/ESP32-WROVER-Contained-Compatible-Bluetooth-Tutorials/dp/B09BC1N9LL) (not using camera for this project, but the power & GPIO breakout adapter is really handy)
* [MT8870 module](https://microcontrollerslab.com/mt8870-dtmf-decoder-module-pinout-interfacing-with-arduino-features/)
* [Level Shifter](https://www.pololu.com/product/2595)
* [LM358P op-amp](https://www.mouser.com/ProductDetail/Texas-Instruments/LM358P?qs=X1HXWTtiZ0QtOTT8%252BVnsyw%3D%3D)
* [WS2812S module](https://a.co/d/4Lo5N9y)

## Software
* [VisualStudio Code](https://code.visualstudio.com/) with [PlatformIO extension](https://platformio.org/?utm_source=platformio&utm_medium=piohome) using Arduino libraries in C++
* this repository
* [DTMF software decoder](https://github.com/Adrianotiger/phoneDTMF)
* [Mozzi](https://sensorium.github.io/Mozzi/)
* [FastLED](https://github.com/FastLED/FastLED)
* [ESP32 MP3 Player](https://github.com/atomic14/esp32-play-mp3-demo)
* [minimp3.h](https://github.com/lieff/minimp3)

## Call Progress Modes
I made this chart to help me track what transitional modes I should implement and what is active during each mode. Work in progress and likely to change as I get deeper into it and discover which assumptions aren't correct. 

`Phone` column represents user experience (interactive elements like handset audio, bell ringer). `MF-` prefix means multi-frequency tone. The `Dialer` detects dialed numbers on local phone, which is not really in the `Trunk` category but I didn't want another column just for the Dialer.

|State|Hook|Trunk|Phone|Notes|
|---|---|---|---|---|
|Idle|ON|LISTEN|Website|&bull; website active for status, statistics & configuration (maybe all the time if not affecting operation)|
|Ready|OFF|Dialer|MF-Dialtone|&bull; switch to Dialing as soon as first number dialed|
|Tone-Dialing|OFF|Dialer||&bull; restart timeout after each dialed number (maybe not necessary; how does real phone system do it?)|
|Pulse-Dialing|OFF|Dialer||&bull; restart timeout after each dialed number (maybe not necessary; how does real phone system do it?)|
|Connecting|OFF|ROUTE||&bull; negotiate connection|
|Busy|OFF||MF-Busy|&bull; start timeout|
|Route Fail|OFF||Recording|&bull; "number not in service"<br>&bull; start timeout|
|Ringing|OFF|LOOP|MF-Ring|&bull; must send signaling to keep route alive<br>&bull; receiving end can optionally disconnect if no answer after custom duration|
|Connected|OFF|AUDIO|Live Audio||
|Disconnected|OFF|||&bull; call audio stream terminated by remote end (ringing or active call)<br>&bull; start timeout in case user fails to hang up|
|Timeout|OFF||Recording, MF-Howler|&bull; left off hook too long unconnected<br>&bull; how long is appropriate for timeout?<br>&bull; "please hang up and try your call again", then play howler|
|Abandoned|OFF|||&bull; gave up waiting for you to hang up, so line is abandoned and services are disabled until back on hook|
|Incoming|ON|LOOP|Physical Ringing|&bull; remote end must send repeated or continuous signal, which abandons ringing if it ends|

## Call Progress Flow
```mermaid
flowchart TD
  subgraph Originator
    idle3(idle) -- off-hook --> ready
    ready -->|dialing\nstarts| dialing(dialing\npulse or tone)
    ready -->|off-hook\ntoo long| timeout
    dialing -->|enough\nnumbers\ndialed| connecting
    dialing -->|off-hook\ntoo long| timeout
    timeout -->|howler\nignored| abandoned
    connecting --> busy
    connecting --> fail
    connecting --> ringing
    ringing --> connected
    ringing --> disconnected
    connected --> disconnected
    connected -- on-hook --> idle(idle)
    disconnected -->|off-hook\ntoo long| timeout
    disconnected -- on-hook --> idle
    timeout -- on-hook --> idle
    abandoned -- on-hook --> idle
  end
  subgraph Receiver
    idle4(idle) --> incoming2[incoming\n-ringing-]
    incoming2 -->|ignored until caller\ngives up or too many\nrings aborts the call| idle2
    incoming2 -->|answered| connected2[connected]
    connected2 --> disconnected2[disconnected]
    connected2 -- on-hook --> idle2(idle)
    disconnected2 -- on-hook --> idle2
    disconnected2 --> timeout2[timeout]
    timeout2 --> abandoned2[abandoned]
    timeout2 -- on-hook --> idle2
    abandoned2 -- on-hook --> idle2
  end
  connecting -.->|routing| incoming2
  ringing <-.->|signaling\nbidirectional| incoming2
  connected <-.->|audio\nactive| connected2
```