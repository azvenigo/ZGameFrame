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

    virtual bool    OnChar(char key);
    virtual bool    HandleMessage(const ZMessage& message);
    bool            OnMouseDownL(int64_t x, int64_t y);
    bool            OnMouseUpL(int64_t x, int64_t y);
    bool            OnMouseMove(int64_t x, int64_t y);
    bool            OnMouseDownR(int64_t x, int64_t y);

private:
    void            UpdateFontByParams();
    void            UpdateText();
    bool            LoadFont();
    bool            SaveFont();


    int32_t         mnSelectedFontIndex;
    bool            mbEnableKerning;
    int64_t         mCustomFontHeight;
    bool            mbViewBackground;
    tZFontPtr       mpFont;

    tZBufferPtr     mpBackground;

    ZGUI::TextBox   mTextBox;
    bool            mbViewShadow;
    float           mShadowRadius;

    bool            mDraggingTextbox;
    ZPoint          mDraggingTextboxAnchor;

    ZGUI::Palette mPalette;
};


