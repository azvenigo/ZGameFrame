#include "ZScreenBuffer.h"
#include "ZGraphicSystem.h"
#include "ZDebug.h"
#include "ZRasterizer.h"
#include "ZTimer.h"
#include "ZStringHelpers.h"
#include "ZGUIStyle.h"
#include <iostream>

#ifdef _WIN64
#include <GdiPlus.h>
#include <windows.h>
#endif

//#ifdef _DEBUG
extern ZFont				gDebugFont;
extern int64_t				gnFramesPerSecond;
extern bool					gbGraphicSystemResetNeeded;
extern ZTimer				gTimer;
//#endif

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _WIN64
using namespace Gdiplus;
#endif
using namespace std;

ZScreenBuffer::ZScreenBuffer()
{
    mbRenderingEnabled = true;
    mbCurrentlyRendering = false;
#ifdef USE_D3D
	ZeroMemory((void*)&mLockedRect, sizeof(mLockedRect));
	ZeroMemory((void*)&mSurfaceDesc, sizeof(mSurfaceDesc));
#endif
}

ZScreenBuffer::~ZScreenBuffer()
{
	Shutdown();
}


bool ZScreenBuffer::Init(int64_t nWidth, int64_t nHeight, ZGraphicSystem* pGraphicSystem)
{
	mpGraphicSystem = pGraphicSystem;
	if (!ZBuffer::Init(nWidth, nHeight))
		return false;

	return true;	
}

bool ZScreenBuffer::Shutdown()
{
	return ZBuffer::Shutdown();
}

void ZScreenBuffer::EnableRendering(bool bEnable)
{
    if (bEnable)
    {
        mbVisibilityNeedsComputing = true;
        mbRenderingEnabled = true;
    }
    else
    {
        while (mbCurrentlyRendering)
        {
            mbRenderingEnabled = false; // set so that no more rendering after it's finished
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
        }

        mbRenderingEnabled = false;
    }
};

void ZScreenBuffer::BeginRender()
{
    if (!mbRenderingEnabled)
        return;

    mbCurrentlyRendering = true;
#ifdef _WIN64
    // Copy to our window surface
    mDC = BeginPaint(mpGraphicSystem->GetMainHWND(), &mPS);
#endif

}

void ZScreenBuffer::EndRender()
{
    mbCurrentlyRendering = false;
    if (!mbRenderingEnabled)
        return;
#ifdef _WIN64
    EndPaint(mpGraphicSystem->GetMainHWND(), &mPS);
#endif
}

