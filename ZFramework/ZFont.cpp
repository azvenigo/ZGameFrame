#include "ZFont.h"
#include <math.h>
#include "ZStringHelpers.h"
#include "ZTimer.h"
#include <fstream>
#include <ZMemBuffer.h>
#include "ZZipAPI.h"
#include "ZGUIStyle.h"
#include <assert.h>
#include "ZRasterizer.h"

ZFontParams::ZFontParams()
{
    nScalePoints = 0;
    nWeight = 0;
    nTracking = 1;
    nFixedWidth = 0;
    bItalic = false;
    bSymbolic = false;
}

ZFontParams::ZFontParams(const ZFontParams& rhs)
{
    nScalePoints = rhs.nScalePoints;
    nWeight = rhs.nWeight;
    nTracking = rhs.nTracking;
    nFixedWidth = rhs.nFixedWidth;
    bItalic = rhs.bItalic;
    bSymbolic = rhs.bSymbolic;
    sFacename = rhs.sFacename;
}



#ifdef _WIN64
extern HWND ghWnd;
#endif
extern ZTimer				gTimer;

#pragma pack (1)

string sZFontHeader("ZFONT v3.1 by Alex Zvenigorodsky");

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


ZFont::ZFont()
{
	mbInitted = false;
    mbEnableKerning = true;
    mnWidestCharacterWidth = 0;
    mnWidestNumberWidth = 0;

#ifdef _WIN64
    mhWinTargetDC = 0;
    mhWinTargetBitmap = 0;
    memset(&mDIBInfo, 0, sizeof(BITMAPINFO));
    mpBits = nullptr;
    mWinHDC = 0;
    mhWinFont = 0;
#endif
}

ZFont::~ZFont()
{
#ifdef _WIN64
    DeleteObject(mhWinFont);
    mhWinFont = 0;

    DeleteObject(mhWinTargetDC);
    mhWinTargetDC = 0;

    DeleteObject(mhWinTargetBitmap);
    mhWinTargetBitmap = 0;
#endif
}

#define COMPRESSED_FONTS

bool ZFont::LoadFont(const string& sFilename)
{
    uint64_t nStartTime = gTimer.GetUSSinceEpoch();

    std::ifstream fontFile(sFilename.c_str(), ios::in | ios::binary | ios::ate);    // open and set pointer at end

    if (!fontFile.is_open())
	{
        ZOUT("Failed to open file \"%s\"\n", sFilename.c_str());
		return false;
	}

    uint32_t nFileSize = (uint32_t) fontFile.tellg();
    fontFile.seekg(0, ios::beg);
    if (nFileSize == 0)
        return false;

	ZASSERT(nFileSize < 10000000);	// 10 meg font?  really?

#ifdef COMPRESSED_FONTS

    ZMemBufferPtr compressedBuffer( new ZMemBuffer(nFileSize));
    fontFile.read((char*)compressedBuffer->data(), nFileSize);
    if (fontFile.gcount() != nFileSize)
        return 0;
    compressedBuffer->seekp(nFileSize);

    ZMemBufferPtr uncompressedBuffer(new ZMemBuffer);
    bool res = ZZipAPI::Decompress(compressedBuffer, uncompressedBuffer, true);
    assert(res);
#else
    
    ZMemBufferPtr uncompressedBuffer(new ZMemBuffer);
    uncompressedBuffer->reserve(nFileSize);
    fontFile.read((char*)uncompressedBuffer->data(), nFileSize);
    assert(fontFile.gcount() == nFileSize);
    uncompressedBuffer->setwrite(nFileSize);
#endif



	uint8_t* pData = uncompressedBuffer->data();

    int64_t nHeaderLength = sZFontHeader.length();
    if (memcmp(pData, sZFontHeader.data(), nHeaderLength) != 0)
	{
        assert(false);
		return false;
	}

	pData += nHeaderLength;

    memcpy(&mFontHeight, pData, sizeof(mFontHeight));
    pData += sizeof(mFontHeight);

    mFontParams.nScalePoints = ZFontParams::ScalePoints(mFontHeight);


    memcpy(&mFontParams.nWeight, pData,  sizeof(mFontParams.nWeight));
    pData += sizeof(mFontParams.nWeight);

    memcpy(&mFontParams.nTracking, pData, sizeof(mFontParams.nTracking));
    pData += sizeof(mFontParams.nTracking);

    memcpy(&mFontParams.nFixedWidth, pData, sizeof(mFontParams.nFixedWidth));
    pData += sizeof(mFontParams.nFixedWidth);

    memcpy(&mFontParams.bItalic, pData, sizeof(mFontParams.bItalic));
    pData += sizeof(mFontParams.bItalic);

    memcpy(&mFontParams.bSymbolic, pData, sizeof(mFontParams.bSymbolic));
    pData += sizeof(mFontParams.bSymbolic);

//	mColorGradient.resize(mFontParams.nHeight);

    // facename length & content
    int16_t nFacenameLength = *((uint16_t*) pData);
    pData += sizeof(uint16_t);

    mFontParams.sFacename.assign((char*) pData, nFacenameLength);
    pData += nFacenameLength;


	for (int32_t i = 0; i < kMaxChars; i++)
	{
        mCharDescriptors[i].nCharWidth = *((uint16_t*) pData);
		pData += 2;

		uint16_t spanDataBytes = *((uint16_t*) pData);
        assert(spanDataBytes %2 == 0);       // has to be even number of data
		pData += sizeof(uint16_t);

		mCharDescriptors[i].pixelData.resize(spanDataBytes);
        memcpy(mCharDescriptors[i].pixelData.data(), pData, spanDataBytes);      // pixel data should be contiguous as a vector
        pData += spanDataBytes;

        memcpy(mCharDescriptors[i].kerningArray, pData, kMaxChars * sizeof(uint16_t));
        pData += kMaxChars * sizeof(uint16_t);
	}

//    ZOUT("Loaded ", sFilename, " in ", gTimer.GetUSSinceEpoch()-nStartTime, "us");

    mbInitted = true;
	return true;
}

