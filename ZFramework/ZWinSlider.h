#pragma once

#include "ZWin.h"
#include "ZFont.h"

extern ZRect	grSliderBgEdge;
extern ZRect	grSliderThumbEdge;


class ZWinSlider : public ZWin
{

public:
    enum eBehavior
    {
        kNone                       = 0,
        kInvalidateOnMouseUp        = 1,
        kInvalidateOnMove           = 2,
        kDrawSliderValueOnMouseOver = 4,
        kDrawSliderValueAlways      = 8
    };


	ZWinSlider(int64_t* pnSliderValue);
	~ZWinSlider();

	bool            Init();
	bool			Shutdown();

    // initialization
    void            SetSliderSetMessage(const std::string& sMessage) { msSliderSetMessage = sMessage; }
    void			SetSliderRange(int64_t nMin, int64_t nMax, int64_t nMultiplier = 1, double fThumbSizeRatio = 0.1);

    void			SetDrawSliderValueFlag(bool bEnable, bool bMouseOverDrawOnly, tZFontPtr pFont);

    void			GetSliderRange(int64_t& nMin, int64_t& nMax) { nMin = mnSliderUnscaledValMin; nMax = mnSliderUnscaledValMax; }
    void			SetSliderValue(int64_t nVal);



    // ZWin
    bool	        OnMouseIn();
    bool	        OnMouseOut();

	bool			OnMouseDownL(int64_t x, int64_t y);
	bool			OnMouseUpL(int64_t x, int64_t y);
	bool			OnMouseMove(int64_t x, int64_t y);
	bool			OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);

    bool            IsVertical() { return mAreaToDrawTo.Height() > mAreaToDrawTo.Width(); }

protected:
	virtual bool	Paint();

public:
    // look
    ZGUI::Style mStyle;

    std::string msSliderSetMessage;     // on release
    uint32_t    mBehavior;

private:
	// helper functions
	ZRect		ThumbRect();
    int64_t     ThumbSize();
    int64_t     PageSize();

    int64_t     WindowToScaledValue(int64_t x, int64_t y);    // uses y for vertical, x for horizontal orientation
    ZPoint      ScaledValueToWindowPosition(int64_t val);     // return y for vertical, x for horizontal


    tZBufferPtr	mCustomSliderThumb;
    tZBufferPtr	mCustomSliderBackground;
	ZRect		mrCustomThumbEdge;
	ZRect		mrCustomBackgroundEdge;

	// Value
	int64_t*	mpnSliderValue;	// bound value
	int64_t		mnSliderUnscaledValMin;
	int64_t		mnSliderUnscaledValMax;
    int64_t     mnSliderValScalar;      
    double      mfThumbSizeRatio;

    ZPoint      mSliderGrabAnchor;
};

