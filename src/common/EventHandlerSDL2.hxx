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

#ifndef EVENTHANDLER_SDL2_HXX
#define EVENTHANDLER_SDL2_HXX

#include "SDL_lib.hxx"
#include "EventHandler.hxx"

/**
  This class handles event collection from the point of view of the specific
  backend toolkit (SDL2).  It converts from SDL2-specific events into events
  that the Stella core can understand.

  @author  Stephen Anthony
*/
class EventHandlerSDL2 : public EventHandler
{
  public:
    /**
      Create a new SDL2 event handler object
    */
    EventHandlerSDL2(OSystem& osystem);
    virtual ~EventHandlerSDL2() = default;

    void resetEvents() override;

  private:
    // Callback function invoked by the event-reset timer
    static uInt32 resetEventsCallback(uInt32 interval, void* param);

    /**
      Enable/disable text events (distinct from single-key events).
    */
    void enableTextEvents(bool enable) override;

    /**
      Returns the human-readable name for a StellaKey.
    */
    const char* nameForKey(StellaKey key) const override {
      return SDL_GetScancodeName(SDL_Scancode(key));
    }

    /**
      Collects and dispatches any pending SDL2 events.
    */
    void pollEvent() override;

  private:
    SDL_Event myEvent;

    // A thin wrapper around a basic StellaJoystick, holding the pointer to
    // the underlying SDL stick.
    class JoystickSDL2 : public StellaJoystick
    {
      public:
        JoystickSDL2(int idx);
        virtual ~JoystickSDL2();

      private:
        SDL_Joystick* myStick;
    };

  private:
    // Following constructors and assignment operators not supported
    EventHandlerSDL2() = delete;
    EventHandlerSDL2(const EventHandlerSDL2&) = delete;
    EventHandlerSDL2(EventHandlerSDL2&&) = delete;
    EventHandlerSDL2& operator=(const EventHandlerSDL2&) = delete;
    EventHandlerSDL2& operator=(EventHandlerSDL2&&) = delete;
};

#endif
