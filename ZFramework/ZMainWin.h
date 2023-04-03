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

	bool			Init();
	bool			Shutdown();
	bool			Paint();
	//bool			OnMouseDownL(int64_t x, int64_t y);
    bool            ComputeVisibility();


protected:
	virtual bool	HandleMessage(const ZMessage& message);
	void  			ShowMainMenu();
};

extern ZMainWin* gpMainWin;


