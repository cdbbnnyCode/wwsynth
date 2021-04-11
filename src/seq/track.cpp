#include "track.h"

#include <memory>
#include <cmath>

SeqController::SeqController(AudioSystem &sys, SeqParser &parser, float samplerate)
  : audioSys(sys), parser(parser), samplerate(samplerate)
{
  addTrack(255, 0);
}

void SeqController::addTrack(uint8_t id, uint32_t off)
{
  newTracks.push_back(SeqTrack(this, &parser, off, id, samplerate));
}

void SeqController::removeTrack(SeqTrack *t)
{
  if (t == nullptr) return;
  oldTracks.push_back(t);
}

uint32_t SeqController::getSamplesPerTick()
{
  return (samplerate * 60.0) / ((double)tempo * timebase);
}

bool SeqController::tick(stk::WvOut &out)
{

  for (SeqTrack* &t : oldTracks)
  {
    tracks.remove(*t);
  }
  for (SeqTrack &t : newTracks)
  {
    tracks.push_back(t);
    printf("-> New track %p @ %06x\n", &tracks.back(), tracks.back().getPC());
  }
  newTracks.clear();
  oldTracks.clear();

  if (tracks.size() == 0)
  {
    printf("Track list empty\n");
    return false;
  }

  // clear buffer data
  tickBufL.resize(tickBufL.frames(), 1, 0);
  tickBufR.resize(tickBufL.frames(), 1, 0);
  uint32_t dataSize = 0;

  for (SeqTrack &t : tracks)
  {
    stk::StkFrames trackData;
    if (!t.tick(trackData)) return false;
    
    double panL = std::sqrt(-t.getPan() + 1);
    double panR = std::sqrt( t.getPan());

    if (trackData.frames() != tickBufL.frames())
    {
      tickBufL.resize(trackData.frames(), 1, 0);
      tickBufR.resize(trackData.frames(), 1, 0);
    }
    
    for (uint32_t i = 0; i < trackData.frames(); i++)
    {
      tickBufL[i] += trackData[i] * panL;
      tickBufR[i] += trackData[i] * panR;
    }
  }
  stk::StkFrames outData(tickBufL.frames(), 2);
  outData.setChannel(0, tickBufL, 0);
  outData.setChannel(1, tickBufR, 0);
  out.tick(tickBufL);

  samples_processed += outData.frames();
  tick_count++;
  return true;
}

SeqTrack::SeqTrack(SeqController *controller, SeqParser *parser, uint32_t pc,
                    uint8_t id, float samplerate)
  : controller(controller), parser(parser), pc(pc), trackid(id)
{
  instrument.setSampleRate(samplerate);
  for (int i = 0; i < 7; i++)
  {
    voices[i] = std::vector<Note *>();
  }
}

static float semitones_to_pitch(float pitch)
{
  return std::pow(2.0, pitch / 12);
}

void SeqTrack::setPerf(uint32_t type, float v)
{
  if (type == 0)      volume = v;
  else if (type == 1) pitch = v;
  else if (type == 2) reverb = v;
  else                pan = v;
}

bool SeqTrack::tick(stk::StkFrames &data)
{
  while (delay_timer == 0)
  {
    Step s = step();
    if (s == STEP_FINISHED)
    {
      controller->removeTrack(this);
      return true;
    }
    else if (s == STEP_ERROR)
    {
      return false;
    }
    else if (s == STEP_WAITING)
    {
      break;
    }
  }

  auto it = slides.begin();
  while (it != slides.end())
  {
    Slide &s = *it;
    if (s.t >= s.duration)
    {
      setPerf(s.type, s.end);
      slides.erase(it++);
    }
    else
    {
      setPerf(s.type, (s.end - s.start) * ((float)s.t / s.duration) + s.start);
      s.t++;
      it++;
    }
  }

  uint32_t samples = controller->getSamplesPerTick();
  if (data.size() < samples)
  {
    data.resize(samples);
  }
  for (uint32_t i = 0; i < samples; i++)
  {
    stk::StkFloat total = 0;
    auto iter = notes.begin();
    while (iter != notes.end())
    {
      Note *note = *iter;
      note->pitch_adj = semitones_to_pitch(pitch);
      stk::StkFloat v = note->tick();
      if (note->isFinished())
      {
        notes.erase(iter++);
      }
      else
      {
        total += v;
        iter++;
      }
    }
    data[i] = total * volume;
  }
  delay_timer--;
  return true;
}

