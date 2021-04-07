#include "aaf.h"

#include "util.h"

#include <stdio.h>

AAFFile::AAFFile(std::string waves_path)
  : waves_path(waves_path)
{

}

bool AAFFile::load(std::string filename)
{
  std::ifstream f(filename, std::ios::binary);
  
  uint32_t chunktype;
  while (true)
  {
    chunktype = readu32(f);

    // known chunk types
    // 0: end
    // 1: unknown; contains <off> <size>
    // 2: IBNK; contains <off> <size> <id>
    // 3: WSYS; contains <off> <size> <id>
    // 4: sequence archive; contains <off> <size> [found in SMS I think]
    // 5: unknown; contains <off> <size>
    // 6: unknown; contains <off> <size>
    // 7: unknown; contains <off> <size>

    if (chunktype == AAFChunk::TYPE_END) break;

    while (true)
    {
      uint32_t off = readu32(f);
      if (off == 0) break;

      AAFChunk chunk;
      chunk.off = off;
      chunk.size = readu32(f);
      chunk.type = chunktype;

      if (chunktype == AAFChunk::TYPE_IBNK || chunktype == AAFChunk::TYPE_WSYS)
      {
        chunk.id = readu32(f);
      }

      this->chunks.push_back(chunk);
    }
  }

  for (uint32_t i = 0; i < this->chunks.size(); i++)
  {
    AAFChunk &chunk = this->chunks[i];
    f.seekg(chunk.off);
    // check WSYS ID
    if (chunk.type == AAFChunk::TYPE_WSYS)
    {
      uint32_t wsysid;
      bool success = Wavesystem::getID(f, &wsysid);
      if (!success) return false; // something bad happened
      this->wsys_chunks[wsysid] = &chunk; // pointer to data in vector

      f.seekg(chunk.off);
    }
    else if (chunk.type == AAFChunk::TYPE_IBNK)
    {
      this->ibnk_chunks[chunk.id] = &chunk;
    }

    chunk.data.resize(chunk.size);
    f.read(chunk.data.data(), chunk.size);
  }
  return true;
}

void AAFFile::writeChunks()
{
  for (AAFChunk &chunk : this->chunks)
  {
    std::string outpath = "../data/chunk.t" + std::to_string(chunk.type);
    if (chunk.id != 0xFFFFFFFF) outpath += "." + std::to_string(chunk.id);

    std::ofstream of(outpath);
    of.write(chunk.data.data(), chunk.data.size());
  }
}

/*
void AAFFile::readChunks(std::string waves_path)
{
  for (AAFChunk &chunk : this->chunks)
  {
    membuf buf = membuf(chunk.data.data(), chunk.data.data() + chunk.size);
    std::istream f(&buf);
    if (chunk.type == AAFChunk::TYPE_WSYS)
    {
      Wavesystem wsys;
      wsys.load(f, waves_path);
      wavesystems.push_back(wsys);
    }
  }
}
*/

Wavesystem AAFFile::loadWavesystem(uint32_t idx)
{
  if (this->wsys_chunks.count(idx) == 0) // is that a valid ID?
  {
    return Wavesystem(); // return an empty (invalid) wavesystem
  }

  AAFChunk *chunk = this->wsys_chunks[idx];
  membuf buf = membuf(chunk->data.data(), chunk->data.data() + chunk->size);
  std::istream f(&buf);

  Wavesystem wsys;
  if (!wsys.load(f, this->waves_path)) return Wavesystem(); // return a dummy one if it didn't load properly

  return wsys;
}

IBNK AAFFile::loadBank(uint32_t idx)
{
  if (this->ibnk_chunks.count(idx) == 0)
  {
    return IBNK();
  }

  AAFChunk *chunk = this->ibnk_chunks[idx];
  membuf buf = membuf(chunk->data.data(), chunk->data.data() + chunk->size);
  std::istream f(&buf);

  IBNK bank;
  if (!bank.load(f)) return IBNK();

  return bank;
}
