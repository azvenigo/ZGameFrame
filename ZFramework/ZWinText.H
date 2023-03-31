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


class ZWinTextEdit : public ZWin
{
public:
    ZWinTextEdit(std::string* pText);
    virtual	bool    Init();

    virtual void    SetArea(const ZRect& area);

    virtual bool    OnChar(char c);
    virtual bool    OnKeyDown(uint32_t c);
    virtual bool    OnKeyUp(uint32_t c);


    virtual bool    OnMouseDownL(int64_t x, int64_t y);
    virtual bool    OnMouseMove(int64_t x, int64_t y);
    virtual bool    OnMouseUpL(int64_t x, int64_t y);
    virtual bool    Process();


    ZGUI::Style     mStyle;

protected:
    std::string*    mpText;

    virtual bool    Paint();
    void            StartSelecting();
    void            CancelSelecting();
    void            DeleteSelection();

    void            HandleCursorMove(int64_t newCursorPos, bool bScrollView = true);
    int64_t         FindNextBreak(int64_t nDir);
    void            ConstrainCursorPosition();
    int64_t         StyleOffset();      // if style is centered, helps compute view offset

    int64_t         WindowToTextPosition(int64_t x);

    int64_t         mCursorPosition;
    int64_t         mnViewOffset;

    ZRect           mrCursorArea;
    ZRect           mrVisibleTextArea;
    int64_t         mnLeftBoundary;
    int64_t         mnRightBoundary;
    bool            mbCursorVisible;
    bool            mbCTRLDown;
    bool            mbSHIFTDown;
    bool            mbMouseDown;

    bool            mbSelecting;
    int64_t         mnSelectionStart;
    int64_t         mnSelectionEnd;

};

