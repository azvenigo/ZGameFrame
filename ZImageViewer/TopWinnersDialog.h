#pragma once

#include "ZWin.H"
#include <string>
#include "ZGUIStyle.h"
#include "ZWinFormattedText.h"
#include <list>
#include <filesystem>
#include "ImageContest.h"


class TopWinnersDialog : public ZWinDialog
{
public:
    TopWinnersDialog();
    bool        Init();
    virtual bool    OnKeyDown(uint32_t c);

    ZGUI::Style mStyle;

    static TopWinnersDialog* ShowDialog(tImageMetaList sortedList, ZRect rArea);


protected:
    bool        HandleMessage(const ZMessage& message);
    void        OnOK();
    bool        Paint();

    ZWinFormattedDoc* mpFilesList;

    tImageMetaList sortedList;
};
