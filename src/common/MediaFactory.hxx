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

#ifndef MEDIA_FACTORY_HXX
#define MEDIA_FACTORY_HXX

#include "bspf.hxx"

#include "OSystem.hxx"
#include "Settings.hxx"
#include "SerialPort.hxx"
#if defined(BSPF_UNIX) || defined(BSPF_ROKU)
  #include "SerialPortUNIX.hxx"
  #include "SettingsUNIX.hxx"
  #include "OSystemUNIX.hxx"
#elif defined(BSPF_WINDOWS)
  #include "SerialPortWINDOWS.hxx"
  #include "SettingsWINDOWS.hxx"
  #include "OSystemWINDOWS.hxx"
#elif defined(BSPF_MAC_OSX)
  #include "SerialPortMACOSX.hxx"
  #include "SettingsMACOSX.hxx"
  #include "OSystemMACOSX.hxx"
  extern "C" {
    int stellaMain(int argc, char* argv[]);
  }
#else
  #error Unsupported platform!
#endif

#if defined(BSPF_VIDEO_SDL2)
  #include "FrameBufferSDL2.hxx"
  #define HAVE_VIDEO
#endif

#if defined(BSPF_VIDEO_ROKU)
  #include "FrameBufferRoku.hxx"
  #define HAVE_VIDEO
#endif

#ifndef HAVE_VIDEO
  #error No video support
#endif

#if defined(BSPF_EVENTS_SDL2)
  #include "EventHandlerSDL2.hxx"
  #define HAVE_EVENTS
#endif

#if defined(BSPF_EVENTS_ROKU)
 #include "EventHandlerRoku.hxx"
  #define HAVE_EVENTS
#endif

#ifndef HAVE_EVENTS
  #error No event support
#endif

#ifdef SOUND_SUPPORT
  #if defined(BSPF_AUDIO_SDL2)
    #include "SoundSDL2.hxx"
  #endif
  #if defined(BSPF_AUDIO_ROKU)
    #include "SoundRoku.hxx"
  #endif
#endif

#include "SoundNull.hxx"

/**
  This class deals with the different framebuffer/sound/event
  implementations for the various ports of Stella, and always returns a
  valid object based on the specific port and restrictions on that port.

  As of SDL2, this code is greatly simplified.  However, it remains here
  in case we ever have multiple backend implementations again (should
  not be necessary since SDL2 covers this nicely).

  @author  Stephen Anthony
*/
class MediaFactory
{
  public:
    static unique_ptr<OSystem> createOSystem()
    {
    #if defined(BSPF_UNIX) || defined(BSPF_ROKU)
      return make_ptr<OSystemUNIX>();
    #elif defined(BSPF_WINDOWS)
      return make_ptr<OSystemWINDOWS>();
    #elif defined(BSPF_MAC_OSX)
      return make_ptr<OSystemMACOSX>();
    #else
      #error Unsupported platform for OSystem!
    #endif
    }

    static unique_ptr<Settings> createSettings(OSystem& osystem)
    {
    #if defined(BSPF_UNIX) || defined(BSPF_ROKU)
      return make_ptr<SettingsUNIX>(osystem);
    #elif defined(BSPF_WINDOWS)
      return make_ptr<SettingsWINDOWS>(osystem);
    #elif defined(BSPF_MAC_OSX)
      return make_ptr<SettingsMACOSX>(osystem);
    #else
      #error Unsupported platform for Settings!
    #endif
    }

    static unique_ptr<SerialPort> createSerialPort()
    {
    #if defined(BSPF_UNIX) || defined(BSPF_ROKU)
      return make_ptr<SerialPortUNIX>();
    #elif defined(BSPF_WINDOWS)
      return make_ptr<SerialPortWINDOWS>();
    #elif defined(BSPF_MAC_OSX)
      return make_ptr<SerialPortMACOSX>();
    #else
      return make_ptr<SerialPort>();
    #endif
    }

#if defined(BSPF_VIDEO_SDL2) || defined(BSPF_AUDIO_SDL2) || defined(BSPF_EVENTS_SDL2)
    static string getSDL2Info()
    {
      ostringstream info;
      SDL_version ver;
      SDL_GetVersion(&ver);

      info << "SDL " << int(ver.major)
           << "." << int(ver.minor)
           << "."<< int(ver.patch);
      return info.str();
    }
#endif

