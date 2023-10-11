#pragma once

#include "ZWinText.h"
#include "ZTimer.h"
#include <cctype>
#include "ZInput.h"
//#include "ZGraphicSystem.h"
//#include "ZScreenBuffer.h"


using namespace std;

ZWinLabel::ZWinLabel(uint8_t _behavior)
{
    mBehavior = (eBehavior)_behavior;
    mbAcceptsCursorMessages = (mBehavior&CloseOnClick)!=0;  
    mStyle = gStyleCaption;
}


bool ZWinLabel::OnMouseDownL(int64_t x, int64_t y)
{
    if (mBehavior & CloseOnClick)
    {
        gMessageSystem.Post("kill_child", GetTopWindow(), "name", GetTargetName());
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
        rTextArea = pTooltipFont->Arrange(mAreaLocal, (uint8_t*)msTooltipText.c_str(), msTooltipText.length(), mStyleTooltip.pos);
        rTextArea.InflateRect(pTooltipFont->Height()/4, pTooltipFont->Height()/4);


        // ensure the tooltip is entirely visible on the window
        ZPoint pt(x - rTextArea.Width() + 16, y - rTextArea.Height() + 16);
        limit<int64_t>(pt.x, grFullArea.left, grFullArea.right - rTextArea.Width());
        limit<int64_t>(pt.y, grFullArea.top, grFullArea.bottom - rTextArea.Height());


        rTextArea.MoveRect(pt);
        pWin->SetArea(rTextArea);

        GetTopWindow()->ChildAdd(pWin);
    }

    return ZWin::OnMouseHover(x, y);
}

bool ZWinLabel::Process()
{
    if (mBehavior & CloseOnMouseOut && gInput.captureWin == nullptr)   // only process this if no window has capture
    {
        if (!mAreaAbsolute.PtInRect(gInput.lastMouseMove))        // if the mouse is outside of our area we hide. (Can't use OnMouseOut because we get called when the mouse goes over one of our controls)
        {
            SetVisible(false);
            mBehavior = None;   // no further actions
            gMessageSystem.Post("kill_child", GetTopWindow(), "name", GetTargetName());
        }
    }

    return ZWin::Process();
}

bool ZWinLabel::Paint()
{
    if (!mpSurface /*|| !mbVisible*/)
        return false;

    const lock_guard<recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());
    if (!mbInvalid)
        return false;

    if (ARGB_A(mStyle.bgCol) > 0xf0)
    {
        mpSurface->Fill(mStyle.bgCol);
    }
    else if (ARGB_A(mStyle.bgCol) > 0x0f)
    {
        // Translucent fill.... (expensive render but should not be frequent or large)
        GetTopWindow()->RenderToBuffer(mpSurface, mAreaAbsolute, mAreaLocal, this);

//        gpGraphicSystem->GetScreenBuffer()->RenderVisibleRectsToBuffer(mpSurface.get(), mAreaAbsolute);

//        ZDEBUG_OUT("x:", mAreaAbsolute.left, "y:", mAreaAbsolute.top, "\n");
        mpSurface->Blur(2);
        mpSurface->FillAlpha(mStyle.bgCol);
        mpSurface->DrawRectAlpha(mpSurface->GetArea(), 0xff000000 | mStyle.bgCol);
    }
    else
    {
        // Transparent
        PaintFromParent();
    }

    if (mStyle.wrap)
    {
        mStyle.Font()->DrawTextParagraph(mpSurface.get(), msText, mAreaLocal, &mStyle);
    }
    else
    {
        ZRect rOut = mStyle.Font()->Arrange(mAreaLocal, (uint8_t*)msText.c_str(), msText.length(), mStyle.pos);
        mStyle.Font()->DrawText(mpSurface.get(), msText, rOut, &mStyle.look);
    }

    return ZWin::Paint();
}


