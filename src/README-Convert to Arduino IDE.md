### Converting from PlatformIO to Arduino IDE:
1. make `main` folder in your Arduino IDE project folder
1. copy `partitions.csv` file and `src`, `lib` and `data` folders from PlatformIO project into `main`
1. flatten all code files from `src` and `lib` into `main` folder, then delete those folders
1. rename `main.cpp` as `main.ino`
1. make sure all files use quotes around #include targets instead of angle brackets 
    * already done in advance, but useful to know about
1. import any external libraries you need (i.e., FastLED)
    * basically, if it's in .zip-lib folder, then you need to install it or import the zip in Arduino IDE
1. upload data folder to spiffs partition
    * you can skip this step if files previously uploaded, even if done from PlatformIO
    * install [filesystem uploader plugin](https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/) for Arduino IDE
      * NOTE: you have to use Arduino IDE v1.x (1.8.19 is latest) to upload spiffs because v2.x doesn't have plugin support, and it's written in a different language so none of the 1.x plugins are compatible. You can switch back to 2.x IDE for normal program dev and uploads. 
      * NOTE: if you don't want to mess with Arduino IDE plugin or switching versions, you can also use PlatformIO to upload the data folder. See [data/README.md](../data/README.md) for those instructions.