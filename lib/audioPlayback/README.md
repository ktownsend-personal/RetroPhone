I created this library based on files I found [here](https://github.com/atomic14/esp32-play-mp3-demo), which also use the `audio_output`, `minimp3` and `Mozzi` libraries. From Mozzi I'm just using the `Oscil` class and a sinewave table. 

I really like the FreeRTOS task approach I found in the original example, which works out much better for this project than Mozzi's loop approach. In particular, Mozzi and PhoneDTMF can't operate together in the same loop due to how much time PhoneDTMF requires to listen to the ADC. With the task approach we have the ability to choose either core to run on and it doesn't interfere with the main program loop. 

[Mozzi](https://sensorium.github.io/Mozzi/) is a complex library with far more capability than I need, so I extracted Oscil.h and sin2048_int8.h from Mozzi and simplified them to the bare essence of what I need. It did not affect my compiled size, but it makes the code easier to understand. Had I not already been using these from Mozzi in my earlier audio incarnations I probably would have gone a different route and cobbled together a sine table and oscillator functions based on other great examples I saw online in my original research. 

## TODO
* stitch tones and mp3's together in one playback (i.e., replicating the stepped tones at start of a recording)
* notification mechanism back to main loop that a playback is finished to improve coordination of modes
* playing slices of mp3 files, such as the numbers file or the prhase segments file (this is mostly done)
