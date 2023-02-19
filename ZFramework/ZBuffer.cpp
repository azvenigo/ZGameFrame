#include "ZBuffer.h"
#include <algorithm>
#include "ZStringHelpers.h"
#include "helpers/StringHelpers.h"
#include "ZRasterizer.h"
#include <math.h>

#ifdef _WIN64
#include <GdiPlus.h>
using namespace Gdiplus;
#include "GDIImageTags.h"
#endif

using namespace std;

#include "ZGraphicSystem.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


ZBuffer::ZBuffer()
{
	mpPixels                = NULL;
    mRenderFlags            = kRenderFlag_None; // 0
	mSurfaceArea.SetRect(0,0,0,0);
}

ZBuffer::ZBuffer(const ZBuffer* pSrc) : ZBuffer()
{
    ZRect rArea(pSrc->mSurfaceArea);
    Init(rArea.Width(), rArea.Height());
    Blt((ZBuffer*) pSrc, rArea, rArea);
}


ZBuffer::~ZBuffer()
{
	Shutdown();
}

bool ZBuffer::Init(int64_t nWidth, int64_t nHeight)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);


	ZASSERT(nWidth > 0 && nHeight > 0);
	if (nWidth > 0 && nHeight > 0)
	{
        if (!mpPixels || nWidth * nHeight != mSurfaceArea.Width() * mSurfaceArea.Height())   // if the number of pixels hasn't changed, no need to reallocate
        {
            delete[] mpPixels;
            mpPixels = new uint32_t[nWidth * nHeight];
        }

		mSurfaceArea.SetRect(0, 0, nWidth, nHeight);
		Fill(mSurfaceArea, 0);

		return true;
	}
    else
    {
        delete[] mpPixels;
        mpPixels = nullptr;
    }


	return false;
}

bool ZBuffer::Shutdown()
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (mpPixels)
	{
		delete[] mpPixels;
		mpPixels = NULL;
	}

	return true;
};

bool ZBuffer::LoadBuffer(const string& sFilename)
{
    mBufferProps.clear();
#ifdef _WIN64
	Bitmap bitmap(StringHelpers::string2wstring(sFilename).c_str());
    if (bitmap.GetLastStatus() != Status::Ok)
    {
        //OutputDebugImmediate("ZBuffer::LoadBuffer failed to load %s. Status:%d\n", sFilename.c_str(), bitmap.GetLastStatus());
        ZDEBUG_OUT("ZBuffer::LoadBuffer failed to load ", sFilename, " status:", bitmap.GetLastStatus());
        return false;
    }


    uint32_t nPropertySize = 0;
    uint32_t nPropertyCount = 0;

    bitmap.GetPropertySize(&nPropertySize, &nPropertyCount);


    uint32_t nHeight = bitmap.GetHeight();

    PropertyItem* pPropBuffer = (PropertyItem*)malloc(nPropertySize);
    bitmap.GetAllPropertyItems(nPropertySize, nPropertyCount, pPropBuffer);

    ZDEBUG_OUT("Properties for image:", sFilename);
    for (size_t i = 0; i < nPropertyCount; i++)
    {
        PropertyItem* pi = &pPropBuffer[i];
        if (pi->id == PropertyTagOrientation)
        {
            auto orientation = *((uint16_t*)pi->value);
            if (orientation != 1)
            {
                ZDEBUG_OUT("Rotating image. Orientation property:", orientation);

                switch (orientation)
                {
                case 2:
                    bitmap.RotateFlip(RotateNoneFlipX);
                    break;
                case 3:
                    bitmap.RotateFlip(Rotate180FlipNone);
                    break;
                case 4:
                    bitmap.RotateFlip(Rotate180FlipX);
                    break;
                case 5:
                    bitmap.RotateFlip(Rotate90FlipX);
                    break;
                case 6:
                    bitmap.RotateFlip(Rotate90FlipNone);
                    break;
                case 7:
                    bitmap.RotateFlip(Rotate270FlipX);
                    break;
                case 8:
                    bitmap.RotateFlip(Rotate270FlipNone);
                    break;
                }
            }
        }

        BufferProp prop;
        prop.sName = GDIHelpers::TagFromID(pi->id);
        prop.sType = GDIHelpers::TypeString(pi->type);
        int32_t nLength = pi->length;
        if (nLength > 128)
            nLength = 128;
        prop.sValue = GDIHelpers::ValueStringByType(pi->type, pi->value, nLength);
        mBufferProps.push_back(prop);

//        OutputDebugImmediate("%d:%s\n", i, GDIHelpers::PropertyItemToString(&pPropBuffer[i]).c_str());
        ZDEBUG_OUT_LOCKLESS(i, ":", GDIHelpers::PropertyItemToString(&pPropBuffer[i]));
    }
    

    free(pPropBuffer);

	Shutdown(); // Clear out any existing data

	if (!Init(bitmap.GetWidth(), bitmap.GetHeight()))
		return false;

	for (int64_t y = 0; y < (int64_t) bitmap.GetHeight(); y++)
	{
		for (int64_t x = 0; x < (int64_t) bitmap.GetWidth(); x++)
		{
			Color col = 0;
			bitmap.GetPixel((INT)x, (INT)y, &col);
			SetPixel(x, y, col.GetValue());
		}
	}

	return true;
#endif
}

