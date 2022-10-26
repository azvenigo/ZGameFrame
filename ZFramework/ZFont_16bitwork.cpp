#include "ZFont.h"
#include <math.h>
#include "ZStringHelpers.h"
#include "ZTimer.h"
#include <fstream>


static uint8_t alphaArray[8] = { 0, 32, 64, 128, 160, 192, 224, 255 };

ZFontParams::ZFontParams()
{
    nHeight = 0;
    nWeight = 0;
    bItalic = false;
    bUnderline = false;
    bStrikeOut = false;
}

ZFontParams::ZFontParams(const ZFontParams& rhs)
{
    nHeight = rhs.nHeight;
    nWeight = rhs.nWeight;
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

string sZFontHeader("ZFONT v4.0 by Alex Zvenigorodsky");

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


bool ZFont::LoadFont(const string& sFilename)
{
    std::ifstream fontFile(sFilename.c_str(), ios::in | ios::binary | ios::ate);    // open and set pointer at end

    if (!fontFile.is_open())
	{
        ZOUT("Failed to open file \"%s\"\n", sFilename.c_str());
		return false;
	}

    streampos nNumToRead = fontFile.tellg();
    fontFile.seekg(0, ios::beg);

	CEASSERT(nNumToRead < 10000000);	// 10 meg font?  really?

	uint8_t* pBuf = new uint8_t[nNumToRead];
//	ReadFile(hFile, pBuf, nNumToRead, &nNumRead, NULL);
    fontFile.read((char*) pBuf, nNumToRead);

    assert(fontFile.gcount() == nNumToRead);

	uint8_t* pData = pBuf;

    int64_t nHeaderLength = sZFontHeader.length();
    if (memcmp(pData, sZFontHeader.data(), nHeaderLength) != 0)
	{
        assert(false);
		delete[] pBuf;
		return false;
	}

	pData += nHeaderLength;

    memcpy(&mFontParams.nHeight, pData, sizeof(mFontParams.nHeight));
    pData += sizeof(mFontParams.nHeight);

    memcpy(&mFontParams.nWeight, pData,  sizeof(mFontParams.nWeight));
    pData += sizeof(mFontParams.nWeight);

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

		uint16_t nPixelDataSize = *((uint16_t*) pData);
//        assert(nPixelDataSize%2 == 0);       // has to be even number of data
		pData += 2;

		mCharDescriptors[i].pixelData.resize(nPixelDataSize);
        memcpy(mCharDescriptors[i].pixelData.data(), pData, nPixelDataSize * sizeof(uint16_t));      // pixel data should be contiguous as a vector
        pData += nPixelDataSize;

        memcpy(mCharDescriptors[i].kerningArray, pData, kMaxChars);
        pData += kMaxChars;
	}

	delete[] pBuf;

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

//    int32_t nNumWritten = 0;

//    WriteFile(hFile, sZFontHeader.data(), sZFontHeader.length(), &nNumWritten, NULL);
    fontFile.write(sZFontHeader.data(), sZFontHeader.length());

//    WriteFile(hFile, &mFontParams.nHeight, sizeof(mFontParams.nHeight), &nNumWritten, NULL);
    fontFile.write((const char*) &mFontParams.nHeight, sizeof(mFontParams.nHeight));

//    WriteFile(hFile, &mFontParams.nWeight, sizeof(mFontParams.nWeight), &nNumWritten, NULL);
    fontFile.write((const char*) &mFontParams.nWeight, sizeof(mFontParams.nWeight));


//    WriteFile(hFile, &mFontParams.bItalic, sizeof(mFontParams.bItalic), &nNumWritten, NULL);
    fontFile.write((const char*)&mFontParams.bItalic, sizeof(mFontParams.bItalic));

//    WriteFile(hFile, &mFontParams.bStrikeOut, sizeof(mFontParams.bStrikeOut), &nNumWritten, NULL);
    fontFile.write((const char*)&mFontParams.bStrikeOut, sizeof(mFontParams.bStrikeOut));

    // sFacename length
    int16_t nFacenameLength = (int16_t) mFontParams.sFacename.length();
//    WriteFile(hFile, &nFacenameLength, sizeof(nFacenameLength), &nNumWritten, NULL);
    fontFile.write((const char*)&nFacenameLength, sizeof(nFacenameLength));

//    WriteFile(hFile, mFontParams.sFacename.data(), nFacenameLength, &nNumWritten, NULL);
    fontFile.write((const char*)mFontParams.sFacename.data(), nFacenameLength);


    for (int32_t i = 0; i < kMaxChars; i++)
    {
        // Write the char width (1 byte)
        sCharDescriptor& fontChar = mCharDescriptors[i];
//        ::WriteFile(hFile, &fontChar.nCharWidth, sizeof(uint16_t), &nNumWritten, NULL);
        fontFile.write((const char*)&fontChar.nCharWidth, sizeof(uint16_t));

        // Write out the number of pixel data   
        uint16_t nPixelDataSize = (uint16_t) mCharDescriptors[i].pixelData.size();
        assert(nPixelDataSize%2 == 0);
//        WriteFile(hFile, &nPixelDataSize, 2, &nNumWritten, NULL);
        fontFile.write((const char*)&nPixelDataSize, 2);

//        WriteFile(hFile, mCharDescriptors[i].pixelData.data(), nPixelDataSize, &nNumWritten, 0);
        fontFile.write((const char*)mCharDescriptors[i].pixelData.data(), nPixelDataSize*sizeof(uint16_t));

        // kerning data for this char
//        WriteFile(hFile, mCharDescriptors[i].kerningArray, kMaxChars, &nNumWritten, 0);
        fontFile.write((const char*) mCharDescriptors[i].kerningArray, kMaxChars);
    }

    return true;
}

