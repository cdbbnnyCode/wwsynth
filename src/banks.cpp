#include "banks.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <fstream>

#include <stk/FileWvOut.h>

#define HIGH_NIBBLE(x) ((x >> 4) & 0xF)
#define LOW_NIBBLE(x)  (x & 0xF)

Wavesystem::Wavesystem(void) { }

static void decode_adpcm4(stk::StkFrames &data, std::ifstream &f, uint32_t size);
static void decode_pcm8(stk::StkFrames &data, std::ifstream &f, uint32_t size);
static void decode_pcm16(stk::StkFrames &data, std::ifstream &f, uint32_t size);

bool Wavesystem::getID(std::istream &f, uint32_t *id)
{
  uint32_t base_off = f.tellg();
  char magic[4];
  f.read(magic, 4);
  if (memcmp(magic, "WSYS", 4) != 0)
  {
    printf("WSYS peek error: bad magic\n");
    return false;
  }

  f.read(magic, 4); // file size
  *id = readu32(f);

  return true;
}

bool Wavesystem::isLoaded()
{
  return this->loaded;
}

Wave *Wavesystem::getWave(uint16_t aw_id, uint16_t wave_id)
{
  WaveEntry entry{aw_id, wave_id};
  
  if (this->waves.count(entry) == 0) return nullptr;
  else return this->waves[entry].get();
}

bool Wavesystem::load(std::istream &f, std::string waves_path)
{
  if (isLoaded()) return false;
  this->loaded = true; // might be only partially initialized but shouldn't overwrite

  uint32_t base_off = f.tellg();
  char magic[4];
  f.read(magic, 4);
  if (memcmp(magic, "WSYS", 4) != 0)
  {
    printf("WSYS load error: bad magic\n");
    return false;
  }

  uint32_t file_size = readu32(f);
  this->wsys_id = readu32(f);

  readu32(f); // unknown

  uint32_t winf_offset = readu32(f);
  uint32_t wbct_offset = readu32(f);


  printf("Header:\n");
  printf("-> File size: %u\n", file_size);
  printf("-> Wavesystem ID: %u\n", wsys_id);
  printf("-> Wave info @ %08x\n", winf_offset);
  printf("-> WBCT @ %08x\n", wbct_offset);

  f.seekg(base_off + winf_offset);
  f.read(magic, 4);
  if (memcmp(magic, "WINF", 4) != 0)
  {
    printf("WSYS error: bad WINF magic\n");
    return false;
  }
  uint32_t num_groups = readu32(f);
  uint32_t winf_group_offs[num_groups];

  for (uint32_t i = 0; i < num_groups; i++)
  {
    winf_group_offs[i] = readu32(f);
  }

  f.seekg(base_off + wbct_offset);
  f.read(magic, 4);
  if (memcmp(magic, "WBCT", 4) != 0)
  {
    printf("WSYS error: bad WBCT magic\n");
    return false;
  }
  f.read(magic, 4); // read 4 more bytes that we don't care about

  uint32_t num_wbct_groups = readu32(f);
  if (num_wbct_groups != num_groups)
  {
    printf("WSYS error: WBCT groups != WINF groups\n");
    return false;
  }
  uint32_t wbct_group_offs[num_groups];

  for (uint32_t i = 0; i < num_groups; i++)
  {
    wbct_group_offs[i] = readu32(f);
  }

  for (uint32_t i = 0; i < num_groups; i++)
  {
    // WINF group info
    f.seekg(base_off + winf_group_offs[i]);
    char aw_filename[0x70];
    f.read(aw_filename, 0x70);
    uint32_t wave_count = readu32(f);
    uint32_t winf_wave_offs[wave_count];

    for (uint32_t j = 0; j < wave_count; j++)
    {
      winf_wave_offs[j] = readu32(f);
    }

    // WBCT scene info
    f.seekg(base_off + wbct_group_offs[i]);
    f.read(magic, 4);
    if (memcmp(magic, "SCNE", 4) != 0)
    {
      printf("WSYS error: bad SCNE magic\n");
      return false;
    }
    f.read(magic, 4);
    f.read(magic, 4);

    uint32_t cdf_off = readu32(f);
    f.seekg(base_off + cdf_off);
    f.read(magic, 4);
    if (memcmp(magic, "C-DF", 4) != 0)
    {
      printf("WSYS error: bad C-DF magic\n");
      return false;
    }

    uint32_t cdf_entry_count = readu32(f);
    if (cdf_entry_count != wave_count)
    {
      printf("WSYS error: C-DF entries != WINF entries\n");
      return false;
    }
    uint32_t cdf_entry_offs[cdf_entry_count];

    for (uint32_t j = 0; j < wave_count; j++)
    {
      cdf_entry_offs[j] = readu32(f);
    }

    // read data
    for (uint32_t j = 0; j < wave_count; j++)
    {
      Wave wave;
      WaveEntry entry;

      memcpy(wave.aw_filename, aw_filename, 0x70);
      
      f.seekg(base_off + winf_wave_offs[j] + 1);
      wave.format = f.get();
      wave.base_key = f.get();
      f.get();
      wave.sample_rate = read_float(f);
      wave.wavedata_offset = readu32(f);
      wave.wavedata_size = readu32(f);
      wave.loop = (bool)readu32(f);
      wave.loop_start = readu32(f);
      wave.loop_end = readu32(f);
      wave.sample_count = readu32(f);

      f.seekg(base_off + cdf_entry_offs[j]);
      entry.aw_id = readu16(f);
      entry.wave_id = readu16(f);
      wave.aw_id = entry.aw_id;
      wave.wave_id = entry.wave_id;

      waves[entry] = std::move(std::make_unique<Wave>(wave));

      printf("Wave: AW %6u, wave %6u -> fmt %u key %3u, %5d bytes @ %08x, %6u samples @%5.0fHz %s %6u-%6u in %s\n",
        entry.aw_id, entry.wave_id, wave.format, wave.base_key, wave.wavedata_size, wave.wavedata_offset,
        wave.sample_count, wave.sample_rate, wave.loop ? "looped    " : "not looped", wave.loop_start,
        wave.loop_end, wave.aw_filename);
    }
  }

  printf("\n");
  for (std::pair<const WaveEntry, std::unique_ptr<Wave>> &entry : waves)
  {
    const WaveEntry &waveid = entry.first;
    std::unique_ptr<Wave> &wave = waves[waveid];

    std::string outfilename = "../data/waves/wsys" + std::to_string(wsys_id) + "_aw" + std::to_string(waveid.aw_id) + "_wv" \
                              + std::to_string(waveid.wave_id) + ".wav";
    std::string infile = waves_path + "/" + std::string(wave->aw_filename);

    printf("Decoding %s:%08x-%08x\n", wave->aw_filename, wave->wavedata_offset, wave->wavedata_offset + wave->wavedata_size);
    wave->data.resize(wave->sample_count, 1, 0);

    std::ifstream in_data(infile, std::ios::binary);
    in_data.seekg(wave->wavedata_offset);
    if (wave->format == 0) decode_adpcm4(wave->data, in_data, wave->wavedata_size);
    else if (wave->format == 2) decode_pcm8(wave->data, in_data, wave->wavedata_size);
    else if (wave->format == 3) decode_pcm16(wave->data, in_data, wave->wavedata_size);
    else
    {
      printf("Unknown wave format %d\n", wave->format);
    }
    

  /*
    printf("Writing %s\n", outfilename.c_str());
    stk::Stk::setSampleRate(wave.sample_rate);
    stk::FileWvOut writer(outfilename, 1);
    writer.tick(wave.data); 
  */
  }

  return true;
}

