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

    ZFontParams(const std::string name, int64_t height, int64_t weight = 200, int64_t tracking = 0, int64_t fixed = 0, bool italic = false, bool underline = false, bool strikeout = false)
    {
        sFacename = name;
        nHeight = height;
        nWeight = weight;
        nTracking = tracking;
        nFixedWidth = fixed;
        bItalic = italic;
        bUnderline = underline;
        bStrikeOut = strikeout;
    }

    ZFontParams(const std::string& sJSON)
    {
        nlohmann::json j = nlohmann::json::parse(sJSON);
        sFacename = j["n"];
        nHeight = j["h"];
        nWeight = j["w"];
        nTracking = j["t"];
        nFixedWidth = j["f"];

        if (j.contains("i"))
            bItalic = j["i"];
        else
            bItalic = false;

        if (j.contains("u"))
            bUnderline = j["u"];
        else
            bUnderline = false;

        if (j.contains("s"))
            bStrikeOut = j["s"];
        else
            bStrikeOut = false;
    }

    // overloaded cast
    operator std::string() const
    {
        nlohmann::json j;
        j["n"] = sFacename.c_str();
        j["h"] = nHeight;
        j["w"] = nWeight;
        j["t"] = nTracking;
        j["f"] = nFixedWidth;

        if (bItalic)
            j["i"] = bItalic;

        if (bUnderline)
            j["u"] = bUnderline;

        if (bStrikeOut)
            j["s"] = bStrikeOut;

        return j.dump();
    }

    bool operator < (const ZFontParams& rhs) const
    {
        int nNameCompare = sFacename.compare(rhs.sFacename);
        if (nNameCompare < 0)
            return true;
        else if (nNameCompare > 0)
            return false;

        if (nHeight < rhs.nHeight)
            return true;
        else if (nHeight > rhs.nHeight)
            return false;

        if (nWeight < rhs.nWeight)
            return true;
        else if (nWeight > rhs.nWeight)
            return false;

        return false;
    }

    int64_t nHeight;
    int64_t nWeight;
    int64_t nTracking;
    int64_t nFixedWidth;
    bool bItalic;
    bool bUnderline;
    bool bStrikeOut;
    std::string sFacename;
};

class ZTextLook
{
public:
    enum eStyle : uint8_t
    {
        kNormal = 0,
        kShadowed = 1,
        kEmbossed = 2
    };

    ZTextLook(eStyle _style = kNormal, uint32_t _colTop = 0xffffffff, uint32_t _colBottom = 0xffffffff) { style = _style; colTop = _colTop; colBottom = _colBottom; }

    uint32_t colTop;
    uint32_t colBottom;
    eStyle   style;
};

class ZFont
{
public:


public:
	ZFont();
	~ZFont();

    virtual bool    LoadFont(const std::string& sFilename);
    virtual bool    SaveFont(const std::string& sFilename);

    ZFontParams&    GetFontParams() { return mFontParams; }
    int64_t         Height() { return mFontParams.nHeight; }

    void            SetEnableKerning(bool bEnable) { mbEnableKerning = bEnable; }
    void            SetTracking(int64_t nTracking) { mFontParams.nTracking = nTracking; }

    bool            DrawText(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, const ZTextLook& look = {}, ZRect* pClip = NULL);
    bool            DrawTextParagraph(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, const ZTextLook& look = {}, ZGUI::ePosition pos = ZGUI::LT, ZRect* pClip = NULL);


    int64_t         CharWidth(uint8_t c);
    int32_t         GetSpaceBetweenChars(uint8_t c1, uint8_t c2);
    ZRect           GetOutputRect(ZRect rArea, const uint8_t* pChars, size_t nNumChars, ZGUI::ePosition pos, int64_t nPadding = 0);


