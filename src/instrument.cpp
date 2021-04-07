#include "instrument.h"
#include "freq_table.h"

#include <cmath>

SampleInstr::SampleInstr(void) { }

void SampleInstr::setBank(IBNK *bank, Wavesystem *wsys)
{
  this->bank = bank;
  this->wsys = wsys;
}

void SampleInstr::setInstr(uint32_t instrument)
{
  if (this->bank == nullptr || this->wsys == nullptr)
  {
    printf("WARN: Bank not set before setting instrument. Instrument not changed.\n");
    return;
  }

  if (instrument >= bank->instruments.size())
  {
    printf("WARN: Invalid instrument ID %d\n");
    inst = nullptr;
    return;
  }

  this->inst = &this->bank->instruments[instrument];
}

bool SampleInstr::createNote(uint8_t key, uint8_t vel, Note *note)
{
  note->wave = nullptr;
  if (this->bank == nullptr || this->wsys == nullptr || this->inst == nullptr)
  {
    printf("WARN: Unable to create note before setBank() and setInstr() are called");
    return false;
  }

  KeyInfo *info = this->inst->keys.getKeyInfo(key, vel);
  if (info == nullptr) // no sound on that key; don't play anything
    return false;

  Wave *wave = this->wsys->getWave(0, info->waveid);
  if (wave == nullptr) // no wave in the wavesystem
  {
    printf("WARN: Unable to find wave %u in wavesystem %u\n", info->waveid, wsys->getWsysID());
    return false;
  }

  note->wave = wave;
  note->volume = inst->volume * info->volume * info->rgn->volume;
  note->pitch  = inst->pitch  * info->pitch  * info->rgn->pitch;

  note->key = key;
  note->vel = vel;
  note->adsr = stk::ADSR(); // TODO get ADSR data
  note->setOutputSampleRate(this->sample_rate);

  note->isPercussion = inst->isPercussion;

  return true;
}

void Note::start()
{
  if (playing) return;
  if (!isPlayable()) return;

  adsr.keyOn();
  position = 0;
  lastFrame[0] = 0;
  playing = true;
}

void Note::stop()
{
  adsr.keyOff();
}

void Note::reset()
{
  adsr.setValue(0);
  playing = false;
}

static stk::StkFloat getLoopedPos(stk::StkFloat pos, uint32_t loop_start, uint32_t loop_end)
{
  if (pos >= loop_end)
  {
    return std::fmod(pos - loop_start, loop_end - loop_start) + loop_start;
  }
  else
  {
    return pos;
  }
  
}

stk::StkFloat Note::tick()
{
  if (!playing) return 0;
  if (!isPlayable()) return 0;
  stk::StkFloat envValue = adsr.tick();

  if (envValue == 0)
  {
    playing = false;
    return 0;
  }

  if (!wave->loop && position >= wave->loop_end)
  {
    playing = false;
    return 0;
  }

  stk::StkFloat tick_delta = (samplerate / wave->sample_rate) * this->pitch;
  if (!isPercussion)
  {
    int diffNote = (int)key - wave->base_key;
    if (diffNote < 0) tick_delta /= MIDI_NOTES[-diffNote];
    else              tick_delta *= MIDI_NOTES[diffNote];
  }

  stk::StkFloat vel = ((stk::StkFloat)this->vel / 127);
  stk::StkFloat volume = envValue * this->volume * (vel * vel);

  stk::StkFloat start_pos = getLoopedPos(position, wave->loop_start, wave->loop_end);
  uint32_t start_sample = (uint32_t)start_pos;
  uint32_t end_sample   = (uint32_t)getLoopedPos(start_sample + 1, wave->loop_start, wave->loop_end);

  stk::StkFloat off = start_pos - start_sample;
  
  stk::StkFloat start = wave->data[start_sample];
  stk::StkFloat end   = wave->data[end_sample];

  stk::StkFloat sample = (end - start) * off + start;

  position += tick_delta;

  return sample * volume;
}

void Note::setOutputSampleRate(stk::StkFloat samplerate)
{
  adsr.setSampleRate(samplerate);
}
