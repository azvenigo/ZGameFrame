#include "ZInput.h"
#include "helpers/StringHelpers.h"
#include "ZTimer.h"
#include "ZMainWin.h"
#include "ZWinText.H"

using namespace std;

ZInput::ZInput() : captureWin(nullptr), mouseOverWin(nullptr), keyboardFocusWin(nullptr), lastMouseMove(-1, -1), lastMouseMoveTime(0), bMouseHoverPosted(false), lButtonDown(false), rButtonDown(false)
{
    mpTooltipWin = new ZWinLabel();
}


void ZInput::OnKeyDown(uint32_t key)
{
    // for certain keys, don't send repeat events
    if (key == VK_MENU || key == VK_CONTROL || key == VK_SHIFT)
    {
        if ((keyState[(uint8_t)key] & 0x80) != 0)
            return;
    }

    keyState[(uint8_t)key] |= 0x80;

    IMessageTarget* pTarget = keyboardFocusWin;
    if (!pTarget)
        pTarget = gpMainWin;

    gMessageSystem.Post("{keydown}", pTarget, "code", key);
}

void ZInput::OnKeyUp(uint32_t key)
{
    keyState[(uint8_t)key] &= ~0x80;

    IMessageTarget* pTarget = keyboardFocusWin;
    if (!pTarget)
        pTarget = gpMainWin;

    gMessageSystem.Post("{keyup}", pTarget, "code", key);
}

void ZInput::OnChar(uint32_t key)
{
    IMessageTarget* pTarget = keyboardFocusWin;
    if (!pTarget)
        pTarget = gpMainWin;

    gMessageSystem.Post("{chardown}", pTarget, "code", key);
}

void ZInput::OnLButtonUp(int64_t x, int64_t y)
{
    lButtonDown = false;
    ZMessage cursorMessage;
    cursorMessage.SetType("cursor_msg");
    cursorMessage.SetParam("subtype", "l_up");
    cursorMessage.SetParam("x", SH::FromInt(x));
    cursorMessage.SetParam("y", SH::FromInt(y));
    if (captureWin)
        cursorMessage.SetTarget(captureWin->GetTargetName());
    else
        cursorMessage.SetTarget(gpMainWin->GetTargetName());

    gMessageSystem.Post(cursorMessage);
}

void ZInput::OnLButtonDown(int64_t x, int64_t y)
{
    lButtonDown = true;
    ZMessage cursorMessage;
    cursorMessage.SetType("cursor_msg");
    cursorMessage.SetParam("subtype", "l_down");
    cursorMessage.SetParam("x", SH::FromInt(x));
    cursorMessage.SetParam("y", SH::FromInt(y));
    if (captureWin)
        cursorMessage.SetTarget(captureWin->GetTargetName());
    else
        cursorMessage.SetTarget(gpMainWin->GetTargetName());

    gMessageSystem.Post(cursorMessage);
}

void ZInput::OnRButtonUp(int64_t x, int64_t y)
{
    rButtonDown = false;
    ZMessage cursorMessage;
    cursorMessage.SetType("cursor_msg");
    cursorMessage.SetParam("subtype", "r_up");
    cursorMessage.SetParam("x", SH::FromInt(x));
    cursorMessage.SetParam("y", SH::FromInt(y));
    if (captureWin)
        cursorMessage.SetTarget(captureWin->GetTargetName());
    else
        cursorMessage.SetTarget(gpMainWin->GetTargetName());

    gMessageSystem.Post(cursorMessage);
}

void ZInput::OnRButtonDown(int64_t x, int64_t y)
{
    rButtonDown = true;
    mouseDown.Set(x, y);

    ZMessage cursorMessage;
    cursorMessage.SetType("cursor_msg");
    cursorMessage.SetParam("subtype", "r_down");
    cursorMessage.SetParam("x", SH::FromInt(x));
    cursorMessage.SetParam("y", SH::FromInt(y));
    if (captureWin)
        cursorMessage.SetTarget(captureWin->GetTargetName());
    else
        cursorMessage.SetTarget(gpMainWin->GetTargetName());

    gMessageSystem.Post(cursorMessage);
}


