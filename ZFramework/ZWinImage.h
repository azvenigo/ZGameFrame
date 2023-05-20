#pragma once

#include "ZWin.h"
#include "ZBuffer.h"
#include "ZTypes.h"
#include "ZAnimObjects.h"
#include "ZWinControlPanel.h"
class ZWinImage : public ZWin
{
public:
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

    void        FitImageToWindow();

    void        SetEnableControlPanel(bool bEnable) { mbControlPanelEnabled = bEnable; }
    void        SetZoomable(bool bZoomable, double fMinZoom = 0.01, double fMaxZoom = 1000.0) { mbZoomable = bZoomable; mfMinZoom = fMinZoom; mfMaxZoom = fMaxZoom; }
    void        SetZoom(double fZoom);
    double      GetZoom();
    void        ScrollTo(int64_t nX, int64_t nY);
    void        SetMouseUpLMessage(const std::string& sMessage) { msMouseUpLMessage = sMessage; }


    void        SetFill(uint32_t nCol) { mFillColor = nCol; Invalidate(); }
    void        SetShowZoom(tZFontPtr pFont, const ZTextLook& look, ZGUI::ePosition pos, bool bShow100Also) { mbShowZoom = true; mpZoomCaptionFont = pFont; mZoomCaptionLook = look; mZoomCaptionPos = pos; mbShow100Also = bShow100Also; }
    void        SetCaption(const std::string& sCaption, tZFontPtr pFont, const ZTextLook& look, ZGUI::ePosition pos) { msCaption = sCaption; mpCaptionFont = pFont; mCaptionLook = look; mCaptionPos = pos; }
    void        SetCloseButtonMessage(const std::string& sMessage) { msCloseButtonMessage = sMessage; }
    void        SetSaveButtonMessage(const std::string& sMessage) { msSaveButtonMessage = sMessage; }


    bool        LoadImage(const std::string& sName);
    void        SetImage(tZBufferPtr pImage);
    tZBufferPtr GetImage() { return mpImage; }

    void        SetArea(const ZRect& newArea);


protected:
    bool HandleMessage(const ZMessage& message);

private:
    bool                mbScrollable;
    bool                mbZoomable;
    bool                mbControlPanelEnabled;

    int64_t             mnControlPanelButtonSide;
    int64_t             mnControlPanelFontHeight;

    double              mfZoom;
    double              mfPerfectFitZoom;
    double              mfMinZoom;
    double              mfMaxZoom;
    ZRect               mImageArea;
    std::string         msMouseUpLMessage;
    std::string         msCloseButtonMessage;
    std::string         msSaveButtonMessage;


    bool                mbShowZoom;
    bool                mbShow100Also;
    tZFontPtr           mpZoomCaptionFont;
    ZTextLook           mZoomCaptionLook;
    ZGUI::ePosition     mZoomCaptionPos;

    std::string         msCaption;
    tZFontPtr           mpCaptionFont;
    ZTextLook           mCaptionLook;
    ZGUI::ePosition     mCaptionPos;

    uint32_t            mFillColor;

    tZBufferPtr         mpImage;
    ZWinControlPanel*   mpPanel;
};
