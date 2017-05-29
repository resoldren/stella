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
//============================================================================

#ifndef EVENTHANDLER_ROKU_HXX
#define EVENTHANDLER_ROKU_HXX

#include "EventHandler.hxx"

#include <ndkwrappers/RokuInput.h>

class EventHandlerRoku : public EventHandler
{
  public:
    /**
      Create a new Roku event handler object
    */
    EventHandlerRoku(OSystem& osystem);
    virtual ~EventHandlerRoku();

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
		return "TODO"; // SDL_GetScancodeName(SDL_Scancode(key));
    }

    /**
      Collects and dispatches any pending Roku events.
    */
    void pollEvent() override;

  private:
    // SDL_Event myEvent;
	Roku::Input::Context* myInputContext;
	bool addedJoystick;
	uint32_t prevButtons[2];

  private:
    // Following constructors and assignment operators not supported
    EventHandlerRoku() = delete;
    EventHandlerRoku(const EventHandlerRoku&) = delete;
    EventHandlerRoku(EventHandlerRoku&&) = delete;
    EventHandlerRoku& operator=(const EventHandlerRoku&) = delete;
    EventHandlerRoku& operator=(EventHandlerRoku&&) = delete;

    class JoystickRoku : public StellaJoystick
    {
      public:
        JoystickRoku();
    };
};

#endif
