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

#include "FBSurfaceRoku.hxx"

#include <roku/robitmap.h>
#if defined(TIME_RENDERMEMCOPY) || defined(TIME_FULLRENDER)
#include <chrono>
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceRoku::FBSurfaceRoku(FrameBufferRoku& buffer,
                             uInt32 width, uInt32 height, const uInt32* data)
  : myFB(buffer),
    myCurrentDrawBitmap(-1),
    myNextDrawBitmap(-1),
    myCurrentFillingBitmap(-1),
    myBitmaps{nullptr, nullptr},
    myMemory(nullptr),
	//    mySurface(nullptr),
	//    myTexture(nullptr),
//    stopping(false),
    mySurfaceIsDirty(true),
    myIsVisible(true),
    myTexAccess(SDL_TEXTUREACCESS_STREAMING),
    myInterpolate(false),
    myBlendEnabled(false),
    myBlendAlpha(255),
    myStaticData(nullptr)
{
  printf("FBSurfaceRoku::FBSurfaceRoku %p (buf, %u, %u, %p)\n", this, width, height, data);
  createSurface(width, height, data);
//  printf("RUN: Starting thread for %p\n", this);
//  myDrawThread = std::thread([this](){ run(); });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceRoku::~FBSurfaceRoku()
{
//    stopping = true;
    myBitmapCondVar.notify_one();
//    if (myDrawThread.joinable()) {
//        myDrawThread.join();
//    }
	//	printf("FBSurfaceRoku::~FBSurfaceRoku %p ()\n", this);
    myFB.callGraphicsMethod([this](R2D2::RoGraphics* graphics) {
        if (myBitmaps[0]) {
            myBitmaps[0]->Unlock();
            delete myBitmaps[0];
            myBitmaps[0] = nullptr;
            myBitmaps[1]->Unlock();
            delete myBitmaps[1];
            myBitmaps[1] = nullptr;
            delete[] myMemory;
            myMemory = nullptr;
        }
    }, true);
#if 0
  if(mySurface)
    SDL_FreeSurface(mySurface);
#endif
  
  free();

  if(myStaticData)
  {
    delete[] myStaticData;
    myStaticData = nullptr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FBSurfaceRoku::width() const
{
	//	printf("FBSurfaceRoku::width %p ()\n", this);
	return myBitmapWidth;
	//  return mySurface->w;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FBSurfaceRoku::height() const
{
	//	printf("FBSurfaceRoku::height %p ()\n", this);
	return myBitmapHeight;
	//  return mySurface->h;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const GUI::Rect& FBSurfaceRoku::srcRect() const
{
	//	printf("FBSurfaceRoku::srcRect %p ()\n", this);
  return mySrcGUIR;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const GUI::Rect& FBSurfaceRoku::dstRect() const
{
	//	printf("FBSurfaceRoku::dstRect %p ()\n", this);
  return myDstGUIR;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceRoku::setSrcPos(uInt32 x, uInt32 y)
{
	//	printf("FBSurfaceRoku::setSrcPos %p (%u, %u)\n", this, x, y);
	//  mySrcR.x = x;  mySrcR.y = y;
  mySrcGUIR.moveTo(x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceRoku::setSrcSize(uInt32 w, uInt32 h)
{
	//	printf("FBSurfaceRoku::setSrcSize %p (%u, %u)\n", this, w, h);
	//  mySrcR.w = w;  mySrcR.h = h;
  mySrcGUIR.setWidth(w);  mySrcGUIR.setHeight(h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceRoku::setDstPos(uInt32 x, uInt32 y)
{

	//  myDstR.x = x;  myDstR.y = y;
  myDstGUIR.moveTo(x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceRoku::setDstSize(uInt32 w, uInt32 h)
{
	//	printf("FBSurfaceRoku::setDstSize %p (%u, %u)\n", this, w, h);
	//  myDstR.w = w;  myDstR.h = h;
  myDstGUIR.setWidth(w);  myDstGUIR.setHeight(h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceRoku::setVisible(bool visible)
{
	//	printf("FBSurfaceRoku::setVisible %p (%s)\n", this, visible ? "true" : "false");
  myIsVisible = visible;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceRoku::translateCoords(Int32& x, Int32& y) const
{
	//	printf("FBSurfaceRoku::translateCoords %p (%d, %d)\n", this, x, y);
	x -= myDstGUIR.x();
	y -= myDstGUIR.y();
}

//void FBSurfaceRoku::run()
//{
//    printf("RUN: starting for %p\n", this);
//    while (!stopping) {
//        {
//            std::unique_lock<std::mutex> lock(myBitmapMutex);
//            printf("RUN: about to wait in %p\n", this);
//            myBitmapCondVar.wait(lock, [this](){return stopping || myNextDrawBitmap >= 0;});
//            if (!stopping) {
//                myCurrentDrawBitmap = myNextDrawBitmap;
//                myNextDrawBitmap = -1;
//            }
//            printf("RUN: done waiting in %p\n", this);
//        }
//        if (myCurrentDrawBitmap >= 0) {
//            myFB.callScreenMethod([this](R2D2::RoScreen* screen) {
//                myBitmaps[myCurrentDrawBitmap]->Unlock();
//#ifndef SKIP_DRAWOBJECT
//                screen->DrawObject(
//                      R2D2::RoRect(myDstGUIR.x(), myDstGUIR.y(), myDstGUIR.width(), myDstGUIR.height()),
//                      *myBitmaps[myCurrentDrawBitmap],
//                      R2D2::RoRect(mySrcGUIR.x(), mySrcGUIR.y(), mySrcGUIR.width(), mySrcGUIR.height()));
//#endif
//                myBitmapPtrs[myCurrentDrawBitmap] = reinterpret_cast<uInt8*>(myBitmaps[myCurrentDrawBitmap]->Lock(true));
//                std::lock_guard<std::mutex> lock(myBitmapMutex);
//                myCurrentDrawBitmap = -1;
//            }, false);
//        }
//    }
//}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FBSurfaceRoku::render()
{
  if(mySurfaceIsDirty && myIsVisible)
  {
      // Pick which Bitmap to fill
      {
        std::lock_guard<std::mutex> lock(myBitmapMutex);
        if (myCurrentDrawBitmap >= 0) {
            // if one is being drawn, the other one should be filled
            myCurrentFillingBitmap = 1 - myCurrentDrawBitmap;
            // if the bitmap about to be filled was previous marked as next to draw, forget that
            if (myNextDrawBitmap == myCurrentFillingBitmap) {
                myNextDrawBitmap = -1;
            }
        } else if (myNextDrawBitmap >= 0) {
            // if a bitmap had been marked as next to draw, fill the other one, in case drawing starts during the fill
            myCurrentFillingBitmap = 1 - myNextDrawBitmap;
        } else {
            // Everything is free. Just pick one randomly
            myCurrentFillingBitmap = 0;
        }
      }

#if 0
    printf("FBSurfaceRoku::render %p ()\n", this);
    cerr << "src: x=" << mySrcGUIR.x() << ", y=" << mySrcGUIR.y() << ", w=" << mySrcGUIR.width() << ", h=" << mySrcGUIR.height() << endl;
    cerr << "dst: x=" << myDstGUIR.x() << ", y=" << myDstGUIR.y() << ", w=" << myDstGUIR.width() << ", h=" << myDstGUIR.height() << endl;
#endif
#ifdef TIME_FULLRENDER
    auto renderTime1 = std::chrono::high_resolution_clock::now();
#endif
    readPixels(myBitmapPtrs[myCurrentFillingBitmap], myBitmapWidth, GUI::Rect(myBitmapWidth, myBitmapHeight));
    {
      std::lock_guard<std::mutex> lock(myBitmapMutex);
      myNextDrawBitmap = myCurrentFillingBitmap;
      myCurrentFillingBitmap = -1;
    }
    myBitmapCondVar.notify_one();

#if 1
#ifdef TIME_FULLRENDER
    auto renderTime2 = std::chrono::high_resolution_clock::now();
#endif
    // SDL_UpdateTexture(myTexture, &mySrcR, mySurface->pixels, mySurface->pitch);
//    SDL_RenderCopy(myFB.myRenderer, myTexture, &mySrcR, &myDstR);
#ifndef SKIP_DRAWOBJECT
//    myFB.callScreenMethod([this](R2D2::RoScreen* screen) {
//        {
//            std::unique_lock<std::mutex> lock(myBitmapMutex);
//            myCurrentDrawBitmap = myNextDrawBitmap;
//            myNextDrawBitmap = -1;
//        }
//        screen->DrawObject(
//            R2D2::RoRect(myDstGUIR.x(), myDstGUIR.y(), myDstGUIR.width(), myDstGUIR.height()),
//            *myBitmaps[0],
//            R2D2::RoRect(mySrcGUIR.x(), mySrcGUIR.y(), mySrcGUIR.width(), mySrcGUIR.height()));
//    }, false);
        myFB.callScreenMethod([this](R2D2::RoScreen* screen) {
            {
                std::lock_guard<std::mutex> lock(myBitmapMutex);
                myCurrentDrawBitmap = myNextDrawBitmap;
                myNextDrawBitmap = -1;
            }
            if (myCurrentDrawBitmap >= 0) {
#ifdef TIME_FULLRENDER
    auto renderTime1 = std::chrono::high_resolution_clock::now();
#endif
                myBitmaps[myCurrentDrawBitmap]->Unlock();
#ifdef TIME_FULLRENDER
    auto renderTime2 = std::chrono::high_resolution_clock::now();
#endif
#ifndef SKIP_DRAWOBJECT
                screen->DrawObject(
                      R2D2::RoRect(myDstGUIR.x(), myDstGUIR.y(), myDstGUIR.width(), myDstGUIR.height()),
                      *myBitmaps[myCurrentDrawBitmap],
                      R2D2::RoRect(mySrcGUIR.x(), mySrcGUIR.y(), mySrcGUIR.width(), mySrcGUIR.height()));
#endif
#ifdef TIME_FULLRENDER
    auto renderTime3 = std::chrono::high_resolution_clock::now();
#endif
                myBitmapPtrs[myCurrentDrawBitmap] = reinterpret_cast<uInt8*>(myBitmaps[myCurrentDrawBitmap]->Lock(true));
#ifdef TIME_FULLRENDER
    auto renderTime4 = std::chrono::high_resolution_clock::now();
    auto total = std::chrono::duration_cast<std::chrono::milliseconds>(renderTime4 - renderTime1).count();
    printf("Render: total: %lldms (%lldms/%lldms/%lldms)\n",
           total,
           std::chrono::duration_cast<std::chrono::milliseconds>(renderTime2 - renderTime1).count(),
           std::chrono::duration_cast<std::chrono::milliseconds>(renderTime3 - renderTime2).count(),
           std::chrono::duration_cast<std::chrono::milliseconds>(renderTime4 - renderTime3).count());
#endif
                std::lock_guard<std::mutex> lock(myBitmapMutex);
                myCurrentDrawBitmap = -1;
            }
        }, false);
#endif
#ifdef TIME_FULLRENDER
    auto renderTime3 = std::chrono::high_resolution_clock::now();
    auto total = std::chrono::duration_cast<std::chrono::milliseconds>(renderTime3 - renderTime1).count();
    if (total > 0) {
        printf("outer Render: total: %lldms (%lldms/%lldms)\n",
               total,
               std::chrono::duration_cast<std::chrono::milliseconds>(renderTime2 - renderTime1).count(),
               std::chrono::duration_cast<std::chrono::milliseconds>(renderTime3 - renderTime2).count());
    }
#endif
#endif

    mySurfaceIsDirty = false;

    // Let postFrameUpdate() know that a change has been made
    return myFB.myDirtyFlag = true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceRoku::invalidate()
{
	//	printf("FBSurfaceRoku::invalidate %p ()\n", this);
	// fillRect(0, 0, width(), height(), myFB.mapRGB(0, 0, 0));
	//  SDL_FillRect(mySurface, nullptr, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceRoku::free()
{
	//	printf("FBSurfaceRoku::free %p ()\n", this);
#if WRONG
	if (myBitmap) {
		myBitmap->Unlock();
		delete myBitmap;
		myBitmap = nullptr;
	}
#endif
#if 0
  if(myTexture)
  {
    SDL_DestroyTexture(myTexture);
    myTexture = nullptr;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceRoku::reload()
{
	printf("FBSurfaceRoku::reload %p ()\n", this);
	printf("  TODO\n");
#if 0
  // Re-create texture; the underlying SDL_Surface is fine as-is
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, myInterpolate ? "1" : "0");
  myTexture = SDL_CreateTexture(myFB.myRenderer, myFB.myPixelFormat->format,
      myTexAccess, mySurface->w, mySurface->h);
#endif
  
#if 1
  // If the data is static, we only upload it once
  if(myTexAccess == SDL_TEXTUREACCESS_STATIC) {
	  // int size = width() * height() * 4;
	  //	  printf(" myPixels: %p  myStaticData: %p size: %d\n", myPixels, myStaticData, size);
	  //	  printf("    RELOAD: COPYING 100 pixels\n");
	  // memset(myPixels, 0xFF, 4);
	  	   memcpy(myPixels, myStaticData, this->width() * this->height() * 4);
  }
#endif
  
  for (uInt32 i = 0; i < width(); ++i) {
	  for (uInt32 j = 0; j < height(); ++j) {
		  myPixels[j * width() + i] = 0xFF0000FF | (i << 8) | (j << 16);
		}
	}
#if 0
    SDL_UpdateTexture(myTexture, nullptr, myStaticData, myStaticPitch);
#endif

#if 0
  // Blending enabled?
  if(myBlendEnabled)
  {
    SDL_SetTextureBlendMode(myTexture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(myTexture, myBlendAlpha);
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceRoku::resize(uInt32 width, uInt32 height)
{
	printf("FBSurfaceRoku::resize %p (%u, %u)\n", this, width, height);
  // We will only resize when necessary, and not using static textures
    if((myTexAccess == SDL_TEXTUREACCESS_STATIC) || (myBitmaps[0]/*mySurface*/ &&
	   width <= this->width() && height <= this->height())) {
		printf("******  REUSING BUFFER *****\n");
    return;  // don't need to resize at all
	}
	
#if 0
  if(mySurface)
    SDL_FreeSurface(mySurface);
#endif
  free();

  createSurface(width, height, nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceRoku::createSurface(uInt32 width, uInt32 height,
                                  const uInt32* data)
{
    //	printf("FBSurfaceRoku::crateSurface %p (%u, %u, %p)\n", this, width, height, data);
    myFB.callGraphicsMethod([this, width, height](R2D2::RoGraphics* graphics) {
        if (myBitmaps[0]) {
            myBitmaps[0]->Unlock();
            delete myBitmaps[0];
            myBitmaps[0] = nullptr;
            myBitmaps[1]->Unlock();
            delete myBitmaps[1];
            myBitmaps[1] = nullptr;
            delete[] myMemory;
            myMemory = nullptr;
        }
        myBitmaps[0] = graphics->CreateBitmap(width, height, R2D2::RGBA8888);
        myBitmaps[1] = graphics->CreateBitmap(width, height, R2D2::RGBA8888);
        myBitmapWidth = static_cast<uInt32>(myBitmaps[0]->GetBounds().width);
        myBitmapHeight = static_cast<uInt32>(myBitmaps[0]->GetBounds().height);
        myBitmapPtrs[0] = reinterpret_cast<uInt8*>(myBitmaps[0]->Lock(true));
        myBitmapPtrs[1] = reinterpret_cast<uInt8*>(myBitmaps[1]->Lock(true));
    }, true);
    myMemory = new uInt32[myBitmapWidth * myBitmapHeight];
#if 0
  // Create a surface in the same format as the parent GL class
  const SDL_PixelFormat* pf = myFB.myPixelFormat;

  mySurface = SDL_CreateRGBSurface(0, width, height,
      pf->BitsPerPixel, pf->Rmask, pf->Gmask, pf->Bmask, pf->Amask);
#endif

  // We start out with the src and dst rectangles containing the same
  // dimensions, indicating no scaling or re-positioning
  mySrcGUIR.moveTo(0, 0);
  myDstGUIR.moveTo(0, 0);
  mySrcGUIR.setWidth(width);  mySrcGUIR.setHeight(height);
  myDstGUIR.setWidth(width);  myDstGUIR.setHeight(height);

  ////////////////////////////////////////////////////
  // These *must* be set for the parent class
  myPixels = myMemory;
  myPitch = this->width();
  //  * mySurface->pitch / pf->BytesPerPixel;
  ////////////////////////////////////////////////////

  if(data)
  {
    myTexAccess = SDL_TEXTUREACCESS_STATIC;
	//    myStaticPitch = this->width() * 4; // mySurface->w * 4;  // we need pitch in 'bytes'
    myStaticData = new uInt32[this->width()*this->height()]; // mySurface->w * mySurface->h;
	//	printf("copying %d bytes\n", this->width() * this->height() * 4);
    memcpy(myStaticData, data, this->width() * this->height()/*mySurface->w * mySurface->h*/ * 4);
	//	printf("MyStaticData: %p\n", myStaticData);
  }

  applyAttributes(false);

  // To generate texture
  reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceRoku::applyAttributes(bool immediate)
{
	//	printf("FBSurfaceRoku::applyAttributes %p (%s)\n", this, immediate ? "true" : "false");
  myInterpolate  = myAttributes.smoothing;
  myBlendEnabled = myAttributes.blending;
  myBlendAlpha   = uInt8(myAttributes.blendalpha * 2.55);

  if(immediate)
  {
    // Re-create the texture with the new settings
    free();
    reload();
  }
}