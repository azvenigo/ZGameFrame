#pragma once

#include "ZWin.h"
#include "ZFont.h"

extern ZRect	grSliderBgEdge;
extern ZRect	grSliderThumbEdge;


template <class T>
class tZWinSlider : public ZWin
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


    tZWinSlider(T* pnSliderValue);
	~tZWinSlider();

	bool    Init();
	bool    Shutdown();

    // initialization
    void    SetSliderSetMessage(const std::string& sMessage) { msSliderSetMessage = sMessage; }
    void    SetSliderRange(T nMin, T nMax, T nMultiplier = 1, double fThumbSizeRatio = 0.1);

    //void    SetDrawSliderValueFlag(bool bEnable, bool bMouseOverDrawOnly, tZFontPtr pFont);

    void    GetSliderRange(T& nMin, T& nMax) { nMin = mnSliderUnscaledValMin; nMax = mnSliderUnscaledValMax; }
    void    SetSliderValue(T nVal);



    // ZWin
    bool    OnMouseIn();
    bool    OnMouseOut();

	bool    OnMouseDownL(int64_t x, int64_t y);
	bool    OnMouseUpL(int64_t x, int64_t y);
	bool    OnMouseMove(int64_t x, int64_t y);
	bool    OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);

    bool    IsVertical() { return mAreaLocal.Height() > mAreaLocal.Width(); }

protected:
	bool    Paint();

public:
    // look
    ZGUI::Style mStyle;

    std::string msSliderSetMessage;     // on release
    uint32_t    mBehavior;

private:
	// helper functions
	ZRect       ThumbRect();
    int64_t     ThumbSize();
    int64_t     PageSize();

    T           WindowToScaledValue(int64_t x, int64_t y);    // uses y for vertical, x for horizontal orientation
    ZPoint      ScaledValueToWindowPosition(T val);     // return y for vertical, x for horizontal


    tZBufferPtr	mCustomSliderThumb;
    tZBufferPtr	mCustomSliderBackground;
	ZRect       mrCustomThumbEdge;
	ZRect       mrCustomBackgroundEdge;

	// Value
    T*	    mpnSliderValue;	// bound value
    T       mnSliderUnscaledValMin;
    T       mnSliderUnscaledValMax;
    T       mnSliderValScalar;
    double  mfThumbSizeRatio;

    ZPoint  mSliderGrabAnchor;
};

using ZWinSlider = tZWinSlider<int64_t>;
using ZWinSliderF = tZWinSlider<float>;