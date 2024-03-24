#include "ZGUIHelpers.h"
#include "helpers/StringHelpers.h"
#include "ZStringHelpers.h"
#include "ZDebug.h"
#include "json.hpp"

using namespace std;

namespace ZGUI
{
    string ToString(ePosition pos)
    {
        string val;
        if (pos&HOutside)
            val += "O";
        else
            val += "I";

        if (pos&L)
            val += "L";
        else if (pos&HC)
            val += "C";
        else
            val += "R";

        if (pos&VOutside)
            val += "O";
        else
            val += "I";

        if (pos&T)
            val += "T";
        else if (pos&VC)
            val += "C";
        else
            val += "R";

        return val;
    }

    ePosition FromString(string s)
    {
        SH::makelower(s);

        // Single position
        if (s == "l" || s == "Left")
            return ePosition::L;

        if (s == "t" || s == "top")
            return ePosition::T;

        if (s == "r" || s == "right")
            return ePosition::R;

        if (s == "b" || s == "bottom")
            return ePosition::B;

        if (s == "c" || s == "center")
            return ePosition::C;

        if (s == "hc" || s == "hcenter")
            return ePosition::HC;

        if (s == "vc" || s == "vcenter")
            return ePosition::VC;

        if (s == "hi" || s == "hinside")
            return ePosition::HInside;

        if (s == "ho" || s == "houtside")
            return ePosition::HOutside;

        if (s == "i" || s == "inside")
            return ePosition::I;

        if (s == "o" || s == "outside")
            return ePosition::O;


        // compound position
        if (s == "lt" || s == "lefttop")
            return ePosition::LT;

        if (s == "ct" || s == "centertop")
            return ePosition::CT;

        if (s == "rt" || s == "righttop")
            return ePosition::RT;

        if (s == "lc" || s == "leftcenter")
            return ePosition::LC;

        if (s == "rc" || s == "rightcenter")
            return ePosition::RC;

        if (s == "lb" || s == "leftbottom")
            return ePosition::LB;

        if (s == "cb" || s == "centerbottom")
            return ePosition::CB;

        if (s == "rb" || s == "rightbottom")
            return ePosition::RB;


        ZASSERT(s.size() == 4);

        if (s.size() == 4)
        {
            uint32_t pos = 0;

            if (s[0] == 'i')
            {
                pos |= ePosition::HInside;
            }
            else
            {
                ZASSERT(s[0] == 'o');
                pos |= ePosition::HOutside;
            }

            if (s[1] == 'l')
            {
                pos |= ePosition::L;
            }
            else if (s[1] == 'c')
            {
                pos |= ePosition::HC;
            }
            else
            {
                ZASSERT(s[1] == 'r');
                pos |= ePosition::R;
            }

            if (s[2] == 'i')
            {
                pos |= ePosition::VInside;
            }
            else
            {
                ZASSERT(s[2] == 'o');
                pos |= ePosition::VOutside;
            }

            if (s[3] == 't')
            {
                pos |= ePosition::T;
            }
            else if (s[3] == 'c')
            {
                pos |= ePosition::VC;
            }
            else
            {
                ZASSERT(s[3] == 'b');
                pos |= ePosition::B;
            }

            return (ePosition)pos;
        }

        return ePosition::Unknown;
    }


    ZRect Arrange(const ZRect& r, const ZRect& ref, ePosition pos, int64_t paddingH, int64_t paddingV)
    {
        int64_t w = r.Width();
        int64_t h = r.Height();

        // if no inside/outside, assign inside
        if (pos == LT)
            pos = ILIT;
        else if (pos == CT)
            pos = ICIT;
        else if (pos == RT)
            pos = IRIT;
        else if (pos == LC)
            pos = ILIC;
        else if (pos == C)
            pos = ICIC;
        else if (pos == RC)
            pos = IRIC;
        else if (pos == LB)
            pos = ILIB;
        else if (pos == CB)
            pos = ICIB;
        else if (pos == RB)
            pos = IRIB;

        // horizontal position
        int64_t newX = ref.left;
        if (pos&HOutside)
        {
            if (pos&Left)
                newX = ref.left - w - paddingH;            // outside left
            else if (pos&Right)
                newX = ref.right + paddingH;
            else
                ZASSERT(false); // if outside it has to be either left or right... outside center isn't a thing....is it?
        }
        else if (pos&HInside)
        {
//            ZASSERT(pos&HInside);
            if (pos&Left)
                newX = ref.left + paddingH;
            else if (pos&HCenter)
                newX = (ref.left + ref.right)/2 - w/2;  // center horizontally within ref
            else if (pos&Right)
                newX = ref.right - w - paddingH;   // inside right
            else
                ZASSERT(false);

        }

        // vertical
        int64_t newY = ref.top;
        if (pos&VOutside)
        {
            if (pos&Top)
                newY = ref.top - h - paddingV;
            else if (pos&Bottom)
                newY = ref.bottom + paddingV;
            else
                ZASSERT(false); // outside center, again?
        }
        else if (pos&VInside)
        {
            if (pos&Top)            // inside top
                newY = ref.top + paddingV;
            else if (pos&VCenter)
                newY = (ref.top + ref.bottom)/2 - h/2;        // center vertically within ref
            else if (pos&Bottom)    // inside bottom
                newY = ref.bottom - h - paddingV;
            else
                ZASSERT(false);
        }

        return ZRect(newX, newY, newX + w, newY + h);
    }

