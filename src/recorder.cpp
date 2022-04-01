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

  uint32_t bank_id = 60;
  uint32_t inst_id = 0;

  IBNK *bank = system.getBank(bank_id);
  Wavesystem *wsys = system.getWsysFor(bank);

  SampleInstr inst;
  inst.setSampleRate(44100);
  
  inst.setBank(bank, wsys);
  inst.setInstr(inst_id);
  
  stk::Stk::setSampleRate(44100);
  stk::FileWvOut out("test.wav");

  Note *n = system.getNewNote();
  inst.createNote(60, 63, n);

  n->env.setSamplerate(44100);

  for (int j = 0; j < 3; j++)
  {
    printf("atk: ");
    for (const Envp &env : n->env.getOscillator()->atkEnv)
    {
      printf("%02x: t=%d -> %d, ", env.mode, env.time, env.value);
    }
    printf("\n");
    printf("rel: ");
    for (const Envp &env : n->env.getOscillator()->relEnv)
    {
      printf("%02x: t=%d -> %d, ", env.mode, env.time, env.value);
    }
    printf("\n");


    n->start();
    for (int i = 0; i < 441000; i++)
    {
      stk::StkFloat v = system.tickAllNotes();
      out.tick(v);
    }

    n->stop();

    int i = 0;
    while (system.getNumActiveNotes() != 0)
    {
      stk::StkFloat v = system.tickAllNotes();
      out.tick(v);
      i++;
    }
    printf("rel for %d samples\n", i);

    n->reset();
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
  stk::FileWvOut out(fname + ".wav", 2);
  while (true)
  {
    if (!controller.tick(out)) break;
    
    uint32_t tick_count = controller.getTickCount();
  }

}

int main(int argc, char **argv)
{

  test_audio();
  // test_seq();
  // test_seq_play(argv, argc);
  
  return 0;
}