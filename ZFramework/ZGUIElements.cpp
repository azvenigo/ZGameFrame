#include "ZGUIElements.h"
#include "helpers/StringHelpers.h"

using namespace std;

namespace ZGUI
{
    void TextBox::Paint(ZBuffer* pDst)
    {
        if (sText.empty())
            return;

        assert(pDst);
        // assuming pDst is locked
        ZRect rDraw(area);
        if (rDraw.Width() == 0 || rDraw.Height() == 0)
            rDraw = pDst->GetArea();

        style.Font()->DrawTextParagraph(pDst, sText, rDraw, &style);
    }

    void TextBox::Paint(ZBuffer* pDst, tTextboxMap& textBoxMap)
    {
        for (auto& t : textBoxMap)
            t.second.Paint(pDst);
    }




    void ZTable::SetRowStyle(size_t row, const ZGUI::Style& style)
    {
        if (row > mRows.size())
            return;

        auto& rowIterator = mRows.begin();
        size_t r = 0;
        for (r = 0; r < row; r++)
        {
            rowIterator++;
        }

        tCellArray& rowArray = *rowIterator;
        for (auto& cell : rowArray)
        {
            cell.style = style;
        }
    }

    void ZTable::SetColStyle(size_t col, const ZGUI::Style& style)
    {
        for (auto& row : mRows)
        {
            if (col < row.size())
                row[col].style = style;
        }
    }

    bool ZTable::Paint(ZBuffer* pDest)
    {
        const std::lock_guard<std::recursive_mutex> lock(mTableMutex);

        // Compute max width of each column
        std::vector<size_t> columnWidths;
        columnWidths.resize(mColumns);


        std::vector<size_t> rowHeights;
        rowHeights.resize(mRows.size());

        size_t nRow = 0;
        for (auto& row : mRows)
        {
            size_t nCol = 0;
            size_t nTallestCell = 0;    // track the tallest cell on each row
            for (auto& s : row)
            {
                int64_t nWidth = s.style.Font()->StringWidth(s.val);
                if (s.style.fp.nHeight > nTallestCell)
                    nTallestCell = s.style.fp.nHeight;

                if (columnWidths[nCol] < nWidth)
                    columnWidths[nCol] = nWidth;
                nCol++;
            }

            rowHeights[nRow] = nTallestCell;
            nRow++;
        }

        size_t nTotalColumnWidths = mCellStyle.paddingH*4;  // left and right margins
        for (auto& c : columnWidths)
            nTotalColumnWidths += (c + mCellStyle.paddingH);

        size_t nTotalRowHeights = mCellStyle.paddingV;    // top and bottom margins
        for (auto& r : rowHeights)
            nTotalRowHeights += (r + mCellStyle.paddingV);

        ZRect rAreaToDrawTo(0, 0, nTotalColumnWidths, nTotalRowHeights);

        rAreaToDrawTo = ZGUI::Arrange(rAreaToDrawTo, pDest->GetArea(), mTableStyle.pos, mTableStyle.paddingH, mTableStyle.paddingV);

        if (mTableStyle.bgCol != 0)
        {
            pDest->FillAlpha(rAreaToDrawTo, mTableStyle.bgCol);
        }



        int64_t nY = mCellStyle.paddingV;

        // Draw top border

        // Now print each row based on column widths
        nRow = 0;
        for (auto& row : mRows)
        {
            // Draw left border

            size_t nRowHeight = rowHeights[nRow];

            int64_t nX = mCellStyle.paddingH;
            for (size_t nCol = 0; nCol < mColumns; nCol++)
            {
                size_t nColWidth = columnWidths[nCol];
                ZCell& cell = row[nCol];
                ZRect rCellArea(0, 0, nColWidth, nRowHeight);
                rCellArea.OffsetRect(nX + rAreaToDrawTo.left, nY + rAreaToDrawTo.top);

                ZRect rString = cell.style.Font()->GetOutputRect(rCellArea, (uint8_t*)cell.val.data(), cell.val.length(), mCellStyle.pos, mCellStyle.paddingH);
                //ZRect rString = ZGUI::Arrange(rString, rAreaToDrawTo, cell.style.pos, cell.style.paddingH, cell.style.paddingV);

                cell.style.Font()->DrawText(pDest, cell.val, rString, &mCellStyle.look);

                nX += nColWidth + mCellStyle.paddingH;
            }

            // Draw right border
            nY += nRowHeight + mCellStyle.paddingV;
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


    ZCell* ZTable::ElementAt(size_t row, size_t col)
    {
        if (row > mRows.size() || col > mColumns)
            return nullptr;

        auto& rowIterator = mRows.begin();
        size_t r = 0;
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