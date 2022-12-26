#pragma once

#include "ZWin.h"
#include "ZFont.h"

extern ZRect	grSliderBgEdge;
extern ZRect	grSliderThumbEdge;


class ZSliderWin : public ZWin
{

public:

	enum eOrientation
	{
		kHorizontal = 0,
		kVertical = 1
	};

	ZSliderWin(int64_t* pnSliderValue);
	~ZSliderWin();

	bool            Init(tZBufferPtr pBufSliderThumb, ZRect rEdgeThumb, tZBufferPtr pBufSliderBackground, ZRect& rEdgeBackground, eOrientation orientation = kVertical);
	bool			Shutdown();

	virtual bool	OnMouseIn() { return ZWin::OnMouseIn(); }
	virtual bool	OnMouseOut() { return ZWin::OnMouseOut(); }

	bool			OnMouseDownL(int64_t x, int64_t y);
	bool			OnMouseUpL(int64_t x, int64_t y);
	bool			OnMouseMove(int64_t x, int64_t y);
	bool			OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);

	void			SetSliderRange(int64_t nMin, int64_t nMax, int64_t nMultiplier = 1);
	void			GetSliderRange(int64_t& nMin, int64_t& nMax) { nMin = mnSliderValMin; nMax = mnSliderValMax; }

	int64_t			GetSliderValue() { return *mpnSliderValue; }
	void			SetSliderValue(int64_t nVal);

	virtual void	SetAreaToDrawTo();

    void            SetSliderSetMessage(const std::string& sMessage) { msSliderSetMessage = sMessage; }

    void			SetDrawSliderValueFlag(bool bEnable, bool bMouseOverDrawOnly, tZFontPtr pFont);

protected:
	virtual bool	Paint();


private:
	// helper functions
//	void		SetSliderPos(int64_t nPos);
	ZRect		GetSliderThumbRect();

	void		UpdateSliderDrawBounds();


    tZBufferPtr	mpBufSliderThumb;          // not owned by this window
    tZBufferPtr	mpBufSliderBackground;     // not owned by this window

	ZRect		mrEdgeThumb;
	ZRect		mrEdgeBackground;

	int64_t		mnThumbSize;

	// Window coordinates
//	int64_t		mnSliderPos;     // center of the slider
	int64_t		mnSliderPosMin;
	int64_t		mnSliderPosMax;

	// Value
	int64_t*	mpnSliderValue;	// bound value
	int64_t		mnSliderValMin;
	int64_t		mnSliderValMax;
    int64_t     mnSliderValMultiplier;      

	double		mfMouseDownValue;

	bool		mbDrawValue;
	bool		mbMouseOverDrawOnly;
	tZFontPtr   mpFont;

    std::string msSliderSetMessage;     // on release

	eOrientation mOrientation;
};

