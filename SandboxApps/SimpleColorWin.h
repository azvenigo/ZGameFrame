#pragma once

#include "ZStdTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZTimer.h"

/////////////////////////////////////////////////////////////////////////
// 
class SimpleColorWin : public ZWin
{
public:
    SimpleColorWin();

    void    SetNumChildrenToSpawn(int32_t n) { mnNumChildren = n; }

    bool    Init();
    bool    OnKeyDown(uint32_t key);
    bool    OnMouseDownL(int64_t x, int64_t y);
    bool    OnMouseDownR(int64_t x, int64_t y);
    bool    Paint();

private:
    uint32_t    mFillColor;
    int32_t     mnNumChildren;
};

