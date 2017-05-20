#ifdef SOUND_SUPPORT

#include <sstream>
#include <cassert>
#include <cmath>

#include <roku/RokuSndFxPcm.h>

#include "TIASnd.hxx"
#include "TIATables.hxx"
#include "FrameBuffer.hxx"
#include "Settings.hxx"
#include "System.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "SoundRoku.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundRoku::SoundRoku(OSystem& osystem)
  : Sound(osystem),
    myIsEnabled(false),
    myIsInitializedFlag(false),
    myLastRegisterSetCycle(0),
    myNumChannels(0),
    myFragmentSizeLogBase2(0),
    myFragmentSizeLogDiv1(0),
    myFragmentSizeLogDiv2(0),
    myIsMuted(true),
    myVolume(100),
	myPlayer(nullptr),
	myCallbackSamples(0)
{
  myOSystem.logMessage("SoundRoku::SoundRoku started ...", 2);
  printf("SoundRoku::SoundRoku started ...\n");

  RokuSndFx_Err sfx_err = RokuSndFx_PcmCreate(&myPlayer);
  if (sfx_err != ROKU_SFX_ERR_OK) {
    myOSystem.logMessage("SoundRoku::SoundRoku: RokuSndFx_PcmCreate failed %d\n", sfx_err);
    myPlayer = nullptr;
    return;
  }
  printf("RokuSndFx_PcmCreate returned %p\n", myPlayer);

  RokuSndFx_PcmFormat stream_prop;
  stream_prop.endian = ROKU_SFX_ENDIAN_LIT;//BIG;
  // desired.format = AUDIO_S16SYS;
  stream_prop.sign   = 1;
  stream_prop.width  = 16;
  // desired.channels = 2;
  stream_prop.num_ch = 2;
  // desired.freq   = myOSystem.settings().getInt("freq");
  int rate = myOSystem.settings().getInt("freq");
  stream_prop.rate   = rate; // 44100;

  // desired.samples  = samples
  // myCallbackSamples = myOSystem.settings().getInt("fragsize") * 2; // 2048
  myCallbackSamples = rate / 50;

  sfx_err = RokuSndFx_PcmSetFormat(myPlayer, &stream_prop);
  if(sfx_err != ROKU_SFX_ERR_OK) {
    ostringstream buf;
    buf << "SoundRoku::SoundRoku: RokuSndFx_PcmSetFormat failed " << sfx_err << "\n";
    myOSystem.logMessage(buf.str(), 0);
    return;
  }
  printf("RokuSndFx_PcmSetFormat(%d, %d, %d, %d)\n",
		 stream_prop.sign,
		 stream_prop.width,
		 stream_prop.num_ch,
		 stream_prop.rate);
		 
  sfx_err = RokuSndFx_PcmPrepare(myPlayer, 0);
  if(sfx_err != ROKU_SFX_ERR_OK) {
    ostringstream buf;
    buf << "SoundRoku::SoundRoku: RokuSndFx_PcmPrepare failed " << sfx_err << "\n";
    myOSystem.logMessage(buf.str(), 0);
    return;
  }
  printf("RokuSndFx_PcmPrepare()\n");
#if 1
  int maxDelay = 1200;
  sfx_err = RokuSndFx_PcmSetMaxDelay(myPlayer, maxDelay);
  if(sfx_err != ROKU_SFX_ERR_OK) {
    ostringstream buf;
    buf << "SoundRoku::SoundRoku: RokuSndFx_PcmSetMaxDelay failed " << sfx_err << "\n";
    myOSystem.logMessage(buf.str(), 0);
    return;
  }
  printf("RokuSndFx_PcmSetMaxDelay(%d)\n", maxDelay);
#endif
  sfx_err = RokuSndFx_PcmStart(myPlayer, 900);
  if(sfx_err != ROKU_SFX_ERR_OK) {
    ostringstream buf;
    buf << "SoundRoku::SoundRoku: RokuSndFx_PcmStart failed " << sfx_err << "\n";
    myOSystem.logMessage(buf.str(), 0);
    return;
  }
  printf("RokuSndFx_PcmStart(%d)\n", 900);
  
  RokuSndFx_PcmStatus status;
  sfx_err = RokuSndFx_PcmGetStatus(myPlayer, &status);
  // printf("status: (err=%d) state=%d delay=%d avail=%u avail_max=%u processed=%u ppdelay=%d\n",
  //  sfx_err, status.state, status.delay, status.avail, status.avail_max, status.processed, status.pp_delay);

#if 0
  // desired.callback = callback;
  // desired.userdata = static_cast<void*>(this);
  sfx_err = RokuSndFx_PcmSetBufferAvailCb(myPlayer, myCallbackSamples, callback, static_cast<void*>(this));
  if(sfx_err != ROKU_SFX_ERR_OK) {
    ostringstream buf;
    buf << "SoundRoku::SoundRoku: RokuSndFx_PcmSetBufferAvailCb failed " << sfx_err << "\n";
    myOSystem.logMessage(buf.str(), 0);
    return;
  }
  printf("RokuSndFx_PcmSetBufferAvailCb(%d)\n", myCallbackSamples);
#endif

  sfx_err = RokuSndFx_PcmSetPeriodCb(myPlayer, perCallback, static_cast<void*>(this));
  if(sfx_err != ROKU_SFX_ERR_OK) {
    ostringstream buf;
    buf << "SoundRoku::SoundRoku: RokuSndFx_PcmSetPeriodCb failed " << sfx_err << "\n";
    myOSystem.logMessage(buf.str(), 0);
    return;
  }
  printf("RokuSndFx_PcmSetPeriodCb()\n");
  
  // TODO??
  myHardwareSamples = status.avail_max;
  myHardwareFrequency = rate;
  myHardwareChannels = stream_prop.num_ch;

  myFrameDuration = 1.0 / rate;
  printf("frame duration = %g\n", myFrameDuration);

  // Pre-compute fragment-related variables as much as possible
  myFragmentSizeLogBase2 = log(myHardwareSamples) / log(2.0);
  myFragmentSizeLogDiv1 = myFragmentSizeLogBase2 / 60.0;
  myFragmentSizeLogDiv2 = (myFragmentSizeLogBase2 - 1) / 60.0;

  myIsInitializedFlag = true;
  // TODO
  //  SDL_PauseAudio(1);

  myOSystem.logMessage("SoundRoku::SoundRoku initialized", 2);
  printf("SoundRoku::SoundRoku initialized\n");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundRoku::~SoundRoku()
{
  myOSystem.logMessage("SoundRoku::~SoundRoku", 2);
  if (myPlayer) {
	RokuSndFx_PcmDestroy(myPlayer);
    printf("RokuSndFx_PcmDestroy(%p)\n", myPlayer);
	myPlayer = nullptr;
  }
  myIsEnabled = myIsInitializedFlag = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::setEnabled(bool state)
{
  printf("SoundRoku::setEnabled(%s)\n", state ? "true" : "false");
  myOSystem.settings().setValue("sound", state);

  myOSystem.logMessage(state ? "SoundRoku::setEnabled(true)" : 
                                "SoundRoku::setEnabled(false)", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::open()
{
  printf("SoundRoku::open()\n");
  myOSystem.logMessage("SoundRoku::open started ...", 2);
  myIsEnabled = false;
  mute(true);
  if(!myIsInitializedFlag || !myOSystem.settings().getBool("sound"))
  {
    myOSystem.logMessage("Sound disabled\n", 1);
    return;
  }

  // Now initialize the TIASound object which will actually generate sound
  myTIASound.outputFrequency(myHardwareFrequency);
  const string& chanResult =
      myTIASound.channels(myHardwareChannels, myNumChannels == 2);

  // Adjust volume to that defined in settings
  myVolume = myOSystem.settings().getInt("volume");
  setVolume(myVolume);

  // Show some info
  ostringstream buf;
  buf << "Sound enabled:"  << endl
      << "  Volume:      " << myVolume << endl
      << "  Frag size:   " << uInt32(myHardwareSamples) << endl
      << "  Frequency:   " << uInt32(myHardwareFrequency) << endl
      << "  Channels:    " << uInt32(myHardwareChannels)
                           << " (" << chanResult << ")" << endl
      << endl;
  myOSystem.logMessage(buf.str(), 1);

  // And start the SDL sound subsystem ...
  myIsEnabled = true;
  mute(false);

  myOSystem.logMessage("SoundRoku::open finished", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::close()
{
  printf("SoundRoku::close()\n");
  if(myIsInitializedFlag)
  {
    myIsEnabled = false;
    printf("SKIPPING RokuSndFx_PcmStop(%s)\n", "false");
    // RokuSndFx_PcmStop(myPlayer, false); // SDL_PauseAudio(1);
    myLastRegisterSetCycle = 0;
    myTIASound.reset();
    myRegWriteQueue.clear();
    myOSystem.logMessage("SoundRoku::close", 2);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::mute(bool state)
{
  printf("SoundRoku::mute(%s)\n", state ? "true" : "false");
  if(myIsInitializedFlag)
  {
    myIsMuted = state;
    // SDL_PauseAudio(myIsMuted ? 1 : 0);
    printf("SKIPPING RokuSndFx_PcmSetVolume(%d)\n", myIsMuted ? 0 : myVolume * 10);
	// RokuSndFx_PcmSetVolume(myPlayer, myIsMuted ? 0 : myVolume * 10);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::reset()
{
  printf("SoundRoku::reset()\n");
  if(myIsInitializedFlag)
  {
    // SDL_PauseAudio(1);
    myLastRegisterSetCycle = 0;
    myTIASound.reset();
    myRegWriteQueue.clear();
    mute(myIsMuted);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::setVolume(Int32 percent)
{
  printf("SoundRoku::setVolume(%d)\n", percent);
  if(myIsInitializedFlag && (percent >= 0) && (percent <= 100))
  {
    myOSystem.settings().setValue("volume", percent);
    // SDL_LockAudio();
    myVolume = percent;
    myTIASound.volume(percent);
	RokuSndFx_PcmSetVolume(myPlayer, percent * 10);
    printf("RokuSndFx_PcmSetVolume(%d)\n", percent * 10);
    // SDL_UnlockAudio();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::adjustVolume(Int8 direction)
{
  printf("SoundRoku::adjustVolume(%d)\n", direction);
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
void SoundRoku::adjustCycleCounter(Int32 amount)
{
	//  printf("SoundRoku::adjustCycleCounter(%d)\n", amount);
  myLastRegisterSetCycle += amount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::setChannels(uInt32 channels)
{
  printf("SoundRoku::setChannels(%u)\n", channels);
  if(channels == 1 || channels == 2)
    myNumChannels = channels;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::setFrameRate(float framerate)
{
	//   printf("SoundRoku::setFrameRate(%g)\n", framerate);
  // Recalculate since frame rate has changed
  // FIXME - should we clear out the queue or adjust the values in it?
  myFragmentSizeLogDiv1 = myFragmentSizeLogBase2 / framerate;
  myFragmentSizeLogDiv2 = (myFragmentSizeLogBase2 - 1) / framerate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::set(uInt16 addr, uInt8 value, Int32 cycle)
{
	//  printf("SoundRoku::set(%u, %u, %d)\n", addr, value, cycle);
  // SDL_LockAudio();

  // First, calculate how many seconds would have past since the last
  // register write on a real 2600
  double delta = double(cycle - myLastRegisterSetCycle) / 1193191.66666667;

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

  // SDL_UnlockAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::processFragment(Int16* stream, uInt32 length)
{
  //  printf("SoundRoku::processFragment(%p, %u)\n", stream, length);
  uInt32 channels = myHardwareChannels;
  length = length / channels;

  // If there are excessive items on the queue then we'll remove some
  if(false && myRegWriteQueue.duration() > myFragmentSizeLogDiv1)
  {
	  printf("REMOVING!!! %g > %g\n", myRegWriteQueue.duration(), myFragmentSizeLogDiv1);
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
      double duration = remaining / myHardwareFrequency;

      // Does the register update occur before the end of the fragment?
      if(info.delta <= duration)
      {
        // If the register update time hasn't already passed then
        // process samples upto the point where it should occur
        if(info.delta > 0.0)
        {
          // Process the fragment upto the next TIA register write.  We
          // round the count passed to process up if needed.
          double samples = (myHardwareFrequency * info.delta);
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

// #define CALLBACK_PRINT
// #define CALLBACK_PRINT2

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::perCallback(void* udata)
{
	static FILE* f = fopen("/tmp/out.wav", "wb");
  SoundRoku* sound = static_cast<SoundRoku*>(udata);
  RokuSndFx_Err sfx_err;
  RokuSndFx_PcmStatus status;
  sfx_err = RokuSndFx_PcmGetStatus(sound->myPlayer, &status);
#ifdef CALLBACK_PRINT2
  printf("status: (err=%d) state=%d delay=%d avail=%u avail_max=%u processed=%u ppdelay=%d\n",
		 sfx_err, status.state, status.delay, status.avail, status.avail_max, status.processed, status.pp_delay);
#endif
  if (status.state == 5) {
	  sfx_err = RokuSndFx_PcmResume(sound->myPlayer, 900);
      if (sfx_err != ROKU_SFX_ERR_OK) {
		  printf("SoundRoku::SoundRoku: RokuSndFx_PcmResume failed %d\n", sfx_err);
	  }
  }
#if 1
  //  uint8_t* buffer = nullptr;
  //  uint32_t avail = 0;
  //  sfx_err = RokuSndFx_PcmGetBuffer(sound->myPlayer, &buffer, &avail);
  //  if (sfx_err != ROKU_SFX_ERR_OK) {
  //	  printf("SoundRoku::SoundRoku: RokuSndFx_PcmGetBuffer failed %d\n", sfx_err);
  //	  return;
  //  }
  double soundAvailable = sound->myRegWriteQueue.duration();
  int framesAvailable = soundAvailable / sound->myFrameDuration - 10;
  if (framesAvailable < 0) {
	  framesAvailable = 0;
  }
  uint32_t write = framesAvailable; // sound->myCallbackSamples; // avail;
  //  if (write + status.delay < 1000) {
  //	  write = 1000 - status.delay;
  //  }
  if (write > status.avail) {
	  write = status.avail;
  }
  if (!write) {
	  return;
  }
  Int16* stream = new Int16[write * 2];
	
  if(sound->myIsEnabled)
  {
	  //	   printf("a: write=%u avail=%u sAvail=%g, fAvail=%d\n", write, status.avail, soundAvailable, framesAvailable);
    // The callback is requesting 8-bit (unsigned) data, but the TIA sound
    // emulator deals in 16-bit (signed) data
    // So, we need to convert the pointer and half the length
	  sound->processFragment(stream, write * 2);
	  double soundAvailable2 = sound->myRegWriteQueue.duration();
	  // printf("a: write=%u avail=%u sAvail=%g, sAvail2=%g fAvail=%d\n", write, status.avail, soundAvailable, soundAvailable2, framesAvailable);
  }
  else {
	  //	   printf("b: write=%u avail=%u sAvail=%g, fAvail=%d\n", write, status.avail, soundAvailable, framesAvailable);
	  //    memset(stream, 0, write * 2 * 2);  // Write 'silence'
  }
  sfx_err = RokuSndFx_PcmWrite(sound->myPlayer, (uint8_t*)stream, write);
  fwrite(stream, 1, write * 2 * 2, f);
  fflush(f);
  if (sfx_err != ROKU_SFX_ERR_OK) {
	  printf("SoundRoku::SoundRoku: RokuSndFx_PcmWrite failed %d\n", sfx_err);
	  return;
  }
  delete[] stream;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::callback(void* udata)
{
#ifdef CALLBACK_PRINT
  printf("SoundRoku::callback()\n");
#endif
  SoundRoku* sound = static_cast<SoundRoku*>(udata);
#if 0
  uint8_t* buffer = nullptr;
  uint32_t avail = 0;
  RokuSndFx_Err sfx_err = RokuSndFx_PcmGetBuffer(sound->myPlayer, &buffer, &avail);
  if (sfx_err != ROKU_SFX_ERR_OK) {
	  printf("SoundRoku::SoundRoku: RokuSndFx_PcmGetBuffer failed %d\n", sfx_err);
	  return;
  }
  printf("avail: %u\n", avail);
  uint32_t write = avail; // sound->myCallbackSamples; // avail;
  if (write > avail) {
	  write = avail;
  }
  if(sound->myIsEnabled)
  {
    printf("a: write=%u\n", write);
    // The callback is requesting 8-bit (unsigned) data, but the TIA sound
    // emulator deals in 16-bit (signed) data
    // So, we need to convert the pointer and half the length
	  sound->processFragment((Int16*)buffer, write);
  }
  else {
    printf("b: write=%u\n", write);
    memset(buffer, 0, write * 2 * 2);  // Write 'silence'
  }
  printf("put\n");
  sfx_err = RokuSndFx_PcmPutBuffer(sound->myPlayer, write);
  if (sfx_err != ROKU_SFX_ERR_OK) {
	  printf("SoundRoku::SoundRoku: RokuSndFx_PcmGetBuffer failed %d\n", sfx_err);
	  return;
  }
#else
  int len = sound->myCallbackSamples;
  Int16* stream = new Int16[len * 2];
  if(sound->myIsEnabled)
  {
    // The callback is requesting 8-bit (unsigned) data, but the TIA sound
    // emulator deals in 16-bit (signed) data
    // So, we need to convert the pointer and half the length
	sound->processFragment(stream, len);
  }
  else {
    /*SDL_*/memset(stream, 0, len * 2 * 2);  // Write 'silence'
  }
  RokuSndFx_Err sfx_err = RokuSndFx_PcmWrite(sound->myPlayer, (uint8_t*)stream, len);
  delete[] stream;
  if (sfx_err != ROKU_SFX_ERR_OK) {
	  printf("SoundRoku::SoundRoku: RokuSndFx_PcmWrite failed %d\n", sfx_err);
	  return;
  }
#ifdef CALLBACK_PRINT
  printf("RokuSndFx_PcmWrite\n");
#endif
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundRoku::save(Serializer& out) const
{
  printf("SoundRoku::save()\n");
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
    myOSystem.logMessage("ERROR: SoundRoku::save", 0);
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundRoku::load(Serializer& in)
{
  printf("SoundRoku::load()\n");
  try
  {
    if(in.getString() != name())
      return false;

    // Only update the TIA sound registers if sound is enabled
    // Make sure to empty the queue of previous sound fragments
    if(myIsInitializedFlag)
    {
      // SDL_PauseAudio(1);
      myRegWriteQueue.clear();
      myTIASound.set(TIARegister::AUDC0, in.getByte());
      myTIASound.set(TIARegister::AUDC1, in.getByte());
      myTIASound.set(TIARegister::AUDF0, in.getByte());
      myTIASound.set(TIARegister::AUDF1, in.getByte());
      myTIASound.set(TIARegister::AUDV0, in.getByte());
      myTIASound.set(TIARegister::AUDV1, in.getByte());
      // if(!myIsMuted) SDL_PauseAudio(0);
    }
    else
      for(int i = 0; i < 6; ++i)
        in.getByte();

    myLastRegisterSetCycle = in.getInt();
  }
  catch(...)
  {
    myOSystem.logMessage("ERROR: SoundRoku::load", 0);
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundRoku::RegWriteQueue::RegWriteQueue(uInt32 capacity)
  : myBuffer(make_ptr<RegWrite[]>(capacity)),
    myCapacity(capacity),
    mySize(0),
    myHead(0),
    myTail(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::RegWriteQueue::clear()
{
  myHead = myTail = mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::RegWriteQueue::dequeue()
{
  if(mySize > 0)
  {
    myHead = (myHead + 1) % myCapacity;
    --mySize;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SoundRoku::RegWriteQueue::duration() const
{
  double duration = 0.0;
  for(uInt32 i = 0; i < mySize; ++i)
  {
    duration += myBuffer[(myHead + i) % myCapacity].delta;
  }
  return duration;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::RegWriteQueue::enqueue(const RegWrite& info)
{
  // If an attempt is made to enqueue more than the queue can hold then
  // we'll enlarge the queue's capacity.
  if(mySize == myCapacity)
    grow();

  myBuffer[myTail] = info;
  myTail = (myTail + 1) % myCapacity;
  ++mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundRoku::RegWrite& SoundRoku::RegWriteQueue::front() const
{
  assert(mySize != 0);
  return myBuffer[myHead];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundRoku::RegWriteQueue::size() const
{
  return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundRoku::RegWriteQueue::grow()
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
