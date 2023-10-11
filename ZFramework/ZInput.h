#pragma once

#include "ZTypes.h"
#include "ZMessageSystem.h"
#include "ZGUIStyle.h"
#include <map>

class ZWin;
class ZWinLabel;

class ZInput
{
public:
    ZInput();

    void    OnKeyDown(uint32_t key);
    void    OnKeyUp(uint32_t key);
    void    OnChar(uint32_t key);

    void    OnLButtonDown(int64_t x, int64_t y);
    void    OnLButtonUp(int64_t x, int64_t y);
    void    OnRButtonDown(int64_t x, int64_t y);
    void    OnRButtonUp(int64_t x, int64_t y);
    void    OnMouseMove(int64_t x, int64_t y);
    void    OnMouseWheel(int64_t x, int64_t y, int64_t delta);

    void    CheckForMouseOverNewWindow();
    void    CheckMouseForHover();

    void    Process();

    bool    IsKeyDown(uint8_t k) { return (keyState[k] & 0x80) != 0; }

    bool    ShowTooltip(const std::string& tooltip, const ZGUI::Style& style);

    uint8_t         keyState[256];
    ZPoint          lastMouseMove;
    uint64_t        lastMouseMoveTime;

    ZPoint          mouseDown;
    ZWin*           captureWin;
    ZWin*           mouseOverWin;
    ZWin*           keyboardFocusWin;

    ZWinLabel*      mpTooltipWin;
    ZPoint          toolTipAppear;

protected:
    bool            UpdateTooltipLocation(ZPoint pt);    // based on cursoe

    bool            bMouseHoverPosted;
};

extern ZInput   gInput;

