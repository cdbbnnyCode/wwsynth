#ifndef SYNTH_SEQ_PARSER_H
#define SYNTH_SEQ_PARSER_H

#include <vector>
#include <stdint.h>
#include <istream>
#include <string>
#include <memory>

class SeqCommand
{
protected:
  uint32_t size = 0;
public:
  SeqCommand(uint32_t size) : size(size) {}

  virtual uint32_t read(std::vector<unsigned char> &data, uint32_t off) = 0;

  
  virtual uint8_t getParamCount() = 0;
  virtual uint8_t getParamWidth(uint8_t p) = 0;
  virtual void setParam(uint8_t p, uint32_t v) = 0;

  virtual std::string getDisasm() = 0;
  uint32_t getSize() {return size;}
};

class BadCmd : public SeqCommand
{
private:
  uint32_t errcode;
public:
  enum
  {
    ERR_EOF = 1,
    ERR_INVALID_OPCODE,
    ERR_INVALID_DATA
  };

  BadCmd(uint32_t size, uint32_t errcode) : SeqCommand(size), errcode(errcode) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override { return 0; }
  uint8_t getParamCount() override {return 0;}
  uint8_t getParamWidth(uint8_t p) override {return 0;}
  void setParam(uint8_t p, uint32_t v) override {}
  std::string getDisasm() override {return "[invalid]";}

  uint32_t getError() {return errcode;}
};

class CmdIDontCare : public SeqCommand
{
private:
  const std::string msg;
public:
  CmdIDontCare(const std::string msg, uint32_t size)
    : msg(msg), SeqCommand(size) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override { return 0; }
  uint8_t getParamCount() override {return 0;}
  uint8_t getParamWidth(uint8_t p) override {return 0;}
  void setParam(uint8_t p, uint32_t v) override {}
  std::string getDisasm() override {return msg;}
};

class SeqParser
{
private:
  uint32_t cmdset;

public:
  std::vector<unsigned char> seqdata;
  enum CmdSet
  {
    /*
      command set for the "original" JAudio (WW and other GC titles);
      does not support registers, debug messages, or register commands
    */
    JAUDIO_1 = 0x01, 
    /*
      command set for the "improved" JAudio2 (TP and Wii titles such as SMG/SMG2)
      uses registers for timebase, reverb, vibrato, and other extra effects
      and ports for communicating with the main program and dynamically
      jumping to different parts of the sequence
    */
    JAUDIO_2 = 0x02
  };

  SeqParser() {}

  void load(std::istream &f, uint32_t cmdset=JAUDIO_1);
  std::unique_ptr<SeqCommand> readCommand(uint32_t pc);
};

/////////////////////////////////////////////////////////////////////////////////////
// COMMANDS (YES THERE ARE A LOT OF THESE)

/* Full (known) comand set
  
  0x00 .. 0x7F : CmdNoteOn   <voice:u8> <velocity:u8>
          0x80 : CmdWait     <delay:u8>
  0x81 .. 0x87 : CmdVoiceOff <voice:u8>
          0x88 : CmdWait     <delay:u16 ------------>
 [0x89 .. 0x8F : *unused*]
  0x90 .. 0x9F : RegisterCmd <value:u8-u24 ----------------------> <command...> [JAudio2]
 [0xA0 .. 0xA3 : *unused*]
          0xA4 : CmdSetParam <value:u8>                                         [JAudio1]
 [0xA5 .. 0xAB : *unused*]
          0xAC : CmdSetParam <value:u16 ------------>                           [JAudio1]
 [0xAD .. 0xB7 : *unused*]
          0xB8 : CmdSetPerf  <value:u8>                                         [JAudio2]
          0xB9 : CmdSetPerf  <value:u16 ------------>                           [JAudio2]
 [0xBA .. 0xC0 : *unused*]
          0xC1 : CmdOpenTrack<track:u8> <offset:u24 -------------------------->
          0xC2 : [known but unused]
          0xC3 : CmdCall     <offset:u24 ------------------------>
          0xC4 : CmdCallF    <cond: u8> <offset:u24 -------------------------->

*/

