# audioPlayback

I created this library based on files I found [here](https://github.com/atomic14/esp32-play-mp3-demo), which also use the `audio_output` and `minimp3` libraries. I have evolved it quite a bit, but that was a great starting point.

I really like the FreeRTOS task approach I found in the original example, which works out much better for this project than Mozzi's loop approach. In particular, Mozzi and PhoneDTMF can't operate together in the same loop due to how much time PhoneDTMF requires to listen to the ADC. With the task approach we have the ability to choose either core to run on and it doesn't interfere with the main program loop. 

[Mozzi](https://sensorium.github.io/Mozzi/) is a complex library with far more capability than I need, so I extracted Oscil.h and sin2048_int8.h from Mozzi and simplified them to the bare essence of what I need. It did not affect my compiled size, but it makes the code easier to understand. Had I not already been using these from Mozzi in my earlier audio incarnations I probably would have gone a different route and cobbled together a sine table and oscillator functions based on other great examples I saw online in my original research. 

## Features:
* play slices of mp3 files, such as phrases and numbers to build a message
* sequencing any combination of tones and mp3 slices so we can do things like read out a phone number or play the triple-tone signal at the start of error messages

## TODO
* notification mechanism back to main loop that a playback is finished to improve coordination of modes

## NOTES
* we need to balance the smallest chunk size we can get away with for generating tones
  * as small as possible so dialtone can cancel ASAP, but long enough to avoid audio glitches when the CPU is working on other stuff
    > The less we have in the buffer the less there is remaining to play when we cancel the dialtone. This is critical to avoid hearing a blip of dialtone after pulse dialing "1" because it's so short. 
  * the shortest chunk I was able to get working was 33 samples (1ms at 32768Hz), but I had to increase to 50 samples later due to CPU activity; this may require future adjustment as we start getting into WiFi comms
  * `Hz * seconds = chunk size` &rarr; example: 32768Hz * 0.001s = 33 samples
  * for comparison, mp3 handler chunks at 26.122ms: 576@22050Hz, 1152@44100Hz