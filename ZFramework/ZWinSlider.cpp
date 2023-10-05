#include "ZWinSlider.h"
#include "ZBuffer.h"
#include "ZDebug.h"
#include "ZGraphicSystem.h"
#include "ZStringHelpers.h"
#include "ZGUIStyle.h"
#include "Resources.h"
#include "ZInput.h"


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
using namespace std;

ZWinSlider::ZWinSlider(int64_t* pnSliderValue)
{
    mStyle = gStyleButton;

	mpnSliderValue = pnSliderValue;	// bound value
	mnSliderUnscaledValMin = 0;
	mnSliderUnscaledValMax = 0;
    mnSliderValScalar = 1;
    mfThumbSizeRatio = 0.1;

    mBehavior = kInvalidateOnMouseUp|kDrawSliderValueOnMouseOver;
    mbInvalidateParentWhenInvalid = true;
}

ZWinSlider::~ZWinSlider()
{
}

bool ZWinSlider::Init()
{
	mbAcceptsCursorMessages = true;

	return ZWin::Init();
}

bool ZWinSlider::Shutdown()
{
	return ZWin::Shutdown();
}

void ZWinSlider::SetSliderRange(int64_t nMin, int64_t nMax, int64_t nMultiplier, double fThumbSizeRatio)
{ 
	mnSliderUnscaledValMin = nMin; 
	mnSliderUnscaledValMax = nMax; 
    mnSliderValScalar = nMultiplier;
    mfThumbSizeRatio = fThumbSizeRatio;

	ZASSERT(nMax - nMin > 0);
    limit<int64_t>(*mpnSliderValue, nMin * mnSliderValScalar, nMax * mnSliderValScalar);
    limit<double>(mfThumbSizeRatio, 0.1, 1.0);
}

bool ZWinSlider::OnMouseDownL(int64_t x, int64_t y)
{
    WindowToScaledValue(x, y);

	ZRect rThumbRect(ThumbRect());

    if (rThumbRect.PtInRect(x, y))
    {
        mMouseDownOffset.Set(x, y);
        mSliderGrabAnchor.Set(x-rThumbRect.left, y-rThumbRect.top);

        SetCapture();
        return true;
    }

	if (IsVertical())
	{
		if (y < rThumbRect.top)
			SetSliderValue((*mpnSliderValue - PageSize()));
		else
			SetSliderValue((*mpnSliderValue + PageSize()));

        if (!msSliderSetMessage.empty())
            gMessageSystem.Post(msSliderSetMessage);
    }
	else
	{
		if (x < rThumbRect.left)
			SetSliderValue((*mpnSliderValue - PageSize()));
		else
			SetSliderValue((*mpnSliderValue + PageSize()));

        if (!msSliderSetMessage.empty())
            gMessageSystem.Post(msSliderSetMessage);
    }
    mpParentWin->InvalidateChildren();
	return ZWin::OnMouseDownL(x, y);
}

bool ZWinSlider::OnMouseUpL(int64_t x, int64_t y)
{
    if (AmCapturing())
    {
        ReleaseCapture();
        mpParentWin->InvalidateChildren();
    }
    if (!msSliderSetMessage.empty())
        gMessageSystem.Post(msSliderSetMessage);

	return ZWin::OnMouseUpL(x, y);
}

int64_t ZWinSlider::WindowToScaledValue(int64_t x, int64_t y)
{
    double fNormalizedValue;
    int64_t nThumbSize = ThumbSize();
    if (IsVertical())
        fNormalizedValue = (double)(y - mAreaLocal.top - nThumbSize/2) / ((double)mAreaLocal.Height() - nThumbSize);
    else
        fNormalizedValue = (double)(x - mAreaLocal.left - nThumbSize/2) / ((double)mAreaLocal.Width() - nThumbSize);

    limit<double>(fNormalizedValue, 0.0, 1.0);

    int64_t nUnscaledValRange = mnSliderUnscaledValMax - mnSliderUnscaledValMin;
    int64_t nScaledValue = mnSliderValScalar * (mnSliderUnscaledValMin + (int64_t)((double)(nUnscaledValRange) * fNormalizedValue));
    //ZDEBUG_OUT("normalized:", fNormalizedValue, " scaled:", nScaledValue, "\n");

    return nScaledValue;
}

