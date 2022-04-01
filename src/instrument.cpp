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
    printf("createNote: Note is null\n");
    return false;
  }

  note->wave = nullptr;
  if (this->bank == nullptr || this->wsys == nullptr || this->inst == nullptr)
  {
    printf("createNote: Unable to create note before setBank() and setInstr() are called");
    return false;
  }

  KeyInfo *info = this->inst->keys.getKeyInfo(key, vel);
  if (info == nullptr) // no sound on that key; don't play anything
  {
    printf("createNote: no key/velocity region for key %u vel %u\n", key, vel);
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


  // note->dbg_rel *= (note->vel / 128.0f) * 2;

  note->env.init(&(inst->osci));
  
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

  position = 0;
  lastFrame[0] = 0;
  playing = true;
}

void Note::stop()
{
  env.beginRelease();
}

void Note::reset()
{
  env.reset();
  playing = false;
  finished = false;
}

void Note::stopNow()
{
  env.force_stop();
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
  stk::StkFloat envValue = env.tick();

  if (env.getStatus() == Envelope::FINISHED)
  {
    // printf("note end\n");
    playing = false;
    finished = true;
    return 0;
  }

  if (!wave->loop && position >= wave->loop_end)
  {
    // printf("past wave end @ %f (vol=%08x pitch=%08x)\n", position, *(uint32_t *)&volume, *(uint32_t *)&pitch);
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
  stk::StkFloat volume = envValue * (this->volume * this->volume) * (vel) * (volume_adj);

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
  env.setSamplerate(samplerate);
}

// Envelope

void Envelope::init(Osci *osc)
{
  this->osci = osc;
  this->reset();
}

void Envelope::reset()
{
  this->pos = 0;
  this->curr_env = 0;
  this->last_env = {0, 0, 0}; // generic envelope point
  this->release = false;
  this->force_off = false;
  this->last_val = 0;
  this->hold_val = 0;
}

void Envelope::force_stop()
{
  this->force_off = true;
}

void Envelope::setSamplerate(uint32_t tickrate)
{
  // take reciprocal
  this->inc = 1000.0 / tickrate;
}

static double convertEnvValue(int16_t v)
{
  return v / 32767.0; // may need this to be logarithmic
}

stk::StkFloat Envelope::tick()
{
  // tick (1 sample)
  // are we in stop or hold?
  Envelope::Status status = getStatus();
  stk::StkFloat next_value;
  if (status == Envelope::HOLD)
  {
    next_value = convertEnvValue(last_env.value);
  }
  else if (status == Envelope::FINISHED)
  {
    next_value = 0;
  }
  else
  {
    // check if we're on the next vertex
    std::vector<Envp> &env_data = release ? osci->relEnv : osci->atkEnv;

    if (curr_env >= env_data.size()-1)
    {
      // shouldn't be here
      printf("bad\n");
      return 0;
    }

    Envp next_data = env_data[curr_env];

    bool new_env = false;
    if (pos >= next_data.time)
    {
      // printf("t=%.3f > %d - update to %d\n", pos, next_data.time, curr_env+1);
      last_env = env_data[curr_env];
      curr_env++;
      next_data = env_data[curr_env];
      new_env = true;
    }

    // total width of this vertex
    double dt = next_data.time - last_env.time;
    // position into that time
    double t = pos - last_env.time;
    // starting value
    double y;
    // the only time we have 0xff in last_env is after beginRelease was called.
    if (last_env.mode == 0xff) y = hold_val;
    else y = convertEnvValue(last_env.value);
    // value difference
    double dy = convertEnvValue(next_data.value) - y;

    /*
    if (new_env)
    {
      printf("t=%.3f [%u]: %02x over %.0f ticks from %.3f to %.3f\n", pos, curr_env, next_data.mode, dt, y, y+dy);
    }
    */

    // increment time
    // TODO test if this should increment by inc or just 1
    pos += inc;

    if (next_data.mode == Envp::LINEAR)
    {
      next_value = y + dy * (t / dt);
    }
    else if (next_data.mode == Envp::SQUARE)
    {
      next_value = y + dy * (t / dt) * (t / dt);
    }
    else if (next_data.mode == Envp::ROOT)
    {
      next_value = y + dy * std::sqrt(t / dt);
    }
    else if (next_data.mode == Envp::DIRECT)
    {
      next_value = y + dy * 1;
    }
    else if (next_data.mode == Envp::HOLD)
    {
      next_value = y;
    }
    else
    {
      // stop or invalid
      next_value = 0;
    }
  }

  last_val = next_value;
  return next_value;
}

Envelope::Status Envelope::getStatus()
{
  if (osci == nullptr) return EMPTY;
  else if (force_off) return FINISHED;
  else
  {
    std::vector<Envp> &env_data = release ? osci->relEnv : osci->atkEnv;
    Envp &env = env_data[curr_env];

    if (env.mode == Envp::STOP) return FINISHED;
    else if (env.mode == Envp::HOLD) return HOLD;
    else return ACTIVE;
  }
}

void Envelope::beginRelease()
{
  if (!release)
  {
    // printf("release\n");
    release = true;
    pos = 0;
    curr_env = 0;
    last_env = {0xff, 0, 0};

    hold_val = last_val;
  }
  else
  {
    printf("Release called twice!\n");
  }
}

stk::StkFloat Envelope::getValue()
{
  return last_val;
}