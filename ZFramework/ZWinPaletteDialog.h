#pragma once

#include "ZWin.H"
#include <string>
#include "ZGUIStyle.h"
#include <list>

typedef std::list<uint32_t> tColorList;

struct ColorWatch
{
    ColorWatch(uint32_t* _pWatch = nullptr, const std::string& _sLabel = "") : mpWatchColor(_pWatch), msWatchLabel(_sLabel) {}
    uint32_t* mpWatchColor;
    std::string msWatchLabel;
};

typedef std::list<ColorWatch> tColorWatchList;

class ZWinPaletteDialog : public ZWinDialog
{
public:
    ZWinPaletteDialog();
    bool        Init();

    void        ShowRecentColors(size_t nCount);

    bool        OnMouseDownL(int64_t x, int64_t y);
    bool        OnMouseUpL(int64_t x, int64_t y);
    bool        OnMouseMove(int64_t x, int64_t y);
    bool        OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);

    ZGUI::Style mStyle;

    static ZWinPaletteDialog* ShowPaletteDialog(std::string sCaption, ColorWatch& watch, size_t nRecentColors = 0);

protected:
    bool        Process();
    bool        Paint();

    void        SelectSV(int64_t x, int64_t y); // relative to mrSVArea
    void        SelectH(int64_t y); // relative to mrHArea.top

    void        ComputeAreas();

    tColorWatchList mWatchList;
    tColorList      mRecentColors;

    bool            mbSelectingSV;
    bool            mbSelectingH;

    uint32_t        mCurH;
    uint32_t        mCurS;
    uint32_t        mCurV;


    std::string     msCaption;

    ZRect           mrSVArea;
    ZRect           mrHArea;
    ZRect           mrSelectingColorArea;

    ZRect           mrPaletteArea;
    ZRect           mrRecentColorsArea;
};