    ZRect ScaledFit(const ZRect& r, const ZRect& ref)
    {
        double fRatio = (double)r.Height() / (double)r.Width();

        int64_t nImageWidth = ref.Width();
        int64_t nImageHeight = (int64_t)(nImageWidth * fRatio);

        if (nImageHeight > ref.Height())
        {
            nImageHeight = ref.Height();
            nImageWidth = (int64_t)(nImageHeight / fRatio);
        }

        int64_t nXOffset = ref.left + (ref.Width() - nImageWidth) / 2;
        int64_t nYOffset = ref.top + (ref.Height() - nImageHeight) / 2;


        return ZRect(nXOffset, nYOffset, nXOffset + nImageWidth, nYOffset + nImageHeight);
    }


    ZRect RelativeArea::Area(const ZRect& ref) const
    {
        // compute area from reference

        int64_t rh = ref.Height();

        double h = ref.Height() * sizeratio;
        double w = h * aspectratio;

        ZRect r((int64_t)w, (int64_t)h);

        double x;
        double y;

        if (Set(ePosition::R))
        {
            x = (double)ref.right - ((double)ref.Width()) * offset.x;
            if (Set(ePosition::HInside))    // if inside the rect, shift left
                x -= w;
        }
        else
        {
            x = (double)ref.left + ((double)ref.Width() * offset.x);
            if (Set(ePosition::HOutside))    // if outside the rect, shift left
                x -= w;
        }

        if (Set(ZGUI::ePosition::B))
        {
            y = (double)ref.bottom - ((double)ref.Height()) * offset.y;
            if (Set(ePosition::VInside))    // if outside the rect, shift up
                y -= h;
        }
        else
        {
            y = (double)ref.top + ((double)ref.Height() * offset.y);
            if (Set(ePosition::VOutside))    // if outside the rect, shift up
                y -= h;
        }

        r.OffsetRect((int64_t)x, (int64_t)y);


        return r;
    }



    ePosition   anchorpos;
    ZFPoint     offset; // normalized % offset from anchor 0.0-1.0
    double      aspectratio;
    double      sizeratio;  // normalized % of size based on parent (my_height / parent_height)

    RelativeArea::operator std::string() const
    {
        nlohmann::json j;
        j["p"] = (uint32_t)anchorpos;
        j["ox"] = offset.x;
        j["oy"] = offset.y;
        j["a"] = aspectratio;
        j["s"] = sizeratio;

        return j.dump();
    }

    RelativeArea::RelativeArea(string s)
    {
        nlohmann::json j = nlohmann::json::parse(s);

        if (j.contains("p")) anchorpos = (ePosition)j["p"];
        if (j.contains("ox")) offset.x = j["ox"];
        if (j.contains("oy")) offset.y = j["oy"];
        if (j.contains("a")) aspectratio = j["a"];
        if (j.contains("s")) sizeratio = j["s"];
    }


    RA_Descriptor::RA_Descriptor(std::string s)
    {
        nlohmann::json j = nlohmann::json::parse(s);

        j.at("al").get_to(area.left);
        j.at("at").get_to(area.top);
        j.at("ar").get_to(area.right);
        j.at("ab").get_to(area.bottom);

        j.at("rl").get_to(ref.left);
        j.at("rt").get_to(ref.top);
        j.at("rr").get_to(ref.right);
        j.at("rb").get_to(ref.bottom);

        j.at("pos").get_to(anchorpos);
    }

    RA_Descriptor::operator std::string() const
    {
        nlohmann::json j;
        j["al"] = area.left;
        j["at"] = area.top;
        j["ar"] = area.right;
        j["ab"] = area.bottom;

        j["rl"] = ref.left;
        j["rt"] = ref.top;
        j["rr"] = ref.right;
        j["rb"] = ref.bottom;

        j["pos"] = (uint32_t)anchorpos;

        return j.dump();
    }

};