static const int16_t adpcm_coeffs[32] =
{
      0,     0,
   2048,     0,
      0,  2048,
   1024,  1024,
   4096, -2048,
   3584, -1536,
   3072, -1024,
   4608, -2560,
   4200, -2248,
   4800, -2300,
   5120, -3072,
   2048, -2048,
   1024, -1024,
  -1024,  1024,
  -1024,     0,
  -2048,     0
};

static const int8_t signed_4bit[16] = {
  0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1
};

static void decode_adpcm4(stk::StkFrames &data, std::ifstream &f, uint32_t size)
{
  // 4 bit ADPCM (format 0)
  // How it works:
  // * Start with 2 history samples as 0 and 0. These are the last 2
  //   computed samples to calculate off of.
  // * Read 2 nibbles: these are, respectively:
  //   -> The base-2 log of the factor to multiply each delta value by
  //   -> Which coefficient pair to use
  // * Read the next 16 nibbles. These are the delta values, which are
  //   scaled by the factor in the header and adjusted by the history
  //   samples to create a sample.
  // * Next, iterate over the 16 delta values and apply the following:
  //   int32_t rawsample = (factor * delta) + ((hist * coeff1) + (hist2 * coeff2)) / 2048
  //   if (rawsample > 32767) rawsample = 32767;
  //   if (rawsample < -32768) rawsample = -32768;
  //   int16_t sample = (int16_t)rawsample;
  //   hist2 = hist;
  //   hist = sample;

  int16_t hist1 = 0;
  int16_t hist2 = 0;

  int16_t coeff1, coeff2;

  if (size % 9 != 0)
  {
    printf("Data is not a whole number of ADPCM frames\n");
    return;
  }
  /*
  if ((size / 9) * 16 > data.size())
  {
    printf("Data buffer not large enough to fit sample data (%d > %d)\n", (size / 9 * 16), data.size());
    return;
  }
  */

  uint32_t frames = size / 9;
  char framedata[9];
  int32_t deltas[16];
  for (uint32_t frame = 0; frame < frames; frame++)
  {
    f.read(framedata, 9);

    int32_t factor = (1 << HIGH_NIBBLE(framedata[0]));
    uint16_t coeff_index = LOW_NIBBLE(framedata[0]);

    coeff1 = adpcm_coeffs[coeff_index * 2];
    coeff2 = adpcm_coeffs[coeff_index * 2 + 1];

    for (uint8_t b = 0; b < 8; b++)
    {
      deltas[2*b] = signed_4bit[HIGH_NIBBLE(framedata[1 + b])];
      deltas[2*b + 1] = signed_4bit[LOW_NIBBLE(framedata[1 + b])];
    }

    for (uint8_t d = 0; d < 16; d++)
    {
      int32_t raw_sample = (factor * deltas[d]) + ((((int32_t)hist1 * coeff1) + ((int32_t)hist2 * coeff2)) >> 11);
      int16_t sample;

      // printf("%d  ",hist1);

      if (raw_sample > 32767) sample = 32767;
      else if (raw_sample < -32768) sample = -32768;
      else sample = (int16_t)raw_sample;


      if (frame * 16 + d < data.size())
        data[frame * 16 + d] = (stk::StkFloat)sample / 32768.0;

      hist2 = hist1;
      hist1 = sample;
    }
  }
}

