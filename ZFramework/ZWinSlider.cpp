#include "ZWinSlider.h"
#include "ZBuffer.h"
#include "ZDebug.h"
#include "ZGraphicSystem.h"
#include "ZStringHelpers.h"


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

	mnThumbSize = 0;
	mnSliderPosMin = 0;
	mnSliderPosMax = 0;

	mpnSliderValue = pnSliderValue;	// bound value
	mnSliderValMin = 0;
	mnSliderValMax = 0;
    mnSliderValMultiplier = 1;

    mbMouseOverDrawOnly = false;
    mbDrawValue = true;

	mfMouseDownValue = 0.0;

	mOrientation = kHorizontal;
    mbInvalidateParentWhenInvalid = true;
}

ZWinSlider::~ZWinSlider()
{
}

bool ZWinSlider::Init(tZBufferPtr pBufSliderThumb, ZRect rEdgeThumb, tZBufferPtr pBufSliderBackground, ZRect& rEdgeBackground, eOrientation orientation )
{
	ZASSERT(pBufSliderThumb);
	ZASSERT(pBufSliderBackground);

	mpBufSliderBackground = pBufSliderBackground;
	mpBufSliderThumb = pBufSliderThumb;

	mrEdgeBackground = rEdgeBackground;
	mrEdgeThumb = rEdgeThumb;

	mOrientation = orientation;

	mbAcceptsCursorMessages = true;

//    mIdleSleepMS = 100;

	return ZWin::Init();
}

bool ZWinSlider::Shutdown()
{
	return ZWin::Shutdown();
}

void ZWinSlider::UpdateSliderDrawBounds()
{
    if (!mbInitted)
        return;

	if (mOrientation == kVertical)
	{
		int64_t nMaxPixels = mAreaToDrawTo.Height();

		if (mnSliderValMax - mnSliderValMin > 0)
			mnThumbSize = (int64_t)(nMaxPixels * ((float)nMaxPixels / (float)(nMaxPixels + mnSliderValMax - mnSliderValMin)));

		if (mnThumbSize < grSliderThumbEdge.top + mpBufSliderThumb->GetArea().Height() - grSliderThumbEdge.bottom)
			mnThumbSize = grSliderThumbEdge.top + mpBufSliderThumb->GetArea().Height() - grSliderThumbEdge.bottom;
		if (mnThumbSize > mAreaToDrawTo.Height())
			mnThumbSize = mAreaToDrawTo.Height();

		mnSliderPosMin = mnThumbSize / 2;
		mnSliderPosMax = mAreaToDrawTo.Height() - mnThumbSize / 2 - (mnThumbSize & 1);   // subract an extra pixel if the thumb size is odd
	}
	else
	{
		// Horizontal
		int64_t nMaxPixels = mAreaToDrawTo.Width();

		if (mnSliderValMax - mnSliderValMin > 0)
			mnThumbSize = (int64_t)(((float)nMaxPixels / (float)(mnSliderValMax - mnSliderValMin)));

		if (mnThumbSize < mpBufSliderThumb->GetArea().Width())
			mnThumbSize = mpBufSliderThumb->GetArea().Width();
		if (mnThumbSize > mAreaToDrawTo.Width())
			mnThumbSize = mAreaToDrawTo.Width();

		mnSliderPosMin = mnThumbSize / 2;
		mnSliderPosMax = mAreaToDrawTo.Width() - mnThumbSize / 2 - (mnThumbSize & 1);   // subract an extra pixel if the thumb size is odd
	}

	Invalidate();
}

void ZWinSlider::SetDrawSliderValueFlag(bool bEnable, bool bMouseOverDrawOnly, tZFontPtr pFont) 
{ 
    mbDrawValue = bEnable; 
    mbMouseOverDrawOnly = bMouseOverDrawOnly; 
    if (pFont)
        mpFont = pFont;
    else
        mpFont = gpFontSystem->GetDefaultFont();
    ZASSERT(mpFont);
}

void ZWinSlider::SetAreaToDrawTo()
{
	ZWin::SetAreaToDrawTo();	// compute

	UpdateSliderDrawBounds();
}

void ZWinSlider::SetSliderRange(int64_t nMin, int64_t nMax, int64_t nMultiplier)
{ 
	ZASSERT(mbInitted);

	mnSliderValMin = nMin; 
	mnSliderValMax = nMax; 
    mnSliderValMultiplier = nMultiplier;

	ZASSERT(nMax - nMin > 0);

	if (*mpnSliderValue < nMin * mnSliderValMultiplier)
		*mpnSliderValue = nMin * mnSliderValMultiplier;
	if (*mpnSliderValue > nMax * mnSliderValMultiplier)
		*mpnSliderValue = nMax * mnSliderValMultiplier;

	UpdateSliderDrawBounds();
}


