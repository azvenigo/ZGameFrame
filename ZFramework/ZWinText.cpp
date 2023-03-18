#pragma once

#include "ZWinText.h"


bool ZWinLabel::OnMouseHover(int64_t x, int64_t y)
{
    return ZWin::OnMouseHover(x, y);
}

void ZWinLabel::SetLook(tZFontPtr pFont, const ZTextLook& look, ZGUI::ePosition pos, uint32_t nFill)
{ 
    mpFont = pFont;  
    mLook = look;
    mPosition = pos;
    mFillColor = nFill;
    Invalidate();
}

void ZWinLabel::UpdateLook(tZFontPtr pFont)
{
    mpFont = pFont;
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

void ZWinLabel::SetHoverMessage(const std::string& sMessage)
{ 
    msHoverMessage = sMessage; 
}

bool ZWinLabel::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return true;

    if (ARGB_A(mFillColor) > 5)
        mpTransformTexture->Fill(mAreaToDrawTo, mFillColor);

    ZRect rOut = mpFont->GetOutputRect(mAreaToDrawTo, (uint8_t*)msText.c_str(), msText.length(), mPosition);
    mpFont->DrawTextA(mpTransformTexture.get(), msText, rOut, mLook);

    return ZWin::Paint();
}
