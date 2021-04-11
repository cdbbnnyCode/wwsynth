#include "parser.h"

void SeqParser::load(std::istream &f, uint32_t cmdset)
{
  this->cmdset = cmdset;
  f.seekg(0, std::ios::end);
  uint32_t size = f.tellg();
  this->seqdata.resize(size);
  f.seekg(0);
  f.read((char *)(this->seqdata.data()), size);
}

std::unique_ptr<SeqCommand> SeqParser::readCommand(uint32_t pc)
{
  if (pc >= seqdata.size()) return std::make_unique<BadCmd>(BadCmd(0, BadCmd::ERR_EOF));
  uint8_t opcode = seqdata[pc];
  std::unique_ptr<SeqCommand> cmd;

  //////////////////////////////////////////////
  // OPCODE TABLE
  // currently defined:
  // * CmdNoteOn   : 0x00 - 0x7F
  // * CmdWait     : 0x80
  // * CmdVoiceOff : 0x81 - 0x87
  // * CmdWait     : 0x88
  // * CmdSetPerf  : 0x94 - 0x9F
  // * CmdSetParam : 0xA4
  // * CmdSetParam : 0xAC
  // * CmdOpenTrack: 0xC1
  // * CmdJump [c] : 0xC3
  // * CmdReturn   : 0xC5
  // * CmdJump [j] : 0xC7
  // * CmdTimebase : 0xFD
  // * CmdTempo    : 0xFE
  // * CmdTrackEnd : 0xFF

  if (opcode < 0x80)
    cmd = std::make_unique<CmdNoteOn>(CmdNoteOn());
  else if (opcode == 0x80)
    cmd = std::make_unique<CmdWait>(CmdWait());
  else if (opcode < 0x88)
    cmd = std::make_unique<CmdVoiceOff>(CmdVoiceOff());
  else if (opcode == 0x88)
    cmd = std::make_unique<CmdWait>(CmdWait());
  else if (opcode == 0x94 || opcode == 0x98)
    cmd = std::make_unique<CmdSetPerf>(CmdSetPerf(3));
  else if (opcode == 0x96 || opcode == 0x9A || opcode == 0x9C)
    cmd = std::make_unique<CmdSetPerf>(CmdSetPerf(4));
  else if (opcode == 0x97 || opcode == 0x9B || opcode == 0x9E)
    cmd = std::make_unique<CmdSetPerf>(CmdSetPerf(5));
  else if (opcode == 0x9F)
    cmd = std::make_unique<CmdSetPerf>(CmdSetPerf(6));
  else if (opcode == 0xA4)
    cmd = std::make_unique<CmdSetParam>(CmdSetParam(false));
  else if (opcode == 0xAC)
    cmd = std::make_unique<CmdSetParam>(CmdSetParam(true));
  else if (opcode == 0xC1)
    cmd = std::make_unique<CmdOpenTrack>(CmdOpenTrack());
  else if (opcode == 0xC3)
    cmd = std::make_unique<CmdJump>(CmdJump(true));
  else if (opcode == 0xC4)
    cmd = std::make_unique<CmdJumpF>(CmdJumpF(true));
  else if (opcode == 0xC5)
    cmd = std::make_unique<CmdReturn>(CmdReturn());
  else if (opcode == 0xC6)
    cmd = std::make_unique<CmdReturnF>(CmdReturnF());
  else if (opcode == 0xC7)
    cmd = std::make_unique<CmdJump>(CmdJump(false));
  else if (opcode == 0xC8)
    cmd = std::make_unique<CmdJumpF>(CmdJumpF(false));
  else if (opcode == 0xE6)
    cmd = std::make_unique<CmdIDontCare>(CmdIDontCare("vibrato", 3));
  else if (opcode == 0xE7)
    cmd = std::make_unique<CmdIDontCare>(CmdIDontCare("sync cpu", 3));
  else if (opcode == 0xF4)
    cmd = std::make_unique<CmdIDontCare>(CmdIDontCare("vibrato pitch", 2));
  else if (opcode == 0xFD)
    cmd = std::make_unique<CmdTempo>(CmdTempo());
  else if (opcode == 0xFE)
    cmd = std::make_unique<CmdTimebase>(CmdTimebase());
  else if (opcode == 0xFF)
    cmd = std::make_unique<CmdTrackEnd>(CmdTrackEnd());
  else
    return std::make_unique<BadCmd>(BadCmd(1, BadCmd::ERR_INVALID_OPCODE));

  if (pc + cmd->getSize() > seqdata.size())
  {
    return std::make_unique<BadCmd>(BadCmd(cmd->getSize(), BadCmd::ERR_EOF));
  }

  uint32_t status = cmd->read(seqdata, pc);
  if (status != 0)
  {
    return std::make_unique<BadCmd>(BadCmd(cmd->getSize(), status));
  }

  return cmd;
}