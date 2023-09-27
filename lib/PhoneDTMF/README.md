# PhoneDTMF

Source: https://github.com/Adrianotiger/phoneDTMF

I'm using this for software DTMF decoding. It mostly works as long as it's the only heavy resource using the main loop. If another heavy resource is also called in the main loop, both resources will flail badly. An example is Mozzi, which works well on its own but is murdered while PhoneDTMF is sampling and PhoneDTMF won't read well either. I was able to work around that particular issue by using FreeRTOS for my audio playback on the other core. It may be possible to refactor PhoneDTMF to use FreeRTOS to play better with other resources; TBD.