void ZScreenBuffer::Render(tZBufferPtr pTexture, ZRect& rAreaToDrawTo)
{
    if (!mbRenderingEnabled)
        return;

    ZASSERT(mbCurrentlyRendering == true);

#ifdef _DEBUG
	string sTemp;
	ZRect rArea(2,2,66,32);
	Sprintf(sTemp, "FPS:%d\ntime:%d", gnFramesPerSecond, gTimer.GetElapsedTime());
	FillAlpha(rArea, 0x66000000);
    gpFontSystem->GetDefaultFont()->DrawTextParagraph(this, sTemp, ZRect(4,4,320,120), &gStyleGeneralText);
#endif

#ifdef USE_D3D 
	if (mpGraphicSystem->GetD3DDevice()->TestCooperativeLevel() == D3DERR_DEVICELOST)
	{
		gbGraphicSystemResetNeeded = true;
		return;
	}

	IDirect3DSurface9* pBackBuffer = NULL;
	HRESULT hr = mpGraphicSystem->GetD3DDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	if (hr != D3D_OK)
	{
		return;
	}

	D3DLOCKED_RECT lockedRect;
	hr = pBackBuffer->LockRect(&lockedRect, NULL, 0);
	if (hr == D3D_OK)
	{
		size_t nRowBytes = mSurfaceArea.Width()*sizeof(uint32_t);
		for (int y = 0; y < mSurfaceArea.Height(); y++)
		{
			uint32_t* pDstRow = ((uint32_t*) lockedRect.pBits + y * lockedRect.Pitch/sizeof(uint32_t));
			uint32_t* pSrcRow = mpPixels + y * mSurfaceArea.Width();
			memcpy((void*) pDstRow, pSrcRow, nRowBytes);
		}
	}
	else
	{
		CEASSERT(false);
	}

	pBackBuffer->UnlockRect();

	pBackBuffer->Release();

	// Render all dirty rects
	if (pClip)
	{
		RECT rPresent;
		rPresent.left = pClip->left;
		rPresent.top = pClip->top;
		rPresent.right = pClip->right;
		rPresent.bottom = pClip->bottom;

		hr = mpGraphicSystem->GetD3DDevice()->Present(&rPresent, &rPresent, NULL, NULL);
	}
	else
		hr = mpGraphicSystem->GetD3DDevice()->Present(NULL, NULL, NULL, NULL);

	if (hr == D3DERR_DEVICELOST)
	{
		gbGraphicSystemResetNeeded = true;
	}
#endif


#ifdef _WIN64

    ZRect rTexture(pTexture->GetArea());

    BITMAPINFO bmpInfo;
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biCompression = BI_RGB;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
    bmpInfo.bmiHeader.biWidth = (LONG)rTexture.Width();
    bmpInfo.bmiHeader.biHeight = (LONG)-rTexture.Height();


    int nRet = SetDIBitsToDevice(mDC,       // HDC
        (DWORD) rAreaToDrawTo.left,                 // Dest X
        (DWORD) rAreaToDrawTo.top,                  // Dest Y
        (DWORD) rTexture.Width(),            // Dest Width
        (DWORD) rTexture.Height(),           // Dest Height
        0,                                  // Src X
        0,                                  // Src Y
        0,                                  // Start Scanline
        (DWORD)rTexture.Height(),           // Num Scanlines
        (void*)pTexture->GetPixels(),       // * pixels
        &bmpInfo,                          // BMPINFO
        DIB_RGB_COLORS);                    // Usage

//	EndPaint(mpGraphicSystem->GetMainHWND(), &ps);
#endif
}

#define USE_LOCKING_SCREEN_RECTS

#ifdef _WIN64

int32_t ZScreenBuffer::RenderVisibleRects()
{
    if (!mbRenderingEnabled)
        return 0;

//    TIME_SECTION_START(RenderVisibleRects);

	BITMAPINFO bmpInfo;
	bmpInfo.bmiHeader.biBitCount = 32;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);


    tZBufferPtr pCurBuffer;

    int64_t nRenderedCount = 0;

	for (auto sr : mScreenRectList)
	{
        // Since the mScreenRectList happens to be sorted so that referenced textures are together, we should lock, render all from that texture, then unlock
        if (sr.mpSourceBuffer != pCurBuffer)
        {
            if (pCurBuffer)
                pCurBuffer->ClearRenderFlag(ZBuffer::kRenderFlag_ReadyToRender);

#ifdef USE_LOCKING_SCREEN_RECTS
            if (pCurBuffer)
                pCurBuffer->GetMutex().unlock();
#endif

            pCurBuffer = sr.mpSourceBuffer;

#ifdef USE_LOCKING_SCREEN_RECTS
            pCurBuffer->GetMutex().lock();
#endif
        }

        if (pCurBuffer->GetRenderFlag(ZBuffer::kRenderFlag_ReadyToRender))
        {
            nRenderedCount++;
            ZRect rTexture(sr.mpSourceBuffer->GetArea());

            bmpInfo.bmiHeader.biWidth = (LONG)rTexture.Width();
            bmpInfo.bmiHeader.biHeight = (LONG)-rTexture.Height();

            DWORD nStartScanline = (DWORD)sr.mSourcePt.y;
            DWORD nScanLines = (DWORD)sr.mrDest.Height();

            void* pBits = sr.mpSourceBuffer->GetPixels() + sr.mSourcePt.y * rTexture.Width();

            int nRet = SetDIBitsToDevice(mDC,       // HDC
                (DWORD)sr.mrDest.left,                 // Dest X
                (DWORD)sr.mrDest.top,                  // Dest Y
                (DWORD)sr.mrDest.Width(),            // Dest Width
                (DWORD)sr.mrDest.Height(),           // Dest Height
                (DWORD)sr.mSourcePt.x,               // Src X
                (DWORD)sr.mSourcePt.y,                // Src Y
                nStartScanline,                                  // Start Scanline
                nScanLines,           // Num Scanlines
                pBits,       // * pixels
                &bmpInfo,                          // BMPINFO
                DIB_RGB_COLORS);                    // Usage
        }
	}

    if (pCurBuffer)
        pCurBuffer->ClearRenderFlag(ZBuffer::kRenderFlag_ReadyToRender);

