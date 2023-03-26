#pragma once

#include "ZWinText.h"
#include "ZTimer.h"
#include <cctype>


using namespace std;

bool ZWinLabel::OnMouseDownL(int64_t x, int64_t y)
{
    if (mBehavior & CloseOnClick)
    {
        gMessageSystem.Post("kill_child", GetTopWindow(), "child", GetTargetName());
    }

    return ZWin::OnMouseDownL(x, y);
}

bool ZWinLabel::OnMouseHover(int64_t x, int64_t y)
{
    if (!msTooltipText.empty())
    {
        ZWinLabel* pWin = new ZWinLabel(ZWinLabel::CloseOnMouseOut | ZWinLabel::CloseOnClick);
        pWin->msText = msTooltipText;
        pWin->mStyle = mStyleTooltip;

        ZRect rTextArea;
        
        tZFontPtr pTooltipFont = gpFontSystem->GetFont(mStyleTooltip.fp);
        rTextArea = pTooltipFont->GetOutputRect(mAreaToDrawTo, (uint8_t*)msTooltipText.c_str(), msTooltipText.length(), mStyleTooltip.pos);
        rTextArea.InflateRect(pTooltipFont->Height()/4, pTooltipFont->Height()/4);


        rTextArea.MoveRect(ZPoint(x - rTextArea.Width() + 16, y - rTextArea.Height() + 16));
        pWin->SetArea(rTextArea);

        GetTopWindow()->ChildAdd(pWin);
    }

    return ZWin::OnMouseHover(x, y);
}

bool ZWinLabel::Process()
{
    if (mBehavior & CloseOnMouseOut && gpCaptureWin == nullptr)   // only process this if no window has capture
    {
        if (!mAreaAbsolute.PtInRect(gLastMouseMove))        // if the mouse is outside of our area we hide. (Can't use OnMouseOut because we get called when the mouse goes over one of our controls)
        {
            SetVisible(false);
            mBehavior = None;   // no further actions
            gMessageSystem.Post("kill_child", GetTopWindow(), "child", GetTargetName());
        }
    }

    return ZWin::Process();
}

bool ZWinLabel::Paint()
{
    const lock_guard<recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return false;

    if (ARGB_A(mStyle.bgCol) > 5)
        mpTransformTexture->Fill(mAreaToDrawTo, mStyle.bgCol);

    if (mStyle.wrap)
    {
        mStyle.Font()->DrawTextParagraph(mpTransformTexture.get(), msText, mAreaToDrawTo, mStyle.look, mStyle.pos);
    }
    else
    {
        ZRect rOut = mStyle.Font()->GetOutputRect(mAreaToDrawTo, (uint8_t*)msText.c_str(), msText.length(), mStyle.pos);
        mStyle.Font()->DrawText(mpTransformTexture.get(), msText, rOut, mStyle.look);
    }

    return ZWin::Paint();
}








ZWinTextEdit::ZWinTextEdit(string* pText)
{
    ZASSERT(pText);
    mpText = pText;

    mCursorPosition = mpText->length();
    mnViewOffset = 0;
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mbCTRLDown = false;
}

bool ZWinTextEdit::Init()
{
    ZASSERT(mpText);
    HandleCursorMove(0);

    return ZWin::Init();
}


bool ZWinTextEdit::OnMouseDownL(int64_t x, int64_t y)
{
    int64_t nPixelsIntoString = (x - mrVisibleTextArea.left)+mnViewOffset;
    int64_t i;
    for (i = 0; i < mpText->length() && mStyle.Font()->StringWidth(mpText->substr(0, i)) < nPixelsIntoString; i++);

    HandleCursorMove(i, false);

    return ZWin::OnMouseDownL(x, y);
}

bool ZWinTextEdit::Process()
{
    if (GetFocus() == this)
    {
        mbCursorVisible = (gTimer.GetMSSinceEpoch() / 250) % 2;
        Invalidate();
    }
    else if (mbCursorVisible)
    {
        mbCursorVisible = false;
        Invalidate();
    }

    return ZWin::Process();
}

void ZWinTextEdit::SetArea(const ZRect& area)
{
    ZWin::SetArea(area);
    mrVisibleTextArea = mAreaToDrawTo;
    mrVisibleTextArea.DeflateRect(mStyle.padding, mStyle.padding);
    mrVisibleTextArea = ZGUI::Arrange(mrVisibleTextArea, mAreaToDrawTo, mStyle.pos, mStyle.padding);

    mnLeftBoundary = mrVisibleTextArea.Width() / 4; // 25%
    mnRightBoundary = (mrVisibleTextArea.Width() * 3) / 4;  // 75%
}

bool ZWinTextEdit::Paint()
{
    const lock_guard<recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return false;

    ZRect rOut(mrVisibleTextArea.left, mrVisibleTextArea.top, mrVisibleTextArea.right+mStyle.Font()->StringWidth(*mpText), mrVisibleTextArea.bottom);
    rOut.OffsetRect(-mnViewOffset, 0);

    mpTransformTexture.get()->Fill(mAreaToDrawTo, mStyle.bgCol);

    mStyle.Font()->DrawTextParagraph(mpTransformTexture.get(), *mpText, rOut, mStyle.look, mStyle.pos, &mAreaToDrawTo);

    if (mbCursorVisible)
        mpTransformTexture.get()->Fill(mrCursorArea, 0xffffffff);

    return ZWin::Paint();
}

