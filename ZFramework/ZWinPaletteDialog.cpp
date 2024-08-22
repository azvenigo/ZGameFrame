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
    mbTransformable = true;
    mbAcceptsFocus = true;
    mpRGBEdit = nullptr;
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
        SelectFromPalette(x, y);
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
    UpdateTextEdit();
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
    UpdateTextEdit();
}

void ZWinPaletteDialog::UpdatePalette()
{
    if (!mpColorMap)
        return;

    ZGUI::EditableColor& col = (*mpColorMap)[mnSelectingColorIndex];
    uint32_t originalA = (mOriginalColorMap[mnSelectingColorIndex].col & 0xff000000);
    col.col = originalA | COL::AHSV_To_ARGB(0, mCurH, mCurS, mCurV);
}

void ZWinPaletteDialog::UpdateTextEdit()
{
    ZGUI::EditableColor& col = (*mpColorMap)[mnSelectingColorIndex];

    string s = SH::ToHexString(col.col).substr(2);  // strip off 0X
    if (msRGBTextValue != s)
    {
        msRGBTextValue = s;
        if (mpRGBEdit)
            mpRGBEdit->Invalidate();
    }
}


ZRect ZWinPaletteDialog::PaletteRect(int64_t nIndex, eCategory category)
{
    ZRect r(mrPaletteEntryArea.left, mrPaletteEntryArea.top, mrPaletteEntryArea.right, mrPaletteEntryArea.top + mnPaletteEntryHeight);
    int64_t w = r.Width() / 3;

    r.OffsetRect(0, nIndex * mnPaletteEntryHeight);

    if (category == kDefault)
        return ZRect(r.left, r.top, r.left + w, r.bottom);
    else if (category == kOriginal)
        return ZRect(r.left+2, r.top, r.right - w, r.bottom);

    return ZRect(r.left + 2*w, r.top, r.right, r.bottom);
}

void ZWinPaletteDialog::SelectFromPalette(int64_t x, int64_t y)
{
    if (!mpColorMap)
        return;

    size_t nIndex = (y-mrPaletteEntryArea.top) / mnPaletteEntryHeight;

    ZRect rE(PaletteRect(nIndex, kEdited));
    ZRect rO(PaletteRect(nIndex, kOriginal));
    ZRect rD(PaletteRect(nIndex, kDefault));

    if (rE.PtInRect(x,y))
        SelectPaletteIndex(nIndex, kEdited);
    if (rO.PtInRect(x, y))
        SelectPaletteIndex(nIndex, kOriginal);
    if (rD.PtInRect(x, y))
        SelectPaletteIndex(nIndex, kDefault);
}

void ZWinPaletteDialog::SelectPaletteIndex(size_t nIndex, eCategory category)
{
    if (!mpColorMap)
        return;

    if (nIndex < mpColorMap->size())
    {
        mnSelectingColorIndex = nIndex;

        ZGUI::EditableColor& original = mOriginalColorMap[mnSelectingColorIndex];

        if (category == kOriginal)
            SetCurColor(original.col);
        else if (category == kDefault)
            SetCurColor(original.default_color);

        UpdateTextEdit();
    }
}

void ZWinPaletteDialog::SetCurColor(uint32_t col)
{
    ZGUI::EditableColor& editCol = (*mpColorMap)[mnSelectingColorIndex];
    editCol.col = col;

    uint32_t hsv = COL::ARGB_To_AHSV(col);

    mCurH = AHSV_H(hsv);
    mCurS = AHSV_S(hsv);
    mCurV = AHSV_V(hsv);

    Invalidate();
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

    ZDEBUG_OUT("palette dialog on cancel\n");

    ZWinDialog::OnCancel();
}


bool ZWinPaletteDialog::Process()
{
    return ZWinDialog::Process();
}

