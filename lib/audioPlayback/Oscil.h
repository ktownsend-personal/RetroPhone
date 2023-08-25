/*
    All code in this file was copied from Mozzi's Oscil.h and simplified/reformatted to just what I need to keep it simple.
    https://sensorium.github.io/Mozzi/
*/

#ifndef OSCIL_H_
#define OSCIL_H_

#include "Arduino.h"

// fractional bits for oscillator index precision
#define OSCIL_F_BITS 16
#define OSCIL_F_BITS_AS_MULTIPLIER 65536

template<typename T> inline T FLASH_OR_RAM_READ(T* address) {return (T) (*address);}

template <uint16_t NUM_TABLE_CELLS, uint16_t UPDATE_RATE>
class Oscil {

  public:
    Oscil(const int8_t * TABLE_NAME):table(TABLE_NAME){}

    inline int8_t next()	{
      phase_fractional += phase_increment_fractional;
      return FLASH_OR_RAM_READ<const int8_t>(table + ((phase_fractional >> OSCIL_F_BITS) & (NUM_TABLE_CELLS - 1)));
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
