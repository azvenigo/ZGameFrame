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

    ZFontParams(const std::string name, float _fScale, int64_t weight = 200, int64_t tracking = 0, int64_t fixed = 0, bool italic = false, bool symbolic = false)
    {
        assert(_fScale > 0 && _fScale < 10.0);
        sFacename = name;
        fScale = _fScale;
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
        fScale = j["fs"];
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
        j["fs"] = fScale;
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
        assert(fScale > 0 && fScale < 100.0);
        return (int64_t)(fScale * (float)gM);
    }

    static float Scale(int64_t h) 
    { 
        float scale = truncate<float>(((float)h / (float)gM), 3);
        assert(scale > 0 && scale < 100.0);
        return scale;
    }

    bool operator < (const ZFontParams& rhs) const
    {
        int nNameCompare = sFacename.compare(rhs.sFacename);
        if (nNameCompare < 0)
            return true;
        else if (nNameCompare > 0)
            return false;

        if (fScale < rhs.fScale)
            return true;
        else if (fScale > rhs.fScale)
            return false;


        if (nWeight < rhs.nWeight)
            return true;
        else if (nWeight > rhs.nWeight)
            return false;

        return false;
    }

    float  fScale; // scaled height by ZGUI::gM    measure
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

class ZFont
{
public:


public:
	ZFont();
	~ZFont();

    virtual bool    LoadFont(const std::string& sFilename);
    virtual bool    SaveFont(const std::string& sFilename);

    ZFontParams&    GetFontParams() { return mFontParams; }
    int64_t         Height() { return mFontHeight; }

    void            SetEnableKerning(bool bEnable) { mbEnableKerning = bEnable; }
    void            SetTracking(int64_t nTracking) { mFontParams.nTracking = nTracking; }

    bool            DrawText(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, ZGUI::ZTextLook* pLook = nullptr, ZRect* pClip = nullptr);
    bool            DrawText(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, ZGUI::Style* pStyle, ZRect* pClip = nullptr);
    bool            DrawTextParagraph(ZBuffer* pBuffer, const std::string& sText, const ZRect& rAreaToDrawTo, ZGUI::Style* pStyle = nullptr, ZRect* pClip = NULL);


    int64_t         CharWidth(uint8_t c);
    int32_t         GetSpaceBetweenChars(uint8_t c1, uint8_t c2);
    ZRect           Arrange(ZRect rArea, const uint8_t* pChars, size_t nNumChars, ZGUI::ePosition pos, int64_t nPadding = 0);
	int64_t         StringWidth(const std::string& sText);
    ZRect           StringRect(const std::string& sText);

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
    int64_t                 mFontHeight;
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
        lhs.fScale              == rhs.fScale &&
        lhs.nWeight             == rhs.nWeight &&
        lhs.nTracking           == rhs.nTracking &&
        lhs.bItalic             == rhs.bItalic &&
        lhs.bSymbolic           == rhs.bSymbolic &&
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

//    std::vector<std::string>    GetFontNames();
//    std::vector<int32_t>        GetAvailableSizes(const std::string& sFontName);

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