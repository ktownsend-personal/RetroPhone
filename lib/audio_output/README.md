# audio_output

I obtained the files in this folder from [here](https://github.com/atomic14/esp32-play-mp3-demo), but I suspect they came from another source originally.

### Modifications
<u>weird playback speed</u>
I was having a weird playback speed issue on the first playthrough of an mp3, so I added a call to `i2s_set_sample_rates()` in `DACOutput.cpp`. When I was experiencing the issue I was playing the files right after boot. *I have since removed this modification due to other changes making it unnecessary.*

<u>adjustable bitrate</u>
I later added a new method Output.set_frequency(hz) so I could keep the object started for the life of the app and update the sample rate only when it needs to change. This was necessary becuase opening and closing the I2S driver in rapid succession was causing trouble. 

<u>destructor</u>
I also added a destructor to Output to close the driver explicitly so that if I do sometimes need to shut it down I still can.

<u>popping & clicking</u>
I discovered that the i2s_config.tx_desc_auto_clear feature, originally set true in the source I copied from, was causing popping sounds because it idles the output at 0v rather than 0-signal (mid-voltage range between 0v and 3.3v) and the sharp transition causes popping in the speaker. Before I knew about this setting I added ramps to the buffer to smooth the transition, which worked well until I refactored my playback code to splice different audio segments together. While debugging that I found this auto-clear feature and realized it is why my very first try of stuffing the buffer with zeros wasn't effective (before I resorted to ramps). By turning this feature off, stuffing the buffer with zeros works great and I don't need ramps. This makes the audio output *so much easier to manage*!
