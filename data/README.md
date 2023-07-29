## Credits
The recordings were obtained from [ThisIsARecording.com](http://ThisIsARecording.com). 
Specifically, some were here: https://www.thisisarecording.com/Joyce-Gordon.html

## Uploading
How to upload to ESP32 onboard flash with Platformio on VSCode
* Source: https://randomnerdtutorials.com/esp32-vs-code-platformio-spiffs/
* Summary:
  * Close any serial connections to ESP32
  * In Platformio extension view
    * expand device type and Platform tree
    * then select "Build Filesystem Image"
    * then "Upload Filesystem Image"

Note, I had to modify the partition table for the onboard flash to increase the size of the spiffs partition. See partitions.csv in the root folder.

## Removing embedded art
I used [MP3TAG](https://www.mp3tag.de/en/) to remove the album art to conserve space. I left the text tags to preserve attribution. I did this right after I found out the album art is more than half the file size! Each file had the same 137KB image!

## Slicing
There are a couple of audio files with numbers or phrase segments so I made a slicing feature.

I started out with a trial and error approach to discovering the right file offset and sample count for each segment, but that would have taken a month so I took the time to figure out the math. I had the code figure out exactly how many file bytes and audio samples were decoded (ignoring non-audio bytes), and the sample rate. I then use Audacity to view the waveform and get the start and end times of each segment in fractional seconds. I derived these formulas and put them in an Excel file to simplify my work:
*	audio_start_byte = file_size_bytes – audio_size_bytes
*	audio_total_seconds = audio_samples / audio_rate
*	segment_offset_byte = audio_start_byte + (audio_size_bytes / audio_total_seconds * segment_start_seconds)
*	segment_samples = (segment_stop_seconds – segment_start_seconds) * audio_rate

For the anna-1DSS-default-vocab.mp3 file I have these measurements:
*	file_size_bytes = 217067
*	audio_size_bytes = 214682
*	audio_samples = 1180800
*	audio_rate = 44100 (Hz)

Refer to [mp3 slicing.xlsx](../docs/mp3%20slicing.xlsx) for the results. 