ZPoint ZWinSlider::ScaledValueToWindowPosition(int64_t val)
{
    ZPoint pt;
    if (mnSliderUnscaledValMax > mnSliderUnscaledValMin)
    {
        int64_t nUnscaledVal = val / mnSliderValScalar;  // from scaled to internal

        int64_t nUnscaledValRange = mnSliderUnscaledValMax - mnSliderUnscaledValMin;
        double fNormalizedValue = (double)(nUnscaledVal-mnSliderUnscaledValMin) / nUnscaledValRange;
        limit<double>(fNormalizedValue, 0.0, 1.0);
        int64_t nThumbSize = ThumbSize();
        if (IsVertical())
            pt.y = (int64_t)(mAreaLocal.top + fNormalizedValue * (double)(mAreaLocal.Height() - nThumbSize));
        else
            pt.x = (int64_t)(mAreaLocal.left + fNormalizedValue * (double)(mAreaLocal.Width() - nThumbSize));
    }

    return pt;
}


bool ZWinSlider::OnMouseMove(int64_t x, int64_t y)
{
	if (AmCapturing())
	{
        int64_t nThumbSize = ThumbSize();
        int64_t nValue = WindowToScaledValue(x-mSliderGrabAnchor.x+nThumbSize/2, y-mSliderGrabAnchor.y+nThumbSize/2);
		SetSliderValue(nValue);
        if (mBehavior & kInvalidateOnMove)
            mpParentWin->InvalidateChildren();
	}

	return ZWin::OnMouseMove(x, y);
}

bool ZWinSlider::OnMouseWheel(int64_t /*x*/, int64_t /*y*/, int64_t nDelta)
{
    // for vertical slider, wheel forward is up.... for horizontal, wheel forward is right
    if (IsVertical())
        nDelta = -nDelta;

    if (nDelta > 0)
	    SetSliderValue(*mpnSliderValue  - PageSize());
    else
        SetSliderValue(*mpnSliderValue + PageSize());

    if (mBehavior & kInvalidateOnMove)
        mpParentWin->InvalidateChildren();
    return true;
}


bool ZWinSlider::OnMouseIn() 
{ 
    if (mBehavior & kDrawSliderValueOnMouseOver)
        Invalidate();
    return ZWin::OnMouseIn(); 
}

bool ZWinSlider::OnMouseOut() 
{ 
    if (mBehavior & kDrawSliderValueOnMouseOver)
        Invalidate();
    return ZWin::OnMouseOut();
}


bool ZWinSlider::Paint()
{
	if (!mbInvalid || !mbInitted|| !mpSurface.get())
		return false;

    tZBufferPtr pThumb = mCustomSliderThumb;
    ZRect rThumbEdge = mrCustomThumbEdge;
    if (!pThumb)
    {
        if (IsVertical())
            pThumb = gSliderThumbVertical;
        else
            pThumb = gSliderThumbHorizontal;
        rThumbEdge = grSliderThumbEdge;
    }

    tZBufferPtr pBackground = mCustomSliderBackground;
    ZRect rBackgroundEdge = mrCustomBackgroundEdge;
    if (!pBackground)
    {
        pBackground = gSliderBackground;
        rBackgroundEdge = grSliderBgEdge;
    }


    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());
   
    mpSurface.get()->BltEdge(pBackground.get(), rBackgroundEdge, mAreaLocal);

	ZRect rThumb(ThumbRect());
	rThumb.OffsetRect(mAreaLocal.left, mAreaLocal.top);

    mpSurface.get()->BltEdge(pThumb.get(), rThumbEdge, rThumb);


    bool bDrawLabel = mBehavior & kDrawSliderValueAlways;
    bDrawLabel |= (mBehavior & kDrawSliderValueOnMouseOver && (gInput.mouseOverWin == this || gInput.captureWin == this));

	if (bDrawLabel)
	{
		string sLabel;
		Sprintf(sLabel, "%d", *mpnSliderValue);
        
        mStyle.Font()->DrawTextParagraph(mpSurface.get(), sLabel, rThumb, &mStyle);
	}



	return ZWin::Paint();
}

