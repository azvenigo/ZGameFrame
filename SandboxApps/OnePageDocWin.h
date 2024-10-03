#pragma once

#include "ZTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZBuffer.h"
#include "ZWinImage.h"
#include "ZGUIStyle.h"

/////////////////////////////////////////////////////////////////////////
// 
class OnePageDocWin : public ZWin
{
public:
    OnePageDocWin();
   
   bool         Init();
   bool         Process();
   bool         Paint();
   virtual bool	OnKeyDown(uint32_t key);

   bool         OnParentAreaChange();
  
private:
    ZWinImage*      mpWinImage;
    ZGUI::Style     mDocStyle;
};
