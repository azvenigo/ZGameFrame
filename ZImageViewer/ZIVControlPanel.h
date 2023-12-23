#pragma once

#include "ZWinControlPanel.h"



class ZIVControlPanel : public ZWinControlPanel
{
public:
    ZIVControlPanel();
    bool            Init();
    void            UpdateUI();

    void		    SetArea(const ZRect& newArea);

protected:
    bool            HandleMessage(const ZMessage& message);
    bool            Paint();

    bool            mbShow;
};
