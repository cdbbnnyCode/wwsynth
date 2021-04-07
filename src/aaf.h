#ifndef SYNTH_AAF_H
#define SYNTH_AAF_H

#include <stdint.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "banks.h"

struct AAFChunk
{
  enum
  {
    TYPE_END = 0,
    TYPE_UNKNOWN_1,
    TYPE_IBNK,
    TYPE_WSYS,
    TYPE_SEQ_ARC,
    TYPE_UNKNOWN_5,
    TYPE_UNKNOWN_6,
    TYPE_UNKNOWN_7
  };

  uint32_t type;
  uint32_t off;
  uint32_t size;
  uint32_t id = 0xFFFFFFFF;

  std::vector<char> data;
};

class AAFFile
{
private:
  std::vector<AAFChunk> chunks;
  std::unordered_map<uint32_t, AAFChunk *> wsys_chunks;
  std::unordered_map<uint32_t, AAFChunk *> ibnk_chunks;
  std::string waves_path;
  // TODO banks

public:
  AAFFile(std::string waves_path);

  bool load(std::string filename);
  
  void writeChunks();
  
  Wavesystem loadWavesystem(uint32_t index);
  IBNK loadBank(uint32_t index);
};

#endif // SYNTH_AAF_H