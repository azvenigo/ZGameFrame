#pragma once

#include "ZBuffer.h"
#include <string>
#include "ZStdTypes.h"
#include "ZStdDebug.h"
#include <vector>

#ifdef _WIN64
#include <windowsx.h>       // for dynamic font glyph generation
#endif

const int32_t kMaxChars = 128;  // lower ascii 0-127


// Data is in this format:
// char: nAlphaValue   0-255
// char: nNumPixels at that value
//
// The list ends with two 0s
typedef std::vector<uint8_t> tPixelDataList;

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
    int64_t nTracking;
    bool bItalic;
    bool bUnderline;
    bool bStrikeOut;
    std::string sFacename;
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

    virtual bool    LoadFont(const std::string& sFilename);
    virtual bool    SaveFont(const std::string& sFilename);

    ZFontParams*    GetFontParams() { return &mFontParams; }

    std::string     GetFilename() { return msFilename; }

    bool            DrawText(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol = 0xffffffff, uint32_t nCol2 = 0xffffffff, eStyle style = kNormal, ZRect* pClip = NULL);
    bool            DrawTextParagraph(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol = 0xffffffff, uint32_t nCol2 = 0xffffffff, ePosition paragraphPosition = kTopLeft, eStyle style = kNormal, ZRect* pClip = NULL);

	int64_t         CharWidth(char c);
    void            SetEnableKerning(bool bEnable) { mbEnableKerning = bEnable; }
    void            SetTracking(int64_t nTracking) { mFontParams.nTracking = nTracking; }
	int64_t         FontHeight() { return mFontParams.nHeight; }
    int32_t         GetSpaceBetweenChars(char c1, char c2);


	int64_t         StringWidth(const std::string& sText);
	int64_t         CalculateWordsThatFitOnLine(int64_t nLineWidth, const char* pChars, size_t nNumChars);  // returns the number of characters that should be drawn on this line, breaking at words
	int64_t         CalculateLettersThatFitOnLine(int64_t nLineWidth, const char* pChars, size_t nNumChars);	// returns the number of characters that fit on that line
	int64_t         CalculateNumberOfLines(int64_t nLineWidth, const char* pChars, size_t nNumChars);	// returns the number of lines required to draw text

	ZRect           GetOutputRect(ZRect rArea, const char* pChars, size_t nNumChars, ePosition position, int64_t nPadding = 0);

protected:
	bool                    DrawText_Helper(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol, ZRect* pClip);
	bool                    DrawText_Gradient_Helper(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol, uint32_t nCol2, ZRect* pClip);

    void                    BuildGradient(uint32_t nColor1, uint32_t nColor2);
   
    virtual void            DrawCharNoClip(ZBuffer* pBuffer, char c, uint32_t nCol, int64_t nX, int64_t nY);
    virtual void            DrawCharClipped(ZBuffer* pBuffer, char c, uint32_t nCol, int64_t nX, int64_t nY, ZRect* pClip);
    virtual void            DrawCharGradient(ZBuffer* pBuffer, char c, int64_t nX, int64_t nY, ZRect* pClip);

    void                    FindKerning(char c1, char c2);

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

extern std::vector<std::string> gWindowsFontFacenames;


inline bool operator==(const ZFontParams& lhs, const ZFontParams& rhs)
{
    bool bEqual = 
        lhs.nHeight             == rhs.nHeight &&
        lhs.nWeight             == rhs.nWeight &&
        lhs.nTracking           == rhs.nTracking &&
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
    bool                SaveFont(const std::string& sFilename);  // overload to first ensure all glyphs have been generated

protected:
    // overloads from ZFont that will generate glyphs if needed
    virtual void        DrawCharNoClip(ZBuffer* pBuffer, char c, uint32_t nCol, int64_t nX, int64_t nY);
    virtual void        DrawCharClipped(ZBuffer* pBuffer, char c, uint32_t nCol, int64_t nX, int64_t nY, ZRect* pClip);
    virtual void        DrawCharGradient(ZBuffer* pBuffer, char c, int64_t nX, int64_t nY, ZRect* pClip);


private:

    bool                GenerateGlyph(char c);
    bool                ExtractChar(char c);
    int32_t             FindWidestNumberWidth();
    bool                RetrieveKerningPairs();
    ZRect               FindCharExtents();

    ZRect               mrScratchArea;


    HDC                 mhWinTargetDC;
    HBITMAP             mhWinTargetBitmap;
    BITMAPINFO          mDIBInfo;
    uint8_t*            mpBits;
    HDC                 mWinHDC;
    HFONT               mhWinFont;

    int32_t             mnWidestNumberWidth;

};

#endif

typedef std::shared_ptr<ZFont>  tZFontPtr;
typedef std::map< int32_t, tZFontPtr > tSizeToFont;
typedef std::map< std::string, tSizeToFont >  tNameToFontMap;


class ZFontSystem
{
public:
    ZFontSystem();
    ~ZFontSystem();

    bool        Init();        
    void        Shutdown();

    tZFontPtr   LoadFont(const std::string& sFilename);          // returns loaded font index
#ifdef _WIN64
    tZFontPtr   CreateFont(const ZFontParams& params);      // returns created font index
#endif


    std::vector<std::string>    GetFontNames();
    std::vector<int32_t>        GetAvailableSizes(const std::string& sFontName);

    tZFontPtr       GetDefaultFont(const std::string& sFontName, int32_t nFontSize);

    void            SetDefaultFontName(const std::string& sName) { msDefaultFontName = sName; }
    int32_t         GetDefaultFontCount() { return (int32_t) mNameToFontMap[msDefaultFontName].size(); }
    tZFontPtr       GetDefaultFont(int32_t nIndex);

private:
     
    tNameToFontMap  mNameToFontMap;
    std::string     msDefaultFontName;
};

extern ZFontSystem* gpFontSystem;