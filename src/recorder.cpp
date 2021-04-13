#include "banks.h"
#include "instrument.h"
#include "audio_system.h"
#include "seq/parser.h"
#include "seq/track.h"

#include <stdio.h>
#include <cmath>

#include <stk/FileWvOut.h>
// #include <stk/RtWvOut.h>

void test_audio(void)
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
}

void test_seq(void)
{
  std::ifstream f("../data/JaiSeqs/sea.bms");
  SeqParser parser;
  parser.load(f);

  uint32_t pc = 0;
  while (true)
  {
    std::unique_ptr<SeqCommand> cmd = parser.readCommand(pc);
    printf("%06x | ", pc);
    for (uint32_t i = 0; i < cmd->getSize(); i++)
    {
      uint32_t off = i + pc;
      if (off >= parser.seqdata.size()) printf("-- ");
      else printf("%02x ", (uint8_t)parser.seqdata[off]);
    }
    printf("| %s\n", cmd->getDisasm().c_str());
    if (dynamic_cast<BadCmd *>(cmd.get()) != nullptr)
      break;
    pc += cmd->getSize();
  }
}


void test_seq_play(char **argv, int argc)
{
  if (argc < 2) return;
  std::string fname = std::string(argv[1]);
  std::ifstream f(fname);
  f.get(); // try to read a byte
  if (f.fail()) return;
  f.seekg(0);

  SeqParser parser;
  parser.load(f);
  AudioSystem system("../data/JaiInit.aaf", "../data/Banks/");

  SeqController controller(system, parser, 44100);
  
  stk::Stk::setSampleRate(44100);
  stk::FileWvOut out(fname + ".wav");
  while (true)
  {
    if (!controller.tick(out)) break;
    
    uint32_t tick_count = controller.getTickCount();
  }

}

int main(int argc, char **argv)
{

  // test_audio();
  // test_seq();
  test_seq_play(argv, argc);
  
  return 0;
}