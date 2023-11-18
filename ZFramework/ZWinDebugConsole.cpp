#include "ZWinDebugConsole.h"
#include "ZStringHelpers.h"
#include "helpers/StringHelpers.h"
#include "ZMessageSystem.h"
#include "ZWinSlider.h"
#include "Resources.h"
#include "ZInput.h"

using namespace std;

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ZWinDebugConsole::ZWinDebugConsole()
{
	mpWinSlider = NULL;

	mrDocumentArea.SetRect(0,0,0,0);

    mStyle = gStyleGeneralText;
    mStyle.bgCol = 0xff444444;
    mStyle.look.colTop = 0xff00ffff;
    mStyle.look.colBottom = 0xff00ffff;

	mbScrollable				= true;

	mnMouseDownSliderVal		= 0;
	mfMouseMomentum				= 0.0;
    mnSliderVal = 0;

    mnDebugHistoryLastSeenCounter = 0;

    mIdleSleepMS = 200;
	mbAcceptsCursorMessages = true;

    if (msWinName.empty())
        msWinName = "ZWinDebugConsole_" + gMessageSystem.GenerateUniqueTargetName();
}



bool ZWinDebugConsole::Init()
{
    if (mbInitted)
        return true;

    if (!mFont)
        mFont = gpFontSystem->GetFont(ZFontParams("consolas", 24, 200, 0, 14));

    if (ZWin::Init())
	{
        ZASSERT(mFont);
		UpdateScrollbar();
	}

	return true;
}

bool ZWinDebugConsole::Process()
{
    if (mnDebugHistoryLastSeenCounter != gDebug.Counter())
    {
        mnDebugHistoryLastSeenCounter = gDebug.Counter();
        if (mbVisible)
        {
            UpdateScrollbar();
            Invalidate();
        }
        return true;
    }

    return false;
}



void ZWinDebugConsole::SetArea(const ZRect& newArea)
{
	ZWin::SetArea(newArea);
	UpdateScrollbar();
}



bool ZWinDebugConsole::OnMouseWheel(int64_t /*x*/, int64_t /*y*/, int64_t nDelta)
{
	if (mpWinSlider)
	{
		int64_t nMin;
		int64_t nMax;
		mpWinSlider->GetSliderRange(nMin, nMax);
		if (nMax-nMin > 0)
		{
			mpWinSlider->SetSliderValue(mnSliderVal + nDelta);
		}
	}

    Invalidate();

	return true;
}

void ZWinDebugConsole::UpdateScrollbar()
{
	int64_t nFullTextHeight = gDebug.mHistory.size();

    mrDocumentArea.SetRect(mAreaLocal);
    mrDocumentArea.DeflateRect(gSpacer, gSpacer);
    mrDocumentArea.right -= 16;

    size_t nVisible = GetVisibleLines();

	if (mbScrollable && mFont && nFullTextHeight > (int64_t)nVisible)
	{
        if (!mpWinSlider)
        {
            mpWinSlider = new ZWinSlider(&mnSliderVal);
            mpWinSlider->SetArea(ZRect(mAreaInParent.Width() - 32, 0, mAreaInParent.Width(), mAreaInParent.Height()));
            ChildAdd(mpWinSlider);
        }

        double fThumbRatio = (double)nVisible / (double)nFullTextHeight;
        mpWinSlider->SetSliderRange(0, nFullTextHeight- nVisible, 1, fThumbRatio);
        mpWinSlider->SetSliderValue(nFullTextHeight - nVisible);
	}
	else
	{
		if (mpWinSlider)
		{
			ChildDelete(mpWinSlider);
			mpWinSlider = NULL;
		}
	}
    Invalidate();
}

size_t ZWinDebugConsole::GetVisibleLines()
{
    if (mFont)
        return mrDocumentArea.Height() / mFont->Height();
    return 0;
}



bool ZWinDebugConsole::Paint()
{
	if (!mbInvalid || !mbVisible)
		return false;

    if (!mpSurface.get())
        return false;


    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

    mpSurface.get()->Fill(mStyle.bgCol, &mrDocumentArea);


    int64_t nHeight = mFont->Height();
    int64_t nFixedWidth = mFont->GetFontParams().nFixedWidth;
    int64_t nCharsPerLine = mrDocumentArea.Width() / nFixedWidth;

    const std::lock_guard<std::mutex> lock(gDebug.mHistoryMutex);
    list<sDbgMsg>::reverse_iterator it = gDebug.mHistory.rbegin();       // reverse begin


    int64_t nScroll = 0;
    if (mpWinSlider)
    {
        int64_t nMin = 0;
        int64_t nMax = 0;
        mpWinSlider->GetSliderRange(nMin, nMax);
        nScroll = nMax - mnSliderVal;
    }


    while (it != gDebug.mHistory.rend() && nScroll-- > 0)
        it++;



    int64_t nCurLineBottom = mrDocumentArea.bottom;

    while (it != gDebug.mHistory.rend())
    {
        sDbgMsg& msg = *it;
        int64_t nLines = (msg.sLine.length() + nCharsPerLine - 1) / nCharsPerLine;

        int64_t nOffset = 0;
        while (nOffset < (int64_t) msg.sLine.length())
        {
            ZRect rLine(mrDocumentArea.left, nCurLineBottom - nLines * mFont->Height(), mrDocumentArea.right, nCurLineBottom);
            string sPartial(msg.sLine.substr(nOffset, nCharsPerLine));
            mFont->DrawTextA(mpSurface.get(), sPartial, rLine, &ZGUI::ZTextLook(ZGUI::ZTextLook::kNormal, msg.nLevel, msg.nLevel));
            nOffset += sPartial.length();
            rLine.top += nHeight;
            nCurLineBottom -= nHeight;
        };

        if (nCurLineBottom <= mrDocumentArea.top)
            break;

        it++;
    }

	return ZWin::Paint();
}

void ZWinDebugConsole::ScrollTo(int64_t nSliderValue)
{
	if (mpWinSlider)
		mpWinSlider->SetSliderValue(nSliderValue);
    Invalidate();
}