#ifdef _WIN64
int GetEncoderClsid(const string& sFilename, CLSID* pClsid)
{
    string sExtension;
    size_t nLastDot = sFilename.rfind('.');
    if (nLastDot == std::string::npos)
    {
        cerr << "Couldn't extract extension from:" << sFilename << "\n";
        return -1;
    }

    sExtension = sFilename.substr(nLastDot + 1);
    StringHelpers::makelower(sExtension);

    wstring wsFormat;
    if (sExtension == "jpg" || sExtension == "jpeg")
        wsFormat = L"image/jpeg";
    else if (sExtension == "gif")
        wsFormat = L"image/gif";
    else if (sExtension == "bmp")
        wsFormat = L"image/bmp";
    else if (sExtension == "png")
        wsFormat = L"image/png";
    else if (sExtension == "tiff" || sExtension == "tif")
        wsFormat = L"image/tiff";
    else
    {
        ZDEBUG_OUT("Unsupported extension for filename:", sFilename);
        return -1;
    }

        




    UINT  num = 0;          // number of image encoders
    UINT  size = 0;         // size of the image encoder array in bytes

    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;  // Failure

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL)
        return -1;  // Failure

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, wsFormat.c_str()) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // Success
        }
    }

    free(pImageCodecInfo);
    return -1;  // Failure
}

bool ZBuffer::SaveBuffer(const string& sFilename)
{
    BITMAPINFO bi;
    
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = (LONG)GetArea().Width();
    bi.bmiHeader.biHeight = (LONG)-GetArea().Height();
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = 0;
    bi.bmiHeader.biXPelsPerMeter = 72;
    bi.bmiHeader.biYPelsPerMeter = 72;
    bi.bmiHeader.biClrUsed = bi.bmiHeader.biWidth * bi.bmiHeader.biHeight * 4;
    bi.bmiHeader.biClrImportant = 0;
//    bi.bmiColors = mpPixels;

    CLSID   encoderClsid;
       // Get the CLSID of the PNG encoder.
    GetEncoderClsid(sFilename, &encoderClsid);

    Bitmap bitmap(&bi, (void*) mpPixels);
    bitmap.Save(StringHelpers::string2wstring(sFilename).c_str(), (const CLSID*)&encoderClsid);

    return true;
}
#endif


#ifdef _WIN64
// TBD, do we want to keep loading from a resource?
bool ZBuffer::LoadBuffer(uint32_t nResourceID)
{
	Bitmap bitmap((HINSTANCE)::GetModuleHandle(NULL), MAKEINTRESOURCEW(nResourceID));

	Shutdown(); // Clear out any existing data

	if (!Init(bitmap.GetWidth(), bitmap.GetHeight()))
		return false;

	for (int64_t y = 0; y < (int64_t) bitmap.GetHeight(); y++)
	{
		for (int64_t x = 0; x < (int64_t) bitmap.GetWidth(); x++)
		{
			Color col = 0;
			bitmap.GetPixel((INT)x, (INT)y, &col);
			SetPixel(x, y, col.GetValue());
		}
	}

	return true;
}
#endif

bool ZBuffer::BltNoClip(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, eAlphaBlendType type)
{
	int64_t nBltWidth = rDst.Width();
	int64_t nBltHeight = rDst.Height();

	uint32_t* pSrcBits = pSrc->GetPixels() + (rSrc.top * pSrc->GetArea().Width()) + rSrc.left;
	uint32_t* pDstBits = mpPixels + (rDst.top * mSurfaceArea.Width()) + rDst.left;

	for (int64_t y = 0; y < nBltHeight; y++)
	{
		for (int64_t x = 0; x < nBltWidth; x++)
		{
#ifdef _DEBUG
//			CEASSERT_MESSAGE( pSrcBits < pSrc->GetPixels() + pSrc->GetArea().Width() * pSrc->GetArea().Height(), "Source buffer pointer out of range." );
//			CEASSERT_MESSAGE( pDstBits < mpPixels + mSurfaceArea.Width() * mSurfaceArea.Height(), "Dest buffer pointer out of range." );
#endif

			uint8_t nAlpha = ARGB_A(*pSrcBits);
			if (nAlpha > 250)
				*pDstBits = *pSrcBits;
			else if (nAlpha >= 5)
			{
				if (type == kAlphaDest)
					*pDstBits = AlphaBlend_Col2Alpha(*pSrcBits, *pDstBits, nAlpha);
				else
					*pDstBits = AlphaBlend_Col1Alpha(*pSrcBits, *pDstBits, nAlpha);
			}

			pSrcBits++;
			pDstBits++;
		}

		pSrcBits += (pSrc->GetArea().Width() - nBltWidth);  // Next line in the source buffer
		pDstBits += (mSurfaceArea.Width() - nBltWidth);        // Next line in the destination buffer
	}

	return true;
}


