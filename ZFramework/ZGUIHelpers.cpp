#include "ZGUIHelpers.h"
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

    ZRect Arrange(const ZRect& r, const ZRect& ref, ePosition pos, int64_t HPadding, int64_t VPadding)
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
        int64_t newX;
        if (pos&HOutside)
        {
            if (pos&Left)
                newX = ref.left - w - HPadding;            // outside left
            else if (pos&Right)
                newX = ref.right + HPadding;
            else
                ZASSERT(false); // if outside it has to be either left or right... outside center isn't a thing....is it?
        }
        else
        {
            ZASSERT(pos&HInside);
            if (pos&Left)
                newX = ref.left + HPadding;
            else if (pos&HCenter)
                newX = (ref.left + ref.right - w) / 2;  // center horizontally within ref
            else if (pos&Right)
                newX = ref.right - w - HPadding;   // inside right
            else
                ZASSERT(false);

        }

        // vertical
        int64_t newY;
        if (pos&VOutside)
        {
            if (pos&Top)
                newY = ref.top - h - VPadding;
            else if (pos&Bottom)
                newY = ref.bottom + VPadding;
            else
                ZASSERT(false); // outside center, again?
        }
        else
        {
            if (pos&Top)            // inside top
                newY = ref.top + VPadding;
            else if (pos&VCenter)
                newY = (ref.top + ref.bottom - h) / 2;        // center vertically within ref
            else if (pos&Bottom)    // inside bottom
                newY = ref.bottom - h - VPadding;
            else
                ZASSERT(false);
        }

        return ZRect(newX, newY, newX + w, newY + h);
    }
};