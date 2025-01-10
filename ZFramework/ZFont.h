#pragma once

#include "ZBuffer.h"
#include <string>
#include "ZTypes.h"
#include "ZDebug.h"
#include "ZGUIHelpers.h"
#include <vector>
#include "json/json.hpp"

#ifdef _WIN64
#include <windowsx.h>       // for dynamic font glyph generation
#endif

const int32_t kMaxChars = 256;

extern int64_t gM;    // gMeasure as set globally 


// Data is in this format:
// char: nAlphaValue   0-255
// char: nNumPixels at that value
//
// The list ends with two 0s
typedef std::vector<uint8_t> tPixelDataList;

struct CharDesc
{
    static const int16_t kUnknown = -32768;

    CharDesc() 
    { 
        nCharWidth = 0; 
        for (int i = 0; i < kMaxChars; i++)
            kerningArray[i] = kUnknown;
    }

    int16_t Kerning(uint8_t c2)
    {
        if (kerningArray[c2] == kUnknown)
            return 0;

        return kerningArray[c2];
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

    ZFontParams(const std::string name, int64_t _scalePoints, int64_t weight = 200, int64_t tracking = 0, int64_t fixed = 0, bool italic = false, bool symbolic = false)
    {
        assert(_scalePoints > 0 && _scalePoints < 10000.0);
        sFacename = name;
        nScalePoints = _scalePoints;
        nWeight = weight;
        nTracking = tracking;
        nFixedWidth = fixed;
        bItalic = italic;
        bSymbolic = symbolic;
    }

    ZFontParams(const std::string& sJSON)
    {
        nlohmann::json j = nlohmann::json::parse(sJSON);
        sFacename = j["n"];
        nScalePoints = j["fs"];
        nWeight = j["w"];
        nTracking = j["t"];
        nFixedWidth = j["f"];

        if (j.contains("i"))
            bItalic = j["i"];
        else
            bItalic = false;

        if (j.contains("s"))
            bSymbolic = j["s"];
        else
            bSymbolic = false;
    }

    // overloaded cast
    operator std::string() const
    {
        nlohmann::json j;
        j["n"] = sFacename.c_str();
        j["fs"] = nScalePoints;
        j["w"] = nWeight;
        j["t"] = nTracking;
        j["f"] = nFixedWidth;

        if (bItalic)
            j["i"] = bItalic;

        if (bSymbolic)
            j["s"] = bSymbolic;

        return j.dump();
    }

    int64_t Height() const 
    { 
        assert(nScalePoints > 0 && nScalePoints < 100000.0);
        return (nScalePoints * gM)/1000;
    }

    static int64_t ScalePoints(int64_t h)
    { 
        int64_t scale = (h * 1000) / gM;
        assert(scale > 0 && scale < 100000.0);
        return scale;
    }

    bool operator < (const ZFontParams& rhs) const
    {
        int nNameCompare = sFacename.compare(rhs.sFacename);
        if (nNameCompare < 0)
            return true;
        else if (nNameCompare > 0)
            return false;

        if (nScalePoints < rhs.nScalePoints)
            return true;
        else if (nScalePoints > rhs.nScalePoints)
            return false;


        if (nWeight < rhs.nWeight)
            return true;
        else if (nWeight > rhs.nWeight)
            return false;

        return false;
    }

    bool operator == (const ZFontParams& rhs) const
    {
        return (
            nScalePoints == rhs.nScalePoints &&
            nWeight == rhs.nWeight &&
            nTracking == rhs.nTracking &&
            nFixedWidth == rhs.nFixedWidth &&
            bItalic == rhs.bItalic &&
            bSymbolic == rhs.bSymbolic &&
            sFacename == rhs.sFacename);               
    }

    int64_t nScalePoints; // scaled height by ZGUI::gM    measure is scaled int 
    int64_t nWeight;
    int64_t nTracking;
    int64_t nFixedWidth;
    bool bItalic;
    bool bSymbolic;
    std::string sFacename;
};

typedef std::shared_ptr<ZFontParams> tZFontParamsPtr;

namespace ZGUI
{
    class ZTextLook;
    class Style;
};

// Under windows the DynamicFont class can generate glyphs on demand
#ifdef _WIN64
extern std::vector<std::string> gWindowsFontFacenames;
#endif

class ZFont
{
public:


public:
	ZFont();
	~ZFont();

    bool        Init(const ZFontParams& params);
    void        Shutdown();


    virtual bool    LoadFont(const std::string& sFilename);
    virtual bool    SaveFont(const std::string& sFilename);

    ZFontParams&    GetFontParams() { return mFontParams; }
    int64_t         Height() { return mFontHeight; }

    void            SetEnableKerning(bool bEnable) { mbEnableKerning = bEnable; }
    void            SetTracking(int64_t nTracking) { mFontParams.nTracking = nTracking; }

    bool            DrawText(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, ZGUI::ZTextLook* pLook = nullptr, ZRect* pClip = nullptr);
    bool            DrawText(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, ZGUI::Style* pStyle, ZRect* pClip = nullptr);
    bool            DrawTextParagraph(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, ZGUI::Style* pStyle = nullptr, ZRect* pClip = NULL);


    inline int64_t  CharWidth(uint8_t c)
    {
        if (mCharDescriptors[c].nCharWidth == 0)
            GenerateGlyph(c);

        return mCharDescriptors[c].nCharWidth;
    }

