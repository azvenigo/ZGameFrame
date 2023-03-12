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

    std::string     mCaption;
    WatchType       mType;
    void*           pMem;

    ZTextLook       captionLook;
    ZTextLook       textLook;

    ZRect           mArea;
    ZTextLook       mLook;
};

typedef std::list<WatchStruct> tWatchList;


/////////////////////////////////////////////////////////////////////////
// 
class ZWinWatchPanel : public ZWin
{
public:
    ZWinWatchPanel();

    virtual bool    Init();

    bool            AddItem(WatchType type, const std::string& sCaption, void* pWatchAddress = nullptr, const ZTextLook& captionlook = {}, const ZTextLook& textlook = {});
    void            AddSpace(int64_t nSpace) { mrNextControl.OffsetRect(0,nSpace); }

    bool            Process();
    bool		    Paint();


private:
    tWatchList      mWatchList;

    std::vector<std::string>     mWatchedStrings;
    std::vector<int64_t>    mWatchedInt64s;
    std::vector<double>     mWatchedDoubles;
    std::vector<bool>       mWatchedBools;


    int64_t         mnBorderWidth;
    int64_t         mnControlHeight;
    ZRect           mrNextControl;       // area for next control to be added
};


