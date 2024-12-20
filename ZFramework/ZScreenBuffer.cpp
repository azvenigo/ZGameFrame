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


    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = nWidth;
    desc.Height = nHeight;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;  // Matches ARGB layout
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;

    HRESULT hr = mpGraphicSystem->mD3DDevice->CreateTexture2D(&desc, nullptr, &mDynamicTexture);
    if (FAILED(hr)) 
    {
        throw std::runtime_error("Failed to create staging texture");
    }

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


#define USE_LOCKING_SCREEN_RECTS

#ifdef _WIN64


bool ZScreenBuffer::PaintToSystem()
{
    if (!mbRenderingEnabled)
        return false;

    uint64_t start = gTimer.GetUSSinceEpoch();

    void* pBits = mpPixels;

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = mpGraphicSystem->mD3DContext->Map(mDynamicTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to map staging texture");
    }

    // Assuming your ARGB buffer is stored in `argbBuffer`:
    UINT height = mSurfaceArea.Height();
    UINT width = mSurfaceArea.Width();


/*    for (UINT row = 0; row < height; ++row)
    {
        memcpy(static_cast<BYTE*>(mapped.pData) + row * mapped.RowPitch,
            (BYTE*)pBits + row * width * 4, // Assuming each pixel is 4 bytes
            width * 4);
    }*/
    assert(width * 4 == mapped.RowPitch);
    memcpy(mapped.pData, pBits, (width * height * 4));


    mpGraphicSystem->mD3DContext->Unmap(mDynamicTexture, 0);

    // Copy staging texture to back buffer
    mpGraphicSystem->mD3DContext->CopyResource(mpGraphicSystem->mBackBuffer, mDynamicTexture);

    mpGraphicSystem->mSwapChain->Present(1, 0); // VSync: 1

/*    uint64_t end = gTimer.GetUSSinceEpoch();


    static uint64_t delay = 0;

    static uint64_t frames = 0;
    static uint64_t totalTime = 0;
    delay++;

    if (delay > 100)
    {
        frames++;
        totalTime += (end - start);

        cout << "frames:" << frames << " avg paint time: " << (totalTime) / frames << "\n";
    }*/


    return true;
}
/*
* 
// Converts an sRGB value to linear space
float SRGBToLinear(float c) {
    return (c <= 0.04045f) ? (c / 12.92f) : powf((c + 0.055f) / 1.055f, 2.4f);
}

// Maps ARGB (0-255 per channel) to HDR float16 range (e.g., 0-1000)
float MapToHDRRange(float linearColor, float maxHDRValue) {
    return linearColor * maxHDRValue; // Scale to desired HDR range
}

bool ZScreenBuffer::PaintToSystem()
{
    if (!mbRenderingEnabled)
        return false;

    void* pBits = mpPixels;

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = mpGraphicSystem->mD3DContext->Map(mStagingTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to map staging texture");
    }

    UINT height = mSurfaceArea.Height();
    UINT width = mSurfaceArea.Width();
    float maxHDRValue = 1000.0f; // Example HDR range max luminance

    for (UINT row = 0; row < height; ++row) {
        auto* dest = reinterpret_cast<DirectX::PackedVector::XMHALF4*>(
            (BYTE*) (mapped.pData) + row * mapped.RowPitch);
        const uint32_t* src = reinterpret_cast<const uint32_t*>(
            (BYTE*)(mpPixels) + row * width * 4);

        for (UINT col = 0; col < width; ++col) {
            // Decompose ARGB to individual channels
            uint8_t A = (src[col] >> 24) & 0xFF;
            uint8_t R = (src[col] >> 16) & 0xFF;
            uint8_t G = (src[col] >> 8) & 0xFF;
            uint8_t B = src[col] & 0xFF;

            // Convert to normalized floats
            float rLinear = MapToHDRRange(SRGBToLinear(R / 255.0f), maxHDRValue);
            float gLinear = MapToHDRRange(SRGBToLinear(G / 255.0f), maxHDRValue);
            float bLinear = MapToHDRRange(SRGBToLinear(B / 255.0f), maxHDRValue);
            float aLinear = A / 255.0f; // Alpha typically remains normalized

            // Write HDR values to destination
            dest[col] = DirectX::PackedVector::XMHALF4(rLinear, gLinear, bLinear, aLinear);
        }
    }

    mpGraphicSystem->mD3DContext->Unmap(mStagingTexture, 0);

    // Copy staging texture to back buffer
    mpGraphicSystem->mD3DContext->CopyResource(mpGraphicSystem->mBackBuffer, mStagingTexture);

    mpGraphicSystem->mSwapChain->Present(1, 0); // VSync: 1
    return true;
}
*/


