#pragma once

#include "ZWinText.h"




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
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
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
