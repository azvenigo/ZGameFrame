#pragma once

#include "ZWinPaletteDialog.h"
#include "Resources.h"
#include "ZMainWin.h"

using namespace std;

const string kPaletteDialogName("PaletteDialog");

ZWinPaletteDialog::ZWinPaletteDialog()
{
    mbSelectingSV = false;
    mbSelectingH = false;
    mCurH = 0;
    mCurS = 0;
    mCurV = 0;
    msWinName = kPaletteDialogName;

}

bool ZWinPaletteDialog::Init()
{
    ComputeAreas();

    SelectPaletteIndex(0);

    return ZWinDialog::Init();
}

void ZWinPaletteDialog::ShowRecentColors(size_t nCount)
{
}

bool ZWinPaletteDialog::OnMouseDownL(int64_t x, int64_t y)
{
    if (mrSVArea.PtInRect(x, y))
    {
        SetCapture();
        mbSelectingSV = true;
        SelectSV(x-mrSVArea.left, y-mrSVArea.top);
        Invalidate();
        return false;
    }
    else if (mrHArea.PtInRect(x, y))
    {
        SetCapture();
        mbSelectingH = true;
        SelectH(y-mrHArea.top);
        Invalidate();
        return false;
    }
    else if (mrPaletteArea.PtInRect(x, y))
    {
        SelectFromPalette(x - mrPaletteArea.left, y - mrPaletteArea.top);
        return false;
    }

    return ZWinDialog::OnMouseDownL(x, y);
}

bool ZWinPaletteDialog::OnMouseUpL(int64_t x, int64_t y)
{
    if (AmCapturing())
    {
        ReleaseCapture();
        mbSelectingH = false;
        mbSelectingSV = false;
        return true;
    }
    return ZWinDialog::OnMouseUpL(x, y);
}

void ZWinPaletteDialog::SelectSV(int64_t x, int64_t y)
{
    if (x < 0)
        x = 0;
    else if (x > mrSVArea.Width())
        x = mrSVArea.Width();

    if (y < 0)
        y = 0;
    else if (y > mrSVArea.Height())
        y = mrSVArea.Height();

    double fScalar = 1023.0 / (double)mrSVArea.Height();

    mCurS = x * fScalar;
    mCurV = y * fScalar;
    UpdatePalette();
}

void ZWinPaletteDialog::SelectH(int64_t y)
{
    if (y < 0)
        y = 0;
    else if (y > mrHArea.Height())
        y = mrHArea.Height();

    double fScalar = 1023.0 / (double)mrSVArea.Height();

    mCurH = y * fScalar;
    UpdatePalette();
}

void ZWinPaletteDialog::UpdatePalette()
{
    ColorWatch& colWatch = mWatchArray[mnSelectingColorIndex];
    *colWatch.mpWatchColor = COL::AHSV_To_ARGB(0xff, mCurH, mCurS, mCurV);
}


void ZWinPaletteDialog::SelectFromPalette(int64_t x, int64_t y)
{
    if (x < 0)
        x = 0;
    else if (x > mrPaletteArea.Width())
        x = mrPaletteArea.Width();

    if (y < 0)
        y = 0;
    else if (y > mrPaletteArea.Height())
        y = mrPaletteArea.Height();


    size_t nSwatchWidth = mrPaletteArea.Width() / mWatchArray.size();
    size_t nIndex = x / nSwatchWidth;

    // if clicking on lower half, select the original
    bool bOriginal = y > mrPaletteArea.Height() / 2;

    SelectPaletteIndex(nIndex, bOriginal);
}

void ZWinPaletteDialog::SelectPaletteIndex(size_t nIndex, bool bOriginalColor)
{
    if (nIndex < mWatchArray.size())
    {
        mnSelectingColorIndex = nIndex;

        ColorWatch& colWatch = mWatchArray[mnSelectingColorIndex];

        if (bOriginalColor)
            *colWatch.mpWatchColor = colWatch.mOriginalColor;

        uint32_t hsv = COL::ARGB_To_AHSV(*colWatch.mpWatchColor);

        mCurH = AHSV_H(hsv);
        mCurS = AHSV_S(hsv);
        mCurV = AHSV_V(hsv);
        Invalidate();
    }
}


