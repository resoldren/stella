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

#ifndef FBSURFACE_ROKU_HXX
#define FBSURFACE_ROKU_HXX

#include "bspf.hxx"
#include "FrameBufferRoku.hxx"

typedef bool SDL_TextureAccess;

#include <roku/robitmap.h>
#include <condition_variable>
#include <mutex>
#include <thread>

/**
  An FBSurface suitable for the Roku Render2D API, making use of hardware
  acceleration behind the scenes.

  @author  Stephen Anthony
*/
class FBSurfaceRoku : public FBSurface
{
	SDL_TextureAccess SDL_TEXTUREACCESS_STREAMING = true;
	SDL_TextureAccess SDL_TEXTUREACCESS_STATIC = false;
  public:
    FBSurfaceRoku(FrameBufferRoku& buffer, uInt32 width, uInt32 height,
                  const uInt32* data);
    virtual ~FBSurfaceRoku();

    // With hardware surfaces, it's faster to just update the entire surface
    void setDirty() override { mySurfaceIsDirty = true; }

    uInt32 width() const override;
    uInt32 height() const override;

    const GUI::Rect& srcRect() const override;
    const GUI::Rect& dstRect() const override;
    void setSrcPos(uInt32 x, uInt32 y) override;
    void setSrcSize(uInt32 w, uInt32 h) override;
    void setDstPos(uInt32 x, uInt32 y) override;
    void setDstSize(uInt32 w, uInt32 h) override;
    void setVisible(bool visible) override;

    void translateCoords(Int32& x, Int32& y) const override;
    bool render() override;
    void invalidate() override;
    void free() override;
    void reload() override;
    void resize(uInt32 width, uInt32 height) override;

  protected:
    void applyAttributes(bool immediate) override;

  private:
    void createSurface(uInt32 width, uInt32 height, const uInt32* data);

    // Following constructors and assignment operators not supported
    FBSurfaceRoku() = delete;
    FBSurfaceRoku(const FBSurfaceRoku&) = delete;
    FBSurfaceRoku(FBSurfaceRoku&&) = delete;
    FBSurfaceRoku& operator=(const FBSurfaceRoku&) = delete;
    FBSurfaceRoku& operator=(FBSurfaceRoku&&) = delete;

  private:
    FrameBufferRoku& myFB;

    std::mutex myBitmapMutex;
    std::condition_variable myBitmapCondVar;
    int myCurrentDrawBitmap;
    int myNextDrawBitmap;
    int myCurrentFillingBitmap;
    R2D2::RoBitmap* myBitmaps[2];
    uInt8* myBitmapPtrs[2];
    uInt32* myMemory;
	uInt32 myBitmapWidth;
	uInt32 myBitmapHeight;
//    SDL_Surface* mySurface;
//    SDL_Texture* myTexture;

//    bool stopping;
//    std::thread myDrawThread;
//    void run();

    bool mySurfaceIsDirty;
    bool myIsVisible;

	SDL_TextureAccess myTexAccess;  // Is pixel data constant or can it change?
    bool myInterpolate;   // Scaling is smoothed or blocky
    bool myBlendEnabled;  // Blending is enabled
    uInt8 myBlendAlpha;   // Alpha to use in blending mode

    uInt32* myStaticData; // The data to use when the buffer contents are static
    uInt32 myStaticPitch; // The number of bytes in a row of static data

    GUI::Rect mySrcGUIR, myDstGUIR;
};

#endif
