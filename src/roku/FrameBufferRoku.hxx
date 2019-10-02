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

#ifndef FRAMEBUFFER_ROKU_HXX
#define FRAMEBUFFER_ROKU_HXX

class OSystem;
class FBSurfaceRoku;

#include "bspf.hxx"
#include "FrameBuffer.hxx"

#include <roku/robitmap.h>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

class FrameBufferRoku : public FrameBuffer
{
  friend class FBSurfaceRoku;

  public:
    /**
      Creates a new Roku framebuffer
    */
    FrameBufferRoku(OSystem& osystem);
    virtual ~FrameBufferRoku();

    //////////////////////////////////////////////////////////////////////
    // The following are derived from public methods in FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      Toggles the use of grabmouse (only has effect in emulation mode).
      The method changes the 'grabmouse' setting and saves it.
    */
    void toggleGrabMouse();

    /**
      Shows or hides the cursor based on the given boolean value.
    */
    void showCursor(bool show) override;

    /**
      Answers if the display is currently in fullscreen mode.
    */
    bool fullScreen() const override;

    /**
      This method is called to retrieve the R/G/B data from the given pixel.

      @param pixel  The pixel containing R/G/B data
      @param r      The red component of the color
      @param g      The green component of the color
      @param b      The blue component of the color
    */
    void getRGB(uInt32 pixel, uInt8* r, uInt8* g, uInt8* b) const override;

    /**
      This method is called to map a given R/G/B triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    uInt32 mapRGB(uInt8 r, uInt8 g, uInt8 b) const override;

    /**
      This method is called to get a copy of the specified ARGB data from the
      viewable FrameBuffer area.  Note that this isn't the same as any
      internal surfaces that may be in use; it should return the actual data
      as it is currently seen onscreen.

      @param buffer  A copy of the pixel data in ARGB8888 format
      @param pitch   The pitch (in bytes) for the pixel data
      @param rect    The bounding rectangle for the buffer
    */
    void readPixels(uInt8* buffer, uInt32 pitch, const GUI::Rect& rect) const override;

  protected:
    //////////////////////////////////////////////////////////////////////
    // The following are derived from protected methods in FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This method is called to query and initialize the video hardware
      for desktop and fullscreen resolution information.
    */
    void queryHardware(vector<GUI::Size>& displays, VariantList& renderers) override;

    /**
      This method is called to query the video hardware for the index
      of the display the current window is displayed on

      @return  the current display index or a negative value if no
               window is displayed
    */
    Int32 getCurrentDisplayIndex() override;

    /**
      This method is called to change to the given video mode.

      @param title The title for the created window
      @param mode  The video mode to use

      @return  False on any errors, else true
    */
    bool setVideoMode(const string& title, const VideoMode& mode) override;

    /**
      This method is called to invalidate the contents of the entire
      framebuffer (ie, mark the current content as invalid, and erase it on
      the next drawing pass).
    */
    void invalidate() override;

    /**
      This method is called to create a surface with the given attributes.

      @param w     The requested width of the new surface.
      @param h     The requested height of the new surface.
      @param data  If non-null, use the given data values as a static surface
    */
    unique_ptr<FBSurface> createSurface(uInt32 w, uInt32 h, const uInt32* data)
        const override;

    /**
      Grabs or ungrabs the mouse based on the given boolean value.
    */
    void grabMouse(bool grab) override;

    /**
      Set the icon for the main window.
    */
    void setWindowIcon() override;

    /**
      This method is called to provide information about the FrameBuffer.
    */
    string about() const override;

    /**
      This method is called after any drawing is done (per-frame).
    */
    void postFrameUpdate() override;

    using RoScreenCallback = std::function<void(R2D2::RoScreen*)>;
    using RoGraphicsCallback = std::function<void(R2D2::RoGraphics*)>;

    void callScreenMethod(RoScreenCallback fn, bool wait);

    void callGraphicsMethod(RoGraphicsCallback fn, bool wait);

  private:
    bool myStopping;

    std::thread myRenderThread;

    // Used by mapRGB (when palettes are created)
    // SDL_PixelFormat* myPixelFormat;

    // Indicates that the renderer has been modified, and should be redrawn
    bool myDirtyFlag;

    class QueueItem {
    public:
        QueueItem(RoScreenCallback callback, bool waitFor);
        QueueItem(RoGraphicsCallback callback, bool waitFor);
        void operator()(R2D2::RoGraphics*, R2D2::RoScreen*);
        void wait();
    private:
        QueueItem(const QueueItem&) = delete;
        QueueItem& operator=(const QueueItem&) = delete;
        std::mutex mutex;
        std::condition_variable cv;
        bool done;

        RoScreenCallback cbScreen;
        RoGraphicsCallback cbGraphics;
        bool waitFor;

    };

    void run(int width, int height);
    std::mutex myQueueMutex;
    std::condition_variable myQueueCondVar;
    std::deque<QueueItem> myQueue;

  private:
    // Following constructors and assignment operators not supported
    FrameBufferRoku() = delete;
    FrameBufferRoku(const FrameBufferRoku&) = delete;
    FrameBufferRoku(FrameBufferRoku&&) = delete;
    FrameBufferRoku& operator=(const FrameBufferRoku&) = delete;
    FrameBufferRoku& operator=(FrameBufferRoku&&) = delete;

};

#endif

