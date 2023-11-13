#pragma once

#include "ZWinPaletteDialog.h"
#include "Resources.h"
#include "ZMainWin.h"

using namespace std;

const string kPaletteDialogName("PaletteDialog");

ZWinPaletteDialog::ZWinPaletteDialog()
{
    mpColorMap = nullptr;
    mbSelectingSV = false;
    mbSelectingH = false;
    mCurH = 0;
    mCurS = 0;
    mCurV = 0;
    msWinName = kPaletteDialogName;
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
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

    mCurS = (uint32_t)(x * fScalar);
    mCurV = (uint32_t)(y * fScalar);
    UpdatePalette();
}

void ZWinPaletteDialog::SelectH(int64_t y)
{
    if (y < 0)
        y = 0;
    else if (y > mrHArea.Height())
        y = mrHArea.Height();

    double fScalar = 1023.0 / (double)mrSVArea.Height();

    mCurH = (uint32_t)(y * fScalar);
    UpdatePalette();
}

void ZWinPaletteDialog::UpdatePalette()
{
    if (!mpColorMap)
        return;

    ZGUI::EditableColor& col = (*mpColorMap)[mnSelectingColorIndex];
    uint32_t originalA = (mOriginalColorMap[mnSelectingColorIndex].col & 0xff000000);
    col.col = originalA | COL::AHSV_To_ARGB(0, mCurH, mCurS, mCurV);
}


void ZWinPaletteDialog::SelectFromPalette(int64_t x, int64_t y)
{
    if (!mpColorMap)
        return;

    if (x < 0)
        x = 0;
    else if (x > mrPaletteArea.Width())
        x = mrPaletteArea.Width();

    if (y < 0)
        y = 0;
    else if (y > mrPaletteArea.Height())
        y = mrPaletteArea.Height();


    size_t nSwatchWidth = mrPaletteArea.Width() / mpColorMap->size();
    size_t nIndex = x / nSwatchWidth;

    // if clicking on lower half, select the original
    bool bOriginal = y > mrPaletteArea.Height() / 2;

    SelectPaletteIndex(nIndex, bOriginal);
}

void ZWinPaletteDialog::SelectPaletteIndex(size_t nIndex, bool bOriginalColor)
{
    if (!mpColorMap)
        return;

    if (nIndex < mpColorMap->size())
    {
        mnSelectingColorIndex = nIndex;

        ZGUI::EditableColor& col = (*mpColorMap)[mnSelectingColorIndex];
        ZGUI::EditableColor& original = mOriginalColorMap[mnSelectingColorIndex];

        if (bOriginalColor)
            col.col = original.col;

        uint32_t hsv = COL::ARGB_To_AHSV(col.col);

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
    if (!mpColorMap)
        return;

    *mpColorMap = mOriginalColorMap;

    ZWinDialog::OnCancel();
}


bool ZWinPaletteDialog::Process()
{
    return ZWinDialog::Process();
}

bool ZWinPaletteDialog::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());
    if (!mbInvalid)
        return false;

    ZWinDialog::Paint();

    tZFontPtr pTitleFont = gpFontSystem->GetFont(gDefaultTitleFont);
    pTitleFont->DrawText(mpSurface.get(), msCaption, ZGUI::Arrange(pTitleFont->StringRect(msCaption), mAreaLocal, ZGUI::ICIT, gSpacer, gSpacer), &ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xffeeeeee, 0xffaaaaaa));


    double fScalar = 1023.0/(double)mrSVArea.Height();


    ZRect rSVOutline(mrSVArea);
    rSVOutline.InflateRect(4, 4);
    mpSurface.get()->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, rSVOutline, ZBuffer::kEdgeBltMiddle_None);


    for (int y = 0; y < mrSVArea.Height(); y++)
    {
        for (int x = 0; x < mrSVArea.Width(); x++)
        {
            uint32_t nS = (uint32_t)(x * fScalar);
            uint32_t nV = (uint32_t)(y * fScalar);


            mpSurface->SetPixel(mrSVArea.left+x, mrSVArea.top+y, COL::AHSV_To_ARGB(0xff, mCurH, nS, nV));
        }
    }


    ZRect rHOutline(mrHArea);
    rHOutline.InflateRect(4, 4);
    mpSurface.get()->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, rHOutline, ZBuffer::kEdgeBltMiddle_None);

    for (int y = 0; y < mrHArea.Height(); y++)
    {
        uint32_t nH = (uint32_t)(y * fScalar);
        uint32_t nCol = COL::AHSV_To_ARGB(0xff, nH, 1024, 1024);
        for (int x = 0; x < mrHArea.Width(); x++)
        {
            mpSurface->SetPixel(mrHArea.left + x, mrHArea.top + y,nCol);
        }
    }


    ZPoint svCur((int64_t) (mrSVArea.left + mCurS / fScalar), (int64_t)(mrSVArea.top +mCurV / fScalar));
    ZRect rsvCur(svCur.x - 5, svCur.y - 5, svCur.x + 5, svCur.y + 5);

    mpSurface.get()->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, rsvCur, ZBuffer::kEdgeBltMiddle_None);


    ZPoint hCur(mrHArea.left, (int64_t)(mrHArea.top +mCurH / fScalar));

    ZRect rhCur(mrHArea.left - 5, hCur.y - 5, mrHArea.right + 5, hCur.y + 5);
    mpSurface.get()->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, rhCur, ZBuffer::kEdgeBltMiddle_None);


    if (mpColorMap)
    {
        // Color area
        ZGUI::EditableColor& edCol = (*mpColorMap)[mnSelectingColorIndex];
        uint32_t hsv = COL::ARGB_To_AHSV(edCol.col);



        size_t nWatchedColors = (*mpColorMap).size();
        size_t nWatchedColorSwatchSide = mrPaletteArea.Width() / nWatchedColors;
        ZRect rSwatch(mrPaletteArea.left, mrPaletteArea.top, mrPaletteArea.left + nWatchedColorSwatchSide, mrPaletteArea.bottom);
        for (int i = 0; i < mpColorMap->size(); i++)
        {
            ZGUI::EditableColor& col = (*mpColorMap)[i];
            ZGUI::EditableColor& originalCol = mOriginalColorMap[i];
            ZRect rSwatchHalf(rSwatch);
            rSwatchHalf.bottom = rSwatch.top + rSwatch.Height() / 2;

            mpSurface->Fill(col.col, &rSwatchHalf);
            mStyle.Font()->DrawText(mpSurface.get(), col.name, ZGUI::Arrange(mStyle.Font()->StringRect(col.name), rSwatch, ZGUI::ICOT));

            rSwatchHalf.OffsetRect(0, rSwatchHalf.Height());
            mpSurface->Fill(originalCol.col, &rSwatchHalf);

            rSwatch.OffsetRect(rSwatch.Width(), 0);
        }

        ZRect rCaption(mrPaletteArea);
        rCaption.bottom = (mrPaletteArea.top + mrPaletteArea.bottom) / 2;

        mStyle.Font()->DrawText(mpSurface.get(), "new", ZGUI::Arrange(mStyle.Font()->StringRect("new"), rCaption, ZGUI::ORIC, gSpacer, gSpacer));
        rCaption.OffsetRect(0, rCaption.Height());
        mStyle.Font()->DrawText(mpSurface.get(), "old", ZGUI::Arrange(mStyle.Font()->StringRect("old"), rCaption, ZGUI::ORIC, gSpacer, gSpacer));


        mpSurface.get()->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, mrPaletteArea, ZBuffer::kEdgeBltMiddle_None);
    }

    return ZWin::Paint();
}

