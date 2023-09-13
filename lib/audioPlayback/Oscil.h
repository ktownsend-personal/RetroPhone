/*
    All code in this file was copied from Mozzi's Oscil.h and mozzi_rand.cpp and simplified/reformatted to just what I need to keep it simple.
    https://sensorium.github.io/Mozzi/
*/

#ifndef OSCIL_H_
#define OSCIL_H_

#include "Arduino.h"

// fractional bits for oscillator index precision
#define OSCIL_F_BITS 16
#define OSCIL_F_BITS_AS_MULTIPLIER 65536

// testing dithering to improve frequency spurs; not sure if it's useful or audible; may only notice on scope
#define DITHER

template<typename T> inline T FLASH_OR_RAM_READ(T* address) {return (T) (*address);}

#ifdef DITHER
  // Random number generator. A faster replacement for Arduino's random function, which is too slow to use with Mozzi.  
  // Based on Marsaglia, George. (2003). Xorshift RNGs. http://www.jstatsoft.org/v08/i14/xorshift.pdf
  unsigned long xorshift96() { //period 2^96-1
    static unsigned long x=132456789, y=362436069, z=521288629;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;
    unsigned long t = x;
    x = y;
    y = z;
    return z = t ^ x ^ y;
  };
#endif

template <uint16_t NUM_TABLE_CELLS, uint16_t UPDATE_RATE>
class Oscil {

  public:
    Oscil(const int8_t * TABLE_NAME):table(TABLE_NAME){}

    inline int8_t next()	{
      phase_fractional += phase_increment_fractional;
      #ifdef DITHER
        return FLASH_OR_RAM_READ<const int8_t>(table + (((phase_fractional + ((int)(xorshift96()>>16))) >> OSCIL_F_BITS) & (NUM_TABLE_CELLS - 1)));
      #else
        return FLASH_OR_RAM_READ<const int8_t>(table + ((phase_fractional >> OSCIL_F_BITS) & (NUM_TABLE_CELLS - 1)));
      #endif
    }

    void setPhase(unsigned int phase)	{
      phase_fractional = (unsigned long)phase << OSCIL_F_BITS;
    }

    inline void setFreq (int frequency) {
      phase_increment_fractional = ((unsigned long)frequency) * ((OSCIL_F_BITS_AS_MULTIPLIER*NUM_TABLE_CELLS)/UPDATE_RATE);
    }

    inline int8_t atIndex(unsigned int index)	{
      return FLASH_OR_RAM_READ<const int8_t>(table + (index & (NUM_TABLE_CELLS - 1)));
    }

  private:
    unsigned long phase_fractional;
    unsigned long phase_increment_fractional;
    const int8_t * table;
};

#endif /* OSCIL_H_ */
