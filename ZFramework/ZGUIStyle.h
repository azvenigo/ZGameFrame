#pragma once

#include "ZTypes.h"
#include "ZFont.h"
#include "ZGUIHelpers.h"


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