void ZInput::OnMouseMove(int64_t x, int64_t y)
{
    if (lastMouseMove != ZPoint(x,y))
    {
        lastMouseMove.Set(x, y);
        lastMouseMoveTime = gTimer.GetElapsedTime();

        ZMessage cursorMessage;
        cursorMessage.SetType("cursor_msg");
        cursorMessage.SetParam("subtype", "move");
        cursorMessage.SetParam("x", SH::FromInt(x));
        cursorMessage.SetParam("y", SH::FromInt(y));
        if (captureWin)
            cursorMessage.SetTarget(captureWin->GetTargetName());
        else
            cursorMessage.SetTarget(gpMainWin->GetTargetName());

        gMessageSystem.Post(cursorMessage);

        CheckForMouseOverNewWindow();

        if (mpTooltipWin->mbVisible)
            UpdateTooltipLocation(lastMouseMove);
    }
}

void ZInput::Process()
{
    CheckMouseForHover();
}


void ZInput::OnMouseWheel(int64_t x, int64_t y, int64_t delta)
{
    ZMessage cursorMessage;
    cursorMessage.SetType("cursor_msg");
    cursorMessage.SetParam("subtype", "wheel");
    cursorMessage.SetParam("x", SH::FromInt(x));
    cursorMessage.SetParam("y", SH::FromInt(y));
    cursorMessage.SetParam("delta", SH::FromInt(delta));
    if (captureWin)
        cursorMessage.SetTarget(captureWin->GetTargetName());
    else
        cursorMessage.SetTarget(gpMainWin->GetTargetName());
    gMessageSystem.Post(cursorMessage);
}

void ZInput::CheckForMouseOverNewWindow()
{
    ZWin* pMouseOverWin = gpMainWin->GetChildWindowByPoint(lastMouseMove.x, lastMouseMove.y);

    // Check whether a "cursor out" message needs to be sent.  (i.e. a previous window had the cursor over it but no longer)
    if (mouseOverWin != pMouseOverWin)
    {
        //ZOUT("bMouseHoverPosted = false\n");

        bMouseHoverPosted = false;	// Only post a mouse hover message if the cursor has moved into a new window

        // Mouse Out of old window
        if (mouseOverWin)
            mouseOverWin->OnMouseOut();

        mouseOverWin = pMouseOverWin;

        // Mouse In to new window
        if (pMouseOverWin)
            pMouseOverWin->OnMouseIn();
    }
}

uint32_t    kMouseHoverInitialTime = 200;

void ZInput::CheckMouseForHover()
{
    if (lButtonDown || rButtonDown)
        return;

    uint64_t nCurTime = gTimer.GetElapsedTime();

    if (!bMouseHoverPosted && nCurTime - lastMouseMoveTime > kMouseHoverInitialTime)
    {
        ZWin* pMouseOverWin = gpMainWin->GetChildWindowByPoint(lastMouseMove.x, lastMouseMove.y);
        if (pMouseOverWin)
        {
            ZMessage cursorMessage;
            cursorMessage.SetType("cursor_msg");
            cursorMessage.SetParam("subtype", "hover");
            cursorMessage.SetParam("x", SH::FromInt(lastMouseMove.x));
            cursorMessage.SetParam("y", SH::FromInt(lastMouseMove.y));
            if (captureWin)
                cursorMessage.SetTarget(captureWin->GetTargetName());
            else
                cursorMessage.SetTarget(gpMainWin->GetTargetName());

            gMessageSystem.Post(cursorMessage);
        }

        bMouseHoverPosted = true;
    }
}

bool ZInput::ShowTooltip(const string& tooltip, const ZGUI::Style& style)
{
    if (!mpTooltipWin->mbInitted)
        gpMainWin->ChildAdd(mpTooltipWin);
    if (mpTooltipWin->Init())
    {
        mpTooltipWin->mIdleSleepMS = 100;
    }
    mpTooltipWin->msText = tooltip;
    mpTooltipWin->mStyle = style;
    ZRect rTooltip;
    tZFontPtr pTooltipFont = mpTooltipWin->mStyle.Font();
    rTooltip = pTooltipFont->StringRect(tooltip);
    rTooltip.InflateRect(style.paddingH, style.paddingV);
    mpTooltipWin->SetArea(rTooltip);
    mpTooltipWin->SetVisible();
    toolTipAppear = lastMouseMove;
    UpdateTooltipLocation(lastMouseMove);
    bMouseHoverPosted = true;   // set true so that it's not replaced by other hover
    return true;
}

