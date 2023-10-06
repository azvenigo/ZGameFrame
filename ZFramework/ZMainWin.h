#pragma once

#include "ZTypes.h"
#include "ZWin.h"
#include "ZWinBtn.h"
#include "ZTimer.h"


/////////////////////////////////////////////////////////////////////////
// 
class ZMainWin : public ZWin
{
public:
	ZMainWin();

	bool    Init();
	bool    Shutdown();
	bool    Paint();
    bool    ComputeVisibility();
    void    SetArea(const ZRect& newArea);
    void    RenderToBuffer(tZBufferPtr pTexture, ZRect rSrc, ZRect rDst, ZWin* pThis = nullptr); // override because this window is never visible


    void    ShowMessageBox(std::string title, std::string message, std::string onOKMessage);

protected:
    bool    HandleMessage(const ZMessage& message);
	void    ShowMainMenu();
};

extern ZMainWin* gpMainWin;


