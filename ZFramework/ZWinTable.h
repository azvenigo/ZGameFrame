#pragma once

#include "ZFont.h"
#include "ZGUIHelpers.h"
#include "ZGUIStyle.h"
#include <vector>

typedef std::vector<std::string> tStringArray;

class ZWinTable 
{
public:
    ZWinTable();

    bool        Paint(ZBuffer* pDest);


    // Table manimpulation
    void Clear() { mRows.clear(); }

    template <typename T, typename...Types>
    inline void AddRow(T arg, Types...more)
    {
        tStringArray columns;
        ToStringList(columns, arg, more...);

        // Check if the row being added has more columns than any row before.
        // If so, resize all existing rows to match
        if (columns.size() > mColumns)
        {
            mColumns = columns.size();
            for (auto& row : mRows)
                row.resize(mColumns);
        }
        else if (columns.size() < mColumns)
            columns.resize(mColumns);

        mRows.push_back(columns);
    }

    void AddMultilineRow(std::string& sMultiLine)
    {
        std::stringstream ss;
        ss << sMultiLine;
        std::string s;
        while (std::getline(ss, s, '\n'))
            AddRow(s);
    }

    std::string ElementAt(size_t row, size_t col)
    {
        if (row > mRows.size() || col > mColumns)
            return "";

        auto rowIterator = mRows.begin();
        size_t r = 0;
        for (r = 0; r < row; r++)
        {
            rowIterator++;
        }

        tStringArray& rowArray = *rowIterator;
        if (col > rowArray.size())
            return "";

        return rowArray[col];
    }

    // Output
    size_t GetRowCount() const
    {
        return mRows.size();
    }

    template <typename S, typename...SMore>
    inline void ToStringList(tStringArray& columns, S arg, SMore...moreargs)
    {
        std::stringstream ss;
        ss << arg;
        columns.push_back(ss.str());
        return ToStringList(columns, moreargs...);
    }

    inline void ToStringList(tStringArray&) {}   // needed for the variadic with no args





    ZGUI::Style mTableStyle;
    ZGUI::Style mCellStyle;
    ZRect rArea;
private:

    std::list<tStringArray> mRows;
    size_t mColumns;
};


