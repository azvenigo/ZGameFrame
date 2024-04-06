#pragma once

#include "ZTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZTimer.h"
#include "ZBuffer.h"

/////////////////////////////////////////////////////////////////////////
// 
class TextTestWin : public ZWin
{
public:
    TextTestWin();
   
    bool        Init();
    bool        Shutdown();
    bool        Paint();

   virtual bool	OnChar(char key);
   virtual bool HandleMessage(const ZMessage& message);

private:
    void        UpdateFontByParams();
    bool        LoadFont();
    bool        SaveFont();


    int32_t     mnSelectedFontIndex;
    bool        mbEnableKerning;
    ZFontParams mCustomFontParams;
    int64_t     mCustomFontHeight;
    tZFontPtr   mpFont;

    tZBufferPtr  mpBackground;

    std::string msText;

    ZGUI::Palette mPalette;
};