    int32_t         GetSpaceBetweenChars(uint8_t c1, uint8_t c2);
    ZRect           Arrange(ZRect rArea, const std::string& sText, ZGUI::ePosition pos, int64_t h_padding = 0, int64_t v_padding = 0);
	int64_t         StringWidth(const std::string& sText);
    ZRect           StringRect(const std::string& sText);
    ZRect           StringRect(const std::string& sText, int64_t nLineWidth);                                   // returns the rectangle required to fully render a multi-line string given a width

	int64_t         CalculateWordsThatFitInWidth(int64_t nLineWidth, const uint8_t* pChars, size_t nNumChars);  // returns the number of characters that should be drawn on this line, breaking at words
	int64_t         CalculateLettersThatFitOnLine(int64_t nLineWidth, const uint8_t* pChars, size_t nNumChars); // returns the number of characters that fit on that line
	int64_t         CalculateNumberOfLines(int64_t nLineWidth, const uint8_t* pChars, size_t nNumChars);        // returns the number of lines required to draw text
    int64_t         CalculateWidestLine(int64_t nLineWidth, const uint8_t* pChars, size_t nNumChars);           // returns the widest line that would be visible drawing the text

protected:
	bool            DrawText_Helper(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol, ZRect* pClip);
	bool            DrawText_Gradient_Helper(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol, uint32_t nCol2, ZRect* pClip);

    void            BuildGradient(uint32_t nColor1, uint32_t nColor2, std::vector<uint32_t>& gradient);

    virtual void    DrawCharNoClip(ZBuffer* pBuffer, uint8_t c, uint32_t nCol, int64_t nX, int64_t nY);
    virtual void    DrawCharClipped(ZBuffer* pBuffer, uint8_t c, uint32_t nCol, int64_t nX, int64_t nY, ZRect* pClip);
    virtual void    DrawCharGradient(ZBuffer* pBuffer, uint8_t c, std::vector<uint32_t>& gradient, int64_t nX, int64_t nY, ZRect* pClip);

    void            FindKerning(uint8_t c1, uint8_t c2);
    int64_t         StringWidth(const uint8_t* pChar, size_t length);

    ZFontParams     mFontParams;
    int64_t         mFontHeight;
    bool            mbEnableKerning;

	CharDesc        mCharDescriptors[kMaxChars];  // lower 128 ascii chars
	bool            mbInitted;

#ifdef _WIN64

private:

    bool                GenerateGlyph(uint8_t c);
    std::mutex          mGenerateGlyph;         // when held, a glyph is being generated


    bool                ExtractChar(uint8_t c);
    void                FindWidestCharacterWidth();
    bool                RetrieveKerningPairs();
    ZRect               FindCharExtents();

    ZRect               mrScratchArea;


    HDC                 mhWinTargetDC;
    HBITMAP             mhWinTargetBitmap;
    BITMAPINFO          mDIBInfo;
    uint8_t* mpBits;
    HDC                 mWinHDC;
    HFONT               mhWinFont;
    TEXTMETRICA         mWinTextMetrics;
    int32_t             mnWidestNumberWidth;
    int32_t             mnWidestCharacterWidth;
#endif
};



/*inline bool operator==(const ZFontParams& lhs, const ZFontParams& rhs)
{
    bool bEqual =
        //        lhs.nScalePoints        == rhs.nScalePoints &&
        lhs.Height() == rhs.Height() &&
        lhs.nWeight == rhs.nWeight &&
        lhs.nTracking == rhs.nTracking &&
        lhs.bItalic == rhs.bItalic &&
        lhs.bSymbolic == rhs.bSymbolic &&
        lhs.sFacename.compare(rhs.sFacename) == 0;

    return bEqual;
}*/


typedef std::shared_ptr<ZFont>              tZFontPtr;
typedef std::map<ZFontParams, tZFontPtr>    tZFontMap;
typedef std::map<int64_t, tZFontMap>        tFontHeightToZFontMap;

class ZFontSystem
{
public:
    ZFontSystem();
    ~ZFontSystem();

    bool        Init();
    void        Shutdown();


    tZFontPtr   LoadFont(const std::string& sFilename);
#ifdef _WIN64
    tZFontPtr   CreateFont(const ZFontParams& params);
#endif

    size_t          GetFontCount();

    bool            SetDefaultFont(const ZFontParams& params);
    ZFontParams&    GetDefaultFontParam() { return mpDefault->GetFontParams(); }
    tZFontPtr       GetDefaultFont() { return mpDefault; }

    tZFontPtr       GetFont(const std::string& sFontName, int32_t nFontSize);
    tZFontPtr       GetFont(const ZFontParams& params);

    // Cache related
    bool            SetCacheFolder(const std::string& sFolderPath); // returns false if folder doesn't exist
    bool            IsCached(const ZFontParams& params);
    std::string     FontCacheFilename(const ZFontParams& params);   // name to store for a given set of parameters (including folder)

private:


    tZFontPtr                   mpDefault;
    tFontHeightToZFontMap       mHeightToFontMap;
    std::recursive_mutex        mFontMapMutex;

    // caching
    bool            mbCachingEnabled;
    std::string     msCacheFolder;
};



extern ZFontSystem* gpFontSystem;