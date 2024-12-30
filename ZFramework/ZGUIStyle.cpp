#include "ZGUIStyle.h"
#include "helpers/StringHelpers.h"

using namespace std;

uint32_t        gDefaultText(0xff575757);
uint32_t        gDefaultDialogFill(0xff575757);
uint32_t        gDefaultTextAreaFill(0xff888888);
uint32_t        gDefaultTooltipFill(0xaa000088);
uint32_t        gDefaultHighlight(0xff008800);
uint32_t        gDefaultButtonUnchecked(0x00000000);

int64_t         gM; // default measure defined by window diagonal ( diagonal / 100 )... all scaled GUI elements should be based off of this measure
int64_t         gSpacer;

//Fonts
ZFontParams     gDefaultTitleFont("Gadugi", 1500);
ZFontParams     gDefaultTextFont("Gadugi", 500);

ZGUI::Style gStyleTooltip(ZFontParams("Verdana", 1000), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xffffffff, 0xffffffff), ZGUI::C, ZGUI::Padding(8, 8, 0xff888888), gDefaultTooltipFill);
ZGUI::Style gStyleCaption(ZFontParams("Gadugi", 1200, 400), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xffffffff, 0xffffffff), ZGUI::C, ZGUI::Padding(), 0x00000000);
ZGUI::Style gStyleButton(ZFontParams("Verdana", 1200, 600), ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xffffffff, 0xffffffff), ZGUI::C, ZGUI::Padding(), gDefaultDialogFill);
ZGUI::Style gStyleSlider(ZFontParams("Verdana", 600, 400), ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xffffffff, 0xffffffff), ZGUI::C, ZGUI::Padding(), gDefaultDialogFill);
ZGUI::Style gStyleToggleChecked(ZFontParams("Verdana", 800, 600), ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xff00ff00, 0xff008800), ZGUI::C, ZGUI::Padding(), 0x00000000);
ZGUI::Style gStyleToggleUnchecked(ZFontParams("Verdana", 800, 600), ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xffffffff, 0xff888888), ZGUI::C, ZGUI::Padding(), 0x00000000);
ZGUI::Style gStyleGeneralText(ZFontParams("Verdana", 1200), ZGUI::ZTextLook(ZGUI::ZTextLook::kNormal, 0xffffffff, 0xffffffff), ZGUI::LT, ZGUI::Padding(), 0x00000000, true);
ZGUI::Style gDefaultDialogStyle(gDefaultTextFont, ZGUI::ZTextLook(), ZGUI::LT, ZGUI::Padding((int32_t)gSpacer, (int32_t)gSpacer), gDefaultDialogFill, true);
ZGUI::Style gDefaultWinTextEditStyle(gDefaultTextFont, ZGUI::ZTextLook(), ZGUI::LT, ZGUI::Padding((int32_t)gSpacer, (int32_t)gSpacer), gDefaultTextAreaFill);
ZGUI::Style gDefaultGroupingStyle(ZFontParams("Ariel Greek", 500, 200, 2), ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xffffffff, 0xffffffff), ZGUI::LT, ZGUI::Padding(8, 8));
ZGUI::Style gDefaultFormattedDocStyle(gDefaultTitleFont, ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xff000000, 0xff000000), ZGUI::Unknown, ZGUI::Padding((int32_t)gSpacer, (int32_t)gSpacer), 0, false);
ZGUI::Style gDefaultPanelStyle(gDefaultTitleFont, ZGUI::ZTextLook(ZGUI::ZTextLook::kNormal, 0xff000000, 0xff000000), ZGUI::Unknown, ZGUI::Padding((int32_t)gSpacer, (int32_t)gSpacer));

ZGUI::RA_Descriptor gDefaultRAD({}, "full", ZGUI::C, 0.25, 0.25);   

ZGUI::Palette gAppPalette{
{
    { "Dialog Fill", gDefaultDialogFill },              // 0
    { "Text Fill", gDefaultTextAreaFill },              // 1
    { "Highlight",  gDefaultHighlight},                 // 2
    { "Toggle Unchecked",  gDefaultButtonUnchecked},    // 3
    { "Tooltip Fill", gDefaultTooltipFill },            // 4

} };


namespace ZGUI
{
    void ComputeLooks()
    {
        ZOUT_LOCKLESS("ComputeLooks()\n");
//        gM = (int64_t) (sqrt(grFullArea.Width() * grFullArea.Width() + grFullArea.Height() * grFullArea.Height()) / 100.0);  // default measure
        gM = (int64_t)grFullArea.Height() / 64;
        gSpacer = gM / 5;
        gSpacer = std::max<int64_t>(gSpacer, 4);

        gStyleButton.pad.h = (int32_t)gSpacer;
        gStyleButton.pad.v = (int32_t)gSpacer;

        gStyleTooltip.pad.h = (int32_t)gSpacer;
        gStyleTooltip.pad.v = (int32_t)gSpacer;


        gDefaultGroupingStyle.pad.h = (int32_t)std::max<int64_t>(4, (int64_t)(gM / 4));
        gDefaultGroupingStyle.pad.v = (int32_t)std::max<int64_t>(4, (int64_t)(gM / 4));

        gDefaultFormattedDocStyle.pad.h = (int32_t)gSpacer;
        gDefaultFormattedDocStyle.pad.v = (int32_t)gSpacer;

        gDefaultPanelStyle.pad.h = (int32_t)gSpacer;
        gDefaultPanelStyle.pad.v = (int32_t)gSpacer;


        // update colors
        gAppPalette.GetByName("Tooltip Fill", gStyleTooltip.bgCol);
        gAppPalette.GetByName("Dialog Fill", gStyleButton.bgCol);
        gAppPalette.GetByName("Highlight", gDefaultHighlight);
        gAppPalette.GetByName("Dialog Fill", gDefaultDialogStyle.bgCol);
        gAppPalette.GetByName("Text Fill", gDefaultWinTextEditStyle.bgCol);
        gAppPalette.GetByName("Text Fill", gDefaultFormattedDocStyle.bgCol);
        gAppPalette.GetByName("Dialog Fill", gStyleButton.bgCol);
        gAppPalette.GetByName("Dialog Fill", gDefaultPanelStyle.bgCol);


//        gStyleToggleChecked.bgCol = gDefaultHighlight;
    }


