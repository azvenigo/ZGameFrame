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
    nFixedWidth = 0;
    bItalic = false;
    bUnderline = false;
    bStrikeOut = false;
}

ZFontParams::ZFontParams(const ZFontParams& rhs)
{
    nHeight = rhs.nHeight;
    nWeight = rhs.nWeight;
    nTracking = rhs.nTracking;
    nFixedWidth = rhs.nFixedWidth;
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

string sZFontHeader("ZFONT v3.1 by Alex Zvenigorodsky");

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



ZTextLook::ZTextLook(eDeco _decoration, uint32_t _colTop, uint32_t _colBottom)
{ 
    decoration = _decoration; 
    colTop = _colTop; 
    colBottom = _colBottom; 
}

ZTextLook::ZTextLook(const std::string& s)
{
    nlohmann::json j = nlohmann::json::parse(s);

    if (j.contains("deco"))
        decoration = j["deco"];

    if (j.contains("colT"))
        colTop = j["colT"];

    if (j.contains("colB"))
        colBottom = j["colB"];
}

ZTextLook::operator string() const
{
    nlohmann::json j;
    j["deco"] = decoration;
    j["colT"] = colTop;
    j["colB"] = colBottom;

    return j.dump();
}





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

    memcpy(&mFontParams.nFixedWidth, pData, sizeof(mFontParams.nFixedWidth));
    pData += sizeof(mFontParams.nFixedWidth);

    memcpy(&mFontParams.bItalic, pData, sizeof(mFontParams.bItalic));
    pData += sizeof(mFontParams.bItalic);

    memcpy(&mFontParams.bStrikeOut, pData, sizeof(mFontParams.bStrikeOut));
    pData += sizeof(mFontParams.bStrikeOut);

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

    ZOUT("Loaded ", sFilename, " in ", gTimer.GetUSSinceEpoch()-nStartTime, "us");

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

//    uint8_t* pUncompressedFontData = new uint8_t[nUncompressedBytes];
    ZMemBufferPtr uncompBuffer( new ZMemBuffer());

    uncompBuffer->write((uint8_t*) sZFontHeader.data(), (uint32_t) sZFontHeader.length());
    uncompBuffer->write((uint8_t*) &mFontParams.nHeight, sizeof(mFontParams.nHeight));
    uncompBuffer->write((uint8_t*)&mFontParams.nWeight, sizeof(mFontParams.nWeight));
    uncompBuffer->write((uint8_t*)&mFontParams.nTracking, sizeof(mFontParams.nTracking));
    uncompBuffer->write((uint8_t*)&mFontParams.nFixedWidth, sizeof(mFontParams.nFixedWidth));
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

int32_t ZFont::GetSpaceBetweenChars(uint8_t c1, uint8_t c2)
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
    const uint8_t* pChar;
    for (pChar = (uint8_t*)sText.c_str(); pChar < (uint8_t*)sText.data()+nLength-1; pChar++)   // add up all of the chars except last one
    {
        nWidth += mCharDescriptors[*pChar].nCharWidth + 1 + GetSpaceBetweenChars(*pChar, *(pChar+1)); // char width adjusted by kerning with next char
    }

    if (*pChar)
        nWidth += mCharDescriptors[*pChar].nCharWidth;  // last char

	return nWidth;
}

