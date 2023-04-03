#pragma once

#include "ZWinPaletteDialog.h"
#include "Resources.h"
#include "ZMainWin.h"


ZWinPaletteDialog::ZWinPaletteDialog()
{
    mbSelectingSV = false;
    mbSelectingH = false;
    mCurH = 0;
    mCurS = 0;
    mCurV = 0;

}

bool ZWinPaletteDialog::Init()
{
    ComputeAreas();

    ColorWatch& colWatch = *mWatchList.begin();
    uint32_t hsv = COL::ARGB_To_AHSV(*colWatch.mpWatchColor);

    mCurH = AHSV_H(hsv);
    mCurS = AHSV_S(hsv);
    mCurV = AHSV_V(hsv);

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
}

void ZWinPaletteDialog::SelectH(int64_t y)
{
    if (y < 0)
        y = 0;
    else if (y > mrHArea.Height())
        y = mrHArea.Height();

    double fScalar = 1023.0 / (double)mrSVArea.Height();

    mCurH = y * fScalar;
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


    ColorWatch& colWatch = *mWatchList.begin();
    uint32_t hsv = COL::ARGB_To_AHSV(*colWatch.mpWatchColor);

    ZRect rOriginal(mrSelectingColorArea);
    rOriginal.right = rOriginal.left + mrSelectingColorArea.Width() / 2;
    mpTransformTexture->Fill(rOriginal, *colWatch.mpWatchColor);
    rOriginal.left = mrSelectingColorArea.left + mrSelectingColorArea.Width() / 2;
    rOriginal.right = mrSelectingColorArea.right;
    mpTransformTexture->Fill(rOriginal, COL::AHSV_To_ARGB(0xff, mCurH, mCurS, mCurV));

    mpTransformTexture.get()->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, mrSelectingColorArea, ZBuffer::kEdgeBltMiddle_None);


    return ZWin::Paint();
}

void ZWinPaletteDialog::ComputeAreas()
{
    size_t nMeasure = mAreaToDrawTo.Width() / 10;
    size_t nHSSide = nMeasure * 6;

    mrSVArea.SetRect(nMeasure, nMeasure, nMeasure + nHSSide, nMeasure + nHSSide);
    mrHArea.SetRect(mrSVArea.right + nMeasure, nMeasure, mAreaToDrawTo.Width() - nMeasure, mrSVArea.bottom);

    mrSelectingColorArea.SetRect(mrSVArea.left, mrSVArea.bottom + nMeasure, mrSVArea.left + nMeasure * 2, mrSVArea.bottom + nMeasure * 2);
}


ZWinPaletteDialog* ZWinPaletteDialog::ShowPaletteDialog(std::string sCaption, ColorWatch& watch, size_t nRecentColors)
{
    ZWinPaletteDialog* pDialog = new ZWinPaletteDialog();

    ZRect rMain(gpMainWin->GetArea());

    ZRect r(0, 0, rMain.Width() / 4, rMain.Width() / 4);
    r = ZGUI::Arrange(r, rMain, ZGUI::C);

    pDialog->mBehavior |= ZWinDialog::eBehavior::Draggable | ZWinDialog::eBehavior::OKButton | ZWinDialog::eBehavior::CancelButton;
    pDialog->mStyle = gDefaultDialogStyle;
    pDialog->mWatchList.push_back(watch);
    pDialog->SetArea(r);

    gpMainWin->ChildAdd(pDialog);
    return pDialog;
}
