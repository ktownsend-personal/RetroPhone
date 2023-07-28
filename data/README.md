# Credits
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

## Notes
* there are a couple of audio files with numbers or phrase segments that I want to experiment with slicing on the fly to build responses