void ZWinPaletteDialog::ComputeAreas()
{
    size_t nMeasure = mAreaLocal.Width() / 20;
    size_t nHSSide = nMeasure * 15;

    mrSVArea.SetRect(nMeasure, nMeasure*2, nMeasure + nHSSide, nMeasure *2  + nHSSide);
    mrHArea.SetRect(mrSVArea.right + nMeasure, mrSVArea.top, mAreaLocal.Width() - nMeasure, mrSVArea.bottom);

//    mrSelectingColorArea.SetRect(mrSVArea.left, mrSVArea.bottom + nMeasure, mrSVArea.left + nMeasure * 4, mrSVArea.bottom + nMeasure * 3);

    mrPaletteArea.SetRect(mrSVArea.left, mrHArea.bottom + nMeasure * 2, mrSVArea.right, mrHArea.bottom + nMeasure * 5);
    ZWin::ComputeAreas();
}


ZWinPaletteDialog* ZWinPaletteDialog::ShowPaletteDialog(std::string sCaption, ZGUI::tColorMap* pColorMap, std::string sOnOKMeessage, size_t nRecentColors)
{
    assert(pColorMap);

    ZWinPaletteDialog* pDialog = new ZWinPaletteDialog();

    ZRect rMain(gpMainWin->GetArea());

    ZRect r(rMain.Width() / 6, (int64_t)(rMain.Height() / 2.5));
    r = ZGUI::Arrange(r, rMain, ZGUI::C);

    pDialog->mBehavior |= ZWinDialog::eBehavior::Draggable | ZWinDialog::eBehavior::OKButton | ZWinDialog::eBehavior::CancelButton;
    pDialog->mStyle = gDefaultDialogStyle;

    pDialog->mpColorMap = pColorMap;
    pDialog->mOriginalColorMap = *pColorMap;

    pDialog->SetArea(r);
    pDialog->msOnOKMessage = sOnOKMeessage;
    pDialog->msCaption = sCaption;
    pDialog->mTransformIn = ZWin::kSlideDown;

    gpMainWin->ChildAdd(pDialog, false);
    return pDialog;
}
