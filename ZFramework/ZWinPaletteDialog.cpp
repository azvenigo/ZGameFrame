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
    mpRGBEdit = nullptr;

    if (msWinName.empty())
        msWinName = "ZWinPaletteDialog_" + gMessageSystem.GenerateUniqueTargetName();
}

bool ZWinPaletteDialog::Init()
{
    mpRGBEdit = new ZWinTextEdit(&msRGBTextValue);
    mpRGBEdit->mStyle = gStyleButton;
    ChildAdd(mpRGBEdit);

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
    else if (mrPaletteEntryArea.PtInRect(x, y))
    {
        SelectFromPalette(x - mrPaletteEntryArea.left, y - mrPaletteEntryArea.top);
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

    string s = SH::ToHexString(col.col);
    if (msRGBTextValue != s)
    {
        msRGBTextValue = s;
        if (mpRGBEdit)
            mpRGBEdit->Invalidate();
    }
}


void ZWinPaletteDialog::SelectFromPalette(int64_t x, int64_t y)
{
    if (!mpColorMap)
        return;

    limit<int64_t>(x, 0, mrPaletteEntryArea.Width());
    limit<int64_t>(y, 0, mrPaletteEntryArea.Height());

    size_t nIndex = y / gM;


    ZRect rColorEntry(mrPaletteEntryArea.left, mrPaletteEntryArea.top, mrPaletteEntryArea.right, mrPaletteEntryArea.top + gM);
    rColorEntry.OffsetRect(0, nIndex * gM);

    ZRect rCurColor(rColorEntry);
    rCurColor.left = rCurColor.right - rColorEntry.Width() / 3;
    rCurColor.MoveRect(mrPaletteEntryArea.Width() - gM, nIndex*gM);

    ZRect rOldColor(rCurColor);
    rOldColor.OffsetRect(-rCurColor.Width(), 0);

    ZRect rDefaultColor(rOldColor);
    rDefaultColor.OffsetRect(-rCurColor.Width(), 0);

    if (rCurColor.PtInRect(x, y))
        SelectPaletteIndex(nIndex, kEdited);
    else if (rOldColor.PtInRect(x, y))
        SelectPaletteIndex(nIndex, kOriginal);
    else if (rDefaultColor.PtInRect(x, y))
        SelectPaletteIndex(nIndex, kDefault);
}

void ZWinPaletteDialog::SelectPaletteIndex(size_t nIndex, eCategory category)
{
    if (!mpColorMap)
        return;

    if (nIndex < mpColorMap->size())
    {
        mnSelectingColorIndex = nIndex;

        ZGUI::EditableColor& col = (*mpColorMap)[mnSelectingColorIndex];
        ZGUI::EditableColor& original = mOriginalColorMap[mnSelectingColorIndex];

        if (category == kOriginal)
            col.col = original.col;
        else if (category == kDefault)
            col.col = col.default_color;

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

    ZGUI::Style titleStyle(gStyleCaption);
    titleStyle.look.colTop = 0xffeeeeee;
    titleStyle.look.colBottom = 0xffaaaaaa;
    titleStyle.look.decoration = ZGUI::ZTextLook::kShadowed;
    titleStyle.pos = ZGUI::CT;

    titleStyle.Font()->DrawText(mpSurface.get(), msCaption, mrTitleArea, &titleStyle);


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



        ZRect rColorEntry(mrPaletteEntryArea.left, mrPaletteEntryArea.top, mrPaletteEntryArea.right, mrPaletteEntryArea.top + gM);

        for (int i = 0; i < mpColorMap->size(); i++)
        {


            ZGUI::EditableColor& col = (*mpColorMap)[i];
            ZGUI::EditableColor& originalCol = mOriginalColorMap[i];

            ZRect rSwatch(rColorEntry);
            rSwatch.left = rSwatch.right - rColorEntry.Width()/3;

            mpSurface->Fill(col.col, &rSwatch); // cur color
            rSwatch.OffsetRect(-rSwatch.Width(), 0);
            mpSurface->Fill(originalCol.col, &rSwatch); // original.col
            rSwatch.OffsetRect(-rSwatch.Width(), 0);
            mpSurface->Fill(originalCol.default_color, &rSwatch); // default color

            mStyle.Font()->DrawText(mpSurface.get(), col.name, ZGUI::Arrange(mStyle.Font()->StringRect(col.name), rColorEntry, ZGUI::ILIC, gSpacer, 0));


            rColorEntry.OffsetRect(0, rColorEntry.Height());
        }


        ZRect rCaption(mrPaletteEntryArea.Width()/3, mStyle.fp.nHeight);
        rCaption = ZGUI::Arrange(rCaption, mrPaletteEntryArea, ZGUI::ILOT, gSpacer, gSpacer);


        ZGUI::Style style(mStyle);
        style.pos = ZGUI::CB;

        mStyle.Font()->DrawText(mpSurface.get(), "def", rCaption, &style);
        rCaption.OffsetRect(rCaption.Width(), 0);
        mStyle.Font()->DrawText(mpSurface.get(), "prev", rCaption, &style);
        rCaption.OffsetRect(rCaption.Width(), 0);
        mStyle.Font()->DrawText(mpSurface.get(), "new", rCaption, &style);

        mpSurface.get()->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, mrPaletteEntryArea, ZBuffer::kEdgeBltMiddle_None);
    }

    return ZWin::Paint();
}