bool ZBuffer::BltAlphaNoClip(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, uint32_t nAlpha, eAlphaBlendType type)
{
	if (nAlpha < 5)     // If less than a small threshhold, we won't see anything from the source buffer anyway
		return true;

//	if (nAlpha > 250)     // If close to 1, just do a plain blt
//		return BltNoClip(pSrc, rSrc, rDst);

	int64_t nBltWidth = rDst.Width();
	int64_t nBltHeight = rDst.Height();

	uint32_t* pSrcBits = pSrc->GetPixels() + (rSrc.top * pSrc->GetArea().Width()) + rSrc.left;
	uint32_t* pDstBits = mpPixels + (rDst.top * mSurfaceArea.Width()) + rDst.left;


	for (int64_t y = 0; y < nBltHeight; y++)
	{
		for (int64_t x = 0; x < nBltWidth; x++)
		{
			uint32_t nColSrc = *pSrcBits;
			if (ARGB_A(nColSrc) != 0)
			{
				if (type == kAlphaDest)
					*pDstBits = AlphaBlend_Col2Alpha(*pSrcBits, *pDstBits, nAlpha);
				else
					*pDstBits = AlphaBlend_Col1Alpha(*pSrcBits, *pDstBits, nAlpha);
			}

			pSrcBits++;
			pDstBits++;
		}

		pSrcBits += (pSrc->GetArea().Width() - nBltWidth);  // Next line in the source buffer
		pDstBits += (mSurfaceArea.Width() - nBltWidth);        // Next line in the destination buffer
	}


	return true;
}
/*inline
bool cCEBuffer::BltToNoClip(cCEBuffer* pDst, ZRect& rSrc, ZRect& rDst)
{
CEASSERT(!"No implemented");
return false;
}*/

bool ZBuffer::CopyPixels(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, ZRect* pClip)
{
	ZRect rSrcClipped(rSrc);
	ZRect rDstClipped(rDst);

	ZRect rClip;
	if (pClip)
	{
		rClip.SetRect(*pClip);
		rClip.IntersectRect(&mSurfaceArea);
	}
	else
		rClip.SetRect(mSurfaceArea);

	if (Clip(rClip, rSrcClipped, rDstClipped))
	{
		int64_t nBltWidth = rDst.Width();
		int64_t nBltHeight = rDst.Height();

		uint32_t* pSrcBits = pSrc->GetPixels() + (rSrc.top * pSrc->GetArea().Width()) + rSrc.left;
		uint32_t* pDstBits = mpPixels + (rDst.top * mSurfaceArea.Width()) + rDst.left;

		for (int64_t y = 0; y < nBltHeight; y++)
		{
			memcpy(pDstBits, pSrcBits, nBltWidth * sizeof(uint32_t));
/*			for (int64_t x = 0; x < nBltWidth; x++)
			{
				*pDstBits = *pSrcBits;

				pSrcBits++;
				pDstBits++;
			}*/

			pSrcBits += (pSrc->GetArea().Width());  // Next line in the source buffer
			pDstBits += (mSurfaceArea.Width());        // Next line in the destination buffer
		}

		return true;
	}
	return false;
}


bool ZBuffer::Blt(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, ZRect* pClip, eAlphaBlendType type)
{
	ZRect rSrcClipped(rSrc);
	ZRect rDstClipped(rDst);

	ZRect rClip;
	if (pClip)
	{
		rClip.SetRect(*pClip);
		rClip.IntersectRect(&mSurfaceArea);
	}
	else
		rClip.SetRect(mSurfaceArea);

	if (Clip(rClip, rSrcClipped, rDstClipped))
		return BltNoClip(pSrc, rSrcClipped, rDstClipped, type);
	return false;
}

bool ZBuffer::BltAlpha(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, uint32_t nAlpha, ZRect* pClip, eAlphaBlendType type)
{
	ZRect rSrcClipped(rSrc);
	ZRect rDstClipped(rDst);

	ZRect rClip;
	if (pClip)
	{
		rClip.SetRect(*pClip);
		rClip.IntersectRect(&mSurfaceArea);
	}
	else
		rClip.SetRect(mSurfaceArea);

	if (Clip(rClip, rSrcClipped, rDstClipped))
		return BltAlphaNoClip(pSrc, rSrcClipped, rDstClipped, nAlpha, type);
	return false;
}

bool ZBuffer::Fill(ZRect& rDst, uint32_t nCol)
{
	ZRect rSrcArea(mSurfaceArea);

	if(Clip(rSrcArea, rSrcArea, rDst))
	{
		int64_t nDstStride = mSurfaceArea.Width();

		int64_t nFillWidth = rDst.Width();
		int64_t nFillHeight = rDst.Height();


		// Fully opaque
		for (int64_t y = 0; y < nFillHeight; y++)
		{
			uint32_t* pDstBits = (uint32_t*) (mpPixels + ((y + rDst.top) * nDstStride) + rDst.left);
			for (int64_t x = 0; x < nFillWidth; x++)
				*pDstBits++ = nCol;
		}
	}

	return true;
}

