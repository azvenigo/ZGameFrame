#include "ZGUIHelpers.h"
#include "helpers/StringHelpers.h"
#include "ZDebug.h"

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
        StringHelpers::makelower(s);

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
            uint32_t pos;

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


    ZRect Arrange(const ZRect& r, const ZRect& ref, ePosition pos, int64_t padding)
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
                newX = ref.left - w - padding;            // outside left
            else if (pos&Right)
                newX = ref.right + padding;
            else
                ZASSERT(false); // if outside it has to be either left or right... outside center isn't a thing....is it?
        }
        else if (pos&HInside)
        {
//            ZASSERT(pos&HInside);
            if (pos&Left)
                newX = ref.left + padding;
            else if (pos&HCenter)
                newX = (ref.left + ref.right - w) / 2;  // center horizontally within ref
            else if (pos&Right)
                newX = ref.right - w - padding;   // inside right
            else
                ZASSERT(false);

        }

        // vertical
        int64_t newY = ref.top;
        if (pos&VOutside)
        {
            if (pos&Top)
                newY = ref.top - h - padding;
            else if (pos&Bottom)
                newY = ref.bottom + padding;
            else
                ZASSERT(false); // outside center, again?
        }
        else if (pos&VInside)
        {
            if (pos&Top)            // inside top
                newY = ref.top + padding;
            else if (pos&VCenter)
                newY = (ref.top + ref.bottom - h) / 2;        // center vertically within ref
            else if (pos&Bottom)    // inside bottom
                newY = ref.bottom - h - padding;
            else
                ZASSERT(false);
        }

        return ZRect(newX, newY, newX + w, newY + h);
    }
};