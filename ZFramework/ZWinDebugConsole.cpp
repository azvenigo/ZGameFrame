#include "ZWinDebugConsole.h"
#include "ZStringHelpers.h"
#include "helpers/StringHelpers.h"
#include "ZMessageSystem.h"
#include "ZSliderWin.h"
#include "Resources.h"

using namespace std;

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ZWinDebugConsole::ZWinDebugConsole()
{
	mpSliderWin = NULL;

	mrTextArea.SetRect(0,0,0,0);
    mTextCol = 0xff00ffff;

	mbScrollable				= true;

	mnMouseDownSliderVal		= 0;
	mfMouseMomentum				= 0.0;
    mnSliderVal = 0;

    mnDebugHistoryLastSeenCounter = 0;

	mbAcceptsCursorMessages = true;

}



bool ZWinDebugConsole::Init()
{
    if (!mFont)
        mFont = gpFontSystem->GetFont(ZFontParams("consolas", 24, 200, 0, 14));

    if (ZWin::Init())
	{
        mIdleSleepMS = 10000;

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
        UpdateScrollbar();
    }
    return true;
}



void ZWinDebugConsole::SetArea(const ZRect& newArea)
{
	ZWin::SetArea(newArea);
	UpdateScrollbar();
}



bool ZWinDebugConsole::OnMouseWheel(int64_t /*x*/, int64_t /*y*/, int64_t nDelta)
{
	if (mpSliderWin)
	{
		int64_t nMin;
		int64_t nMax;
		mpSliderWin->GetSliderRange(nMin, nMax);
		if (nMax-nMin > 0)
		{
			mpSliderWin->SetSliderValue(mpSliderWin->GetSliderValue() + nDelta);
		}
	}

    Invalidate();

	return true;
}

void ZWinDebugConsole::UpdateScrollbar()
{
	int64_t nFullTextHeight = gDebug.mHistory.size();

    mrTextArea.SetRect(mAreaToDrawTo);
    mrTextArea.DeflateRect(gDefaultSpacer, gDefaultSpacer);
    mrTextArea.right -= 16;

    size_t nVisible = GetVisibleLines();

	if (mbScrollable && mFont && nFullTextHeight > nVisible)
	{
        if (!mpSliderWin)
        {
            mpSliderWin = new ZSliderWin(&mnSliderVal);
            mpSliderWin->Init(gSliderThumbVertical, grSliderThumbEdge, gSliderBackground, grSliderBgEdge);
            mpSliderWin->SetArea(ZRect(mArea.Width() - 32, 0, mArea.Width(), mArea.Height()));
            ChildAdd(mpSliderWin);
        }

		mpSliderWin->SetSliderRange(0, nFullTextHeight- nVisible);
        mpSliderWin->SetSliderValue(nFullTextHeight - nVisible);
	}
	else
	{
		if (mpSliderWin)
		{
			ChildDelete(mpSliderWin);
			mpSliderWin = NULL;
		}
	}
    Invalidate();
}

size_t ZWinDebugConsole::GetVisibleLines()
{
    if (mFont)
        return mrTextArea.Height() / mFont->Height();
    return 0;
}



bool ZWinDebugConsole::Paint()
{
	if (!mbInvalid)
		return true;

    if (!mpTransformTexture.get())
        return false;


    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());

    mpTransformTexture.get()->Fill(mrTextArea, 0xff000000);


    int64_t nHeight = mFont->Height();
    int64_t nFixedWidth = mFont->GetFontParams().nFixedWidth;
    int64_t nCharsPerLine = mrTextArea.Width() / nFixedWidth;

    const std::lock_guard<std::mutex> lock(gDebug.mHistoryMutex);
    list<sDbgMsg>::reverse_iterator it = gDebug.mHistory.rbegin();       // reverse begin


    int64_t nScroll = 0;
    if (mpSliderWin)
    {
        int64_t nMin = 0;
        int64_t nMax = 0;
        mpSliderWin->GetSliderRange(nMin, nMax);
        nScroll = nMax - mnSliderVal;
    }


    while (it != gDebug.mHistory.rend() && nScroll-- > 0)
        it++;



    int64_t nCurLineBottom = mrTextArea.bottom;

    while (it != gDebug.mHistory.rend())
    {
        sDbgMsg& msg = *it;
        int64_t nLines = (msg.sLine.length() + nCharsPerLine - 1) / nCharsPerLine;

        int64_t nOffset = 0;
        while (nOffset < msg.sLine.length())
        {
            ZRect rLine(mrTextArea.left, nCurLineBottom - nLines * mFont->Height(), mrTextArea.right, nCurLineBottom);
            string sPartial(msg.sLine.substr(nOffset, nCharsPerLine));
            mFont->DrawTextA(mpTransformTexture.get(), sPartial, rLine, msg.nLevel, msg.nLevel);
            nOffset += sPartial.length();
            rLine.top += nHeight;
            nCurLineBottom -= nHeight;
        };

        if (nCurLineBottom <= mrTextArea.top)
            break;

        it++;
    }

	return ZWin::Paint();
}

void ZWinDebugConsole::ScrollTo(int64_t nSliderValue)
{
	if (mpSliderWin)
		mpSliderWin->SetSliderValue(nSliderValue);
    Invalidate();
}
