#pragma once

#include "ZWin.h"
#include "ZBuffer.h"
#include "ZStdTypes.h"
#include "ZAnimObjects.h"
class ZImageWin : public ZWin
{
public:
    ZImageWin();
    ~ZImageWin();

    bool        Init();

    bool        Paint();
    bool        InitFromXML(ZXMLNode* pNode);
    bool        OnMouseUpL(int64_t x, int64_t y);
    bool        OnMouseDownL(int64_t x, int64_t y);
    bool        OnMouseDownR(int64_t x, int64_t y);
    bool        OnMouseMove(int64_t x, int64_t y);
    bool        OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);

    void        FitImageToWindow();
    void        SetZoomable(bool bZoomable, double fMinZoom = 0.01, double fMaxZoom = 1000.0) { mbZoomable = bZoomable; mfMinZoom = fMinZoom; mfMaxZoom = fMaxZoom; }
    void        SetZoom(double fZoom);
    double      GetZoom();
    void        ScrollTo(int64_t nX, int64_t nY);

    void        SetFill(uint32_t nCol) { mFillColor = nCol; Invalidate(); }
    void        SetShowZoom(int32_t nFontID, uint32_t nCol, ZFont::ePosition pos, bool bShow100Also) { mbShowZoom = true; mZoomCaptionFontID = nFontID; mZoomCaptionColor = nCol; mZoomCaptionPos = pos; mbShow100Also = bShow100Also; }
    void        SetCaption(const std::string& sCaption, int32_t nFontID, uint32_t nCol, ZFont::ePosition pos) { msCaption = sCaption; mCaptionFontID = nFontID; mnCaptionCol = nCol; mCaptionPos = pos; }
    void        SetOnClickMessage(const std::string& sMessage) { msOnClickMessage = sMessage; mbAcceptsCursorMessages = !sMessage.empty(); }

    void        LoadImage(const std::string& sName);
    void        SetImage(ZBuffer* pImage);

private:
    bool        mbScrollable;
    bool        mbZoomable;

    double      mfZoom;
    double      mfPerfectFitZoom;
    double      mfMinZoom;
    double      mfMaxZoom;
    ZRect       mImageArea;


    bool                mbShowZoom;
    bool                mbShow100Also;
    int32_t             mZoomCaptionFontID;
    uint32_t            mZoomCaptionColor;
    ZFont::ePosition    mZoomCaptionPos;

    std::string         msCaption;
    int32_t             mCaptionFontID;
    ZFont::ePosition    mCaptionPos;
    uint32_t            mnCaptionCol;
    std::string         msOnClickMessage;

    uint32_t            mFillColor;

	ZBuffer*            mpImage;
};
