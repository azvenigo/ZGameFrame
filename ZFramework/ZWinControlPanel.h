#pragma once

#include "ZWin.h"
#include "ZFont.h"
#include "ZGUIHelpers.h"
#include "ZGUIStyle.h"

/////////////////////////////////////////////////////////////////////////
// 
class ZWinFormattedText;
class ZWinSizablePushBtn;
class ZWinCheck;
class ZWinSlider;
class ZWinLabel;

class ZWinControlPanel : public ZWin
{
public:
    ZWinControlPanel() : mbHideOnMouseExit(false), mStyle(gDefaultDialogStyle) {}

    virtual bool        Init();

    void                SetTriggerRect(const ZRect& rTrigger) { mrTrigger.SetRect(rTrigger); }

    void                FitToControls();

    void                SetHideOnMouseExit(bool bHideOnMouseExit) { mbHideOnMouseExit = bHideOnMouseExit; }

    ZWinLabel*          AddCaption(const std::string& sCaption, const ZGUI::Style& style = gStyleCaption);
    ZWinSizablePushBtn* AddButton(  const std::string& sCaption, const std::string& sMessage, const ZGUI::Style& style = gStyleButton);

    ZWinCheck*          AddToggle(  bool* pbValue,
                                const std::string& sCaption, 
                                const std::string& sCheckMessage, 
                                const std::string& sUncheckMessage,         
                                const std::string& sRadioGroup,             // If part of a radio group, all others in the group get auto unchecked
                                const ZGUI::Style& checkedStyle = gStyleToggleChecked,
                                const ZGUI::Style& uncheckedStyle = gStyleToggleUnchecked);

    ZWinSlider*         AddSlider(  int64_t* pnSliderValue,
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


    ZGUI::Style         mStyle;

private:
    ZRect   mrTrigger;

    int64_t mnBorderWidth;
    int64_t mnControlHeight;
    ZRect   mrNextControl;       // area for next control to be added
    bool    mbHideOnMouseExit;
};