bool ZFont::SaveFont(const string& sFilename)
{
    std::ofstream fontFile(sFilename.c_str(), ios::out | ios::binary | ios::trunc);

    if (!fontFile.is_open())
    {
        ZOUT("Failed to open file \"", sFilename, "\"\n");
        return false;
    }

    for (int32_t c = 1; c < kMaxChars; c++)
    {
        if (mCharDescriptors[c].nCharWidth == 0)
            GenerateGlyph(c);
    }


//    uint8_t* pUncompressedFontData = new uint8_t[nUncompressedBytes];
    ZMemBufferPtr uncompBuffer( new ZMemBuffer());

    uncompBuffer->write((uint8_t*) sZFontHeader.data(), (uint32_t) sZFontHeader.length());
//    uncompBuffer->write((uint8_t*) &mFontParams.fScale, sizeof(mFontParams.fScale));
    uncompBuffer->write((uint8_t*)&mFontHeight, sizeof(mFontHeight));
    uncompBuffer->write((uint8_t*)&mFontParams.nWeight, sizeof(mFontParams.nWeight));
    uncompBuffer->write((uint8_t*)&mFontParams.nTracking, sizeof(mFontParams.nTracking));
    uncompBuffer->write((uint8_t*)&mFontParams.nFixedWidth, sizeof(mFontParams.nFixedWidth));
    uncompBuffer->write((uint8_t*)&mFontParams.bItalic, sizeof(mFontParams.bItalic));
    uncompBuffer->write((uint8_t*)&mFontParams.bSymbolic, sizeof(mFontParams.bSymbolic));

    // sFacename length
    int16_t nFacenameLength = (int16_t) mFontParams.sFacename.length();
    uncompBuffer->write((uint8_t*)&nFacenameLength, sizeof(int16_t));
    uncompBuffer->write((uint8_t*)mFontParams.sFacename.data(), nFacenameLength);

    for (int32_t i = 0; i < kMaxChars; i++)
    {
        CharDesc& fontChar = mCharDescriptors[i];
        uncompBuffer->write((uint8_t*)&fontChar.nCharWidth, sizeof(uint16_t));

        // Write out the number of pixel data   
        uint16_t spanDataBytes = (uint16_t) mCharDescriptors[i].pixelData.size();
        assert(spanDataBytes %2 == 0);
        uncompBuffer->write((uint8_t*)&spanDataBytes, sizeof(uint16_t));
        uncompBuffer->write((uint8_t*)mCharDescriptors[i].pixelData.data(), spanDataBytes);

        // kerning data for this char
        uncompBuffer->write((uint8_t*)mCharDescriptors[i].kerningArray, kMaxChars*sizeof(uint16_t));
    }

#ifdef COMPRESSED_FONTS
    ZMemBufferPtr compBuffer(new ZMemBuffer());
    ZZipAPI::Compress(uncompBuffer, compBuffer, true);

    fontFile.write((const char*) compBuffer->data(), compBuffer->size());
#else
    fontFile.write((const char*) uncompBuffer->data(), uncompBuffer->size());
#endif


    fontFile.close();



    return true;
}

int32_t ZFont::GetSpaceBetweenChars(uint8_t c1, uint8_t c2)
{
    if (c1 < 32 || c2 < 32)
        return 0;

    if (mbEnableKerning)
    {
        if (mCharDescriptors[c1].nCharWidth == 0)
        {
            if (!GenerateGlyph(c1))
                return 0;
        }

        if (mCharDescriptors[c2].nCharWidth == 0)
        {
            if (!GenerateGlyph(c2))
                return 0;
        }

        if (mCharDescriptors[c1].kerningArray[c2] == CharDesc::kUnknown)
        {
            FindKerning(c1, c2);
        }

        return mCharDescriptors[c1].kerningArray[c2] + (int32_t) mFontParams.nTracking;
    }

    return (int32_t) mFontParams.nTracking;
}

int64_t ZFont::StringWidth(const string& sText)
{
    ZASSERT(mbInitted);

    if (sText.empty())
        return 0;

    return StringWidth((uint8_t*)sText.c_str(), sText.length());
}

int64_t ZFont::StringWidth(const uint8_t* pChar, size_t length)
{
    int64_t widest = 0;
    int64_t nWidth = 0;
    const uint8_t* pEnd = pChar + length-1;
    for (; pChar < pEnd; pChar++)   // add up all of the chars except last one
    {
        int32_t nBetween = GetSpaceBetweenChars(*pChar, *(pChar + 1));
        nWidth += CharWidth(*pChar) + 1;
        if (nWidth > widest)
            widest = nWidth;
        nWidth += nBetween; // char width adjusted by kerning with next char
    }

    if (*pChar)
    {
        nWidth += CharWidth(*pChar);  // last char
        if (nWidth > widest)
            widest = nWidth;
    }

    return widest;
}


bool ZFont::DrawText(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, ZGUI::Style* pStyle, ZRect* pClip)
{
    ZRect r(ZGUI::Arrange(StringRect(sText), rAreaToDrawTo, pStyle->pos, pStyle->pad.h, pStyle->pad.v));
    return DrawText(pBuffer, sText, r, &pStyle->look, pClip);
}

bool ZFont::DrawText( ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, ZGUI::ZTextLook* pLook, ZRect* pClip )
{
	ZASSERT(mbInitted);
	ZASSERT(pBuffer);

	if (!pBuffer || !mbInitted )
		return false;
    if (!pLook)
        pLook = &gStyleGeneralText.look;

	switch (pLook->decoration)
	{
    case ZGUI::ZTextLook::kNormal:
		if (pLook->colTop == pLook->colBottom && ARGB_A(pLook->colTop) == 0xff)
			return DrawText_Helper(pBuffer, sText, rAreaToDrawTo, pLook->colTop, pClip);
		return DrawText_Gradient_Helper(pBuffer, sText, rAreaToDrawTo, pLook->colTop, pLook->colBottom, pClip);
		break;

	case ZGUI::ZTextLook::kShadowed:
		{
			uint32_t nShadowColor = pLook->colTop & 0xAA000000;		// draw the shadow, but only as dark as the text alpha
            int64_t height = mFontHeight;
            assert(height > 0 && height < 1024);
			int64_t nShadowOffset = height /20;
            limit<int64_t>(nShadowOffset, 1, 32);


			ZRect rShadowRect(rAreaToDrawTo);
			rShadowRect.Offset(nShadowOffset, nShadowOffset);
			if (pLook->colTop == pLook->colBottom)
			{
				DrawText_Helper(pBuffer, sText, rShadowRect, nShadowColor, pClip);
				return DrawText_Helper(pBuffer, sText, rAreaToDrawTo, pLook->colTop, pClip);
			};

			DrawText_Helper(pBuffer, sText, rShadowRect, nShadowColor, pClip);
			return DrawText_Gradient_Helper(pBuffer, sText, rAreaToDrawTo, pLook->colTop, pLook->colBottom, pClip);
		}
		break;

	case ZGUI::ZTextLook::kEmbossed:
		{
			uint32_t nShadowColor = pLook->colTop & 0xAA000000;		// draw the shadow, but only as dark as the text alpha
			uint32_t nHighlightColor = (pLook->colTop & 0xAA000000) | 0x00ffffff;	// the highlight will also only be as bright as the text alpha
            int64_t height = mFontHeight;
            int64_t nShadowOffset = height / 20;
            limit<int64_t>(nShadowOffset, 1, 32);

			ZRect rShadowRect(rAreaToDrawTo);
			rShadowRect.Offset(-nShadowOffset, -nShadowOffset);
	
			ZRect rHighlightRect(rAreaToDrawTo);
			rHighlightRect.Offset(nShadowOffset, nShadowOffset);

			DrawText_Helper(pBuffer, sText, rShadowRect, nShadowColor, pClip);			// draw the shadow
			DrawText_Helper(pBuffer, sText, rHighlightRect, nHighlightColor, pClip);			// draw the highlight
			if (pLook->colTop == pLook->colBottom)
				return DrawText_Helper(pBuffer, sText, rAreaToDrawTo, pLook->colTop, pClip);

			return DrawText_Gradient_Helper(pBuffer, sText, rAreaToDrawTo, pLook->colTop, pLook->colBottom, pClip);
		}
		break;
	}

	return false;
}


