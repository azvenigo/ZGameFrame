#include "ZMainWin.h"
#include "ZFont.h"
#include "ZMessageSystem.h"
#include "ZTimer.h"
#include "ZWinFormattedText.h"
#include "helpers/StringHelpers.h"
#include "ZWinText.H"
#include <iostream>

extern bool             gbApplicationExiting;
extern bool             gbApplicationRestart;


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
    gMessageSystem.AddNotification("app_restart", this);
    gMessageSystem.AddNotification("quit_app_confirmed", this);
	gMessageSystem.AddNotification("chardown", this);
	gMessageSystem.AddNotification("keydown", this);
	gMessageSystem.AddNotification("keyup", this);
    gMessageSystem.AddNotification("message_box", this);

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

void ZMainWin::RenderToBuffer(tZBufferPtr pDest, const ZRect& rAbsSrc, const ZRect& rDst, ZWin* pThis)
{
    if (!mbInitted || pThis == this)
        return;

    ZRect rAbsIntersection(rAbsSrc);
    if (!rAbsIntersection.Intersect(mAreaAbsolute))
        return;// no intersection

    for (tWinList::reverse_iterator it = mChildList.rbegin(); it != mChildList.rend(); it++)
    {
        ZWin* pChild = *it;
        pChild->RenderToBuffer(pDest, rAbsSrc, rDst, pThis);
    }
}


void ZMainWin::SetArea(const ZRect& newArea)
{
    if (mAreaInParent == newArea)
        return;

    ZWin::SetArea(newArea);
    OnParentAreaChange();   // update all children
    InvalidateChildren();
}


bool ZMainWin::ComputeVisibility()
{
    // Main window not visible......bypassing  adding our rect
    //const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    if (!mChildListMutex.try_lock())
        return false;

    for (tWinList::reverse_iterator it = mChildList.rbegin(); it != mChildList.rend(); it++)
    {
        ZWin* pWin = *it;

        //ZDEBUG_OUT("ChildPaint:0x%x\n", uint32_t(pWin));

        if (pWin->mbVisible)
        {
            if (!pWin->ComputeVisibility())
            {
                mChildListMutex.unlock();
                return false;
            }
        }
    }

    mChildListMutex.unlock();
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
		return true;
	}
	else if (sType == "keydown")
	{
		return true;
	}
	else if (sType == "keyup")
	{
		return true;
	}
	else if (sType == "quit_app_confirmed")
	{
        gbApplicationExiting = true;
        InvalidateChildren();       // wakes all children that may be waiting on CVs
		return true;
	}
    else if (sType == "app_restart")
    {
        gbApplicationRestart = true;
        gbApplicationExiting = true;
        InvalidateChildren();       // wakes all children that may be waiting on CVs
        return true;
    }
    else if (sType == "message_box")
    {
        ShowMessageBox(message.GetParam("caption"), message.GetParam("text"), message.GetParam("on_ok"));
    }

	return ZWin::HandleMessage(message);
}


void ZMainWin::ShowMessageBox(std::string caption, std::string text, std::string onOKMessage)
{
    ZWinDialog* pMsgBox = new ZWinDialog();
    pMsgBox->msWinName = "MessageBox";
    pMsgBox->mbAcceptsCursorMessages = true;
    pMsgBox->mbAcceptsFocus = true;

    ZRect r(800, 300);
    pMsgBox->SetArea(r);
    pMsgBox->mBehavior = ZWinDialog::Draggable | ZWinDialog::OKButton;
    pMsgBox->mStyle = gDefaultDialogStyle;

    pMsgBox->msOnOKMessage = onOKMessage;

    ZRect rCaption(r);
    ZWinLabel* pLabel = new ZWinLabel();
    pLabel->msText = caption;
    pMsgBox->SetArea(rCaption);
    pMsgBox->mStyle = gStyleCaption;
    pMsgBox->mStyle.pos = ZGUI::CT;
    pMsgBox->mStyle.fp.nScalePoints = 1000; // 1.0 
    pMsgBox->mStyle.fp.nWeight = 500;
    pMsgBox->mStyle.look = ZGUI::ZTextLook::kEmbossed;
    pMsgBox->mStyle.look.colTop = 0xff999999;
    pMsgBox->mStyle.look.colBottom = 0xff777777;
    pMsgBox->ChildAdd(pLabel);



    ZWinFormattedDoc* pForm = new ZWinFormattedDoc();

    ZGUI::Style textStyle(gStyleGeneralText);
    ZGUI::Style sectionText(gStyleGeneralText);
    sectionText.fp.nWeight = 800;
    sectionText.look.colTop = 0xffaaaaaa;
    sectionText.look.colBottom = 0xffaaaaaa;

    ZRect rForm(1400, 1000);
    rForm = ZGUI::Arrange(rForm, r, ZGUI::C);
    pForm->SetArea(rForm);
    pForm->SetScrollable();
    pForm->mStyle.bgCol = gDefaultDialogFill;
    pForm->SetBehavior(ZWinFormattedDoc::kBackgroundFill, true);
    pForm->mbAcceptsCursorMessages = false;
    pMsgBox->ChildAdd(pForm);
    ChildAdd(pMsgBox);
    pMsgBox->Arrange(ZGUI::C, mAreaLocal);

    pForm->AddMultiLine(text, textStyle);


    pForm->Invalidate();


}

