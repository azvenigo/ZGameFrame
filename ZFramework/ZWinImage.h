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

    void        SetZoom(double fZoom);
    double      GetZoom();

    void        ScrollTo(int64_t nX, int64_t nY);
    void        SetMouseUpLMessage(const std::string& sMessage) { msMouseUpLMessage = sMessage; }


    void        SetFill(uint32_t nCol) { mFillColor = nCol; Invalidate(); }
    void        SetShowZoom(const ZGUI::Style& style);
    void        SetCaption(const std::string& sCaption, const ZGUI::Style& captionStyle);
    void        SetCloseButtonMessage(const std::string& sMessage) { msCloseButtonMessage = sMessage; }
    void        SetSaveButtonMessage(const std::string& sMessage) { msSaveButtonMessage = sMessage; }


    bool        LoadImage(const std::string& sName);
    void        SetImage(tZBufferPtr pImage);
    tZBufferPtr GetImage() { return mpImage; }

    void        SetArea(const ZRect& newArea);


    uint32_t    mBehavior;
    uint32_t    mManipulationHotkey;
    double      mfMinZoom;
    double      mfMaxZoom;

protected:
    bool HandleMessage(const ZMessage& message);

private:
    //bool                mbScrollable;
    //bool                mbZoomable;
    //bool                mbControlPanelEnabled;

    int64_t             mnControlPanelButtonSide;
    int64_t             mnControlPanelFontHeight;

    double              mfZoom;
    double              mfPerfectFitZoom;
    ZRect               mImageArea;
    std::string         msMouseUpLMessage;
    std::string         msCloseButtonMessage;
    std::string         msSaveButtonMessage;


    //bool                mbShowZoom;
    //bool                mbShow100Also;
//    tZFontPtr           mpZoomCaptionFont;
//    ZTextLook           mZoomCaptionLook;
//    ZGUI::ePosition     mZoomCaptionPos;
    ZGUI::Style         mZoomStyle;
    ZGUI::Style         mCaptionStyle;

    bool                mbHotkeyActive;
    //bool                mbManipulate;

    std::string         msCaption;
//    tZFontPtr           mpCaptionFont;
//    ZTextLook           mCaptionLook;
//    ZGUI::ePosition     mCaptionPos;

    uint32_t            mFillColor;

    tZBufferPtr         mpImage;
    ZWinControlPanel*   mpPanel;
};