inline bool ZFont::DrawText_Helper(ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol, ZRect* pClip)
{
    //cout << "DrawText_Helper... height:" << mFontHeight << "\n";

	int64_t nX = rAreaToDrawTo.left;
	int64_t nY = rAreaToDrawTo.top;
	ZRect rOutputText(nX, nY, nX + StringWidth(sText), nY + mFontHeight);

	ZRect rClip;
	if (pClip)
		rClip.Set(*pClip);
	else
		rClip.Set(pBuffer->GetArea());

	// If the output rect is anywhere outside of the rAreaToDrawTo
	if (rOutputText.left < rClip.left || rOutputText.top < rClip.top || 
		rOutputText.right > rAreaToDrawTo.right || rOutputText.bottom > rAreaToDrawTo.bottom ||
		rOutputText.right > rClip.right || rOutputText.bottom > rClip.bottom)
	{
		ZRect rSrc(rAreaToDrawTo);
		ZRect rClipDest(rOutputText);
        if (pBuffer->Clip(rClip, rSrc, rClipDest))
        {

            // Draw the text clipped
            const uint8_t* pChar = (uint8_t*)sText.data();
            for (; pChar < (uint8_t*)sText.data() + sText.length(); pChar++)
            {
                DrawCharClipped(pBuffer, *pChar, nCol, nX, nY, (ZRect*)&rClipDest);

                if (mFontParams.nFixedWidth > 0)
                {
                    nX += mFontParams.nFixedWidth;
                }
                else
                {
                    int32_t nBetween = GetSpaceBetweenChars(*pChar, *(pChar + 1));
                    nX += CharWidth(*pChar) + 1 + nBetween;
                }

                if (nX >= rAreaToDrawTo.right)
                    break;
            }
        }
	}
	else
	{
        const uint8_t* pChar = (uint8_t*)sText.data();
        for (; pChar < (uint8_t*)sText.data() + sText.length(); pChar++)
        {
			DrawCharNoClip(pBuffer, *pChar, nCol, nX, nY);

            if (mFontParams.nFixedWidth > 0)
            {
                nX += mFontParams.nFixedWidth;
            }
            else
            {
                int32_t nBetween = GetSpaceBetweenChars(*pChar, *(pChar + 1));
                nX += CharWidth(*pChar) + 1 + nBetween;
            }

            if (nX >= rAreaToDrawTo.right)
                break;
		}
	}

	return true;
}

inline bool ZFont::DrawText_Gradient_Helper(ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol, uint32_t nCol2, ZRect* pClip)
{
	int64_t nX = rAreaToDrawTo.left;
	int64_t nY = rAreaToDrawTo.top;
	ZRect rOutputText(nX, nY, nX + StringWidth(sText), nY + mFontHeight);

	ZRect rClip;
	if (pClip)
		rClip.Set(*pClip);
	else
		rClip.Set(pBuffer->GetArea());

	ZRect rSrc(rAreaToDrawTo);
	ZRect rClipDest(rOutputText);
	pBuffer->Clip(rClip, rSrc, rClipDest);

	// IF the text is entirely clipped, exit early
	if (rClipDest.Width() < 1 || rClipDest.Height() < 1)
		return true;

    std::vector<uint32_t> gradient;
	BuildGradient(nCol, nCol2, gradient);

	// Draw the text clipped
    const uint8_t* pChar = (uint8_t*)sText.data();
    for (; pChar < (uint8_t*)sText.data() + sText.length(); pChar++)
    {
		DrawCharGradient(pBuffer, *pChar, gradient, nX, nY, (ZRect*) &rClipDest);

        if (mFontParams.nFixedWidth > 0)
        {
            nX += mFontParams.nFixedWidth;
        }
        else
        {
            int32_t nBetween = GetSpaceBetweenChars(*pChar, *(pChar + 1));
            nX += CharWidth(*pChar) + 1 + nBetween;
        }

		if (nX >= rAreaToDrawTo.right)
            break;
	}
	return true;
}


inline 
void ZFont::DrawCharNoClip(ZBuffer* pBuffer, uint8_t c, uint32_t nCol, int64_t nX, int64_t nY)
{
	ZASSERT(pBuffer);
    if (c <= 32)        // no visible chars below this
        return;

    if (mCharDescriptors[c].nCharWidth == 0)   // generate glyph if not already generated
    {
        if (!GenerateGlyph(c))
            return;
    }


	int64_t nDestStride = pBuffer->GetArea().Width();
	uint32_t* pDest = (pBuffer->GetPixels()) + (nY * nDestStride) + nX;

	int64_t nCharWidth = mCharDescriptors[c].nCharWidth;

    uint8_t nDrawAlpha = nCol >> 24;

	int64_t nScanlineOffset = 0;
    uint8_t* pData = mCharDescriptors[c].pixelData.data();
    uint8_t* pEnd = pData + mCharDescriptors[c].pixelData.size();

    while (pData < pEnd)
	{

        int64_t nBright = (int64_t) *pData;

        pData++;

        int64_t nNumPixels = (int64_t) *pData;
        pData++;

		if (nBright < 5)
		{
			nScanlineOffset += nNumPixels;

			// While the next offset is past the end of this scanline.... advance the dest
			while (nScanlineOffset >= nCharWidth)
			{
				pDest += nDestStride;
				nNumPixels -= nCharWidth;
				nScanlineOffset -= nCharWidth;
			}

			pDest += nNumPixels;
        }
		else
		{
			uint32_t nAlpha = (uint32_t) nBright;
			while (nNumPixels > 0)
			{
                if (nAlpha > 0xf0 && nDrawAlpha > 0xf0)
                    *pDest = nCol;
                else
				    *pDest = COL::AlphaBlend_BlendAlpha(nCol, *pDest, nAlpha);

				// Advance the destination, wrapping around if necessary
				pDest++;

				nScanlineOffset++;
				if (nScanlineOffset >= nCharWidth)
				{
					nScanlineOffset -= nCharWidth;
					pDest += nDestStride - nCharWidth;
                }
				nNumPixels--;
			}
		}
	}
}

inline 
void ZFont::DrawCharClipped(ZBuffer* pBuffer, uint8_t c, uint32_t nCol, int64_t nX, int64_t nY, ZRect* pClip)
{
	ZASSERT(pBuffer);
    if (c <= 32)        // no visible chars below this
        return;

    if (mCharDescriptors[c].nCharWidth == 0)   // generate glyph if not already generated
    {
        if (!GenerateGlyph(c))
            return;
    }

	ZRect rClip;
	if (pClip)
		rClip = *pClip;
	else
		rClip = pBuffer->GetArea();

	int64_t nDestX = nX;
	int64_t nDestY = nY;

	int64_t nDestStride = pBuffer->GetArea().Width();
	uint32_t* pDest = (pBuffer->GetPixels()) + (nY * nDestStride) + nX;

	int64_t nCharWidth = mCharDescriptors[c].nCharWidth;
    uint8_t nDrawAlpha = nCol >> 24;


	int64_t nScanlineOffset = 0;
	for (tPixelDataList::iterator it = mCharDescriptors[c].pixelData.begin(); it != mCharDescriptors[c].pixelData.end(); it++)
	{
		int64_t nBright = *it;
		it++;
		int64_t nNumPixels = *it;

		if (nBright < 0x0f)
		{
			nScanlineOffset += nNumPixels;
			nDestX += nNumPixels;

			// While the next offset is past the end of this scanline.... advance the dest
			while (nScanlineOffset >= nCharWidth)
			{
				pDest += nDestStride;
				nNumPixels -= nCharWidth;
				nScanlineOffset -= nCharWidth;
				nDestX -= nCharWidth;
				nDestY++;
			}

			pDest += nNumPixels;
		}
		else
		{
			uint32_t nAlpha = (uint32_t) nBright;
			while (nNumPixels > 0)
			{
                if (rClip.PtInRect(nDestX, nDestY))
                {
                    if (nAlpha > 0xf0 && nDrawAlpha > 0xf0)
                        *pDest = nCol;
                    else
                        *pDest = COL::AlphaBlend_BlendAlpha(nCol, *pDest, nAlpha);
                }

				// Advance the destination, wrapping around if necessary
				pDest++;

				nScanlineOffset++;
				nDestX++;
				if (nScanlineOffset >= nCharWidth)
				{
					nScanlineOffset -= nCharWidth;
					nDestX -= nCharWidth;
					nDestY++;
					pDest += nDestStride - nCharWidth;
				}
				nNumPixels--;
			}
		}
	}
}


