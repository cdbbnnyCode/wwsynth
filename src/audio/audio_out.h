#ifndef SYNTH_AUDIO_AUDIO_OUT_H
#define SYNTH_AUDIO_AUDIO_OUT_H

#include <SDL2/SDL_audio.h>
#include <stk/WvOut.h>
#include <stk/Stk.h>
#include <stdint.h>

class SDLAudioOut : public stk::WvOut
{
private:
  SDL_AudioSpec audio_spec;
  bool err = false;
  double volume = 1;
  bool started = false;
  
  int dev_id = -1;
public:
  SDLAudioOut(float samplerate, uint32_t bufsize=1024);
  ~SDLAudioOut();

  void tick(const stk::StkFloat val) override;
  void tick(const stk::StkFrames &data) override;

  bool bad() { return err; }

  void setVolume(double volume) { this->volume = volume; }
  double getVolume() { return volume; }
};

#endif // SYNTH_AUDIO_AUDIO_OUT_H