SeqTrack::Step SeqTrack::step()
{
  if (delay_timer > 0) return Step::STEP_WAITING;

  std::unique_ptr<SeqCommand> cmd = parser->readCommand(pc);
  // printf("[track %u <%p>] %06x | %s\n", trackid, this, pc, cmd->getDisasm().c_str());
  if (dynamic_cast<BadCmd *>(cmd.get()) != nullptr) return Step::STEP_ERROR;
  pc += cmd->getSize();

  {
    CmdOpenTrack *cmd_ = dynamic_cast<CmdOpenTrack *>(cmd.get());
    if (cmd_ != nullptr)
    {
      controller->addTrack(cmd_->getTrackID(), cmd_->getOffset());
      return Step::STEP_OK;
    }
  }

  {
    CmdTempo *cmd_ = dynamic_cast<CmdTempo *>(cmd.get());
    if (cmd_ != nullptr)
    {
      controller->tempo = cmd_->getTempo();
      return Step::STEP_OK;
    }
  }

  {
    CmdTimebase *cmd_ = dynamic_cast<CmdTimebase *>(cmd.get());
    if (cmd_ != nullptr)
    {
      controller->timebase = cmd_->getTimebase();
      return Step::STEP_OK;
    }
  }

  {
    CmdSetParam *cmd_ = dynamic_cast<CmdSetParam *>(cmd.get());
    if (cmd_ != nullptr)
    {
      if (cmd_->getType() == 0x20) // BANK
      {
        printf("Set bank %d\n", cmd_->getValue());
        IBNK *bank = controller->audioSys.getBank(cmd_->getValue());
        if (bank == nullptr) return Step::STEP_ERROR;

        printf("-> Use wsys %d\n", bank->getWavesystemID());

        Wavesystem *wsys = controller->audioSys.getWsysFor(bank);
        if (wsys == nullptr) return Step::STEP_ERROR;

        instrument.setBank(bank, wsys);
      }
      else if (cmd_->getType() == 0x21) // PROG
      {
        instrument.setInstr(cmd_->getValue());
      }
      return Step::STEP_OK;
    }
  }

  {
    CmdSetPerf *cmd_ = dynamic_cast<CmdSetPerf *>(cmd.get());
    if (cmd_ != nullptr)
    {
      float val;
      if (cmd_->isWide()) val = (int16_t)cmd_->getValue() / 32768.0;
      else                val = (int8_t)cmd_->getValue()  / 128.0;
      
      if (cmd_->getDuration() > 0)
      {
        float start = 0;
        if (cmd_->getType() == 0) start = volume;
        else if (cmd_->getType() == 1) start = pitch;
        else if (cmd_->getType() == 2) start = reverb;
        else if (cmd_->getType() == 3) start = pan;
        slides.push_back(Slide{cmd_->getType(), start, val, cmd_->getDuration(), 0});
      }
      else
      {
        setPerf(cmd_->getType(), val);
      }
      return Step::STEP_OK;
    }
  }

  {
    CmdTrackEnd *cmd_ = dynamic_cast<CmdTrackEnd *>(cmd.get());
    if (cmd_ != nullptr)
    {
      return Step::STEP_FINISHED;
    }
  }

  {
    CmdJump *cmd_ = dynamic_cast<CmdJump *>(cmd.get());
    if (cmd_ != nullptr)
    {
      if (cmd_->isCall())
      {
        callstack.push(pc);
        pc = cmd_->getTarget();
      }
      else
      {
        // TODO loop detection?
        pc = cmd_->getTarget();
        loops++;
        if (loops >= 2) controller->removeTrack(this);
      }
      return Step::STEP_OK;
    }
  }

  {
    CmdReturn *cmd_ = dynamic_cast<CmdReturn *>(cmd.get());
    if (cmd_ != nullptr)
    {
      if (callstack.empty())
      {
        printf("Seq ERROR: Stack underflow\n");
        return Step::STEP_ERROR;
      }
      pc = callstack.top();
      callstack.pop();
      return Step::STEP_OK;
    }
  }

  {
    CmdReturnF *cmd_ = dynamic_cast<CmdReturnF *>(cmd.get());
    if (cmd_ != nullptr)
    {
      if (callstack.empty())
      {
        printf("Seq ERROR: Stack underflow\n");
        return Step::STEP_ERROR;
      }
      pc = callstack.top();
      callstack.pop();
      return Step::STEP_OK;
    }
  }

  {
    CmdJumpF *cmd_ = dynamic_cast<CmdJumpF *>(cmd.get());
    if (cmd_ != nullptr)
    {
      // TODO check condition
      if (cmd_->isCall())
      {
        callstack.push(pc);
        pc = cmd_->getTarget();
      }
      else
      {
        // TODO loop detection?
        pc = cmd_->getTarget();
        loops++;
        if (loops >= 2) controller->removeTrack(this);
      }
      return Step::STEP_OK;
    }
  }

  {
    CmdNoteOn *cmd_ = dynamic_cast<CmdNoteOn *>(cmd.get());
    if (cmd_ != nullptr)
    {
      if (cmd_->getVoice() < 1 || cmd_->getVoice() > 7)
      {
        return Step::STEP_ERROR;
      }

      Note *note = controller->audioSys.getNewNote();
      if (note == nullptr)
      {
        return Step::STEP_ERROR;
      }
      bool success = instrument.createNote(cmd_->getNote(), cmd_->getVelocity(), note);
      if (!success)
      {
        note->stopNow(); // reset state
        return Step::STEP_ERROR;
      }
      note->start();
      
      voices[cmd_->getVoice() - 1].push_back(note);
      notes.push_back(note);
      return Step::STEP_OK;
    }
  }

  {
    CmdVoiceOff *cmd_ = dynamic_cast<CmdVoiceOff *>(cmd.get());
    if (cmd_ != nullptr)
    {
      for (Note* &note : voices[cmd_->getVoice() - 1])
      {
        if (note != nullptr) note->stop();
      }
      voices[cmd_->getVoice() - 1].clear();
      return Step::STEP_OK;
    }
  }

  {
    CmdWait *cmd_ = dynamic_cast<CmdWait *>(cmd.get());
    if (cmd_ != nullptr)
    {
      delay_timer = cmd_->getDelay();
    }
    return Step::STEP_OK;
  }
}