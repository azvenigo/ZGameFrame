#pragma once

#include "ZWinControlPanel.h"



class ZIVControlPanel : public ZWinControlPanel
{
public:

    enum mMode : uint8_t
    {
        kNone = 0,
        kSift = 1,
        kFavor = 2,
        kManage = 3
    };


    ZIVControlPanel();
    bool            Init();
    void            UpdateUI();

    void		    SetArea(const ZRect& newArea);

protected:
    bool            HandleMessage(const ZMessage& message);
    bool            Paint();

    bool            mbShow;


    eState          mMode;
};
