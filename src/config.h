#ifndef main_config_h
#define main_config_h

#define PIN_LED      2      // using onboard LED for basic off-hook status
#define PIN_RGB      21     // addressable RGB for call progress mode status
#define PIN_BTN      12     // external button to initiate ringing
#define PIN_AUDIO_IN 32     // software DTMF and future live audio digitization

// Internal DAC pins        // just defining so I remember these are used by the audio libraries
#define PIN_AUDIO_OUT_L 25  // left  output channel pin for internal DAC
#define PIN_AUDIO_OUT_R 26  // right output channel pin for internal DAC

// SLIC module
#define PIN_SHK 13          // SLIC SHK, off-hook
#define PIN_RM  22          // SLIC RM, ring mode enable
#define PIN_FR  23          // SLIC FR, ring toggle, use PWM 50%
#define CH_FR   0           // SLIC FR, PWM channel

// DTMF module
#define PIN_Q1  36          // DTMF bit 1
#define PIN_Q2  39          // DTMF bit 2
#define PIN_Q3  34          // DTMF bit 3
#define PIN_Q4  35          // DTMF bit 4
#define PIN_STQ 27          // DTMF value ready to read

bool softwareDTMF = false;  // software DTMF decoding is finicky, so I want to switch between hardware and software DTMF decoders easily
bool detectMT870  = true;   // detect presence of hardware decoder module and fallback on software decoding if not installed

#endif