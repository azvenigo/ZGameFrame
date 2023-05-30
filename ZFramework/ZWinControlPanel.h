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


class GroupingBorder
{
public:
    GroupingBorder(const std::string& _caption = "", ZRect _area = {}, const ZGUI::Style& _style = {}) : msCaption(_caption), mArea(_area), mStyle(_style) {}

    std::string     msCaption;
    ZRect           mArea;
    ZGUI::Style     mStyle;
};

typedef std::list<GroupingBorder> tGroupingBorders;


class ZWinControlPanel : public ZWin
{
public:
    ZWinControlPanel() : mbHideOnMouseExit(false), mStyle(gDefaultDialogStyle) {}

    virtual bool        Init();

    void                FitToControls();

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
                                double fThumbSizeRatio = 0.1,
                                const std::string& sMessage = "", 
                                bool bDrawValue = false, 
                                bool bMouseOnlyDrawValue = false, 
                                tZFontPtr pFont = nullptr);

    void                AddGrouping(const std::string& sCaption, const ZRect& rArea, const ZGUI::Style& groupingStyle = gDefaultGroupingStyle);

    void                AddSpace(int64_t nSpace) { mrNextControl.OffsetRect(0,nSpace); }

    bool                Process();
    bool		        Paint();


    ZGUI::Style         mStyle;
    ZRect               mrTrigger;
    bool                mbHideOnMouseExit;

private:

    int64_t mnBorderWidth;
    int64_t mnControlHeight;
    ZRect   mrNextControl;       // area for next control to be added

    tGroupingBorders mGroupingBorders;
};


