#include "ZGUIElements.h"
#include "helpers/StringHelpers.h"
#include "ZScreenBuffer.h"

using namespace std;

namespace ZGUI
{
    bool TextBox::Paint(ZBuffer* pDst)
    {
        if (sText.empty()/* || !visible*/)
            return true;

        assert(pDst);
        // assuming pDst is locked
        ZRect rDraw(area);
        ZRect rFill(area);

        if (rDraw.Width() == 0 || rDraw.Height() == 0)
        {
            rDraw = pDst->GetArea();
            rFill = style.Font()->Arrange(rDraw, sText, style.pos);
        }

        if (style.pos == ZGUI::Fit)
            style.fp.nScalePoints = ZFontParams::ScalePoints(rDraw.Height()/2);

        // Fill area if background style specifies
        if (ARGB_A(style.bgCol) > 0xf0)
        {
            pDst->Fill(style.bgCol, &rFill);
        }
        else if (ARGB_A(style.bgCol) > 0x0f)
        {
//            pDst->Blt(gpGraphicSystem->GetScreenBuffer(), rFill, rFill);
            pDst->FillAlpha(style.bgCol, &rFill);

            if (blurBackground > 0.0)
                pDst->Blur(blurBackground, &rFill);
        }

        // Draw outline in padded area if style specifies
        if (ARGB_A(style.pad.col) > 0x00)
            pDst->DrawRectAlpha(style.pad.col, rFill);
            

        return style.Font()->DrawTextParagraph(pDst, sText, rDraw, &style);
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

        // If the rendered image needs to be re-rendered, do so here
        if (mTextbox.area != renderedArea || mTextbox.sText != renderedText)
        {
            mTextbox.Paint(pDst);
            renderedImage.Init(mTextbox.area.Width(), mTextbox.area.Height());
            renderedImage.Blt(pDst, mTextbox.area, renderedImage.GetArea());    // cache

            renderedArea = mTextbox.area;
            renderedText = mTextbox.sText;
            cout << "rendering tooltip x:" << mTextbox.area.left << " y:" << mTextbox.area.top << "\n";
            return true;
        }

        // just draw the 
        pDst->Blt(&renderedImage, renderedImage.GetArea(), renderedArea);

        return true;
    }







    bool SVGImageBox::Load(const std::string& sFilename)
    {
        const std::lock_guard<std::recursive_mutex> lock(mDocMutex);
        
        mSVGDoc = lunasvg::Document::loadFromFile(sFilename);

        if (!mSVGDoc)
        {
            cerr << "Failed to load SVG:" << sFilename << "\n";
            return false;
        }

        return true;
    }

    bool SVGImageBox::Paint(ZBuffer* pDest)
    {
        const std::lock_guard<std::recursive_mutex> lock(mDocMutex);

        if (mSVGDoc == nullptr /*|| !visible*/)
        {
            return true;
        }

        ZRect rDest(0,0, (int64_t)mSVGDoc->width(), (int64_t)mSVGDoc->height());
        rDest = ZGUI::ScaledFit(rDest, area);
        rDest.DeflateRect(style.pad.h, style.pad.v);
        rDest = ZGUI::Arrange(rDest, area, style.pos);

        if (mRendered == nullptr || rDest.Width() != mRendered->GetArea().Width() || rDest.Height() != mRendered->GetArea().Height())
        {
            mRendered.reset(new ZBuffer());
            auto svgbitmap = mSVGDoc->renderToBitmap((uint32_t)rDest.Width(), (uint32_t)rDest.Height());

            int64_t w = (int64_t)svgbitmap.width();
            int64_t h = (int64_t)svgbitmap.height();
            uint32_t s = svgbitmap.stride();
            //ZDEBUG_OUT("Rendering SVG at:", w, "x", h, "\n");

            mRendered->Init(w, h);
            uint32_t* pPixels = mRendered->GetPixels();
            for (int64_t y = 0; y < h; y++)
            {
                memcpy(pPixels + y * w, svgbitmap.data() + y * s, w * 4);
            }
            mRendered->mbHasAlphaPixels = true;
        }

        pDest->Blt(mRendered.get(), mRendered->GetArea(), rDest);

        return true;
    }

    void SVGImageBox::Paint(ZBuffer* pDst, tSVGImageMap& svgImageBoxMap)
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

                ZRect rString = cell.style.Font()->Arrange(rCellArea, cell.val, mCellStyle.pos, mCellStyle.pad.h);
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