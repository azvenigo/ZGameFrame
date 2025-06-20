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

template class tZWinSlider<int64_t>;
template class tZWinSlider<float>;

template <class T>
tZWinSlider<T>::tZWinSlider(T* pnSliderValue)
{
    mStyle = gStyleSlider;

	mpnSliderValue = pnSliderValue;	// bound value
	mnSliderUnscaledValMin = 0;
	mnSliderUnscaledValMax = 0;
    mnSliderValScalar = 1;
    mfThumbSizeRatio = 0.1;

    mBehavior = kInvalidateOnMouseUp|kDrawSliderValueOnMouseOver;
    mbInvalidateParentWhenInvalid = true;

    if (msWinName.empty())
        msWinName = "ZWinSlider_" + gMessageSystem.GenerateUniqueTargetName();
}

template <class T>
tZWinSlider<T>::~tZWinSlider()
{
}

template <class T>
bool tZWinSlider<T>::Init()
{
	mbAcceptsCursorMessages = true;

	return ZWin::Init();
}

template <class T>
bool tZWinSlider<T>::Shutdown()
{
	return ZWin::Shutdown();
}


template <class T>
void tZWinSlider<T>::SetSliderRange(T nMin, T nMax, T nMultiplier, double fThumbSizeRatio)
{ 
	mnSliderUnscaledValMin = nMin; 
	mnSliderUnscaledValMax = nMax; 
    mnSliderValScalar = nMultiplier;
    mfThumbSizeRatio = fThumbSizeRatio;

	ZASSERT(nMax - nMin > 0);
    limit<T>(*mpnSliderValue, nMin * mnSliderValScalar, nMax * mnSliderValScalar);
    limit<double>(mfThumbSizeRatio, 0.1, 1.0);
}

template <class T>
bool tZWinSlider<T>::OnMouseDownL(int64_t x, int64_t y)
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

template <class T>
bool tZWinSlider<T>::OnMouseUpL(int64_t x, int64_t y)
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

template <class T>
T tZWinSlider<T>::WindowToScaledValue(int64_t x, int64_t y)
{
    double fNormalizedValue;
    int64_t nThumbSize = ThumbSize();
    if (IsVertical())
        fNormalizedValue = (double)(y - mAreaLocal.top - nThumbSize/2) / ((double)mAreaLocal.Height() - nThumbSize);
    else
        fNormalizedValue = (double)(x - mAreaLocal.left - nThumbSize/2) / ((double)mAreaLocal.Width() - nThumbSize);

    limit<double>(fNormalizedValue, 0.0, 1.0);

    T nUnscaledValRange = mnSliderUnscaledValMax - mnSliderUnscaledValMin;
    T nScaledValue = mnSliderValScalar * (mnSliderUnscaledValMin + (T)((double)(nUnscaledValRange) * fNormalizedValue));
    //ZDEBUG_OUT("normalized:", fNormalizedValue, " scaled:", nScaledValue, "\n");

    return nScaledValue;
}

template <class T>
ZPoint tZWinSlider<T>::ScaledValueToWindowPosition(T val)
{
    ZPoint pt;
    if (mnSliderUnscaledValMax > mnSliderUnscaledValMin)
    {
        T nUnscaledVal = val / mnSliderValScalar;  // from scaled to internal

        T nUnscaledValRange = mnSliderUnscaledValMax - mnSliderUnscaledValMin;
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


template <class T>
bool tZWinSlider<T>::OnMouseMove(int64_t x, int64_t y)
{
	if (AmCapturing())
	{
        int64_t nThumbSize = ThumbSize();
        T nValue = WindowToScaledValue(x-mSliderGrabAnchor.x+nThumbSize/2, y-mSliderGrabAnchor.y+nThumbSize/2);
		SetSliderValue(nValue);
        if (mBehavior & kInvalidateOnMove)
            mpParentWin->InvalidateChildren();
	}

	return ZWin::OnMouseMove(x, y);
}

template <class T>
bool tZWinSlider<T>::OnMouseWheel(int64_t /*x*/, int64_t /*y*/, int64_t nDelta)
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


template <class T>
bool tZWinSlider<T>::OnMouseIn()
{ 
    if (mBehavior & kDrawSliderValueOnMouseOver)
        Invalidate();
    return ZWin::OnMouseIn(); 
}

template <class T>
bool tZWinSlider<T>::OnMouseOut()
{ 
    if (mBehavior & kDrawSliderValueOnMouseOver)
        Invalidate();
    return ZWin::OnMouseOut();
}


template <class T>
bool tZWinSlider<T>::Paint()
{
    if (!PrePaintCheck())
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
	rThumb.Offset(mAreaLocal.left, mAreaLocal.top);

    mpSurface.get()->BltEdge(pThumb.get(), rThumbEdge, rThumb);


    bool bDrawLabel = mBehavior & kDrawSliderValueAlways;
    bDrawLabel |= (mBehavior & kDrawSliderValueOnMouseOver && (gInput.mouseOverWin == this || gInput.captureWin == this));

	if (bDrawLabel)
	{
        string sLabel = std::to_string(*mpnSliderValue);
//		Sprintf(sLabel, "%d", *mpnSliderValue);
        
        mStyle.Font()->DrawTextParagraph(mpSurface.get(), sLabel, rThumb, &mStyle);
	}



	return ZWin::Paint();
}

template <class T>
int64_t tZWinSlider<T>::ThumbSize()
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

    ZRect rThumb(pThumb->GetArea());


    if (IsVertical())
    {
        nThumbSize = (int64_t)(mAreaLocal.Height() * mfThumbSizeRatio);
        limit <int64_t>(nThumbSize, rThumbEdge.top + pThumb->GetArea().Height() - (rThumb.Height()-rThumbEdge.bottom), mAreaLocal.Height());
    }
    else
    {
        nThumbSize = (int64_t)(mAreaLocal.Width() * mfThumbSizeRatio);
        limit <int64_t>(nThumbSize, rThumbEdge.left + pThumb->GetArea().Width() - (rThumb.Width()-rThumbEdge.right), mAreaLocal.Width());
    }


    return nThumbSize;
}

template <class T>
ZRect tZWinSlider<T>::ThumbRect()
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

template <class T>
int64_t tZWinSlider<T>::PageSize()
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
    nScaledSliderPageDelta = ((nScaledSliderPageDelta + mnSliderValScalar - 1) / mnSliderValScalar) * (int64_t)mnSliderValScalar;
    return nScaledSliderPageDelta;
}



template <class T>
void tZWinSlider<T>::SetSliderValue(T nVal)
{
    limit<T>(nVal, mnSliderUnscaledValMin * mnSliderValScalar, mnSliderUnscaledValMax * mnSliderValScalar);

    if (*mpnSliderValue != nVal)
    {
        *mpnSliderValue = nVal;
        Invalidate();
    }
}