int32_t ZFont::GetKerning(char c1, char c2)
{
    if (mbEnableKerning)
    {
        if (c1 > 0 && c2 > 0)
            return mCharDescriptors[c1].kerningArray[c2];
    }

    return 0;
}

int64_t ZFont::StringWidth(const string& sText)
{
	CEASSERT(mbInitted);

    if (sText.empty())
        return 0;

	int64_t nWidth = 0;
    size_t nLength = sText.length();
    const char* pChar;
    for (pChar = sText.c_str(); pChar < sText.data()+nLength-1; pChar++)   // add up all of the chars except last one
    {
        nWidth += mCharDescriptors[*pChar].nCharWidth + 1 + GetKerning(*pChar, *(pChar+1)); // char width adjusted by kerning with next char
    }

    if (*pChar)
        nWidth += mCharDescriptors[*pChar].nCharWidth;  // last char

	return nWidth;
}

bool ZFont::DrawText( ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol /*= 0xff000000*/, uint32_t nCol2 /*= 0xff000000*/, eStyle style /*= kNormal*/, ZRect* pClip /*= NULL*/ )
{
	CEASSERT(mbInitted);
	CEASSERT(pBuffer);

	if (!pBuffer || !mbInitted )
		return false;

	switch (style)
	{
	case kNormal:
		if (nCol == nCol2)
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

			nX += mCharDescriptors[*pChar].nCharWidth + 1 + GetKerning(*pChar, *(pChar+1));
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

			nX += mCharDescriptors[*pChar].nCharWidth + 1 + GetKerning(*pChar, *(pChar+1));
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

		nX += mCharDescriptors[*pChar].nCharWidth + 1 + GetKerning(*pChar, *(pChar+1));
		if (nX >= rAreaToDrawTo.right)
            break;
	}
	return true;
}


