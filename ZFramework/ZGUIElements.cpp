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

        assert(!sText.empty());
        if (rDraw.Width() == 0 || rDraw.Height() == 0)
        {
            rDraw = pDst->GetArea();
        }

        assert(!sText.empty());
        if (style.pos == ZGUI::Fit)
            style.fp.nScalePoints = ZFontParams::ScalePoints(rDraw.Height()/2);

        assert(!sText.empty());
        assert(style.fp.nScalePoints > 0);
        rDraw = style.Font()->Arrange(rDraw, sText, style.pos, style.pad.h, style.pad.v);
        ZRect rLabel(rDraw.Width(), rDraw.Height());

        assert(!sText.empty());

        bool bDrawDropShadow = ARGB_A(dropShadowColor) > 0;
        if (bDrawDropShadow)
        {
            if (dropShadowOffset.x > 0)
            {
                rDraw.right += (dropShadowOffset.x + (int64_t)dropShadowBlur*2);
            }
            else
            {
                int64_t offset = (dropShadowOffset.x - (int64_t)dropShadowBlur*2);
                rLabel.OffsetRect(-offset, 0);
                rDraw.left += offset;
            }

            if (dropShadowOffset.y > 0)
            {
                rDraw.bottom += (dropShadowOffset.y + (int64_t)dropShadowBlur*2);
            }
            else
            {
                int64_t offset = (dropShadowOffset.y - (int64_t)dropShadowBlur*2);
                rLabel.OffsetRect(0, -offset);
                rDraw.top += offset;
                
            }
        }

        assert(!sText.empty());

        if (renderedBuf.GetArea().Width() != rDraw.Width() || renderedBuf.GetArea().Height() != rDraw.Height() || renderedText != sText || renderedStyle != style)
        {
//            cout << "Rendering textbox\n";
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
                    renderedBuf.Blur(blurBackground, &rLabel);
                }
                else
                    renderedBuf.Fill(style.bgCol);
            }
            else
                renderedBuf.Fill(0);

            assert(!sText.empty());

            // Draw outline in padded area if style specifies
            if (ARGB_A(style.pad.col) > 0x00)
                renderedBuf.DrawRectAlpha(style.pad.col, rLabel);

            assert(!sText.empty());

            style.Font()->DrawTextParagraph(&renderedBuf, sText, rLabel, &style);

            if (bDrawDropShadow)
            {
                ZBuffer shadowTemp;
                shadowTemp.Init(rDraw.Width(), rDraw.Height());

                ZRect rShadow(rLabel);
                rShadow.OffsetRect(dropShadowOffset);
                shadowTemp.Blt(&renderedBuf, rLabel, rShadow, 0, ZBuffer::kAlphaSource);

                uint32_t* pStart = shadowTemp.mpPixels;
                uint32_t* pEnd = pStart + shadowTemp.GetArea().Width() * shadowTemp.GetArea().Height();
                uint32_t shadowAlphaMask = (0xff000000 & dropShadowColor);
                while (pStart < pEnd)
                {
                    *pStart = (*pStart & shadowAlphaMask); // set each pixel color to 0 but leave alpha
                    pStart++;
                }



                if (dropShadowBlur > 0.0)
                    shadowTemp.Blur(dropShadowBlur);
                shadowTemp.Blt(&renderedBuf, rLabel, rLabel, 0, ZBuffer::kAlphaBlend);

                renderedBuf.CopyPixels(&shadowTemp);
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
            if (loadedFilename != imageFilename)
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