/*
  Note On: opcode = {0x00 - 0x7F}
  <opcode = note> <voice: u8> <vel: u8>
  -> voice must be between 1 and 7
  -> vel must be between 0 and 127
*/
class CmdNoteOn : public SeqCommand
{
private:
  uint8_t note = 0;
  uint8_t voice = 0;
  uint8_t vel = 0;

public:
  CmdNoteOn() : SeqCommand(3) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override
  {
    note = data[off];
    if (note >= 0x80) return BadCmd::ERR_INVALID_DATA;

    voice = data[off+1];
    if (voice >= 8 || voice == 0) return BadCmd::ERR_INVALID_DATA;

    vel = data[off+2];
    if (vel >= 128) return BadCmd::ERR_INVALID_DATA;

    return 0;
  }

  uint8_t getParamCount() override { return 2; }

  uint8_t getParamWidth(uint8_t param) override
  {
    if (param < 2) return 1;
    else return 0;
  }

  void setParam(uint8_t p, uint32_t v) override
  {
    if (p == 0) voice = v;
    else if (p == 1) vel = v;
  }

  std::string getDisasm() override
  {
    return "note " + std::to_string(note) + " voice=" + std::to_string(voice) + " vel=" + std::to_string(vel);
  }

  uint8_t getNote() { return note; }
  uint8_t getVoice() { return voice; }
  uint8_t getVelocity() { return vel; }
};

/*
  Voice Off: opcode = {0x81 - 0x87}
  <voice: opcode & 0x7>
*/
class CmdVoiceOff : public SeqCommand
{
private:
  uint8_t voice = 0;

public:
  CmdVoiceOff() : SeqCommand(1) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override
  {
    voice = data[off] & 0x7;
    if (voice == 0) return BadCmd::ERR_INVALID_DATA;
    return 0;
  }

  uint8_t getParamCount() override { return 0; }
  uint8_t getParamWidth(uint8_t p) override { return 0; }
  void setParam(uint8_t p, uint32_t v) override {}

  std::string getDisasm() override
  {
    return "voice off " + std::to_string(voice);
  }

  uint8_t getVoice() { return voice; }
};

/*
  Handles delay from several opcodes:
  0xF0 <delay: vint>
  0xF1 <delay: u8>
  0x80 <delay: u8>
  0x88 <delay: u16>
*/
class CmdWait : public SeqCommand
{
private:
  uint32_t delay = 0;

public:
  CmdWait() : SeqCommand(5) {} // 5 bytes is the maximum possible size of this command

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override
  {
    uint8_t opcode = data[off];
    if (opcode == 0xF0) 
    {
      // read variable length (MIDI-style) int
      uint32_t v = 0;
      uint32_t o = 0;
      for (; o < 4; o++)
      {
        uint8_t b = data[off + o + 1];
        v |= (b & 0x7F);
        if (!(b & 0x80)) break;
        v <<= 7;
      }
      size = 2 + o;
      delay = v;
    }
    else if (opcode == 0xF1 || opcode == 0x80)
    {
      delay = data[off+1];
      size = 2;
    }
    else if (opcode == 0x88)
    {
      delay = (data[off+1] << 8) | data[off+2];
      size = 3;
    }
    else
    {
      size = 1;
      return BadCmd::ERR_INVALID_DATA;
    }
    return 0;
  }

  /* don't even bother */
  uint8_t getParamCount() override { return 0; }
  uint8_t getParamWidth(uint8_t p) override { return 0; }
  void setParam(uint8_t p, uint32_t v) override {}

  std::string getDisasm() override
  {
    return "delay " + std::to_string(delay);
  }

  uint32_t getDelay() { return delay; }
};

/*
  Open Track: 0xC1 <num:u8> <off:u24>
*/
class CmdOpenTrack : public SeqCommand
{
private:
  uint8_t trackn = 0;
  uint32_t offset = 0;

public:
  CmdOpenTrack() : SeqCommand(5) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override
  {
    trackn = data[off+1];
    offset = (data[off+2] << 16) | (data[off+3] << 8) | data[off+4];
    return 0;
  }

