#include "Marquee.h"
#include "ZRandom.h"
#include "ZFont.h"

using namespace std;

Marquee::Marquee()
{
    mFillColor = 0;
    mIdleSleepMS = 1000;
}
   
bool Marquee::Init()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mbInvalidateParentWhenInvalid = true;

    SetFocus();

    mfScrollPos = (double) mAreaToDrawTo.right;
    mnLastMoveTimestamp = gTimer.GetMSSinceEpoch();

    return ZWin::Init();
}

bool Marquee::OnChar(char key)
{
#ifdef _WIN64
    switch (key)
    {
    case VK_ESCAPE:
        gMessageSystem.Post("quit_app_confirmed");
        break;
    }
#endif
    return ZWin::OnChar(key);
}


void Marquee::SetText(const string& sText, uint32_t nFont, uint32_t nFontCol1, uint32_t nFontCol2, ZFont::eStyle style)
{
    msText = sText;
    mpFont = gpFontSystem->GetDefaultFont(nFont);
    assert(mpFont);
    mFontCol1 = nFontCol1;
    mFontCol2 = nFontCol2;
    mFontStyle = style;
}

bool Marquee::Process()
{
    uint64_t nCurTime = gTimer.GetMSSinceEpoch();
    uint64_t nDeltaMS = nCurTime - mnLastMoveTimestamp;
    mnLastMoveTimestamp = nCurTime;

    double fPixelsToScroll = (mfScrollPixelsPerSecond * (double) nDeltaMS / 1000.0);
    mfScrollPos += fPixelsToScroll;

    return true;
}

bool Marquee::Paint()
{
    if (!mbInvalid)
        return false;

    const std::lock_guard<std::mutex> surfaceLock(mpTransformTexture->GetMutex());
    mpTransformTexture->FillAlpha(mAreaToDrawTo, mFillColor);

    ZRect rDraw(mpFont->GetOutputRect(mAreaToDrawTo, msText.data(), msText.length(), ZFont::kMiddleLeft));

    rDraw.OffsetRect((int64_t) mfScrollPos, 0);

    if (rDraw.right < mAreaToDrawTo.left)
    {
        mfScrollPos = (double) (mAreaToDrawTo.Width());
        rDraw = mpFont->GetOutputRect(mAreaToDrawTo, msText.data(), msText.length(), ZFont::kMiddleLeft);
        rDraw.OffsetRect((int64_t) mfScrollPos, 0);
    }

    mpFont->DrawText(mpTransformTexture, msText, rDraw, mFontCol1, mFontCol2, mFontStyle);

    ZWin::Paint();

    mbInvalid = true;
    return true;
}
   