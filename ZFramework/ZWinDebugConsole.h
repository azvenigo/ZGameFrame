#pragma once

#include "ZWin.h"
#include "ZFont.h"
#include <list>
#include "ZWinSlider.h"
#include "ZGUIStyle.h"
#include "ZDebug.h"

typedef std::list<std::string> tStringList;

class ZWinDebugConsole : public ZWin
{
public:
    ZWinDebugConsole();

    virtual bool		Init();
    virtual bool 		Paint();

    virtual void		SetScrollable(bool bScrollable = true) { mbScrollable = bScrollable; }

    void				ScrollTo(int64_t nSliderValue);		 // normalized 0.0 to 1.0
    void				UpdateScrollbar();					// Creates a scrollbar if one is needed

    virtual void		SetArea(const ZRect& newArea);
    virtual bool		OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);


    ZGUI::Style         mStyle;

protected:
    virtual bool        Process();
private:
    size_t              GetVisibleLines();


    ZWinSlider*         mpWinSlider;
    int64_t				mnSliderVal;

    size_t              mnDebugHistoryLastSeenCounter;

    ZRect        		mrDocumentArea;
    tZFontPtr           mFont;

    bool				mbScrollable;

    int64_t				mnMouseDownSliderVal;
    double				mfMouseMomentum;
};
