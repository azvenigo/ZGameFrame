#include "ZFont.h"
#include <math.h>
#include "ZStringHelpers.h"
#include "ZTimer.h"
#include <fstream>
#include <ZMemBuffer.h>
#include "ZZipAPI.h"



ZFontParams::ZFontParams()
{
    nHeight = 0;
    nWeight = 0;
    nTracking = 1;
    bItalic = false;
    bUnderline = false;
    bStrikeOut = false;
}

ZFontParams::ZFontParams(const ZFontParams& rhs)
{
    nHeight = rhs.nHeight;
    nWeight = rhs.nWeight;
    nTracking = rhs.nTracking;
    bItalic = rhs.bItalic;
    bUnderline = rhs.bUnderline;
    bStrikeOut = rhs.bStrikeOut;
    sFacename = rhs.sFacename;
}


#ifdef _WIN64
extern HWND ghWnd;
#endif
extern ZTimer				gTimer;

#pragma pack (1)

string sZFontHeader("ZFONT v3.0 by Alex Zvenigorodsky");

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ZFont::ZFont()
{
	mbInitted = false;
    mbEnableKerning = true;
}

ZFont::~ZFont()
{
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

	ZASSERT(nFileSize < 10000000);	// 10 meg font?  really?

#ifdef COMPRESSED_FONTS

    ZMemBufferPtr compressedBuffer( new ZMemBuffer(nFileSize));
    fontFile.read((char*)compressedBuffer->data(), nFileSize);
    assert(fontFile.gcount() == nFileSize);
    compressedBuffer->seekp(nFileSize);

    ZMemBufferPtr uncompressedBuffer(new ZMemBuffer);
    ZZipAPI::Decompress(compressedBuffer, uncompressedBuffer, true);
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

    memcpy(&mFontParams.nHeight, pData, sizeof(mFontParams.nHeight));
    pData += sizeof(mFontParams.nHeight);

    memcpy(&mFontParams.nWeight, pData,  sizeof(mFontParams.nWeight));
    pData += sizeof(mFontParams.nWeight);

    memcpy(&mFontParams.nTracking, pData, sizeof(mFontParams.nTracking));
    pData += sizeof(mFontParams.nTracking);

    memcpy(&mFontParams.bItalic, pData, sizeof(mFontParams.bItalic));
    pData += sizeof(mFontParams.bItalic);

    memcpy(&mFontParams.bStrikeOut, pData, sizeof(mFontParams.bStrikeOut));
    pData += sizeof(mFontParams.bStrikeOut);

	mColorGradient.resize(mFontParams.nHeight);

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

    ZOUT("Loaded %s in %lldus.\n", sFilename.c_str(), gTimer.GetUSSinceEpoch()-nStartTime);

    mbInitted = true;
	return true;
}

