## Atribution
I obtained the files in this folder from [here](https://github.com/atomic14/esp32-play-mp3-demo), but I suspect they came from another source originally.

## Modifications
I was having a weird playback speed issue on the first playthrough of an mp3, so I added a call to `i2s_set_sample_rates()` in `DACOutput.cpp`. When I was experiencing the issue I was playing the files right after boot. *I have since removed this modification due to other changes making it unnecessary.*

I later added a new method Output.set_frequency(hz) so I could keep the object started for the life of the app and update the sample rate for each audio request. This was necessary becuase opening and closing the I2S driver in rapid succession was causing trouble. 

I also added a destructor to Output to close the driver explicitly so that if I do sometimes need to shut it down I still can.