/*ZWinLabel* ZWinLabel::ShowTooltip(ZWin* pMainWin, const std::string& sTooltip, const ZGUI::Style& style)
{
    bool bToolTipAlreadyExists = true;
    ZWinLabel* pWin = (ZWinLabel*)pMainWin->GetChildWindowByWinName("winlabel_tooltip");

    if (!pWin)
    {
        bToolTipAlreadyExists = false;
        pWin = new ZWinLabel(ZWinLabel::CloseOnMouseOut);
    }

    pWin->msWinName = "winlabel_tooltip";
    pWin->msText = sTooltip;
    pWin->mStyle = style;
    pWin->mIdleSleepMS = 100;


    ZRect rTextArea;
    tZFontPtr pTooltipFont = gpFontSystem->GetFont(pWin->mStyle.fp);
    rTextArea = pTooltipFont->StringRect(sTooltip);
    rTextArea.InflateRect(style.paddingH, style.paddingV);


    // ensure the tooltip is entirely visible on the window + a little padding
    ZPoint pt(gInput.lastMouseMove.x - rTextArea.Width() + pTooltipFont->Height() / 4, gInput.lastMouseMove.y - rTextArea.Height() + pTooltipFont->Height() / 4);
    limit<int64_t>(pt.x, grFullArea.left+gSpacer, grFullArea.right - rTextArea.Width()- gSpacer);
    limit<int64_t>(pt.y, grFullArea.top+ gSpacer, grFullArea.bottom - rTextArea.Height()- gSpacer);

    rTextArea.MoveRect(pt);
    pWin->SetArea(rTextArea);

    if (bToolTipAlreadyExists || pMainWin->ChildAdd(pWin))
        return pWin;

    delete pWin;
    return nullptr;
}
*/






ZWinTextEdit::ZWinTextEdit(string* pText)
{
    ZASSERT(pText);
    mpText = pText;

    mCursorPosition = mpText->length();
    mnViewOffset = 0;
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mbCTRLDown = false;
    mbSelecting = false;
    mbSHIFTDown = false;
    mbMouseDown = false;
    mnSelectionStart = -1;
    mnSelectionEnd = -1;
    mStyle = gDefaultWinTextEditStyle;
}

bool ZWinTextEdit::Init()
{
    ZASSERT(mpText);
    HandleCursorMove(0);

    return ZWin::Init();
}


bool ZWinTextEdit::OnMouseDownL(int64_t x, int64_t y)
{
    SetCapture();
    if (mbSelecting)
        CancelSelecting();

    HandleCursorMove(WindowToTextPosition(x), false);

    mbMouseDown = true;

    return ZWin::OnMouseDownL(x, y);
}

int64_t ZWinTextEdit::WindowToTextPosition(int64_t x)
{
    int64_t nPixelsIntoString = (x - mrVisibleTextArea.left) + mnViewOffset-StyleOffset();
    int64_t i;
    for (i = 0; i < (int64_t)mpText->length() && mStyle.Font()->StringWidth(mpText->substr(0, i)) < nPixelsIntoString; i++);
    return i;
}


bool ZWinTextEdit::OnMouseUpL(int64_t x, int64_t y)
{
    ReleaseCapture();
    mbMouseDown = false;
    return ZWin::OnMouseUpL(x, y);
}

bool ZWinTextEdit::OnMouseMove(int64_t x, int64_t y)
{
    if (mbMouseDown && AmCapturing())
    {
        StartSelecting();

        HandleCursorMove(WindowToTextPosition(x), false);
    }

    return ZWin::OnMouseMove(x, y);
}


bool ZWinTextEdit::Process()
{
   
    if (gInput.keyboardFocusWin == this)
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
    mrVisibleTextArea = mAreaLocal;
    mrVisibleTextArea.DeflateRect(mStyle.paddingH, mStyle.paddingV);
    mrVisibleTextArea = ZGUI::Arrange(mrVisibleTextArea, mAreaLocal, mStyle.pos, mStyle.paddingH, mStyle.paddingV);

    mnLeftBoundary = mrVisibleTextArea.Width() / 4; // 25%
    mnRightBoundary = (mrVisibleTextArea.Width() * 3) / 4;  // 75%
}

