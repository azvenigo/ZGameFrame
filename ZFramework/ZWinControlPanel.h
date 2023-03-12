#pragma once

#include "ZWin.h"
#include "ZFont.h"
#include "ZGUIHelpers.h"

/////////////////////////////////////////////////////////////////////////
// 
class ZFormattedTextWin;
class ZWinSizablePushBtn;
class ZWinCheck;
class ZSliderWin;

class ZWinControlPanel : public ZWin
{
public:
    ZWinControlPanel() : mbHideOnMouseExit(false) {}

    virtual bool        Init();

    void                SetTriggerRect(const ZRect& rTrigger) { mrTrigger.SetRect(rTrigger); }

    void                SetHideOnMouseExit(bool bHideOnMouseExit) { mbHideOnMouseExit = bHideOnMouseExit; }

    ZFormattedTextWin*  AddCaption( const std::string& sCaption,
                                const ZFontParams& fontParams, 
                                const ZTextLook& look = {ZTextLook::kNormal, 0xffffffff, 0xffffffff},
                                ZGUI::ePosition = ZGUI::Center,
                                uint32_t fillColor = 0x00000000);

    ZWinSizablePushBtn* AddButton(  const std::string& sCaption,
                                const std::string& sMessage, 
                                tZFontPtr pFont, 
                                const ZTextLook& look = { ZTextLook::kShadowed, 0xffffffff, 0xffffffff });

    ZWinCheck*          AddToggle(  bool* pbValue,
                                const std::string& sCaption, 
                                const std::string& sCheckMessage, 
                                const std::string& sUncheckMessage,         
                                const std::string& sRadioGroup,             // If part of a radio group, all others in the group get auto unchecked
                                tZFontPtr pFont, 
                                const ZTextLook& checkedLook   = {ZTextLook::kNormal, 0xff000000, 0xff000000},
                                const ZTextLook& uncheckedLook = {ZTextLook::kNormal, 0xff000000, 0xff000000});

    ZSliderWin*         AddSlider(  int64_t* pnSliderValue,
                                int64_t nMin, 
                                int64_t nMax, 
                                int64_t nMultiplier, 
                                const std::string& sMessage = "", 
                                bool bDrawValue = false, 
                                bool bMouseOnlyDrawValue = false, 
                                tZFontPtr pFont = nullptr);

    void                AddSpace(int64_t nSpace) { mrNextControl.OffsetRect(0,nSpace); }

    bool                Process();
    bool		        Paint();


private:
    ZRect   mrTrigger;

    int64_t mnBorderWidth;
    int64_t mnControlHeight;
    ZRect   mrNextControl;       // area for next control to be added
    bool    mbHideOnMouseExit;
};


