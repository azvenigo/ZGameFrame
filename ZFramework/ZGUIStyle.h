#pragma once

#include "ZTypes.h"
#include "ZFont.h"
#include "ZGUIHelpers.h"
#include "json.hpp"
#include <map>



//////////////////////////////////////////////////////////////////
// Default styles to be instantiated by app

extern ZRect		grFullArea;

extern int64_t      gM; // default measure defined by window diagonal ( diagonal / 100 )... all scaled GUI elements should be based off of this measure
extern int64_t      gSpacer;

extern uint32_t     gDefaultDialogFill;
extern uint32_t     gDefaultTextAreaFill;
extern uint32_t     gDefaultHighlight;
//extern int64//_t      gnDefaultGroupInlaySize;



extern ZFontParams gDefaultButtonFont;
extern ZFontParams gDefaultTitleFont;
extern ZFontParams gDefaultTextFont;

extern ZGUI::Style gStyleTooltip;
extern ZGUI::Style gStyleCaption;
extern ZGUI::Style gStyleButton;
extern ZGUI::Style gStyleSlider;
extern ZGUI::Style gStyleToggleChecked;
extern ZGUI::Style gStyleToggleUnchecked;
extern ZGUI::Style gStyleGeneralText;
extern ZGUI::Style gDefaultDialogStyle;
extern ZGUI::Style gDefaultWinTextEditStyle;
extern ZGUI::Style gDefaultGroupingStyle;
extern ZGUI::Style gDefaultFormattedDocStyle;
extern ZGUI::Style gDefaultPanelStyle;
extern ZGUI::RA_Descriptor gDefaultRAD;

namespace ZGUI
{
    void ComputeLooks();    // To be called by application whenever creating or changing screen size to calculate proper sizing


    class ZTextLook
    {
    public:
        enum eDeco : uint8_t
        {
            kNormal = 0,
            kShadowed = 1,
            kEmbossed = 2
        };

        ZTextLook(eDeco _decoration = kNormal, uint32_t _colTop = 0xffffffff, uint32_t _colBottom = 0xffffffff);
        ZTextLook(const std::string& s);

        void SetCol(uint32_t col)
        {
            colTop = col;
            colBottom = col;
        }

        void SetCol(uint32_t top, uint32_t bottom)
        {
            colTop = top;
            colBottom = bottom;
        }


        operator std::string() const;
        bool operator == (const ZTextLook& rhs) const
        {
            return (colTop == rhs.colTop && colBottom == rhs.colBottom && decoration == rhs.decoration);
        }

        uint32_t    colTop;
        uint32_t    colBottom;
        eDeco       decoration;
    };

    class Padding
    {
    public:
        Padding(int32_t _h = 0, int32_t _v = 0, uint32_t _col = 0) : h(_h), v(_v), col(_col) {}
        Padding(const std::string& s);

        operator std::string() const;
        void operator = (const Padding& rhs);
        bool operator == (const Padding& rhs) const
        {
            return (h == rhs.h && v == rhs.v && col == rhs.col);
        }

        int32_t h;
        int32_t v;
        int32_t col;
    };


    class Style
    {
    public:
        Style(const ZFontParams& _fp = {}, const ZTextLook& _look = {}, ePosition _pos = ePosition::Unknown, Padding _pad = {}, uint32_t _bgCol = 0, bool _wrap = false);
        Style(const std::string& s);

        tZFontPtr Font();
        bool Uninitialized() { return fp.sFacename.empty() || fp.nScalePoints == 0; }

        operator std::string() const;
        void operator = (const Style& rhs);
        bool operator == (const Style& rhs) const
        {
            return (
                fp == rhs.fp &&
                look == rhs.look &&
                pos == rhs.pos &&
                pad == rhs.pad &&
                bgCol == rhs.bgCol &&
                wrap == rhs.wrap);
        }

        ZFontParams fp;
        ZTextLook   look;
        ePosition   pos;
        Padding     pad;
        uint32_t    bgCol;
        bool        wrap;

    protected:
        tZFontPtr   mpFont;
    };


    class EditableColor
    {
    public:
        EditableColor(const std::string& _name, uint32_t _default_color) : name(_name), col(_default_color), default_color(_default_color) {}
        EditableColor(const std::string& _name, uint32_t _col, uint32_t _default_color) : name(_name), col(_col), default_color(_default_color) {}
        EditableColor(const std::string& _encoded_string);
        EditableColor() : col(0), default_color(0) {}

        bool IsDefault() { return col == default_color; }
        void ResetToDefault() { col = default_color; }

        uint32_t    col;    // currently set color
        std::string name;       // for

        operator std::string() const;
        uint32_t default_color;
    };

    typedef std::vector<EditableColor> tColorMap;

    class Palette
    {
    public:
        Palette() {}
        Palette(const tColorMap& _map);
        Palette(const std::string& _encoded_string);


        bool GetByName(const std::string& name, uint32_t& col);
        uint32_t Get(const std::string& name); 


        operator std::string() const;

        // for json serialization
        void to_json(nlohmann::json& j, const Palette& pal)
        {
            j["count"] = pal.mColorMap.size();
            for (int i = 0; i < pal.mColorMap.size(); i++)
            {
                j[i] = (std::string)pal.mColorMap[i];
            }
        }

        void from_json(nlohmann::json& j, Palette& pal)
        {
            mColorMap.clear();
            int count = j["count"];

            mColorMap.resize(count);

            for (int i = 0; i < count; i++)
            {
                mColorMap[i] = EditableColor(j[i]);
            }
        }


        tColorMap mColorMap;
    };
};