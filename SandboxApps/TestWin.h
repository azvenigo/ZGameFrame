#pragma once

#include "ZTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZTimer.h"

/////////////////////////////////////////////////////////////////////////
// 
class TestWin : public ZWin
{
public:
    TestWin();
   
   bool		Init();
   bool     Process();
   bool		Paint();
   virtual bool	OnKeyDown(uint32_t key);

   bool     OnParentAreaChange();



  
private:
    int64_t  mnSliderVal;

    ZGUI::TextBox   textBox;
};