bool ZInput::UpdateTooltipLocation(ZPoint pt)
{
    if (!mpTooltipWin->mbVisible)
        return true;

    ZRect rTooltip(mpTooltipWin->GetArea());

    // ensure the tooltip is entirely visible on the window + a little padding
    ZPoint adjustedPt(lastMouseMove.x - rTooltip.Width()/16, lastMouseMove.y + gM);

    limit<int64_t>(adjustedPt.x, grFullArea.left + gSpacer, grFullArea.right - (rTooltip.Width() - gSpacer));
    limit<int64_t>(adjustedPt.y, grFullArea.top + gSpacer, grFullArea.bottom - (rTooltip.Height() - gSpacer));

    if ((toolTipAppear.x - pt.x) * (toolTipAppear.x - pt.x) + (toolTipAppear.y - pt.y) * (toolTipAppear.y - pt.y) > 3 * (gM * gM))
        mpTooltipWin->SetVisible(false);
    else
    {
        mpTooltipWin->MoveTo(adjustedPt.x, adjustedPt.y);
        mpTooltipWin->Invalidate();
    }

    return true;
}

string ZInput::GetClipboard()
{
#ifdef _WIN64
    if (OpenClipboard(nullptr)) 
    {
        HANDLE hData = GetClipboardData(CF_TEXT);

        if (hData != nullptr) 
        {
            char* text = static_cast<char*>(GlobalLock(hData));

            if (text != nullptr) 
            {
                mClipboardText.assign(text, strlen(text));
                ZOUT("Text copied from clipboard\n");

                GlobalUnlock(hData);
            }
            else 
            {
                ZERROR("Failed to lock memory.\n");
            }
        }
        else 
        {
            ZDEBUG_OUT("Failed to get clipboard data.\n");
        }

        CloseClipboard();
    }
    else 
    {
        ZDEBUG_OUT("Failed to open OS clipboard.\n");
    }
#endif

    return mClipboardText;
}

bool ZInput::SetClipboard(const std::string& text)
{
    mClipboardText = text;
    ZOUT("Set Clipboard to:", text);

#ifdef _WIN64
    if (OpenClipboard(nullptr))
    {
        EmptyClipboard();

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, gInput.mClipboardText.length() + 1);

        if (hMem != nullptr)
        {
            strcpy_s(static_cast<char*>(GlobalLock(hMem)), gInput.mClipboardText.length() + 1, gInput.mClipboardText.data());
            GlobalUnlock(hMem);

            SetClipboardData(CF_TEXT, hMem);
            CloseClipboard();

            ZOUT("Copied to clipboard\n");
            return true;
        }
        else
        {
            std::cerr << "Failed to allocate memory." << std::endl;
            return false;
        }
    }
#endif
    return true;
}

bool ZInput::SetClipboard(ZBuffer* pBuffer)
{
#ifdef _WIN64
    if (!OpenClipboard(nullptr)) 
        return false;

    if (!EmptyClipboard()) 
    {
        CloseClipboard();
        return false;
    }

    ZRect r(pBuffer->GetArea());

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = (LONG)r.Width();
    bmi.bmiHeader.biHeight = (LONG)-r.Height();
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;


    size_t dataSize = sizeof(BITMAPINFOHEADER) + r.Area() * 4;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, dataSize);
    if (hMem == NULL)
        return false;

    void* lpMem = GlobalLock(hMem);
    if (lpMem == NULL)
    {
        GlobalFree(hMem);
        return false;
    }

    memcpy(lpMem, &bmi, sizeof(bmi));
    memcpy((uint8_t*)lpMem+sizeof(bmi), pBuffer->mpPixels, r.Area() * 4);

    GlobalUnlock(hMem);

    if (!SetClipboardData(CF_DIB, hMem))
{
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
#else
    return false;
#endif
    
}