  uint8_t getParamCount() override { return 2; }
  uint8_t getParamWidth(uint8_t p) override
  {
    if (p == 0) return 1;
    else if (p == 1) return 3;
    else return 0;
  }
  void setParam(uint8_t p, uint32_t v) override
  {
    if (p == 0) trackn = v;
    else if (p == 1) offset = v;
  }

  std::string getDisasm() override
  {
    char hex[8];
    snprintf(hex, 7, "%06x", offset);
    hex[7] = 0;
    return "open track " + std::to_string(trackn) + " @ " + std::string(hex);
  }

  uint8_t getTrackID() { return trackn; }
  uint32_t getOffset() { return offset; }
};

/*
  Track End: 0xFF
*/
class CmdTrackEnd : public SeqCommand
{
public:
  CmdTrackEnd() : SeqCommand(1) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override
  {
    return 0;
  }

  uint8_t getParamCount() override { return 0; }
  uint8_t getParamWidth(uint8_t p) override { return 0; }
  void setParam(uint8_t p, uint32_t v) override {}

  std::string getDisasm() override
  {
    return "track end";
  }
};

/*
  Perf (with duration): set track volume/pitch/reverb/pan. available in JAudio 1 only
  <opcode> <type:u8> <value> [duration]
  0x94: value:u8, no duration
  [0x95 invalid]
  0x96: value:u8, duration:u8
  0x97: value:u8, duration:u16

  0x98: value:s8, no duration
  [0x99 invalid]
  0x9A: value:s8, duration:u8
  0x9B: value:s8, duration:u16

  0x9C: value:s16, no duration
  [0x9D invalid]
  0x9E: value:s16, duration:u8
  0x9F: value:s16, duration:u16
*/

class CmdSetPerf : public SeqCommand
{
private:
  uint8_t type = 0;
  int32_t value = 0;
  uint32_t duration = 0;
  bool wideValue = false;
public:
  CmdSetPerf(uint32_t size) : SeqCommand(size) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override
  {
    type = data[off+1];
    uint32_t value_size = 1;
    switch (data[off] & 0xFC)
    {
    case 0x94:
      value = data[off+2];
      break;
    case 0x98:
      value = (int8_t)data[off+2];
      break;
    case 0x9C:
      value = (int16_t)((data[off+2] << 8) | data[off+3]);
      wideValue = true;
      value_size = 2;
      break;
    default:
      return BadCmd::ERR_INVALID_DATA;
    }

    switch (data[off] & 0x03)
    {
    case 0x00:
      break; // no duration
    case 0x02:
      duration = data[off + 2 + value_size];
      break;
    case 0x03:
      duration = (data[off + 2 + value_size] << 8) | data[off + 3 + value_size];
      break;
    default:
      return BadCmd::ERR_INVALID_DATA;
    }

    return 0;
  }

  /* JAudio 1 doesn't use these */
  uint8_t getParamCount() override { return 0; }
  uint8_t getParamWidth(uint8_t p) override { return 0; }
  void setParam(uint8_t p, uint32_t v) override {}

  std::string getDisasm() override
  {
    return "set perf " + std::to_string(type) + " -> " + std::to_string(value) + " over " + std::to_string(duration) + " ticks";
  }

  uint8_t getType() { return type; }
  int32_t getValue() { return value; }
  uint32_t getDuration() { return duration; }
  bool isWide() { return wideValue; }
};

/*
  Tempo: 0xFE <tempo:u16>
*/
class CmdTempo : public SeqCommand
{
private:
  uint16_t tempo = 0;
public:
  CmdTempo() : SeqCommand(3) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override
  {
    tempo = (data[off+1] << 8) | data[off+2];
    return 0;
  }

  uint8_t getParamCount() override { return 1; }
  uint8_t getParamWidth(uint8_t p) override { return 2; }
  void setParam(uint8_t p, uint32_t v) override
  {
    if (p == 0) tempo = v;
  }

  std::string getDisasm() override
  {
    return "tempo " + std::to_string(tempo);
  }

  uint16_t getTempo() { return tempo; }
};

/*
  CmdTimebase : 0xFD <timebase:u16>
*/
class CmdTimebase : public SeqCommand
{
private:
  uint16_t timebase = 0;
public:
  CmdTimebase() : SeqCommand(3) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override
  {
    timebase = (data[off+1] << 8) | data[off+2];
    return 0;
  }

