#include "banks.h"
#include "instrument.h"
#include "audio_system.h"

#include <stdio.h>
#include <cmath>

#include <stk/FileWvOut.h>

int main(void)
{

  AudioSystem system("../data/JaiInit.aaf", "../data/Banks/");

  uint32_t bank_id = 0;
  uint32_t inst_id = 0;

  IBNK *bank = system.getBank(bank_id);
  Wavesystem *wsys = system.getWsysFor(bank);

  SampleInstr inst;
  inst.setSampleRate(44100);
  
  inst.setBank(bank, wsys);
  inst.setInstr(inst_id);
  
  stk::Stk::setSampleRate(44100);
  stk::FileWvOut out("test.wav");

  for (int i = 60; i < 72; i++)
  {
    Note *n = system.getNewNote();
    Note *n2 = system.getNewNote();
    inst.createNote(i, 63, n);
    inst.createNote(i + 8, 63, n2);
    // printf("note %d: wave %d\n", i, n->wave->wave_id);
    n->start();
    n2->start();

    for (int j = 0; j < (44100); j++)
    {
      double t = (j / 44100.0) * 6.283185;
      n->pitch_adj = 1 + 0.1 * std::sin(t * 3);
      stk::StkFloat v = system.tickAllNotes();
      out.tick(v);
    }

    n->stop();
    n2->stop();
  }

  while (system.getNumActiveNotes() != 0)
  {
    stk::StkFloat v = system.tickAllNotes();
    out.tick(v);
  }
  
  return 0;
}