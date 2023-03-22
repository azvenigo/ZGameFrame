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
    if (!msTooltipMessage.empty())
    {
        ZWinLabel* pWin = new ZWinLabel();
        pWin->SetText(msTooltipMessage);
        pWin->SetBehavior(ZWinLabel::CloseOnMouseOut| ZWinLabel::CloseOnClick);

        ZRect rTextArea;
        
        tZFontPtr pTooltipFont = gpFontSystem->GetFont(mTooltipFontParams);
        rTextArea = pTooltipFont->GetOutputRect(mAreaToDrawTo, (uint8_t*)msTooltipMessage.c_str(), msTooltipMessage.length(), mPosition);
        pWin->SetLook(mTooltipFontParams, mTooltipLook, mPosition, mTooltipFill);
        rTextArea.InflateRect(pTooltipFont->Height()/4, pTooltipFont->Height()/4);


        rTextArea.MoveRect(ZPoint(x - rTextArea.Width() + 16, y - rTextArea.Height() + 16));
//        rTextArea.MoveRect(0,0);
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



void ZWinLabel::SetLook(ZFontParams font, const ZTextLook& look, ZGUI::ePosition pos, bool bWrap, uint32_t nFill)
{ 
    mpFont = gpFontSystem->GetFont(font);  
    mLook = look;
    mPosition = pos;
    mFillColor = nFill;
    mbWrap = bWrap;
    Invalidate();
}

void ZWinLabel::UpdateLook(ZFontParams font)
{
    mpFont = gpFontSystem->GetFont(font);
    Invalidate();
}

void ZWinLabel::UpdateLook(const ZTextLook& look)
{
    mLook = look;
    Invalidate();
}

void ZWinLabel::UpdateLook(ZGUI::ePosition pos)
{
    mPosition = pos;
    Invalidate();
}

void ZWinLabel::UpdateLook(uint32_t fillColor)
{
    mFillColor = fillColor;
    Invalidate();
}



void ZWinLabel::SetText(const std::string& text )
{ 
    msText = text; 
    Invalidate();
}

void ZWinLabel::SetTooltip(const std::string& sMessage, ZFontParams* pTooltipFontParams, ZTextLook* pTooltipLook, bool bWrap, uint32_t nTooltipFill)
{ 
    msTooltipMessage = sMessage;
    if (pTooltipFontParams)
        mTooltipFontParams = *pTooltipFontParams;

    if (pTooltipLook)
        mTooltipLook = *pTooltipLook;

    mbTooltipWrap = bWrap;

    if (nTooltipFill > 0)
        mTooltipFill = nTooltipFill;

}

bool ZWinLabel::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return false;

    if (ARGB_A(mFillColor) > 5)
        mpTransformTexture->Fill(mAreaToDrawTo, mFillColor);

    if (mbWrap)
    {
        mpFont->DrawTextParagraph(mpTransformTexture.get(), msText, mAreaToDrawTo, mLook, mPosition);
    }
    else
    {
        ZRect rOut = mpFont->GetOutputRect(mAreaToDrawTo, (uint8_t*)msText.c_str(), msText.length(), mPosition);
        mpFont->DrawText(mpTransformTexture.get(), msText, rOut, mLook);
    }

    return ZWin::Paint();
}