  uint8_t getParamCount() override { return 1; }
  uint8_t getParamWidth(uint8_t p) override { return 2; }
  void setParam(uint8_t p, uint32_t v) override
  {
    timebase = v;
  }

  std::string getDisasm() override
  {
    return "timebase " + std::to_string(timebase);
  }

  uint16_t getTimebase() { return timebase; }
};

/*
  Jump: 0xC7
  Call: 0xC3
*/
class CmdJump : public SeqCommand
{
private:
  uint32_t offset = 0;
  bool doCall;
public:

  CmdJump(bool call) : SeqCommand(4), doCall(call) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override
  {
    offset = (data[off+1] << 16) | (data[off+2] << 8) | data[off+3];
    return 0;
  }

  uint8_t getParamCount() override { return 1; }
  uint8_t getParamWidth(uint8_t p) override { return 3; }
  void setParam(uint8_t p, uint32_t v) override
  {
    offset = v;
  }

  std::string getDisasm() override
  {
    return "jump " + std::to_string(offset);
  }

  uint16_t getTarget() { return offset; }
  bool isCall() { return doCall; }
};

/*
  JumpF: 0xC8
  CallF: 0xC4
*/
class CmdJumpF : public SeqCommand
{
private:
  uint32_t offset = 0;
  uint8_t cond = 0;
  bool doCall;
public:

  CmdJumpF(bool call) : SeqCommand(5), doCall(call) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override
  {
    cond = data[off+1];
    offset = (data[off+2] << 16) | (data[off+3] << 8) | data[off+4];
    return 0;
  }

  uint8_t getParamCount() override { return 1; }
  uint8_t getParamWidth(uint8_t p) override { return 3; }
  void setParam(uint8_t p, uint32_t v) override
  {
    offset = v;
  }

  std::string getDisasm() override
  {
    return "jump " + std::to_string(offset);
  }

  uint16_t getTarget() { return offset; }
  bool isCall() { return doCall; }
};


/*
  Return: 0xC5
*/
class CmdReturn : public SeqCommand
{
public:
  CmdReturn() : SeqCommand(1) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override
  {
    return 0;
  }

  uint8_t getParamCount() override { return 0; }
  uint8_t getParamWidth(uint8_t p) override { return 0; }
  void setParam(uint8_t p, uint32_t v) override {}

  std::string getDisasm() override
  {
    return "return";
  }

};

class CmdReturnF : public SeqCommand
{
public:
  CmdReturnF() : SeqCommand(2) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override
  {
    return 0;
  }

  uint8_t getParamCount() override { return 0; }
  uint8_t getParamWidth(uint8_t p) override { return 0; }
  void setParam(uint8_t p, uint32_t v) override {}

  std::string getDisasm() override
  {
    return "returnF";
  }

};

/*
 Set param [8bits]:  0xA4 <type:u8> <val:u8>
 Set param [16bits]: 0xAC <type:u8> <val:u16>
*/
class CmdSetParam : public SeqCommand
{
private:
  uint8_t type = 0;
  uint16_t v = 0;
  bool wide = false;

public:
  CmdSetParam(bool wide) : SeqCommand(wide ? 4 : 3), wide(wide) {}

  uint32_t read(std::vector<unsigned char> &data, uint32_t off) override
  {
    type = data[off+1];
    if (wide) v = (data[off+2] << 8) | data[off+3];
    else      v =  data[off+2];
    return 0;
  }

  uint8_t getParamCount() override { return 2; }
  uint8_t getParamWidth(uint8_t p) override
  {
    if (p == 0) return 1;
    else if (p == 1) return wide ? 2 : 1;
    else return 0;
  }
  void setParam(uint8_t p, uint32_t v) override
  {
    if (p == 0) type = v;
    else if (p == 1) this->v = v;
  }

  std::string getDisasm() override
  {
    return "set param " + std::to_string(type) + " -> " + std::to_string(v);
  }

  uint8_t getType() { return type; }
  uint16_t getValue() { return v; }
  bool isWide() { return wide; }
};

#endif // SYNTH_SEQ_PARSER_H