    static string getVideoInfo()
    {
      #if defined(BSPF_VIDEO_SDL2)
        return getSDL2Info();
      #else
        return "Invalid";
      #endif
    }

    static unique_ptr<FrameBuffer> createVideo(OSystem& osystem)
    {
      std::string video = osystem.settings().getString("lib.video");
      #if defined (BSPF_VIDEO_DEFAULT)
        if (video.empty())
	  video = BSPF_VIDEO_DEFAULT;
      #endif
      #if defined(BSPF_VIDEO_SDL2)
        if (video == "SDL2")
	{
	  return make_ptr<FrameBufferSDL2>(osystem);
        }
      #endif
      #if defined(BSPF_VIDEO_ROKU)
        if (video == "Roku")
	{
	  return make_ptr<FrameBufferRoku>(osystem);
        }
      #endif
      throw runtime_error("Unsupported lib.video value");
    }

    static string getAudioInfo()
    {
      #if defined(BSPF_AUDIO_SDL2)
        return getSDL2Info();
      #else
        return "Invalid";
      #endif
    }

    static unique_ptr<Sound> createAudio(OSystem& osystem)
    {
      std::string audio = osystem.settings().getString("lib.audio");
      printf("audio: %s\n", audio.c_str());
      #ifdef SOUND_SUPPORT
        #if defined (BSPF_AUDIO_DEFAULT)
          if (audio.empty())
            audio = BSPF_AUDIO_DEFAULT;
          printf("audio: %s\n", audio.c_str());
        #endif
        #if defined(BSPF_AUDIO_SDL2)
          if (audio == "SDL2")
          {
            return make_ptr<SoundSDL2>(osystem);
          }
        #endif
        #if defined(BSPF_AUDIO_ROKU)
          if (audio == "Roku")
	  {
            printf("Making Roku Sound\n");
            return make_ptr<SoundRoku>(osystem);
          }
        #endif
      #endif
      if (audio == "none")
      {
        return make_ptr<SoundNull>(osystem);
      }
      // TODO - what to return
      return make_ptr<SoundNull>(osystem);
      return nullptr;
    }

    static string getEventInfo()
    {
      #if defined(BSPF_EVENTS_SDL2)
        return getSDL2Info();
      #else
        return "Invalid";
      #endif
    }

    static unique_ptr<EventHandler> createEventHandler(OSystem& osystem)
    {
      std::string events = osystem.settings().getString("lib.events");
      #if defined (BSPF_EVENTS_DEFAULT)
        if (events.empty())
	  events = BSPF_EVENTS_DEFAULT;
      #endif
      #if defined(BSPF_EVENTS_SDL2)
        if (events == "SDL2")
        {
          return make_ptr<EventHandlerSDL2>(osystem);
        }
      #endif
      #if defined(BSPF_EVENTS_ROKU)
        if (events == "Roku")
        {
          return make_ptr<EventHandlerRoku>(osystem);
        }
      #endif
      return nullptr;
    }

  #ifndef HAVE_GETTIMEOFDAY
    #if defined(BSPF_VIDEO_SDL2) || defined(BSPF_AUDIO_SDL2) || defined(BSPF_EVENTS_SDL2)
    static uInt32 getTicks()
    {
      return SDL_GetTicks();
    }
    #else
      #error Unsupported platform for getTicks!
    #endif
  #endif

  #if defined(BSPF_VIDEO_SDL2) || defined(BSPF_AUDIO_SDL2) || defined(BSPF_EVENTS_SDL2)
    static void delay(uInt32 milliseconds)
    {
      SDL_Delay(milliseconds);
    }
  #else
    static void delay(uInt32 milliseconds)
    {
      struct timespec ts;
      ts.tv_sec = milliseconds / 1000;
      ts.tv_nsec = (milliseconds % 1000) * 1000000;
      while (1) {
        struct timespec rem;
        int r = nanosleep(&ts, &rem);
        if (r == 0)
        {
          break;
        }
        else if (errno == EINTR)
        {
          ts = rem;
        }
        else
        {
          break; // Error
        }
      }
    }
  #endif

  private:
    // Following constructors and assignment operators not supported
    MediaFactory() = delete;
    MediaFactory(const MediaFactory&) = delete;
    MediaFactory(MediaFactory&&) = delete;
    MediaFactory& operator=(const MediaFactory&) = delete;
    MediaFactory& operator=(MediaFactory&&) = delete;
};

#endif
