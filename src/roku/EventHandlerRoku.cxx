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
	  prevButtons()
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

#define CHECKEVENT(JOYNUM, _rokubutton, _stellaevent)					\
	if (diff(prevButtons[JOYNUM], btns[JOYNUM], _rokubutton, pressed)) { \
		handleEvent(_stellaevent, pressed);								\
	}

#define CHECKKEYEVENT(JOYNUM, _rokubutton, _stellakeyevent, _stellakeymod) \
	if (diff(prevButtons[JOYNUM], btns[JOYNUM], _rokubutton, pressed)) { \
		handleKeyEvent(_stellakeyevent, _stellakeymod, pressed);		\
	}

void EventHandlerRoku::pollEvent()
{
	static int primaryRemote = 0;
	static int secondaryRemote = 1;
	bool playMode = state() == S_EMULATE || state() == S_PAUSE;

	// TODO - is this needed?
	if (!addedJoystick) {
		addJoystick(new JoystickRoku());
		addedJoystick = true;
	}
	Roku::Input::Event e;
	uint32_t btns[2];
	bool anyChanges = false;
	// Loop backwards in order to prefer remote 0 as primary in ties
	for (int i = 1; i >= 0; --i) {
		Roku::Input::ControllerState state;
		myInputContext->getControllerState(i, state);
		btns[i] = state.buttons();
		if (btns[i] != prevButtons[i]) {
			anyChanges = true;

			// Last button pressed before emulation (!playMode) determines
			// primary as it was probably the button that started the game
			if (!playMode) {
				primaryRemote = i;
				secondaryRemote = 1 - i;
			}
		}
	}

	//////////////
	// TODO - try to use events everywhere instead of keys
	//////////////
//	ConsoleLeftDiffA, ConsoleLeftDiffB,
//	ConsoleRightDiffA, ConsoleRightDiffB,
//	ConsoleSelect, ConsoleReset,
//	ConsoleLeftDiffToggle, ConsoleRightDiffToggle
//	Paddle...

//	UIUp, UIDown, UILeft, UIRight, UIHome, UIEnd, UIPgUp, UIPgDown,
//	UISelect, UINavPrev, UINavNext, UIOK, UICancel, UIPrevDir,

	bool paddle[2] = {0, 0};

	// TODO - make paddle events based on time since last firing
	if (myOSystem.hasConsole()) {
		if (btns[primaryRemote] & (ROKU_INPUT_BUTTON_UP | ROKU_INPUT_BUTTON_DOWN)) {
			paddle[primaryRemote] = BSPF::startsWithIgnoreCase(myOSystem.console().properties().get(Controller_Left), "PADDLES");
			if (paddle[primaryRemote]) {
				if (btns[primaryRemote] & ROKU_INPUT_BUTTON_UP) {
					handleMouseMotionEvent(0, 0, -3, 0, 0);
				}
				if (btns[primaryRemote] & ROKU_INPUT_BUTTON_DOWN) {
					handleMouseMotionEvent(0, 0, 3, 0, 0);
				}
			}
		}
		if (btns[secondaryRemote] & (ROKU_INPUT_BUTTON_UP | ROKU_INPUT_BUTTON_DOWN)) {
			paddle[secondaryRemote] = BSPF::startsWithIgnoreCase(myOSystem.console().properties().get(Controller_Right), "PADDLES");
			if (paddle[secondaryRemote]) {
				if (btns[secondaryRemote] & ROKU_INPUT_BUTTON_UP) {
					handleMouseMotionEvent(0, 0, 0, -3, 0);
				}
				if (btns[secondaryRemote] & ROKU_INPUT_BUTTON_DOWN) {
					handleMouseMotionEvent(0, 0, 0, 3, 0);
				}
			}
		}
	}
	if (anyChanges) {
		bool pressed = false;
		if (btns[primaryRemote] != prevButtons[primaryRemote]) {
			if (playMode) {
				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_RIGHT, Event::JoystickZeroUp);
//				handleJoyAxisEvent(1, 1, -32000);
//				handleKeyEvent(StellaKey(KBDK_W), KBDM_NONE, pressed);
//				handleKeyEvent(StellaKey(KBDK_I), KBDM_NONE, pressed);
				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_LEFT, Event::JoystickZeroDown);
//				handleKeyEvent(StellaKey(KBDK_S), KBDM_NONE, pressed);
//				handleKeyEvent(StellaKey(KBDK_K), KBDM_NONE, pressed);
//				handleJoyAxisEvent(1, 1, 32000);
				if (!paddle[primaryRemote]) {
					CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_UP, Event::JoystickZeroLeft);
//					handleKeyEvent(StellaKey(KBDK_A), KBDM_NONE, pressed);
//					handleKeyEvent(StellaKey(KBDK_J), KBDM_NONE, pressed);
//					handleJoyAxisEvent(1, 0, -32000);
					CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_DOWN, Event::JoystickZeroRight);
//					handleKeyEvent(StellaKey(KBDK_D), KBDM_NONE, pressed);
//					handleKeyEvent(StellaKey(KBDK_L), KBDM_NONE, pressed);
//					handleJoyAxisEvent(1, 0, 32000);
				}
				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_B, Event::JoystickZeroFire);
//				handleJoyEvent(1, 0, pressed);
                CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_A, Event::JoystickZeroFire);
//				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_A, Event::PauseMode);

				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_BACK, StellaKey(KBDK_ESCAPE), KBDM_NONE);
//				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_BACK, Event::MenuMode);
				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_INSTANT_REPLAY, Event::ConsoleReset); // F2
