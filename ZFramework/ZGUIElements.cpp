#include "ZGUIElements.h"
#include "helpers/StringHelpers.h"
#include "ZScreenBuffer.h"

using namespace std;

namespace ZGUI
{
    bool TextBox::Paint(ZBuffer* pDst)
    {
        const std::lock_guard<std::mutex> lock(clearMutex);

        if (sText.empty() || !visible || style.Uninitialized())
            return true;

        assert(pDst);
        // assuming pDst is locked
        ZRect rDraw(area);

        if (rDraw.Width() == 0 || rDraw.Height() == 0)
        {
            rDraw = pDst->GetArea();
        }

        if (style.pos == ZGUI::Fit)
            style.fp.nScalePoints = ZFontParams::ScalePoints(rDraw.Height()/2);

        assert(!sText.empty());
        assert(style.fp.nScalePoints > 0);
        rDraw = style.Font()->Arrange(rDraw, sText, style.pos, style.pad.h, style.pad.v);
        ZRect rLabel(rDraw.Width(), rDraw.Height());
        bool bDrawShadow = ARGB_A(shadow.col) > 0;

        if (bDrawShadow)
        {
            ZRect rBounds(shadow.Bounds(rDraw));
            rBounds.OffsetRect(shadow.offset);

            rDraw.UnionRect(rBounds);
        }

        if (IsInvalid())
        {
            renderedText = sText;
            renderedStyle = style;

            renderedBuf.Init(rDraw.Width(), rDraw.Height());
            renderedBuf.mbHasAlphaPixels = true;

            // Fill area if background style specifies
            if (ARGB_A(style.bgCol) > 0xf0)
            {
                renderedBuf.Fill(style.bgCol, &rLabel);
            }
            else if (ARGB_A(style.bgCol) > 0x0f)
            {
                if (blurBackground > 0.0)
                {
                    renderedBuf.CopyPixels(pDst, rDraw, renderedBuf.GetArea());
                    uint32_t* pStart = renderedBuf.mpPixels;
                    uint32_t* pEnd = pStart + renderedBuf.GetArea().Width() * renderedBuf.GetArea().Height();
                    while (pStart < pEnd)
                    {
                        *pStart = (*pStart | 0xff000000); // all alpha values to full opaque
                        pStart++;
                    }

                    renderedBuf.FillAlpha(style.bgCol, &rLabel);
                    renderedBuf.Blur(blurBackground, 1.0f, &rLabel);
                }
                else
                    renderedBuf.Fill(style.bgCol);
            }
            else
                renderedBuf.Fill(0);


            // Draw outline in padded area if style specifies
            if (ARGB_A(style.pad.col) > 0x00)
                renderedBuf.DrawRectAlpha(style.pad.col, rLabel);

            style.Font()->DrawTextParagraph(&renderedBuf, sText, rLabel, &style);

            if (bDrawShadow)
            {
                // render the shadow into internal
                shadow.Render(&renderedBuf, rLabel);
                shadow.Paint(&renderedBuf);


/*                // composite the 
                ZBuffer composite;
                composite.Init(rDraw.Width(), rDraw.Height());
                composite.Blt(&renderedBuf, rLabel, rLabel, 0, ZBuffer::kAlphaSource);

                renderedBuf.CopyPixels(&composite);*/
                style.Font()->DrawTextParagraph(&renderedBuf, sText, rLabel, &style);  // final text overlay after shadow compositing
            }
        }

        return pDst->Blt(&renderedBuf, renderedBuf.GetArea(), rDraw, nullptr, ZBuffer::kAlphaDest);
    }

    void TextBox::Paint(ZBuffer* pDst, tTextboxMap& textBoxMap)
    {
        for (auto& t : textBoxMap)
            t.second.Paint(pDst);
    }


    ToolTip::ToolTip()
    {
    }

    bool ToolTip::Paint(ZBuffer* pDst)
    {
        if (!mTextbox.visible || mTextbox.sText.empty())
            return true;

        if (mTextbox.area != renderedArea || mTextbox.sText != renderedText)
        {
            renderedArea = mTextbox.area;
            renderedText = mTextbox.sText;
            mTextbox.renderedText.clear();  // force re-rendering this way
        }

        mTextbox.Paint(pDst);

        // If the rendered image needs to be re-rendered, do so here
/*        if (mTextbox.area != renderedArea || mTextbox.sText != renderedText)
        {
            renderedImage.Init(mTextbox.area.Width(), mTextbox.area.Height());
            renderedImage.mbHasAlphaPixels = true;
            renderedImage.Blt(pDst, mTextbox.area, renderedImage.GetArea());    // cache
            mTextbox.Paint(&renderedImage);


            renderedArea = mTextbox.area;
            renderedText = mTextbox.sText;
            cout << "rendering tooltip x:" << mTextbox.area.left << " y:" << mTextbox.area.top << "\n";
            return true;
        }

        // just draw the 
        pDst->Blt(&renderedImage, renderedImage.GetArea(), renderedArea);*/

        return true;
    }