inline 
void ZFont::DrawCharNoClip(ZBuffer* pBuffer, char c, uint32_t nCol, int64_t nX, int64_t nY)
{
	CEASSERT(pBuffer);
    if (c <= 32)        // no visible chars below this
        return;

	int64_t nDestStride = pBuffer->GetArea().Width();
	uint32_t* pDest = (pBuffer->GetPixels()) + (nY * nDestStride) + nX;

	int64_t nCharWidth = mCharDescriptors[c].nCharWidth;

	int64_t nScanlineOffset = 0;
	for (tPixelDataList::iterator it = mCharDescriptors[c].pixelData.begin(); it != mCharDescriptors[c].pixelData.end(); it++)
	{
        uint8_t nBrightIndex = (*it >> 13);  // 3 bits, values 0-7
        assert(nBrightIndex < 8);

		uint32_t nAlpha = alphaArray[nBrightIndex];
		int64_t nNumPixels = (*it & 0X1FFF);     // rightmost 13 bits

		if (nAlpha == 0)
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
			while (nNumPixels > 0)
			{
                if (nAlpha == 0xff)
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
	CEASSERT(pBuffer);
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
        uint8_t nBrightIndex = (*it >> 13);  // 3 bits, values 0-7
        assert(nBrightIndex < 8);

        uint32_t nAlpha = alphaArray[nBrightIndex];
        int64_t nNumPixels = (*it & 0X1FFF);     // rightmost 13 bits

		if (nAlpha == 0)
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
	CEASSERT(pBuffer);
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
        uint8_t nBrightIndex = (*it >> 13);  // 3 bits, values 0-7
        assert(nBrightIndex < 8);

        uint32_t nAlpha = alphaArray[nBrightIndex];
        int64_t nNumPixels = (*it & 0X1FFF);     // rightmost 13 bits

		if (nAlpha == 0)
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
	CEASSERT(pChars);

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
		    nWidthSoFar += (int64_t) mCharDescriptors[c].nCharWidth + 1 + GetKerning(*pChars, *(pChars+1));

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
	CEASSERT(pChars);
	CEASSERT(nLineWidth > 0)

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
	CEASSERT(mFontParams.nHeight > 1);
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


    mWinHDC = GetDC(ghWnd);
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

    for (char c = 32; c <= 126; c++)        // 33 '!' through 126 '~' are the visible chars
        GenerateGlyph(c);


    RetrieveKerningPairs();


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
    for (int y = (int) mrScratchArea.bottom - 1; y >= 0; y--)
    {
        for (int x = 0; x < (int) mrScratchArea.right; x++)
        {

            uint32_t nCol = (uint32_t) *((uint32_t*) mpBits + y*mrScratchArea.right + x);
            if (nCol != 0xFFFFFFFF)
            {
                rExtents.bottom = y;
                // break out of both loops
                y = -1;
                x = (int) mrScratchArea.right;
            }
        }
    }

    // find right
    for (int x = (int) mrScratchArea.right - 1; x >= 0; x--)
    {
        for (int y = 0; y < (int) mrScratchArea.bottom; y++)
        {
            uint32_t nCol = (uint32_t) * ((uint32_t*)mpBits + y * mrScratchArea.right + x);
            if (nCol != 0xFFFFFFFF)
            {
                rExtents.right = x;
                // break out of both loops
                y = (int) mrScratchArea.bottom;
                x = -1;
            }
        }
    }

    // find left
    for (int x = 0; x < (int) mrScratchArea.right; x++)
    {
        for (int y = 0; y < (int) mrScratchArea.bottom; y++)
        {
            uint32_t nCol = (uint32_t) * ((uint32_t*)mpBits + y * mrScratchArea.right + x);
            if (nCol != 0xFFFFFFFF)
            {
                rExtents.left = x;
                // break out of both loops
                y = (int) mrScratchArea.bottom;
                x = (int) mrScratchArea.right;
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
    if (c >= '1' && c <= '9')
    {
        int nZeroWidth = mCharDescriptors['0'].nCharWidth;
        assert(nZeroWidth > 0);         // need to have this one already.....may need to look into this requirement
        int nNumWidth = (int) rExtents.right - (int) rExtents.left;

        int nPadding = (nZeroWidth - nNumWidth) / 2;

        //rExtents.left = rExtents.left - nPadding;
        rExtents.right = (int) rExtents.left + nZeroWidth;
    }

    
    mCharDescriptors[c].nCharWidth = (uint16_t) (rExtents.right - rExtents.left + 1);

    //ZDEBUG_OUT("\nExtracting %ld:'%c' width:%ld - extents(%ld,%ld,%ld,%ld) offsets:", c, c, mCharDescriptors[c].nCharWidth, rExtents.left, rExtents.top, rExtents.right, rExtents.bottom);


    unsigned char nPen = 0;
    unsigned char nOffset = 0;

    for (int y = 0; y <= rExtents.bottom; y++)
    {
        for (int x = (int) rExtents.left; x <= (int) rExtents.right; x++)
        {
//            DWORD nCol = GetPixel(mhWinTargetDC, x, y);
//            unsigned char nBright = 255 - GetGValue(nCol);
            uint32_t nCol = (uint32_t) *((uint32_t*)mpBits + y * mrScratchArea.right + x);
            
            unsigned char nAlpha = 255-nCol&0x000000ff;

            unsigned char nBrightIndex = nAlpha / 24;
            if (nBrightIndex > 7)
                nBrightIndex = 7;
//            assert(nBrightIndex < 8);


            if (nPen != nBrightIndex)
            {
                //            sprintf(buf, " %ld,%ld", nBright, nOffset);
                //            OutputDebugString(buf);

                uint16_t nPacked = (nBrightIndex) << 13 | (nOffset & 0X1fff);    // bright index is leftmost 3 bits

//                mCharDescriptors[c].pixelData.push_back(nPen);
                mCharDescriptors[c].pixelData.push_back(nPacked);
                nOffset = 0;
                nPen = nBrightIndex;
            }

            nOffset++;

            assert(nOffset < 0x1FFF);

            // So if we reach the max, we write the current current pen and just keep counting
            if (nOffset == 0x1FFF)
            {
                //            sprintf(buf, "found offset overrun for char:'%c'\n", c);
                //            OutputDebugString(buf);

                uint16_t nPacked = (nBrightIndex) << 13 | 0x1FFF;    // bright index is leftmost 3 bits
//                mCharDescriptors[c].pixelData.push_back(nPen);
                mCharDescriptors[c].pixelData.push_back(nPacked);
                nOffset = 0;
            }
        }
    }

    if (nPen != 0)
    {
        //      sprintf(buf, " %ld", nOffset);
        //      OutputDebugString(buf);
        uint16_t nPacked = (nPen) << 13 | (nOffset & 0x1FFF);    // bright index is leftmost 3 bits
//        mCharDescriptors[c].pixelData.push_back(nPen);
        mCharDescriptors[c].pixelData.push_back(nPacked);
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


bool ZDynamicFont::GenerateGlyph(char c)
{
    if (c <= 0)
        return false;

    if (c == '#')
        int breakhere = 5;

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
        mCharDescriptors[c].nCharWidth = (uint16_t) (mFontParams.nHeight / 4);
    }
    else
    {
        ExtractChar(c);
    }

    return true;
}

bool ZDynamicFont::RetrieveKerningPairs()
{
    DWORD nPairs = GetKerningPairsA(mhWinTargetDC, 0, 0);   // how many pairs required?

    KERNINGPAIR* pKerningPairArray = new KERNINGPAIR[nPairs];

    DWORD nResult = GetKerningPairsA(mhWinTargetDC, nPairs, pKerningPairArray);

    for (int i = 0; i < (int) nPairs; i++)
    {
        char cFirst = (char) pKerningPairArray[i].wFirst;
        char cSecond = (char) pKerningPairArray[i].wSecond;
        int16_t nKern = pKerningPairArray[i].iKernAmount-2;

        // only pairs in the ascii set 0 - 127
        if (cFirst > 0 && cSecond > 0)
        {

            mCharDescriptors[cFirst].kerningArray[cSecond] = nKern;
            ZDEBUG_OUT("kerning %c:%c = %d\n", cFirst, cSecond, nKern);
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
    gWindowsFontFacenames.push_back(WideToAscii(lpelfe->lfFaceName));
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
    mFonts.clear();
}

int32_t ZFontSystem::LoadFont(const string& sFilename)
{
    ZFont* pNewFont = new ZFont();
    if (!pNewFont->LoadFont(sFilename))
    {
        delete pNewFont;
        ZDEBUG_OUT("Failed to load font:%s\n", sFilename.c_str());
        return -1;
    }

    mFonts.emplace_back(pNewFont);

    return (int32_t) (mFonts.size()-1);
}

#ifdef _WIN64
int32_t ZFontSystem::CreateFont(const ZFontParams& params)
{
    ZDynamicFont* pNewFont = new ZDynamicFont();
    pNewFont->Init(params);

    mFonts.emplace_back(pNewFont);

    int32_t nID = (int32_t) (mFonts.size() - 1);

    ZDEBUG_OUT("ZFontSystem::CreateFont ID:%d - size:%d, name:%s\n", nID, params.nHeight, params.sFacename.c_str());

    return nID;
}
#endif

std::shared_ptr<ZFont> ZFontSystem::GetFont(int32_t nIndex)
{
    if (nIndex < 0 || nIndex >= mFonts.size())
        return nullptr;

    return mFonts[nIndex];
}
