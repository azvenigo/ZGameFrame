#pragma once

#include "ZWin.h"
#include "ZWinBtn.H"
#include "ZGUIStyle.h"
#include <string>
#include <filesystem>

class ZWinSizablePushBtn;
class ZWinFormattedDoc;

class ZWinFolderSelector : public ZWin
{
public:
    enum eState : uint8_t
    {
        kCollapsed = 0,
        kFullPath = 1,
        kExpanded = 2
    };    
    
    ZWinFolderSelector();

    virtual bool            Init();
    virtual bool            Paint();

    virtual void            SetArea(const ZRect& newArea);
    virtual bool            HandleMessage(const ZMessage& message);

    virtual bool            OnMouseIn();
    virtual bool            OnMouseDownL(int64_t x, int64_t y);
    virtual bool            OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);
    virtual bool            OnMouseMove(int64_t x, int64_t y);

    ZGUI::Style             mStyle;

    std::filesystem::path   mCurPath;

    void                    SetState(eState state);

    ZRect                   VisibleArea();
    std::string             VisiblePath();
    ZRect                   PathArea();

protected:
    virtual bool            Process();

private:




    bool ScanFolder(const std::filesystem::path& folder);
    void MouseOver(int64_t x, std::filesystem::path& subPath, ZRect& rArea);

    eState mState;
    ZRect mrMouseOver;
    std::filesystem::path mMouseOverSubpath;



//    int64_t ComputeTailVisibleChars(int64_t nWidth);  // how many chars from current path can be visible in the area provided


    ZWinSizablePushBtn* mpOpenFolderBtn;
    ZWinFormattedDoc* mpFolderList;
};
