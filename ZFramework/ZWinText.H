#pragma once

#include "ZWin.h"
#include <string>
#include "ZMessageSystem.h"
#include "ZFont.h"
#include "ZGUIHelpers.h"
#include "ZGUIStyle.h"

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

    ZWinLabel(uint8_t _behavior = None) : mBehavior((eBehavior)_behavior)
    {
        mbAcceptsCursorMessages = true;
    }


    virtual bool		OnMouseDownL(int64_t x, int64_t y);
    virtual bool		OnMouseHover(int64_t x, int64_t y);
    virtual bool        Process();

    virtual void        SetBehavior(uint8_t b) { mBehavior = (eBehavior)b; }

    std::string		    msText;
    ZGUI::Style         mStyle;

    std::string		    msTooltipText;
    ZGUI::Style         mStyleTooltip;

protected:   
    virtual bool		Paint();

    eBehavior           mBehavior;
};


/*class ZWinTextEdit : public ZWin
{
    
};*/