void ZWinPaletteDialog::ComputeAreas()
{
    ZRect rFull = mAreaLocal;
    rFull.DeflateRect(gM, gM);

    int64_t nHSVSide = rFull.Width();
    if (rFull.Height() < rFull.Width())
        nHSVSide = rFull.Height();
    nHSVSide = nHSVSide * 2 / 3;

    mrTitleArea = ZGUI::Arrange(ZRect(rFull.Width(), gDefaultTitleFont.nHeight + gSpacer * 2), rFull, ZGUI::ICIT, gSpacer, gSpacer);


    ZRect rHSVArea = ZGUI::Arrange(ZRect(nHSVSide, nHSVSide), rFull, ZGUI::IRIT, gSpacer, mrTitleArea.bottom + gM);    // top right under title area

    int64_t nSVSide = nHSVSide - gM * 2;

    mrHArea = ZGUI::Arrange(ZRect(gM, nSVSide), rHSVArea, ZGUI::IRIT);                               // Right HUE rectangle
    mrSVArea = ZGUI::Arrange(ZRect(nSVSide, nSVSide), rHSVArea, ZGUI::ILIT);               // Left SV square

    mrColorValueBoxesArea.SetRect(rHSVArea.left, rHSVArea.bottom + gSpacer, rHSVArea.right, rFull.bottom);
    mrPaletteEntryArea.SetRect(rFull.left, rHSVArea.top, rHSVArea.left - gM, rHSVArea.bottom);


    if (mpRGBEdit)
    {
        ZRect rEdit(mStyle.Font()->StringRect("0X00000000"));   // enough space for ARGB hex
        rEdit.InflateRect(gSpacer, gSpacer);
        rEdit = ZGUI::Arrange(rEdit, mrSVArea, ZGUI::ILOB, 0, gM);
        mpRGBEdit->SetArea(rEdit);
    }

    ZWin::ComputeAreas();
}


ZWinPaletteDialog* ZWinPaletteDialog::ShowPaletteDialog(std::string sCaption, ZGUI::tColorMap* pColorMap, std::string sOnOKMeessage, size_t nRecentColors)
{
    assert(pColorMap);

    ZWinPaletteDialog* pDialog = new ZWinPaletteDialog();

    ZRect rMain(gpMainWin->GetArea());

    ZRect r(rMain.Width() / 3, (int64_t)(rMain.Width() / 4));
    r = ZGUI::Arrange(r, rMain, ZGUI::C);

    pDialog->mBehavior |= ZWinDialog::eBehavior::Draggable | ZWinDialog::eBehavior::OKButton | ZWinDialog::eBehavior::CancelButton;
    pDialog->mStyle = gDefaultDialogStyle;
    pDialog->mStyle.fp = gStyleButton.fp;

    pDialog->mpColorMap = pColorMap;
    pDialog->mOriginalColorMap = *pColorMap;

    pDialog->SetArea(r);
    pDialog->msOnOKMessage = sOnOKMeessage;
    pDialog->msCaption = sCaption;
    pDialog->mTransformIn = ZWin::kSlideDown;

    gpMainWin->ChildAdd(pDialog, false);
    return pDialog;
}
