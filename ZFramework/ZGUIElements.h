#pragma once

#include "ZTypes.h"
#include "ZFont.h"
#include "ZGUIHelpers.h"
#include "ZGUIStyle.h"
#include "lunasvg.h"
#include <map>
#include <mutex>

namespace ZGUI
{

    class Shadow
    {
    public:
        Shadow(uint32_t _col = 0xff000000, float _radius = 1.0f/*, float _falloff = 1.0f*/);

        bool Render(ZBuffer* pSrc, ZRect rCastSrc, bool bForceInvalid = false);
        bool Paint(ZBuffer* pDst, ZRect rCastSrc);
        bool IsInvalid() { return renderedColor != col || renderedRadius != radius /*|| renderedSigma != sigma;*/; }

        ZRect Bounds(ZRect r); // returns limits of shadow based on spread and falloff
        void Compute(ZBuffer* pSrc, ZRect rSrc, ZBuffer* pDst, ZPoint dstOffset, float radius, float falloff);



        uint32_t    col;
        float       radius;
//        float       sigma;

        ZPoint      offset;


    private:
        tZBufferPtr renderedShadow;
        uint32_t    renderedColor;
        float       renderedRadius;
//        float       renderedSigma;
    };



    typedef std::map<std::string, class TextBox> tTextboxMap; // named textboxes

    class TextBox
    {
    public:
        TextBox() { Clear(); }
        bool        Paint(ZBuffer* pDst);
        static void Paint(ZBuffer* pDst, tTextboxMap& textBoxMap);
        void        Clear()
        {
            const std::lock_guard<std::mutex> lock(clearMutex);

            sText.clear();
            style = {};
            area = {};
            visible = true;
            blurBackground = 0.0;

/*            dropShadowColor = 0x00000000;
            dropShadowSpread = 0.0f;
            dropShadowFalloff = 1.0f;*/
        }

        bool IsInvalid()
        {
            return renderedBuf.GetArea().Width() != area.Width() || renderedBuf.GetArea().Height() != area.Height() || renderedText != sText || renderedStyle != style || shadow.IsInvalid();
        };

        std::string sText;
        Style       style;
        ZRect       area;
        float       blurBackground;

//        ZPoint      dropShadowOffset;
//        uint32_t    dropShadowColor;    // if alpha of dropshadow is not 0, render a drop shadow
//        float       dropShadowSpread;
        //float       dropShadowFalloff;


        ZBuffer     renderedBuf;
        std::string renderedText;
        Style       renderedStyle;
        class Shadow      shadow;


        std::mutex  clearMutex;

        bool        visible;
    };


    class ToolTip 
    {
    public:
        ToolTip();
        bool    Paint(ZBuffer* pDst);

        TextBox mTextbox;

        // to determine if textbox needs to be re-drawn
        ZBuffer renderedImage;
        ZRect renderedArea; 
        std::string renderedText;
    };



    typedef std::map<std::string, class ImageBox> tSVGImageMap; // named SVGImageBoxes

    class ImageBox
    {
    public:
        ImageBox() { Clear(); }

        bool    Paint(ZBuffer* pDst);
        static void Paint(ZBuffer* pDst, tSVGImageMap& svgImageBoxMap);

        ZRect       RenderRect();
        void        Clear()
        {
            style = {};
            area = {};
            visible = true;
        }

        Style       style;
        ZRect       area;
        bool        visible;

        std::string imageFilename;  
        tZBufferPtr mRendered;

    private:
        bool    Load();

        std::string loadedFilename;
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
            if ((int32_t)columns.size() > mColumns)
            {
                mColumns = (int32_t)columns.size();
                for (auto& row : mRows)
                    row.resize(mColumns);
            }
            else if (columns.size() < mColumns)
                columns.resize(mColumns);

            mRows.push_back(columns);
            mbAreaNeedsComputing = true;
        }

        void AddMultilineRow(std::string& sMultiLine);


        ZCell* ElementAt(int32_t row, int32_t col);

        void SetRowStyle(int32_t row, const ZGUI::Style& style);
        void SetColStyle(int32_t col, const ZGUI::Style& style);



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
        int32_t mColumns;

        bool mbAreaNeedsComputing;      // if true, table has changed and area needs to be recomputed
        void ComputeAreas(const ZRect& rTarget);

        std::vector<int32_t> mColumnWidths;
        std::vector<int32_t> mRowHeights;
    };
};