bool ZWinPaletteDialog::OnMouseMove(int64_t x, int64_t y)
{
    if (AmCapturing() && (mbSelectingSV || mbSelectingH))
    {
        if (mbSelectingSV)
        {
            SelectSV(x - mrSVArea.left, y - mrSVArea.top);
            Invalidate();
            return false;
        }
        else if (mbSelectingH)
        {
            SelectH(y-mrHArea.top);
            Invalidate();
            return false;
        }

        return false;
    }

    return ZWinDialog::OnMouseMove(x, y);
}

bool ZWinPaletteDialog::OnMouseWheel(int64_t x, int64_t y, int64_t nDelta)
{
    return ZWinDialog::OnMouseWheel(x, y, nDelta);
}

void ZWinPaletteDialog::OnOK()
{
    gMessageSystem.Post(msOnOKMessage);
    ZWinDialog::OnOK();
}

void ZWinPaletteDialog::OnCancel()
{
    // revert the colors00000
    for (auto watch : mWatchArray)
    {
        *watch.mpWatchColor = watch.mOriginalColor;
    }

    ZWinDialog::OnCancel();
}


bool ZWinPaletteDialog::Process()
{
    return ZWinDialog::Process();
}

bool ZWinPaletteDialog::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return false;

    ZWinDialog::Paint();

    tZFontPtr pTitleFont = gpFontSystem->GetFont(gDefaultTitleFont);
    pTitleFont->DrawText(mpTransformTexture.get(), msCaption, ZGUI::Arrange(pTitleFont->StringRect(msCaption), mAreaToDrawTo, ZGUI::ICIT, gSpacer, gSpacer), &ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xffeeeeee, 0xffaaaaaa));


    double fScalar = 1023.0/(double)mrSVArea.Height();


    ZRect rSVOutline(mrSVArea);
    rSVOutline.InflateRect(4, 4);
    mpTransformTexture.get()->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, rSVOutline, ZBuffer::kEdgeBltMiddle_None);


    for (int y = 0; y < mrSVArea.Height(); y++)
    {
        for (int x = 0; x < mrSVArea.Width(); x++)
        {
            uint32_t nS = (uint32_t)(x * fScalar);
            uint32_t nV = (uint32_t)(y * fScalar);


            mpTransformTexture->SetPixel(mrSVArea.left+x, mrSVArea.top+y, COL::AHSV_To_ARGB(0xff, mCurH, nS, nV));
        }
    }


    ZRect rHOutline(mrHArea);
    rHOutline.InflateRect(4, 4);
    mpTransformTexture.get()->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, rHOutline, ZBuffer::kEdgeBltMiddle_None);

    for (int y = 0; y < mrHArea.Height(); y++)
    {
        uint32_t nH = (uint32_t)(y * fScalar);
        uint32_t nCol = COL::AHSV_To_ARGB(0xff, nH, 1024, 1024);
        for (int x = 0; x < mrHArea.Width(); x++)
        {
            mpTransformTexture->SetPixel(mrHArea.left + x, mrHArea.top + y,nCol);
        }
    }


    ZPoint svCur(mrSVArea.left + mCurS / fScalar, mrSVArea.top +mCurV / fScalar);
    ZRect rsvCur(svCur.x - 5, svCur.y - 5, svCur.x + 5, svCur.y + 5);

    mpTransformTexture.get()->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, rsvCur, ZBuffer::kEdgeBltMiddle_None);


    ZPoint hCur(mrHArea.left, mrHArea.top +mCurH / fScalar);

    ZRect rhCur(mrHArea.left - 5, hCur.y - 5, mrHArea.right + 5, hCur.y + 5);
    mpTransformTexture.get()->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, rhCur, ZBuffer::kEdgeBltMiddle_None);


    // Color area
    ColorWatch& colWatch = mWatchArray[mnSelectingColorIndex];
    uint32_t hsv = COL::ARGB_To_AHSV(*colWatch.mpWatchColor);



    size_t nWatchedColors = mWatchArray.size();
    size_t nWatchedColorSwatchSide = mrPaletteArea.Width() / nWatchedColors;
    ZRect rSwatch(mrPaletteArea.left, mrPaletteArea.top, mrPaletteArea.left + nWatchedColorSwatchSide, mrPaletteArea.bottom);
    for (auto watch : mWatchArray)
    {
        ZRect rSwatchHalf(rSwatch);
        rSwatchHalf.bottom = rSwatch.top + rSwatch.Height() / 2;

        mpTransformTexture->Fill(*watch.mpWatchColor, &rSwatchHalf);
        mStyle.Font()->DrawText(mpTransformTexture.get(), watch.msWatchLabel, ZGUI::Arrange(mStyle.Font()->StringRect(watch.msWatchLabel), rSwatch, ZGUI::ICOT));

        rSwatchHalf.OffsetRect(0, rSwatchHalf.Height());
        mpTransformTexture->Fill(watch.mOriginalColor, &rSwatchHalf);

        rSwatch.OffsetRect(rSwatch.Width(), 0);
    }

    ZRect rCaption(mrPaletteArea);
    rCaption.bottom = (mrPaletteArea.top + mrPaletteArea.bottom) / 2;

    mStyle.Font()->DrawText(mpTransformTexture.get(), "new", ZGUI::Arrange(mStyle.Font()->StringRect("new"), rCaption, ZGUI::ORIC, gSpacer, gSpacer));
    rCaption.OffsetRect(0, rCaption.Height());
    mStyle.Font()->DrawText(mpTransformTexture.get(), "old", ZGUI::Arrange(mStyle.Font()->StringRect("old"), rCaption, ZGUI::ORIC, gSpacer, gSpacer));


    mpTransformTexture.get()->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, mrPaletteArea, ZBuffer::kEdgeBltMiddle_None);


    return ZWin::Paint();
}

