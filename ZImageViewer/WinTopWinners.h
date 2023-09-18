#pragma once

#include "ZWin.H"
#include <string>
#include "ZGUIStyle.h"
#include "ZWinFormattedText.h"
#include <list>
#include <filesystem>
#include "ImageContest.h"


class WinTopWinners : public ZWin
{
    const int64_t   kTopNEntries = 20;
public:
    WinTopWinners();
    bool            Init();
    void            UpdateUI();
    bool            OnMouseDownL(int64_t x, int64_t y);
    bool            OnMouseMove(int64_t x, int64_t y);
    bool            OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);
    bool            OnMouseOut();

    void		    SetArea(const ZRect& newArea);

    ZGUI::Style     mStyle;

    tImageMetaList* pMetaList;

    bool            mbDynamicThumbnailSizing;   // sizes thumbnails under the mouse larger....work in progress

    int64_t         mnTopNEntries;

protected:

    std::vector<ZRect> mThumbRects;

    bool            HandleMessage(const ZMessage& message);
    bool            Paint();
};
