#include "util.h"

uint16_t readu16(std::istream &f)
{
    return (f.get() << 8) | f.get();
}

uint32_t readu24(std::istream &f)
{
    return (f.get() << 16) | (f.get() << 8) | f.get();
}

uint32_t readu32(std::istream &f)
{
    return (f.get() << 24) | (f.get() << 16) | (f.get() << 8) | f.get();
}

int16_t reads16(std::istream &f)
{
    return (int16_t)readu16(f);
}

int32_t reads32(std::istream &f)
{
    return (int32_t)readu32(f);
}

float read_float(std::istream &f)
{
    uint32_t val = readu32(f);
    return *((float *)&val);
}
