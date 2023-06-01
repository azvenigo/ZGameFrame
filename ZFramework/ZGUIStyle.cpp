#include "ZGUIStyle.h"
#include "helpers/StringHelpers.h"

using namespace std;

namespace ZGUI
{
    Style::Style(const ZFontParams& _fp, const ZTextLook& _look, ePosition _pos, int32_t _padding, uint32_t _bgCol, bool _wrap)
    {
        fp = _fp;
        look = _look;
        pos = _pos;
        padding = _padding;
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
            padding = j["pad"];

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
        j["pad"] = padding;
        j["bg"] = bgCol;

        return j.dump();
    }

    tZFontPtr Style::Font()
    {
        if (!mpFont)
            mpFont = gpFontSystem->GetFont(fp); // cache

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

    void TextBox::Paint(ZBuffer* pDst)
    {
        if (sText.empty())
            return;

        assert(pDst);
        // assuming pDst is locked
        ZRect rDraw(area);
        if (rDraw.Width() == 0 || rDraw.Height() == 0)
            rDraw = pDst->GetArea();

        style.Font()->DrawTextParagraph(pDst, sText, rDraw, style.look, style.pos);
    }

    void TextBox::Paint(ZBuffer* pDst, tTextboxMap& textBoxMap)
    {
        for (auto& t : textBoxMap)
            t.second.Paint(pDst);
    }
};