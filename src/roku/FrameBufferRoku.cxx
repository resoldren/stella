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

#include <sstream>
#include <time.h>
#include <fstream>

#include "bspf.hxx"

#include "Console.hxx"
#include "Font.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "TIA.hxx"

#include "FBSurfaceRoku.hxx"
#include "FrameBufferRoku.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferRoku::FrameBufferRoku(OSystem& osystem)
  : FrameBuffer(osystem),
	//    myWindow(nullptr),
	//    myRenderer(nullptr),
    myGraphics(R2D2::RoGraphics::Create()),

    myScreen(myGraphics->CreateScreen(378, 213, true)), // SMALLEST 16x9

    //myScreen(myGraphics->CreateScreen(426, 240, true)), // OKAY
	//myScreen(myGraphics->CreateScreen(394, 222, true)), // OKAY
	// myScreen(myGraphics->CreateScreen(362, 204, true)), // JUST TOO SMALL
	//myScreen(myGraphics->CreateScreen(320, 180, true)), // NO GO
	//myScreen(myGraphics->CreateScreen(213, 120, true)), // NO GO
	
    //myScreen(myGraphics->CreateScreen(640, 480, true)), // Minimum needed for launcher window, but stretches screen
    //myScreen(myGraphics->CreateScreen(852, 480, true)), // Works with launcher window, but more is off screen than 640x480

	//myScreen(myGraphics->CreateScreen(160, 192, true)), // NO GO
	//myScreen(myGraphics->CreateScreen(1280, 720, true)), // OKAY
    myDirtyFlag(true)
{
	//	printf("FrameBufferRoku::FrameBufferRoku %p ()\n", this);
#if 0
  // Initialize Roku context
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) < 0)
  {
    ostringstream buf;
    buf << "ERROR: Couldn't initialize SDL: " << SDL_GetError() << endl;
    myOSystem.logMessage(buf.str(), 0);
    throw runtime_error("FATAL ERROR");
  }
  myOSystem.logMessage("FrameBufferRoku::FrameBufferRoku SDL_Init()", 2);

  // We need a pixel format for palette value calculations
  // It's done this way (vs directly accessing a FBSurfaceRoku object)
  // since the structure may be needed before any FBSurface's have
  // been created
  myPixelFormat = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferRoku::~FrameBufferRoku()
{
	//	printf("FrameBufferRoku::~FrameBufferRoku %p ()\n", this);
#if 0
	SDL_FreeFormat(myPixelFormat);
  if(myRenderer)
  {
    SDL_DestroyRenderer(myRenderer);
    myRenderer = nullptr;
  }
  if(myWindow)
  {
    SDL_SetWindowFullscreen(myWindow, 0); // on some systems, a crash occurs
                                          // when destroying fullscreen window
    SDL_DestroyWindow(myWindow);
    myWindow = nullptr;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferRoku::queryHardware(vector<GUI::Size>& displays,
                                    VariantList& renderers)
{
	//	printf("FrameBufferRoku::queryHardware %p ()\n", this);
	R2D2::RoRect r = myScreen->GetBounds();
	displays.emplace_back(r.width, r.height);
  VarList::push_back(renderers, "Roku", "roku");
#if 0
  // First get the maximum windowed desktop resolution
  SDL_DisplayMode display;
  int maxDisplays = SDL_GetNumVideoDisplays();
  for(int i = 0; i < maxDisplays; ++i)
  {
    SDL_GetDesktopDisplayMode(i, &display);
    displays.emplace_back(display.w, display.h);
  }

  // For now, supported render types are hardcoded; eventually, SDL may
  // provide a method to query this
#if defined(BSPF_WINDOWS)
  VarList::push_back(renderers, "Direct3D", "direct3d");
#endif
  VarList::push_back(renderers, "OpenGL", "opengl");
  VarList::push_back(renderers, "OpenGLES2", "opengles2");
  VarList::push_back(renderers, "OpenGLES", "opengles");
  VarList::push_back(renderers, "Software", "software");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 FrameBufferRoku::getCurrentDisplayIndex()
{
	//	printf("FrameBufferRoku::getCurrentDisplayIndex %p ()\n", this);
	return 0;
	//  return SDL_GetWindowDisplayIndex(myWindow);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferRoku::setVideoMode(const string& title, const VideoMode& mode)
{
	//	printf("FrameBufferRoku::setVideoMode %p (%s)\n", this, title.c_str());
	return true;
	#if 0
  // If not initialized by this point, then immediately fail
  if(SDL_WasInit(SDL_INIT_VIDEO) == 0)
    return false;

  // Always recreate renderer (some systems need this)
  if(myRenderer)
  {
    // Always clear the (double-buffered) renderer surface
    SDL_RenderClear(myRenderer);
    SDL_RenderPresent(myRenderer);
    SDL_RenderClear(myRenderer);
    SDL_DestroyRenderer(myRenderer);
    myRenderer = nullptr;
  }

  Int32 displayIndex = mode.fsIndex;
  if(displayIndex == -1)
  {
    // windowed mode
    if (myWindow)
    {
      // Show it on same screen as the previous window
      displayIndex = SDL_GetWindowDisplayIndex(myWindow);
    }
    if(displayIndex < 0)
    {
      // fallback to the first screen
      displayIndex = 0;
    }
  }

  uInt32 pos = myOSystem.settings().getBool("center")
                 ? SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex)
                 : SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex);
  uInt32 flags = mode.fsIndex != -1 ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;

  // OSX seems to have issues with destroying the window, and wants to keep
  // the same handle
  // Problem is, doing so on other platforms results in flickering when
  // toggling fullscreen windowed mode
  // So we have a special case for OSX
#ifndef BSPF_MAC_OSX
  // Don't re-create the window if its size hasn't changed, as it's not
  // necessary, and causes flashing in fullscreen mode
  if(myWindow)
  {
    int w, h;
    SDL_GetWindowSize(myWindow, &w, &h);
    if(uInt32(w) != mode.screen.w || uInt32(h) != mode.screen.h)
    {
      SDL_DestroyWindow(myWindow);
      myWindow = nullptr;
    }
  }
  if(myWindow)
  {
    // Even though window size stayed the same, the title may have changed
    SDL_SetWindowTitle(myWindow, title.c_str());
  }
#else
  // OSX wants to *never* re-create the window
  // This sometimes results in the window being resized *after* it's displayed,
  // but at least the code works and doesn't crash
  if(myWindow)
  {
    SDL_SetWindowFullscreen(myWindow, flags);
    SDL_SetWindowSize(myWindow, mode.screen.w, mode.screen.h);
    SDL_SetWindowPosition(myWindow, pos, pos);
    SDL_SetWindowTitle(myWindow, title.c_str());
  }
#endif
  else
  {
    myWindow = SDL_CreateWindow(title.c_str(), pos, pos,
                                mode.screen.w, mode.screen.h, flags);
    if(myWindow == nullptr)
    {
      string msg = "ERROR: Unable to open SDL window: " + string(SDL_GetError());
      myOSystem.logMessage(msg, 0);
      return false;
    }
    setWindowIcon();
  }

  Uint32 renderFlags = SDL_RENDERER_ACCELERATED;
  if(myOSystem.settings().getBool("vsync"))  // V'synced blits option
    renderFlags |= SDL_RENDERER_PRESENTVSYNC;
  const string& video = myOSystem.settings().getString("video");  // Render hint
  if(video != "")
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, video.c_str());
  myRenderer = SDL_CreateRenderer(myWindow, -1, renderFlags);
  if(myRenderer == nullptr)
  {
    string msg = "ERROR: Unable to create SDL renderer: " + string(SDL_GetError());
    myOSystem.logMessage(msg, 0);
    return false;
  }
  SDL_RendererInfo renderinfo;
  if(SDL_GetRendererInfo(myRenderer, &renderinfo) >= 0)
    myOSystem.settings().setValue("video", renderinfo.name);

  return true;
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrameBufferRoku::about() const
{
	return "Roku";
#if 0
  ostringstream out;
  out << "Video system: " << SDL_GetCurrentVideoDriver() << endl;
  SDL_RendererInfo info;
  if(SDL_GetRendererInfo(myRenderer, &info) >= 0)
  {
    out << "  Renderer: " << info.name << endl;
    if(info.max_texture_width > 0 && info.max_texture_height > 0)
      out << "  Max texture: " << info.max_texture_width << "x"
                               << info.max_texture_height << endl;
    out << "  Flags: "
        << ((info.flags & SDL_RENDERER_PRESENTVSYNC) ? "+" : "-") << "vsync, "
        << ((info.flags & SDL_RENDERER_ACCELERATED) ? "+" : "-") << "accel"
        << endl;
  }
  return out.str();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferRoku::invalidate()
{
	//	printf("FrameBufferRoku::invalidate %p ()\n", this);
  myDirtyFlag = true;
  myScreen->Clear(R2D2::RoColor(0,0,0,255));
  #if 0
  SDL_RenderClear(myRenderer);
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferRoku::showCursor(bool show)
{
	//	printf("FrameBufferRoku::showCursor %p (%s)\n", this, show ? "true" : "false");
  #if 0
  SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferRoku::grabMouse(bool grab)
{
	//	printf("FrameBufferRoku::grabMouse %p (%s)\n", this, grab ? "true" : "false");
  #if 0
  SDL_SetRelativeMouseMode(grab ? SDL_TRUE : SDL_FALSE);
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferRoku::fullScreen() const
{
	//	printf("FrameBufferRoku::fullScreen %p ()\n", this);
	return true;
	  #if 0

#ifdef WINDOWED_SUPPORT
  return SDL_GetWindowFlags(myWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP;
#else
  return true;
#endif
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferRoku::getRGB(uInt32 pixel, uInt8* r, uInt8* g, uInt8* b) const
{
	//	printf("FrameBufferRoku::getRGB %p (%08X) = %d %d %d\n", this, pixel, pixel & 0xFF, (pixel >> 8) & 0xFF, (pixel >> 16) & 0xFF);
	*r = pixel & 0xFF;
	*g = (pixel >> 8) & 0xFF;
	*b = (pixel >> 16) & 0xFF;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBufferRoku::mapRGB(uInt8 r, uInt8 g, uInt8 b) const
{
	uInt32 ans = 0xFF << 24 | b << 16 | g << 8 | r << 0;
	// printf("FrameBufferRoku::mapRGB %p (%d, %d, %d) = %x\n", this, r, g, b, ans);
	return ans;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferRoku::postFrameUpdate()
{
	//	printf("FrameBufferRoku::postFrameUpdate %p ()\n", this);
  if(myDirtyFlag)
  {
    // Now show all changes made to the renderer
	  //    SDL_RenderPresent(myRenderer);
	myScreen->SwapBuffers();
    myDirtyFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferRoku::setWindowIcon()
{
	//	printf("FrameBufferRoku::setWindowIcon %p ()\n", this);
	#if 0
#ifndef BSPF_MAC_OSX        // Currently not needed for OSX
#include "stella_icon.hxx"  // The Stella icon

  SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(stella_icon, 32, 32, 32,
                         32 * 4, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF000000);
  SDL_SetWindowIcon(myWindow, surface);
  SDL_FreeSurface(surface);
#endif
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<FBSurface> FrameBufferRoku::createSurface(uInt32 w, uInt32 h,
                                          const uInt32* data) const
{
  printf("FrameBufferRoku::createSurface %p (%u, %u, %p)\n", this, w, h, data);
  return make_ptr<FBSurfaceRoku>(const_cast<FrameBufferRoku&>(*this), w, h, data);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferRoku::readPixels(uInt8* pixels, uInt32 pitch,
                                 const GUI::Rect& rect) const
{
	//	printf("FrameBufferRoku::readPixels %p (%u)\n", this, pitch);
	#if 0
  SDL_Rect r;
  r.x = rect.x();  r.y = rect.y();
  r.w = rect.width();  r.h = rect.height();

  SDL_RenderReadPixels(myRenderer, &r, 0, pixels, pitch);
  #endif
}