inline 
void ZFont::DrawCharGradient(ZBuffer* pBuffer, uint8_t c, std::vector<uint32_t>& gradient, int64_t nX, int64_t nY, ZRect* pClip)
{
	ZASSERT(pBuffer);
    if (c <= 32)        // no visible chars below this
        return;

    if (mCharDescriptors[c].nCharWidth == 0)   // generate glyph if not already generated
    {
        if (!GenerateGlyph(c))
            return;
    }

	ZRect rClip;
	if (pClip)
		rClip = *pClip;
	else
		rClip = pBuffer->GetArea();

	int64_t nDestX = nX;
	int64_t nDestY = nY;

	int64_t nDestStride = pBuffer->GetArea().Width();
	uint32_t* pDest = (pBuffer->GetPixels()) + (nY * nDestStride) + nX;

	int64_t nCharWidth = mCharDescriptors[c].nCharWidth;

	int64_t nScanlineOffset = 0;
	int64_t nScanLine = 0;
	for (tPixelDataList::iterator it = mCharDescriptors[c].pixelData.begin(); it != mCharDescriptors[c].pixelData.end(); it++)
	{
		int64_t nBright = *it;
		it++;
		int64_t nNumPixels = *it;

		if (nBright < 5)
		{
			nScanlineOffset += nNumPixels;
			nDestX += nNumPixels;

			// While the next offset is past the end of this scanline.... advance the dest
			while (nScanlineOffset >= nCharWidth)
			{
				pDest += nDestStride;
				nNumPixels -= nCharWidth;
				nScanLine++;
				nScanlineOffset -= nCharWidth;
				nDestX -= nCharWidth;
				nDestY++;
			}

			pDest += nNumPixels;
		}
		else
		{
			uint32_t nAlpha = (uint32_t) nBright;
			while (nNumPixels > 0)
			{
                if (rClip.PtInRect(nDestX, nDestY))
                    *pDest = COL::AlphaBlend_BlendAlpha(gradient[nScanLine], *pDest, nAlpha);

				// Advance the destination, wrapping around if necessary
				pDest++;

				nScanlineOffset++;
				nDestX++;
				if (nScanlineOffset >= nCharWidth)
				{
					nScanLine++;
					nScanlineOffset -= nCharWidth;
					nDestX -= nCharWidth;
					nDestY++;
					pDest += nDestStride - nCharWidth;
				}
				nNumPixels--;
			}
		}
	}
}


ZRect ZFont::StringRect(const std::string& sText)
{
    return ZRect(0, 0, StringWidth(sText), mFontHeight);
}

ZRect ZFont::StringRect(const std::string& sText, int64_t nLineWidth)
{
    ZRect r;
    int64_t lines = CalculateNumberOfLines(nLineWidth, (uint8_t*)sText.c_str(), sText.length());
    int64_t widest = CalculateWidestLine(nLineWidth, (uint8_t*)sText.c_str(), sText.length());
    return ZRect(0, 0, widest, mFontHeight*lines);
}


// This function helps format text by returning a rectangle where the text should be output
// It will not clip, however... That should be done by the caller if necessary
ZRect ZFont::Arrange(ZRect rArea, const std::string& sText, ZGUI::ePosition pos, int64_t h_padding, int64_t v_padding)
{
//	ZRect rText(0,0, StringWidth(sText), mFontHeight);
    ZRect rText(StringRect(sText, rArea.Width()));
    rText.Inflate(h_padding, v_padding);

    return ZGUI::Arrange(rText, rArea, pos, h_padding, v_padding);
}

bool ZFont::DrawTextParagraph( ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, ZGUI::Style* pStyle, ZRect* pClip)
{

    if (!pStyle)
        pStyle = &gStyleGeneralText;
    ZRect rTextLine(rAreaToDrawTo);
    if (pStyle->pos != ZGUI::C)
        rTextLine.Deflate(pStyle->pad.h, pStyle->pad.v);

    if (rTextLine.bottom <= rTextLine.top || rTextLine.right <= rTextLine.left)
        return false;

    int64_t nLines = 1;
    if (pStyle->wrap)
        nLines = CalculateNumberOfLines(rTextLine.Width(), (uint8_t*)sText.data(), sText.length());

    ZGUI::ePosition lineStyle = ZGUI::LB;

	// Calculate the top line 
	switch (pStyle->pos)
	{
	case ZGUI::LT:
	case ZGUI::CT:
	case ZGUI::RT:
        rTextLine.top = rAreaToDrawTo.top + pStyle->pad.v;
		break;
	case ZGUI::LC:
	case ZGUI::C:
    case ZGUI::F:
    case ZGUI::RC:
		rTextLine.top = (rAreaToDrawTo.top + rAreaToDrawTo.bottom - (mFontHeight * nLines))/2;
		break;
	case ZGUI::LB:
	case ZGUI::CB:
	case ZGUI::RB:
		rTextLine.top = rAreaToDrawTo.bottom - (mFontHeight * nLines) - pStyle->pad.v;
		break;
	}

	// Which individual line style to use
	switch (pStyle->pos)
	{
	case ZGUI::CT:
	case ZGUI::C:
    case ZGUI::F:
    case ZGUI::CB:
		lineStyle = ZGUI::CB;
		break;
	case ZGUI::RT:
	case ZGUI::RC:
	case ZGUI::RB:
		lineStyle = ZGUI::RB;
	}

	rTextLine.bottom = rTextLine.top + mFontHeight;

	int64_t nCharsDrawn = 0;
    int64_t nLinesDrawn = 0;

    const char* pChars = sText.data();
    const char* pEnd = pChars + sText.length();

    while (pChars < pEnd && rTextLine.top < rTextLine.bottom && nLinesDrawn < nLines)
	{
        int64_t nLettersToDraw = CalculateWordsThatFitInWidth(rTextLine.Width(), (uint8_t*) pChars, pEnd-pChars);
       
        ZRect rAdjustedLine(Arrange(rTextLine, string(pChars, nLettersToDraw), lineStyle));
        if (rAdjustedLine.bottom > rAreaToDrawTo.top && rAdjustedLine.top < rAreaToDrawTo.bottom)       // only draw if line is within rAreaToDrawTo
		    DrawText(pBuffer, string(pChars, nLettersToDraw), rAdjustedLine, &pStyle->look, pClip);

		nCharsDrawn += nLettersToDraw;

		rTextLine.Offset(0, mFontHeight);
		pChars += nLettersToDraw;
        nLinesDrawn++;
	}

	return nCharsDrawn == sText.length();
}