bool ZBuffer::FillAlpha(ZRect& rDst, uint32_t nCol)
{
	ZRect rSrcArea(mSurfaceArea);

	if(Clip(rSrcArea, rSrcArea, rDst))
	{
		int64_t nDstStride = mSurfaceArea.Width();

		int64_t nFillWidth = rDst.Width();
		int64_t nFillHeight = rDst.Height();


		int64_t nAlpha = ARGB_A(nCol);
		if (nAlpha > 250)
		{
			// Fully opaque
			for (int64_t y = 0; y < nFillHeight; y++)
			{
				uint32_t* pDstBits = (uint32_t*) (mpPixels + ((y + rDst.top) * nDstStride) + rDst.left);
				for (int64_t x = 0; x < nFillWidth; x++)
					*pDstBits++ = nCol;
			}
		}
		else
		{
			for (int64_t y = 0; y < nFillHeight; y++)
			{
				uint32_t* pDstBits = (uint32_t*) (mpPixels + ((y + rDst.top) * nDstStride) + rDst.left);
				for (int64_t x = 0; x < nFillWidth; x++)
				{
					*pDstBits++ = AlphaBlend_Col2Alpha(nCol, *pDstBits, ARGB_A(nCol));
				}
			}
		}
	}

	return true;
}


inline uint32_t ZBuffer::ConvertRGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	return ARGB(a, r, g, b);
}

inline void ZBuffer::ConvertToRGB(uint32_t col, uint8_t& a, uint8_t& r, uint8_t& g, uint8_t& b)
{
	a = ARGB_A(col);
	r = (uint8_t) (ARGB_R(col));
	b = (uint8_t) (ARGB_B(col));
	g = (uint8_t) (ARGB_G(col));
}



bool ZBuffer::Clip(const ZRect& fullDstRect, ZRect& srcRect, ZRect& dstRect)
{
	int64_t   nOverhangDistance;  
	ZRect	rBufferArea(fullDstRect);

	//Clipping:
	if((nOverhangDistance = dstRect.right - rBufferArea.right) > 0)
	{
		dstRect.right -= nOverhangDistance;
		srcRect.right  -= nOverhangDistance;
		if(dstRect.right <= dstRect.left)
			return false; //The dest rect is entirely outside of clipping area.
	}
	if((nOverhangDistance = rBufferArea.left - dstRect.left) > 0)
	{
		dstRect.left += nOverhangDistance;
		srcRect.left  += nOverhangDistance;
		if(dstRect.left >= dstRect.right)
			return false; //The dest rect is entirely outside of clipping area.
	}
	if((nOverhangDistance = dstRect.bottom - rBufferArea.bottom) > 0)
	{
		dstRect.bottom -= nOverhangDistance;
		srcRect.bottom  -= nOverhangDistance;
		if(dstRect.bottom <= dstRect.top)
			return false; //The dest rect is entirely outside of clipping area.
	}
	if((nOverhangDistance = rBufferArea.top - dstRect.top) > 0)
	{
		dstRect.top += nOverhangDistance;
		srcRect.top  += nOverhangDistance;
		if(dstRect.top >= dstRect.bottom)
			return false; //The dest rect is entirely outside of clipping area.
	}

	return true;
}

inline
uint32_t ZBuffer::GetPixel(int64_t x, int64_t y)
{
	return *(mpPixels + y * (mSurfaceArea.Width()) + x);
}

inline
void ZBuffer::SetPixel(int64_t x, int64_t y, uint32_t nCol)
{
	*(mpPixels + y * mSurfaceArea.Width() + x) = nCol;
}

bool ZBuffer::BltTiled(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, int64_t nStartX, int64_t nStartY, ZRect* pClip, eAlphaBlendType type)
{
	int64_t nSrcWidth = rSrc.Width();
	int64_t nSrcHeight = rSrc.Height();

	//CEASSERT(nSrcHeight > 0 && nSrcWidth > 0);
	if (nSrcHeight <= 0 || nSrcWidth <= 0)
		return false;

	ZRect rClip(rDst);
	if (pClip)
	{
		if (!rClip.IntersectRect(pClip))
		{
			return false;
		}
	}

	for (int64_t y = rDst.top-nStartY; y < rDst.bottom; y+= nSrcHeight)
	{
		for (int64_t x = rDst.left-nStartX; x < rDst.right; x+= nSrcWidth)
		{
			ZRect rCurrentDst(x, y, x + nSrcWidth, y + nSrcHeight);
			Blt(pSrc, rSrc, rCurrentDst, &rClip, type);
		}
	}

	return true;
}