int64_t ZWinSlider::ThumbSize()
{
    int64_t nThumbSize = 0;

    tZBufferPtr pThumb = mCustomSliderThumb;
    ZRect rThumbEdge = mrCustomThumbEdge;
    if (!pThumb)
    {
        if (IsVertical())
            pThumb = gSliderThumbVertical;
        else
            pThumb = gSliderThumbHorizontal;
        rThumbEdge = grSliderThumbEdge;
    }



    if (IsVertical())
    {
        nThumbSize = (int64_t)(mAreaLocal.Height() * mfThumbSizeRatio);
        limit <int64_t>(nThumbSize, rThumbEdge.top + pThumb->GetArea().Height() - rThumbEdge.bottom, mAreaLocal.Height());
    }
    else
    {
        nThumbSize = (int64_t)(mAreaLocal.Width() * mfThumbSizeRatio);
        limit <int64_t>(nThumbSize, rThumbEdge.left + pThumb->GetArea().Width() - rThumbEdge.right, mAreaLocal.Width());
    }


    return nThumbSize;
}

ZRect ZWinSlider::ThumbRect()
{
    ZPoint sliderPt = ScaledValueToWindowPosition(*mpnSliderValue);
    int64_t nThumbSize = ThumbSize();

	if (IsVertical())
	{
		int64_t nSliderTop = sliderPt.y;
		int64_t nSliderBottom = nSliderTop + nThumbSize;
		return ZRect(0, nSliderTop, mAreaLocal.Width(), nSliderBottom);
	}

    // else horizontal
	int64_t nSliderLeft = sliderPt.x;
	int64_t nSliderRight = nSliderLeft + nThumbSize;
	return ZRect(nSliderLeft, 0, nSliderRight, mAreaLocal.Height());
}

int64_t ZWinSlider::PageSize()
{
    tZBufferPtr pThumb = mCustomSliderThumb;
    ZRect rThumbEdge = mrCustomThumbEdge;
    if (!pThumb)
    {
        if (IsVertical())
            pThumb = gSliderThumbVertical;
        else
            pThumb = gSliderThumbHorizontal;
        rThumbEdge = grSliderThumbEdge;
    }


    double fThumbGUIRatio;
    int64_t nThumbSize;
    if (IsVertical())
    {
        nThumbSize = (int64_t)(mAreaLocal.Height() * mfThumbSizeRatio);
        limit <int64_t>(nThumbSize, rThumbEdge.top + pThumb->GetArea().Height() - rThumbEdge.bottom, mAreaLocal.Height());
        fThumbGUIRatio = (double)nThumbSize / (double)mAreaLocal.Height();
    }
    else
    {
        nThumbSize = (int64_t)(mAreaLocal.Width() * mfThumbSizeRatio);
        limit <int64_t>(nThumbSize, rThumbEdge.left + pThumb->GetArea().Width() - rThumbEdge.right, mAreaLocal.Width());
        fThumbGUIRatio = (double)nThumbSize / (double)mAreaLocal.Width();
    }


    int64_t nScaledSliderPageDelta = (int64_t) (((double)(mnSliderUnscaledValMax - mnSliderUnscaledValMin) * mnSliderValScalar) * fThumbGUIRatio);
    nScaledSliderPageDelta = ((nScaledSliderPageDelta + mnSliderValScalar - 1) / mnSliderValScalar) * mnSliderValScalar;
    return nScaledSliderPageDelta;
}



void ZWinSlider::SetSliderValue(int64_t nVal)
{
    limit<int64_t>(nVal, mnSliderUnscaledValMin * mnSliderValScalar, mnSliderUnscaledValMax * mnSliderValScalar);

    if (*mpnSliderValue != nVal)
    {
        *mpnSliderValue = nVal;
        Invalidate();
    }
}

