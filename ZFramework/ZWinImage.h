#pragma once

#include "ZWin.h"
#include "ZBuffer.h"
#include "ZTypes.h"
#include "ZAnimObjects.h"
#include "ZGUIElements.h"

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
        kShowCaptionOnMouseOver = 1 << 5,   // 32
        kSelectableArea         = 1 << 6,   // 64
        kNotifyOnClick          = 1 << 7,   // 128
        kLaunchGeolocation      = 1 << 8,   // 256
    };

    enum eViewState : uint32_t
    {
        kNoState                = 0,
        kFitToWindow            = 1,
        kZoom100                = 2,
        kZoomedInToSmallImage   = 3,
        kZoomedOutOfLargeImage  = 4,
    };

    ZWinImage();
    ~ZWinImage();

    bool        Init();
    void        Clear();

    bool        Paint();
    bool        OnMouseUpL(int64_t x, int64_t y);
    bool        OnMouseDownL(int64_t x, int64_t y);
    bool        OnMouseDownR(int64_t x, int64_t y);
    bool        OnMouseMove(int64_t x, int64_t y);
    bool        OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);
    bool        OnKeyDown(uint32_t c);
    bool        OnKeyUp(uint32_t c);


    void        FitImageToWindow();
    void        CenterImage();
    bool        IsSizedToWindow();

    void        SetZoom(double fZoom);
    double      GetZoom();

    void        ScrollTo(int64_t nX, int64_t nY);

    bool        LoadImage(const std::string& sName);
    void        SetImage(tZBufferPtr pImage);

    ZRect       GetSelection();   // in window
    void        ClearSelection();


    void        SetArea(const ZRect& newArea);
    bool        OnParentAreaChange();

    uint32_t    mBehavior;
    uint32_t    mZoomHotkey;
    double      mfMinZoom;
    double      mfMaxZoom;
    std::string msMouseUpLMessage;
    std::string msCloseButtonMessage;
    std::string msLoadButtonMessage;
    std::string msSaveButtonMessage;

    tZBufferPtr mpImage;
    uint32_t    nSubsampling;


    ZGUI::tTextboxMap   mCaptionMap;
    ZGUI::ZTable*       mpTable;

    uint32_t            mFillColor;


protected:
    bool HandleMessage(const ZMessage& message);
    void ToImageCoords(ZRect& r);   // compute image coords


private:
    double              mfZoom;
    double              mfPerfectFitZoom;
    ZRect               mImageArea;
    ZRect               mrSelection;
    eViewState          mViewState;
};