#ifdef USE_LOCKING_SCREEN_RECTS
    if (pCurBuffer)
        pCurBuffer->GetMutex().unlock();
#endif

//    if (nRenderedCount > 0)
//        ZDEBUG_OUT("ScreenBuffer Rendered:%d ", nRenderedCount);

//    TIME_SECTION_END(RenderVisibleRects);

	return (int32_t) nRenderedCount;
}

bool ZScreenBuffer::RenderBuffer(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst)
{
    if (!mbRenderingEnabled || !pSrc)
        return false;

    BITMAPINFO bmpInfo;
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biCompression = BI_RGB;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);


    pSrc->GetMutex().lock();

    ZRect rTexture(pSrc->GetArea());

    bmpInfo.bmiHeader.biWidth = (LONG)rTexture.Width();
    bmpInfo.bmiHeader.biHeight = (LONG)-rTexture.Height();

    DWORD nStartScanline = (DWORD)rSrc.top;
    DWORD nScanLines = (DWORD)rDst.Height();

    void* pBits = pSrc->GetPixels() + rSrc.top * rTexture.Width();

    int nRet = SetDIBitsToDevice(mDC,       // HDC
        (DWORD)rDst.left,                 // Dest X
        (DWORD)rDst.top,                  // Dest Y
        (DWORD)rDst.Width(),            // Dest Width
        (DWORD)rDst.Height(),           // Dest Height
        (DWORD)rSrc.left,               // Src X
        (DWORD)rSrc.top,                // Src Y
        nStartScanline,                                  // Start Scanline
        nScanLines,           // Num Scanlines
        pBits,       // * pixels
        &bmpInfo,                          // BMPINFO
        DIB_RGB_COLORS);                    // Usage

    pSrc->GetMutex().unlock();

    return true;
}



#endif // _WIN64

// #define DEBUG_VISIBILITY
#ifdef DEBUG_VISIBILITY
#define DebugVisibilityOutput OutputDebug
#else
#define DebugVisibilityOutput
#endif

