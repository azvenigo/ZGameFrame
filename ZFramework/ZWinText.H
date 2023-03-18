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
	virtual bool		OnMouseHover(int64_t x, int64_t y);

    virtual void		SetLook(tZFontPtr pFont, const ZTextLook& look, ZGUI::ePosition pos, uint32_t nFillColor = 0x000000);
    virtual void        UpdateLook(tZFontPtr pFont);
    virtual void        UpdateLook(const ZTextLook& look);
    virtual void        UpdateLook(ZGUI::ePosition pos);
    virtual void        UpdateLook(uint32_t fillColor);


    virtual void		SetText(const std::string& text);

    virtual void		SetHoverMessage(const std::string& sMessage);

protected:   
    virtual bool		Paint();

	std::string		    msText;
    uint32_t            mFillColor;
	tZFontPtr           mpFont;
    ZTextLook           mLook;
    ZGUI::ePosition     mPosition;

	std::string		    msHoverMessage;
};

