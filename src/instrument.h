#ifndef SYNTH_INSTRUMENT_H
#define SYNTH_INSTRUMENT_H

#include <stdint.h>
#include <stk/Instrmnt.h>
#include <stk/Stk.h>
#include <stk/ADSR.h>
#include <vector>
#include <string>

#include "banks.h"

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
  stk::ADSR adsr;
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

  bool createNote(uint8_t key, uint8_t vel, Note *note);
};


#endif // SYNTH_INSTRUMENT_H
