#pragma once

#include "ZWin.h"
#include <string>
#include "ZMessageSystem.h"
#include "ZFont.h"
#include "ZGUIHelpers.h"

class ZBuffer;
class ZFont;

class ZWinLabel : public ZWin
{
public:
    enum eBehavior : uint8_t
    {
        None                = 0,
        PopupOnHover        = 1,        // 1   
        CloseOnMouseOut     = 1 << 1,   // 2
        CloseOnClick        = 1 << 2,   // 4
    };

    ZWinLabel() : mBehavior(None), mFillColor(0), mPosition(ZGUI::C) {}


    virtual bool		OnMouseDownL(int64_t x, int64_t y);
    virtual bool		OnMouseHover(int64_t x, int64_t y);
    virtual bool        Process();

    virtual void        SetBehavior(eBehavior b) { mBehavior = b; }
    virtual void		SetLook(ZFontParams font, const ZTextLook& look, ZGUI::ePosition pos, uint32_t nFillColor = 0);
    virtual void        UpdateLook(ZFontParams font);
    virtual void        UpdateLook(const ZTextLook& look);
    virtual void        UpdateLook(ZGUI::ePosition pos);
    virtual void        UpdateLook(uint32_t fillColor);


    virtual void		SetText(const std::string& text);

    virtual void		SetHoverMessage(const std::string& sMessage, ZFontParams* pHoverFontParams = nullptr, ZTextLook* pHoverLook = nullptr, uint32_t nHoverFill = 0);

protected:   
    virtual bool		Paint();

	std::string		    msText;
    uint32_t            mFillColor;
	tZFontPtr           mpFont;
    ZTextLook           mLook;
    ZGUI::ePosition     mPosition;

    eBehavior           mBehavior;

	std::string		    msHoverMessage;
    ZFontParams         mHoverFontParams;
    ZTextLook           mHoverLook;
    uint32_t            mHoverFill;
};


/*class ZWinTextEdit : public ZWin
{
    
};*/

