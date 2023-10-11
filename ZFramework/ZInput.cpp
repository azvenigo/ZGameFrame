#include "ZInput.h"
#include "helpers/StringHelpers.h"
#include "ZTimer.h"
#include "ZMainWin.h"
#include "ZWinText.H"


ZInput::ZInput() : captureWin(nullptr), mouseOverWin(nullptr), keyboardFocusWin(nullptr), lastMouseMove(-1, -1), lastMouseMoveTime(0), bMouseHoverPosted(false)
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

    gMessageSystem.Post("keydown", pTarget, "code", key);
}

void ZInput::OnKeyUp(uint32_t key)
{
    keyState[(uint8_t)key] &= ~0x80;

    IMessageTarget* pTarget = keyboardFocusWin;
    if (!pTarget)
        pTarget = gpMainWin;

    gMessageSystem.Post("keyup", pTarget, "code", key);
}

void ZInput::OnChar(uint32_t key)
{
    IMessageTarget* pTarget = keyboardFocusWin;
    if (!pTarget)
        pTarget = gpMainWin;

    gMessageSystem.Post("chardown", pTarget, "code", key);
}

void ZInput::OnLButtonUp(int64_t x, int64_t y)
{
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

bool ZInput::ShowTooltip(const std::string& tooltip, const ZGUI::Style& style)
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
    return true;
}

bool ZInput::UpdateTooltipLocation(ZPoint pt)
{
    if (!mpTooltipWin->mbVisible)
        return true;

    ZRect rTooltip(mpTooltipWin->GetArea());

    // ensure the tooltip is entirely visible on the window + a little padding
    ZPoint adjustedPt(lastMouseMove.x - rTooltip.Width()/2, lastMouseMove.y + gM/2);

    limit<int64_t>(adjustedPt.x, grFullArea.left + gSpacer, grFullArea.right - (rTooltip.Width() - gSpacer));
    limit<int64_t>(adjustedPt.y, grFullArea.top + gSpacer, grFullArea.bottom - (rTooltip.Height() - gSpacer));

    if ((toolTipAppear.x - pt.x) * (toolTipAppear.x - pt.x) + (toolTipAppear.y - pt.y) * (toolTipAppear.y - pt.y) > 2 * (gM * gM))
        mpTooltipWin->SetVisible(false);
    else
    {
        mpTooltipWin->MoveTo(adjustedPt.x, adjustedPt.y);
        mpTooltipWin->Invalidate();
    }

    return true;
}



