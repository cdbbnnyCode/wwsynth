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

  if (instrument >= IBNK::NUM_INSTRUMENTS)
  {
    printf("WARN: Invalid instrument ID %d\n");
    inst = nullptr;
    return;
  }

  this->inst = this->bank->instruments[instrument].get();
  if (this->inst == nullptr)
  {
    printf("WARN: Invalid instrument %d\n");
  }
}

bool SampleInstr::createNote(uint8_t key, uint8_t vel, Note *note)
{
  if (note == nullptr)
  {
    printf("WARN: note is null\n");
    return false;
  }

  note->wave = nullptr;
  if (this->bank == nullptr || this->wsys == nullptr || this->inst == nullptr)
  {
    printf("WARN: Unable to create note before setBank() and setInstr() are called");
    return false;
  }

  KeyInfo *info = this->inst->keys.getKeyInfo(key, vel);
  if (info == nullptr) // no sound on that key; don't play anything
  {
    printf("no key/velocity region for key %u vel %u\n", key, vel);
    return false;
  }

  Wave *wave = this->wsys->getWave(0, info->waveid);
  if (wave == nullptr) // no wave in the wavesystem
  {
    printf("WARN: Unable to find wave %u in wavesystem %u\n", info->waveid, wsys->getWsysID());
    return false;
  }

  note->wave = wave;
  note->volume = inst->volume * info->volume * info->rgn->volume;
  note->pitch  = inst->pitch  * info->pitch  * info->rgn->pitch;
  // printf("inst vol=%f; info vol=%f; rgn vol=%f\n", inst->volume, info->volume, info->rgn->volume);

  note->key = key;
  note->vel = vel;
  note->adsr = stk::ADSR(); // TODO get ADSR data
  
  note->adsr.setAttackTime(((float)inst->osci.attack / 32767) * 10 + 0.0001);
  note->adsr.setDecayTime(((float)inst->osci.decay / 32767) * 10 + 0.0001);
  note->adsr.setSustainLevel(((float)inst->osci.sustain / 32767));
  note->adsr.setReleaseTime(((float)inst->osci.release / 32767) * 10 + 0.0001);
  
  note->setOutputSampleRate(this->sample_rate);

  note->isPercussion = inst->isPercussion;
  /*
  if (inst->isPercussion)
  {
    printf("create percussion note %u; wave id=%u\n", key, wave->wave_id);
  }
  */

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
  finished = false;
}

void Note::stopNow()
{
  adsr.setValue(0);
  playing = false;
  finished = true;
}

static stk::StkFloat getLoopedPos(stk::StkFloat pos, uint32_t loop_start, uint32_t loop_end)
{
  if (pos >= loop_end - 1)
  {
    return std::fmod(pos - loop_start, loop_end - loop_start - 1) + loop_start;
  }
  else
  {
    return pos;
  }
  
}

stk::StkFloat Note::tick()
{
  if (!playing)
  {
    finished = true;
    return 0;
  }
  if (!isPlayable())
  {
    playing = false;
    finished = true;
    return 0;
  }
  stk::StkFloat envValue = adsr.tick();

  if (envValue == 0)
  {
    // printf("note end\n");
    playing = false;
    finished = true;
    return 0;
  }

  if (!wave->loop && position >= wave->loop_end)
  {
    printf("past wave end @ %f (vol=%08x pitch=%08x)\n", position, *(uint32_t *)&volume, *(uint32_t *)&pitch);
    playing = false;
    finished = true;
    return 0;
  }

  double tick_delta = ((double)wave->sample_rate / (double)samplerate) * (double)this->pitch * (double)pitch_adj;
  if (!isPercussion)
  {
    tick_delta *= MIDI_NOTES[key] / MIDI_NOTES[wave->base_key];
  }
  

  stk::StkFloat vel = ((stk::StkFloat)this->vel / 127);
  stk::StkFloat volume = envValue * this->volume * (vel * vel) * volume_adj;

  stk::StkFloat start_pos = getLoopedPos(position, wave->loop_start + 1, wave->loop_end);
  uint32_t start_sample = (uint32_t)start_pos;
  uint32_t end_sample   = (uint32_t)getLoopedPos(start_sample + 1, wave->loop_start + 1, wave->loop_end);

  stk::StkFloat off = start_pos - start_sample;
  if (end_sample > wave->loop_end) printf("end passed loop end\n");
  
  stk::StkFloat start = wave->data[start_sample];
  stk::StkFloat end   = wave->data[end_sample];

  stk::StkFloat sample = (end - start) * off + start;

  // printf("start=%u end=%u x=%.4f -> %.4f\n", start_sample, end_sample, off, sample);

  position += tick_delta;

  return sample * volume;
}

void Note::setOutputSampleRate(stk::StkFloat samplerate)
{
  this->samplerate = samplerate;
  adsr.setSampleRate(samplerate);
}