int64_t ZFont::CalculateNumberOfLines(int64_t nLineWidth, const uint8_t* pChars, size_t nNumChars)
{
	ZASSERT(pChars);

	int64_t nLines = 0;
    uint8_t* pEnd = (uint8_t*) pChars + nNumChars;
	while (pChars < pEnd)
	{
		int64_t nLettersToDraw = CalculateWordsThatFitInWidth(nLineWidth, pChars, pEnd-pChars);
		pChars += nLettersToDraw;
		nLines++;
	}

	return nLines;
}


int64_t ZFont::CalculateLettersThatFitOnLine(int64_t nLineWidth, const uint8_t* pChars, size_t nNumChars)
{
	size_t nChars = 0;
	int64_t nWidthSoFar = 0;

	while (nChars < nNumChars)
	{
        uint8_t c = *(pChars + nChars);
        if(c >= 0)
		    nWidthSoFar += (int64_t) mCharDescriptors[c].nCharWidth + 1 + GetSpaceBetweenChars(*pChars, *(pChars+1));

        if (nWidthSoFar > nLineWidth || c == '\n')
        {
            if (nChars == 0)    // needs to be a minimum of one, even clipped
                return 1;

            return nChars;
        }

		nChars++;
	}

	return nChars;
}

int64_t ZFont::CalculateWidestLine(int64_t nLineWidth, const uint8_t* pChars, size_t nNumChars)
{
    size_t nChars = 0;
    int64_t nWidest = 0;

    while (nChars < nNumChars)
    {
        int64_t nLineChars = CalculateWordsThatFitInWidth(nLineWidth, pChars, nNumChars-nChars);
        int64_t width = StringWidth(pChars, nLineChars);
        nWidest = std::max<int64_t>(nWidest, width);

        nChars += nLineChars;
        pChars += nLineChars;
    }

    return nWidest;
}



// Take into account line break character '\n'
int64_t ZFont::CalculateWordsThatFitInWidth(int64_t nLineWidth, const uint8_t* pChars, size_t nNumChars)
{
	ZASSERT(pChars);
	ZASSERT(nLineWidth > 0)

	int64_t nNumLettersThatFit = 0;
    uint8_t* pCurChar = (uint8_t*) pChars;
	bool bNextLineFound = false;

	bool bLoop = true;
	while (bLoop)
	{
		// Find the next word break... (or the end of the string)
		while (*pCurChar && *pCurChar != ' ' && (pCurChar - pChars) < (int64_t) nNumChars)
		{
			if (*pCurChar == '\n')
			{
				bNextLineFound = true;
				break;
			}

			pCurChar++;
		}

		if (*pCurChar == ' ')
			pCurChar++;

		// If it doesn't fit on the line.... return the previous number of letters that fit
		if (StringWidth(string((char*)pChars, (pCurChar - pChars))) > nLineWidth)
		{
			if (nNumLettersThatFit > 0)
				return nNumLettersThatFit;

			return CalculateLettersThatFitOnLine(nLineWidth, pChars, nNumChars);
		}

		// otherwise set the number of letters that fit... and look for the next word
		nNumLettersThatFit = pCurChar - pChars;

		if (bNextLineFound)
		{
			return nNumLettersThatFit + 1;     // pass the '\n'
		}

		if (nNumLettersThatFit == nNumChars || *pCurChar == NULL)
			return nNumChars;
	}

	return nNumChars;
}

void ZFont::BuildGradient(uint32_t nColor1, uint32_t nColor2, std::vector<uint32_t>& gradient)
{
    int64_t height = mFontHeight;
    ZASSERT(height > 1);
    gradient.resize(height);
    if (gradient[0] == nColor1 && gradient[height - 1] == nColor2)
        return;
    gradient[0] = nColor1;
    gradient[mFontHeight - 1] = nColor2;
    for (int64_t i = 1; i < height - 1; i++)
    {
        double fRange = 10.0 * (((double)(i - 1) - (double)(height - 3) / 2.0) / (double)height);
        double fArcTanTransition = 0.5 + (atan(fRange) / 3.0);
        if (fArcTanTransition < -0)
            fArcTanTransition = 0;
        else if (fArcTanTransition > 1.0)
            fArcTanTransition = 1.0;
        int64_t nInverseAlpha = (int64_t)(255.0 * fArcTanTransition);
        int64_t nAlpha = 255 - nInverseAlpha;
        gradient[i] = ARGB(
            (uint8_t)(((ARGB_A(nColor1) * nAlpha + ARGB_A(nColor2) * nInverseAlpha)) >> 8),
            (uint8_t)(((ARGB_R(nColor1) * nAlpha + ARGB_R(nColor2) * nInverseAlpha)) >> 8),
            (uint8_t)(((ARGB_G(nColor1) * nAlpha + ARGB_G(nColor2) * nInverseAlpha)) >> 8),
            (uint8_t)(((ARGB_B(nColor1) * nAlpha + ARGB_B(nColor2) * nInverseAlpha)) >> 8)
        );
    }
}


void ZFont::FindKerning(uint8_t c1, uint8_t c2)
{
    // if either char is a number do not kern
    if ((c1 >= '0' && c1 <= '9') || 
        (c2 >= '0' && c2 <= '9') || 
        c1 == '_' || c2 == '_' || 
        c1 == '\'' || c2 == '\'' ||
        c1 == ' ' || c2 == ' '
        )
    {
        mCharDescriptors[c1].kerningArray[c2] = 0;
        return;
    }

    std::vector<int> C1rightmostPixel;
    std::vector<int> C2leftmostPixel;

    int64_t height = mFontHeight;
    C1rightmostPixel.resize(height);
    C2leftmostPixel.resize(height);


    // Compute rightmost pixel of c1
    int64_t nCharWidth = mCharDescriptors[c1].nCharWidth;
   
    int64_t nScanlineOffset = 0;
    int64_t nScanline = 0;
    for (tPixelDataList::iterator it = mCharDescriptors[c1].pixelData.begin(); it != mCharDescriptors[c1].pixelData.end(); it++)
    {
        int64_t nBright = *it;
        it++;
        int64_t nNumPixels = *it;

        if (nBright < 5 && nScanline < height)
        {
            nScanlineOffset += nNumPixels;

            // While the next offset is past the end of this scanline.... advance the dest
            while (nScanlineOffset >= nCharWidth)
            {
                nNumPixels -= nCharWidth;
                nScanlineOffset -= nCharWidth;
                nScanline++;
            }
        }
        else
        {
            while (nNumPixels > 0 && nScanline < height)
            {
                // Advance the destination, wrapping around if necessary
                nScanlineOffset++;
                C1rightmostPixel[nScanline] = (int32_t) nScanlineOffset;
                if (nScanlineOffset >= nCharWidth)
                {
                    nScanlineOffset -= nCharWidth;
                    nScanline++;
                }
                nNumPixels--;
            }
        }
    }



    // Compute leftmost pixel of c2
    nCharWidth = mCharDescriptors[c2].nCharWidth;

    nScanlineOffset = 0;
    nScanline = 0;
    for (tPixelDataList::iterator it = mCharDescriptors[c2].pixelData.begin(); it != mCharDescriptors[c2].pixelData.end(); it++)
    {
        int64_t nBright = *it;
        it++;
        int64_t nNumPixels = *it;

        if (nBright < 5 && nScanline < height)
        {
            nScanlineOffset += nNumPixels;

            // While the next offset is past the end of this scanline.... advance the dest
            while (nScanlineOffset >= nCharWidth)
            {
                nNumPixels -= nCharWidth;
                nScanlineOffset -= nCharWidth;
                nScanline++;
            }
        }
        else
        {
            while (nNumPixels > 0 && nScanline < height)
            {
                // Advance the destination, wrapping around if necessary
                nScanlineOffset++;
                if (C2leftmostPixel[nScanline] == 0)                // for each scanline, as soon as we see a colored pixel we set it and that's all for this scanline
                    C2leftmostPixel[nScanline] = (int32_t) nScanlineOffset;

                if (nScanlineOffset >= nCharWidth)
                {
                    nScanlineOffset -= nCharWidth;
                    nScanline++;
                }
                nNumPixels--;
            }
        }
    }






    int nAvailableKern = -mCharDescriptors[c1].nCharWidth;
    for (int i = 0; i < height; i++)
    {
        if (C1rightmostPixel[i] > 0 && C2leftmostPixel[i] > 0)      // only consider for scanlines where both chars have data
        {
            int nC1RightGap = mCharDescriptors[c1].nCharWidth - C1rightmostPixel[i];
            int nC2LeftGap = C2leftmostPixel[i];

            if (-(nC1RightGap + nC2LeftGap) > nAvailableKern)
                nAvailableKern = -(nC1RightGap + nC2LeftGap);

//            ZOUT("scanline:%d c1rightmost:%d       c2leftmost:%d    kern:%d\n", i, C1rightmostPixel[i], C2leftmostPixel[i], -(nC1RightGap + nC2LeftGap));
        }
    }

    mCharDescriptors[c1].kerningArray[c2] = nAvailableKern;
}


