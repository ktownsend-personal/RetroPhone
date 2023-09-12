# RetroPhone - *The Code*

This project uses the Arduino library, but instead of using the Arduino IDE I chose to use the PlatformIO extension with Visual Studio Code. I plan to add some instructions to convert this to an Arduino IDE project for anyone that prefers that environment. I use Visual Studio Code for my real job, so I find it more comfortable to use.

Start with main.cpp to get a feel for what this code is doing. The `setup()` and `loop()` functions are the entry points, just like any Arduino project.

I have encapsulated many of the functions as separate libraries to help me keep the concepts organized. I have also used a few libraries written by others, sometimes heavily modified for my needs. I try to document where I found things and what I changed in a README file within the library folder.

## Call Progress Mode Transitions

This is the heart of the operation. Essentially a state machine, which I have split into discrete functions to make the flow easier to reason about. 

### Loop Flow
I organized the loop flow as follows:
1. run current mode
2. switch to a deferred mode if time elapsed
3. run a deferred action if time elapsed
4. check inputs for mode change
6. if mode change from inputs:
    1. if not bouncing, switch to it

### Run Current Mode
`modeRun(mode)`
This is just a switch statement on progress modes to call into the appropriate handler and do something meaningful. Typically used for things that need a slice of execution time every loop iteration. Things running in FreeRTOS tasks don't need this, such as audio playback.

### Changing Mode
There are three ways to change the progress mode, all of which result in a call to `modeGo(newmode)`:
1. external conditions checked during loop
    for example, user takes phone off hook or we have an incoming call
    * `newmode = modeDetect()` to check conditions
    * `if(modeBouncing(newmode)) return` to wait for inputs to stabilize
    * `if(newmode != mode) modeGo(newmode)` to commit the mode change
2. logic executed during the loop decides to 
    * change the mode immediately
      * `modeGo(newmode)` 
    * change the mode after a non-blocking delay
      * `modeDefer(mode, delay)` to schedule the change
      * `modeDeferCheck()` during loop to change at the right time
      * `modeGo(newmode)` when time has elapsed
      * `modeStop(mode)` will cancel a scheduled mode change

When a mode ultimately changes via `modeGo()` it calls `modeStop()` and `modeStart()`. 

### Deferred Action
Sometimes it is useful to schedule an action after a non-blocking delay, but cancel it if the mode changes before the time has elapsed. The `deferActionUnlessModeChange(time, callback)` function lets us do that, and it works the same way as a deferred mode change. If the mode changes it will cancel a scheduled action.

## Functional Components

### Audio Playback
This feature is managed by the [lib/audioPlayback](../lib/audioPlayback/) library, which utilizses [lib/audio_output](../lib/audio_output/), [lib/minimp3](../lib/minimp3/) and [lib/spiffs](../lib/spiffs/). See README files in each of those folders.

This is responsible for playing multi-frequency tones (up to 4 frequencies) with or without a cadence, and mp3 files of pre-recorded content, such as authentic error messages. I have designed the playback feature to allow stitching together a combination of tones and partial or whole mp3's. Partial mp3's are for splicing together the digits of a phone number as part of an error message.  

### Tone Dialing
There are two libraries in this project, one for hardware decoding ([lib/dtmfModule](../lib/dtmfModule/)) and one for software decoding ([lib/dtmfHandler](../lib/dtmfHandler/), which also uses [lib/PhoneDTMF](../lib/PhoneDTMF/)). They are interchangeable, but only one is used at a time. I have a star-code menu option to switch between the two, as well as detection of the hardware decoder at startup to automatically fallback to software decoding if the MT8870 hardware module is not present.

### Pulse Dialing
The [lib/pulseHandler](../lib/pulseHandler/) library handles detection of pulse dialing. It uses the same callbacks as tone dialing to make the dialing behavior consistent.

### Star-Code Menu
This is currently managed as a function in `main.cpp` but I might try extracting it to a library later. 

This feature allows us to change options at runtime, such as the region used for ringing and progress tones and more.

### Status LED
The [lib/statusHandler](../lib/statusHandler/) runs our addressable RBG LED to indicate the current call mode. We are using the [FastLED](https://fastled.io/) library. See README file in that folder for more information.

### Ring the Bell
The [lib/ringHandler](../lib/ringHandler/) handles ringing the bell when there is an incoming call. At the moment we have a button to trigger ringing, but eventually it will be triggered by an incoming call.

### Regional Sounds
The [lib/regionConfig](../lib/regionConfig/) library encapsulates the various settings for frequencies and cadences for call progress tones and bell ringing. We currently have configurations for North America and United Kingdom.
