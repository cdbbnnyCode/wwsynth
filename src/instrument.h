#ifndef SYNTH_INSTRUMENT_H
#define SYNTH_INSTRUMENT_H

#include <stdint.h>
#include <stk/Instrmnt.h>
#include <stk/Stk.h>
#include <stk/ADSR.h>
#include <vector>
#include <string>

#include "banks.h"

class Envelope
{
private:
  double pos;
  double inc;
  uint32_t curr_env;
  Osci *osci = nullptr;

  bool release = false;
  bool force_off = false;

  // previous envelope point
  Envp last_env;
  // last value generated
  stk::StkFloat last_val;
  // last value generated before release activated
  stk::StkFloat hold_val;
public:
  enum Status
  {
    EMPTY,
    ACTIVE,
    HOLD,
    FINISHED
  };

  void init(Osci *osci);

  void reset();

  void force_stop();

  void setSamplerate(uint32_t spt);

  Status getStatus();

  stk::StkFloat tick();

  stk::StkFloat getValue();

  const Osci *getOscillator() { return osci; }

  void beginRelease();
};

class Note
{
private:
  stk::StkFloat position = 0;
  bool finished = false;
  bool playing = false;
  stk::StkFrames lastFrame;
  stk::StkFloat samplerate;

public:
  Wave *wave;
  float volume;
  float pitch;

  uint8_t key;
  uint8_t vel;
  Envelope env;
  bool isPercussion;

  float volume_adj = 1;
  float pitch_adj = 1;

  Note(void)
  {
    lastFrame.resize(1, 1, 0.0);
  }

  bool isPlayable()
  {
    return this->wave != nullptr;
  }

  bool isFinished()
  {
    return this->finished;
  }

  bool isPlaying()
  {
    return playing;
  }

  void start();
  void stop();
  void stopNow();
  void reset();

  stk::StkFloat tick();

  void setOutputSampleRate(stk::StkFloat samplerate);
};

class SampleInstr
{
private:
  IBNK *bank = nullptr;
  Wavesystem *wsys = nullptr;
  BankInstrument *inst = nullptr;
  float sample_rate = 0;

public:
  SampleInstr(void);

  void setSampleRate(float samplerate) 
  {
    this->sample_rate = samplerate;
  }

  void setBank(IBNK *bank, Wavesystem *wsys);
  void setInstr(uint32_t instrument);
  bool isValid() { return bank != nullptr && wsys != nullptr && inst != nullptr; }

  bool createNote(uint8_t key, uint8_t vel, Note *note);
};


#endif // SYNTH_INSTRUMENT_H
