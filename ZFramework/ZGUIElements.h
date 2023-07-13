#pragma once

#include "ZTypes.h"
#include "ZFont.h"
#include "ZGUIHelpers.h"
#include "ZGUIStyle.h"
#include <map>

namespace ZGUI
{
    typedef std::map<std::string, class TextBox> tTextboxMap; // named textbox to descriptor

    class TextBox
    {
    public:
        void        Paint(ZBuffer* pDst);
        static void Paint(ZBuffer* pDst, tTextboxMap& textBoxMap);
        void        Clear()
        {
            sText.clear();
            style = {};
            area = {};
            visible = true;
        }

        std::string sText;
        Style       style;
        ZRect       area;
        bool        visible;
    };


    class ZCell
    {
    public:
        ZCell(std::string _val = "", ZGUI::Style* _style = nullptr)
        {
            val = _val;
            if (_style)
                style = *_style;
            else
                style = gDefaultDialogStyle;
        };

        std::string val;
        ZGUI::Style style;
    };

    typedef std::vector<ZCell> tCellArray;

    class ZTable
    {
    public:
        ZTable() : mTableStyle(gDefaultDialogStyle), mCellStyle(gDefaultDialogStyle), mColumns(0), mbAreaNeedsComputing(true) {}

        // Table manimpulation
        void Clear() 
        { 
            mRows.clear();
            mbAreaNeedsComputing = true;
        }

        template <typename T, typename...Types>
        inline void AddRow(T arg, Types...more)
        {
            tCellArray columns;
            ToCellList(columns, arg, more...);

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
            mbAreaNeedsComputing = true;
        }

        void AddMultilineRow(std::string& sMultiLine);


        ZCell* ElementAt(size_t row, size_t col);

        void SetRowStyle(size_t row, const ZGUI::Style& style);
        void SetColStyle(size_t col, const ZGUI::Style& style);



        // Output
        size_t GetRowCount() const { return mRows.size(); }

        template <typename S, typename...SMore>
        inline void ToCellList(tCellArray& columns, S arg, SMore...moreargs)
        {
            std::stringstream ss;
            ss << arg;
            columns.push_back( ZCell(ss.str(), &mCellStyle));
            return ToCellList(columns, moreargs...);
        }

        inline void ToCellList(tCellArray&) {}   // needed for the variadic with no args

        std::recursive_mutex mTableMutex;

        // Rendering
        bool Paint(ZBuffer* pDest);
        ZGUI::Style mTableStyle;
        ZGUI::Style mCellStyle;
        ZRect       mrAreaToDrawTo;

    private:
        std::list<tCellArray> mRows;
        size_t mColumns;

        bool mbAreaNeedsComputing;      // if true, table has changed and area needs to be recomputed
        void ComputeAreas(const ZRect& rTarget);

        std::vector<size_t> mColumnWidths;
        std::vector<size_t> mRowHeights;
    };

};









