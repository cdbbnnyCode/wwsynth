#ifndef SYNTH_BANKS_H
#define SYNTH_BANKS_H

#include <string>
#include <vector>
#include <map>
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
  std::map<WaveEntry, Wave> waves; // TODO change this into an unordered_map?
  uint32_t wsys_id;
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
  std::vector<KeyInfo> keys;
};

class KeyMap
{
private:
  std::vector<KeyRgn> regions;
public:
  bool isPercussion;

  KeyRgn &addRgn(uint8_t maxKey);

  KeyInfo *getKeyInfo(uint8_t key, uint8_t vel);
};

struct BankInstrument
{
  bool isPercussion;
  float volume = 1;
  float pitch = 1;
  KeyMap keys;
};

class IBNK
{
private:
  uint32_t wsysid;

  bool loaded = false;

public:
  std::vector<BankInstrument> instruments;

  IBNK(void);
  bool load(std::istream &f);
  bool isLoaded();

  uint32_t getWavesystemID();
};

#endif // SYNTH_BANKS_H