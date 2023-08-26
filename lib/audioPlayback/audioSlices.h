#ifndef audioSlices_h
#define audioSlices_h

#include "Arduino.h"

struct sliceConfig {
  String filename;
  unsigned long byteOffset;
  unsigned long samplesToPlay;
  String label;
};

struct sliceConfigs {
  sliceConfig zero =                                    sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 4670,   23770, "0"};
  sliceConfig one =                                     sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 8944,   18787, "1"};
  sliceConfig two =                                     sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 12520,  18566, "2"};
  sliceConfig three =                                   sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 16088,  19580, "3"};
  sliceConfig four =                                    sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 19768,  17684, "4"};
  sliceConfig five =                                    sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 23015,  27651, "5"};
  sliceConfig six =                                     sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 28291,  23682, "6"};
  sliceConfig seven =                                   sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 32749-1000,  18610+1000, "7"};
  sliceConfig eight =                                   sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 36172,  18213-2000, "8"};
  sliceConfig nine =                                    sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 39468,  25049-500, "9"};
  sliceConfig zero_alt =                                sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 44038,  23814-2500, "0 (alternate)"};
  sliceConfig please_note =                             sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 48391,  32810-500, "please note"};
  sliceConfig this_is_a_recording =                     sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 54373,  48334-1500, "this is a recording"};
  sliceConfig has_been_changed =                        sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 63104-1000,  52082+1000, "has been changed"};
  sliceConfig the_number_you_have_dialed =              sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 72565,  68002, "the number you have dialed"};
  sliceConfig is_not_in_service =                       sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 84929-500,  57065, "is not in service"};
  sliceConfig please_check_the_number_and_dial_again =  sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 95336,  98343-2500, "please check the number and dial again"};
  sliceConfig the_number_dialed =                       sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 113184-1000, 48334+2000, "the number dialed"};
  sliceConfig the_new_number_is =                       sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 121971-1000, 57771, "the new number is"};
  sliceConfig enter_function =                          sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 132475-1000, 31840, "enter function"};
  sliceConfig please_enter =                            sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 138263-1000, 29547, "please enter"};
  sliceConfig area_code =                               sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 143635-1000, 33913, "area code"};
  sliceConfig new_number =                              sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 149801-1000, 27783, "new number"};
  sliceConfig invalid =                                 sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 154852-1000, 27563, "invalid"};
  sliceConfig not_available =                           sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 159864-1000, 32414, "not available"};
  sliceConfig enter_service_code =                      sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 165757-1000, 57418, "enter service code"};
  sliceConfig deleted =                                 sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 176196-500, 21344, "deleted"};
  sliceConfig category =                                sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 180077-500, 28489-700, "category"};
  sliceConfig date =                                    sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 185256-300, 17993, "date"};
  sliceConfig re_enter =                                sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 188527-1000, 25754, "re-enter"};
  sliceConfig thank_you =                               sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 193210-800, 20551, "thank you"};
  sliceConfig or_dial_directory_assistance =            sliceConfig{"/fs/anna-1DSS-default-vocab.mp3", 196946-500, 75499, "or dial directory assistance"};
};

#endif