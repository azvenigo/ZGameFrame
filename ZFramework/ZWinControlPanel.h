#pragma once

#include "ZWin.h"
#include "ZFont.h"
#include "ZGUIHelpers.h"
#include "ZGUIStyle.h"
#include "ZWinSlider.h"
#include <map>

/////////////////////////////////////////////////////////////////////////
// 
class ZWinFormattedDoc;
class ZWinBtn;
class ZWinCheck;
class ZWinLabel;
class ZWinPopupPanelBtn;

typedef std::map<std::string, ZWinBtn*>  tIDToButtonMap;
typedef std::map<std::string, ZWinCheck*>           tIDToCheckMap;
typedef std::map<std::string, ZWinSlider*>          tIDToSliderMap;
typedef std::map<std::string, ZWinSliderF*>         tIDToSliderFMap;
typedef std::map<std::string, ZWinLabel*>           tIDToLabelMap;
typedef std::map<std::string, ZWinPopupPanelBtn*>   tIDToPopupPanelBtnMap;

class ZWinControlPanel : public ZWin
{
public:
    ZWinControlPanel();

    virtual bool            Init();

    void                    FitToControls();

    ZWinLabel*              Caption(    const std::string& sID, 
                                        const std::string& sCaption);

    ZWinBtn*     Button(     const std::string& sID, 
                                        const std::string& sCaption = "", 
                                        const std::string& sMessage = "");

    ZWinBtn*     SVGButton(  const std::string& sID, 
                                        const std::string& sSVGFilepath = "", 
                                        const std::string& sMessage = "");

    ZWinPopupPanelBtn*      PopupPanelButton(   const std::string& sID,
                                                const std::string& sSVGFilepath,
                                                const std::string& sPanelLayout,
                                                const ZFPoint& panelScaleVsBtn,
                                                const ZGUI::ePosition panelPos);

    ZWinCheck*              Toggle(     const std::string& sID,
                                        bool* pbValue,
                                        const std::string& sCaption = "", 
                                        const std::string& sCheckMessage = "", 
                                        const std::string& sUncheckMessage = "");

    ZWinSlider*             Slider(  const std::string& sID,
                                        int64_t* pnSliderValue = nullptr,
                                        int64_t nMin = 0, 
                                        int64_t nMax = 0, 
                                        int64_t nMultiplier = 1, 
                                        double fThumbSizeRatio = 0.1,
                                        const std::string& sMessage = "", 
                                        bool bDrawValue = false, 
                                        bool bMouseOnlyDrawValue = false);

    ZWinSliderF*            Slider(  const std::string& sID,
                                        float* pnSliderValue = nullptr,
                                        float nMin = 0, 
                                        float nMax = 0, 
                                        float nMultiplier = 1, 
                                        double fThumbSizeRatio = 0.1,
                                        const std::string& sMessage = "", 
                                        bool bDrawValue = false, 
                                        bool bMouseOnlyDrawValue = false);

    void                    AddSpace( int64_t nSpace) { mrNextControl.Offset(0,nSpace); }

    bool		            OnMouseOut();
    bool                    Process();
    bool		            Paint();


    ZGUI::Style             mStyle;
    ZRect                   mrTrigger;
    bool                    mbHideOnMouseExit;

    ZGUI::Style             mGroupingStyle;

    tIDToButtonMap          mButtons;
    tIDToCheckMap           mChecks;
    tIDToSliderMap          mSliders;
    tIDToSliderFMap         mSlidersF;
    tIDToLabelMap           mLabels;
    tIDToPopupPanelBtnMap   mPopupPanelButtons;

private:

    int64_t mnBorderWidth;
//    int64_t mnControlHeight;
    ZRect   mrNextControl;       // area for next control to be added
};


