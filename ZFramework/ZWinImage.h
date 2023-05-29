#pragma once

#include "ZWin.h"
#include "ZBuffer.h"
#include "ZTypes.h"
#include "ZAnimObjects.h"
#include "ZWinControlPanel.h"
class ZWinImage : public ZWin
{
public:


    enum eBehavior : uint32_t
    {
        kNone                   = 0,
        kScrollable             = 1,
        kZoom                   = 1 << 1,   // 2
        kHotkeyZoom             = 1 << 2,   // 4
        kShowZoomCaption        = 1 << 3,   // 8
        kShowCaption            = 1 << 4,   // 16
        kShowControlPanel       = 1 << 5,   // 32
        kShowOnHotkey           = 1 << 6,   // 64
        kShowLoadButton         = 1 << 7,   // 128
        kShowSaveButton         = 1 << 8,   // 256
        kShowCloseButton        = 1 << 9,   // 512
    };

    enum eViewState : uint32_t
    {
        kFitToWindow            = 1,
        kZoom100                = 2,
        kZoomedInToSmallImage   = 3,
        kZoomedOutOfLargeImage  = 4,
    };


    ZWinImage();
    ~ZWinImage();

    bool        Init();

    bool        Paint();
    bool        InitFromXML(ZXMLNode* pNode);
    bool        OnMouseUpL(int64_t x, int64_t y);
    bool        OnMouseDownL(int64_t x, int64_t y);
    bool        OnMouseDownR(int64_t x, int64_t y);
    bool        OnMouseMove(int64_t x, int64_t y);
    bool        OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);
    bool        OnKeyDown(uint32_t c);
    bool        OnKeyUp(uint32_t c);


    void        FitImageToWindow();
    bool        IsSizedToWindow();

    void        SetZoom(double fZoom);
    double      GetZoom();

    void        ScrollTo(int64_t nX, int64_t nY);


    void        SetFill(uint32_t nCol) { mFillColor = nCol; Invalidate(); }
    void        SetShowZoom(const ZGUI::Style& style);
    void        SetCaption(const std::string& sCaption, const ZGUI::Style& captionStyle);


    bool        LoadImage(const std::string& sName);
    void        SetImage(tZBufferPtr pImage);

    void        SetArea(const ZRect& newArea);

    uint32_t    mBehavior;
    uint32_t    mManipulationHotkey;
    double      mfMinZoom;
    double      mfMaxZoom;
    std::string msMouseUpLMessage;
    std::string msCloseButtonMessage;
    std::string msLoadButtonMessage;
    std::string msSaveButtonMessage;

    tZBufferPtr mpImage;

protected:
    bool HandleMessage(const ZMessage& message);

private:
    int64_t             mnControlPanelButtonSide;

    double              mfZoom;
    double              mfPerfectFitZoom;
    ZRect               mImageArea;
    eViewState          mViewState;


    ZGUI::Style         mZoomStyle;
    ZGUI::Style         mCaptionStyle;

    bool                mbHotkeyActive;

    std::string         msCaption;
    uint32_t            mFillColor;

    ZWinControlPanel*   mpPanel;
};