    ZTextLook::ZTextLook(eDeco _decoration, uint32_t _colTop, uint32_t _colBottom)
    {
        decoration = _decoration;
        colTop = _colTop;
        colBottom = _colBottom;
    }

    ZTextLook::ZTextLook(const std::string& s)
    {
        nlohmann::json j = nlohmann::json::parse(s);

        if (j.contains("deco"))
            decoration = j["deco"];

        if (j.contains("colT"))
            colTop = j["colT"];

        if (j.contains("colB"))
            colBottom = j["colB"];
    }

    ZTextLook::operator string() const
    {
        nlohmann::json j;
        j["deco"] = decoration;
        j["colT"] = colTop;
        j["colB"] = colBottom;

        return j.dump();
    }


    Padding::operator string() const
    {
        nlohmann::json j;
        j["v"] = v;
        j["h"] = h;
        j["col"] = col;

        return j.dump();
    }

    Padding::Padding(const std::string& s)
    {
        nlohmann::json j = nlohmann::json::parse(s);

        if (j.contains("h"))
            h = j["h"];

        if (j.contains("v"))
            v = j["v"];

        if (j.contains("col"))
            col = j["col"];
    }

    void Padding::operator = (const Padding& rhs)
    {
        h = rhs.h;
        v = rhs.v;
        col = rhs.col;
    }


    Style::Style(const ZFontParams& _fp, const ZTextLook& _look, ePosition _pos, Padding _pad, uint32_t _bgCol, bool _wrap)
    {
        fp = _fp;
        look = _look;
        pos = _pos;
        pad = _pad;
        bgCol = _bgCol;
        wrap = _wrap;
    }

    Style::Style(const string& s)
    {
        nlohmann::json j = nlohmann::json::parse(s);

        if (j.contains("fp"))
            fp = ZFontParams(j["fp"]);

        if (j.contains("look"))
            look = ZTextLook((string)j["look"]);

        if (j.contains("pos"))
            pos = j["pos"];

        if (j.contains("pad"))
            pad = Padding((string)j["pad"]);

        if (j.contains("bg"))
            bgCol = j["bg"];

        if (j.contains("wrap"))
            wrap = j["wrap"];
    }

    Style::operator string() const
    {
        nlohmann::json j;
        j["fp"] = (string)fp;
        j["look"] = (string)look;
        j["pos"] = pos;
        j["pad"] = (string)pad;
        j["bg"] = bgCol;

        return j.dump();
    }

    void Style::operator = (const Style& rhs)
    {
        fp = rhs.fp;
        look = rhs.look;
        pos = rhs.pos;
        pad = rhs.pad;
        bgCol = rhs.bgCol;
        wrap = rhs.wrap;
        if (rhs.mpFont && rhs.mpFont->GetFontParams() == fp)
            mpFont = rhs.mpFont;
    }


    tZFontPtr Style::Font()
    {
        assert(fp.nScalePoints > 100  && fp.nScalePoints < 50000.0f);
        if (!mpFont || !(mpFont->GetFontParams() == fp))
            mpFont = gpFontSystem->GetFont(fp); // cache

        assert(mpFont);
        return mpFont;
    }

    EditableColor::EditableColor(const string& _encoded_string)
    {
        nlohmann::json j = nlohmann::json::parse(_encoded_string);

        name = j["n"];
        col = j["c"];
        default_color = j["d"];
    }

    EditableColor::operator string() const
    {
        nlohmann::json j;

        j["n"] = name;
        j["c"] = col;
        j["d"] = default_color;

        return j.dump();
    }

    Palette::Palette(const tColorMap& _map)
    {
        mColorMap = _map;
    }


    bool Palette::GetByName(const string& name, uint32_t& col)
    {
        for (int i = 0; i < mColorMap.size(); i++)
        {
            if (mColorMap[i].name == name)
            {
                col = mColorMap[i].col;
                return true;
            }
        }

        return false;
    }




    // for json serialization
    Palette::operator string() const
    {
        nlohmann::json j;
        j["count"] = SH::FromInt(mColorMap.size());
        for (size_t i = 0; i < mColorMap.size(); i++)
        {
            j[SH::FromInt(i)] = (string)mColorMap[i];
        }

        return j.dump();
    }

    Palette::Palette(const string& _encoded_string)
    {
        mColorMap.clear();
        nlohmann::json j = nlohmann::json::parse(_encoded_string);

        int64_t count = SH::ToInt(j["count"]);

        mColorMap.resize(count);

        for (int64_t i = 0; i < count; i++)
        {
            mColorMap[i] = EditableColor(j[SH::FromInt(i)]);
        }
    }
};