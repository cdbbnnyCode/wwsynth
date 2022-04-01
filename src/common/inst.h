#ifndef SYNTH_COMMON_INST_H
#define SYNTH_COMMON_INST_H

#include <vector>
#include "wave.h"

namespace Common
{

struct Note
{
  float volume;
  float pitch;
  Wave *wave;

  
};

class Instrument
{
protected:
  float volume;
  float pitch;

  virtual Wave *getWave(uint8_t key, uint8_t vel) = 0;
  virtual float getKeyPitch(uint8_t key);

public:
  
};

} // namespace Common

#endif // SYNTH_COMMON_INST_H