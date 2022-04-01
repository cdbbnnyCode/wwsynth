#ifndef SYNTH_COMMON_WAVE_H
#define SYNTH_COMMON_WAVE_H

#include <vector>
#include <stdint.h>
#include <stddef.h>

namespace Common
{

class Wavedata
{
private:
  std::vector<float> data;
  uint32_t samplerate;

  bool looped = false;
  size_t loop_start = 0;
  size_t loop_end = 0;

public:
  Wavedata(size_t size=0, uint32_t samplerate=44100);
  Wavedata(float *data, size_t size, uint32_t samplerate=44100);
  Wavedata(uint16_t *data, size_t size, uint32_t samplerate=44100);

  void setLoopInfo(bool loop, size_t start, size_t end);
  
  uint32_t getSize() { return data.size(); }
  uint32_t getSamplerate() { return samplerate; }

  void put(float pt);
  void put(float *data, size_t size);
  void put(uint16_t *data, size_t size);

  void resize(size_t size);
  void setSamplerate(uint32_t samplerate);

  float &operator[](size_t pos);
  const float operator[](size_t pos) const;

  float get(float pos);
  float *getData(); // this pointer is invalidated if the size changes
};

struct Wave
{
  Wavedata wavedata;
  float volume;
  float pitch;
  uint8_t root_key;
};

} // namespace Common

#endif // SYNTH_COMMON_WAVE_H