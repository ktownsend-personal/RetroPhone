## Atribution
I obtained the files in this folder from [here](https://github.com/atomic14/esp32-play-mp3-demo), but I suspect they came from another source originally.

## Modifications
I was having a weird playback speed issue on the first playthrough of an mp3, so I added a call to `i2s_set_sample_rates()` in `DACOutput.cpp`. 
