#pragma once

#include "ZWin.h"
#include "ZFont.h"


enum WatchType
{
    kLabel = 0,
    kString = 1,
    kInt64 = 2,
    kDouble = 3,
    kBool = 4
};

class WatchStruct
{
public:
    WatchStruct()
    {
        mType = kLabel;
        pMem = nullptr;
        
    }

    string      mCaption;
    WatchType   mType;
    void*       pMem;

    uint32_t    nFont;
    uint32_t    nCaptionCol;
    uint32_t    nWatchCol;
    ZRect       mArea;
    ZFont::eStyle   mStyle;
};

typedef std::list<WatchStruct> tWatchList;


/////////////////////////////////////////////////////////////////////////
// 
class ZWinWatchPanel : public ZWin
{
public:
    ZWinWatchPanel();

    virtual bool    Init();

    bool            AddItem(WatchType type, const string& sCaption, void* pWatchAddress = nullptr, uint32_t nFont = 1, uint32_t nCaptionCol = 0xffaaaaaa, uint32_t nWatchFont = 0xffffffff, ZFont::eStyle style = ZFont::kNormal);
    void            AddSpace(int64_t nSpace) { mrNextControl.OffsetRect(0,nSpace); }

    bool            Process();
    bool		    Paint();


private:
    tWatchList      mWatchList;

    std::vector<string>     mWatchedStrings;
    std::vector<int64_t>    mWatchedInt64s;
    std::vector<double>     mWatchedDoubles;
    std::vector<bool>       mWatchedBools;


    int64_t         mnBorderWidth;
    int64_t         mnControlHeight;
    ZRect           mrNextControl;       // area for next control to be added
};


