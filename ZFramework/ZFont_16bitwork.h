#pragma once

#include "ZBuffer.h"
#include <string>
#include "ZStdTypes.h"
#include "ZStdDebug.h"
#include <vector>

#ifdef _WIN64
#include <windowsx.h>       // for dynamic font glyph generation
#endif


using namespace std;


const int32_t kMaxChars = 128;  // lower ascii 0-127


// Data is in this format:
// 3 bits for alpha (0, 32, 64, 96, 128, 160, 192, 256)  Note: not linear
// 13 bits for nNumPixels at that value
// char: nNumPixels at that value
//
// The list ends with two 0s
typedef std::vector<uint16_t> tPixelDataList;

struct sCharDescriptor
{
    sCharDescriptor() 
    { 
        nCharWidth = 0; 
        memset(kerningArray, 0, sizeof(kerningArray));
    }
	uint16_t        nCharWidth;
    int16_t         kerningArray[kMaxChars];    // array of kernings against other ascii chars
	tPixelDataList  pixelData;                  // The number of pixels from the current to the next (using the nCharWidth to determine when to wrap around)
};


class ZFontParams
{
public:
    ZFontParams();
    ZFontParams(const ZFontParams& rhs);

    int64_t nHeight;
    int64_t nWeight;
    bool bItalic;
    bool bUnderline;
    bool bStrikeOut;
    string sFacename;
};

class ZFont
{
public:
	enum ePosition
	{
		kTopLeft       = 0,
		kTopCenter     = 1,
		kTopRight      = 2,
		kMiddleLeft    = 3,
		kMiddleCenter  = 4,
		kMiddleRight   = 5,
		kBottomLeft    = 6,
		kBottomCenter  = 7,
		kBottomRight   = 8
	};

	enum eStyle
	{
		kNormal     = 0,
		kShadowed   = 1,
		kEmbossed   = 2
	};


public:
	ZFont();
	~ZFont();

    virtual bool    LoadFont(const string& sFilename);
    virtual bool    SaveFont(const string& sFilename);

    ZFontParams*    GetFontParams() { return &mFontParams; }

    std::string     GetFilename() { return msFilename; }

    bool            DrawText(ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol = 0xffffffff, uint32_t nCol2 = 0xffffffff, eStyle style = kNormal, ZRect* pClip = NULL);
    bool            DrawTextParagraph(ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol = 0xffffffff, uint32_t nCol2 = 0xffffffff, ePosition paragraphPosition = kTopLeft, eStyle style = kNormal, ZRect* pClip = NULL);

	int64_t         CharWidth(char c);
    void            SetEnableKerning(bool bEnable) { mbEnableKerning = bEnable; }
	int64_t         FontHeight() { return mFontParams.nHeight; }
    int32_t         GetKerning(char c1, char c2);


	int64_t         StringWidth(const string& sText);
	int64_t         CalculateWordsThatFitOnLine(int64_t nLineWidth, const char* pChars, size_t nNumChars);  // returns the number of characters that should be drawn on this line, breaking at words
	int64_t         CalculateLettersThatFitOnLine(int64_t nLineWidth, const char* pChars, size_t nNumChars);	// returns the number of characters that fit on that line
	int64_t         CalculateNumberOfLines(int64_t nLineWidth, const char* pChars, size_t nNumChars);	// returns the number of lines required to draw text

	ZRect           GetOutputRect(ZRect rArea, const char* pChars, size_t nNumChars, ePosition position, int64_t nPadding = 0);

protected:
	bool                    DrawText_Helper(ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol, ZRect* pClip);
	bool                    DrawText_Gradient_Helper(ZBuffer* pBuffer, const string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol, uint32_t nCol2, ZRect* pClip);

    void                    BuildGradient(uint32_t nColor1, uint32_t nColor2);
   
    virtual void            DrawCharNoClip(ZBuffer* pBuffer, char c, uint32_t nCol, int64_t nX, int64_t nY);
    virtual void            DrawCharClipped(ZBuffer* pBuffer, char c, uint32_t nCol, int64_t nX, int64_t nY, ZRect* pClip);
    virtual void            DrawCharGradient(ZBuffer* pBuffer, char c, int64_t nX, int64_t nY, ZRect* pClip);

    std::string             msFilename;
//	uint32_t                mnFontHeight;
    ZFontParams             mFontParams;
    bool                    mbEnableKerning;

	sCharDescriptor         mCharDescriptors[kMaxChars];  // lower 128 ascii chars
	bool                    mbInitted;
	std::vector<uint32_t>	mColorGradient;
};


// Under windows the DynamicFont class can generate glyphs on demand
#ifdef _WIN64

extern std::vector<string> gWindowsFontFacenames;


inline bool operator==(const ZFontParams& lhs, const ZFontParams& rhs)
{
    bool bEqual = 
        lhs.nHeight             == rhs.nHeight &&
        lhs.nWeight             == rhs.nWeight &&
        lhs.bItalic             == rhs.bItalic &&
        lhs.bUnderline          == rhs.bUnderline &&
        lhs.bStrikeOut          == rhs.bStrikeOut &&
        lhs.sFacename.compare(rhs.sFacename) == 0;

    return bEqual;
}

// Font that generates glyphs on the fly
class ZDynamicFont : public ZFont
{
public:
    ZDynamicFont();
    ~ZDynamicFont();

    bool                Init(const ZFontParams& params);
    bool                SaveFont(const string& sFilename);  // overload to first ensure all glyphs have been generated

protected:
    // overloads from ZFont that will generate glyphs if needed
    virtual void        DrawCharNoClip(ZBuffer* pBuffer, char c, uint32_t nCol, int64_t nX, int64_t nY);
    virtual void        DrawCharClipped(ZBuffer* pBuffer, char c, uint32_t nCol, int64_t nX, int64_t nY, ZRect* pClip);
    virtual void        DrawCharGradient(ZBuffer* pBuffer, char c, int64_t nX, int64_t nY, ZRect* pClip);


private:
    // Experimental generating glyphs on the fly
    bool                GenerateGlyph(char c);
    bool                ExtractChar(char c);
    bool                RetrieveKerningPairs();
    ZRect               FindCharExtents();

    ZRect               mrScratchArea;


    HDC                 mhWinTargetDC;
    HBITMAP             mhWinTargetBitmap;
    BITMAPINFO          mDIBInfo;
    uint8_t*            mpBits;
    HDC                 mWinHDC;
    HFONT               mhWinFont;

};

#endif


typedef std::vector< std::shared_ptr<ZFont> > tFontVector;


class ZFontSystem
{
public:
    ZFontSystem();
    ~ZFontSystem();

    bool                    Init();        
    void                    Shutdown();

    int32_t                 LoadFont(const string& sFilename);          // returns loaded font index
#ifdef _WIN64
    int32_t                 CreateFont(const ZFontParams& params);      // returns created font index
#endif

    int32_t                 GetFountCount() { return (int32_t) mFonts.size(); }
    std::shared_ptr<ZFont>  GetFont(int32_t nIndex);

    // convenience functions
    //std::shared_ptr<ZFont>  GetFontByFilename(const string& sFilename);
    //std::shared_ptr<ZFont>  GetFontByParams(const ZFontParams& params);

private:
     
    tFontVector     mFonts;
};

extern ZFontSystem*     gpFontSystem;