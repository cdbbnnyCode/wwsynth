#include "audio_system.h"

AudioSystem::AudioSystem(std::string aaf_path, std::string waves_path)
  : waves_path(waves_path), aaf(waves_path)
{
  this->aaf.load(aaf_path);
}

Note *AudioSystem::getNewNote()
{
  for (std::unique_ptr<Note> &n : notes)
  {
    if (n->isFinished())
    {
      n->reset(); // mark as in-use again
      return n.get();
    }
  }
  notes.push_back(std::move(std::make_unique<Note>()));
  return notes.back().get();
}

IBNK *AudioSystem::getBank(uint32_t id)
{
  if (banks.count(id) == 0)
  {
    banks[id] = std::move(std::make_unique<IBNK>(aaf.loadBank(id)));
  }
  return banks[id].get();
}

Wavesystem *AudioSystem::getWavesystem(uint32_t id)
{
  if (wavesystems.count(id) == 0)
  {
    wavesystems[id] = std::move(std::make_unique<Wavesystem>(aaf.loadWavesystem(id)));
  }
  return wavesystems[id].get();
}

Wavesystem *AudioSystem::getWsysFor(IBNK *bank)
{
  return getWavesystem(bank->getWavesystemID());
}

stk::StkFloat AudioSystem::tickAllNotes()
{
  stk::StkFloat total = 0;
  for (std::unique_ptr<Note> &note : notes)
  {
    if (note->isPlaying())
    {
      total += note->tick();
    }
  }

  return total;
}

uint32_t AudioSystem::getNumActiveNotes()
{
  uint32_t total = 0;
  for (std::unique_ptr<Note> &note : notes)
  {
    if (note->isPlaying())
    {
      total++;
    }
  }
  return total;
}
