#include <string>
#include <fstream>
#include <istream>

#include "seq/parser.h"
#include "seq/track.h"
#include "audio/audio_out.h"
#include "audio_system.h"


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
  controller.loop_limit = -1;
  
  stk::Stk::setSampleRate(44100);
  SDLAudioOut out(44100);
  out.setVolume(0.5);
  while (true)
  {
    if (!controller.tick(out)) break;
    if (out.bad()) break;
  }
}

int main(int argc, char **argv)
{
  test_seq_play(argv, argc);
  return 0;
}