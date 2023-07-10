#include "ZWinSlider.h"
#include "ZBuffer.h"
#include "ZDebug.h"
#include "ZGraphicSystem.h"
#include "ZStringHelpers.h"
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
	mpBufSliderThumb = NULL;
	mpBufSliderBackground = NULL;

	mpnSliderValue = pnSliderValue;	// bound value
	mnSliderUnscaledValMin = 0;
	mnSliderUnscaledValMax = 0;
    mnSliderValScalar = 1;
    mfThumbSizeRatio = 0.1;

    mBehavior = kVertical|kInvalidateOnMouseUp|kDrawSliderValueOnMouseOver;
    mbInvalidateParentWhenInvalid = true;
}

ZWinSlider::~ZWinSlider()
{
}

bool ZWinSlider::Init()
{
	mbAcceptsCursorMessages = true;

    if (!mpBufSliderThumb)
    {
        if (mBehavior & kHorizontal)
            mpBufSliderThumb = gSliderThumbHorizontal;
        else
            mpBufSliderThumb = gSliderThumbVertical;
        mrEdgeThumb = grSliderThumbEdge;
    }

    if (!mpBufSliderBackground)
    {
        mpBufSliderBackground = gSliderBackground;
        mrEdgeBackground = grSliderBgEdge;
    }

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

	if (mBehavior & kVertical)
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
    if (mBehavior & kVertical)
        fNormalizedValue = (double)(y - mAreaToDrawTo.top - nThumbSize/2) / ((double)mAreaToDrawTo.Height() - nThumbSize);
    else
        fNormalizedValue = (double)(x - mAreaToDrawTo.left - nThumbSize/2) / ((double)mAreaToDrawTo.Width() - nThumbSize);

    limit<double>(fNormalizedValue, 0.0, 1.0);

    int64_t nUnscaledValRange = mnSliderUnscaledValMax - mnSliderUnscaledValMin;
    int64_t nScaledValue = mnSliderValScalar * (mnSliderUnscaledValMin + (int64_t)((double)(nUnscaledValRange) * fNormalizedValue));
    ZDEBUG_OUT("normalized:", fNormalizedValue, " scaled:", nScaledValue, "\n");

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
        if (mBehavior & kVertical)
            pt.y = mAreaToDrawTo.top + fNormalizedValue * (double)(mAreaToDrawTo.Height() - nThumbSize);
        else
            pt.x = mAreaToDrawTo.left + fNormalizedValue * (double)(mAreaToDrawTo.Width() - nThumbSize);
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
    if (mBehavior & kVertical)
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
	if (!mbInvalid || !mbInitted|| !mpTransformTexture.get())
		return false;


    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
   
    mpTransformTexture.get()->BltEdge(mpBufSliderBackground.get(), mrEdgeBackground, mAreaToDrawTo);

	ZRect rThumb(ThumbRect());
	rThumb.OffsetRect(mAreaToDrawTo.left, mAreaToDrawTo.top);

    mpTransformTexture.get()->BltEdge(mpBufSliderThumb.get(), mrEdgeThumb, rThumb);


    bool bDrawLabel = mBehavior & kDrawSliderValueAlways;
    bDrawLabel |= (mBehavior & kDrawSliderValueOnMouseOver && (gInput.mouseOverWin == this || gInput.captureWin == this));

	if (bDrawLabel && mpFont)
	{
		string sLabel;
		Sprintf(sLabel, "%d", *mpnSliderValue);
		ZRect rText(mpFont->Arrange(rThumb, (uint8_t*)sLabel.data(), sLabel.length(), ZGUI::Center));
        mpFont->DrawText(mpTransformTexture.get(), sLabel, rText, &ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xffffffff, 0xffffffff));
	}



	return ZWin::Paint();
}

int64_t ZWinSlider::ThumbSize()
{
    int64_t nThumbSize = 0;

    if (mBehavior & kVertical)
    {
        nThumbSize = mAreaToDrawTo.Height() * mfThumbSizeRatio;
        limit <int64_t>(nThumbSize, grSliderThumbEdge.top + mpBufSliderThumb->GetArea().Height() - grSliderThumbEdge.bottom, mAreaToDrawTo.Height());
    }
    else
    {
        nThumbSize = mAreaToDrawTo.Width() * mfThumbSizeRatio;
        limit <int64_t>(nThumbSize, grSliderThumbEdge.left + mpBufSliderThumb->GetArea().Width() - grSliderThumbEdge.right, mAreaToDrawTo.Width());
    }


    return nThumbSize;
}

ZRect ZWinSlider::ThumbRect()
{
    ZPoint sliderPt = ScaledValueToWindowPosition(*mpnSliderValue);
    int64_t nThumbSize = ThumbSize();

	if (mBehavior & kVertical)
	{
		int64_t nSliderTop = sliderPt.y;
		int64_t nSliderBottom = nSliderTop + nThumbSize;
		return ZRect(0, nSliderTop, mAreaToDrawTo.Width(), nSliderBottom);
	}

    // else horizontal
	int64_t nSliderLeft = sliderPt.x;
	int64_t nSliderRight = nSliderLeft + nThumbSize;
	return ZRect(nSliderLeft, 0, nSliderRight, mAreaToDrawTo.Height());
}

int64_t ZWinSlider::PageSize()
{
    double fThumbGUIRatio;
    int64_t nThumbSize;
    if (mBehavior & kVertical)
    {
        nThumbSize = mAreaToDrawTo.Height() * mfThumbSizeRatio;
        limit <int64_t>(nThumbSize, grSliderThumbEdge.top + mpBufSliderThumb->GetArea().Height() - grSliderThumbEdge.bottom, mAreaToDrawTo.Height());
        fThumbGUIRatio = (double)nThumbSize / (double)mAreaToDrawTo.Height();
    }
    else
    {
        nThumbSize = mAreaToDrawTo.Width() * mfThumbSizeRatio;
        limit <int64_t>(nThumbSize, grSliderThumbEdge.left + mpBufSliderThumb->GetArea().Width() - grSliderThumbEdge.right, mAreaToDrawTo.Width());
        fThumbGUIRatio = (double)nThumbSize / (double)mAreaToDrawTo.Width();
    }


    int64_t nScaledSliderPageDelta = (int64_t)((double)(mnSliderUnscaledValMax - mnSliderUnscaledValMin) * mnSliderValScalar) * fThumbGUIRatio;
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

void ZWinSlider::SetSliderImages(tZBufferPtr pBufSliderThumb, ZRect rEdgeThumb, tZBufferPtr pBufSliderBackground, ZRect& rEdgeBackground)
{
    ZASSERT(pBufSliderThumb);
    ZASSERT(pBufSliderBackground);

    mpBufSliderBackground = pBufSliderBackground;
    mpBufSliderThumb = pBufSliderThumb;

    mrEdgeBackground = rEdgeBackground;
    mrEdgeThumb = rEdgeThumb;
}

void ZWinSlider::SetBehavior(uint32_t behavior_flags, tZFontPtr pFont)
{
    mBehavior = behavior_flags;
    mpFont = pFont;
}