bool ZFont::DrawText( ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, const ZTextLook& look, ZRect* pClip )
{
	ZASSERT(mbInitted);
	ZASSERT(pBuffer);

	if (!pBuffer || !mbInitted )
		return false;

	switch (look.decoration)
	{
    case ZTextLook::kNormal:
		if (look.colTop == look.colBottom && ARGB_A(look.colTop) == 0xff)
			return DrawText_Helper(pBuffer, sText, rAreaToDrawTo, look.colTop, pClip);
		return DrawText_Gradient_Helper(pBuffer, sText, rAreaToDrawTo, look.colTop, look.colBottom, pClip);
		break;

	case ZTextLook::kShadowed:
		{
			uint32_t nShadowColor = look.colTop & 0xAA000000;		// draw the shadow, but only as dark as the text alpha
			int64_t nShadowOffset = mFontParams.nHeight/20;
            if (nShadowOffset < 1)
                nShadowOffset = 1;
            else if (nShadowOffset > 8)
                nShadowOffset = 8;


			ZRect rShadowRect(rAreaToDrawTo);
			rShadowRect.OffsetRect(nShadowOffset, nShadowOffset);
			if (look.colTop == look.colBottom)
			{
				DrawText_Helper(pBuffer, sText, rShadowRect, nShadowColor, pClip);
				return DrawText_Helper(pBuffer, sText, rAreaToDrawTo, look.colTop, pClip);
			};

			DrawText_Helper(pBuffer, sText, rShadowRect, nShadowColor, pClip);
			return DrawText_Gradient_Helper(pBuffer, sText, rAreaToDrawTo, look.colTop, look.colBottom, pClip);
		}
		break;

	case ZTextLook::kEmbossed:
		{
			uint32_t nShadowColor = look.colTop & 0xAA000000;		// draw the shadow, but only as dark as the text alpha
			uint32_t nHighlightColor = (look.colTop & 0xAA000000) | 0x00ffffff;	// the highlight will also only be as bright as the text alpha

			int64_t nShadowOffset = mFontParams.nHeight/32;
            if (nShadowOffset < 1)
                nShadowOffset = 1;
            else if (nShadowOffset > 8)
                nShadowOffset = 8;

			ZRect rShadowRect(rAreaToDrawTo);
			rShadowRect.OffsetRect(-nShadowOffset, -nShadowOffset);
	
			ZRect rHighlightRect(rAreaToDrawTo);
			rHighlightRect.OffsetRect(nShadowOffset, nShadowOffset);

			DrawText_Helper(pBuffer, sText, rShadowRect, nShadowColor, pClip);			// draw the shadow
			DrawText_Helper(pBuffer, sText, rHighlightRect, nHighlightColor, pClip);			// draw the highlight
			if (look.colTop == look.colBottom)
				return DrawText_Helper(pBuffer, sText, rAreaToDrawTo, look.colTop, pClip);

			return DrawText_Gradient_Helper(pBuffer, sText, rAreaToDrawTo, look.colTop, look.colBottom, pClip);
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
        const uint8_t* pChar = (uint8_t*)sText.data();
        for (;pChar < (uint8_t*)sText.data()+sText.length(); pChar++)
		{
			DrawCharClipped(pBuffer, *pChar, nCol, nX, nY, (ZRect*) &rClipDest);

            if (mFontParams.nFixedWidth > 0)
                nX += mFontParams.nFixedWidth;
            else
			    nX += mCharDescriptors[*pChar].nCharWidth + 1 + GetSpaceBetweenChars(*pChar, *(pChar+1));

            if (nX >= rAreaToDrawTo.right)
                break;
		}
	}
	else
	{
        const uint8_t* pChar = (uint8_t*)sText.data();
        for (; pChar < (uint8_t*)sText.data() + sText.length(); pChar++)
        {
			DrawCharNoClip(pBuffer, *pChar, nCol, nX, nY);

            if (mFontParams.nFixedWidth > 0)
                nX += mFontParams.nFixedWidth;
            else
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

    std::vector<uint32_t> gradient;
	BuildGradient(nCol, nCol2, gradient);

	// Draw the text clipped
    const uint8_t* pChar = (uint8_t*)sText.data();
    for (; pChar < (uint8_t*)sText.data() + sText.length(); pChar++)
    {
		DrawCharGradient(pBuffer, *pChar, gradient, nX, nY, (ZRect*) &rClipDest);

        if (mFontParams.nFixedWidth > 0)
            nX += mFontParams.nFixedWidth;
        else
            nX += mCharDescriptors[*pChar].nCharWidth + 1 + GetSpaceBetweenChars(*pChar, *(pChar+1));

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
				    *pDest = COL::AlphaBlend_AddAlpha(nCol, *pDest, nAlpha);

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
                        *pDest = COL::AlphaBlend_AddAlpha(nCol, *pDest, nAlpha);
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
                    *pDest = COL::AlphaBlend_AddAlpha(gradient[nScanLine], *pDest, nAlpha);

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
    return ZRect(0, 0, StringWidth(sText), mFontParams.nHeight);
}


// This function helps format text by returning a rectangle where the text should be output
// It will not clip, however... That should be done by the caller if necessary
ZRect ZFont::GetOutputRect(ZRect rArea, const uint8_t* pChars, size_t nNumChars, ZGUI::ePosition pos, int64_t nPadding)
{
	ZRect rText(0,0, StringWidth(string((char*)pChars,nNumChars)), mFontParams.nHeight);
	rArea.DeflateRect(nPadding, nPadding);

/*	int64_t nXCenter = rArea.left + (rArea.Width()  - rText.Width())/2;
	int64_t nYCenter = rArea.top  + (rArea.Height() - mFontParams.nHeight)/2;

	switch (pos)
	{
	case ZGUI::LT:
		rText.OffsetRect(rArea.left, rArea.top);
		break;
	case ZGUI::CT:
		rText.OffsetRect(nXCenter, rArea.top);
		break;
	case ZGUI::RT:
		rText.OffsetRect(rArea.right - rText.Width(), rArea.top);
		break;

	case ZGUI::LC:
		rText.OffsetRect(rArea.left, nYCenter);
		break;
	case ZGUI::C:
		rText.OffsetRect(nXCenter, nYCenter);
		break;
	case ZGUI::RC:
		rText.OffsetRect(rArea.right - rText.Width(), nYCenter);
		break;

	case ZGUI::LB:
		rText.OffsetRect(rArea.left, rArea.bottom - mFontParams.nHeight);
		break;
	case ZGUI::CB:
		rText.OffsetRect(nXCenter, rArea.bottom - mFontParams.nHeight);
		break;
	case ZGUI::RB:
		rText.OffsetRect(rArea.right - rText.Width(), rArea.bottom - mFontParams.nHeight);
		break;
	}

	return rText;*/

    return ZGUI::Arrange(rText, rArea, pos, nPadding);
}

bool ZFont::DrawTextParagraph( ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, const ZTextLook& look, ZGUI::ePosition pos, ZRect* pClip)
{
	ZRect rTextLine(rAreaToDrawTo);

	int64_t nLines = CalculateNumberOfLines(rAreaToDrawTo.Width(), (uint8_t*)sText.data(), sText.length());
    ZGUI::ePosition lineStyle = ZGUI::LB;

	// Calculate the top line 
	switch (pos)
	{
	case ZGUI::LT:
	case ZGUI::CT:
	case ZGUI::RT:
		break;
	case ZGUI::LC:
	case ZGUI::C:
	case ZGUI::RC:
		rTextLine.top = (rAreaToDrawTo.top + rAreaToDrawTo.bottom - (mFontParams.nHeight * nLines))/2;
		break;
	case ZGUI::LB:
	case ZGUI::CB:
	case ZGUI::RB:
		rTextLine.top = rAreaToDrawTo.bottom - (mFontParams.nHeight * nLines);
		break;
	}

	// Which individual line style to use
	switch (pos)
	{
	case ZGUI::CT:
	case ZGUI::C:
	case ZGUI::CB:
		lineStyle = ZGUI::CB;
		break;
	case ZGUI::RT:
	case ZGUI::RC:
	case ZGUI::RB:
		lineStyle = ZGUI::RB;
	}

	rTextLine.bottom = rTextLine.top + mFontParams.nHeight;

	int64_t nCharsDrawn = 0;

    const char* pChars = sText.data();
    const char* pEnd = pChars + sText.length();

    while (pChars < pEnd && rTextLine.top < rTextLine.bottom)
	{
        int64_t nLettersToDraw = CalculateWordsThatFitOnLine(rTextLine.Width(), (uint8_t*) pChars, pEnd-pChars);
       
        ZRect rAdjustedLine(GetOutputRect(rTextLine, (uint8_t*)pChars, nLettersToDraw, lineStyle));
        if (rAdjustedLine.bottom > rAreaToDrawTo.top && rAdjustedLine.top < rAreaToDrawTo.bottom)       // only draw if line is within rAreaToDrawTo
		    DrawText(pBuffer, string(pChars, nLettersToDraw), rAdjustedLine, look, pClip);

		nCharsDrawn += nLettersToDraw;

		rTextLine.OffsetRect(0, mFontParams.nHeight);
		pChars += nLettersToDraw;
	}

	return nCharsDrawn == sText.length();
}

int64_t ZFont::CharWidth(uint8_t c)
{ 
    if (c > 0)
        return mCharDescriptors[c].nCharWidth; 

    return 0;
}


int64_t ZFont::CalculateNumberOfLines(int64_t nLineWidth, const uint8_t* pChars, size_t nNumChars)
{
	ZASSERT(pChars);

	int64_t nLines = 0;
    uint8_t* pEnd = (uint8_t*) pChars + nNumChars;
	while (pChars < pEnd)
	{
		int64_t nLettersToDraw = CalculateWordsThatFitOnLine(nLineWidth, pChars, pEnd-pChars);
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
int64_t ZFont::CalculateWordsThatFitOnLine(int64_t nLineWidth, const uint8_t* pChars, size_t nNumChars)
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
    ZASSERT(mFontParams.nHeight > 1);
    gradient.resize(mFontParams.nHeight);
    //	CEASSERT(mColorGradient.size() == mFontParams.nHeight);
        // If the gradient is already set, don't bother
    if (gradient[0] == nColor1 && gradient[mFontParams.nHeight - 1] == nColor2)
        return;
    //ZDEBUG_OUT("Building gradient for colors %x - %x mFontParams.nHeight:%d", nColor1, nColor2, mFontParams.nHeight);
    gradient[0] = nColor1;
    gradient[mFontParams.nHeight - 1] = nColor2;
    for (int64_t i = 1; i < mFontParams.nHeight - 1; i++)
    {
        double fRange = 10.0 * (((double)(i - 1) - (double)(mFontParams.nHeight - 3) / 2.0) / (double)mFontParams.nHeight);
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


bool ZDynamicFont::Init(const ZFontParams& params, bool bInitGlyphs, bool bKearn)
{
    if (mbInitted)
    {
        ZDEBUG_OUT("Dynamic font already exists. Cannot be re-initted\n");
        return false;
    }

    if (params.sFacename.empty())
        return false;

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
    mDIBInfo.bmiHeader.biXPelsPerMeter = 2835;
    mDIBInfo.bmiHeader.biYPelsPerMeter = 2835;
    mDIBInfo.bmiColors[0].rgbBlue = 0;
    mDIBInfo.bmiColors[0].rgbGreen = 0;
    mDIBInfo.bmiColors[0].rgbRed = 0;
    mDIBInfo.bmiColors[0].rgbReserved = 0;

    DWORD pitch = DEFAULT_PITCH;
    if (mFontParams.nFixedWidth > 0)
    {
        pitch = FIXED_PITCH;
        mbEnableKerning = false;
    }

    DWORD nCharSet = ANSI_CHARSET;
    if (mFontParams.sFacename.find("Wingdings") != string::npos)
        nCharSet = SYMBOL_CHARSET;

    //mWinHDC = GetDC(ghWnd);
    mhWinTargetBitmap = CreateDIBSection(mWinHDC, &mDIBInfo, DIB_RGB_COLORS, (void**)&mpBits, NULL, 0);

    mhWinTargetDC = CreateCompatibleDC(mWinHDC);
    mhWinFont = CreateFontA( 
        (int)mFontParams.nHeight,
        0,                      /*nWidth*/
        0,                      /*nEscapement*/
        0,                      /*nOrientation*/
        (int) mFontParams.nWeight, 
        mFontParams.bItalic, 
        mFontParams.bUnderline, 
        mFontParams.bStrikeOut, 
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

    mnWidestCharacterWidth = FindWidestCharacterWidth();
    mnWidestNumberWidth = FindWidestNumberWidth();

    if (bInitGlyphs)
    {
        for (int32_t c = 32; c <= kMaxChars; c++) 
            GenerateGlyph((uint8_t)c);
    }

    if (bKearn && mFontParams.nFixedWidth == 0)
    {
        // Variable size font
        if (!RetrieveKerningPairs())    // try and retrieve built in kerning pairs
        {
            // none available. manually compute 
            for (uint8_t c1 = 33; c1 <= 127; c1++)
            {
                for (uint8_t c2 = 33; c2 <= 127; c2++)
                    FindKerning(c1, c2);
            }
        }
    }


    int64_t nEnd = gTimer.GetUSSinceEpoch();
    ZDEBUG_OUT("Pregeneration took:%dus\n", (int)nEnd - nStart);
    
    mbInitted = true;

    return true;
}

ZRect ZDynamicFont::FindCharExtents()
{
    ZRect rExtents;

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


bool ZDynamicFont::ExtractChar(uint8_t c)
{
    if (c <= 0)
        return false;

    ZRect rExtents = FindCharExtents();

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

        rExtents.left = rExtents.left - nPadding;
        rExtents.right = rExtents.left + mnWidestCharacterWidth;
        ZASSERT(rExtents.left > 0 && rExtents.right < mrScratchArea.right);
    }
    else if (c >= '0' && c <= '9')  // otherwise numbers use fixed width based on their own widest char
    {
        assert(mnWidestNumberWidth > 0);         // need to have this one already.....may need to look into this requirement
        int64_t nNumWidth = rExtents.right - rExtents.left;
        int64_t nPadding = (mnWidestNumberWidth - nNumWidth) / 2;

        rExtents.left = rExtents.left - nPadding;
        rExtents.right = rExtents.left + mnWidestNumberWidth;
    }

    
    mCharDescriptors[c].nCharWidth = (uint16_t) (rExtents.right - rExtents.left + 1);

    //ZDEBUG_OUT("\nExtracting %ld:'%c' width:%ld - extents(%ld,%ld,%ld,%ld) offsets:", c, c, mCharDescriptors[c].nCharWidth, rExtents.left, rExtents.top, rExtents.right, rExtents.bottom);


    uint8_t nPen = 0;
    uint8_t nOffset = 0;

    for (int64_t y = 0; y <= rExtents.bottom; y++)
    {
        for (int64_t x = rExtents.left; x <= rExtents.right; x++)
        {
            uint32_t nCol = (uint32_t) *((uint32_t*)mpBits + y * mrScratchArea.right + x);
            uint8_t nBright = 255-nCol&0x000000ff;


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


int32_t ZDynamicFont::FindWidestCharacterWidth()
{
    int32_t nWidest = 0;

    std::list<uint8_t> charsToTest = { 'W', 'M', '@', '_' };

    for (auto c : charsToTest)
    {
        RECT r;
        r.left = 0;
        r.top = 0;
        r.right = (LONG)mrScratchArea.right;
        r.bottom = (LONG)mrScratchArea.bottom;

        SelectObject(mhWinTargetDC, mhWinTargetBitmap);
        SetBkMode(mhWinTargetDC, TRANSPARENT);
        BOOL bReturn = BitBlt(mhWinTargetDC, 0, 0, (int)mrScratchArea.Width(), (int)mrScratchArea.Height(), NULL, 0, 0, WHITENESS);

        SelectFont(mhWinTargetDC, mhWinFont);
        ::DrawTextA(mhWinTargetDC, (LPCSTR)&c, 1, &r, DT_TOP | DT_CENTER);

        ZRect rExtents = FindCharExtents();
        if (rExtents.Width() > nWidest)
            nWidest = (int)rExtents.Width();
    }

    return nWidest;
}

int32_t ZDynamicFont::FindWidestNumberWidth()
{
    int32_t nWidest = 0;

    for (uint8_t c = '0'; c <= '9'; c++)
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


bool ZDynamicFont::GenerateGlyph(uint8_t c)
{
    if (c <= 0)
        return false;
    
    RECT r;
    r.left = 0;
    r.top = 0;
    r.right = (LONG) mrScratchArea.right;
    r.bottom = (LONG) mrScratchArea.bottom;

    SetBkMode(mhWinTargetDC, TRANSPARENT);
    BOOL bReturn = BitBlt(mhWinTargetDC, 0, 0, (int) mrScratchArea.Width(), (int) mrScratchArea.Height(), NULL, 0, 0, WHITENESS);

    SelectFont(mhWinTargetDC, mhWinFont);
    int nHeightReturned = ::DrawTextA(mhWinTargetDC, (LPCSTR) &c, 1, &r, DT_TOP|DT_CENTER);

    if (c == ' ')
    {
        mCharDescriptors[c].nCharWidth = (uint16_t) (mFontParams.nHeight / 4);  // 25% of the height seems good
//        mCharDescriptors[c].nCharWidth = mnWidestCharacterWidth;
    }
    else
    {
        ExtractChar(c);
    }

    return true;
}

bool ZDynamicFont::GenerateSymbolicGlyph(uint8_t c, uint32_t symbol)
{
    if (c <= 0)
        return false;

    RECT r;
    r.left = 0;
    r.top = 0;
    r.right = (LONG)mrScratchArea.right;
    r.bottom = (LONG)mrScratchArea.bottom;

    SetBkMode(mhWinTargetDC, TRANSPARENT);
    BOOL bReturn = BitBlt(mhWinTargetDC, 0, 0, (int)mrScratchArea.Width(), (int)mrScratchArea.Height(), NULL, 0, 0, WHITENESS);

    SelectFont(mhWinTargetDC, mhWinFont);
    int nHeightReturned = ::DrawTextW(mhWinTargetDC, (LPCWSTR)&symbol, 1, &r, DT_TOP | DT_CENTER);

    ExtractChar(c);

    return true;
}


bool ZDynamicFont::RetrieveKerningPairs()
{
    if (mFontParams.nFixedWidth > 0)
        return false;

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
        uint8_t cFirst = (uint8_t) pKerningPairArray[i].wFirst;
        uint8_t cSecond = (uint8_t) pKerningPairArray[i].wSecond;
        int16_t nKern = pKerningPairArray[i].iKernAmount-2;

        if (cFirst > 0 && cSecond > 0)
        {
            mCharDescriptors[cFirst].kerningArray[cSecond] = nKern;
        }
    }

    delete[] pKerningPairArray;

    return true;
}



void ZDynamicFont::DrawCharNoClip(ZBuffer* pBuffer, uint8_t c, uint32_t nCol, int64_t nX, int64_t nY)
{
    if (mCharDescriptors[c].nCharWidth == 0)   // generate glyph if not already generated
        GenerateGlyph(c);

    return ZFont::DrawCharNoClip(pBuffer, c, nCol, nX, nY);
}

void ZDynamicFont::DrawCharClipped(ZBuffer* pBuffer, uint8_t c, uint32_t nCol, int64_t nX, int64_t nY, ZRect* pClip)
{
    if (mCharDescriptors[c].nCharWidth == 0)   // generate glyph if not already generated
        GenerateGlyph(c);

    return ZFont::DrawCharClipped(pBuffer, c, nCol, nX, nY, pClip);
}

void ZDynamicFont::DrawCharGradient(ZBuffer* pBuffer, uint8_t c, std::vector<uint32_t>& gradient, int64_t nX, int64_t nY, ZRect* pClip)
{
    if (mCharDescriptors[c].nCharWidth == 0)   // generate glyph if not already generated
        GenerateGlyph(c);

    return ZFont::DrawCharGradient(pBuffer, c, gradient, nX, nY, pClip);
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
    mFontMap.clear();
}

bool ZFontSystem::SetDefaultFont(const ZFontParams& params)
{
    mpDefault = CreateFont(params);
    return true;
}


bool ZFontSystem::SetCacheFolder(const std::string& sFolderPath)
{
    if (!std::filesystem::exists(sFolderPath))
    {
        ZDEBUG_OUT("Font Cache Folder:%s Doesn't exist!\n", sFolderPath.c_str());
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
    Sprintf(sFilename, "%s_%d_%d_%d", params.sFacename.c_str(), params.nHeight, params.nWeight, params.nTracking);
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
    ZDEBUG_OUT("Loaded font:%s size:%d\n", fp.sFacename, fp.nHeight);

    mFontMap[pNewFont->GetFontParams()] = pNewFont;
    return pNewFont;
}

#ifdef _WIN64
tZFontPtr ZFontSystem::CreateFont(const ZFontParams& params)
{
    ZASSERT(!params.sFacename.empty());

    if (IsCached(params))
    {
        tZFontPtr pLoaded = LoadFont(FontCacheFilename(params));
        if (pLoaded)
        {
            mFontMap[pLoaded->GetFontParams()] = pLoaded;
            return pLoaded;
        }

        ZDEBUG_OUT("WARNING: cached font failed to load...re-creating.");
    }

    ZDynamicFont* pNewFont = new ZDynamicFont();
    pNewFont->Init(params);

    ZFontParams fp(pNewFont->GetFontParams());
    ZDEBUG_OUT("Created font:%s size:%d\n", fp.sFacename, fp.nHeight);

    mFontMap[pNewFont->GetFontParams()].reset(pNewFont);

    if (!msCacheFolder.empty())
    {
        ZDEBUG_OUT("Saving font:%s to cache", fp.sFacename.c_str());
        pNewFont->SaveFont(FontCacheFilename(params));
    }

    return mFontMap[pNewFont->GetFontParams()];
}
#endif


tZFontPtr ZFontSystem::GetFont(const std::string& sFontName, int32_t nFontSize)
{
    for (auto findIt : mFontMap)
    {
        const ZFontParams& fp = findIt.first;
        if (fp.sFacename == sFontName && fp.nHeight == nFontSize)
            return findIt.second;
    }

    // Not found, create one
    return CreateFont(ZFontParams(sFontName, nFontSize));
}

tZFontPtr ZFontSystem::GetFont(const ZFontParams& params)
{
    auto findIt = mFontMap.find(params);

    // If not found
    if (findIt == mFontMap.end())
    {
        ZDEBUG_OUT("Uncached font requested.");
        return CreateFont(params);
    }

    return (*findIt).second;
}
