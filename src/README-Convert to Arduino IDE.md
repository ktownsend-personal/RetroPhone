### Converting from PlatformIO to Arduino IDE:
1. create `main` folder in your Arduino IDE project folder
1. copy `partitions.csv` file and `src`, `lib` and `data` folders from PlatformIO project into `main`
1. flatten all code files from `src` and `lib` into `main` folder, then delete those folders
1. rename `main.cpp` as `main.ino`
1. update `config.h` with your ESP32 pin settings
1. ensure all files use quotes around #include targets instead of angle brackets 
    * I've already done that in this project, but useful to know about
    * technically Arduino IDE requires the local libraries to use quotes and the built-in libraries can use angle brackets, but all using quotes works fine
1. import any external libraries you need (i.e., FastLED, Preferences.h, etc.)
    * if it's in .zip-lib folder, then you need to install it or import the zip in Arduino IDE
    * other libraries you need will become apparent when you compile, such as Preferences.h which is automatically available in Arduino IDE v2.x but not v1.x
1. upload data folder to spiffs partition
    * you can skip this step if files previously uploaded
    * this only needs to be done if the files in the data folder are changed
    * [instructions for filesystem uploader plugin](https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/) for Arduino IDE
      > NOTE: you have to use Arduino IDE `1.x` to upload to the spiffs partition because `2.x` doesn't have plugin support and might never have it. 
      >You can switch back to `2.x` for working on your program. 
    * You can also use PlatformIO to upload even if you are using Arduino IDE to work on the program. See [data/README.md](../data/README.md) for those instructions.