bool ZWinSlider::OnMouseDownL(int64_t x, int64_t y)
{
	ZRect rThumbRect(GetSliderThumbRect());
	double fSliderRep;

	if (mOrientation == kVertical)
	{
		fSliderRep = (double)rThumbRect.Height() / (double)mAreaToDrawTo.Height();
		if (rThumbRect.PtInRect(x, y))
		{
			SetCapture();
			mfMouseDownValue = (double)(y - rThumbRect.top) / (double)rThumbRect.Height();
			mfMouseDownValue *= fSliderRep;
			return true;
		}
	}
	else
	{
		fSliderRep = (double)rThumbRect.Width() / (double)mAreaToDrawTo.Width();
		if (rThumbRect.PtInRect(x, y))
		{
			SetCapture();
			mfMouseDownValue = (double)(x - rThumbRect.left) / (double)rThumbRect.Width();
			mfMouseDownValue *= fSliderRep;
			return true;
		}
	}

	int64_t nSliderValueRep = (int64_t)((double)(mnSliderValMax - mnSliderValMin) * fSliderRep);


	if (mOrientation == kVertical)
	{
		if (y < rThumbRect.top)
			SetSliderValue(*mpnSliderValue/mnSliderValMultiplier - nSliderValueRep);
		else
			SetSliderValue(*mpnSliderValue/mnSliderValMultiplier + nSliderValueRep);

        if (!msSliderSetMessage.empty())
            gMessageSystem.Post(msSliderSetMessage);
    }
	else
	{
		if (x < rThumbRect.left)
			SetSliderValue(*mpnSliderValue / mnSliderValMultiplier - nSliderValueRep);
		else
			SetSliderValue(*mpnSliderValue / mnSliderValMultiplier + nSliderValueRep);

        if (!msSliderSetMessage.empty())
            gMessageSystem.Post(msSliderSetMessage);
    }
	return ZWin::OnMouseDownL(x, y);
}

bool ZWinSlider::OnMouseUpL(int64_t x, int64_t y)
{
	ReleaseCapture();
    if (!msSliderSetMessage.empty())
        gMessageSystem.Post(msSliderSetMessage);

	return ZWin::OnMouseUpL(x, y);
}

bool ZWinSlider::OnMouseMove(int64_t x, int64_t y)
{
	if (AmCapturing())
	{
		ZRect rThumb(GetSliderThumbRect());

		int64_t nDelta;
		if (mOrientation == kVertical)
			nDelta = y;//-rThumb.Height()/2;
		else
			nDelta = x;

		double fCur = (double) (nDelta) / (double) (mnSliderPosMax-mnSliderPosMin) - mfMouseDownValue;
		int64_t nSliderCurValue = mnSliderValMin + (int64_t)((double)(mnSliderValMax - mnSliderValMin) * fCur);
		SetSliderValue(nSliderCurValue);
	}

	return ZWin::OnMouseMove(x, y);
}

bool ZWinSlider::OnMouseWheel(int64_t /*x*/, int64_t /*y*/, int64_t nDelta)
{
    ZRect rThumbRect(GetSliderThumbRect());
    double fSliderRep;
    if (mOrientation == kVertical)
        fSliderRep = (double)rThumbRect.Height() / (double)mAreaToDrawTo.Height();
    else
        fSliderRep = (double)rThumbRect.Width() / (double)mAreaToDrawTo.Width();
    int64_t nSliderValueRep = (int64_t)((double)(mnSliderValMax - mnSliderValMin) * fSliderRep);


    if (nDelta > 0)
	    SetSliderValue(*mpnSliderValue / mnSliderValMultiplier - nSliderValueRep);
    else
        SetSliderValue(*mpnSliderValue / mnSliderValMultiplier + nSliderValueRep);
	return true;
}


bool ZWinSlider::Paint()
{
	if (!mbInvalid)
		return false;

	if (!mpTransformTexture.get())
		return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
   
    mpTransformTexture.get()->BltEdge(mpBufSliderBackground.get(), mrEdgeBackground, mAreaToDrawTo);

	ZRect rThumb(GetSliderThumbRect());
	rThumb.OffsetRect(mAreaToDrawTo.left, mAreaToDrawTo.top);

    mpTransformTexture.get()->BltEdge(mpBufSliderThumb.get(), mrEdgeThumb, rThumb);

	if (mbDrawValue && mpFont)
	{
		if (!mbMouseOverDrawOnly || (mbMouseOverDrawOnly && (gpMouseOverWin == this || gpCaptureWin == this)))
		{
			string sLabel;
			Sprintf(sLabel, "%d", GetSliderValue());
			ZRect rText(mpFont->GetOutputRect(rThumb, (uint8_t*)sLabel.data(), sLabel.length(), ZGUI::Center));
            mpFont->DrawText(mpTransformTexture.get(), sLabel, rText, ZTextLook(ZTextLook::kShadowed, 0xffffffff, 0xffffffff));
		}	
	}



	return ZWin::Paint();
}

ZRect ZWinSlider::GetSliderThumbRect()
{
	ZRect rSliderThumb;
	double fNormalizedValue = ((double)*mpnSliderValue / mnSliderValMultiplier - (double)mnSliderValMin) / ((double)mnSliderValMax - (double)mnSliderValMin);
	int64_t nSliderPos = mnSliderPosMin + (int64_t)(fNormalizedValue * (double)(mnSliderPosMax - mnSliderPosMin));

	if (mOrientation == kVertical)
	{
		int64_t nSliderTop = nSliderPos - mnThumbSize/2;
		int64_t nSliderBottom = nSliderTop + mnThumbSize;
		rSliderThumb.SetRect(0, nSliderTop, mAreaToDrawTo.Width(), nSliderBottom);
	}
	else
	{
		int64_t nSliderLeft = nSliderPos - mnThumbSize/2;
		int64_t nSliderRight = nSliderLeft + mnThumbSize;
		rSliderThumb.SetRect(nSliderLeft, 0, nSliderRight, mAreaToDrawTo.Height());
	}

	return rSliderThumb;
}


void ZWinSlider::SetSliderValue(int64_t nVal)
{
	if (nVal < mnSliderValMin)
		nVal = mnSliderValMin;
	else if (nVal > mnSliderValMax)
		nVal = mnSliderValMax;

	*mpnSliderValue = nVal * mnSliderValMultiplier;

	Invalidate();
}