// Source Bitmap is split into 9 regions
//
//             w
// /-----------^------------\
//         |       |        |             
//    TL   |   T   |   TR   |              
//         |       |        |              
//  -------+---t---+--------|             
//         |       |        |              
//     L   l   M   r   R    >h             
//         |       |        |              
//  -------+---b---+--------|             
//         |       |        |              
//    BL   |   B   |   BR   |             
//         |       |        /              
//                                        
//  The variables l,t,r,b are passed in the rEdgeRect
//
//  This function Blts the TL, TR, BL, BR corners into the destination corners
//  The edges are tiled into the destination edges
//  The Middle section is tile blt if so desired
//
//
//
//    Destination regions are as follows
//
//            dw
// /-----------^------------\
//         |       |        |              
//    dTL  |   dT  |  dTR   |               
//         |       |        |               
//  -------+--dt---+--------|              
//         |       |        |               
//    dL  dl  dM  dr  dR    >dh
//         |       |        |               
//  -------+--db---+--------|              
//         |       |        |               
//   dBL   |  dB   |  dBR   |              
//         |       |        |               
//                                        
bool ZBuffer::BltEdge(ZBuffer* pSrc, ZRect& rEdgeRect, ZRect& rDst, eBltEdgeMiddleType middleType, ZRect* pClip, eAlphaBlendType type)
{
	ZASSERT(pSrc);

	ZRect rClip(rDst);
	if (pClip)
	{
		if (!rClip.IntersectRect(pClip))
			return false;
	}

	// Set up the Source Rectangles
	int64_t w = pSrc->GetArea().Width();
	int64_t h = pSrc->GetArea().Height();

	int64_t l = rEdgeRect.left;
	int64_t t = rEdgeRect.top;
	int64_t r = rEdgeRect.right;
	int64_t b = rEdgeRect.bottom;

	// Adjustments for destinations whos width or height are smaller than the source rect
/*	if (rDst.Height() < t+b)
	{
		t = rDst.Height()/2;
		b = h - t;
	}

	if (rDst.Width() < l+r)
	{
		l = rDst.Width()/2;
		r = w - l;
	}*/

	ZRect rTL(0, 0, l, t);
	ZRect rTR(r, 0, w, t);
	ZRect rBL(0, b, l, h);
	ZRect rBR(r, b, w, h);

	ZRect rT(l, 0, r, t);
	ZRect rL(0, t, l, b);
	ZRect rR(r, t, w, b);
	ZRect rB(l, b, r, h);

	ZRect rM(l,t,r,b);


	// Set up the Destination Rectangles
	int64_t dl = rDst.left + l;
	int64_t dt = rDst.top + t;
	int64_t dr = rDst.right - (w-r);
	int64_t db = rDst.bottom - (h-b);

	ZRect rdTL(rDst.left, rDst.top, dl, dt);
	ZRect rdTR(dr, rDst.top, rDst.right, dt);
	ZRect rdBL(rDst.left, db, dl, rDst.bottom);
	ZRect rdBR(dr,db, rDst.right, rDst.bottom);

	ZRect rdT(dl, rDst.top, dr, dt);
	ZRect rdL(rDst.left, dt, dl, db);
	ZRect rdR(dr, dt, rDst.right, db);
	ZRect rdB(dl, db, dr, rDst.bottom);

	ZRect rdM(dl, dt, dr, db);

	bool bBlitTop		= rdTL.Height()>0;
	bool bBlitLeft		= rdTL.Width()>0;
	bool bBlitBottom	= rdBR.Height()>0;
	bool bBlitRight		= rdBR.Width()>0;

	if (bBlitTop && bBlitLeft)
		Blt(pSrc, rTL, rdTL, &rClip, type);

	if (bBlitTop && bBlitRight)
		Blt(pSrc, rTR, rdTR, &rClip, type);

	if (bBlitBottom && bBlitLeft)
		Blt(pSrc, rBL, rdBL, &rClip, type);

	if (bBlitBottom && bBlitRight)
		Blt(pSrc, rBR, rdBR, &rClip, type);

	if (bBlitTop)
		BltTiled(pSrc, rT, rdT, 0, 0, &rClip, type);

	if (bBlitLeft)
		BltTiled(pSrc, rL, rdL, 0, 0, &rClip, type);

	if (bBlitRight)
		BltTiled(pSrc, rR, rdR, 0, 0, &rClip, type);

	if (bBlitBottom)
		BltTiled(pSrc, rB, rdB, 0, 0, &rClip, type);

    if (rdM.Width() > 0 && rdM.Height() > 0)
    {
        if (middleType == kEdgeBltMiddle_Tile)
        {
            BltTiled(pSrc, rM, rdM, 0, 0, &rClip, type);
        }
        else if (middleType == kEdgeBltMiddle_Stretch)
        {
            tUVVertexArray middleVerts;
            gRasterizer.RectToVerts(rdM, middleVerts);

            middleVerts[0].u = (double)rEdgeRect.left / (double) w;
            middleVerts[0].v = (double)rEdgeRect.top / (double)h;

            middleVerts[1].u = (double)rEdgeRect.right / (double)w;
            middleVerts[1].v = (double)rEdgeRect.top / (double)h;

            middleVerts[2].u = (double)rEdgeRect.right / (double)w;
            middleVerts[2].v = (double)rEdgeRect.bottom / (double)h;

            middleVerts[3].u = (double)rEdgeRect.left / (double)w;
            middleVerts[3].v = (double)rEdgeRect.bottom / (double)h;

            gRasterizer.Rasterize(this, pSrc, middleVerts, &rClip);
        }
    }

	return true;
}

