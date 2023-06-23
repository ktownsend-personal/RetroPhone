/*
Heavily modified by El_Supremo (Arduino forums) to decode DTMF tones
It is also public domain and provided on an AS-IS basis. There's no warranty
or guarantee of ANY kind whatsoever.

The Goertzel algorithm is long standing so see
http://en.wikipedia.org/wiki/Goertzel_algorithm for a full description.
It is often used in DTMF tone detection as an alternative to the Fast
Fourier Transform because it is quick with low overheard because it
is only searching for a single frequency rather than showing the
occurrence of all frequencies.
This work is entirely based on the Kevin Banks code found at
http://www.eetimes.com/design/embedded/4024443/The-Goertzel-Algorithm
so full credit to him for his generic implementation and breakdown. I've
simply massaged it into an Arduino library. I recommend reading his article
for a full description of whats going on behind the scenes.

Created by Jacob Rosenthal, June 20, 2012.
Released into the public domain.
*/

// ensure this library description is only included once
#ifndef DTMF_h
#define DTMF_h

// include types & constants of Wiring core API
#include "Arduino.h"

//#define DTMF_DEBUG

// library interface description
class PhoneDTMF
{
private:
  static const uint8_t  TONES = 8;
  const uint16_t DTMF_TONES[TONES] =
    {
      697,
      770,
      852,
      941,
     1209,
     1336,
     1477,
     1633
    };
  const uint8_t  DTMF_MAP[TONES * 2] = {0x11,0x21,0x41,0x12,0x22,0x42,0x14,0x24,0x44,0x28,0x81,0x82,0x84,0x88,0x18,0x48};
  const char  DTMF_CHAR[TONES * 2] =   { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'A', 'B', 'C', 'D', '*', '#'};
    // Number of samples to detect tone (normally from 50 to 200)
  int16_t	_iSamplesCount;
	  // Maximal frequence for sampling => is calculated in the Init function
    // Note: this should be at least 6000Hz, since the measurement is done with this frequence
  uint32_t	_iSamplesFrequence;
	// The real sampling frequence from the last measurement
  uint32_t  _iRealFrequence;
	// If the real frequence is not like expected, a compensation delay is applied
  uint16_t	_iCompensation;
	// For detecting the tone, coefficient must be calculated => done in the Init function
  float		_afToneCoeff[TONES];
	// Q for each tone, to find out which frequence has detected. It will be reset in the resetDTMF function
  float		_afQ1[TONES];
	// Q for each tone, to find out which frequence has detected. It will be reset in the resetDTMF function
  float		_afQ2[TONES];
	// Pin used for the analog to digital conversion
  uint8_t	_Pin;
	// The value of ADC if nothing is detected (or in stand by) => this is calculated in the Init function
  int16_t	_iAdcCentre;
  float		_fAdcCentre;
	// Magnitude if nothing is detected (or in stand by) => calculated in the Init function
  float		_fBaseMagnitude;
    // Amplify measured signal, so it will be detected better (only if the signal does not cover the full amplitude)
  float     _fAmplifier;
#ifdef DTMF_DEBUG
  uint16_t  _aiAnalogData[128];
  uint8_t	_iDataIndex;
#endif
  
  // user-accessible "public" interface
public:
    
  PhoneDTMF(int16_t sampleCount = 128, float amplifier = 1.0f);
  uint16_t begin(uint8_t sensorPin, uint32_t maxFrequence = 0);
  uint8_t detect(float* pMagnitudes = NULL, float magnitude = -1.0f);
  char tone2char(uint8_t dtmf);
	
  uint32_t getSampleFrequence();
  uint32_t getRealFrequence();
  uint16_t getAnalogCenter();
  uint16_t getBaseMagnitude();
  uint16_t getMeasurementTime();

#ifdef DTMF_DEBUG
  uint16_t* getMeasurements();
#endif
  
  // library-accessible "private" interface
private:
  //El_Supremo change to int
  void ProcessSample(int16_t);
  void ResetDTMF(void);    
  void singleDetect();
  uint8_t calculateMeasurement(float* pRet = NULL, float magnitude = -1.0f);
};

#endif