/*
bool ZScreenBuffer::PaintToSystem()
{
    if (!mbRenderingEnabled)
        return false;

    BITMAPINFO bmpInfo;
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biCompression = BI_RGB;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);

    mSurfaceArea;

    bmpInfo.bmiHeader.biWidth = (LONG)mSurfaceArea.Width();
    bmpInfo.bmiHeader.biHeight = (LONG)-mSurfaceArea.Height();

    DWORD nStartScanline = (DWORD)0;
    DWORD nScanLines = (DWORD)mSurfaceArea.Height();

    void* pBits = mpPixels;

    int nRet = SetDIBitsToDevice(mDC,       // HDC
        (DWORD)0,                 // Dest X
        (DWORD)0,                  // Dest Y
        (DWORD)mSurfaceArea.Width(),            // Dest Width
        (DWORD)mSurfaceArea.Height(),           // Dest Height
        (DWORD)0,               // Src X
        (DWORD)0,                // Src Y
        nStartScanline,                                  // Start Scanline
        nScanLines,           // Num Scanlines
        pBits,       // * pixels
        &bmpInfo,                          // BMPINFO
        DIB_RGB_COLORS);                    // Usage

    return true;
}
*/
int32_t ZScreenBuffer::RenderVisibleRects()
{
    if (!mbRenderingEnabled)
        return 0;

    tZBufferPtr pCurBuffer;

    int64_t nRenderedCount = 0;

    const std::lock_guard<std::mutex> surfaceLock(mScreenRectListMutex);
    for (auto& sr : mScreenRectList)
	{
        // Since the mScreenRectList happens to be sorted so that referenced textures are together, we should lock, render all from that texture, then unlock
        if (sr.mpSourceBuffer != pCurBuffer)
        {
            if (pCurBuffer)
                pCurBuffer->mRenderState = ZBuffer::kFreeToModify;

#ifdef USE_LOCKING_SCREEN_RECTS
            if (pCurBuffer)
                pCurBuffer->GetMutex().unlock();
#endif

            pCurBuffer = sr.mpSourceBuffer;

#ifdef USE_LOCKING_SCREEN_RECTS
            pCurBuffer->GetMutex().lock();
#endif
        }

        if (pCurBuffer->mRenderState == ZBuffer::kReadyToRender || pCurBuffer->mRenderState == ZBuffer::kFreeToModify)
        {
            nRenderedCount++;
            ZRect rTexture(sr.mpSourceBuffer->GetArea());

            ZRect rSource(sr.mSourcePt.x, sr.mSourcePt.y, sr.mSourcePt.x + sr.mrDest.Width(), sr.mSourcePt.y + sr.mrDest.Height());
            Blt(sr.mpSourceBuffer.get(), rSource, sr.mrDest);
        }
	}

    if (pCurBuffer)
        pCurBuffer->mRenderState = ZBuffer::kFreeToModify;

#ifdef USE_LOCKING_SCREEN_RECTS
    if (pCurBuffer)
        pCurBuffer->GetMutex().unlock();
#endif

//    if (nRenderedCount > 0)
//        ZDEBUG_OUT("ScreenBuffer Rendered:%d ", nRenderedCount);

//    TIME_SECTION_END(RenderVisibleRects);

	return (int32_t) nRenderedCount;
}