    Shadow::Shadow(uint32_t _col, float _spread, float _falloff)
    {
        col = _col;
        spread = _spread;
        falloff = _falloff;
    }

    ZRect Shadow::Bounds(ZRect r)
    {
        int64_t spreadPixels = (int64_t)spread * 2; 
        r.InflateRect(spreadPixels, spreadPixels);
        return r;
    }


    bool Shadow::Render(ZBuffer* pSrc, ZRect rSrc)
    {
        if (!pSrc)
            return false;

        ZRect rBounds(Bounds(rSrc));
        ZRect rRendered(rBounds.Width(), rBounds.Height());

        renderedShadow.reset(new ZBuffer());
        renderedShadow->Init(rRendered.Width(), rRendered.Height());
        renderedShadow->mbHasAlphaPixels = true;

        // blt the shadow source material
        ZRect rDst(ZGUI::Arrange(rSrc, rRendered, ZGUI::C));
//        rDst.OffsetRect(offset.x, offset.y);    
        renderedShadow->Blt(pSrc, rSrc, rDst, nullptr, ZBuffer::kAlphaSource);

/*        uint32_t* pStart = renderedShadow->mpPixels;
        uint32_t* pEnd = pStart + rDst.Area();
        while (pStart < pEnd)
        {
            if (*pStart != 0)
            {
                int stophere = 5;
            }


            *pStart = (*pStart & 0xff000000); // all colors to gray scale with alpha
            pStart++;
        }*/

        renderedShadow->Blur(spread, falloff);

        renderedShadow->DrawRectAlpha(0xff00ffff, renderedShadow->GetArea(), ZBuffer::kAlphaSource);

        renderedColor = 0;  // clear this for updating next paint

        return true;
    }

    bool Shadow::Paint(ZBuffer* pDst)
    {
        if (!pDst || !renderedShadow)
            return false;

        if (renderedColor != col)
        {
            renderedColor = col;

            uint32_t* pStart = renderedShadow->mpPixels;
            uint32_t* pEnd = pStart + renderedShadow->GetArea().Area();
            while (pStart < pEnd)
            {
                *pStart = (*pStart & 0xff000000) | (col &0x00ffffff);
                pStart++;
            }
        }

        ZRect rSrc(renderedShadow->GetArea());
        ZRect rDst(rSrc);
        rDst.OffsetRect(offset.x, offset.y);

        uint32_t shadowA = ARGB_A(col);

        pDst->BltAlpha(renderedShadow.get(), rSrc, rDst, shadowA, nullptr, ZBuffer::eAlphaBlendType::kAlphaBlend);
        return true;
    }




    bool ImageBox::Load()
    {
        ZDEBUG_OUT("ImageBox::Loading image:", imageFilename, "\n");

        tZBufferPtr buf(new ZBuffer());

        ZRect rDest(RenderRect());

        // for loading SVG, this sets the desired dimensions
        buf->Init(rDest.Width(), rDest.Height());
        if (!buf->LoadBuffer(imageFilename))
        {
            cerr << "Failed to load image:" << imageFilename << "\n";
            return false;
        }

        // non-svg images may have loaded at a different dimmension.....if so, scale
        if (buf->GetArea().Width() == rDest.Width() && buf->GetArea().Height() == rDest.Height())
        {
            mRendered = buf;
        }
        else
        {
            mRendered.reset(new ZBuffer());
            mRendered->Init(rDest.Width(), rDest.Height());
            mRendered->BltScaled(buf.get());
        }

        loadedFilename = imageFilename;
        return true;
    }

    ZRect ImageBox::RenderRect()
    {
        ZRect rDest(area);
        rDest.DeflateRect(style.pad.h, style.pad.v);
        return  ZGUI::Arrange(rDest, area, style.pos);
    }


    bool ImageBox::Paint(ZBuffer* pDest)
    {
        if (!visible)
            return true;

        ZRect rDest(RenderRect());

        if (mRendered == nullptr || rDest.Width() != mRendered->GetArea().Width() || rDest.Height() != mRendered->GetArea().Height())
        {
//            if (loadedFilename != imageFilename)
            if (!imageFilename.empty())
            {
                if (!Load())
                {
                    cerr << "Can't load then paint imagebox\n";
                    return false;
                }
            }
        }

        if (mRendered)
            pDest->Blt(mRendered.get(), mRendered->GetArea(), rDest);

        return true;
    }

