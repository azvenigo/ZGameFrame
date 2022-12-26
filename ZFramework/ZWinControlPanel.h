#pragma once

#include "ZWin.h"
#include "ZFont.h"

/////////////////////////////////////////////////////////////////////////
// 
class ZWinControlPanel : public ZWin
{
public:
    ZWinControlPanel() : mbHideOnMouseExit(false) {}

    virtual bool    Init();

    void            SetTriggerRect(const ZRect& rTrigger) { mrTrigger.SetRect(rTrigger); }

    void            SetHideOnMouseExit(bool bHideOnMouseExit) { mbHideOnMouseExit = bHideOnMouseExit; }

    bool            AddCaption(const std::string& sCaption, ZFontParams& fontParams, uint32_t nFontCol = 0xff000000, uint32_t nFillCol = 0xffffffff, ZFont::eStyle style = ZFont::kNormal, ZFont::ePosition = ZFont::kMiddleCenter);
    bool            AddButton(const std::string& sCaption, const std::string& sMessage, tZFontPtr pFont, uint32_t nFontCol1 = 0xffffffff, uint32_t nFontCol2 = 0xffbbbbbb, ZFont::eStyle style = ZFont::kShadowed);
    bool            AddToggle(bool* pbValue, const std::string& sCaption, const std::string& sCheckMessage, const std::string& sUncheckMessage, tZFontPtr pFont, uint32_t nFontCol = 0xff000000, ZFont::eStyle style = ZFont::kNormal);
    bool            AddSlider(int64_t* pnSliderValue, int64_t nMin, int64_t nMax, int64_t nMultiplier, const std::string& sMessage = "", bool bDrawValue = false, bool bMouseOnlyDrawValue = false, tZFontPtr pFont = nullptr);
    void            AddSpace(int64_t nSpace) { mrNextControl.OffsetRect(0,nSpace); }

    bool            Process();
    bool		    Paint();


private:
    ZRect           mrTrigger;

    int64_t         mnBorderWidth;
    int64_t         mnControlHeight;
    ZRect           mrNextControl;       // area for next control to be added
    bool            mbHideOnMouseExit;
};


