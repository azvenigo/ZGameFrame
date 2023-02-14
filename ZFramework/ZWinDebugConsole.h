#pragma once

#include "ZWin.h"
#include "ZFont.h"
#include <list>
#include "ZSliderWin.h"
#include "ZStdDebug.h"

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

protected:
    virtual bool        Process();
private:
    size_t              GetVisibleLines();


    ZSliderWin*         mpSliderWin;
    int64_t				mnSliderVal;

    size_t              mnDebugHistoryLastSeenCounter;

    ZRect        		mrTextArea;
    tZFontPtr           mFont;

    uint32_t            mTextCol;

    bool				mbScrollable;

    int64_t				mnMouseDownSliderVal;
    double				mfMouseMomentum;
};