    void ImageBox::Paint(ZBuffer* pDst, tSVGImageMap& svgImageBoxMap)
    {
        for (auto& i : svgImageBoxMap)
            i.second.Paint(pDst);
    }



    void ZTable::SetRowStyle(int32_t row, const ZGUI::Style& style)
    {
        if (row > mRows.size())
            return;

        auto&& rowIterator = mRows.begin();
        int32_t r = 0;
        for (r = 0; r < row; r++)
        {
            rowIterator++;
        }

        tCellArray& rowArray = *rowIterator;
        for (auto&& cell : rowArray)
        {
            cell.style = style;
        }
        mbAreaNeedsComputing = true;
    }

    void ZTable::SetColStyle(int32_t col, const ZGUI::Style& style)
    {
        for (auto& row : mRows)
        {
            if (col < row.size())
                row[col].style = style;
        }
        mbAreaNeedsComputing = true;
    }

    void ZTable::ComputeAreas(const ZRect& rTarget)
    {
        // Compute max width of each column
        mColumnWidths.clear();
        mColumnWidths.resize(mColumns);

        mRowHeights.clear();
        mRowHeights.resize(mRows.size());

        size_t nRow = 0;
        for (auto& row : mRows)
        {
            int32_t nCol = 0;
            int32_t nTallestCell = 0;    // track the tallest cell on each row
            for (auto& s : row)
            {
                int32_t nWidth = (int32_t)s.style.Font()->StringWidth(s.val);
                if (s.style.fp.Height() > nTallestCell)
                    nTallestCell = (int32_t)s.style.fp.Height();

                if (mColumnWidths[nCol] < nWidth)
                    mColumnWidths[nCol] = nWidth;
                nCol++;
            }

            mRowHeights[nRow] = nTallestCell;
            nRow++;
        }

        mrAreaToDrawTo.SetRect(0, 0, 0, 0);

        int32_t nTotalColumnWidths = mCellStyle.pad.h * 4;  // left and right margins
        for (auto& c : mColumnWidths)
            mrAreaToDrawTo.right += (c + mCellStyle.pad.h);

        int32_t nTotalRowHeights = mCellStyle.pad.v;    // top and bottom margins
        for (auto& r : mRowHeights)
            mrAreaToDrawTo.bottom += (r + mCellStyle.pad.v);

        mrAreaToDrawTo = ZGUI::Arrange(mrAreaToDrawTo, rTarget, mTableStyle.pos, mTableStyle.pad.h, mTableStyle.pad.v);

        mbAreaNeedsComputing = false;
    }

    bool ZTable::Paint(ZBuffer* pDest)
    {
        const std::lock_guard<std::recursive_mutex> lock(mTableMutex);

        if (mbAreaNeedsComputing)
            ComputeAreas(pDest->GetArea());

        if (mTableStyle.bgCol != 0)
            pDest->FillAlpha(mTableStyle.bgCol, &mrAreaToDrawTo);


        int64_t nY = mCellStyle.pad.v;

        // Draw top border

        // Now print each row based on column widths
        int64_t nRow = 0;
        for (auto& row : mRows)
        {
            // Draw left border

            size_t nRowHeight = mRowHeights[nRow];

            int32_t nX = mCellStyle.pad.h;
            for (int32_t nCol = 0; nCol < mColumns; nCol++)
            {
                int32_t nColWidth = mColumnWidths[nCol];
                ZCell& cell = row[nCol];
                ZRect rCellArea(0, 0, nColWidth, nRowHeight);
                rCellArea.OffsetRect(nX + mrAreaToDrawTo.left, nY + mrAreaToDrawTo.top);

                ZRect rString = cell.style.Font()->Arrange(rCellArea, cell.val, mCellStyle.pos, mCellStyle.pad.h, mCellStyle.pad.v);
                cell.style.Font()->DrawText(pDest, cell.val, rString, &mCellStyle.look);

                nX += nColWidth + mCellStyle.pad.h;
            }

            // Draw right border
            nY += nRowHeight + mCellStyle.pad.v;
            nRow++;
        }

        // bottom border

        return true;
    }

    void ZTable::AddMultilineRow(std::string& sMultiLine)
    {
        std::stringstream ss;
        ss << sMultiLine;
        std::string s;
        while (std::getline(ss, s, '\n'))
            AddRow(s);
    }


    ZCell* ZTable::ElementAt(int32_t row, int32_t col)
    {
        if (row > mRows.size() || col > mColumns)
            return nullptr;

        auto&& rowIterator = mRows.begin();
        int32_t r = 0;
        for (r = 0; r < row; r++)
        {
            rowIterator++;
        }

        tCellArray& rowArray = *rowIterator;
        if (col > rowArray.size())
            return nullptr;

        return &rowArray[col];
    }
};