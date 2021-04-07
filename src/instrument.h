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
  stk::StkFloat position;
  bool playing;
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

  Note(void)
  {
    lastFrame.resize(1, 1, 0.0);
  }

  bool isPlayable()
  {
    return this->wave != nullptr;
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