inline
bool ZBuffer::FloatScanLineIntersection(double fScanLine, const ZColorVertex& v1, const ZColorVertex& v2, double& fIntersection, double& fR, double& fG, double& fB, double& fA)
{
	if (v1.y == v2.y || v1.x == v2.x)	
	{
		fIntersection = v1.x;
		fA = (double) (v1.mColor >> 24);
		fR = (double) ((v1.mColor & 0x00ff0000) >> 16);
		fG = (double) ((v1.mColor & 0x0000ff00) >> 8);
		fB = (double) ((v1.mColor & 0x000000ff));
		return false;			
	}

	// Calculate line intersection
	double fM = (double)((v1.y - v2.y)/(v1.x - v2.x)); // slope
	fIntersection = (v2.x + (fScanLine-v2.y)/fM);

	double fT = (fScanLine - v1.y) / (v2.y - v1.y);  // how far along line (0.0 - 1.0)

	// Calculate color at intersection
	int64_t nA1 = (v1.mColor) >> 24;
	int64_t nA2 = (v2.mColor) >> 24;

	int64_t nR1 = (v1.mColor & 0x00ff0000) >> 16;
	int64_t nR2 = (v2.mColor & 0x00ff0000) >> 16;

	int64_t nG1 = (v1.mColor & 0x0000ff00) >> 8;
	int64_t nG2 = (v2.mColor & 0x0000ff00) >> 8;

	int64_t nB1 = (v1.mColor & 0x000000ff);
	int64_t nB2 = (v2.mColor & 0x000000ff);


	fA = (double) (nA1 + (nA2 - nA1) * fT);
	fR = (double) (nR1 + (nR2 - nR1) * fT);
	fG = (double) (nG1 + (nG2 - nG1) * fT);
	fB = (double) (nB1 + (nB2 - nB1) * fT);

	return true;
}

inline 
void ZBuffer::FillInSpan(uint32_t* pDest, int64_t nNumPixels, double fR, double fG, double fB, double fA)
{
	while (nNumPixels > 0)
	{
		uint8_t nCurA;
		uint8_t nCurR;
		uint8_t nCurG;
		uint8_t nCurB;
		ConvertToRGB(*pDest, nCurA, nCurR, nCurG, nCurB);


		uint8_t nDestR = static_cast<uint8_t>((uint32_t) ((fR * fA + (nCurR * (255.0f-fA)))) >> 8);
		uint8_t nDestG = static_cast<uint8_t>((uint32_t) ((fG * fA + (nCurG * (255.0f-fA)))) >> 8);
		uint8_t nDestB = static_cast<uint8_t>((uint32_t) ((fB * fA + (nCurB * (255.0f-fA)))) >> 8);

		*pDest = ConvertRGB(255, nDestR, nDestG, nDestB);

		pDest++;
		nNumPixels--;
	} 
}

void ZBuffer::DrawAlphaLine(const ZColorVertex& v1, const ZColorVertex& v2, ZRect* pClip)
{
	ZRect rDest;
	const double kfThickness = 2.0f;

	if (pClip)
	{
		rDest = *pClip;
		rDest.IntersectRect(&mSurfaceArea);
	}
	else
		rDest = mSurfaceArea;

	ZRect rLineRect;

	rLineRect.top = (int64_t) min(v1.y, v2.y);
	rLineRect.bottom = (int64_t) max(v1.y, v2.y);
	rLineRect.left = (int64_t) min(v1.x - kfThickness/2.0f, v2.x - kfThickness/2.0f);
	rLineRect.right = (int64_t) max(v1.x + kfThickness/2.0f, v2.x + kfThickness/2.0f);

	rLineRect.IntersectRect(/*&rLineRect, */&rDest);

	uint32_t* pSurface = GetPixels();
	int64_t nStride = mSurfaceArea.Width();

	double fScanLine = (double) rLineRect.top;
	double fIntersection;
	double fR;
	double fG;
	double fB;
	double fA;

	double fPrevScanLineIntersection;
	if (!FloatScanLineIntersection(fScanLine, v1, v2, fPrevScanLineIntersection, fR, fG, fB, fA) && v1.y == v2.y)
	{
		// Horizontal line is a special case:
		uint32_t* pDest = pSurface + rLineRect.top * nStride + rLineRect.left;
		FillInSpan(pDest, rLineRect.Width(), fR, fG, fB, fA);
	}

	for (int64_t nScanLine = rLineRect.top; nScanLine <= rLineRect.bottom; nScanLine++)
	{
		FloatScanLineIntersection(fScanLine, v1, v2, fIntersection, fR, fG, fB, fA);

		if (fA > 10.0f)		// Anything less is not really visible
		{
			int64_t nStartPixel = (int64_t) min(fIntersection - kfThickness/2.0f, fPrevScanLineIntersection - kfThickness/2.0f);
			nStartPixel = max(nStartPixel, rLineRect.left);		// clip left

			int64_t nEndPixel = (int64_t) max(fIntersection + kfThickness/2.0f, fPrevScanLineIntersection + kfThickness/2.0f);
			nEndPixel = min(nEndPixel, rLineRect.right);

			uint32_t* pDest = pSurface + nScanLine * nStride + nStartPixel;

			FillInSpan(pDest, nEndPixel - nStartPixel, fR, fG, fB, fA);
		}

		fPrevScanLineIntersection = fIntersection;
		fScanLine += 1.0f;
	}
}


