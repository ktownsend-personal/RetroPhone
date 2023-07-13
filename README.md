# RetroPhone

>*Work-in-progress*

Free for anyone to copy from. I live by the motto "inspire, not require" as much as I can.

This hobby project is to make a few old phones interactive for my retro room so that visitors can experience old-school landline phones without my having to subscribe to an actual phone system. 

## Goals
* [ ] Dial a few phone numbers and get a simulated response from the "other end"
* [ ] Call one of the other phones on display and talk to whoever answers
* [ ] Accurately replicate a real phone experience
  * [x] physical ringing
  * [x] call progress tones
  * [x] rotary dialing (pulse dialing)
  * [X] tone dialing (DTMF)
  * [ ] automated phone service messages

## Optional Goals
* wifi for signaling and comms between phones if ESP32 can handle DAC/ADC simultaneously without noticeable audio problems
* having other devices elsewhere on the Internet and dial them 
  * DIY without using VOIP because if I wanted VOIP [I can buy that for $40](https://a.co/d/4o4eVzs)
* special mode for my old candlestick phone to simulate just picking it up and talking to an operator, possibly with toggling the hook to attract the operator and using a voice assistant to respond to spoken commands
  * if I get that voice assistant idea working, can also use it to respond to dialing zero to get an operator
* status web page, possibly with configurable options
  * could have it run just when on-hook, or maybe only if a special maintenance number is dialed and terminate when phone is hung up again

## Inspiration
* [Telephone Central Office Simulator](https://youtu.be/qM0ZhSyA6Jw) (video) and [related GitHub repo](https://github.com/GadgetReboot/misc_file_bin/tree/master/2022_11/Telephone_Central_Office_wip) from GadgetReboot

## Currently Functional
* button to trigger incoming call (temporary demo until far enough along to establish calls between two devices)
* physical ringing for incoming call
* real call progress tones using [Mozzi](https://sensorium.github.io/Mozzi/); based on work done by [GadgetReboot](https://youtu.be/qM0ZhSyA6Jw)
* stable call progress transitions
* switchable regions for North America and United Kingdom with accurate progress tones and ringing
* tone and pulse dialing
  * call progress detection debounce tuned to ignore dialing pulses
  * hardware DTMF decoding with [MT8870 module](https://microcontrollerslab.com/mt8870-dtmf-decoder-module-pinout-interfacing-with-arduino-features/)
  * software DTMF decoding with [PhoneDTMF](https://github.com/Adrianotiger/phoneDTMF) (has severe limitation...see challenges)
* configuration by dialing star-codes (or 22 instead of * for pulse dialing)
  * region: *11 North America, *12 United Kingdom
  * DTMF decoder: *20 harware, *21 software
* plays audio sample for "call cannot be completed as dialed" when appropriate; based on work done by [GadgetReboot](https://youtu.be/qM0ZhSyA6Jw)
  * full sequence of not finishing dialing is realistic: dialtone, "try again" message twice, howler sound for a while, then silence because it gave up on you
* dialing any 7-digit number will play ring or busy sound depending on first digit even or odd (temporary demo until far enough along to establish calls between two devices)

## Next Steps
* persisted settings to survive reboot and flashing
* ack & err tones for feedback when configuring by dialing
* add filter to block 20Hz ring signal from SLIC's audio out line (mostly to keep it off my external speaker when it's ringing)
* call progress recorded messages
  * need more recordings, maybe need to use MicroSD for storage, but that might be too slow to read from
  * see if file splitting is actually necessary or if we can have single files (original author split to solve problem on ESP8266)
* RGB LED for status colors & patterns representing all of the call states
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
  * This overlap might be normal, even with a real phone system...just rare for anyone to notice because they don't usually have the earpiece to their ear already when going off-hook. If I find a phone on a real phone system I may try to check this.
* The SLIC's audio output pin has the 20Hz ringing signal while ringing (at audio level, not high voltage), likely requiring a filter or an isolation mechanism (relay, solid-state relay, other options?)
* software-DTMF with [PhoneDTMF library](https://github.com/Adrianotiger/phoneDTMF) requires too much sample time and it murders mozzi so it requires not using dialtone, and I had repeating digit issues during steady tone presses

## Notable
* SHK pin from SLIC bounces a lot, so it requires debounce logic. GadgetReboot noticed it bounces longer when powered 3.3V vs. 5V
* RM and FR pins on the SLIC are both necessary for rining. The RM pin sets the higher ringing voltage, and the FR pin flip flops the polarity on high and low cycles. Both are definitely needed, although the electronic ringer on my Sony slimline works fine with just FR toggled, the physical bell on the Snoopy phone requires the RM to have enough power to physically move the armature. The FR pin should be toggled at 50% duty cycle at 20Hz with cadence 2s-on/4s-off for US ring.
* When using PhoneDTMF library, we seem to need 300 sample count and 6000 frequency to avoid detection dropouts while a button is pressed (which causes repeated numbers)
  * I tried 50ms debounce and still got gaps, and since 50ms tone and 50ms space are the standard that I saw someplace we probably shouldn't debounce longer

## Thoughts
* would interrupts be useful in this project so we can put the device to sleep when idle, but still wake up for incoming wifi call or off-hook pin?
  * I wonder if the whole state machine could be based on interrupts and avoid having a loop entirely?
* should probalby have special response for 911 dialing to clearly say it's not a real phone and cannot be used for emergencies

## Tidbits
* `while (!Serial){}` to wait for serial connection (example was right after Serial.begin() in setup())
* `for (const auto &line : lines){}` enumerate a local array (not pointer to array) [ref](https://luckyresistor.me/2019/07/12/write-less-code-using-the-auto-keyword/)
* `Serial.printf("..%s..", stringvar.c_str())` to print `String` type with printf because `%s` epxects C style string not `String` object and will garble the value or even crash the app
* passing arrays to functions, [good explanation](https://stackoverflow.com/a/19894884/8228356)
  in a nutshell, arg is simple pointer and you pass array directly when calling, but really a pointer to first array value is passed and in the func you can access array values by index like normal...but you can't know the length and must pass that value separately

## Hardware
* [KS0835F SLIC module](https://www.youtube.com/redirect?event=video_description&redir_token=QUFFLUhqbEtxcHQ2MnVEQ3c2ZXVjNHRtZW82Tk1JSS1UUXxBQ3Jtc0ttV0g1ZlFleXBXV0JRbVJTbldEbW12X2JVQ0ZJcEJ0NG44ck94cUtmeEowY2xuNi1QSEQwbzFzYmo1cDdGLTFWNHR4QmpVbS0yNlRvdWFYeEN4b3JUcnFYZnN3SWkwUGRmSmI4UDNFSDE3R1Rlb0Iycw&q=https%3A%2F%2Fs.click.aliexpress.com%2Fe%2F_DFeMKoP&v=qM0ZhSyA6Jw) (AG1171/AG1170 clone)
* [ESP32 Freenove WROVER CAM board](https://www.amazon.com/ESP32-WROVER-Contained-Compatible-Bluetooth-Tutorials/dp/B09BC1N9LL) (not using camera for this project, but the power & GPIO breakout adapter is really handy)
* [MT8870 module](https://microcontrollerslab.com/mt8870-dtmf-decoder-module-pinout-interfacing-with-arduino-features/)
* [Level Shifter](https://www.sparkfun.com/products/12009)
* [LM358P op-amp](https://www.mouser.com/ProductDetail/Texas-Instruments/LM358P?qs=X1HXWTtiZ0QtOTT8%252BVnsyw%3D%3D)

## Software
* this repository
* [VisualStudio Code](https://code.visualstudio.com/) with [PlatformIO extension](https://platformio.org/?utm_source=platformio&utm_medium=piohome) using Arduino libraries in C++
* [DTMF software decoder](https://github.com/Adrianotiger/phoneDTMF)
* [Mozzi](https://sensorium.github.io/Mozzi/)

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