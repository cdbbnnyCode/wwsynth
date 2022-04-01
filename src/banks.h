#ifndef SYNTH_BANKS_H
#define SYNTH_BANKS_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdint.h>

#include <iostream>
#include <fstream>

#include <stk/Stk.h>

struct Wave
{
  enum
  {
    FMT_ADPCM_4,
    FMT_PCM_8,
    FMT_PCM_16
  };

  uint8_t format;
  uint8_t base_key;
  float sample_rate;
  bool loop;
  uint32_t loop_start;
  uint32_t loop_end;
  uint32_t sample_count;

  uint32_t wavedata_offset;
  uint32_t wavedata_size;
  char aw_filename[0x70];

  uint16_t aw_id;
  uint16_t wave_id;

  stk::StkFrames data;
};

struct WaveEntry
{
  uint16_t aw_id;
  uint16_t wave_id;

  bool operator<(WaveEntry other) const
  {
    return aw_id < other.aw_id || wave_id < other.wave_id;
  }
};

class Wavesystem
{
private:
  std::map<WaveEntry, std::unique_ptr<Wave>> waves;
  uint32_t wsys_id = 0xFFFFFFFF;
  bool loaded = false;

public:
  Wavesystem(void);
  
  bool load(std::istream &f, std::string waves_path);
  bool isLoaded();
  Wave *getWave(uint16_t aw_id, uint16_t wave_id);
  uint32_t getNumWaves();
  uint32_t getWsysID() { return wsys_id; }

  static bool getID(std::istream &f, uint32_t *id);
};

struct KeyRgn; // it does exist, compiler

struct KeyInfo
{
  uint8_t maxVel;
  uint16_t awid;
  uint16_t waveid;

  float volume = 1;
  float pitch = 1;

  KeyRgn *rgn = nullptr;
};

struct KeyRgn
{
  uint8_t maxKey;

  float volume = 1;
  float pitch = 1;
  std::vector<std::unique_ptr<KeyInfo>> keys;
};

class KeyMap
{
private:
  std::vector<std::unique_ptr<KeyRgn>> regions;
public:
  bool isPercussion;

  KeyRgn *addRgn(uint8_t maxKey);

  KeyInfo *getKeyInfo(uint8_t key, uint8_t vel);
};

struct Envp
{
  enum
  {
    LINEAR,
    SQUARE,
    DIRECT,
    ROOT,
    LOOP = 0x0D,
    HOLD,
    STOP
  };
  uint16_t mode;
  uint16_t time;
  int16_t value;
};

struct Osci
{
  uint8_t mode;
  float rate;
  std::vector<Envp> atkEnv;
  std::vector<Envp> relEnv;
  float width;
  float vertex;
};

struct BankInstrument
{
  bool isPercussion;
  float volume = 1;
  float pitch = 1;
  KeyMap keys;
  Osci osci;
};

class IBNK
{
private:
  uint32_t wsysid = 0xFFFFFFFF;

  bool loaded = false;

public:
  static const uint32_t NUM_INSTRUMENTS = 245;
  std::unique_ptr<BankInstrument> instruments[NUM_INSTRUMENTS];

  IBNK(void);
  bool load(std::istream &f);
  bool isLoaded();

  uint32_t getWavesystemID()
  {
    return wsysid;
  }
};

#endif // SYNTH_BANKS_H