bool ZWinTextEdit::OnChar(char c)
{
    switch (c)
    {
    case VK_ESCAPE:
        gMessageSystem.Post("quit_app_confirmed");
        break;
    case VK_BACK:
        {
            if (mCursorPosition > 0 && !mpText->empty())
            {
                mpText->erase(mCursorPosition - 1, 1);
                HandleCursorMove(mCursorPosition - 1);

            }
        }
        break;
    default:
        {
            mpText->insert(mCursorPosition, 1, c);
            HandleCursorMove(mCursorPosition + 1);
        }
        break;
    }

    return true;
}



int64_t ZWinTextEdit::FindNextBreak(int64_t nDir)
{
    int64_t nCursor = mCursorPosition;
    if (nDir > 0)
    {
        nCursor++;
        if (nCursor < mpText->length())
        {
            if (isalnum((int)(*mpText)[nCursor]))   // cursor is on an alphanumeric
            {
                while (nCursor < mpText->length() && isalnum((int)(*mpText)[nCursor]))   // while an alphanumeric character, skip
                    nCursor++;
            }

            while (nCursor < mpText->length() && isblank((int)(*mpText)[nCursor]))    // while whitespace, skip
                nCursor++;
        }
    }
    else
    {
        nCursor--;
        if (nCursor > 0)
        {
            if (isalnum((int)(*mpText)[nCursor]))   // cursor is on an alphanumeric
            {
                while (nCursor > 0 && isalnum((int)(*mpText)[nCursor - 1]))   // while an alphanumeric character, skip
                    nCursor--;
            }
            else
            {
                while (nCursor > 0 && isblank((int)(*mpText)[nCursor - 1]))    // while whitespace, skip
                    nCursor--;

                while (nCursor > 0 && isalnum((int)(*mpText)[nCursor - 1]))   // while an alphanumeric character, skip
                    nCursor--;
            }
        }
    }

    if (nCursor < 0)
        return 0;
    if (nCursor > mpText->length())
        return mpText->length();

    return nCursor;
}


bool ZWinTextEdit::OnKeyDown(uint32_t c)
{
    switch (c)
    {
    case VK_LEFT:
        if (mbCTRLDown)
            HandleCursorMove(FindNextBreak(-1));
        else
            HandleCursorMove(mCursorPosition - 1);
        break;
    case VK_RIGHT:
        if (mbCTRLDown)
            HandleCursorMove(FindNextBreak(1));
        else
            HandleCursorMove(mCursorPosition + 1);
        break;
    case VK_HOME:
        HandleCursorMove(0);
        break;
    case VK_END:
        HandleCursorMove(mpText->length());
        break;
    case VK_CONTROL:
        mbCTRLDown = true;
        break;
    }
    return true;
}

bool ZWinTextEdit::OnKeyUp(uint32_t c)
{
    switch (c)
    {
    case VK_CONTROL:
        mbCTRLDown = false;
        break;
    }
    return ZWin::OnKeyUp(c);
}


void ZWinTextEdit::HandleCursorMove(int64_t newCursorPos, bool bScrollView)
{
    tZFontPtr pFont = mStyle.Font();

    int64_t nAbsStringPixels = pFont->StringWidth(*mpText);
    int64_t nNewAbsPixelsToCursor = pFont->StringWidth(mpText->substr(0, newCursorPos));

    int64_t nNewVisiblePixelsToCursor = nNewAbsPixelsToCursor - mnViewOffset;

    if (bScrollView)
    {

        if (newCursorPos < mCursorPosition) // moving left
        {
            if (nNewVisiblePixelsToCursor <= mnLeftBoundary && mnViewOffset > 0)  // crossing left
            {
                // adjust view to put cursor at boundary
                mnViewOffset = nNewAbsPixelsToCursor - mnLeftBoundary;
            }
        }
        else // moving right
        {
            if (newCursorPos < (int64_t)mpText->length())
            {
                if (nNewVisiblePixelsToCursor > mnRightBoundary) // moving right but cursor not at end... adjust 
                {
                    mnViewOffset = nNewAbsPixelsToCursor - mnRightBoundary;
                }
            }
            else
            {
                // cursor at end
                if (nNewVisiblePixelsToCursor > mrVisibleTextArea.right)
                {
                    mnViewOffset = nNewAbsPixelsToCursor - mrVisibleTextArea.right;
                }
            }
        }

        if (mnViewOffset < 0)
            mnViewOffset = 0;
        else if (mnViewOffset > 0 && mnViewOffset > nAbsStringPixels - mrVisibleTextArea.Width())
            mnViewOffset = nAbsStringPixels - mrVisibleTextArea.Width();
    }


    mrCursorArea.SetRect(nNewAbsPixelsToCursor - mnViewOffset, mrVisibleTextArea.top, nNewAbsPixelsToCursor - mnViewOffset + 2, mrVisibleTextArea.bottom);

    if (newCursorPos >= 0 && newCursorPos <= mpText->length())
        mCursorPosition = newCursorPos;
    Invalidate();
}
