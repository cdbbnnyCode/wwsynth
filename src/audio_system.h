#ifndef SYNTH_AUDIO_SYSTEM_H
#define SYNTH_AUDIO_SYSTEM_H

#include <stk/Stk.h>

#include "aaf.h"
#include "instrument.h"

#include <vector>
#include <unordered_map>
#include <string>
#include <stdint.h>
#include <memory>

class AudioSystem
{
private:
  AAFFile aaf;

  std::unordered_map<uint32_t, std::unique_ptr<IBNK>> banks;
  std::unordered_map<uint32_t, std::unique_ptr<Wavesystem>> wavesystems;
  std::vector<std::unique_ptr<Note>> notes;

  std::string waves_path;

public:
  AudioSystem(std::string aaf_path, std::string waves_path);

  Note *getNewNote();
  IBNK *getBank(uint32_t id);
  Wavesystem *getWavesystem(uint32_t id);
  Wavesystem *getWsysFor(IBNK *bank);

  stk::StkFloat tickAllNotes();
  uint32_t getNumActiveNotes();
};

#endif // SYNTH_AUDIO_SYSTEM_H