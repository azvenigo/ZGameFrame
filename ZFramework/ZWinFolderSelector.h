#pragma once

#include "ZWin.h"
#include "ZWinBtn.H"
#include "ZGUIStyle.h"
#include <string>
#include <filesystem>

class ZWinSizablePushBtn;
class ZWinFormattedDoc;

class ZWinFolderLabel : public ZWin
{
public:
    enum eBehavior : uint32_t
    {
        kNone = 0,
        kCollapsable = 1,
        kSelectable = 2,
    };


    ZWinFolderLabel();

    virtual bool            Init();
    virtual bool            Paint();

    virtual bool            HandleMessage(const ZMessage& message);

    virtual bool            OnMouseIn();
    virtual bool            OnMouseOut();
    virtual bool            OnMouseDownL(int64_t x, int64_t y);
    virtual bool            OnMouseMove(int64_t x, int64_t y);

    void                    SizeToPath();

    ZGUI::Style             mStyle;

    std::filesystem::path   mCurPath;

    std::string             VisiblePath();
    ZRect                   PathRect();

    eBehavior               mBehavior;

protected:
//    virtual bool            Process();

private:

    void MouseOver(int64_t x, std::filesystem::path& subPath, ZRect& rArea);

    ZRect mrMouseOver;
    std::filesystem::path mMouseOverSubpath;
}; 

class ZWinFolderSelector : public ZWin
{
public:
    ZWinFolderSelector();

    virtual bool            Init();
    virtual bool            Paint();

    virtual bool            HandleMessage(const ZMessage& message);

    virtual bool            OnMouseDownL(int64_t x, int64_t y);



    ZGUI::Style             mStyle;

    std::filesystem::path   mCurPath;

    bool ScanFolder(const std::filesystem::path& folder);
    static ZWinFolderSelector* Show(ZWinFolderLabel* pLabel, ZRect& rList);

protected:
//    virtual bool            Process();

private:


    ZRect mrMouseOver;
    ZRect mrList;

    tZBufferPtr mBackground;

    ZWinSizablePushBtn* mpOpenFolderBtn;
    ZWinFormattedDoc* mpFolderList;
    ZWinFolderLabel* mpLabel;
};
