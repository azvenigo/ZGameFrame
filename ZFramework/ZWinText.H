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
        TooltipOnHover      = 1,        // 1   
        CloseOnMouseOut     = 1 << 1,   // 2
        CloseOnClick        = 1 << 2,   // 4
    };

    ZWinLabel() : mBehavior(None), mFillColor(0), mPosition(ZGUI::C)
    {
        mbAcceptsCursorMessages = true;
    }


    virtual bool		OnMouseDownL(int64_t x, int64_t y);
    virtual bool		OnMouseHover(int64_t x, int64_t y);
    virtual bool        Process();

    virtual void        SetBehavior(uint8_t b) { mBehavior = (eBehavior)b; }
    virtual void		SetLook(ZFontParams font, const ZTextLook& look, ZGUI::ePosition pos, bool bWrap = false, uint32_t nFillColor = 0);
    virtual void        UpdateLook(ZFontParams font);
    virtual void        UpdateLook(const ZTextLook& look);
    virtual void        UpdateLook(ZGUI::ePosition pos);
    virtual void        UpdateLook(uint32_t fillColor);


    virtual void		SetText(const std::string& text);

    virtual void		SetTooltip(const std::string& sMessage, ZFontParams* pTooltipFontParams = nullptr, ZTextLook* pTooltipLook = nullptr, bool bWrap = false, uint32_t nTooltipFill = 0);

protected:   
    virtual bool		Paint();

	std::string		    msText;
    uint32_t            mFillColor;
	tZFontPtr           mpFont;
    ZTextLook           mLook;
    ZGUI::ePosition     mPosition;
    bool                mbWrap;

    eBehavior           mBehavior;

	std::string		    msTooltipMessage;
    ZFontParams         mTooltipFontParams;
    ZTextLook           mTooltipLook;
    uint32_t            mTooltipFill;
    bool                mbTooltipWrap;
};


/*class ZWinTextEdit : public ZWin
{
    
};*/

