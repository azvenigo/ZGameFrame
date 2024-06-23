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
        Fit = L|T|R|B,    // Aligned to all sides
        F = Fit,




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
        RA_Descriptor(const ZRect& _area = {}, 
            const std::string& _referenceWindow = "", 
            uint32_t _alignment = (Left|Right|Top|Bottom), 
            float _wRatio = 1.0, 
            float _hRatio = 1.0, 
            int64_t _minWidth = -1, 
            int64_t _minHeight = -1,
            int64_t _maxWidth = -1,
            int64_t _maxHeight = -1) : referenceWindow(_referenceWindow), alignment(_alignment), wRatio(_wRatio), hRatio(_hRatio), minWidth(_minWidth), minHeight(_minHeight), maxWidth(_maxWidth), maxHeight(_maxHeight)
        {
            assert(alignment < 1024);   // all possible legal flags set
        }

        // serialization
        RA_Descriptor(std::string s);       // from string
        operator std::string() const;       // to string


        ZRect area;
        std::string referenceWindow;
        uint32_t alignment;
        float wRatio;
        float hRatio;
        int64_t minWidth;
        int64_t minHeight;
        int64_t maxWidth;
        int64_t maxHeight;
    };


    inline bool ValidPos(ePosition pos) { return pos < (1 << 10); }    // simple validation that flags do not exceed all set. May want to check that invalid combinations like (Left & Right) don't exits
    
    ZRect       Arrange(const ZRect& r, const ZRect& ref, ePosition pos, int64_t paddingH = 0, int64_t paddingV = 0);  // moves r relative to ref based on position flags
    ZRect       Align(const ZRect& r, const ZRect& ref, uint32_t alignmentflags, int64_t paddingH = 0, int64_t paddingV = 0, float fWidthRatio = 1.0, float fHeightRatio = 1.0, int64_t minWidth = -1, int64_t minHeight = -1, int64_t maxWidth = -1, int64_t maxHeight = -1);  // aligns sides of r to ref depending on flags. 

    ZRect       ScaledFit(const ZRect& r, const ZRect& ref);    // scales r to fit inside ref maintaining r's aspect ratio

    std::string ToString(ePosition pos);
    ePosition   FromString(std::string s);

};

