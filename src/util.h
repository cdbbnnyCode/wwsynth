#ifndef SYNTH_UTIL_H
#define SYNTH_UTIL_H

#include <iostream>
#include <streambuf>

uint16_t readu16(std::istream &f);
uint32_t readu24(std::istream &f);
uint32_t readu32(std::istream &f);

int16_t reads16(std::istream &f);
int32_t reads32(std::istream &f);

float read_float(std::istream &f);

/*
 membuf -- copied from a github gist and modified so it actually works
 */
struct membuf : std::streambuf
{
  membuf(char *begin, char *end) : begin(begin), end(end)
  {
    this->setg(begin, begin, end);
  }
  
  virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in) override
  {
    if(dir == std::ios_base::cur)
      gbump(off);
    else if(dir == std::ios_base::end)
      setg(begin, end+off, end);
    else if(dir == std::ios_base::beg)
      setg(begin, begin+off, end);
    
    return gptr() - eback();
  }
  
  virtual pos_type seekpos(pos_type pos, std::ios_base::openmode mode) override
  {
    return seekoff(pos - pos_type(off_type(0)), std::ios_base::beg, mode);
  }
  
  char *begin, *end;
};

#endif // SYNTH_UTIL_H