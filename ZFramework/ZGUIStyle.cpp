#include "ZGUIStyle.h"
#include "helpers/StringHelpers.h"

using namespace std;

uint32_t        gDefaultDialogFill(0xff575757);
uint32_t        gDefaultTextAreaFill(0xff888888);
uint32_t        gDefaultTooltipFill(0xaa000088);
uint32_t        gDefaultButtonChecked(0xff008800);
uint32_t        gDefaultButtonUnchecked(0xff888888);

ZRect           grDefaultDialogBackgroundEdgeRect(3, 3, 53, 52);


int64_t         gM; // default measure defined by window diagonal ( diagonal / 100 )... all scaled GUI elements should be based off of this measure
int64_t         gSpacer;

//Fonts
ZFontParams     gDefaultTitleFont("Gadugi", 40);
ZFontParams     gDefaultTextFont("Gadugi", 20);

ZGUI::Style gStyleTooltip(ZFontParams("Verdana", 30), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xffffffff, 0xffffffff), ZGUI::C, 8, 8, gDefaultTooltipFill);
ZGUI::Style gStyleCaption(ZFontParams("Gadugi", 36, 400), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xffffffff, 0xffffffff), ZGUI::C, 0, 0, gDefaultDialogFill);
ZGUI::Style gStyleButton(ZFontParams("Verdana", 30, 600), ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xffffffff, 0xffffffff), ZGUI::C, 0, 0, gDefaultDialogFill);
ZGUI::Style gStyleToggleChecked(ZFontParams("Verdana", 30, 600), ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xff00ff00, 0xff008800), ZGUI::C, 0, 0, gDefaultDialogFill);
ZGUI::Style gStyleToggleUnchecked(ZFontParams("Verdana", 30, 600), ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xffffffff, 0xff888888), ZGUI::C, 0, 0, gDefaultDialogFill);
ZGUI::Style gStyleGeneralText(ZFontParams("Verdana", 30), ZGUI::ZTextLook(ZGUI::ZTextLook::kNormal, 0xffffffff, 0xffffffff), ZGUI::LT, 0, 0, 0, true);
ZGUI::Style gDefaultDialogStyle(gDefaultTextFont, ZGUI::ZTextLook(), ZGUI::LT, (int32_t)gSpacer, (int32_t)gSpacer, gDefaultDialogFill, true);
ZGUI::Style gDefaultWinTextEditStyle(gDefaultTextFont, ZGUI::ZTextLook(), ZGUI::LT, (int32_t)gSpacer, (int32_t)gSpacer, gDefaultTextAreaFill);
ZGUI::Style gDefaultGroupingStyle(ZFontParams("Ariel Greek", 16, 200, 2), ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xffffffff, 0xffffffff), ZGUI::LT, 16, -2);
ZGUI::Style gDefaultFormattedDocStyle(gDefaultTitleFont, ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xff000000, 0xff000000), ZGUI::Unknown, (int32_t)gSpacer, (int32_t)gSpacer, 0, false);

ZGUI::Palette gAppPalette{
{
    { "Dialog Fill", gDefaultDialogFill },              // 0
    { "Text Fill", gDefaultTextAreaFill },         // 1
    { "Toggle Checked",  gDefaultButtonChecked},       // 2  checked button color
    { "Toggle Unchecked",  gDefaultButtonUnchecked},   // 3
    { "Tooltip Fill", gDefaultTooltipFill },            // 4

} };


namespace ZGUI
{
    void ComputeLooks()
    {
        gM = (int64_t) (sqrt(grFullArea.Width() * grFullArea.Width() + grFullArea.Height() * grFullArea.Height()) / 100.0);  // default measure
        gSpacer = gM / 5;
        gSpacer = std::max<int64_t>(gSpacer, 4);

        gStyleButton.fp.nHeight = std::max<int64_t>((int64_t)(gM/1.5), 14);
        gStyleButton.paddingH = (int32_t)gSpacer;
        gStyleButton.paddingV = (int32_t)gSpacer;

        gStyleToggleChecked.fp.nHeight = std::max<int64_t>(gM, 14);
        gStyleToggleUnchecked.fp.nHeight = std::max<int64_t>(gM, 14);
        gStyleTooltip.fp.nHeight = std::max<int64_t>((int32_t)(gM/1.5), 14);
        gStyleTooltip.paddingH = (int32_t)gSpacer;
        gStyleTooltip.paddingV = (int32_t)gSpacer;


        gStyleCaption.fp.nHeight = std::max<int64_t>(gM, 14);


        gStyleGeneralText.fp.nHeight = std::max<int64_t>(gM/2, 14);
        gDefaultGroupingStyle.fp.nHeight = (int32_t)(gM*2/5);
        gDefaultGroupingStyle.paddingH = (int32_t)(gM*2/7);
        gDefaultGroupingStyle.paddingV = (int32_t)std::min<int64_t>(-2, (int64_t)(-gM / 5));

        gDefaultTitleFont.nHeight = std::max<int64_t>(gM, 14);
        gDefaultTextFont.nHeight = std::max<int64_t>(gM/4, 10);

        gDefaultFormattedDocStyle.paddingH = (int32_t)gSpacer;
        gDefaultFormattedDocStyle.paddingV = (int32_t)gSpacer;



        // update colors
        gAppPalette.GetByName("Tooltip Fill", gStyleTooltip.bgCol);
        gAppPalette.GetByName("Dialog Fill", gStyleCaption.bgCol);
        gAppPalette.GetByName("Dialog Fill", gStyleButton.bgCol);
        gAppPalette.GetByName("Toggle Checked", gStyleToggleChecked.bgCol);
        gAppPalette.GetByName("Toggle Unchecked", gStyleToggleUnchecked.bgCol);
        gAppPalette.GetByName("Dialog Fill", gDefaultDialogStyle.bgCol);
        gAppPalette.GetByName("Text Fill", gDefaultWinTextEditStyle.bgCol);
        gAppPalette.GetByName("Text Fill", gDefaultFormattedDocStyle.bgCol);
        gAppPalette.GetByName("Dialog Fill", gStyleButton.bgCol);


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

    void Style::operator = (const Style& rhs)
    {
        fp = rhs.fp;
        look = rhs.look;
        pos = rhs.pos;
        paddingH = rhs.paddingH;
        paddingV = rhs.paddingV;
        bgCol = rhs.bgCol;
        wrap = rhs.wrap;
        if (rhs.mpFont && rhs.mpFont->GetFontParams() == fp)
            mpFont = rhs.mpFont;
    }


    tZFontPtr Style::Font()
    {
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