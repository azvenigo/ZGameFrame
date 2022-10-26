#pragma once

#include "ZStdTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZTimer.h"

/////////////////////////////////////////////////////////////////////////
// 
class Marquee : public ZWin
{
public:
    Marquee();

    void    SetText(const string& sText, uint32_t nFont = 1, uint32_t nFontCol1 = 0xffffffff, uint32_t nFontCol2 = 0xffffffff, ZFont::eStyle style = ZFont::kNormal);
    void    SetFill(uint32_t nCol = 0) { mFillColor = nCol; }
    void    SetSpeed(double fScrollPixelsPerSecond) { mfScrollPixelsPerSecond = fScrollPixelsPerSecond; }

    bool    Init();
    bool    Paint();
    bool    Process();
    bool    OnChar(char key);



private:
    uint32_t                mFillColor;

    double                  mfScrollPixelsPerSecond;
    double                  mfScrollPos;
    uint64_t                mnLastMoveTimestamp;

    string                  msText;
    uint32_t                mFontCol1;
    uint32_t                mFontCol2;
    ZFont::eStyle           mFontStyle;
    std::shared_ptr<ZFont>  mpFont;
};