bool ZWinTextEdit::Paint()
{
    const lock_guard<recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());
    if (!mbInvalid)
        return false;

    int64_t nWidth = mStyle.Font()->StringWidth(*mpText);
    ZRect rOut(mrVisibleTextArea.left-StyleOffset(), mrVisibleTextArea.top, mrVisibleTextArea.right+StyleOffset(), mrVisibleTextArea.bottom);

    ZGUI::ePosition posToUse = mStyle.pos;
    if (nWidth > mrVisibleTextArea.Width())
    {
        rOut.OffsetRect(-mnViewOffset, 0);
        posToUse = (ZGUI::ePosition)((uint32_t)mStyle.pos & ~ZGUI::HC | ZGUI::L);  // remove horizontal center and left justify
    }


    ZRect rString(mStyle.Font()->Arrange(rOut, (uint8_t*)mpText->c_str(), mpText->length(), posToUse, 0));

    mpSurface.get()->Fill(mStyle.bgCol);

    mStyle.Font()->DrawText(mpSurface.get(), *mpText, rString, &mStyle.look, &mAreaLocal);

    if (mnSelectionStart != mnSelectionEnd &&
        mnSelectionStart >= 0 && mnSelectionEnd >= 0 &&
        mnSelectionStart <= (int64_t)mpText->length() && mnSelectionEnd <= (int64_t)mpText->length())
    {
        int64_t nMin = min<int64_t>(mnSelectionStart, mnSelectionEnd);
        int64_t nMax = max<int64_t>(mnSelectionStart, mnSelectionEnd);

        int64_t nWidthBefore = mStyle.Font()->StringWidth(mpText->substr(0, nMin));
        int64_t nWidthHighlighted = mStyle.Font()->StringWidth(mpText->substr(nMin, nMax - nMin));
        ZRect rHighlight(0, mrVisibleTextArea.top, nWidthHighlighted, mrVisibleTextArea.bottom);
        rHighlight.OffsetRect(mrVisibleTextArea.left+nWidthBefore- mnViewOffset+StyleOffset(), 0);
        rHighlight.IntersectRect(mrVisibleTextArea);
        mpSurface.get()->FillAlpha(0x8800ffff, &rHighlight);
    }


    if (mbCursorVisible)
        mpSurface.get()->Fill(mStyle.look.colTop, &mrCursorArea);

    return ZWin::Paint();
}


