#include "audio_out.h"

#include <SDL2/SDL.h>

/*
 TODO:
    Make a better low-latency queue implementation than is provided by default.
 */

SDLAudioOut::SDLAudioOut(float samplerate, uint32_t bufsize) : stk::WvOut()
{
  SDL_Init(SDL_INIT_AUDIO);

  audio_spec.callback = nullptr;
  audio_spec.channels = 2;
  audio_spec.format = AUDIO_S16;
  audio_spec.samples = 4096;
  audio_spec.freq = (int)samplerate;
  audio_spec.size = bufsize;
  audio_spec.userdata = nullptr;

  dev_id = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);
  SDL_PauseAudioDevice(dev_id, 0);
  if (dev_id <= 0)
  {
    err = true;
    printf("Failed to open audio: %s\n", SDL_GetError());
  }
}

SDLAudioOut::~SDLAudioOut()
{
  SDL_CloseAudioDevice(dev_id);
}

static bool writeData(int dev_id, int16_t *data, uint32_t size)
{
  if (SDL_QueueAudio(dev_id, data, size) < 0)
  {
    printf("Error writing data: %s\n", SDL_GetError());
    return false;
  }
  
  /*
  if (SDL_GetQueuedAudioSize(dev_id) < 44100) SDL_PauseAudioDevice(dev_id, 1);
  else SDL_PauseAudioDevice(dev_id, 0);
  */
  
  SDL_Event ev;

  while (SDL_GetQueuedAudioSize(dev_id) > 44100 * 16)
  {
    SDL_Delay(1);
    while (SDL_PollEvent(&ev))
    {
      if (ev.type == SDL_QUIT)
      {
        printf("Received Ctrl+C event; quitting soon\n");
        return false;
      }
    }
  }
  return true;
}

void SDLAudioOut::tick(stk::StkFloat val)
{
  if (err) return;
  int16_t pt[2];

  val *= volume;

  pt[0] = (int16_t)(clipTest(val) * 32767);
  pt[1] = pt[0];

  if (!writeData(dev_id, pt, 4)) err = true;
}

void SDLAudioOut::tick(const stk::StkFrames &data)
{
  if (err) return;
  if (data.channels() != 2) return;
  int16_t buf[data.size()];
  for (uint32_t i = 0; i < data.frames(); i++)
  {
    stk::StkFloat left  = data(i, 0) * volume;
    stk::StkFloat right = data(i, 1) * volume;
    buf[i*2]     = (int16_t)(clipTest(left)  * 32767);
    buf[i*2 + 1] = (int16_t)(clipTest(right) * 32767);
  }

  if (!writeData(dev_id, buf, data.size() * 2)) err = true;
}