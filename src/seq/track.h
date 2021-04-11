#ifndef SYNTH_SEQ_TRACK_H
#define SYNTH_SEQ_TRACK_H

#include "parser.h"
#include "../instrument.h"
#include "../audio_system.h"
#include <stk/WvOut.h>
#include <stk/Stk.h>
#include <stack>
#include <set>
#include <list>
#include <vector>


class SeqController;

struct Slide
{
  uint8_t type;
  float start;
  float end;
  uint32_t duration;
  uint32_t t;
};

class SeqTrack
{
private:
  SeqParser *parser;
  SeqController *controller;
  uint32_t pc = 0;
  std::stack<uint32_t> callstack;
  std::list<Slide> slides;
  uint32_t delay_timer = 0;

  uint8_t trackid = 0;
  uint32_t loops = 0;
  
  float volume = 0;
  float pitch  = 0;
  float reverb = 0;
  float pan    = 0.5;

  SampleInstr instrument;
  std::vector<Note *> voices[7];

  void setPerf(uint32_t type, float v);
public:
  enum Step
  {
    STEP_OK = 0,
    STEP_WAITING,
    STEP_FINISHED,
    STEP_ERROR
  };

  SeqTrack(SeqController *controller, SeqParser *parser,
            uint32_t pc, uint8_t id, float samplerate);

  Step step();
  bool tick(stk::StkFrames &data);

  uint32_t getPC() { return pc; }
  float getPan() { return pan; }

  bool operator==(const SeqTrack &other) const
  {
    return memcmp(this, &other, sizeof(SeqTrack)) == 0;
  }
};

class SeqController
{
private:
  std::list<SeqTrack> tracks;
  std::vector<SeqTrack> newTracks;
  std::vector<SeqTrack *> oldTracks; 
  uint32_t tick_count = 0;
  float samplerate;

  stk::StkFrames tickBufL;
  stk::StkFrames tickBufR;

public:
  AudioSystem &audioSys;
  SeqParser &parser;

  uint16_t tempo = 0;
  uint16_t timebase = 0;

  SeqController(AudioSystem& system, SeqParser& parser, float samplerate);
  
  void addTrack(uint8_t id, uint32_t off);
  void removeTrack(SeqTrack *t);

  uint32_t getSamplesPerTick();
  float getSamplerate() { return samplerate; }

  bool tick(stk::WvOut &out);

};

#endif // SYNTH_SEQ_TRACK_H