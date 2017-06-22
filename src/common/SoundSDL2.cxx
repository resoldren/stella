//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifdef SOUND_SUPPORT

#include <sstream>
#include <cassert>
#include <cmath>

#include "SDL_lib.hxx"
#include "TIASnd.hxx"
#include "TIATypes.hxx"
#include "FrameBuffer.hxx"
#include "Settings.hxx"
#include "System.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "SoundSDL2.hxx"

#include <chrono>

static std::chrono::high_resolution_clock::time_point begin;
double myFrameDuration;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::SoundSDL2(OSystem& osystem)
  : Sound(osystem),
    myIsEnabled(false),
    myIsInitializedFlag(false),
    myLastRegisterSetCycle(0),
    myNumChannels(0),
    myFragmentSizeLogBase2(0),
    myFragmentSizeLogDiv1(0),
    myFragmentSizeLogDiv2(0),
    myIsMuted(true),
    myVolume(100)
{
  myOSystem.logMessage("SoundSDL2::SoundSDL2 started ...", 2);
  begin = std::chrono::high_resolution_clock::now();

  // The sound system is opened only once per program run, to eliminate
  // issues with opening and closing it multiple times
  // This fixes a bug most prevalent with ATI video cards in Windows,
  // whereby sound stopped working after the first video change
  SDL_AudioSpec desired;
  desired.freq   = myOSystem.settings().getInt("freq");
  desired.format = AUDIO_S16SYS;
  desired.channels = 2;
  desired.samples  = myOSystem.settings().getInt("fragsize");
  desired.callback = callback;
  desired.userdata = static_cast<void*>(this);

  ostringstream buf;
  if(SDL_OpenAudio(&desired, &myHardwareSpec) < 0)
  {
    buf << "WARNING: Couldn't open SDL audio system! " << endl
        << "         " << SDL_GetError() << endl;
    myOSystem.logMessage(buf.str(), 0);
    return;
  }

  // Make sure the sample buffer isn't to big (if it is the sound code
  // will not work so we'll need to disable the audio support)
  if((float(myHardwareSpec.samples) / float(myHardwareSpec.freq)) >= 0.25)
  {
    buf << "WARNING: Sound device doesn't support realtime audio! Make "
        << "sure a sound" << endl
        << "         server isn't running.  Audio is disabled." << endl;
    myOSystem.logMessage(buf.str(), 0);

    SDL_CloseAudio();
    return;
  }

  myFrameDuration = 1.0 / myHardwareSpec.freq;
  printf("frame duration = %g\n", myFrameDuration);

  // Pre-compute fragment-related variables as much as possible
  myFragmentSizeLogBase2 = log(myHardwareSpec.samples) / log(2.0);
  myFragmentSizeLogDiv1 = myFragmentSizeLogBase2 / 60.0;
  myFragmentSizeLogDiv2 = (myFragmentSizeLogBase2 - 1) / 60.0;

  myIsInitializedFlag = true;
  SDL_PauseAudio(1);

  myOSystem.logMessage("SoundSDL2::SoundSDL2 initialized", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::~SoundSDL2()
{
  // Close the SDL audio system if it's initialized
  if(myIsInitializedFlag)
  {
    SDL_CloseAudio();
    myIsEnabled = myIsInitializedFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setEnabled(bool state)
{
  myOSystem.settings().setValue("sound", state);

  myOSystem.logMessage(state ? "SoundSDL2::setEnabled(true)" :
                                "SoundSDL2::setEnabled(false)", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::open()
{
  myOSystem.logMessage("SoundSDL2::open started ...", 2);
  myIsEnabled = false;
  mute(true);
  if(!myIsInitializedFlag || !myOSystem.settings().getBool("sound"))
  {
    myOSystem.logMessage("Sound disabled\n", 1);
    return;
  }

  // Now initialize the TIASound object which will actually generate sound
  myTIASound.outputFrequency(myHardwareSpec.freq);
  const string& chanResult =
      myTIASound.channels(myHardwareSpec.channels, myNumChannels == 2);

  // Adjust volume to that defined in settings
  myVolume = myOSystem.settings().getInt("volume");
  setVolume(myVolume);

  // Show some info
  ostringstream buf;
  buf << "Sound enabled:"  << endl
      << "  Volume:      " << myVolume << endl
      << "  Frag size:   " << uInt32(myHardwareSpec.samples) << endl
      << "  Frequency:   " << uInt32(myHardwareSpec.freq) << endl
      << "  Channels:    " << uInt32(myHardwareSpec.channels)
                           << " (" << chanResult << ")" << endl
      << endl;
  myOSystem.logMessage(buf.str(), 1);

  // And start the SDL sound subsystem ...
  myIsEnabled = true;
  mute(false);

  myOSystem.logMessage("SoundSDL2::open finished", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::close()
{
  if(myIsInitializedFlag)
  {
    myIsEnabled = false;
    SDL_PauseAudio(1);
    myLastRegisterSetCycle = 0;
    myTIASound.reset();
    myRegWriteQueue.clear();
    myOSystem.logMessage("SoundSDL2::close", 2);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::mute(bool state)
{
  if(myIsInitializedFlag)
  {
    myIsMuted = state;
    SDL_PauseAudio(myIsMuted ? 1 : 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::reset()
{
  if(myIsInitializedFlag)
  {
    SDL_PauseAudio(1);
    myLastRegisterSetCycle = 0;
    myTIASound.reset();
    myRegWriteQueue.clear();
    mute(myIsMuted);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setVolume(Int32 percent)
{
  if(myIsInitializedFlag && (percent >= 0) && (percent <= 100))
  {
    myOSystem.settings().setValue("volume", percent);
    SDL_LockAudio();
    myVolume = percent;
    myTIASound.volume(percent);
    SDL_UnlockAudio();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::adjustVolume(Int8 direction)
{
  ostringstream strval;
  string message;

  Int32 percent = myVolume;

  if(direction == -1)
    percent -= 2;
  else if(direction == 1)
    percent += 2;

  if((percent < 0) || (percent > 100))
    return;

  setVolume(percent);

  // Now show an onscreen message
  strval << percent;
  message = "Volume set to ";
  message += strval.str();

  myOSystem.frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::adjustCycleCounter(Int32 amount)
{
  myLastRegisterSetCycle += amount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setChannels(uInt32 channels)
{
  if(channels == 1 || channels == 2)
    myNumChannels = channels;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setFrameRate(float framerate)
{
  // Recalculate since frame rate has changed
  // FIXME - should we clear out the queue or adjust the values in it?
  myFragmentSizeLogDiv1 = myFragmentSizeLogBase2 / framerate;
  myFragmentSizeLogDiv2 = (myFragmentSizeLogBase2 - 1) / framerate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::set(uInt16 addr, uInt8 value, Int32 cycle)
{
    static auto setLast = std::chrono::high_resolution_clock::now();
    auto setNow = std::chrono::high_resolution_clock::now();
    auto setDistance = setNow - setLast;
    static Int32 setCycles = 0;
    static double setAmount = 0.0;
  SDL_LockAudio();

  // First, calculate how many seconds would have past since the last
  // register write on a real 2600
  setCycles += cycle - myLastRegisterSetCycle;
  double delta = double(cycle - myLastRegisterSetCycle) / 1193191.66666667;
  setAmount += delta;
  if (setDistance >= std::chrono::milliseconds(1000)) {
    printf("%07ld: %gms(%d cycles) in %ldms\n", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()-begin).count(), setAmount,
           setCycles, std::chrono::duration_cast<std::chrono::milliseconds>(setDistance).count());
    setLast = setNow;
    setAmount = 0.0;
    setCycles = 0;
  }

  // Now, adjust the time based on the frame rate the user has selected. For
  // the sound to "scale" correctly, we have to know the games real frame
  // rate (e.g., 50 or 60) and the currently emulated frame rate. We use these
  // values to "scale" the time before the register change occurs.
  RegWrite info;
  info.addr = addr;
  info.value = value;
  info.delta = delta;
  myRegWriteQueue.enqueue(info);

  // Update last cycle counter to the current cycle
  myLastRegisterSetCycle = cycle;

  SDL_UnlockAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::processFragment(Int16* stream, uInt32 length)
{
  uInt32 channels = myHardwareSpec.channels;
  length = length / channels;

  // If there are excessive items on the queue then we'll remove some
  if(myRegWriteQueue.duration() > myFragmentSizeLogDiv1)
  {
    double removed = 0.0;
    while(removed < myFragmentSizeLogDiv2)
    {
      RegWrite& info = myRegWriteQueue.front();
      removed += info.delta;
      myTIASound.set(info.addr, info.value);
      myRegWriteQueue.dequeue();
    }
  }

  double position = 0.0;
  double remaining = length;

  while(remaining > 0.0)
  {
    if(myRegWriteQueue.size() == 0)
    {
      // There are no more pending TIA sound register updates so we'll
      // use the current settings to finish filling the sound fragment
      myTIASound.process(stream + (uInt32(position) * channels),
          length - uInt32(position));

      // Since we had to fill the fragment we'll reset the cycle counter
      // to zero.  NOTE: This isn't 100% correct, however, it'll do for
      // now.  We should really remember the overrun and remove it from
      // the delta of the next write.
      myLastRegisterSetCycle = 0;
      break;
    }
    else
    {
      // There are pending TIA sound register updates so we need to
      // update the sound buffer to the point of the next register update
      RegWrite& info = myRegWriteQueue.front();

      // How long will the remaining samples in the fragment take to play
      double duration = remaining / myHardwareSpec.freq;

      // Does the register update occur before the end of the fragment?
      if(info.delta <= duration)
      {
        // If the register update time hasn't already passed then
        // process samples upto the point where it should occur
        if(info.delta > 0.0)
        {
          // Process the fragment upto the next TIA register write.  We
          // round the count passed to process up if needed.
          double samples = (myHardwareSpec.freq * info.delta);
          myTIASound.process(stream + (uInt32(position) * channels),
              uInt32(samples) + uInt32(position + samples) -
              (uInt32(position) + uInt32(samples)));

          position += samples;
          remaining -= samples;
        }
        myTIASound.set(info.addr, info.value);
        myRegWriteQueue.dequeue();
      }
      else
      {
        // The next register update occurs in the next fragment so finish
        // this fragment with the current TIA settings and reduce the register
        // update delay by the corresponding amount of time
        myTIASound.process(stream + (uInt32(position) * channels),
            length - uInt32(position));
        info.delta -= duration;
        break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::callback(void* udata, uInt8* stream, int len)
{
  SoundSDL2* sound = static_cast<SoundSDL2*>(udata);
  if(sound->myIsEnabled)
  {
    // The callback is requesting 8-bit (unsigned) data, but the TIA sound
    // emulator deals in 16-bit (signed) data
    // So, we need to convert the pointer and half the length
    double soundAvailable = sound->myRegWriteQueue.duration();
    int framesAvailable = soundAvailable / myFrameDuration - 0;
    sound->processFragment(reinterpret_cast<Int16*>(stream), uInt32(len) >> 1);
    double soundAvailable2 = sound->myRegWriteQueue.duration();
    auto now = std::chrono::high_resolution_clock::now();
//    printf("%07ld: a: len=%d sAvail=%g, sAvail2=%g fAvail=%d\n",
//           std::chrono::duration_cast<std::chrono::milliseconds>(now-begin).count(),
//           len, soundAvailable, soundAvailable2, framesAvailable);
  }
  else
    SDL_memset(stream, 0, len);  // Write 'silence'
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL2::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    // Only get the TIA sound registers if sound is enabled
    if(myIsInitializedFlag)
    {
      out.putByte(myTIASound.get(TIARegister::AUDC0));
      out.putByte(myTIASound.get(TIARegister::AUDC1));
      out.putByte(myTIASound.get(TIARegister::AUDF0));
      out.putByte(myTIASound.get(TIARegister::AUDF1));
      out.putByte(myTIASound.get(TIARegister::AUDV0));
      out.putByte(myTIASound.get(TIARegister::AUDV1));
    }
    else
      for(int i = 0; i < 6; ++i)
        out.putByte(0);

    out.putInt(myLastRegisterSetCycle);
  }
  catch(...)
  {
    myOSystem.logMessage("ERROR: SoundSDL2::save", 0);
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL2::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    // Only update the TIA sound registers if sound is enabled
    // Make sure to empty the queue of previous sound fragments
    if(myIsInitializedFlag)
    {
      SDL_PauseAudio(1);
      myRegWriteQueue.clear();
      myTIASound.set(TIARegister::AUDC0, in.getByte());
      myTIASound.set(TIARegister::AUDC1, in.getByte());
      myTIASound.set(TIARegister::AUDF0, in.getByte());
      myTIASound.set(TIARegister::AUDF1, in.getByte());
      myTIASound.set(TIARegister::AUDV0, in.getByte());
      myTIASound.set(TIARegister::AUDV1, in.getByte());
      if(!myIsMuted) SDL_PauseAudio(0);
    }
    else
      for(int i = 0; i < 6; ++i)
        in.getByte();

    myLastRegisterSetCycle = in.getInt();
  }
  catch(...)
  {
    myOSystem.logMessage("ERROR: SoundSDL2::load", 0);
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::RegWriteQueue::RegWriteQueue(uInt32 capacity)
  : myBuffer(make_ptr<RegWrite[]>(capacity)),
    myCapacity(capacity),
    mySize(0),
    myHead(0),
    myTail(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::RegWriteQueue::clear()
{
  myHead = myTail = mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::RegWriteQueue::dequeue()
{
  if(mySize > 0)
  {
    myHead = (myHead + 1) % myCapacity;
    --mySize;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SoundSDL2::RegWriteQueue::duration() const
{
  double duration = 0.0;
  for(uInt32 i = 0; i < mySize; ++i)
  {
    duration += myBuffer[(myHead + i) % myCapacity].delta;
  }
  return duration;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::RegWriteQueue::enqueue(const RegWrite& info)
{
//  printf("%07ld: enqueue %g\n", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()-begin).count(), info.delta);
  // If an attempt is made to enqueue more than the queue can hold then
  // we'll enlarge the queue's capacity.
  if(mySize == myCapacity)
    grow();

  myBuffer[myTail] = info;
  myTail = (myTail + 1) % myCapacity;
  ++mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::RegWrite& SoundSDL2::RegWriteQueue::front() const
{
  assert(mySize != 0);
  return myBuffer[myHead];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundSDL2::RegWriteQueue::size() const
{
  return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::RegWriteQueue::grow()
{
  unique_ptr<RegWrite[]> buffer = make_ptr<RegWrite[]>(myCapacity*2);
  for(uInt32 i = 0; i < mySize; ++i)
    buffer[i] = myBuffer[(myHead + i) % myCapacity];

  myHead = 0;
  myTail = mySize;
  myCapacity *= 2;

  myBuffer = std::move(buffer);
}

#endif  // SOUND_SUPPORT