bool ZFont::SaveFont(const string& sFilename)
{
    std::ofstream fontFile(sFilename.c_str(), ios::out | ios::binary | ios::trunc);

    if (!fontFile.is_open())
    {
        ZOUT("Failed to open file \"%s\"\n", sFilename.c_str());
        return false;
    }

//    uint8_t* pUncompressedFontData = new uint8_t[nUncompressedBytes];
    ZMemBufferPtr uncompBuffer( new ZMemBuffer());

    uncompBuffer->write((uint8_t*) sZFontHeader.data(), (uint32_t) sZFontHeader.length());
    uncompBuffer->write((uint8_t*) &mFontParams.nHeight, sizeof(mFontParams.nHeight));
    uncompBuffer->write((uint8_t*)&mFontParams.nWeight, sizeof(mFontParams.nWeight));
    uncompBuffer->write((uint8_t*)&mFontParams.nTracking, sizeof(mFontParams.nTracking));
    uncompBuffer->write((uint8_t*)&mFontParams.bItalic, sizeof(mFontParams.bItalic));
    uncompBuffer->write((uint8_t*)&mFontParams.bStrikeOut, sizeof(mFontParams.bStrikeOut));

    // sFacename length
    int16_t nFacenameLength = (int16_t) mFontParams.sFacename.length();
    uncompBuffer->write((uint8_t*)&nFacenameLength, sizeof(int16_t));
    uncompBuffer->write((uint8_t*)mFontParams.sFacename.data(), nFacenameLength);

    for (int32_t i = 0; i < kMaxChars; i++)
    {
        sCharDescriptor& fontChar = mCharDescriptors[i];
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

int32_t ZFont::GetSpaceBetweenChars(char c1, char c2)
{
    if (mbEnableKerning)
    {
        if (c1 > 0 && c2 > 0)
            return mCharDescriptors[c1].kerningArray[c2] + (int32_t) mFontParams.nTracking;
    }

    return (int32_t) mFontParams.nTracking;
}

int64_t ZFont::StringWidth(const string& sText)
{
	ZASSERT(mbInitted);

    if (sText.empty())
        return 0;

	int64_t nWidth = 0;
    int32_t nLength = (int32_t) sText.length();
    const char* pChar;
    for (pChar = sText.c_str(); pChar < sText.data()+nLength-1; pChar++)   // add up all of the chars except last one
    {
        nWidth += mCharDescriptors[*pChar].nCharWidth + 1 + GetSpaceBetweenChars(*pChar, *(pChar+1)); // char width adjusted by kerning with next char
    }

    if (*pChar)
        nWidth += mCharDescriptors[*pChar].nCharWidth;  // last char

	return nWidth;
}

bool ZFont::DrawText( ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol /*= 0xff000000*/, uint32_t nCol2 /*= 0xff000000*/, eStyle style /*= kNormal*/, ZRect* pClip /*= NULL*/ )
{
	ZASSERT(mbInitted);
	ZASSERT(pBuffer);

	if (!pBuffer || !mbInitted )
		return false;

	switch (style)
	{
	case kNormal:
		if (nCol == nCol2 && ARGB_A(nCol) == 0xff)
			return DrawText_Helper(pBuffer, sText, rAreaToDrawTo, nCol, pClip);
		return DrawText_Gradient_Helper(pBuffer, sText, rAreaToDrawTo, nCol, nCol2, pClip);
		break;

	case kShadowed:
		{
			uint32_t nShadowColor = nCol & 0xff000000;		// draw the shadow, but only as dark as the text alpha
			int64_t nShadowOffset = mFontParams.nHeight/20;
            if (nShadowOffset < 1)
                nShadowOffset = 1;


			ZRect rShadowRect(rAreaToDrawTo);
			rShadowRect.OffsetRect(nShadowOffset, nShadowOffset);
			if (nCol == nCol2)
			{
				DrawText_Helper(pBuffer, sText, rShadowRect, nShadowColor, pClip);
				return DrawText_Helper(pBuffer, sText, rAreaToDrawTo, nCol, pClip);
			};

//			uint32_t nShadow2Color = (nShadowColor & 0x00ffffff) | (nCol2 & 0xff000000);
			DrawText_Helper(pBuffer, sText, rShadowRect, nShadowColor, pClip);
			return DrawText_Gradient_Helper(pBuffer, sText, rAreaToDrawTo, nCol, nCol2, pClip);
		}
		break;

	case kEmbossed:
		{
			uint32_t nShadowColor = nCol & 0xff000000;		// draw the shadow, but only as dark as the text alpha
			uint32_t nHighlightColor = (nCol & 0xff000000) | 0x00ffffff;	// the highlight will also only be as bright as the text alpha

			int64_t nShadowOffset = mFontParams.nHeight/32;
            if (nShadowOffset < 1)
                nShadowOffset = 1;

			ZRect rShadowRect(rAreaToDrawTo);
			rShadowRect.OffsetRect(-nShadowOffset, -nShadowOffset);
	
			ZRect rHighlightRect(rAreaToDrawTo);
			rHighlightRect.OffsetRect(nShadowOffset, nShadowOffset);

			DrawText_Helper(pBuffer, sText, rShadowRect, nShadowColor, pClip);			// draw the shadow
			DrawText_Helper(pBuffer, sText, rHighlightRect, nHighlightColor, pClip);			// draw the highlight
			if (nCol == nCol2)
				return DrawText_Helper(pBuffer, sText, rAreaToDrawTo, nCol, pClip);

			return DrawText_Gradient_Helper(pBuffer, sText, rAreaToDrawTo, nCol, nCol2, pClip);
		}
		break;
	}

	return false;
}


inline bool ZFont::DrawText_Helper(ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol, ZRect* pClip)
{
	int64_t nX = rAreaToDrawTo.left;
	int64_t nY = rAreaToDrawTo.top;
	ZRect rOutputText(nX, nY, nX + StringWidth(sText), nY + mFontParams.nHeight);

	ZRect rClip;
	if (pClip)
		rClip.SetRect(*pClip);
	else
		rClip.SetRect(pBuffer->GetArea());

	// If the output rect is anywhere outside of the rAreaToDrawTo
	if (rOutputText.left < rClip.left || rOutputText.top < rClip.top || 
		rOutputText.right > rAreaToDrawTo.right || rOutputText.bottom > rAreaToDrawTo.bottom ||
		rOutputText.right > rClip.right || rOutputText.bottom > rClip.bottom)
	{
		ZRect rSrc(rAreaToDrawTo);
		ZRect rClipDest(rOutputText);
		pBuffer->Clip(rClip, rSrc, rClipDest);

		// Draw the text clipped
        const char* pChar = sText.data();
        for (;pChar < sText.data()+sText.length(); pChar++)
		{
			DrawCharClipped(pBuffer, *pChar, nCol, nX, nY, (ZRect*) &rClipDest);

			nX += mCharDescriptors[*pChar].nCharWidth + 1 + GetSpaceBetweenChars(*pChar, *(pChar+1));
            if (nX >= rAreaToDrawTo.right)
                break;
		}
	}
	else
	{
        const char* pChar = sText.data();
        for (; pChar < sText.data() + sText.length(); pChar++)
        {
			DrawCharNoClip(pBuffer, *pChar, nCol, nX, nY);

			nX += mCharDescriptors[*pChar].nCharWidth + 1 + GetSpaceBetweenChars(*pChar, *(pChar+1));
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
	ZRect rOutputText(nX, nY, nX + StringWidth(sText), nY + mFontParams.nHeight);

	ZRect rClip;
	if (pClip)
		rClip.SetRect(*pClip);
	else
		rClip.SetRect(pBuffer->GetArea());

	ZRect rSrc(rAreaToDrawTo);
	ZRect rClipDest(rOutputText);
	pBuffer->Clip(rClip, rSrc, rClipDest);

	// IF the text is entirely clipped, exit early
	if (rClipDest.Width() < 1 || rClipDest.Height() < 1)
		return true;

	BuildGradient(nCol, nCol2);

	// Draw the text clipped
    const char* pChar = sText.data();
    for (; pChar < sText.data() + sText.length(); pChar++)
    {
		DrawCharGradient(pBuffer, *pChar, nX, nY, (ZRect*) &rClipDest);

		nX += mCharDescriptors[*pChar].nCharWidth + 1 + GetSpaceBetweenChars(*pChar, *(pChar+1));
		if (nX >= rAreaToDrawTo.right)
            break;
	}
	return true;
}


inline 
void ZFont::DrawCharNoClip(ZBuffer* pBuffer, char c, uint32_t nCol, int64_t nX, int64_t nY)
{
	ZASSERT(pBuffer);
    if (c <= 32)        // no visible chars below this
        return;

	int64_t nDestStride = pBuffer->GetArea().Width();
	uint32_t* pDest = (pBuffer->GetPixels()) + (nY * nDestStride) + nX;

	int64_t nCharWidth = mCharDescriptors[c].nCharWidth;

	int64_t nScanlineOffset = 0;
	for (tPixelDataList::iterator it = mCharDescriptors[c].pixelData.begin(); it != mCharDescriptors[c].pixelData.end(); it++)
	{
		int64_t nBright = *it;
		it++;
		int64_t nNumPixels = *it;

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
                if (nAlpha > 0xf0)
                    *pDest = nCol;
                else
				    *pDest = pBuffer->AlphaBlend_AddAlpha(nCol, *pDest, nAlpha);

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
void ZFont::DrawCharClipped(ZBuffer* pBuffer, char c, uint32_t nCol, int64_t nX, int64_t nY, ZRect* pClip)
{
	ZASSERT(pBuffer);
    if (c <= 32)        // no visible chars below this
        return;

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
                    if (nAlpha > 0xf0)
                        *pDest = nCol;
                    else
                        *pDest = pBuffer->AlphaBlend_AddAlpha(nCol, *pDest, nAlpha);
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
void ZFont::DrawCharGradient(ZBuffer* pBuffer, char c, int64_t nX, int64_t nY, ZRect* pClip)
{
	ZASSERT(pBuffer);
    if (c <= 32)        // no visible chars below this
        return;

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
                    *pDest = pBuffer->AlphaBlend_AddAlpha(mColorGradient[nScanLine], *pDest, nAlpha);

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


// This function helps format text by returning a rectangle where the text should be output
// It will not clip, however... That should be done by the caller if necessary
ZRect ZFont::GetOutputRect(ZRect rArea, const char* pChars, size_t nNumChars, ePosition position, int64_t nPadding)
{
	ZRect rText(0,0, StringWidth(string(pChars,nNumChars)), mFontParams.nHeight);
	rArea.DeflateRect(nPadding, nPadding);

	int64_t nXCenter = rArea.left + (rArea.Width()  - rText.Width())/2;
	int64_t nYCenter = rArea.top  + (rArea.Height() - mFontParams.nHeight)/2;

	switch (position)
	{
	case kTopLeft:
		rText.OffsetRect(rArea.left, rArea.top);
		break;
	case kTopCenter:
		rText.OffsetRect(nXCenter, rArea.top);
		break;
	case kTopRight:
		rText.OffsetRect(rArea.right - rText.Width(), rArea.top);
		break;

	case kMiddleLeft:
		rText.OffsetRect(rArea.left, nYCenter);
		break;
	case kMiddleCenter:
		rText.OffsetRect(nXCenter, nYCenter);
		break;
	case kMiddleRight:
		rText.OffsetRect(rArea.right - rText.Width(), nYCenter);
		break;

	case kBottomLeft:
		rText.OffsetRect(rArea.left, rArea.bottom - mFontParams.nHeight);
		break;
	case kBottomCenter:
		rText.OffsetRect(nXCenter, rArea.bottom - mFontParams.nHeight);
		break;
	case kBottomRight:
		rText.OffsetRect(rArea.right - rText.Width(), rArea.bottom - mFontParams.nHeight);
		break;
	}

	return rText;
}

bool ZFont::DrawTextParagraph( ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol /*= 0xff000000*/, uint32_t nCol2 /*= 0xff000000*/, ePosition paragraphPosition /*= kTopLeft*/, eStyle style /*= kNormal*/, ZRect* pClip /*= NULL*/ )
{
	ZRect rTextLine(rAreaToDrawTo);

	int64_t nLines = CalculateNumberOfLines(rAreaToDrawTo.Width(), sText.data(), sText.length());
	ePosition lineStyle = kBottomLeft;

	// Calculate the top line 
	switch (paragraphPosition)
	{
	case kTopLeft:
	case kTopCenter:
	case kTopRight:
		break;
	case kMiddleLeft:
	case kMiddleCenter:
	case kMiddleRight:
		rTextLine.top = (rAreaToDrawTo.top + rAreaToDrawTo.bottom - (mFontParams.nHeight * nLines))/2;
		break;
	case kBottomLeft:
	case kBottomCenter:
	case kBottomRight:
		rTextLine.top = rAreaToDrawTo.bottom - (mFontParams.nHeight * nLines);
		break;
	}

	// Which individual line style to use
	switch (paragraphPosition)
	{
	case kTopCenter:
	case kMiddleCenter:
	case kBottomCenter:
		lineStyle = kBottomCenter;
		break;
	case kTopRight:
	case kMiddleRight:
	case kBottomRight:
		lineStyle = kBottomRight;
	}

	rTextLine.bottom = rTextLine.top + mFontParams.nHeight;

	int64_t nCharsDrawn = 0;

    const char* pChars = sText.data();
    const char* pEnd = pChars + sText.length();

    while (pChars < pEnd && rTextLine.top < rTextLine.bottom)
	{
        int64_t nLettersToDraw = CalculateWordsThatFitOnLine(rTextLine.Width(), pChars, pEnd-pChars);
       
        ZRect rAdjustedLine(GetOutputRect(rTextLine, pChars, nLettersToDraw, lineStyle));
        if (rAdjustedLine.bottom > rAreaToDrawTo.top && rAdjustedLine.top < rAreaToDrawTo.bottom)       // only draw if line is within rAreaToDrawTo
		    DrawText(pBuffer, string(pChars, nLettersToDraw), rAdjustedLine, nCol, nCol2, style, pClip);

		nCharsDrawn += nLettersToDraw;

		rTextLine.OffsetRect(0, mFontParams.nHeight);
		pChars += nLettersToDraw;
	}

	return nCharsDrawn == sText.length();
}

int64_t ZFont::CharWidth(char c) 
{ 
    if (c > 0)
        return mCharDescriptors[c].nCharWidth; 

    return 0;
}


int64_t ZFont::CalculateNumberOfLines(int64_t nLineWidth, const char* pChars, size_t nNumChars)
{
	ZASSERT(pChars);

	int64_t nLines = 0;
	char* pEnd = (char*) pChars + nNumChars;
	while (pChars < pEnd)
	{
		int64_t nLettersToDraw = CalculateWordsThatFitOnLine(nLineWidth, pChars, pEnd-pChars);
		pChars += nLettersToDraw;
		nLines++;
	}

	return nLines;
}


int64_t ZFont::CalculateLettersThatFitOnLine(int64_t nLineWidth, const char* pChars, size_t nNumChars)
{
	size_t nChars = 0;
	int64_t nWidthSoFar = 0;

	while (nChars < nNumChars)
	{
        char c = *(pChars + nChars);
        if(c >= 0)
		    nWidthSoFar += (int64_t) mCharDescriptors[c].nCharWidth + 1 + GetSpaceBetweenChars(*pChars, *(pChars+1));

        if (nWidthSoFar > nLineWidth)
        {
            if (nChars == 0)    // needs to be a minimum of one, even clipped
                return 1;

            return nChars;
        }

		nChars++;
	}

	return nChars;
}



// Take into account line break character '\n'
int64_t ZFont::CalculateWordsThatFitOnLine(int64_t nLineWidth, const char* pChars, size_t nNumChars)
{
	ZASSERT(pChars);
	ZASSERT(nLineWidth > 0)

	int64_t nNumLettersThatFit = 0;
	char* pCurChar = (char*) pChars;
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
		if (StringWidth(string(pChars, (pCurChar - pChars))) > nLineWidth)
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

void ZFont::BuildGradient(uint32_t nColor1, uint32_t nColor2)
{
	ZASSERT(mFontParams.nHeight > 1);
    mColorGradient.resize(mFontParams.nHeight);
//	CEASSERT(mColorGradient.size() == mFontParams.nHeight);

	// If the gradient is already set, don't bother
	if (mColorGradient[0] == nColor1 && mColorGradient[mFontParams.nHeight -1] == nColor2)
		return;

	//ZDEBUG_OUT("Building gradient for colors %x - %x mFontParams.nHeight:%d", nColor1, nColor2, mFontParams.nHeight);

	mColorGradient[0] = nColor1;
	mColorGradient[mFontParams.nHeight -1] = nColor2;

	for (int64_t i = 1; i < mFontParams.nHeight -1; i++)
	{
		double fRange = 10.0 * (( (double) (i-1) - (double) (mFontParams.nHeight -3) / 2.0 ) / (double)mFontParams.nHeight);
		double fArcTanTransition = 0.5 + (atan( fRange ) / 3.0);
		if (fArcTanTransition < -0)
			fArcTanTransition = 0;
		else if (fArcTanTransition > 1.0)
			fArcTanTransition = 1.0;

		int64_t nInverseAlpha = (int64_t) (255.0 * fArcTanTransition);
		int64_t nAlpha = 255 - nInverseAlpha;

/*		ZDEBUG_OUT("fRange:%f  fArcTan:%f nInverseAlpha:%d\n", fRange, fArcTanTransition, nInverseAlpha);

		for (int64_t j = 0; j < nInverseAlpha; j++)
		{
			ZDEBUG_OUT(".");
		}
		ZDEBUG_OUT("\n");*/

		mColorGradient[i] = ARGB(
			(uint8_t) (((ARGB_A(nColor1) * nAlpha + ARGB_A(nColor2) * nInverseAlpha)) >> 8),	
			(uint8_t)(((ARGB_R(nColor1) * nAlpha + ARGB_R(nColor2) * nInverseAlpha)) >> 8),
			(uint8_t)(((ARGB_G(nColor1) * nAlpha + ARGB_G(nColor2) * nInverseAlpha)) >> 8),
			(uint8_t)(((ARGB_B(nColor1) * nAlpha + ARGB_B(nColor2) * nInverseAlpha)) >> 8)
			);
	}
}

void ZFont::FindKerning(char c1, char c2)
{
    // if either char is a number do not kern
    if ((c1 >= '0' && c1 <= '9') || (c2 >= '0' && c2 <= '9') || c1 == '_' || c2 == '_' || c1 == '\'' || c2 == '\'')
    {
        mCharDescriptors[c1].kerningArray[c2] = 0;
        return;
    }

    std::vector<int> C1rightmostPixel;
    std::vector<int> C2leftmostPixel;

    C1rightmostPixel.resize(mFontParams.nHeight);
    C2leftmostPixel.resize(mFontParams.nHeight);


    // Compute rightmost pixel of c1
    int64_t nCharWidth = mCharDescriptors[c1].nCharWidth;
   
    int64_t nScanlineOffset = 0;
    int64_t nScanline = 0;
    for (tPixelDataList::iterator it = mCharDescriptors[c1].pixelData.begin(); it != mCharDescriptors[c1].pixelData.end(); it++)
    {
        int64_t nBright = *it;
        it++;
        int64_t nNumPixels = *it;

        if (nBright < 5 && nScanline < mFontParams.nHeight)
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
            while (nNumPixels > 0 && nScanline < mFontParams.nHeight)
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

        if (nBright < 5 && nScanline < mFontParams.nHeight)
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
            while (nNumPixels > 0 && nScanline < mFontParams.nHeight)
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
    for (int i = 0; i < mFontParams.nHeight; i++)
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




ZDynamicFont::ZDynamicFont()
{
    mhWinTargetDC = 0;
    mhWinTargetBitmap = 0;
    memset(&mDIBInfo, 0, sizeof(BITMAPINFO));
    mpBits = nullptr;
    mWinHDC = 0;
    mhWinFont = 0;
}

ZDynamicFont::~ZDynamicFont()
{
    DeleteObject(mhWinFont);
    mhWinFont = 0;

    DeleteObject(mhWinTargetDC);
    mhWinTargetDC = 0;

    DeleteObject(mhWinTargetBitmap);
    mhWinTargetBitmap = 0;
}


bool ZDynamicFont::Init(const ZFontParams& params)
{
    if (mbInitted)
    {
        ZDEBUG_OUT("Dynamic font already exists. Cannot be re-initted\n");
        return false;
    }

    mFontParams = params;

    mrScratchArea.SetRect(0, 0, mFontParams.nHeight * 5, mFontParams.nHeight * 2);     // may need to grow this if the font requested is ever needed to be larger


    mDIBInfo.bmiHeader.biBitCount = 32;
    mDIBInfo.bmiHeader.biClrImportant = 0;
    mDIBInfo.bmiHeader.biClrUsed = 0;
    mDIBInfo.bmiHeader.biCompression = 0;
    mDIBInfo.bmiHeader.biWidth = (LONG) mrScratchArea.Width();
    mDIBInfo.bmiHeader.biHeight = (LONG) -mrScratchArea.Height();
    mDIBInfo.bmiHeader.biPlanes = 1;
    mDIBInfo.bmiHeader.biSize = 40;
    mDIBInfo.bmiHeader.biSizeImage = (DWORD) mrScratchArea.Width() * (DWORD) mrScratchArea.Height() * 4;
    mDIBInfo.bmiHeader.biXPelsPerMeter = 3780;
    mDIBInfo.bmiHeader.biYPelsPerMeter = 3780;
    mDIBInfo.bmiColors[0].rgbBlue = 0;
    mDIBInfo.bmiColors[0].rgbGreen = 0;
    mDIBInfo.bmiColors[0].rgbRed = 0;
    mDIBInfo.bmiColors[0].rgbReserved = 0;


    //mWinHDC = GetDC(ghWnd);
    mhWinTargetBitmap = CreateDIBSection(mWinHDC, &mDIBInfo, DIB_RGB_COLORS, (void**)&mpBits, NULL, 0);

    mhWinTargetDC = CreateCompatibleDC(mWinHDC);

    mhWinFont = CreateFontA( 
        (int) mFontParams.nHeight, 
        0,                      /*nWidth*/
        0,                      /*nEscapement*/
        0,                      /*nOrientation*/
        (int) mFontParams.nWeight, 
        mFontParams.bItalic, 
        mFontParams.bUnderline, 
        mFontParams.bStrikeOut, 
        ANSI_CHARSET,           /*nCharSet*/
        OUT_DEVICE_PRECIS,     /*nOutPrecision*/
        OUT_DEFAULT_PRECIS,     /*nClipPrecision*/
        CLEARTYPE_QUALITY,    /*nQuality*/
        DEFAULT_PITCH,          /*nPitchAndFamily*/
        mFontParams.sFacename.c_str());


    int64_t nStart = gTimer.GetUSSinceEpoch();

    mnWidestNumberWidth = FindWidestNumberWidth();

    for (char c = 32; c <= 126; c++)        // 33 '!' through 126 '~' are the visible chars
        GenerateGlyph(c);

    if (!RetrieveKerningPairs())
    {
        // manually compute kerning pairs
        for (char c1 = 33; c1 <= 126; c1++)        // 33 '!' through 126 '~' are the visible chars
        {
            for (char c2 = 33; c2 <= 126; c2++)        // 33 '!' through 126 '~' are the visible chars
                FindKerning(c1, c2);
        }
    }


    int64_t nEnd = gTimer.GetUSSinceEpoch();
    ZDEBUG_OUT("Pregeneration took:%dus\n", (int)nEnd - nStart);
    
    mbInitted = true;

    return true;
}




/*typedef vector<unsigned char> tDataList;

struct sCEFontChar
{
    unsigned short  charWidth;
    tDataList       data;
};

sCEFontChar gCEFontArray[256];*/

ZRect ZDynamicFont::FindCharExtents()
{
    ZRect rExtents;// = { 0,0,0,0 };

    // find bottom
    for (int64_t y = mrScratchArea.bottom - 1; y >= 0; y--)
    {
        for (int64_t x = 0; x < mrScratchArea.right; x++)
        {

            uint32_t nCol = (uint32_t) *((uint32_t*) mpBits + y*mrScratchArea.right + x);
            if (nCol != 0xFFFFFFFF)
            {
                rExtents.bottom = y;
                // break out of both loops
                y = -1;
                x = mrScratchArea.right;
            }
        }
    }

    // find right
    for (int64_t x = mrScratchArea.right - 1; x >= 0; x--)
    {
        for (int64_t y = 0; y < mrScratchArea.bottom; y++)
        {
            uint32_t nCol = (uint32_t) * ((uint32_t*)mpBits + y * mrScratchArea.right + x);
            if (nCol != 0xFFFFFFFF)
            {
                rExtents.right = x;
                // break out of both loops
                y = mrScratchArea.bottom;
                x = -1;
            }
        }
    }

    // find left
    for (int64_t x = 0; x < mrScratchArea.right; x++)
    {
        for (int64_t y = 0; y < mrScratchArea.bottom; y++)
        {
            uint32_t nCol = (uint32_t) * ((uint32_t*)mpBits + y * mrScratchArea.right + x);
            if (nCol != 0xFFFFFFFF)
            {
                rExtents.left = x;
                // break out of both loops
                y = mrScratchArea.bottom;
                x = mrScratchArea.right;
            }
        }
    }



    //   rExtents.right++;
    rExtents.bottom++;

    return rExtents;
}


bool ZDynamicFont::ExtractChar(char c)
{
    // Special case
 /*   if (c < 'A' || c > 'Z')
    {
       gCEFontArray[c].charWidth = 0;
       return;
    }*/

    if (c <= 0)
        return false;

    ZRect rExtents = FindCharExtents();

//    if (rExtents.bottom - rExtents.top + 1 > mFontParams.nHeight)
//        mFontParams.nHeight = rExtents.bottom - rExtents.top + 1;

    // For numbers, use the width of the '0' character
    if (c >= '0' && c <= '9')
    {
        assert(mnWidestNumberWidth > 0);         // need to have this one already.....may need to look into this requirement
        int64_t nNumWidth = rExtents.right - rExtents.left;
        int64_t nPadding = (mnWidestNumberWidth - nNumWidth) / 2;

        rExtents.left = rExtents.left - nPadding;
        rExtents.right = rExtents.left + mnWidestNumberWidth;
    }

    
    mCharDescriptors[c].nCharWidth = (uint16_t) (rExtents.right - rExtents.left + 1);

    //ZDEBUG_OUT("\nExtracting %ld:'%c' width:%ld - extents(%ld,%ld,%ld,%ld) offsets:", c, c, mCharDescriptors[c].nCharWidth, rExtents.left, rExtents.top, rExtents.right, rExtents.bottom);


    unsigned char nPen = 0;
    unsigned char nOffset = 0;

    for (int64_t y = 0; y <= rExtents.bottom; y++)
    {
        for (int64_t x = rExtents.left; x <= rExtents.right; x++)
        {
//            DWORD nCol = GetPixel(mhWinTargetDC, x, y);
//            unsigned char nBright = 255 - GetGValue(nCol);
            uint32_t nCol = (uint32_t) *((uint32_t*)mpBits + y * mrScratchArea.right + x);
            unsigned char nBright = 255-nCol&0x000000ff;


            if (nPen != nBright)
            {
                //            sprintf(buf, " %ld,%ld", nBright, nOffset);
                //            OutputDebugString(buf);
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
        //      sprintf(buf, " %ld", nOffset);
        //      OutputDebugString(buf);
        mCharDescriptors[c].pixelData.push_back(nPen);
        mCharDescriptors[c].pixelData.push_back(nOffset);
    }

    return true;
}


bool ZDynamicFont::SaveFont(const string& sFilename)
{
    for (int32_t c = 1; c < kMaxChars; c++)
    {
        if (mCharDescriptors[c].nCharWidth == 0)
            GenerateGlyph(c);
    }

    return ZFont::SaveFont(sFilename);
}


int32_t ZDynamicFont::FindWidestNumberWidth()
{

    int32_t nWidest = 0;

    for (char c = '0'; c <= '9'; c++)
    {
        RECT r;
        r.left = 0;
        r.top = 0;
        r.right = (LONG) mrScratchArea.right;
        r.bottom = (LONG) mrScratchArea.bottom;

        SelectObject(mhWinTargetDC, mhWinTargetBitmap);
        SetBkMode(mhWinTargetDC, TRANSPARENT);
        BOOL bReturn = BitBlt(mhWinTargetDC, 0, 0, (int) mrScratchArea.Width(), (int) mrScratchArea.Height(), NULL, 0, 0, WHITENESS);

        SelectFont(mhWinTargetDC, mhWinFont);
        ::DrawTextA(mhWinTargetDC, (LPCSTR)&c, 1, &r, DT_TOP | DT_CENTER);

        ZRect rExtents = FindCharExtents();
        if (rExtents.Width() > nWidest)
            nWidest = (int) rExtents.Width();
    }


    return nWidest;
}


bool ZDynamicFont::GenerateGlyph(char c)
{
    if (c <= 0)
        return false;

    RECT r;
    r.left = 0;
    r.top = 0;
    r.right = (LONG) mrScratchArea.right;
    r.bottom = (LONG) mrScratchArea.bottom;

    SelectObject(mhWinTargetDC, mhWinTargetBitmap);
    SetBkMode(mhWinTargetDC, TRANSPARENT);
    BOOL bReturn = BitBlt(mhWinTargetDC, 0, 0, (int) mrScratchArea.Width(), (int) mrScratchArea.Height(), NULL, 0, 0, WHITENESS);

    SelectFont(mhWinTargetDC, mhWinFont);
    ::DrawTextA(mhWinTargetDC, (LPCSTR) &c, 1, &r, DT_TOP|DT_CENTER);

    if (c == ' ')
    {
        mCharDescriptors[c].nCharWidth = (uint16_t) (mFontParams.nHeight / 4);  // 25% of the height seems good
    }
    else
    {
        ExtractChar(c);
    }

    return true;
}

bool ZDynamicFont::RetrieveKerningPairs()
{
    SelectFont(mhWinTargetDC, mhWinFont);
    SetMapMode(mhWinTargetDC, MM_TEXT);
    DWORD nPairs = GetKerningPairs(mhWinTargetDC, 0, 0);   // how many pairs required?
    if (nPairs == 0)
        return false;

    ZDEBUG_OUT("Found %d pairs for font:%s", nPairs, mFontParams.sFacename.c_str());

    KERNINGPAIR* pKerningPairArray = new KERNINGPAIR[nPairs];

    DWORD nResult = GetKerningPairs(mhWinTargetDC, nPairs, pKerningPairArray);


    for (int64_t i = 0; i < nPairs; i++)
    {
        char cFirst = (char) pKerningPairArray[i].wFirst;
        char cSecond = (char) pKerningPairArray[i].wSecond;
        int16_t nKern = pKerningPairArray[i].iKernAmount-2;

        // only pairs in the ascii set 0 - 127
        if (cFirst > 0 && cSecond > 0)
        {

            mCharDescriptors[cFirst].kerningArray[cSecond] = nKern;
//            ZDEBUG_OUT("kerning %c:%c = %d\n", cFirst, cSecond, nKern);
            FlushDebugOutQueue();
        }
    }

    delete[] pKerningPairArray;

    return true;
}



void ZDynamicFont::DrawCharNoClip(ZBuffer* pBuffer, char c, uint32_t nCol, int64_t nX, int64_t nY)
{
    if (mCharDescriptors[c].nCharWidth == 0)   // generate glyph if not already generated
        GenerateGlyph(c);

    return ZFont::DrawCharNoClip(pBuffer, c, nCol, nX, nY);
}

void ZDynamicFont::DrawCharClipped(ZBuffer* pBuffer, char c, uint32_t nCol, int64_t nX, int64_t nY, ZRect* pClip)
{
    if (mCharDescriptors[c].nCharWidth == 0)   // generate glyph if not already generated
        GenerateGlyph(c);

    return ZFont::DrawCharClipped(pBuffer, c, nCol, nX, nY, pClip);
}

void ZDynamicFont::DrawCharGradient(ZBuffer* pBuffer, char c, int64_t nX, int64_t nY, ZRect* pClip)
{
    if (mCharDescriptors[c].nCharWidth == 0)   // generate glyph if not already generated
        GenerateGlyph(c);

    return ZFont::DrawCharGradient(pBuffer, c, nX, nY, pClip);
}


int CALLBACK EnumFontFamProc(const LOGFONT* lpelfe, const TEXTMETRIC* lpntme, DWORD FontType, LPARAM lParam)
{
    gWindowsFontFacenames.push_back(string(lpelfe->lfFaceName) + "\0");
    return 1;
}


#endif //_WIN64


ZFontSystem::ZFontSystem()
{
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
    mNameToFontMap.clear();
}

std::shared_ptr<ZFont> ZFontSystem::LoadFont(const string& sFilename)
{
    ZFont* pNewFont = new ZFont();
    if (!pNewFont->LoadFont(sFilename))
    {
        delete pNewFont;
        ZDEBUG_OUT("Failed to load font:%s\n", sFilename.c_str());
        return nullptr;
    }
    string name(pNewFont->GetFontParams()->sFacename);
    int32_t nHeight = (int32_t) pNewFont->GetFontParams()->nHeight;

    ZDEBUG_OUT("Loaded font:%s\n", name.c_str());

    mNameToFontMap[name][nHeight].reset(pNewFont);      // add it to the map by name and size

    return mNameToFontMap[name][nHeight];
}

#ifdef _WIN64
std::shared_ptr<ZFont> ZFontSystem::CreateFont(const ZFontParams& params)
{
    ZDynamicFont* pNewFont = new ZDynamicFont();
    pNewFont->Init(params);

    string name(pNewFont->GetFontParams()->sFacename);
    int32_t nHeight = (int32_t) pNewFont->GetFontParams()->nHeight;

    mNameToFontMap[name][nHeight].reset(pNewFont);      // add it to the map by name and size

//    mFonts.emplace_back(pNewFont);

    ZDEBUG_OUT("ZFontSystem::CreateFont size:%d, name:%s\n", params.nHeight, params.sFacename.c_str());

    return mNameToFontMap[name][nHeight];
}
#endif

std::shared_ptr<ZFont> ZFontSystem::GetDefaultFont(int32_t nIndex)
{
    if (nIndex < 0 || nIndex >= mNameToFontMap[msDefaultFontName].size())
    {
        assert(false);
        return nullptr;
    }

    tSizeToFont::iterator sizeIt = mNameToFontMap[msDefaultFontName].begin();
    while (nIndex-- > 0)
        sizeIt++;

    return (*sizeIt).second;
}

std::vector<string> ZFontSystem::GetFontNames()
{
    std::vector<string> names;

    for (auto name : mNameToFontMap)        
        names.emplace_back( name.first );

    return names;
}

std::vector<int32_t> ZFontSystem::GetAvailableSizes(const string& sFontName)
{
    std::vector<int32_t> sizes;

    auto it = mNameToFontMap.find(sFontName);
    if (it != mNameToFontMap.end())
    {
        ZDEBUG_OUT("Found font by name\"%s\" sizes", sFontName.c_str());

        tSizeToFont& sizeToFontMap = (*it).second;

        for (tSizeToFont::iterator fontSizeIterator = sizeToFontMap.begin(); fontSizeIterator != sizeToFontMap.end(); fontSizeIterator++)
        {
            ZDEBUG_OUT(":%d", (*fontSizeIterator).first);
            sizes.push_back((*fontSizeIterator).first);
        }
        ZDEBUG_OUT("\n");
    }
    
    return sizes;
}

std::shared_ptr<ZFont> ZFontSystem::GetDefaultFont(const string& sFontName, int32_t nFontSize)
{
    auto it = mNameToFontMap.find(sFontName);
    if (it != mNameToFontMap.end())
    {
        tSizeToFont& sizeToFontMap = (*it).second;
        auto it2 = sizeToFontMap.find(nFontSize);
        if (it2 != sizeToFontMap.end())
            return (*it2).second;
    }

    assert(false);
    return nullptr;
}

