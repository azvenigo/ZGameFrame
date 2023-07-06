#include "ZWinTable.h"
#include "ZBuffer.h"

ZWinTable::ZWinTable() : mTableStyle(gDefaultDialogStyle), mCellStyle(gDefaultDialogStyle), mColumns(0)
{
}

bool ZWinTable::Paint(ZBuffer* pDest)
{
    // Compute max width of each column
    std::vector<size_t> columnWidths;
    columnWidths.resize(mColumns);

    tZFontPtr pFont = mCellStyle.Font();
    ZRect rAreaToDrawTo = ZGUI::Arrange(rArea, pDest->GetArea(), mTableStyle.pos, mTableStyle.paddingH, mTableStyle.paddingV);


    for (auto row : mRows)
    {
        size_t nCol = 0;
        for (auto s : row)
        {
            int64_t nWidth = pFont->StringWidth(s);

            if (columnWidths[nCol] < nWidth)
                columnWidths[nCol] = nWidth;
            nCol++;
        }
    }

    size_t nTotalColumnWidths = 0;
    for (auto c : columnWidths)
        nTotalColumnWidths += (c + mCellStyle.paddingH);

    int64_t nY = 0;

    // Draw top border

    // Now print each row based on column widths
    for (auto row : mRows)
    {
        // Draw left border

        int64_t nX = 0;
        for (size_t nCol = 0; nCol < mColumns; nCol++)
        {
            size_t nColWidth = columnWidths[nCol];
            std::string s(row[nCol]);

            ZRect rString = pFont->GetOutputRect(rAreaToDrawTo, (uint8_t*)s.data(), s.length(), mCellStyle.pos, mCellStyle.paddingH);
            rString.OffsetRect(nX, nY);

            pFont->DrawText(pDest, s, rString, &mCellStyle.look);

            nX += nColWidth + mCellStyle.paddingH;
        }

        // Draw right border
        nY += mCellStyle.fp.nHeight + mCellStyle.paddingV;
    }

    // bottom border

    return true;
}


