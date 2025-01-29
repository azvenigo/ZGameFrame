#pragma once

#include "WinTopWinners.h"
#include "Resources.h"
#include "ZMainWin.h"
#include "ZWinText.H"
#include "ZThumbCache.h"
#include "ZRandom.h"

using namespace std;

WinTopWinners::WinTopWinners()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = false;
    pMetaList = nullptr;
    mbDynamicThumbnailSizing = false;
    mStyle = gStyleGeneralText;
    mStyle.bgCol = 0xff000000;
    mStyle.fp.nScalePoints = 600;
    mStyle.fp.nWeight = 400;
    mStyle.pos = ZGUI::LB;
    mStyle.look.decoration = ZGUI::ZTextLook::kEmbossed;
    mStyle.look.colTop = 0xffff00ff;
    mStyle.look.colTop = 0xffffffff;
    mnTopNEntries = kTopNEntries;
}

bool WinTopWinners::Init()
{
    if (mbInitted)
        return true;

    UpdateUI();
    return ZWin::Init();
}

void WinTopWinners::UpdateUI()
{
    if (!pMetaList || pMetaList->empty())
        return;

    int64_t nThumbSide;
/*    if (pMetaList->size() < mnTopNEntries)
    {
        mThumbRects.resize(pMetaList->size());
        nThumbSide = mAreaToDrawTo.Height() / pMetaList->size();
    }
    else
    {*/
        mThumbRects.resize(mnTopNEntries);
        nThumbSide = mAreaLocal.Height() / mnTopNEntries;
//    }

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

    int64_t rank = 1;
    for (auto& entry : *pMetaList)
    {
        tZBufferPtr pThumb = entry.Thumbnail();

        ZRect rScaled = ZGUI::ScaledFit(pThumb->GetArea(), rThumb);
        rScaled = ZGUI::Arrange(rScaled, rThumb, ZGUI::C);

        if (fScaleFactor > 1.0)
        {
            int64_t nMouseVertDist = (int64_t)(abs((double)((double)rThumb.top + (double)rThumb.Height() / 2.0 - (double)gInput.lastMouseMove.y) / (double)rThumb.Height()));


            double fScale = 1 + fScaleFactor * exp(-(nMouseVertDist / 2.0) * (nMouseVertDist / 2.0));

            rScaled.right = rScaled.left + (int64_t)(rScaled.Width() * fScale);
            rScaled.bottom = rScaled.top + (int64_t)(rScaled.Height() * fScale);
        }

        if (rScaled.Height() <= nThumbSide)
            rThumb.OffsetRect(0, nThumbSide);
        else
            rThumb.OffsetRect(0, rScaled.Height());

        mThumbRects[rank-1] = rScaled;

        if (rank++ >= mnTopNEntries)
            break;
    }
}

bool WinTopWinners::OnMouseDownL(int64_t x, int64_t y)
{
    size_t i = 0;
    tImageMetaList::iterator it = pMetaList->begin();
    for (auto& r : mThumbRects)
    {
        if (i >= pMetaList->size())
            break;

        if (r.PtInRect(x, y))
        {
            ZRect rScreen(r);
            rScreen.OffsetRect(mAreaAbsolute.TL());

            // post
            string sLink("{select;filename=" + SH::URL_Encode((*it).filename) + ";target=" + mpParentWin->GetTargetName() + ";area=" + RectToString(rScreen) + "}");
            gMessageSystem.Post(sLink);
            ZDEBUG_OUT("Posted\n");
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

void WinTopWinners::SetArea(const ZRect& newArea)
{
    ZWin::SetArea(newArea);
    UpdateUI();
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
    if (!PrePaintCheck())
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());
 
    mpSurface->Fill(mStyle.bgCol);

    if (pMetaList && !pMetaList->empty())
    {
        if (pMetaList->size() != mThumbRects.size())
            UpdateUI(); // compute thumb rects

        int64_t rank = 1;
        for (auto& entry : *pMetaList)
        {
            tZBufferPtr pThumb = entry.Thumbnail();

            ZRasterizer rasterizer;
            tUVVertexArray verts;
            rasterizer.RectToVerts(mThumbRects[rank - 1], verts);

            rasterizer.Rasterize(mpSurface.get(), pThumb.get(), verts);

            string sRank;
            Sprintf(sRank, "#%d", rank);
            mStyle.Font()->DrawTextParagraph(mpSurface.get(), sRank, mThumbRects[rank-1], &mStyle);

            if (rank++ >= mnTopNEntries)
                break;
        }
    }


    return ZWin::Paint();
}