void ZWinPaletteDialog::ComputeAreas()
{
    size_t nMeasure = mAreaToDrawTo.Width() / 20;
    size_t nHSSide = nMeasure * 15;

    mrSVArea.SetRect(nMeasure, nMeasure*2, nMeasure + nHSSide, nMeasure *2  + nHSSide);
    mrHArea.SetRect(mrSVArea.right + nMeasure, mrSVArea.top, mAreaToDrawTo.Width() - nMeasure, mrSVArea.bottom);

//    mrSelectingColorArea.SetRect(mrSVArea.left, mrSVArea.bottom + nMeasure, mrSVArea.left + nMeasure * 4, mrSVArea.bottom + nMeasure * 3);

    mrPaletteArea.SetRect(mrSVArea.left, mrHArea.bottom + nMeasure * 2, mrSVArea.right, mrHArea.bottom + nMeasure * 5);
}


ZWinPaletteDialog* ZWinPaletteDialog::ShowPaletteDialog(std::string sCaption, tColorWatchVector& watchArray, std::string sOnOKMeessage, size_t nRecentColors)
{
    ZWinPaletteDialog* pDialog = new ZWinPaletteDialog();

    ZRect rMain(gpMainWin->GetArea());

    ZRect r(0, 0, rMain.Width() / 6, rMain.Height() / 2.5);
    r = ZGUI::Arrange(r, rMain, ZGUI::C);

    pDialog->mBehavior |= ZWinDialog::eBehavior::Draggable | ZWinDialog::eBehavior::OKButton | ZWinDialog::eBehavior::CancelButton;
    pDialog->mStyle = gDefaultDialogStyle;
    pDialog->mWatchArray = watchArray;
    pDialog->SetArea(r);
    pDialog->msOnOKMessage = sOnOKMeessage;
    pDialog->msCaption = sCaption;

    gpMainWin->ChildAdd(pDialog);
    return pDialog;
}
