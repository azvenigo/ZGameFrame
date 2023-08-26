#include "ZGUIStyle.h"
#include "helpers/StringHelpers.h"

using namespace std;

uint32_t        gDefaultDialogFill(0xff575757);
uint32_t        gDefaultTextAreaFill(0xff888888);
//uint32_t        gDefaultSpacer(16);
//int64_t         gnDefaultGroupInlaySize(9);
ZRect           grDefaultDialogBackgroundEdgeRect(3, 3, 53, 52);
int64_t         gM; // default measure defined by window diagonal ( diagonal / 100 )... all scaled GUI elements should be based off of this measure
int64_t         gSpacer;

//Fonts
ZFontParams     gDefaultTitleFont("Gadugi", 40);
ZFontParams     gDefaultTextFont("Gadugi", 20);

ZGUI::Style gStyleTooltip(ZFontParams("Verdana", 30), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xff000000, 0xff000000), ZGUI::C, 0, 0, gDefaultTextAreaFill);
ZGUI::Style gStyleCaption(ZFontParams("Gadugi", 36, 400), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xffffffff, 0xffffffff), ZGUI::C, 0, 0, gDefaultDialogFill);
ZGUI::Style gStyleButton(ZFontParams("Verdana", 30, 600), ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xffffffff, 0xffffffff), ZGUI::C, 0, 0, gDefaultDialogFill);
ZGUI::Style gStyleToggleChecked(ZFontParams("Verdana", 30, 600), ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xff00ff00, 0xff008800), ZGUI::C, 0, 0, gDefaultDialogFill);
ZGUI::Style gStyleToggleUnchecked(ZFontParams("Verdana", 30, 600), ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xffffffff, 0xff888888), ZGUI::C, 0, 0, gDefaultDialogFill);
ZGUI::Style gStyleGeneralText(ZFontParams("Verdana", 30), ZGUI::ZTextLook(ZGUI::ZTextLook::kNormal, 0xffffffff, 0xffffffff), ZGUI::LT, 0, 0, 0, true);
ZGUI::Style gDefaultDialogStyle(gDefaultTextFont, ZGUI::ZTextLook(), ZGUI::LT, gSpacer, gSpacer, gDefaultDialogFill, true);
ZGUI::Style gDefaultWinTextEditStyle(gDefaultTextFont, ZGUI::ZTextLook(), ZGUI::LT, gSpacer, gSpacer, gDefaultTextAreaFill);
ZGUI::Style gDefaultGroupingStyle(ZFontParams("Ariel Greek", 16, 200, 2), ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xffffffff, 0xffffffff), ZGUI::LT, 16, 2);

ZGUI::Palette gAppPalette{
{
    { "dialog_fill", gDefaultDialogFill },     // 0
    { "text_area_fill", gDefaultTextAreaFill },   // 1
    { "btn_col_checked", 0xff008800 },             // 2  checked button color
    { "", 0xff888888 }             // 3
} };


namespace ZGUI
{
    void ComputeSizes()
    {
//        gDefaultSpacer = (uint32_t)(grFullArea.Height() / 125);
//        gnDefaultGroupInlaySize = 9;
        gM = sqrt(grFullArea.Width() * grFullArea.Width() + grFullArea.Height() * grFullArea.Height()) / 100;  // default measure
        gSpacer = gM / 5;

        gStyleButton.fp.nHeight = std::max<int64_t>(gM/2, 10);
        gStyleToggleChecked.fp.nHeight = std::max<int64_t>(gM, 10);
        gStyleToggleUnchecked.fp.nHeight = std::max<int64_t>(gM, 10);
        gStyleTooltip.fp.nHeight = std::max<int64_t>(gM/3, 10);
        gStyleCaption.fp.nHeight = std::max<int64_t>(gM*.75, 10);
        gStyleGeneralText.fp.nHeight = std::max<int64_t>(gM/3, 10);
        gDefaultGroupingStyle.fp.nHeight = gM*2/5;
        gDefaultGroupingStyle.paddingH = gM*2/7;
        gDefaultGroupingStyle.paddingV = gM / 32;

        gDefaultTitleFont.nHeight = std::max<int64_t>(gM, 10);
        //    gDefaultCaptionFont.nHeight = grFullArea.Height() / 60;
        gDefaultTextFont.nHeight = std::max<int64_t>(gM/4, 10);

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


    Style::Style(const ZFontParams& _fp, const ZTextLook& _look, ePosition _pos, int32_t _paddingH, int32_t _paddingV, uint32_t _bgCol, bool _wrap)
    {
        fp = _fp;
        look = _look;
        pos = _pos;
        paddingH = _paddingH;
        paddingV = _paddingV;
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

        if (j.contains("padH"))
            paddingH = j["padH"];

        if (j.contains("padV"))
            paddingH = j["padV"];

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
        j["padH"] = paddingH;
        j["padV"] = paddingV;
        j["bg"] = bgCol;

        return j.dump();
    }

    tZFontPtr Style::Font()
    {
        if (!mpFont)
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
        for (int i = 0; i < mColorMap.size(); i++)
        {
            j[SH::FromInt(i)] = (string)mColorMap[i];
        }

        return j.dump();
    }

    Palette::Palette(const string& _encoded_string)
    {
        mColorMap.clear();
        nlohmann::json j = nlohmann::json::parse(_encoded_string);

        int count = SH::ToInt(j["count"]);

        mColorMap.resize(count);

        for (int i = 0; i < count; i++)
        {
            mColorMap[i] = EditableColor(j[SH::FromInt(i)]);
        }
    }
};