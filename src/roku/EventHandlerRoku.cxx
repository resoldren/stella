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

#include "OSystem.hxx"
#include "EventHandlerRoku.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandlerRoku::EventHandlerRoku(OSystem& osystem)
	: EventHandler(osystem),
	  myInputContext(new Roku::Input::Context()),
	  addedJoystick(false),
	  prevButtons(0)
{
}

EventHandlerRoku::~EventHandlerRoku()
{
	delete myInputContext;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandlerRoku::resetEvents()
{
  // Reset events almost immediately after starting emulation mode
  // We wait a little while, since 'hold' events may be present, and we want
  // time for the ROM to process them
  (static_cast<EventHandlerRoku*>(this))->EventHandler::resetEvents();
  //  SDL_AddTimer(500, resetEventsCallback, static_cast<void*>(this));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 EventHandlerRoku::resetEventsCallback(uInt32 interval, void* param)
{
	//  (static_cast<EventHandlerRoku*>(param))->EventHandler::resetEvents();
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandlerRoku::enableTextEvents(bool enable)
{
#if 0
  if(enable)
    SDL_StartTextInput();
  else
    SDL_StopTextInput();
#endif
}

static bool diff(uint32_t prev, uint32_t now, uint32_t mask, bool& pressed)
{
	bool ans = (now & mask) != (prev & mask);
	if (ans) {
		pressed = (now & mask) != 0;
	}
	return ans;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandlerRoku::pollEvent()
{
	if (!addedJoystick) {
		addJoystick(new JoystickRoku());
		addedJoystick = true;
	}
	Roku::Input::Event e;
	Roku::Input::ControllerState state;
	myInputContext->getControllerState(0, state);
	uint32_t btns = state.buttons();

#if 0
	switch (state()) {
		case EMULATE:
			break;
		default:
			break;
	}
#endif
	if (btns != prevButtons) {
		bool pressed = false;
		if (diff(prevButtons, btns, ROKU_INPUT_BUTTON_RIGHT, pressed)) {
			handleKeyEvent(StellaKey(KBDK_UP), KBDM_NONE, pressed);
			handleKeyEvent(StellaKey(KBDK_W), KBDM_NONE, pressed);
			handleKeyEvent(StellaKey(KBDK_I), KBDM_NONE, pressed);
			handleJoyAxisEvent(1, 1, -32000);
		}
		if (diff(prevButtons, btns, ROKU_INPUT_BUTTON_LEFT, pressed)) {
			handleKeyEvent(StellaKey(KBDK_DOWN), KBDM_NONE, pressed);
			handleKeyEvent(StellaKey(KBDK_S), KBDM_NONE, pressed);
			handleKeyEvent(StellaKey(KBDK_K), KBDM_NONE, pressed);
			handleJoyAxisEvent(1, 1, 32000);
		}
		if (diff(prevButtons, btns, ROKU_INPUT_BUTTON_UP, pressed)) {
			handleKeyEvent(StellaKey(KBDK_LEFT), KBDM_NONE, pressed);
			handleKeyEvent(StellaKey(KBDK_A), KBDM_NONE, pressed);
			handleKeyEvent(StellaKey(KBDK_J), KBDM_NONE, pressed);
			handleJoyAxisEvent(1, 0, -32000);
		}
		if (diff(prevButtons, btns, ROKU_INPUT_BUTTON_DOWN, pressed)) {
			handleKeyEvent(StellaKey(KBDK_RIGHT), KBDM_NONE, pressed);
			handleKeyEvent(StellaKey(KBDK_D), KBDM_NONE, pressed);
			handleKeyEvent(StellaKey(KBDK_L), KBDM_NONE, pressed);
			handleJoyAxisEvent(1, 0, 32000);
		}
		if (diff(prevButtons, btns, ROKU_INPUT_BUTTON_BACK, pressed)) {
			handleKeyEvent(StellaKey(KBDK_ESCAPE), KBDM_NONE, pressed);
		}
		if (diff(prevButtons, btns, ROKU_INPUT_BUTTON_INSTANT_REPLAY, pressed)) {
			handleKeyEvent(StellaKey(KBDK_F2), KBDM_NONE, pressed); // RESET
		}
		if (diff(prevButtons, btns, ROKU_INPUT_BUTTON_SELECT, pressed)) {
			handleKeyEvent(StellaKey(KBDK_RETURN), KBDM_NONE, pressed);
			handleJoyEvent(1, 0, pressed);
			handleKeyEvent(StellaKey(KBDK_SPACE), KBDM_NONE, pressed); // << this seems to be the one to make pitfall jump
		}
		if (diff(prevButtons, btns, ROKU_INPUT_BUTTON_INFO, pressed)) {
			handleKeyEvent(StellaKey(KBDK_F2), KBDM_NONE, pressed); // RESET
		}
		if (diff(prevButtons, btns, ROKU_INPUT_BUTTON_RWD, pressed)) {
			handleKeyEvent(StellaKey(KBDK_TAB), KBDM_LSHIFT, pressed);
		}
		if (diff(prevButtons, btns, ROKU_INPUT_BUTTON_PLAY, pressed)) {
			handleKeyEvent(StellaKey(KBDK_F1), KBDM_NONE, pressed); // SELECT GAME?
		}
		if (diff(prevButtons, btns, ROKU_INPUT_BUTTON_FWD, pressed)) {
			handleKeyEvent(StellaKey(KBDK_TAB), KBDM_NONE, pressed);
		}
		if (diff(prevButtons, btns, ROKU_INPUT_BUTTON_A, pressed)) {
			handleKeyEvent(StellaKey(KBDK_ESCAPE), KBDM_NONE, pressed);
		}
		if (diff(prevButtons, btns, ROKU_INPUT_BUTTON_B, pressed)) {
			handleKeyEvent(StellaKey(KBDK_RETURN), KBDM_NONE, pressed);
			handleJoyEvent(1, 0, pressed);
			handleKeyEvent(StellaKey(KBDK_SPACE), KBDM_NONE, pressed);
		}
		prevButtons = btns;
	}
	
#if 0
	if (btns & ROKU_INPUT_BUTTON_UP) { printf("DOWN: UP\n"); }
	if (btns & ROKU_INPUT_BUTTON_LEFT) { printf("DOWN: LEFT\n"); }
	if (btns & ROKU_INPUT_BUTTON_RIGHT) { printf("DOWN: RIGHT\n"); }
	if (btns & ROKU_INPUT_BUTTON_DOWN) { printf("DOWN: DOWN\n"); }
	if (btns & ROKU_INPUT_BUTTON_BACK) { printf("DOWN: BACK\n"); }
	if (btns & ROKU_INPUT_BUTTON_INSTANT_REPLAY) { printf("DOWN: INSTANT_REPLAY\n"); }
	if (btns & ROKU_INPUT_BUTTON_SELECT) { printf("DOWN: SELECT\n"); }
	if (btns & ROKU_INPUT_BUTTON_INFO) { printf("DOWN: INFO\n"); }
	if (btns & ROKU_INPUT_BUTTON_RWD) { printf("DOWN: RWD\n"); }
	if (btns & ROKU_INPUT_BUTTON_PLAY) { printf("DOWN: PLAY\n"); }
	if (btns & ROKU_INPUT_BUTTON_FWD) { printf("DOWN: FWD\n"); }
	if (btns & ROKU_INPUT_BUTTON_A) { printf("DOWN: B\n"); }
	if (btns & ROKU_INPUT_BUTTON_B) { printf("DOWN: B\n"); }
#endif
	
#if 0
	while (myInputContext->getEvent(e)) {
		switch (e.type()) {
		case ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED:
		case ROKU_INPUT_EVENT_TYPE_BUTTON_RELEASED:
			printf("key: %s\n", e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED ? "true" : "false");
			switch (e.button()) {
			case ROKU_INPUT_BUTTON_UP:
			    printf("key up %d %d\n", KBDK_UP, StellaKey(KBDK_UP));
				handleKeyEvent(StellaKey(KBDK_UP), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				handleKeyEvent(StellaKey(KBDK_W), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				handleKeyEvent(StellaKey(KBDK_I), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				handleJoyAxisEvent(1, 1, -32000);
				break;
			case ROKU_INPUT_BUTTON_DOWN:
			    printf("key down %d %d\n", KBDK_DOWN, StellaKey(KBDK_DOWN));
				handleKeyEvent(StellaKey(KBDK_DOWN), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				handleKeyEvent(StellaKey(KBDK_S), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				handleKeyEvent(StellaKey(KBDK_K), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				handleJoyAxisEvent(1, 1, 32000);
				break;
			case ROKU_INPUT_BUTTON_LEFT:
			    printf("key left %d %d\n", KBDK_LEFT, StellaKey(KBDK_LEFT));
				handleKeyEvent(StellaKey(KBDK_LEFT), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				handleKeyEvent(StellaKey(KBDK_A), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				handleKeyEvent(StellaKey(KBDK_J), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				handleJoyAxisEvent(1, 0, -32000);
				break;
			case ROKU_INPUT_BUTTON_RIGHT:
			    printf("key right %d %d\n", KBDK_RIGHT, StellaKey(KBDK_RIGHT));
				handleKeyEvent(StellaKey(KBDK_RIGHT), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				handleKeyEvent(StellaKey(KBDK_D), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				handleKeyEvent(StellaKey(KBDK_L), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				handleJoyAxisEvent(1, 0, 32000);
				break;
			case ROKU_INPUT_BUTTON_SELECT:
			    printf("key return %d %d\n", KBDK_RETURN, StellaKey(KBDK_RETURN));
				handleJoyEvent(1, 0, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED ? 1 : 0);
				 handleKeyEvent(StellaKey(KBDK_RETURN), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				 handleKeyEvent(StellaKey(KBDK_SPACE), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				break;
			case ROKU_INPUT_BUTTON_INFO:
				break;
			case ROKU_INPUT_BUTTON_INSTANT_REPLAY:
				handleKeyEvent(StellaKey(KBDK_F2), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				break;
			case ROKU_INPUT_BUTTON_BACK:
			    printf("key esc %d %d\n", KBDK_ESCAPE, StellaKey(KBDK_ESCAPE));
				handleKeyEvent(StellaKey(KBDK_ESCAPE), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				break;
			case ROKU_INPUT_BUTTON_FWD:
				handleKeyEvent(StellaKey(KBDK_TAB), KBDM_NONE, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				break;
			case ROKU_INPUT_BUTTON_PLAY:
				break;
			case ROKU_INPUT_BUTTON_RWD:
				handleKeyEvent(StellaKey(KBDK_TAB), KBDM_LSHIFT, e.type() == ROKU_INPUT_EVENT_TYPE_BUTTON_PRESSED);
				break;
			break;
			}
		default:
			break;
		}
	}
#endif
	
#if 0
  while(SDL_PollEvent(&myEvent))
  {
    switch(myEvent.type)
    {
      // keyboard events
      case SDL_KEYUP:
      case SDL_KEYDOWN:
      {
        if(!myEvent.key.repeat)
          handleKeyEvent(StellaKey(myEvent.key.keysym.scancode),
                         StellaMod(myEvent.key.keysym.mod),
                         myEvent.key.type == SDL_KEYDOWN);
        break;
      }

      case SDL_TEXTINPUT:
      {
        handleTextEvent(*(myEvent.text.text));
        break;
      }

      case SDL_MOUSEMOTION:
      {
        handleMouseMotionEvent(myEvent.motion.x, myEvent.motion.y,
                               myEvent.motion.xrel, myEvent.motion.yrel, 0);
        break;
      }

      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
      {
        bool pressed = myEvent.button.type == SDL_MOUSEBUTTONDOWN;
        switch(myEvent.button.button)
        {
          case SDL_BUTTON_LEFT:
            handleMouseButtonEvent(pressed ? EVENT_LBUTTONDOWN : EVENT_LBUTTONUP,
                                   myEvent.button.x, myEvent.button.y);
            break;
          case SDL_BUTTON_RIGHT:
            handleMouseButtonEvent(pressed ? EVENT_RBUTTONDOWN : EVENT_RBUTTONUP,
                                   myEvent.button.x, myEvent.button.y);
            break;
        }
        break;
      }

      case SDL_MOUSEWHEEL:
      {
        int x, y;
        SDL_GetMouseState(&x, &y);  // we need mouse position too
        if(myEvent.wheel.y < 0)
          handleMouseButtonEvent(EVENT_WHEELDOWN, x, y);
        else if(myEvent.wheel.y > 0)
          handleMouseButtonEvent(EVENT_WHEELUP, x, y);
        break;
      }


      case SDL_QUIT:
      {
        handleEvent(Event::Quit, 1);
        break;  // SDL_QUIT
      }

      case SDL_WINDOWEVENT:
        switch(myEvent.window.event)
        {
          case SDL_WINDOWEVENT_SHOWN:
            handleSystemEvent(EVENT_WINDOW_SHOWN);
            break;
          case SDL_WINDOWEVENT_HIDDEN:
            handleSystemEvent(EVENT_WINDOW_HIDDEN);
            break;
          case SDL_WINDOWEVENT_EXPOSED:
            handleSystemEvent(EVENT_WINDOW_EXPOSED);
            break;
          case SDL_WINDOWEVENT_MOVED:
            handleSystemEvent(EVENT_WINDOW_MOVED,
                              myEvent.window.data1, myEvent.window.data1);
            break;
          case SDL_WINDOWEVENT_RESIZED:
            handleSystemEvent(EVENT_WINDOW_RESIZED,
                              myEvent.window.data1, myEvent.window.data1);
            break;
          case SDL_WINDOWEVENT_MINIMIZED:
            handleSystemEvent(EVENT_WINDOW_MINIMIZED);
            break;
          case SDL_WINDOWEVENT_MAXIMIZED:
            handleSystemEvent(EVENT_WINDOW_MAXIMIZED);
            break;
          case SDL_WINDOWEVENT_RESTORED:
            handleSystemEvent(EVENT_WINDOW_RESTORED);
            break;
          case SDL_WINDOWEVENT_ENTER:
            handleSystemEvent(EVENT_WINDOW_ENTER);
            break;
          case SDL_WINDOWEVENT_LEAVE:
            handleSystemEvent(EVENT_WINDOW_LEAVE);
            break;
          case SDL_WINDOWEVENT_FOCUS_GAINED:
            handleSystemEvent(EVENT_WINDOW_FOCUS_GAINED);
            break;
          case SDL_WINDOWEVENT_FOCUS_LOST:
            handleSystemEvent(EVENT_WINDOW_FOCUS_LOST);
            break;
        }
        break;  // SDL_WINDOWEVENT
    }
  }
#endif
}

EventHandlerRoku::JoystickRoku::JoystickRoku()
{
	initialize(1, "Roku Remote", 2, 7, 0, 0);
}

