#pragma once

#include "ZWin.H"
#include <string>
#include "ZGUIStyle.h"
#include <list>

typedef std::list<uint32_t> tColorList;

/*struct ColorWatch
{
    ColorWatch(uint32_t* _pWatch = nullptr, const std::string& _sLabel = "") : mpWatchColor(_pWatch), mOriginalColor(*_pWatch), msWatchLabel(_sLabel) {}
    uint32_t* mpWatchColor;
    uint32_t mOriginalColor;   // before commiting an edit
    std::string msWatchLabel;
};

//typedef std::vector<ColorWatch> tColorWatchVector;
*/

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

    static ZWinPaletteDialog* ShowPaletteDialog(std::string sCaption, ZGUI::tColorMap* pColorMap, std::string sOnOKMeessage = "", size_t nRecentColors = 0);

protected:
    void        OnOK();
    void        OnCancel();

    bool        Process();
    bool        Paint();

    void        SelectSV(int64_t x, int64_t y); // relative to mrSVArea
    void        SelectH(int64_t y); // relative to mrHArea.top
    void        SelectFromPalette(int64_t x, int64_t y);
    void        SelectPaletteIndex(size_t nIndex, bool bOriginalColor = false);
    void        UpdatePalette();        // based on selected changes

    void        ComputeAreas();

//    tColorWatchVector mWatchArray;
    ZGUI::tColorMap* mpColorMap;

    ZGUI::tColorMap mOriginalColorMap;

    tColorList      mRecentColors;

    bool            mbSelectingSV;
    bool            mbSelectingH;

    uint32_t        mCurH;
    uint32_t        mCurS;
    uint32_t        mCurV;


    std::string     msCaption;

    ZRect           mrSVArea;
    ZRect           mrHArea;

    size_t          mnSelectingColorIndex;

    ZRect           mrPaletteArea;
    ZRect           mrRecentColorsArea;
};