static void decode_pcm8(stk::StkFrames &data, std::ifstream &f, uint32_t size)
{
  for (uint32_t i = 0; i < size; i++)
  {
    int8_t sample = (int8_t)f.get();
    data[i] = (stk::StkFloat)sample / 128.0;
  }
}

static void decode_pcm16(stk::StkFrames &data, std::ifstream &f, uint32_t size)
{
  if (size % 2 != 0)
  {
    printf("Need an even number of bytes\n");
    return;
  }


  for (uint32_t i = 0; i < size/2; i++)
  {
    int16_t sample = reads16(f);
    data[i] = (stk::StkFloat)sample / 32768.0;
  }
}

///////////////////////////////////////////////////////////////////////////
// IBNK decoding

KeyRgn *KeyMap::addRgn(uint8_t maxKey)
{
  this->regions.push_back(std::move(std::make_unique<KeyRgn>(KeyRgn{maxKey})));
  return this->regions.back().get();
}

KeyInfo *KeyMap::getKeyInfo(uint8_t key, uint8_t vel)
{
  if (key > 127 || vel > 127) return nullptr;

  for (std::unique_ptr<KeyRgn> &rgn : regions)
  {
    if ((isPercussion && key == rgn->maxKey) || (!isPercussion && key <= rgn->maxKey))
    {
      for (std::unique_ptr<KeyInfo> &inf : rgn->keys)
      {
        if (vel < inf->maxVel)
        {
          return inf.get();
        }
      }
      break;
    }
  }
  return nullptr;
}

IBNK::IBNK(void) {}

bool IBNK::isLoaded()
{
  return this->loaded;
}