bool ZScreenBuffer::AddScreenRectAndComputeVisibility(const ZScreenRect& screenRect)
{
    assert(screenRect.mpSourceBuffer);

    if (!mbRenderingEnabled)
        return false;

	// The requirement here is that the newly added rect is on top of all previous rects (i.e. painters alg)

	tScreenRectList oldList(std::move(mScreenRectList));		// move over the old list
	tScreenRectList newList;

	// for each screenrect in the list, 
	// 
	// 
	// 	   if there is NO overlap just push the rect on the new list;
	// 
	// 	   if the new window overlaps completely then the oldRect is completely occluded and doesn't need to be considered further
	// 
	//	   if there is overlap with the new screenRect
	// 	   Find the overlapping rect (intersection of two)
	// 	   Find the 8 possible areas that are not occluded
	// 
	// 	   
	//                              rDest
	//     lw      mw      rw     /
	//  ---^-------^-------^---- /
	//         |       |        |             
	//    TL   |   T   |   TR   > th         
	//         |       |        |              
	//  -------+-------+--------|             
	//         |       |        |              
	//     L   |Overlap|   R    > mh             
	//         |       |        |              
	//  -------+-------+--------|             
	//         |       |        |              
	//    BL   |   B   |   BR   > bh 
	//         |       |        |              
	// 
	// For each of those 8 if they have area (i.e. either height or width non zero)
	// 	    Compute the source rect from oldRect
	//      Compute the dest TL based on the oldRect
	//      Add a new ZScreenRect to new list with these attributes

	for (auto oldRect : oldList)
	{
		if (!oldRect.mrDest.Overlaps(screenRect.mrDest))	// no overlap?
		{
			// No overlap so just add old rect to new list
			newList.push_back(std::move(oldRect));		// TODO maybe use emplace_back with copy constructor?
			DebugVisibilityOutput("No overlap... adding old rect. old(%d,%d,%d,%d)  new(%d,%d,%d,%d)\n", oldRect.mrDest.left, oldRect.mrDest.top, oldRect.mrDest.right, oldRect.mrDest.bottom, screenRect.mrDest.left, screenRect.mrDest.top, screenRect.mrDest.right, screenRect.mrDest.bottom);
		}
		else if (screenRect.mrDest.left <= oldRect.mrDest.left &&
			screenRect.mrDest.top <= oldRect.mrDest.top &&
			screenRect.mrDest.right >= oldRect.mrDest.right &&
			screenRect.mrDest.bottom >= oldRect.mrDest.bottom)
		{
			// Complete overlap, so old rect can be removed entirely
			DebugVisibilityOutput("Complete overlap... ignoring old rect. old(%d,%d,%d,%d)  new(%d,%d,%d,%d)\n", oldRect.mrDest.left, oldRect.mrDest.top, oldRect.mrDest.right, oldRect.mrDest.bottom, screenRect.mrDest.left, screenRect.mrDest.top, screenRect.mrDest.right, screenRect.mrDest.bottom);
		}
		else
		{
			ZRect rOverlap(oldRect.mrDest);
			rOverlap.IntersectRect(screenRect.mrDest);

			ZRect rDest(oldRect.mrDest);

			int64_t lw = rOverlap.left - rDest.left;
			int64_t mw = rOverlap.Width();
			int64_t rw = rDest.right - rOverlap.right;

			int64_t th = rOverlap.top - rDest.top;
			int64_t mh = rOverlap.Height();
			int64_t bh = rDest.bottom - rOverlap.bottom;


			if (lw != 0)		// Consider left three?
			{
				if (th != 0)
				{
					// TL
					ZRect rTL(rDest.left, rDest.top, rDest.left + lw, rDest.top + th);
					ZPoint sourcePt(oldRect.mSourcePt);
					newList.emplace_back(ZScreenRect(oldRect.mpSourceBuffer, rTL, sourcePt));
					DebugVisibilityOutput("rTL:(%d,%d,%d,%d) sourcePt:(%d,%d)", rTL.left, rTL.top, rTL.right, rTL.bottom, sourcePt.x, sourcePt.y);

				}

				if (mh != 0)
				{
					// L
					ZRect rL(rDest.left, rOverlap.top, rDest.left + lw, rOverlap.bottom);
					ZPoint sourcePt(oldRect.mSourcePt.x, oldRect.mSourcePt.y+th);
					newList.emplace_back(ZScreenRect(oldRect.mpSourceBuffer, rL, sourcePt));
					DebugVisibilityOutput("rL:(%d,%d,%d,%d) sourcePt:(%d,%d)\n", rL.left, rL.top, rL.right, rL.bottom, sourcePt.x, sourcePt.y);
				}

				if (bh != 0)
				{
					// BL
					ZRect rBL(rDest.left, rOverlap.bottom, rOverlap.left, rDest.bottom);
					ZPoint sourcePt(oldRect.mSourcePt.x, oldRect.mSourcePt.y+th+mh);
					newList.emplace_back(ZScreenRect(oldRect.mpSourceBuffer, rBL, sourcePt));
					DebugVisibilityOutput("rBL:(%d,%d,%d,%d) sourcePt:(%d,%d)\n", rBL.left, rBL.top, rBL.right, rBL.bottom, sourcePt.x, sourcePt.y);
				}
			}

			if (mw != 0)			// consider top and bottom two?
			{
				if (th != 0)
				{
					// T
					ZRect rT(rOverlap.left, rDest.top, rOverlap.right, rDest.top + th);
					ZPoint sourcePt(oldRect.mSourcePt.x+lw, oldRect.mSourcePt.y);
					newList.emplace_back(ZScreenRect(oldRect.mpSourceBuffer, rT, sourcePt));
					DebugVisibilityOutput("rT:(%d,%d,%d,%d) sourcePt:(%d,%d)\n", rT.left, rT.top, rT.right, rT.bottom, sourcePt.x, sourcePt.y);
				}

				if (bh != 0)
				{
					// B
					ZRect rB(rOverlap.left, rOverlap.bottom, rOverlap.right, rDest.bottom);
					ZPoint sourcePt(oldRect.mSourcePt.x+lw, oldRect.mSourcePt.y+th+mh);
					newList.emplace_back(ZScreenRect(oldRect.mpSourceBuffer, rB, sourcePt));
					DebugVisibilityOutput("rB:(%d,%d,%d,%d) sourcePt:(%d,%d)\n", rB.left, rB.top, rB.right, rB.bottom, sourcePt.x, sourcePt.y);
				}
			}

			if (rw != 0)		// consider right three?
			{
				if (th != 0)
				{
					// TR
					ZRect rTR(rOverlap.right, rDest.top, rDest.right, rOverlap.top);
					ZPoint sourcePt(oldRect.mSourcePt.x+lw+mw, oldRect.mSourcePt.y);
					newList.emplace_back(ZScreenRect(oldRect.mpSourceBuffer, rTR, sourcePt));
					DebugVisibilityOutput("rTR:(%d,%d,%d,%d) sourcePt:(%d,%d)\n", rTR.left, rTR.top, rTR.right, rTR.bottom, sourcePt.x, sourcePt.y);
				}

				if (mh != 0)
				{
					// R
					ZRect rR(rOverlap.right, rOverlap.top, rDest.right, rOverlap.bottom);
					ZPoint sourcePt(oldRect.mSourcePt.x+lw+mw, oldRect.mSourcePt.y+th);
					newList.emplace_back(ZScreenRect(oldRect.mpSourceBuffer, rR, sourcePt));
					DebugVisibilityOutput("rR:(%d,%d,%d,%d) sourcePt:(%d,%d)\n", rR.left, rR.top, rR.right, rR.bottom, sourcePt.x, sourcePt.y);
				}

				if (bh != 0)
				{
					// BR
					ZRect rBR(rOverlap.right, rOverlap.bottom, rDest.right, rDest.bottom);
					ZPoint sourcePt(oldRect.mSourcePt.x+lw+mw, oldRect.mSourcePt.y+th+mh);
					newList.emplace_back(ZScreenRect(oldRect.mpSourceBuffer, rBR, sourcePt));
					DebugVisibilityOutput("rBR:(%d,%d,%d,%d) sourcePt:(%d,%d)\n", rBR.left, rBR.top, rBR.right, rBR.bottom, sourcePt.x, sourcePt.y);
				}
			}
		}
	}

	// Finally add the new top level screenRect 
	newList.push_back(std::move(screenRect));

    // Set the render flag for all buffers in the new list
    for (auto sr : newList)
        sr.mpSourceBuffer->SetRenderFlag(ZBuffer::kRenderFlag_ReadyToRender);

	mScreenRectList = newList;
	return true;
}