#ifdef _WIN64

std::vector<string> gWindowsFontFacenames;


bool ZFont::Init(const ZFontParams& params)
{
    if (mbInitted)
    {
        ZDEBUG_OUT("Font already initted. Cannot be re-initted\n");
        return false;
    }

    if (params.sFacename.empty())
        return false;

    if (params.Height() < 4)
        return false;


    mFontParams = params;
    mFontHeight = params.Height();

#ifdef _WIN64
    rGDIScratchArea.Set(0,0,mFontParams.Height() * 3, mFontHeight);
    mExtractBuffer.Init(rGDIScratchArea.right, rGDIScratchArea.bottom);

    if (UseDoubleSizeFont())
    {
        rGDIScratchArea.right *= 2;
        rGDIScratchArea.bottom *= 2;
    }



    mDIBInfo.bmiHeader.biBitCount = 32;
    mDIBInfo.bmiHeader.biClrImportant = 0;
    mDIBInfo.bmiHeader.biClrUsed = 0;
    mDIBInfo.bmiHeader.biCompression = 0;
    mDIBInfo.bmiHeader.biWidth = (LONG)rGDIScratchArea.right;
    mDIBInfo.bmiHeader.biHeight = (LONG) -rGDIScratchArea.bottom;
    mDIBInfo.bmiHeader.biPlanes = 1;
    mDIBInfo.bmiHeader.biSize = 40;
    mDIBInfo.bmiHeader.biSizeImage = (DWORD)rGDIScratchArea.right * (DWORD)rGDIScratchArea.bottom * 4;
    mDIBInfo.bmiHeader.biXPelsPerMeter = 2835;
    mDIBInfo.bmiHeader.biYPelsPerMeter = 2835;
    mDIBInfo.bmiColors[0].rgbBlue = 0;
    mDIBInfo.bmiColors[0].rgbGreen = 0;
    mDIBInfo.bmiColors[0].rgbRed = 0;
    mDIBInfo.bmiColors[0].rgbReserved = 0;

    DWORD pitch = DEFAULT_PITCH;
    if (mFontParams.nFixedWidth > 0 || mFontParams.bSymbolic)
    {
        pitch = FIXED_PITCH;
        mbEnableKerning = false;
    }

    DWORD nCharSet = ANSI_CHARSET;
    if (mFontParams.sFacename.find("Wingdings") != string::npos)
        nCharSet = SYMBOL_CHARSET;

    mWinHDC = GetDC(ghWnd);
    assert(mpBits == nullptr);
    mhWinTargetBitmap = CreateDIBSection(mWinHDC, &mDIBInfo, DIB_RGB_COLORS, (void**)&mpBits, NULL, 0);

    if (mpBits == nullptr)
    {
        HRESULT hr = GetLastError();
        cout << "hr: " << hr << "\n";
    }


    int createFontHeight = (int)mFontHeight;
    if (UseDoubleSizeFont())
        createFontHeight *= 2;

    mhWinTargetDC = CreateCompatibleDC(mWinHDC);
    mhWinFont = CreateFontA( 
        (int)createFontHeight,
        0,                      /*nWidth*/
        0,                      /*nEscapement*/
        0,                      /*nOrientation*/
        (int) mFontParams.nWeight, 
        mFontParams.bItalic, 
        0, 
        0, 
        nCharSet,           /*nCharSet*/
        OUT_DEVICE_PRECIS,     /*nOutPrecision*/
        OUT_DEFAULT_PRECIS,     /*nClipPrecision*/
        CLEARTYPE_QUALITY,    /*nQuality*/
        pitch,          /*nPitchAndFamily*/
        mFontParams.sFacename.c_str());


    SelectObject(mhWinTargetDC, mhWinTargetBitmap);


    SelectFont(mhWinTargetDC, mhWinFont);
    GetTextMetricsA(mhWinTargetDC, &mWinTextMetrics);

    int64_t nStart = gTimer.GetUSSinceEpoch();
    RetrieveKerningPairs();

    FindWidestCharacterWidth();
#endif

    int64_t nEnd = gTimer.GetUSSinceEpoch();
    ZDEBUG_OUT("Pregeneration took:%dus\n", (int)nEnd - nStart);
    
    mbInitted = true;

    return true;
}

bool ZFont::FindCharExtents(ZRect& rOutExtents)
{
    ZRect rExtents(mExtractBuffer.GetArea());

    bool bFoundBottom = false;
    bool bFoundRight = false;
    bool bFoundLeft = false;

    // find bottom
    for (int64_t y = mExtractBuffer.GetArea().bottom - 1; y >= 0; y--)
    {
        for (int64_t x = 0; x < mExtractBuffer.GetArea().right; x++)
        {
            if (mExtractBuffer.GetPixel(x, y) != 0xFFFFFFFF)
            {
                rExtents.bottom = y;
                // break out of both loops
                y = -1;
                x = rExtents.right;
                bFoundBottom = true;
            }
        }
    }

    // find right
    for (int64_t x = mExtractBuffer.GetArea().right - 1; x >= 0; x--)
    {
        for (int64_t y = 0; y < mExtractBuffer.GetArea().bottom; y++)
        {
            if (mExtractBuffer.GetPixel(x, y) != 0xFFFFFFFF)
            {
                rExtents.right = x;
                // break out of both loops
                y = rExtents.bottom;
                x = -1;
                bFoundRight = true;
            }
        }
    }

    // find left
    for (int64_t x = 0; x < mExtractBuffer.GetArea().right; x++)
    {
        for (int64_t y = 0; y < mExtractBuffer.GetArea().bottom; y++)
        {
            uint32_t nCol = (uint32_t) * ((uint32_t*)mpBits + y * rExtents.right + x);
            if (mExtractBuffer.GetPixel(x, y) != 0xFFFFFFFF)
            {
                rExtents.left = x;
                // break out of both loops
                y = rExtents.bottom;
                x = rExtents.right;
                bFoundLeft = true;
            }
        }
    }

    rExtents.bottom++;

    rOutExtents = rExtents;
    return bFoundLeft && bFoundRight && bFoundBottom;
}