inline void TransformPoint(ZFPoint& pt, const ZFPoint& ptOrigin, double angle, double fScale)
{
	const double x(pt.x - ptOrigin.x);
	const double y(pt.y - ptOrigin.y);
	const double cosAngle((double)::cos(angle));
	const double sinAngle((double)::sin(angle));

	pt.x = ptOrigin.x + fScale*(x*cosAngle - y*sinAngle);
	pt.y = ptOrigin.y + fScale*(x*sinAngle + y*cosAngle);
}

bool ZBuffer::BltRotated(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, double fAngle, double fScale, ZRect* pClip)
{
/*	ZUVVertex vert;
	vert.mfX = rSrc.left;
	vert.mfY = rSrc.

	tVertexArray vertArray;
	vertArray.push_back(vert);

	vert.mfX = rSrc.right;
}

/*bool cCEBuffer::BltRotated(cCEBuffer* pSrc, ZRect& rSrc, ZRect& rDst, float fAngle, float fScale, ZRect* pClip)
{*/
	int64_t               i, iEnd;
	ZRect				rDstActual;
	int64_t            numRows; 
	const             uint32_t kStackRowCount = 2048;
	int64_t            spanMinStack[kStackRowCount];  //With this, we eliminate the overhead of malloc for most cases.
	int64_t            spanMaxStack[kStackRowCount];  //This speeds things up and relaxes life on the memory manager.
	int64_t*           spanMinHeap = 0;
	int64_t*           spanMaxHeap = 0;
	int64_t*           spanMin;
	int64_t*           spanMax;
	ZFPoint center((rSrc.left+rSrc.right)/2.0F, //Center of source rect
		(rSrc.top+rSrc.bottom)/2.0F);
	ZFPoint destCenter(rDst.Width()/2.0F,          //Center of dest rect, but in units relative to the left 
		rDst.Height()/2.0F);        //  of the rect, and not the left of the destination buffer.
	ZFPoint origin(0, 0);                              //
	ZFPoint       dTex(1, 0);                                //
	ZFPoint       p[4];

	ZASSERT(pSrc);

	rDstActual = rDst;
	rDstActual.IntersectRect(&mSurfaceArea);
	if(pClip)
		rDstActual.IntersectRect(pClip);

	numRows = rDstActual.Height();
	if(numRows > kStackRowCount)
	{
		spanMin = spanMinHeap = new int64_t[numRows]; //We try to avoid doing this whenever possible.
		spanMax = spanMaxHeap = new int64_t[numRows];
	}
	else
	{
		spanMin = spanMinStack;
		spanMax = spanMaxStack;
	}

	// Init the spans to values that are at the extremes.
	for(i=0; i<numRows; i++)
	{   
		spanMin[i] = 0;
		spanMax[i] = rDstActual.Width();
	}

	// For each segment on the rotated rectangle, rasterize the line setting min and max if necessary
	p[0].x = (double)rSrc.left;    //Store the four corners of rectSource as four points.
	p[0].y = (double)rSrc.top;

	p[1].x = (double)rSrc.left;
	p[1].y = (double)rSrc.bottom;

	p[2].x = (double)rSrc.right;
	p[2].y = (double)rSrc.bottom;

	p[3].x = (double)rSrc.right;
	p[3].y = (double)rSrc.top;

	TransformPoint(p[0], center, fAngle, fScale);   //Rotate these points around the center of rectSource.
	TransformPoint(p[1], center, fAngle, fScale);
	TransformPoint(p[2], center, fAngle, fScale);
	TransformPoint(p[3], center, fAngle, fScale);

	p[0].x += destCenter.x - center.x;             //Now translate these points to be relative to destRect, with destRect.left
	p[0].y += destCenter.y - center.y;             //being 0. As if destRect had it's own coordinate system.
	p[0].x += rDst.left - rDstActual.left; //Then translate these points to be relative to destRectActual
	p[0].y += rDst.top  - rDstActual.top;  //  (clipped destination rectangle) instead of just destRect.
	//Now it's as if destRectActual had it's own coordinate system.
	p[1].x += destCenter.x - center.x;
	p[1].y += destCenter.y - center.y;
	p[1].x += rDst.left - rDstActual.left;
	p[1].y += rDst.top  - rDstActual.top;

	p[2].x += destCenter.x - center.x;
	p[2].y += destCenter.y - center.y;
	p[2].x += rDst.left - rDstActual.left;
	p[2].y += rDst.top  - rDstActual.top;

	p[3].x += destCenter.x - center.x;
	p[3].y += destCenter.y - center.y;
	p[3].x += rDst.left - rDstActual.left;
	p[3].y += rDst.top  - rDstActual.top;

	//For each destination scan line from top to bottom, test for 
	//  intersection against the four segments (edges). When we get done with this
	//  loop, we should have the destination left and right edges for each horizontal
	//  scan line in the destination. After that, we simply write in these spans with
	//  values taken from the source buffer. 
	//
	for(i=0, iEnd=rDstActual.Height(); i<iEnd; i++)
	{
		double m;      //slope of the current line.
		int64_t   xInt;   //x-Intercept of the current line with the beginning of the scan line.

		// segment 0-1 intersection
		if ((p[0].y <= i && p[1].y > i) || //If this scan line vertically is in the same range as the 0-1 line.
			(p[1].y <= i && p[0].y > i))
		{
			if (p[0].x == p[1].x)           //If the 0-1 line is vertical, the possible xIntercept is at x.
				xInt = (int) p[0].x;
			else{
				m = (double)((p[0].y - p[1].y)/(p[0].x - p[1].x));       //Calculate the intersection between the current
				xInt = (int64_t)(p[1].x + (double(i)-p[1].y)/m + 1.0F); //line and the current scan-line.
			} //The p0-p1 line crosses the current 'i' horizontal at x = xInt.
			if (xInt < spanMin[i]) //If this xIntercept is lower than the previous lowest xIntercept. 
				spanMin[i] = xInt;  //Since this is the first run through this, the test should always 
			if (xInt > spanMax[i]) //test as true.
				spanMax[i] = xInt;
		}

		// segment 1-2 intersection
		if ((p[1].y <= i && p[2].y > i) ||
			(p[2].y <= i && p[1].y > i))
		{
			if (p[1].x == p[2].x)
				xInt = (int) p[1].x;
			else{
				m = (double)((p[1].y - p[2].y)/(p[1].x - p[2].x));
				xInt = (int64_t)(p[2].x + (double(i)-p[2].y)/m + 1.0F);
			}
			if (xInt < spanMin[i])
				spanMin[i] = xInt;
			if (xInt > spanMax[i])
				spanMax[i] = xInt;
		}

		// segment 2-3 intersection
		if ((p[2].y <= i && p[3].y > i) ||
			(p[3].y <= i && p[2].y > i))
		{
			if (p[2].x == p[3].x)
				xInt = (int) p[2].x;
			else{
				m = (double)((p[2].y - p[3].y)/(p[2].x - p[3].x));
				xInt = (int64_t)(p[3].x + (double(i)-p[3].y)/m + 1.0F);
			}
			if (xInt < spanMin[i])
				spanMin[i] = xInt;
			if (xInt > spanMax[i])
				spanMax[i] = xInt;
		}

		// segment 3-0 intersection
		if ((p[3].y <= i && p[0].y > i) ||
			(p[0].y <= i && p[3].y > i))
		{
			if (p[3].x == p[0].x)   // vertical line
				xInt = (int) p[0].x;
			else{
				m = (double)((p[3].y - p[0].y)/(p[3].x - p[0].x));
				xInt = (int64_t)(p[0].x + (double(i)-p[0].y)/m + 1.0F);
			}
			if (xInt < spanMin[i])
				spanMin[i] = xInt;
			if (xInt > spanMax[i])
				spanMax[i] = xInt;
		}
	}

	TransformPoint(dTex, origin, -fAngle, fScale);     // This is the texture walking vector

	int64_t      strideDiv2    = mSurfaceArea.Width();
	int64_t      strideSrcDiv2 = pSrc->GetArea().Width();
	uint32_t*     pDestBits     = (uint32_t*)mpPixels+rDstActual.top*strideDiv2; //Points to left side of destination buffer at the current row.
	uint32_t*     pSrcBits      = (uint32_t*)pSrc->GetPixels();
	uint32_t*     pCurrentDestBits;
	uint32_t      nCurrentColor;
	ZFPoint		tP;

	for(i=0; i<numRows; i++)
	{
		tP.x = spanMin[i] + (rDstActual.left-rDst.left) - destCenter.x + center.x; //Find first texture pixel in the source buffer. We do this by
		tP.y = (rDstActual.top + i) - rDst.top  - destCenter.y + center.y; //first taking the position of this pixel in the destination
		TransformPoint(tP, center, -fAngle, fScale);                                      //buffer and then translating and rotating it back to its 
		pCurrentDestBits = pDestBits+rDstActual.left+spanMin[i];               //position in the source buffer.

		tP.x += 0.0015f; //This is rather important because there is a slight chance that due to rounding errors
		tP.y += 0.0015f; // the texel value will be just slightly below zero. During integer rounding, the value
		// would round from -0.1 down to -1. This would be bad and cause crashes. 
		//Without these checks, you get rare crashes. Trust me, it happened. 

		for(int64_t lx=spanMin[i], lxEnd=spanMax[i]; lx<lxEnd; lx++)
		{
			nCurrentColor = *(pSrcBits + ((int64_t) tP.x) + ((int64_t)tP.y)*strideSrcDiv2);
			//*pCurrentDestBits = AlphaBlend_Col2Alpha(nCurrentColor, *pCurrentDestBits, 1.0f);
			*pCurrentDestBits = nCurrentColor;
			tP.x += dTex.x;
			tP.y += dTex.y;
			pCurrentDestBits++;
		}
		pDestBits += strideDiv2;
	}

	delete[] spanMinHeap;
	delete[] spanMaxHeap;

	return true;
}