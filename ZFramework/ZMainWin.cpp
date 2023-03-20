#include "ZMainWin.h"
#include "ZFont.h"
#include "ZMessageSystem.h"
#include "ZTimer.h"
#include "ZFormattedTextWin.h"
#include "ZScriptedDialogWin.h"
#include "helpers/StringHelpers.h"
#include <iostream>

#include "ZGraphicSystem.h"
extern ZGraphicSystem		gGraphicSystem;
extern int64_t				gnFramesPerSecond;
extern ZTimer				gTimer;
extern bool                 gbApplicationExiting;

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;





ZMainWin::ZMainWin()
{
	msWinName = "mainwin";
}

bool ZMainWin::Init()
{
	gMessageSystem.AddNotification("quit_app_confirmed", this);
//	gMessageSystem.AddNotification("kill_window", this);
	gMessageSystem.AddNotification("chardown", this);
	gMessageSystem.AddNotification("keydown", this);
	gMessageSystem.AddNotification("keyup", this);

	mbAcceptsCursorMessages = true;

    mbVisible = false;

	return ZWin::Init();
}

bool ZMainWin::Shutdown()
{
	return ZWin::Shutdown();
}

bool ZMainWin::Paint() 
{
	return ZWin::Paint();
}             

bool ZMainWin::ComputeVisibility()
{
    // Main window not visible......bypassing  adding our rect
    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    for (tWinList::reverse_iterator it = mChildList.rbegin(); it != mChildList.rend(); it++)
    {
        ZWin* pWin = *it;

        //ZDEBUG_OUT("ChildPaint:0x%x\n", uint32_t(pWin));

        pWin->ComputeVisibility();
    }

    return true;
}


bool ZMainWin::HandleMessage(const ZMessage& message)
{
	string sType = message.GetType();

	if (sType == "cursor_msg")
	{
		return ZWin::HandleMessage(message);
	}
	else if (sType == "chardown")
	{
		if (GetFocus())
			GetFocus()->OnChar((char) StringHelpers::ToInt(message.GetParam("code")));
		return true;
	}
	else if (sType == "keydown")
	{
		if (GetFocus())
			GetFocus()->OnKeyDown((uint32_t) StringHelpers::ToInt(message.GetParam("code")));
		return true;
	}
	else if (sType == "keyup")
	{
		if (GetFocus())
			GetFocus()->OnKeyUp((uint32_t) StringHelpers::ToInt(message.GetParam("code")));
		return true;
	}
	else if (sType == "quit_app_confirmed")
	{
		gbApplicationExiting = true;
        InvalidateChildren();       // wakes all children that may be waiting on CVs
		return true;
	}

	return ZWin::HandleMessage(message);
}