bool IBNK::load(std::istream &f)
{
  if (this->loaded) return false;
  this->loaded = true;
  uint32_t base_off = f.tellg();

  char magic[4];
  f.read(magic, 4);

  if (memcmp(magic, "IBNK", 4) != 0)
  {
    printf("IBNK load error: bad magic\n");
    return false;
  }

  uint32_t filesize = readu32(f);
  uint32_t wsys_id  = readu32(f);
  this->wsysid = wsys_id;

  f.seekg(base_off + 0x20); // skip the rest of the header
  f.read(magic, 4);
  if (memcmp(magic, "BANK", 4) != 0)
  {
    printf("IBNK load error: bad BANK magic\n");
    return false;
  }

  const uint32_t num_instruments = 245;

  uint32_t inst_offs[num_instruments];
  for (int i = 0; i < num_instruments; i++)
  {
    inst_offs[i] = readu32(f);
  }

  for (int i = 0; i < num_instruments; i++)
  {
    uint32_t off = inst_offs[i];
    if (off != 0)
    {
      f.seekg(base_off + off);

      f.read(magic, 4);
      if (memcmp(magic, "INST", 4) == 0)
      {
        instruments.push_back(std::make_unique<BankInstrument>());
        std::unique_ptr<BankInstrument> &instrument = instruments.back();

        instrument->isPercussion = false;
        instrument->keys.isPercussion = false;
        f.read(magic, 4); // skip 4 bytes of padding(?)
        float volume = read_float(f);
        float pitch  = read_float(f);

        instrument->volume = volume;
        instrument->pitch = pitch;
        uint32_t osci_off = readu32(f); // TODO find ADSR data

        f.seekg((uint32_t)f.tellg() + 0x14);
        uint32_t key_rgn_count = readu32(f);
        if (key_rgn_count > 128)
        {
          printf("Too many key regions: %u\n", key_rgn_count);
          return false;
        }

        uint32_t rgn_offs[key_rgn_count];
        for (uint32_t j = 0; j < key_rgn_count; j++)
        {
          rgn_offs[j] = readu32(f);
        }

        for (uint32_t j = 0; j < key_rgn_count; j++)
        {
          f.seekg(base_off + rgn_offs[j]);
          f.read(magic, 4); // use magic as a general-purpose 4-byte buffer

          uint8_t maxKey = magic[0];
          KeyRgn *rgn = instrument->keys.addRgn(maxKey);
          
          uint32_t vel_rgn_count = readu32(f);
          if (vel_rgn_count > 128)
          {
            printf("Too many velocity regions: %u\n", vel_rgn_count);
            return false;
          }

          uint32_t vel_rgn_offs[vel_rgn_count];
          for (uint32_t k = 0; k < vel_rgn_count; k++)
          {
            vel_rgn_offs[k] = readu32(f);
          }

          for (uint32_t k = 0; k < vel_rgn_count; k++)
          {
            rgn->keys.push_back(std::make_unique<KeyInfo>());
            std::unique_ptr<KeyInfo> &info = rgn->keys.back();
            info->rgn = rgn;
            f.seekg(base_off + vel_rgn_offs[k]);
            f.read(magic, 4);

            info->maxVel = magic[0];
            info->awid   = readu16(f);
            info->waveid = readu16(f);
            info->volume = read_float(f);
            info->pitch  = read_float(f);
          }
        }
      }
      else if (memcmp(magic, "PER2", 4) == 0)
      {
        instruments.push_back(std::make_unique<BankInstrument>());
        std::unique_ptr<BankInstrument> &instrument = instruments.back();
        instrument->isPercussion = true;
        instrument->keys.isPercussion = true;
        printf("Percussion instrument @ %08x\n", (uint32_t)f.tellg());

        f.seekg((uint32_t)f.tellg() + 0x84);
        printf("%08x\n", (uint32_t)f.tellg());
        uint32_t key_offs[128];

        for (uint32_t j = 0; j < 128; j++)
        {
          key_offs[j] = readu32(f);
          printf("%08x ", key_offs[j]);
          if (j % 8 == 7) printf("\n");
        }

        for (uint32_t j = 0; j < 128; j++)
        {
          if (key_offs[j] == 0)
          {
            continue;
          }

          KeyRgn *rgn = instrument->keys.addRgn(j);
          f.seekg(base_off + key_offs[j]);

          rgn->volume = readu32(f);
          rgn->pitch = readu32(f);

          f.seekg((uint32_t)f.tellg() + 8);
          uint32_t vel_rgn_count = readu32(f);

          if (vel_rgn_count > 128)
          {
            printf("Too many velocity regions: %u\n", vel_rgn_count);
            return false;
          }
          
          uint32_t vel_rgn_offs[vel_rgn_count];

          for (uint32_t k = 0; k < vel_rgn_count; k++)
          {
            vel_rgn_offs[k] = readu32(f);
          }

          for (uint32_t k = 0; k < vel_rgn_count; k++)
          {
            f.seekg(base_off + vel_rgn_offs[k]);

            rgn->keys.push_back(std::make_unique<KeyInfo>());
            std::unique_ptr<KeyInfo> &info = rgn->keys.back();
            info->rgn = rgn;
            f.seekg(base_off + vel_rgn_offs[k]);
            f.read(magic, 4);

            info->maxVel = magic[0];
            info->awid   = readu16(f);
            info->waveid = readu16(f);
            info->volume = read_float(f);
            info->pitch  = read_float(f);
          }
        }
      }
      else
      {
        printf("IBNK load error: bad INST/PER2 magic\n");
        return false;
      }
    }
  }

  return true;
}
