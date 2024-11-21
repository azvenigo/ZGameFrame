#pragma once

#include "ZTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZTimer.h"

/////////////////////////////////////////////////////////////////////////
// 
class OverlayWin : public ZWin
{
public:
    OverlayWin();
   
   bool Init();
   bool Paint();

   bool     OnParentAreaChange();
  
private:
};