//				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_INSTANT_REPLAY, StellaKey(KBDK_F2), KBDM_NONE); // RESET
				// CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_SELECT, Event::NONE); // SELECT is OK, which is next to arrows. It is too easy to hit by accident
                CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_PLAY, StellaKey(KBDK_PAUSE), KBDM_NONE);
                CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_INFO, Event::ConsoleLeftDiffToggle);
				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_RWD, Event::ConsoleSelect); // F1
				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_FWD, Event::ConsoleSelect); // F1
//				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_SELECT, StellaKey(KBDK_RETURN), KBDM_NONE);
//				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_INFO, StellaKey(KBDK_F2), KBDM_NONE); // RESET
//				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_PLAY, StellaKey(KBDK_F1), KBDM_NONE); // SELECT GAME?
			} else {
//				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_RIGHT, Event::UIUp);
//				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_LEFT, Event::UIDown);
//				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_UP, Event::UILeft);
//				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_DOWN, Event::UIRight);
//				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_B, Event::UISelect);
//				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_A, Event::UICancel);
				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_RIGHT, StellaKey(KBDK_UP), KBDM_NONE);
				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_LEFT, StellaKey(KBDK_DOWN), KBDM_NONE);
				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_UP, StellaKey(KBDK_LEFT), KBDM_NONE);
				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_DOWN, StellaKey(KBDK_RIGHT), KBDM_NONE);
				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_SELECT, StellaKey(KBDK_RETURN), KBDM_NONE);
				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_B, StellaKey(KBDK_RETURN), KBDM_NONE);
				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_A, StellaKey(KBDK_ESCAPE), KBDM_NONE);
				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_BACK, StellaKey(KBDK_ESCAPE), KBDM_NONE);
//				CHECKEVENT(primaryRemote, ROKU_INPUT_BUTTON_BACK, Event::UICancel);
				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_RWD, StellaKey(KBDK_TAB), KBDM_LSHIFT);
				CHECKKEYEVENT(primaryRemote, ROKU_INPUT_BUTTON_FWD, StellaKey(KBDK_TAB), KBDM_NONE);
			}
			prevButtons[primaryRemote] = btns[primaryRemote];
		}
		if (btns[secondaryRemote] != prevButtons[secondaryRemote]) {
			if (playMode) {
				CHECKEVENT(secondaryRemote, ROKU_INPUT_BUTTON_RIGHT, Event::JoystickOneUp);
				CHECKEVENT(secondaryRemote, ROKU_INPUT_BUTTON_LEFT, Event::JoystickOneDown);
				if (!paddle[secondaryRemote]) {
					CHECKEVENT(secondaryRemote, ROKU_INPUT_BUTTON_UP, Event::JoystickOneLeft);
					CHECKEVENT(secondaryRemote, ROKU_INPUT_BUTTON_DOWN, Event::JoystickOneRight);
				}
                CHECKEVENT(secondaryRemote, ROKU_INPUT_BUTTON_B, Event::JoystickOneFire);
                CHECKEVENT(secondaryRemote, ROKU_INPUT_BUTTON_A, Event::JoystickOneFire);
				CHECKKEYEVENT(secondaryRemote, ROKU_INPUT_BUTTON_BACK, StellaKey(KBDK_ESCAPE), KBDM_NONE);
                CHECKEVENT(secondaryRemote, ROKU_INPUT_BUTTON_INSTANT_REPLAY, Event::ConsoleReset); //F2
				// CHECKEVENT(secondaryRemote, ROKU_INPUT_BUTTON_SELECT, Event::NONE); // SELECT is OK, which is next to arrows. It is too easy to hit by accident
                CHECKKEYEVENT(secondaryRemote, ROKU_INPUT_BUTTON_PLAY, StellaKey(KBDK_PAUSE), KBDM_NONE);
                CHECKEVENT(secondaryRemote, ROKU_INPUT_BUTTON_INFO, Event::ConsoleRightDiffToggle);
				CHECKEVENT(secondaryRemote, ROKU_INPUT_BUTTON_RWD, Event::ConsoleSelect); // F1
				CHECKEVENT(secondaryRemote, ROKU_INPUT_BUTTON_FWD, Event::ConsoleSelect); // F1
			} else {
				CHECKKEYEVENT(secondaryRemote, ROKU_INPUT_BUTTON_RIGHT, StellaKey(KBDK_UP), KBDM_NONE);
				CHECKKEYEVENT(secondaryRemote, ROKU_INPUT_BUTTON_LEFT, StellaKey(KBDK_DOWN), KBDM_NONE);
				CHECKKEYEVENT(secondaryRemote, ROKU_INPUT_BUTTON_UP, StellaKey(KBDK_LEFT), KBDM_NONE);
				CHECKKEYEVENT(secondaryRemote, ROKU_INPUT_BUTTON_DOWN, StellaKey(KBDK_RIGHT), KBDM_NONE);
				CHECKKEYEVENT(secondaryRemote, ROKU_INPUT_BUTTON_SELECT, StellaKey(KBDK_RETURN), KBDM_NONE);
				CHECKKEYEVENT(secondaryRemote, ROKU_INPUT_BUTTON_B, StellaKey(KBDK_RETURN), KBDM_NONE);
				CHECKKEYEVENT(secondaryRemote, ROKU_INPUT_BUTTON_A, StellaKey(KBDK_ESCAPE), KBDM_NONE);
				CHECKKEYEVENT(secondaryRemote, ROKU_INPUT_BUTTON_BACK, StellaKey(KBDK_ESCAPE), KBDM_NONE);
				CHECKKEYEVENT(secondaryRemote, ROKU_INPUT_BUTTON_RWD, StellaKey(KBDK_TAB), KBDM_LSHIFT);
				CHECKKEYEVENT(secondaryRemote, ROKU_INPUT_BUTTON_FWD, StellaKey(KBDK_TAB), KBDM_NONE);
			}
			prevButtons[secondaryRemote] = btns[secondaryRemote];
		}
	}

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