bool ZFont::ExtractChar(uint8_t c)
{
    if (c <= 0)
        return false;


    if (UseDoubleSizeFont())
    {
        mExtractBuffer.Fill(0);
        uint32_t* pSrc = (uint32_t*)mpBits;
        uint32_t* pDest = mExtractBuffer.mpPixels;
        for (int y = 0; y < rGDIScratchArea.bottom-1; y+=2)
        {
            for (int x = 0; x < rGDIScratchArea.right-1; x+=2)
            {
                uint32_t col1 = *(pSrc + y * rGDIScratchArea.Width() + x);
                uint32_t col2 = *(pSrc + y * rGDIScratchArea.Width() + x + 1);
                uint32_t col3 = *(pSrc + (y+1) * rGDIScratchArea.Width() + x);
                uint32_t col4 = *(pSrc + (y+1) * rGDIScratchArea.Width() + x + 1);

                uint32_t total = (col1 & 0xff) + (col2 & 0xff) + (col3 & 0xff) + (col4 & 0xff);
                uint32_t ave = total / 4;

                if (ave != 255)
                    int stophere = 5;

                *pDest++ = (0xff000000 | (ave << 16) | (ave << 8) | ave);
            }
        }
    }
    else
    {
        memcpy(mExtractBuffer.mpPixels, mpBits, mExtractBuffer.GetArea().Width() * mExtractBuffer.GetArea().Height() * 4);
    }


    ZRect rExtents;
    if (!FindCharExtents(rExtents))
    {
        return false;
    }

    // if we've encountered a wider char than we've seen before
    if (rExtents.right - rExtents.left > mnWidestCharacterWidth)
    {
        mnWidestCharacterWidth = (int32_t) (rExtents.right - rExtents.left);

        // If we have a fixed width font and we've encountered a wider character, adjust the font params?  <tbd if this is a bad idea>
        if (mFontParams.nFixedWidth > 0 && mFontParams.nFixedWidth < mnWidestCharacterWidth)
            mFontParams.nFixedWidth = mnWidestCharacterWidth;
    }

    if (mFontParams.nFixedWidth > 0 && rExtents.Width() > 0)   // for fixed width fonts
    {
        int64_t nCharWidth = rExtents.right - rExtents.left;
        int64_t nPadding = (mnWidestCharacterWidth - nCharWidth) / 2;

        rExtents.left -= nPadding;
        rExtents.right = rExtents.left + mnWidestCharacterWidth;
        ZASSERT(rExtents.left >= 0 && rExtents.right <= rGDIScratchArea.right);
    }
    else if (c >= '0' && c <= '9')  // otherwise numbers use fixed width based on their own widest char
    {
        if ((rExtents.right - rExtents.left) > mnWidestNumberWidth)
        {
            mnWidestNumberWidth = (int32_t)(rExtents.right - rExtents.left);
        }

        int64_t nNumWidth = rExtents.right - rExtents.left;
        int64_t nPadding = (mnWidestNumberWidth - nNumWidth) / 2;

        rExtents.left -= nPadding;
        rExtents.right = rExtents.left + mnWidestNumberWidth;
        ZASSERT(rExtents.left >= 0 && rExtents.right <= rGDIScratchArea.right);
    }

    
    mCharDescriptors[c].nCharWidth = (uint16_t) (rExtents.right - rExtents.left + 1);

    //ZDEBUG_OUT("\nExtracting %ld:'%c' width:%ld - extents(%ld,%ld,%ld,%ld) offsets:", c, c, mCharDescriptors[c].nCharWidth, rExtents.left, rExtents.top, rExtents.right, rExtents.bottom);


    uint8_t nPen = 0;
    uint8_t nOffset = 0;

    for (int64_t y = 0; y < rExtents.bottom; y++)
    {
        for (int64_t x = rExtents.left; x <= rExtents.right; x++)
        {
            uint32_t nCol = mExtractBuffer.GetPixel(x, y);
            uint8_t nBright = 255-nCol&0x000000ff;


            if (nPen != nBright)
            {
                mCharDescriptors[c].pixelData.push_back(nPen);
                mCharDescriptors[c].pixelData.push_back(nOffset);
                nOffset = 0;
                nPen = nBright;
            }

            nOffset++;

            // The offset may wrap around, (as we discovered with a 30 point Futura font with the lowercase 'm')
            // So if we reach the max, we write the current current pen and just keep counting
            if (nOffset == 255)
            {
                //            sprintf(buf, "found offset overrun for char:'%c'\n", c);
                //            OutputDebugString(buf);

                mCharDescriptors[c].pixelData.push_back(nPen);
                mCharDescriptors[c].pixelData.push_back(nOffset);
                nOffset = 0;
            }
        }
    }

    if (nPen != 0)
    {
        mCharDescriptors[c].pixelData.push_back(nPen);
        mCharDescriptors[c].pixelData.push_back(nOffset);
    }

    return true;
}


void ZFont::FindWidestCharacterWidth()
{
    std::list<uint8_t> charsToTest = { 'W', 'M', '@', '_', '0', '2', '6', '7'};

    for (auto c : charsToTest)
    {
        GenerateGlyph(c);

        // need to clear the data for the glyph since data would have changed if a new widest char is found
        mCharDescriptors[c].nCharWidth = 0;
        mCharDescriptors[c].pixelData.clear();
    }
}

bool ZFont::GenerateGlyph(uint8_t c)
{
    if (c < 32)
        return true;

    const std::lock_guard<std::mutex> lock(mGenerateGlyph);

    if (mCharDescriptors[c].nCharWidth > 0)
        return true;


    DWORD overhang = (DWORD)mFontHeight; // indent by a significant amount to allow space for left overhang
    RECT r;
    r.left = overhang;
    r.top = 0;
    r.right = (LONG)rGDIScratchArea.right;
    r.bottom = (LONG)rGDIScratchArea.bottom;

    SetBkMode(mhWinTargetDC, TRANSPARENT);
    BOOL bReturn = BitBlt(mhWinTargetDC, 0, 0, (int)rGDIScratchArea.Width(), (int)rGDIScratchArea.Height(), NULL, 0, 0, WHITENESS);

    SelectFont(mhWinTargetDC, mhWinFont);
    ::DrawTextA(mhWinTargetDC, (LPCSTR)&c, 1, &r, DT_TOP | DT_CENTER);

    if (c == ' ')
    {
        mCharDescriptors[c].nCharWidth = (uint16_t) (mFontHeight / 4);  // 25% of the height seems good
    }
    else
    {
        ExtractChar(c);
    }

    return true;
}

