#pragma once

#include "ZTypes.h"
#include <string>
#include <assert.h>

class ZFontParams;
class ZTextLook;

namespace ZGUI
{
    enum ePosition : uint32_t
    {
        Unknown    = 0,
        Left       = 1 << 0, // 1
        Top        = 1 << 1, // 2
        Right      = 1 << 2, // 4
        Bottom     = 1 << 3, // 8
        HCenter    = 1 << 4, // 16
        VCenter    = 1 << 5, // 32
        HInside    = 1 << 6, // 64
        HOutside   = 1 << 7, // 128
        VInside    = 1 << 8, // 256
        VOutside   = 1 << 9, // 512


        // Aliases
        L = Left,
        T = Top,
        R = Right,
        B = Bottom,
        HC = HCenter,
        VC = VCenter,
        C = HCenter|VCenter,
        I = HInside|VInside,
        O = HOutside|VOutside,




        // Combination positions
        /*
        
        
                      
              +---------------------+
              |LT       CT        RT|
              |                     |
              |                     |
              |LC      Center     RC|
              |                     |
              |                     |
              |LB       CB        RB|
              +---------------------+
        */

        LT = Left|Top,           CT     = Top|HCenter,        RT = Top|Right,
        LC = Left|VCenter,       Center = HCenter|VCenter,    RC = Right|VCenter,
        LB = Left|Bottom,        CB     = Bottom|HCenter,     RB = Bottom|Right,

        // Aliases
        LeftTop = LT,       
        CenterTop = CT,     
        RightTop = RT,

        LeftCenter = LC,
        RightCenter = RC,

        LeftBottom = LB,    
        CenterBottom = CB,  
        RightBottom = RB,
        


        /*
          
          
          
                  Inside/Outside Modifiers                
          
              |                                |
          OLOT|ILOT          ICOT          IROT|OROT
         -----+--------------------------------+-------
          OLIT|ILIT          ICIT          IRIT|ORIT
              |                                |
              |                                |
              |                                |
          OLIC|ILIC          ICIC          IRIC|ORIC
              |                                |
              |                                |
              |                                |
          OLIB|ILIB          ICIB          IRIB|ORIB
         -----+--------------------------------+-------
          OLOB|ILOB          ICOB          IROB|OROB
              |                                |

        */

         OLOT = HOutside|Left|VOutside|Top,     ILOT = HInside|Left|VOutside|Top,    ICOT = HInside|HCenter|VOutside|Top,    IROT = HInside|Right|VOutside|Top,     OROT = HOutside|Right|VOutside|Top,
         OLIT = HOutside|Left|VInside|Top,      ILIT = HInside|Left|VInside|Top,     ICIT = HInside|HCenter|VInside|Top,     IRIT = HInside|Right|VInside|Top,      ORIT = HOutside|Right|VInside|Top,
         OLIC = HOutside|Left|VInside|VCenter,  ILIC = HInside|Left|VInside|VCenter, ICIC = HInside|HCenter|VInside|VCenter, IRIC = HInside|Right|VInside|VCenter,  ORIC = HOutside|Right|VInside|VCenter,
         OLIB = HOutside|Left|VInside|Bottom,   ILIB = HInside|Left|VInside|Bottom,  ICIB = HInside|HCenter|VInside|Bottom,  IRIB = HInside|Right|VInside|Bottom,   ORIB = HOutside|Right|VInside|Bottom,
         OLOB = HOutside|Left|VOutside|Bottom,  ILOB = HInside|Left|VOutside|Bottom, ICOB = HInside|HCenter|VOutside|Bottom, IROB = HInside|Right|VOutside|Bottom,  OROB = HOutside|Right|VOutside|Bottom,
    };


    /// <summary>
    /// RelativeArea - convenient method for child to maintain aspect ratio, size and position relative to a parent.
    /// Once constructed, Area() function returns the computed area
    /// </summary>

    class RA_Descriptor
    {
    public:
        RA_Descriptor(const ZRect& _area = {}, const ZRect& _ref = {}, ePosition _anchorpos = ILIT) : area(_area), ref(_ref), anchorpos(_anchorpos) 
        {
            assert(anchorpos < 1024);   // all possible legal flags set
        }

        // serialization
        RA_Descriptor(std::string s);       // from string
        operator std::string() const;       // to string


        ZRect area;
        ZRect ref;
        ePosition anchorpos;
    };

    class RelativeArea
    {
    public:
        RelativeArea(const RA_Descriptor& rad) : RelativeArea(rad.area, rad.ref, rad.anchorpos){}
        RelativeArea(const ZRect& area = {}, const ZRect& ref = {}, ePosition _anchorpos = ILIT)
        {
            anchorpos = _anchorpos;

            if (area.Area() > 0)
                aspectratio = (double)area.Width() / (double)area.Height();

            if (ref.Area() > 0)
            {
                sizeratio = (double)area.Height() / (double)ref.Height();

                if (Set(ZGUI::ePosition::R))
                    offset.x = (double)(ref.right - area.right) / (double)ref.Width();
                else
                    offset.x = (double)(area.left - ref.left) / (double)ref.Width();

                if (Set(ZGUI::ePosition::B))
                    offset.y = (double)(ref.bottom - area.bottom) / (double)ref.Height();
                else
                    offset.y = (double)(area.top - ref.top) / (double)ref.Height();
            }
        }

        // serialization
        RelativeArea(std::string s);    // from string
        operator std::string() const;   // to string

        ZRect Area(const ZRect& ref) const;

        bool Set(ePosition pos) const { return (anchorpos & pos) != 0; }


        ePosition   anchorpos;
        ZFPoint     offset; // normalized % offset from anchor 0.0-1.0
        double      aspectratio;  
        double      sizeratio;  // normalized % of size based on parent (my_height / parent_height)
    };


    ZRect       Arrange(const ZRect& r, const ZRect& ref, ePosition pos, int64_t paddingH = 0, int64_t paddingV = 0);  // moves r relative to ref based on position flags

    ZRect       ScaledFit(const ZRect& r, const ZRect& ref);    // scales r to fit inside ref maintaining r's aspect ratio

    std::string ToString(ePosition pos);
    ePosition   FromString(std::string s);

};

