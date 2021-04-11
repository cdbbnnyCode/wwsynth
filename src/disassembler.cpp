#include <string>
#include <istream>
#include <fstream>
#include "seq/parser.h"

int main(int argc, char **argv)
{
  if (argc < 2) return 1;
  std::ifstream f = std::ifstream(argv[1], std::ios::binary);
  if (!f) return 1;

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

  return 0;
}