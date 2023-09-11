#pragma once

#include "WinTopWinners.h"
#include "Resources.h"
#include "ZMainWin.h"
#include "ZWinText.H"
#include "ZThumbCache.h"
#include "ZRandom.h"

using namespace std;

const string kTopWinnersDialogName("TopWinnersDialog");

WinTopWinners::WinTopWinners()
{
    msWinName = kTopWinnersDialogName;
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = false;
    pMetaList = nullptr;
    mbDynamicThumbnailSizing = false;
}

bool WinTopWinners::Init()
{
    UpdateUI();
    return ZWin::Init();
}

void WinTopWinners::UpdateUI()
{
    if (!pMetaList)
        return;
    mThumbRects.resize(pMetaList->size());

    int64_t nThumbSide = mAreaToDrawTo.Height() / pMetaList->size();
    ZRect rThumb(nThumbSide, nThumbSide);
    ZRect rThumbStrip(0, 0, nThumbSide, nThumbSide * pMetaList->size());


    const double fScaleMax = 6.0f;

    double fScaleFactor = 1.0;
    if (mbDynamicThumbnailSizing)
    {
        int64_t nWidth = mAreaAbsolute.Width() / 2;
        int64_t nCenterX = (mAreaAbsolute.left + mAreaAbsolute.right) / 2;
        if (gInput.lastMouseMove.x > nCenterX)
            fScaleFactor = fScaleMax;
        else
            fScaleFactor = fScaleMax * (double)(nWidth - (nCenterX - gInput.lastMouseMove.x)) / (nWidth / 2.0);

        ZOUT("fScaleFactor:", fScaleFactor, "\n");

        int64_t nMouseOverThumb = gInput.lastMouseMove.y / nThumbSide;

        limit<int64_t>(nMouseOverThumb, 0, pMetaList->size() - 1);
    }

    int64_t i = 0;
    for (auto& entry : *pMetaList)
    {
        tZBufferPtr pThumb = entry.Thumbnail();

        ZRect rScaled = ZGUI::ScaledFit(pThumb->GetArea(), rThumb);
        rScaled = ZGUI::Arrange(rScaled, rThumb, ZGUI::C);

        if (fScaleFactor > 1.0)
        {
            int64_t nMouseVertDist = abs((double)((double)rThumb.top + (double)rThumb.Height() / 2.0 - (double)gInput.lastMouseMove.y) / (double)rThumb.Height());


            double fScale = 1 + fScaleFactor * exp(-(nMouseVertDist / 2.0) * (nMouseVertDist / 2.0));

            rScaled.right = rScaled.left + rScaled.Width() * fScale;
            rScaled.bottom = rScaled.top + rScaled.Height() * fScale;
        }

        if (rScaled.Height() <= nThumbSide)
            rThumb.OffsetRect(0, nThumbSide);
        else
            rThumb.OffsetRect(0, rScaled.Height());

        mThumbRects[i] = rScaled;

        i++;
    }
}

bool WinTopWinners::OnMouseDownL(int64_t x, int64_t y)
{
    int64_t i = 0;
    tImageMetaList::iterator it = pMetaList->begin();
    for (auto r : mThumbRects)
    {
        if (r.PtInRect(x, y))
        {
            // post
            string sLink("select;filename=" + SH::URL_Encode((*it).filename) + ";target=ZWinImageContest");
            gMessageSystem.Post(sLink);
            break;
        }

        i++;
        it++;
    }

    return ZWin::OnMouseDownL(x, y);
}

bool WinTopWinners::OnMouseMove(int64_t x, int64_t y)
{
    if (mbDynamicThumbnailSizing)
    {
        UpdateUI();
        Invalidate();
    }
    return ZWin::OnMouseMove(x, y);
}

bool WinTopWinners::OnMouseOut()
{
    if (mbDynamicThumbnailSizing)
    {
        UpdateUI();
        Invalidate();
    }
    return ZWin::OnMouseOut();
}


bool WinTopWinners::OnMouseWheel(int64_t x, int64_t y, int64_t nDelta)
{
    return ZWin::OnMouseWheel(x, y, nDelta);
}


bool WinTopWinners::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();

    if (sType == "dlg_ok")
    {
    }

    return ZWin::HandleMessage(message);
}


bool WinTopWinners::Paint()
{
    if (!mpTransformTexture)
        return false;
   
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return false;

    mpTransformTexture->Fill(mStyle.bgCol);

    if (pMetaList && !pMetaList->empty())
    {
        if (pMetaList->size() != mThumbRects.size())
            UpdateUI(); // compute thumb rects

        int64_t i = 0;
        for (auto& entry : *pMetaList)
        {
            tZBufferPtr pThumb = entry.Thumbnail();

            ZRasterizer rasterizer;
            tUVVertexArray verts;
            rasterizer.RectToVerts(mThumbRects[i], verts);

            rasterizer.Rasterize(mpTransformTexture.get(), pThumb.get(), verts);

            i++;
        }
    }


    return ZWin::Paint();
}

/*
TopWinnersDialog* TopWinnersDialog::ShowDialog(tImageMetaList sortedList, ZRect rDialogArea)
{
    ZWin* pContestWin = (ZWin*)gpMainWin->GetChildWindowByWinName("ZWinImageContest");
    if (!pContestWin)
    {
        ZERROR("Something terribly wrong. Couldn't retrieve contest dialog");
        return nullptr;
    }

    // only one dialog
    TopWinnersDialog* pDialog = (TopWinnersDialog*)pContestWin->GetChildWindowByWinName(kTopWinnersDialogName);
    if (pDialog)
    {
        pDialog->SetVisible();
        return pDialog;
    }

    pDialog = new TopWinnersDialog();

    pDialog->mStyle = gDefaultDialogStyle;
    pDialog->SetArea(rDialogArea);
    pDialog->sortedList = sortedList;

    pContestWin->ChildAdd(pDialog);
    return pDialog;
}*/
