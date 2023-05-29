#pragma once

#include "ZTypes.h"
#include "ZFont.h"
#include "ZGUIHelpers.h"
#include "json.hpp"
#include <map>


namespace ZGUI
{
    class Style
    {
    public:
        Style(const ZFontParams& _fp = {}, const ZTextLook& _look = {}, ePosition _pos = ePosition::Unknown, int32_t _padding = 0, uint32_t _bgCol = 0, bool _wrap = false);
        Style::Style(const std::string& s);

        tZFontPtr Font();

        operator std::string() const;

        ZFontParams fp;
        ZTextLook   look;
        ePosition   pos;
        int32_t     padding;
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

    private:
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


//////////////////////////////////////////////////////////////////
// Default styles to be instantiated by app
extern ZFontParams  gDefaultButtonFont;
extern ZFontParams  gDefaultTitleFont;
extern ZFontParams  gDefaultTextFont;

extern ZFontParams  gDefaultTooltipFont;
extern ZTextLook    gDefaultToolitipLook;
extern uint32_t     gDefaultTooltipFill;

extern ZGUI::Style  gStyleTooltip;
extern ZGUI::Style  gStyleCaption;
extern ZGUI::Style  gStyleButton;
extern ZGUI::Style  gStyleToggleChecked;
extern ZGUI::Style  gStyleToggleUnchecked;
extern ZGUI::Style  gStyleGeneralText;
extern ZGUI::Style  gDefaultDialogStyle;
extern ZGUI::Style  gDefaultWinTextEditStyle;
extern ZGUI::Style  gDefaultGroupingStyle;