bool ZFont::RetrieveKerningPairs()
{
    if (mFontParams.nFixedWidth > 0)
        return false;

    SelectFont(mhWinTargetDC, mhWinFont);
    SetMapMode(mhWinTargetDC, MM_TEXT);
    DWORD nPairs = GetKerningPairs(mhWinTargetDC, 0, 0);   // how many pairs required?
    if (nPairs == 0)
        return false;

//    ZDEBUG_OUT("Found ", nPairs, " pairs for font:", mFontParams.sFacename.c_str());

    KERNINGPAIR* pKerningPairArray = new KERNINGPAIR[nPairs];

    DWORD nResult = GetKerningPairs(mhWinTargetDC, nPairs, pKerningPairArray);



    if (nPairs > 0)
    {
        // If pairs were retrieved, initialize all to 0
        for (int first = 0; first < kMaxChars; first++)
        {
            for (int second = 0; second < kMaxChars; second++)
            {
                mCharDescriptors[first].kerningArray[second] = 0;
            }
        }
    }

    for (int64_t i = 0; i < nPairs; i++)
    {
        uint8_t cFirst = (uint8_t) pKerningPairArray[i].wFirst;
        uint8_t cSecond = (uint8_t) pKerningPairArray[i].wSecond;
        int16_t nKern = pKerningPairArray[i].iKernAmount;

        if (cFirst > 0 && cSecond > 0)
        {
            mCharDescriptors[cFirst].kerningArray[cSecond] = nKern;
        }
    }

    delete[] pKerningPairArray;

    return true;
}




int CALLBACK EnumFontFamProc(const LOGFONT* lpelfe, const TEXTMETRIC* lpntme, DWORD FontType, LPARAM lParam)
{
    gWindowsFontFacenames.push_back(string(lpelfe->lfFaceName) + "\0");
    return 1;
}


#endif //_WIN64


ZFontSystem::ZFontSystem()
{
    mbCachingEnabled = true;
}

ZFontSystem::~ZFontSystem()
{
    Shutdown();
}

bool ZFontSystem::Init()
{
#ifdef _WIN64
    gWindowsFontFacenames.clear();
    EnumFontFamilies(GetDC(ghWnd), 0, EnumFontFamProc, 0);

    ::sort(gWindowsFontFacenames.begin(), gWindowsFontFacenames.end());
#endif

    return true;
}

void ZFontSystem::Shutdown()
{
#ifdef _WIN64
    gWindowsFontFacenames.clear();
#endif
    mHeightToFontMap.clear();
}

bool ZFontSystem::SetDefaultFont(const ZFontParams& params)
{
    mpDefault = CreateFont(params);
    return true;
}

size_t ZFontSystem::GetFontCount()
{
    size_t count = 0;
    for (auto& sizeMap : mHeightToFontMap)
        count += sizeMap.second.size();
    return count;
}

bool ZFontSystem::SetCacheFolder(const std::string& sFolderPath)
{
    if (!std::filesystem::exists(sFolderPath))
    {
        //ZDEBUG_OUT("Font Cache Folder:%s Doesn't exist!\n", sFolderPath.c_str());
        return false;
    }

    msCacheFolder = sFolderPath;
    return true;
}

bool ZFontSystem::IsCached(const ZFontParams& params)
{
    if (msCacheFolder.empty())
        return false;

    return std::filesystem::exists(FontCacheFilename(params));
}

string ZFontSystem::FontCacheFilename(const ZFontParams& params)
{
    std::filesystem::path cachePath(msCacheFolder);

    string sFilename;
    Sprintf(sFilename, "%s_%d_%d_%d", params.sFacename.c_str(), params.Height(), params.nWeight, params.nTracking);
    if (params.bItalic)
        sFilename += "_i";
    if (params.nFixedWidth > 0)
        sFilename += "_f";
    sFilename += ".zfont";

    cachePath.append(sFilename);

    return cachePath.string();
}


tZFontPtr ZFontSystem::LoadFont(const string& sFilename)
{
    tZFontPtr pNewFont(new ZFont());
    if (!pNewFont->LoadFont(sFilename))
    {
        return nullptr;
    }
    ZFontParams fp(pNewFont->GetFontParams());
    assert(fp.nScalePoints > 0);
    int64_t height = fp.Height();

    tZFontMap& fontMap = mHeightToFontMap[height];

    fontMap[fp] = pNewFont;
    assert(fp.nScalePoints > 0);
    return pNewFont;
}

#ifdef _WIN64
tZFontPtr ZFontSystem::CreateFont(ZFontParams params)
{
    ZASSERT(!params.sFacename.empty());

    if (mbCachingEnabled)
    {
//        ZDEBUG_OUT("ZFontSystem CACHING ENABLED\n");
        if (IsCached(params))
        {
            assert(params.nScalePoints > 0);
            tZFontPtr pLoaded = LoadFont(FontCacheFilename(params));
            assert(params.nScalePoints > 0);
            if (pLoaded)
            {
                assert(params.nScalePoints > 0);
                if (pLoaded->Height() == params.Height())
                {
                    int64_t height = pLoaded->Height();
                    const std::lock_guard<std::recursive_mutex> lock(mFontMapMutex);
                    mHeightToFontMap[height][pLoaded->GetFontParams()] = pLoaded;
                    assert(pLoaded);
                    return pLoaded;
                }
            }

            ZDEBUG_OUT("WARNING: cached font failed to load...re-creating.");
        }
    }
    else
    {
//        ZDEBUG_OUT("ZFontSystem CACHING ENABLED - Creating font\n");
    }

    ZFont* pNewFont = new ZFont();
    if (!pNewFont->Init(params))
    {
        ZASSERT(false);
        return nullptr;
    }

    ZFontParams fp(pNewFont->GetFontParams());
//    ZDEBUG_OUT("Created font:%s scale:%.3f size:%d\n", fp.sFacename, fp.fScale, pNewFont->Height());

    const std::lock_guard<std::recursive_mutex> lock(mFontMapMutex);
    mHeightToFontMap[fp.Height()][fp].reset(pNewFont);


    if (mbCachingEnabled)
    {
//        ZDEBUG_OUT("ZFontSystem CACHING ENABLED\n");
        if (!msCacheFolder.empty())
        {
            if (!params.bSymbolic) // no caching symbolic fonts
            {
                //ZDEBUG_OUT("Saving font:%s to cache", fp.sFacename.c_str());
                pNewFont->SaveFont(FontCacheFilename(params));
            }
        }
    }
    else
    {
//        ZDEBUG_OUT("ZFontSystem CACHING DISABLED\n");
    }

    return mHeightToFontMap[fp.Height()][fp];
}
#endif


tZFontPtr ZFontSystem::GetFont(const std::string& sFontName, int32_t nHeight)
{
    const std::lock_guard<std::recursive_mutex> lock(mFontMapMutex);

    for (auto& findIt : mHeightToFontMap[nHeight])
    {
        const ZFontParams& fp = findIt.first;
        tZFontPtr pFont = findIt.second;
        if (fp.sFacename == sFontName)
            return pFont;
    }

    // Not found, create one
    return CreateFont(ZFontParams(sFontName, ZFontParams::ScalePoints(nHeight)));
}

tZFontPtr ZFontSystem::GetFont(const ZFontParams& params)
{
    if (params.sFacename.empty() || params.nScalePoints == 0)
    {
        ZDEBUG_OUT("Error with font params\n");
        return nullptr;
    }

    int64_t height = params.Height();
    assert(params.nScalePoints > 0 && params.nScalePoints < 500000);

    const std::lock_guard<std::recursive_mutex> lock(mFontMapMutex);
    auto findIt = mHeightToFontMap[height].find(params);

    // If not found
    if (findIt == mHeightToFontMap[height].end())
    {
        return CreateFont(params);
    }

    return (*findIt).second;
}