	int64_t         StringWidth(const std::string& sText);
	int64_t         CalculateWordsThatFitOnLine(int64_t nLineWidth, const uint8_t* pChars, size_t nNumChars);  // returns the number of characters that should be drawn on this line, breaking at words
	int64_t         CalculateLettersThatFitOnLine(int64_t nLineWidth, const uint8_t* pChars, size_t nNumChars);	// returns the number of characters that fit on that line
	int64_t         CalculateNumberOfLines(int64_t nLineWidth, const uint8_t* pChars, size_t nNumChars);	// returns the number of lines required to draw text


protected:
	bool                    DrawText_Helper(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol, ZRect* pClip);
	bool                    DrawText_Gradient_Helper(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, uint32_t nCol, uint32_t nCol2, ZRect* pClip);

    void                    BuildGradient(uint32_t nColor1, uint32_t nColor2, std::vector<uint32_t>& gradient);
   
    virtual void            DrawCharNoClip(ZBuffer* pBuffer, uint8_t c, uint32_t nCol, int64_t nX, int64_t nY);
    virtual void            DrawCharClipped(ZBuffer* pBuffer, uint8_t c, uint32_t nCol, int64_t nX, int64_t nY, ZRect* pClip);
    virtual void            DrawCharGradient(ZBuffer* pBuffer, uint8_t c, std::vector<uint32_t>& gradient, int64_t nX, int64_t nY, ZRect* pClip);

    void                    FindKerning(uint8_t c1, uint8_t c2);

    ZFontParams             mFontParams;
    bool                    mbEnableKerning;

	sCharDescriptor         mCharDescriptors[kMaxChars];  // lower 128 ascii chars
	bool                    mbInitted;
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

    bool                Init(const ZFontParams& params, bool bInitGlyphs = true, bool bKearn = true);
    bool                SaveFont(const std::string& sFilename);  // overload to first ensure all glyphs have been generated

    bool                GenerateSymbolicGlyph(uint8_t c, uint32_t symbol);

protected:
    // overloads from ZFont that will generate glyphs if needed
    virtual void        DrawCharNoClip(ZBuffer* pBuffer, uint8_t c, uint32_t nCol, int64_t nX, int64_t nY);
    virtual void        DrawCharClipped(ZBuffer* pBuffer, uint8_t c, uint32_t nCol, int64_t nX, int64_t nY, ZRect* pClip);
    virtual void        DrawCharGradient(ZBuffer* pBuffer, uint8_t c, std::vector<uint32_t>& gradient, int64_t nX, int64_t nY, ZRect* pClip);

private:

    bool                GenerateGlyph(uint8_t c);

    bool                ExtractChar(uint8_t c);
    int32_t             FindWidestNumberWidth();
    int32_t             FindWidestCharacterWidth();
    bool                RetrieveKerningPairs();
    ZRect               FindCharExtents();

    ZRect               mrScratchArea;


    HDC                 mhWinTargetDC;
    HBITMAP             mhWinTargetBitmap;
    BITMAPINFO          mDIBInfo;
    uint8_t*            mpBits;
    HDC                 mWinHDC;
    HFONT               mhWinFont;
    TEXTMETRICA         mWinTextMetrics;
    int32_t             mnWidestNumberWidth;
    int32_t             mnWidestCharacterWidth;

};

#endif


#define REPLACE_FONT_SYSTEM_NEW_PARAM_BASED_MAPPING


#ifdef REPLACE_FONT_SYSTEM_NEW_PARAM_BASED_MAPPING

typedef std::shared_ptr<ZFont>              tZFontPtr;
typedef std::map<ZFontParams, tZFontPtr>    tZFontMap;

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

//    std::vector<std::string>    GetFontNames();
//    std::vector<int32_t>        GetAvailableSizes(const std::string& sFontName);

    size_t          GetFontCount() { return mFontMap.size(); }

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


    tZFontPtr       mpDefault;
    tZFontMap       mFontMap;

    // caching
    std::string     msCacheFolder;
};


#else

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

#endif

extern ZFontSystem* gpFontSystem;