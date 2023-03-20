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
    if (!msHoverMessage.empty())
    {
        ZWinLabel* pWin = new ZWinLabel();
        pWin->SetText(msHoverMessage);
        pWin->SetBehavior(ZWinLabel::CloseOnMouseOut);

        ZRect rTextArea;
        
        tZFontPtr pHoverFont = gpFontSystem->GetFont(mHoverFontParams);
        rTextArea = pHoverFont->GetOutputRect(mAreaToDrawTo, (uint8_t*)msHoverMessage.c_str(), msHoverMessage.length(), mPosition);
        pWin->SetLook(mHoverFontParams, mHoverLook, mPosition, mHoverFill);
        rTextArea.InflateRect(pHoverFont->Height()/4, pHoverFont->Height()/4);


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



void ZWinLabel::SetLook(ZFontParams font, const ZTextLook& look, ZGUI::ePosition pos, uint32_t nFill)
{ 
    mpFont = gpFontSystem->GetFont(font);  
    mLook = look;
    mPosition = pos;
    mFillColor = nFill;
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

void ZWinLabel::SetHoverMessage(const std::string& sMessage, ZFontParams* pHoverFontParams, ZTextLook* pHoverLook, uint32_t nHoverFill)
{ 
    msHoverMessage = sMessage; 
    if (pHoverFontParams)
        mHoverFontParams = *pHoverFontParams;

    if (pHoverLook)
        mHoverLook = *pHoverLook;

    if (nHoverFill > 0)
        mHoverFill = nHoverFill;

}

bool ZWinLabel::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return false;

    if (ARGB_A(mFillColor) > 5)
        mpTransformTexture->Fill(mAreaToDrawTo, mFillColor);

    ZRect rOut = mpFont->GetOutputRect(mAreaToDrawTo, (uint8_t*)msText.c_str(), msText.length(), mPosition);
    mpFont->DrawTextA(mpTransformTexture.get(), msText, rOut, mLook);

    return ZWin::Paint();
}