bool ZWinPaletteDialog::Paint()
{
    if (!PrePaintCheck())
        return false;
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

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

        for (int i = 0; i < mpColorMap->size(); i++)
        {
            ZGUI::EditableColor& col = (*mpColorMap)[i];
            ZGUI::EditableColor& originalCol = mOriginalColorMap[i];

            ZRect re = PaletteRect(i, kEdited);
            ZRect ro = PaletteRect(i, kOriginal);
            ZRect rd = PaletteRect(i, kDefault);
            mpSurface->Fill(col.col, &re); // cur color
            mpSurface->Fill(originalCol.col, &ro); // original.col
            mpSurface->Fill(originalCol.default_color, &rd); // default color

            mpSurface->DrawRectAlpha(0x88000000, PaletteRect(i, kEdited));
            mpSurface->DrawRectAlpha(0x88000000, PaletteRect(i, kOriginal));
            mpSurface->DrawRectAlpha(0x88000000, PaletteRect(i, kDefault));

            ZRect rCaption(PaletteRect(i, kDefault));
            mStyle.Font()->DrawText(mpSurface.get(), col.name, ZGUI::Arrange(mStyle.Font()->StringRect(col.name), rCaption, ZGUI::ILIC, gSpacer, 0));
        }


        ZRect rCaption(mrPaletteEntryArea.Width()/3, mStyle.fp.Height());
        rCaption = ZGUI::Arrange(rCaption, mrPaletteEntryArea, ZGUI::ILOT, gSpacer, gSpacer);


        ZGUI::Style style(mStyle);
        style.pos = ZGUI::CB;

        mStyle.Font()->DrawText(mpSurface.get(), "reset", rCaption, &style);
        rCaption.OffsetRect(rCaption.Width(), 0);
        mStyle.Font()->DrawText(mpSurface.get(), "undo", rCaption, &style);
        rCaption.OffsetRect(rCaption.Width(), 0);
        mStyle.Font()->DrawText(mpSurface.get(), "edit", rCaption, &style);

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

    mrTitleArea = ZGUI::Arrange(ZRect(rFull.Width(), gDefaultTitleFont.Height() + gSpacer * 2), rFull, ZGUI::ICIT, gSpacer, gSpacer);


    ZRect rHSVArea = ZGUI::Arrange(ZRect(nHSVSide, nHSVSide), rFull, ZGUI::IRIT, gSpacer, mrTitleArea.bottom + gM);    // top right under title area

    int64_t nSVSide = nHSVSide - gM * 2;

    mrHArea = ZGUI::Arrange(ZRect(gM, nSVSide), rHSVArea, ZGUI::IRIT);                               // Right HUE rectangle
    mrSVArea = ZGUI::Arrange(ZRect(nSVSide, nSVSide), rHSVArea, ZGUI::ILIT);               // Left SV square

    mrColorValueBoxesArea.SetRect(rHSVArea.left, rHSVArea.bottom + gSpacer, rHSVArea.right, rFull.bottom);
    mrPaletteEntryArea.SetRect(rFull.left, rHSVArea.top, rHSVArea.left - gM, rHSVArea.bottom);


    if (mpColorMap)
    {
        mnPaletteEntryHeight = mrPaletteEntryArea.Height() / mpColorMap->size();
        limit<size_t>(mnPaletteEntryHeight, gM, gM * 4);
    }
    else
    {
        mnPaletteEntryHeight = gM * 2;
    }


    if (mpRGBEdit)
    {
        ZRect rEdit(mStyle.Font()->StringRect("WWWWWWWW"));   // enough space for ARGB hex without 0x
        rEdit.InflateRect(gSpacer*2, gSpacer);
        rEdit = ZGUI::Arrange(rEdit, mrSVArea, ZGUI::ILOB, 0, gM);
        mpRGBEdit->SetArea(rEdit);
        mpRGBEdit->mnCharacterLimit = 8;
        mpRGBEdit->msOnChangeMessage = ZMessage("rgb_edit", this);
    }

    ZWin::ComputeAreas();
}

bool ZWinPaletteDialog::HandleMessage(const ZMessage& message)
{
    if (message.GetType() == "rgb_edit")
    {
        string s = "0x" + msRGBTextValue;
        if (s.length() < 10)
            s = "0xFF" + msRGBTextValue;
        SetCurColor((uint32_t) SH::ToInt(s));
        UpdatePalette();
        return true;
    }

    return ZWinDialog::HandleMessage(message);
}


ZWinPaletteDialog* ZWinPaletteDialog::ShowPaletteDialog(std::string sCaption, ZGUI::tColorMap* pColorMap, std::string sOnOKMeessage, size_t nRecentColors)
{
    ZDEBUG_OUT_LOCKLESS("ShowPalette Called\n");

    assert(pColorMap);
    ZWinPaletteDialog* pDialog = (ZWinPaletteDialog*)gpMainWin->GetChildWindowByWinName(kPaletteDialogName);
       
    if (pDialog)
    {
        ZRect rMain(gpMainWin->GetArea());

        ZRect r(rMain.Width() / 3, (int64_t)(rMain.Width() / 4.5));
        r = ZGUI::Arrange(r, rMain, ZGUI::C);
        pDialog->SetArea(r);
        pDialog->SetVisible();
        return pDialog;
    }
        
    pDialog = new ZWinPaletteDialog();

    ZRect rMain(gpMainWin->GetArea());

    ZRect r(rMain.Width() / 3, (int64_t)(rMain.Width() / 4.5));
    r = ZGUI::Arrange(r, rMain, ZGUI::C);

    pDialog->mBehavior |= ZWinDialog::eBehavior::Draggable | ZWinDialog::eBehavior::OKButton | ZWinDialog::eBehavior::CancelButton;
    pDialog->mStyle = gDefaultDialogStyle;
    pDialog->mStyle.fp = gStyleButton.fp;
    pDialog->mStyle.paddingV = gSpacer;
    pDialog->mStyle.paddingH = gSpacer;

    pDialog->mpColorMap = pColorMap;
    pDialog->mOriginalColorMap = *pColorMap;

    pDialog->SetArea(r);
    pDialog->msOnOKMessage = sOnOKMeessage;
    pDialog->msCaption = sCaption;
    //pDialog->mTransformIn = ZWin::kSlideDown;

    gpMainWin->ChildAdd(pDialog);
    return pDialog;
}
