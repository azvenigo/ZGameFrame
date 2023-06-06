#pragma once

#include "ZTypes.h"
#include "ZMessageSystem.h"
#include <map>

class ZWin;

class ZInput
{
public:
    ZInput() : captureWin(nullptr), keyboardFocusWin(nullptr), lastMouseMoveTime(0)
    {
    }

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


    uint8_t         keyState[256];
    ZPoint          lastMouseMove;
    uint64_t        lastMouseMoveTime;

    ZPoint          mouseDown;
    ZWin*           captureWin;
    ZWin*           mouseOverWin;
    ZWin*           keyboardFocusWin;

protected:
    bool            bMouseHoverPosted;
};

extern ZInput   gInput;

