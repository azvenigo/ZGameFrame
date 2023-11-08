#pragma once

#include "ZWin.h"
#include "ZFont.h"
#include "ZGUIHelpers.h"
#include "ZGUIStyle.h"
#include <map>

/////////////////////////////////////////////////////////////////////////
// 
class ZWinFormattedDoc;
class ZWinSizablePushBtn;
class ZWinCheck;
class ZWinSlider;
class ZWinLabel;

typedef std::map<std::string, ZWinSizablePushBtn*> tIDToButtonMap;

class ZWinControlPanel : public ZWin
{
public:
    ZWinControlPanel();

    virtual bool        Init();

    void                FitToControls();

    ZWinLabel*          AddCaption(const std::string& sCaption);

    ZWinSizablePushBtn* Button(const std::string& sID, const std::string& sCaption = "", const std::string& sMessage = "");

    ZWinSizablePushBtn* SVGButton(const std::string& sID, const std::string& sSVGFilepath = "", const std::string& sMessage = "");

    ZWinCheck*          AddToggle(  bool* pbValue,
                                const std::string& sCaption, 
                                const std::string& sCheckMessage = "", 
                                const std::string& sUncheckMessage = "");

    ZWinSlider*         AddSlider(  int64_t* pnSliderValue,
                                int64_t nMin, 
                                int64_t nMax, 
                                int64_t nMultiplier, 
                                double fThumbSizeRatio = 0.1,
                                const std::string& sMessage = "", 
                                bool bDrawValue = false, 
                                bool bMouseOnlyDrawValue = false);

    void                AddSpace(int64_t nSpace) { mrNextControl.OffsetRect(0,nSpace); }

    bool		        OnMouseOut();
    bool                Process();
    bool		        Paint();


    ZGUI::Style         mStyle;
    ZRect               mrTrigger;
    bool                mbHideOnMouseExit;

    ZGUI::Style         mGroupingStyle;

    tIDToButtonMap      mButtons;

private:

    int64_t mnBorderWidth;
//    int64_t mnControlHeight;
    ZRect   mrNextControl;       // area for next control to be added
};