int64_t ZWinTextEdit::FindNextBreak(int64_t nDir)
{
    int64_t nCursor = mCursorPosition;
    if (nDir > 0)
    {
        nCursor++;
        if (nCursor < (int64_t)mpText->length())
        {
            if (isalnum((int)(*mpText)[nCursor]))   // cursor is on an alphanumeric
            {
                while (nCursor < (int64_t)mpText->length() && isalnum((int)(*mpText)[nCursor]))   // while an alphanumeric character, skip
                    nCursor++;
            }

            while (nCursor < (int64_t)mpText->length() && isblank((int)(*mpText)[nCursor]))    // while whitespace, skip
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
    if (nCursor > (int64_t)mpText->length())
        return (int64_t)mpText->length();

    return nCursor;
}


bool ZWinTextEdit::OnChar(char c)
{
    switch (c)
    {

    case VK_ESCAPE:
        gMessageSystem.Post("quit_app_confirmed");
        break;
    default:
        if (!mbCTRLDown && c > 31 && c < 128)   // only specific chars
        {
            if (mbSelecting)
                DeleteSelection();

            mpText->insert(mCursorPosition, 1, c);
            HandleCursorMove(mCursorPosition + 1);
        }
        break;
    }


    return true;
}

void ZWinTextEdit::StartSelecting()
{
    mbSelecting = true;
    if (mnSelectionStart == -1)
        mnSelectionStart = mCursorPosition;
}

void ZWinTextEdit::CancelSelecting()
{
    mbSelecting = false;
    mnSelectionStart = -1;
    mnSelectionEnd = -1;
}


void ZWinTextEdit::DeleteSelection()
{
    if (mbSelecting)
    {
        mbSelecting = false;

        int64_t nMin = min<int64_t>(mnSelectionStart, mnSelectionEnd);
        int64_t nMax = max<int64_t>(mnSelectionStart, mnSelectionEnd);
        if (nMax > nMin)
        {
            mpText->erase(nMin, nMax - nMin);

            if (mCursorPosition >= nMax)
                HandleCursorMove(mCursorPosition - (nMax - nMin));
        }
        mnSelectionStart = -1;
        mnSelectionEnd = -1;
    }
}


bool ZWinTextEdit::OnKeyDown(uint32_t c)
{
    switch (c)
    {
    case VK_LEFT:
        if (mbSHIFTDown)
            StartSelecting();
        else
            CancelSelecting();

        if (mbCTRLDown)
            HandleCursorMove(FindNextBreak(-1));
        else
            HandleCursorMove(mCursorPosition - 1);
        break;
    case VK_RIGHT:
        if (mbSHIFTDown)
            StartSelecting();
        else
            CancelSelecting();

        if (mbCTRLDown)
            HandleCursorMove(FindNextBreak(1));
        else
            HandleCursorMove(mCursorPosition + 1);
        break;
    case VK_HOME:
        if (mbSHIFTDown)
            StartSelecting();
        else
            CancelSelecting();

        HandleCursorMove(0);
        break;
    case VK_END:
        if (mbSHIFTDown)
            StartSelecting();
        else
            CancelSelecting();

        HandleCursorMove(mpText->length());
        break;
    case VK_CONTROL:
        mbCTRLDown = true;
        break;
    case VK_SHIFT:
        mbSHIFTDown = true;
        break;
    case VK_DELETE:
    {
        if (mbSelecting)
        {
            DeleteSelection();
        }
        else if (mCursorPosition < (int64_t)mpText->size())
        {
            mpText->erase(mCursorPosition, 1);
            HandleCursorMove(mCursorPosition);
        }
    }
    break;
    case VK_BACK:
    {
        if (mbSelecting)
        {
            DeleteSelection();
        }
        else if (mCursorPosition > 0 && !mpText->empty())
        {
            mpText->erase(mCursorPosition - 1, 1);
            HandleCursorMove(mCursorPosition - 1);

        }
    }
    break;
    case 'a':
    case 'A':
        if (mbCTRLDown)
        {
            mnSelectionStart = 0;
            mnSelectionEnd = mpText->length();
            mbSelecting = true;
        }
        break;
    case 'c':
    case 'C':
        if (mbCTRLDown)
        {
            // do copy operation
            int64_t nMin = min<int64_t>(mnSelectionStart, mnSelectionEnd);
            int64_t nMax = max<int64_t>(mnSelectionStart, mnSelectionEnd);

            if (nMin < nMax)
            {
                ZOUT("copying:", mpText->substr(nMin, nMax - nMin), "\n");
            }
            break;
        }
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
    case VK_SHIFT:
        mbSHIFTDown = false;
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
                mnViewOffset = mrVisibleTextArea.left+nNewAbsPixelsToCursor - mnLeftBoundary + StyleOffset();
            }
        }
        else // moving right
        {
            if (newCursorPos < (int64_t)mpText->length())
            {
                if (nNewVisiblePixelsToCursor > mnRightBoundary) // moving right but cursor not at end... adjust 
                {
                    mnViewOffset = mrVisibleTextArea.left + nNewAbsPixelsToCursor - mnRightBoundary + StyleOffset();
                }
            }
            else
            {
                // cursor at end
                if (nNewVisiblePixelsToCursor > mrVisibleTextArea.right)
                {
                    mnViewOffset = mrVisibleTextArea.left + nNewAbsPixelsToCursor - mrVisibleTextArea.right+StyleOffset();
                }
            }
        }

        if (mnViewOffset < 0)
            mnViewOffset = 0;
        else if (mnViewOffset > 0 && mnViewOffset > nAbsStringPixels - mrVisibleTextArea.Width() + StyleOffset())
            mnViewOffset = nAbsStringPixels - mrVisibleTextArea.Width() + StyleOffset();
    }


    mrCursorArea.SetRect(mrVisibleTextArea.left+nNewAbsPixelsToCursor - mnViewOffset+StyleOffset(), mrVisibleTextArea.top, mrVisibleTextArea.left+nNewAbsPixelsToCursor - mnViewOffset + mrVisibleTextArea.Height()/32 + StyleOffset()+1, mrVisibleTextArea.bottom);

    if (newCursorPos >= 0 && newCursorPos <= (int64_t)mpText->length())
    {
        mCursorPosition = newCursorPos;

        if (mbSelecting)
        {
            mnSelectionEnd = mCursorPosition;
            ZOUT("select start:", mnSelectionStart, " end:", mnSelectionEnd,  "\n");
        }
    }
    Invalidate();
}

int64_t ZWinTextEdit::StyleOffset()
{
    if (mStyle.pos & ZGUI::HC)
    {
        int64_t nTextWidth = mStyle.Font()->StringWidth(*mpText);


        // if the string is larger than the view width, treat it as a left justified
        if (nTextWidth > mrVisibleTextArea.Width())
            return 0;

        int64_t nOffset = (mrVisibleTextArea.Width() - nTextWidth) / 2;
        return nOffset;
    }
    return 0;
}