bool ZScreenBuffer::PaintToSystem(const ZRect& rClip)
{
    if (!mbRenderingEnabled)
        return false;

    //    TIME_SECTION_START(RenderVisibleRects);

    BITMAPINFO bmpInfo;
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biCompression = BI_RGB;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);

    bmpInfo.bmiHeader.biWidth = (LONG)mSurfaceArea.Width();
    bmpInfo.bmiHeader.biHeight = (LONG)-mSurfaceArea.Height();

    DWORD nStartScanline = (DWORD)rClip.top;
    DWORD nScanLines = (DWORD)rClip.Height();

    void* pBits = mpPixels + rClip.top * mSurfaceArea.Width();

    int nRet = SetDIBitsToDevice(mDC,   // HDC
    (DWORD)rClip.left,       // Dest X
    (DWORD)rClip.top,        // Dest Y
    (DWORD)rClip.Width(),    // Dest Width
    (DWORD)rClip.Height(),   // Dest Height
    (DWORD)rClip.left,     // Src X
    (DWORD)rClip.top,      // Src Y
    nStartScanline,                 // Start Scanline
    nScanLines,                     // Num Scanlines
    pBits,                          // * pixels
    &bmpInfo,                       // BMPINFO
    DIB_RGB_COLORS);                // Usage
  
    return true;
}


bool ZScreenBuffer::RenderBuffer(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst)
{
    if (!mbRenderingEnabled || !pSrc)
        return false;

    if (!mSurfaceArea.Overlaps(rDst))
        return true;

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

    int nRet = SetDIBitsToDevice(mDC,   // HDC
        (DWORD)rDst.left,               // Dest X
        (DWORD)rDst.top,                // Dest Y
        (DWORD)rDst.Width(),            // Dest Width
        (DWORD)rDst.Height(),           // Dest Height
        (DWORD)rSrc.left,               // Src X
        (DWORD)rSrc.top,                // Src Y
        nStartScanline,                 // Start Scanline
        nScanLines,                     // Num Scanlines
        pBits,                          // * pixels
        &bmpInfo,                       // BMPINFO
        DIB_RGB_COLORS);                // Usage

    pSrc->GetMutex().unlock();

    return true;
}


/*int32_t ZScreenBuffer::RenderVisibleRectsToBuffer(ZBuffer* pDst, const ZRect& rClip)
{
    if (!mbRenderingEnabled)
        return 0;

    tZBufferPtr pCurBuffer;

    int64_t nRenderedCount = 0;

    const std::lock_guard<std::mutex> surfaceLock(mScreenRectListMutex);
    for (auto& sr : mScreenRectList)
    {
        // If no overlap, move on
        if (!sr.mrDest.Overlaps(rClip))
            continue;


        // Since the mScreenRectList happens to be sorted so that referenced textures are together, we should lock, render all from that texture, then unlock
        if (sr.mpSourceBuffer != pCurBuffer)
        {
            if (pCurBuffer)
                pCurBuffer->GetMutex().unlock();

            pCurBuffer = sr.mpSourceBuffer;
            pCurBuffer->GetMutex().lock();
        }

        nRenderedCount++;
        ZRect rTexture(sr.mpSourceBuffer->GetArea());

        ZRect rClippedSource(sr.mSourcePt.x, sr.mSourcePt.y, sr.mSourcePt.x + sr.mrDest.Width(), sr.mSourcePt.y + sr.mrDest.Height());
        ZRect rClippedDest(sr.mrDest);

        Clip(rClip, rClippedSource, rClippedDest);

        pDst->Blt(sr.mpSourceBuffer.get(), rClippedSource, rClippedDest);
    }

    if (pCurBuffer)
        pCurBuffer->GetMutex().unlock();

    return (int32_t)nRenderedCount;
}
*/


#endif // _WIN64

//#define DEBUG_VISIBILITY
#ifdef DEBUG_VISIBILITY
#define DebugVisibilityOutput ZDEBUG_OUT
#else
#define DebugVisibilityOutput
#endif

bool ZScreenBuffer::AddScreenRectAndComputeVisibility(const ZScreenRect& screenRect)
{
    assert(screenRect.mpSourceBuffer);

    if (!mbRenderingEnabled)
        return false;

	// The requirement here is that the newly added rect is on top of all previous rects (i.e. painters alg)

    const std::lock_guard<std::mutex> surfaceLock(mScreenRectListMutex);

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
    for (auto& sr : newList)
    {
        if (sr.mpSourceBuffer->mRenderState != ZBuffer::eRenderState::kBusy_SkipRender) // only set ready to render if not busy
            sr.mpSourceBuffer->mRenderState = ZBuffer::kReadyToRender;
    }

	mScreenRectList = newList;
	return true;
}
