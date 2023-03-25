#include "ZGUIStyle.h"

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

    Style::Style(const std::string& s)
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


};