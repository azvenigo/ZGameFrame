#pragma once

#include "ZStdTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZTimer.h"

/////////////////////////////////////////////////////////////////////////
// 
class ImageTestWin : public ZWin
{
public:
    ImageTestWin();

    bool    Init();
    bool    OnKeyDown(uint32_t key);
    bool    Paint();

private:
};

