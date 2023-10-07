#include "ZWin.H"
#include "ZBuffer.h"
#include "ZDebug.h"
#include "ZGraphicSystem.h"
#include "helpers/StringHelpers.h"
#include "ZGUIHelpers.h"
#include "ZGUIStyle.h"
#include "ZAnimObjects.h"
#include "ZAnimator.h"
#include "ZXMLNode.h"
#include "ZTimer.h"
#include "ZWinBtn.H"
#include "Resources.h"
#include <iostream>
#include "ZScreenBuffer.h"
#include "ZInput.h"
#include "ZGUIStyle.h"

extern ZAnimator		gAnimator;
extern ZTimer			gTimer;

extern bool gbApplicationExiting;

const int64_t kDefaultTransformTime = 500;
const int64_t kDefaultIdleTime = 1000;

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
using namespace std;

ZWin::ZWin() : 
mMouseDownOffset(0,0),
mpParentWin(0),
mbInitted(false),
mbVisible(true),
mbAcceptsCursorMessages(false),
mbAcceptsFocus(false),
mbInvalid(true),
mbInvalidateParentWhenInvalid(false),
mpSurface(nullptr),
mTransformIn(kNone),
mTransformOut(kNone),
mnTransformInTime(kDefaultTransformTime),
mnTransformOutTime(kDefaultTransformTime),
mbShutdownFlag(false),
mIdleSleepMS(kDefaultIdleTime)
{
//    Sprintf(msWinName, "win_%x", this); // default to unique window name

//    cout << "ZWin::ZWin() : msWinName:" << msWinName << "\n" << std::flush;

}

ZWin::~ZWin() 
{
	gAnimator.KillContextObjects(this);

	if (gInput.keyboardFocusWin == this)
        gInput.keyboardFocusWin = nullptr;

    if (gInput.captureWin == this)
        gInput.captureWin = nullptr;
}

bool ZWin::Init() 
{
	if (!mbInitted)
	{
		if (msWinName.empty())
			msWinName = gMessageSystem.GenerateUniqueTargetName();

#ifdef _DEBUG
		msDebugName = msWinName;
#endif

		gMessageSystem.RegisterTarget(this);

		mbShutdownFlag = false;

        mbInvalid = true;
        mbInitted = true;
        if (mAreaInParent.Width() > 0 && mAreaInParent.Height() > 0)
        {
            ZTransformable::Init(mAreaAbsolute);
            mpSurface.reset(new ZBuffer());
            mpSurface->Init(mAreaAbsolute.Width(), mAreaAbsolute.Height());
            Paint();        // TBD this may be desired since our texture may be requested before the threadproc has a chance to call paint
            TransformIn();
        }

		mThread = std::thread(&WindowThreadProc, (void*)this);
	}
	return true;
}  

bool ZWin::Shutdown()
{
	if (mbInitted)
	{
        TransformOut();
        SetVisible(false);

		mbShutdownFlag = true;
		mThread.join();

		const std::lock_guard<std::mutex> lock(mShutdownMutex);	// prevent shutdown until this returns

		gMessageSystem.RemoveAllNotifications(this);

		if (!msWinName.empty())
			gMessageSystem.UnregisterTarget(this);

        ChildDeleteAll();

		if (gInput.mouseOverWin == this)
            gInput.mouseOverWin = NULL;


        //const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());

		mbInitted = false;
	}
	return true;
}

void ZWin::SignalShutdown() 
{ 
    mbShutdownFlag = true; 
    mWorkToDoCV.notify_one(); 
}


bool ZWin::InitFromXML(ZXMLNode* pNode)
{
	ZASSERT(pNode);
	msWinName = pNode->GetAttribute("winname");
	
	if (pNode->HasAttribute("area"))
		SetArea(StringToRect(pNode->GetAttribute("area")));

	return true;
}

void ZWin::TransformIn()
{
	if (mTransformIn != kNone)
	{
        ZRect rp;
        if (mpParentWin)
            rp.SetRect(mpParentWin->GetArea());
        else
            rp.SetRect(grFullArea);


        ZTransformation start(ZPoint(mAreaAbsolute.left, mAreaAbsolute.top), 1.0, 0, 255);
        ZTransformation end(ZPoint(mAreaAbsolute.left, mAreaAbsolute.top), 1.0, 0, 255);


        switch (mTransformIn)
        {
        case kFade:			start.mnAlpha = 0;                                  break;
        case kSlideLeft:	start.mPosition.x = rp.right;		                break;
        case kSlideRight:	start.mPosition.x = rp.left - mAreaLocal.Width();   break;
        case kSlideUp:		start.mPosition.y = rp.bottom;		                break;
        case kSlideDown:	start.mPosition.y = rp.top - mAreaLocal.Height();   break;
        case kSpinCW:       start.mRotation = -mnTransformInTime/100.0;          break;
        case kSpinCCW:      start.mRotation = mnTransformInTime / 100.0;         break;
        }

        MoveTo(start.mPosition.x, start.mPosition.y);

        SetVisible();
        tZBufferPtr renderedBuf(new ZBuffer());
        renderedBuf->Init(mAreaInParent.Width(), mAreaInParent.Height());
        RenderToBuffer(renderedBuf, mAreaLocal, mAreaLocal, nullptr);

        tZBufferPtr backgroundBuf(new ZBuffer());
        backgroundBuf->Init(grFullArea.Width(), grFullArea.Height());
        GetTopWindow()->RenderToBuffer(backgroundBuf, grFullArea, grFullArea, this);

        ZAnimObject_TransformingImage* pImage = new ZAnimObject_TransformingImage(renderedBuf, backgroundBuf);



        pImage->StartTransformation(start);

//        mnTransformInTime = 1000;


        ZTransformation lastTrans(end);


        Sprintf(end.msCompletionMessage, "{move_to;x=%d;y=%d;target=%s},{set_visible;visible=1;target=%s}", end.mPosition.x, end.mPosition.y, GetTargetName().c_str(), GetTargetName().c_str());    // set visible on completion
        pImage->AddTransformation(end, mnTransformInTime);
        pImage->AddTransformation(lastTrans, 120);

        gAnimator.AddObject(pImage);
        SetVisible(false);

	}
}

void ZWin::TransformOut()
{
	if (mTransformOut != kNone)
	{
        SetVisible();

        tZBufferPtr renderedBuf(new ZBuffer());
        renderedBuf->Init(mAreaInParent.Width(), mAreaInParent.Height());
        RenderToBuffer(renderedBuf, mAreaLocal, mAreaLocal, nullptr);

        tZBufferPtr backgroundBuf(new ZBuffer());
        backgroundBuf->Init(grFullArea.Width(), grFullArea.Height());
        GetTopWindow()->RenderToBuffer(backgroundBuf, grFullArea, grFullArea, this);


		ZAnimObject_TransformingImage* pImage = new ZAnimObject_TransformingImage(renderedBuf, backgroundBuf);
		ZTransformation start(ZPoint(mAreaInParent.left, mAreaInParent.top));
        ZTransformation end(start);


        pImage->StartTransformation(start);
        Sprintf(start.msCompletionMessage, "set_visible;visible=0;target=%s", GetTargetName().c_str());    // set visible after first rendered 
        pImage->AddTransformation(start, 120);

        ZRect rp;
        if (mpParentWin)
            rp.SetRect(mpParentWin->GetArea());
        else
            rp.SetRect(grFullArea);

		switch (mTransformOut)
		{
		case kFade:			end.mnAlpha = 0;                                              break;
        case kSlideLeft:    end.mPosition.x = rp.left - mAreaLocal.Width();    break;
 		case kSlideRight:	end.mPosition.x = rp.right;                        break;
		case kSlideUp:		end.mPosition.y = rp.top - mAreaLocal.Height();    break;
		case kSlideDown:	end.mPosition.y = rp.bottom;                       break;
        case kSpinCW:       end.mRotation = mnTransformInTime / 100.0;          break;
        case kSpinCCW:      end.mRotation = -mnTransformInTime / 100.0;         break;
        }

        pImage->AddTransformation(end,mnTransformOutTime);

		gAnimator.AddObject(pImage);
//        SetVisible(false);
    }
}

void ZWin::Arrange(ZGUI::ePosition pos, const ZRect& outerRect, int64_t padding)
{
    SetArea(ZGUI::Arrange(mAreaInParent, outerRect, pos, padding, padding));
}

void ZWin::ArrangeWindows(tWinList windows, ZRect rArea, ZGUI::Style& style, int64_t nCols, int64_t nRows)
{
    assert(rArea.Width() > 0 && rArea.Height() > 0);
    if (windows.empty())
        return;

    if (nCols < 1)  // undefined number of columns
    {
        nRows = 1;
        nCols = windows.size();
    }
    else if (nRows < 1) // undefined number of rows
    {
        nCols = 1;
        nRows = windows.size();
    }

    int64_t nTotalHPadding = (nCols + 1) * style.paddingH;
    int64_t nTotalVPadding = (nRows + 1) * style.paddingV;

    int64_t w = (rArea.Width() - nTotalHPadding) / nCols;
    int64_t h = (rArea.Height() - nTotalVPadding) / nRows;

    assert(w > 0 && h > 0);

    tWinList::reverse_iterator childIt = windows.rbegin();
    for (int64_t row = 0; row < nRows; row++)
    {
        for (int64_t col = 0; col < nCols; col++)
        {
            ZRect r(w, h);
            r.OffsetRect(rArea.left + style.paddingH * (col + 1) + col * w,     // offset H by number of horizontal paddings by column number
                rArea.top + style.paddingV * (row + 1) + row * h);    // same for V offset

            (*childIt)->SetArea(r);

            childIt++;
            if (childIt == windows.rend())
                break;
        }
    }
}

bool ZWin::SetParentWin(ZWin* pNewParent) 
{
    ZASSERT(pNewParent != this);

	mpParentWin = pNewParent;
	return true;
}

bool ZWin::ChildAdd(ZWin* child, bool bAutoShow)
{
	ZASSERT(child);
//	ZDEBUG_OUT("ChildAdd:%d %s\n", (uint32_t) child, child->GetFullWinName().c_str());

	child->SetParentWin(this);

    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    ZASSERT_MESSAGE(find(mChildList.begin(), mChildList.end(), child) == mChildList.end(), "Child already added!");
    if (mChildList.empty())
        gMessageSystem.AddNotification("kill_child", this);

    mChildList.push_front(child);

	child->ComputeAreas();

	if (!child->Init())
	{
		ZASSERT(false);
		return false;
	}

    // If transforming in, let the animation end set visibility
    if (child->mTransformIn != kNone)
        bAutoShow = false;

    child->SetVisible(bAutoShow);

	return true;
}

bool ZWin::ChildRemove(ZWin* child) 
{
	ZASSERT(child);

    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    for (tWinList::iterator it = mChildList.begin(); it != mChildList.end(); it++)
	{
		if ((*it) == child) 
		{
			mChildList.erase( it );
//			ZDEBUG_OUT("ChildRemove:0x%x\n", uint32_t(child));

//            const std::lock_guard<std::recursive_mutex> surfaceLock(child->mpTransformTexture.get()->GetMutex());
			child->SetParentWin(NULL);

            if (mChildList.empty())
                gMessageSystem.RemoveNotification("kill_child", this);


			return true;
		}
	}   
	return false;
}

void ZWin::ChildDeleteAll() 
{
    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);

    for (auto pChild : mChildList)		// signal all to shutdown first
    {
        pChild->SetVisible(false);
        pChild->SignalShutdown();
    }

	while (!mChildList.empty())			// ChildDelete modifies the list so we can't use regular iteration
		ChildDelete(*mChildList.begin());

	mChildList.clear();
}

bool ZWin::ChildDelete(ZWin* pChild) 
{
    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    if (pChild)
	{
		if (ChildRemove(pChild)) 
		{
			pChild->Shutdown();
			delete pChild;
			return true;
		}
	}
	return false;
}

bool ZWin::ChildToFront(ZWin *child) 
{
    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
   
    if(child && ChildExists(child))
	{
//		if (mpParentWin)
//			mpParentWin->ChildToFront(this);

		ZDEBUG_OUT("ChildToFront:0x%x\n", uint64_t(child));
		mChildList.remove( child );
		mChildList.push_front( child );
        child->Invalidate();
        Invalidate();
        gpGraphicSystem->GetScreenBuffer()->SetVisibilityComputingFlag(true);
        return true;
	}

	return false;
}

bool ZWin::ChildToBack(ZWin* child) 
{
    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
   
    if(child && ChildExists(child))
	{
		ZDEBUG_OUT("ChildToBack:0x%x\n", uint64_t(child));
		mChildList.remove( child );
		mChildList.push_back( child );
        child->Invalidate();
        Invalidate();
        gpGraphicSystem->GetScreenBuffer()->SetVisibilityComputingFlag(true);
        return true;
	}
	return false;
}

bool ZWin::ChildExists(ZWin* testChild) 
{
    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    return find(mChildList.begin(), mChildList.end(), testChild) != mChildList.end();
}

ZWin* ZWin::GetChildWindowByWinName(const string& sWinName)
{
    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    for (tWinList::iterator it = mChildList.begin(); it != mChildList.end(); it++)
	{
		if ((*it)->msWinName == sWinName)
			return *it;
	}

	return NULL;
}

ZWin* ZWin::GetChildWindowByWinNameRecursive(const string& /*sWinName*/)
{
	ZASSERT_MESSAGE(false, "not yet implemented");
	return NULL;
}


ZWin* ZWin::GetChildWindowByPoint( int64_t x, int64_t y ) 
{
    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
   
    ZWin* pWin = NULL;

	if (mChildList.size() > 0)
	{
		for (tWinList::iterator it = mChildList.begin(); it != mChildList.end(); it++) 
		{
			if ((*it)->mbVisible)
			{
				pWin = (*it)->GetChildWindowByPoint(x, y);
				if (pWin)
				{
					break;
				}
			}
		}
	} 

	if (!pWin) 
	{
		if ( IsPointInWindowScreenCoordinates( x, y ) ) 
		{
			pWin = this;
		}
	}

	return pWin;   
}



tWinList ZWin::GetChildrenInGroup(const std::string& sGroup)
{
    tWinList l;
    for (auto pChild : mChildList)
    {
        if (pChild->msWinGroup == sGroup)
            l.push_back(pChild);
    }

    return l;
}

tGroupNameToWinList ZWin::GetChildGroups()
{
    tGroupNameToWinList groups;

    // create a map of any child belonging to groups
    for (auto pChild : mChildList)
    {
        if (!pChild->msWinGroup.empty())
            groups[pChild->msWinGroup].push_back(pChild);
    }

    return groups;
}

ZRect ZWin::GetBounds(const tWinList& list)
{
    if (list.empty())
        return {};

    tWinList::const_iterator it = list.begin();
    ZRect rBounds((*it)->mAreaInParent);
    while (it != list.end())
    {
        rBounds.UnionRect((*it++)->mAreaInParent);
    }

    return rBounds;
}




bool ZWin::SetFocus()
{
//    ZDEBUG_OUT("SetFocus: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    if (mbAcceptsFocus)
	{
        gInput.keyboardFocusWin = this;
		return true;
	}

	return false;
}

bool ZWin::OnChar(char /*c*/)
{
    mWorkToDoCV.notify_one();
    return false;
}

bool  ZWin::OnKeyDown(uint32_t key) 
{
//    ZDEBUG_OUT("OnKeyDown: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    return false;
}

bool  ZWin::OnKeyUp(uint32_t key) 
{
//    ZDEBUG_OUT("OnKeyUp: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    return false;
}

bool  ZWin::ProcessCursorMessage(const ZMessage& message) 
{
	ZASSERT(message.GetType() == "cursor_msg");

	int64_t nX = 0;
	if (message.HasParam("x"))
		nX = SH::ToInt(message.GetParam("x"));
	int64_t nY = 0;
	if (message.HasParam("y"))
		nY = SH::ToInt(message.GetParam("y"));

	string sSubType = message.GetParam("subtype");

	// If this window doesn't have capture, then see if any child needs the cursor message
	if( !AmCapturing() && mChildList.size() ) 
	{
		ZWin* currChild;
        const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
        tWinList::iterator it = mChildList.begin();

		while( it != mChildList.end() ) 
		{
			currChild = (*it);
			if ( currChild->IsVisible() && currChild->mbAcceptsCursorMessages ) 
			{
				if (currChild->IsPointInWindowScreenCoordinates(nX, nY) ) 
				{
					return currChild->ProcessCursorMessage(message); 
				} 
			} 

			it++;
		}
	}

	if ( IsVisible() )
	{
		ScreenToWindowCoordinates(nX, nY);

		if (sSubType == "move")
		{
			return OnMouseMove(nX, nY);
		}
		else if (sSubType == "l_down")
		{
			SetFocus();
			return OnMouseDownL(nX, nY);
		}
		else if (sSubType == "r_down")
		{
			SetFocus();
			return OnMouseDownR(nX, nY);
		}
		else if (sSubType == "l_up")
		{
			return OnMouseUpL(nX, nY);
		}
		else if (sSubType == "r_up")
		{
			return OnMouseUpR(nX, nY);
		}
		else if (sSubType == "wheel")
		{
			int64_t nDelta = SH::ToInt(message.GetParam("delta"));
			return OnMouseWheel(nX, nY, nDelta);
		}
		else if (sSubType == "hover")
		{
			return OnMouseHover(nX, nY);
		}
	}

	return false;
}


bool ZWin::Process() 
{
    return false; 
}

bool  ZWin::Paint()
{
    if (!mbInitted || !mbInvalid)
        return false;

    mpSurface.get()->SetRenderFlag(ZBuffer::kRenderFlag_ReadyToRender);
	mbInvalid = false;
	return true;
}               


void ZWin::RenderToBuffer(tZBufferPtr pDest, ZRect rSrc, ZRect rDst, ZWin* pThis)
{
    if (!mbInitted || !mbVisible || !rSrc.Overlaps(mAreaLocal) || pThis == this)
        return;

    if (!ZBuffer::Clip(mpSurface.get(), pDest.get(), rSrc, rDst))
        return;

    Paint();
    mpSurface.get()->GetMutex().lock();
    pDest->Blt(mpSurface.get(), rSrc, rDst);
    mpSurface.get()->GetMutex().unlock();

    for (tWinList::reverse_iterator it = mChildList.rbegin(); it != mChildList.rend(); it++)
    {
        ZWin* pChild = *it;

        pChild->RenderToBuffer(pDest, pChild->mAreaLocal, pChild->mAreaInParent, pThis);
    }
}

void ZWin::PaintFromParent()
{
    if (mpParentWin && mpParentWin->IsVisible())
    {
        tZBufferPtr parentBuffer = mpParentWin->mpSurface;
        const std::lock_guard<std::recursive_mutex> surfaceLock(parentBuffer->GetMutex());
        mpSurface.get()->Blt(parentBuffer.get(), mAreaInParent, mAreaLocal, nullptr, ZBuffer::kAlphaSource);
    }
}


bool ZWin::ComputeVisibility()
{
	if (!mbVisible)
		return true;

//	if (mbVisible)
	{
		ZScreenRect sr(mpSurface, mAreaAbsolute, ZPoint(0, 0));
        gpGraphicSystem->GetScreenBuffer()->AddScreenRectAndComputeVisibility(sr);
	}

    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    for (tWinList::reverse_iterator it = mChildList.rbegin(); it != mChildList.rend(); it++)
	{
		ZWin* pWin = *it;

		//ZDEBUG_OUT("ChildPaint:0x%x\n", uint32_t(pWin));

        if ( pWin->mbVisible && pWin->mpSurface)
		    pWin->ComputeVisibility();
	}

	return true;
}

bool ZWin::OnParentAreaChange()
{
    Invalidate();
    for (auto pChild : mChildList)
        pChild->OnParentAreaChange();

    return true;
}



void ZWin::ComputeAreas() 
{
    mAreaAbsolute = mAreaLocal;
    ZWin* pWin = this;
	do
	{
		mAreaAbsolute.OffsetRect(pWin->mAreaInParent.left, pWin->mAreaInParent.top);
        pWin = pWin->mpParentWin;
    } while (pWin);

    for (auto child : mChildList)
        child->ComputeAreas();

    if (gpGraphicSystem && gpGraphicSystem->GetScreenBuffer())
        gpGraphicSystem->GetScreenBuffer()->SetVisibilityComputingFlag(true);
}

bool ZWin::SetCapture()
{
	if (gInput.captureWin)
		return false;
    gInput.captureWin = this;
	return true;
}

bool ZWin::ReleaseCapture()
{
	if (gInput.captureWin != this)
		return false;
    gInput.captureWin = NULL;
	return true;
}

bool ZWin::AmCapturing()
{
	return (gInput.captureWin == this);
}

bool  ZWin::OnMouseDownL(int64_t, int64_t)
{
//    ZDEBUG_OUT("OnMouseDownL: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    return false;
}

bool  ZWin::OnMouseUpL(int64_t, int64_t)
{
//    ZDEBUG_OUT("OnMouseUpL: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    return false;
}

bool  ZWin::OnMouseDownR(int64_t, int64_t)
{
//    ZDEBUG_OUT("OnMouseDownR: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    return false;
}

bool  ZWin::OnMouseUpR(int64_t, int64_t) 
{
//    ZDEBUG_OUT("OnMouseUpR: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    return false;
}

bool  ZWin::OnMouseMove(int64_t x, int64_t y)
{
//    ZDEBUG_OUT("OnMouseMove: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    return false;
}

bool ZWin::OnMouseWheel(int64_t, int64_t, int64_t)
{
//    ZDEBUG_OUT("OnMouseWheel: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    return false;
}

bool ZWin::OnMouseHover(int64_t, int64_t)
{
//    ZDEBUG_OUT("OnMouseHover: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    return true;
}

bool ZWin::OnMouseOut()
{
//    ZDEBUG_OUT("OnMouseOut: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    return true;
}

bool ZWin::OnMouseIn()
{
//    ZDEBUG_OUT("OnMouseIn: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    return true;
}

string ZWin::GetFullWinName()
{
	string sFullName(msWinName);

	// Walk up all parents and prepend their name
	ZWin* pParent = mpParentWin;
	while (pParent)
	{
		string sCur = sFullName;
		sFullName = pParent->msWinName;
		sFullName += string(".");
		sFullName += sCur;

		pParent = pParent->GetParentWin();
	}

	return sFullName;
}

bool ZWin::ReceiveMessage(const ZMessage& message)
{
	const std::lock_guard<std::mutex> lock(mMessageQueueMutex);

	mMessages.push_back(message);

    mWorkToDoCV.notify_one();

	return true;
}

bool ZWin::HandleMessage(const ZMessage& message)
{
	string sType = message.GetType();
	if (sType == "cursor_msg")
	{
		ProcessCursorMessage(message);
		return true;
	}
    else if (sType == "keydown")
    {
        OnKeyDown((uint32_t)SH::ToInt(message.GetParam("code")));
        return true;
    }
    else if (sType == "keyup")
    {
        OnKeyUp((uint32_t)SH::ToInt(message.GetParam("code")));
        return true;
    }
    else if (sType == "chardown")
    {
        OnChar((uint32_t)SH::ToInt(message.GetParam("code")));
        return true;
    }
    else if (sType == "kill_child")
	{
        ZWin* pChild = GetChildWindowByWinName(message.GetParam("name"));
        if (pChild)
		{
			ChildDelete(pChild);
			return true;
		}
	}
    else if (sType == "set_focus")
    {
        SetFocus();
        return true;
    }
    else if (sType == "set_visible")
    {
        SetVisible( SH::ToBool(message.GetParam("visible")) );
        return true;
    }
    else if (sType == "move_to")
    {
        MoveTo(SH::ToInt(message.GetParam("x")), SH::ToInt(message.GetParam("y")));
        return true;
    }

#ifdef _DEBUG
	// If the message is targeted, this had better be the target window
	if (message.GetTarget() == GetTargetName())
	{
		ZASSERT_MESSAGE(false, string(string("Targeted message:\"") + string(message.ToString()) + string("\" was not handled!")).c_str());
	}
#endif

	return false;
}

void ZWin::SetVisible(bool bVisible)
{
    if (bVisible != mbVisible)
    {
        mbVisible = bVisible;
        InvalidateChildren();
        gpGraphicSystem->GetScreenBuffer()->SetVisibilityComputingFlag(true);
    }
}

void ZWin::Invalidate()	
{
//    ZDEBUG_OUT("Invalidate: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    mbInvalid = true;
    if (mbInvalidateParentWhenInvalid && mpParentWin)
        mpParentWin->mbInvalid = true;
}

void ZWin::InvalidateChildren()
{
    Invalidate();
    for (auto pChild : mChildList)
        pChild->InvalidateChildren();
}


void ZWin::SetArea( const ZRect& area ) 
{
    if (!mpSurface || area.Width() != mAreaLocal.Width() || area.Height() != mAreaLocal.Height())
    {
        if (area.Width() > 0 && area.Height() > 0)
        {
            mpSurface.reset(new ZBuffer());
            mpSurface->Init(area.Width(), area.Height());
        }// else no need to create a surface
    }

    mAreaInParent.SetRect(area);
    mAreaLocal.SetRect(0, 0, area.Width(), area.Height());
    ZTransformable::Init(mAreaLocal);

    ComputeAreas();
}

void  ZWin::MoveTo(int64_t newX, int64_t newY) 
{
	SetArea(ZRect(newX, newY, newX+mAreaInParent.Width(), newY+mAreaInParent.Height()));
}

/*void  ZWin::OffsetTo(int64_t deltaX, int64_t deltaY) 
{
	SetArea( mArea.left + deltaX, mArea.top + deltaY, mArea.right + deltaX, mArea.bottom + deltaY );
}
*/

bool ZWin::WindowThreadProc(void* pContext)
{
	ZWin* pThis = (ZWin*)pContext;

#ifdef _WIN64
    HRESULT r;
    string sName;
    Sprintf(sName, "th_0x%x", (int) (int64_t)GetCurrentThread());
    r = SetThreadDescription(GetCurrentThread(), SH::string2wstring(sName).c_str());

    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
#endif



    while (!pThis->mbShutdownFlag && !gbApplicationExiting)
	{
		if (!pThis->mMessages.empty())	// race condition that message could be added after this check. But no big deal as it'll catch it next loop
		{
			// Handle messages
			pThis->mMessageQueueMutex.lock();
			tMessageList messagesToProcess(std::move(pThis->mMessages));	// take all messages from queue to be processed
			pThis->mMessageQueueMutex.unlock();

			for (ZMessage& msg : messagesToProcess)
			{
				if (!pThis->HandleMessage(msg))
				{
					cout << "Failed to process message: " << msg.ToString() << "\n";
				}
			}
		}

        if (pThis->mbShutdownFlag)
            break;

        bool bActive = pThis->Process();
        bActive |= pThis->Paint();
        bActive |= pThis->mMessages.size() > 0;

        if (bActive)    // if either simulation was done or painting was done do a quick sleep
        {
            std::unique_lock<std::mutex> lk(pThis->mMessageQueueMutex);

            uint64_t nStartTime = gTimer.GetMSSinceEpoch();

            pThis->mWorkToDoCV.wait_for(lk, std::chrono::milliseconds(1));

            uint64_t nEndTime = gTimer.GetMSSinceEpoch();
            uint64_t nDelta = nEndTime - nStartTime;

/*            if (nDelta < 1)
            {
                ZOUT("Woken early after: %dms - sleep was:%dms\n", nDelta, 1);
            }*/
        }
        else
        {
            std::unique_lock<std::mutex> lk(pThis->mMessageQueueMutex);

            uint64_t nStartTime = gTimer.GetMSSinceEpoch();

            pThis->mWorkToDoCV.wait_for(lk, std::chrono::milliseconds(pThis->mIdleSleepMS));

            uint64_t nEndTime = gTimer.GetMSSinceEpoch();
            uint64_t nDelta = nEndTime-nStartTime;

/*            if (nDelta < pThis->mIdleSleepMS)
            {
                ZOUT("Woken early after: %dms - sleep was:%dms\n", nDelta, pThis->mIdleSleepMS);
            }*/
        }
	}

	const std::lock_guard<std::mutex> lock(pThis->mShutdownMutex);

	return true;
}



ZWinDialog::ZWinDialog() : mStyle(gDefaultDialogStyle), mBehavior(eBehavior::None)
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
}

bool ZWinDialog::Init()
{
    //ZRect rButton(0, 0, gM * 4, gM * 2);
    ZRect rButtonArea(mAreaLocal);
    rButtonArea.top = rButtonArea.bottom - gM * 2;
//    rButton = ZGUI::Arrange(rButton, mAreaToDrawTo, ZGUI::RB, gSpacer, gSpacer);

    tWinList arrangeList;

    if (mBehavior & eBehavior::CancelButton)
    {
        ZWinSizablePushBtn* pBtn;

        pBtn = new ZWinSizablePushBtn();
        pBtn->mCaption.sText = "Cancel";
        pBtn->mCaption.style = gStyleButton;
        pBtn->SetMessage(ZMessage("dlg_cancel", this));
//        pBtn->SetArea(rButton);
        ChildAdd(pBtn);
        arrangeList.push_back(pBtn);

//        rButton.OffsetRect(-rButton.Width() - gSpacer, 0);
    }

    if (mBehavior & eBehavior::OKButton)
    {
        ZWinSizablePushBtn* pBtn;

        pBtn = new ZWinSizablePushBtn();
        pBtn->SetMessage(ZMessage("dlg_ok", this));
        pBtn->mCaption.sText = "OK";
        pBtn->mCaption.style = gStyleButton;
        arrangeList.push_back(pBtn);

//        pBtn->SetArea(rButton);
        ChildAdd(pBtn);
    }

    if (!arrangeList.empty())
        ArrangeWindows(arrangeList, rButtonArea, gStyleButton, -1, 1);

    mTransformIn = kFade;
    mTransformOut = kFade;

    ZWin::Init();
    SetFocus();


    return true;
}

bool ZWinDialog::Shutdown()
{
    return ZWin::Shutdown();
}


bool ZWinDialog::Paint()
{
    uint32_t cols[4];
    cols[0] = 0xff444444;
    cols[1] = 0xff888888;
    cols[2] = 0xff444444;
    cols[3] = 0xff888888;


    mpSurface.get()->FillGradient(cols);
    mpSurface.get()->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, mAreaLocal, ZBuffer::kEdgeBltMiddle_None);
    return false;
}

bool ZWinDialog::OnMouseDownL(int64_t x, int64_t y)
{
    if ((mBehavior & eBehavior::Draggable) && SetCapture())
    {
        SetMouseDownPos(x, y);
    }
    return ZWin::OnMouseDownL(x, y);
}

bool ZWinDialog::OnMouseUpL(int64_t x, int64_t y)
{
    ReleaseCapture();
    return ZWin::OnMouseUpL(x, y);
}

bool ZWinDialog::OnMouseMove(int64_t x, int64_t y)
{
    if (AmCapturing())
    {
        WindowToScreenCoordinates(x, y);

        MoveTo(x - mMouseDownOffset.x, y - mMouseDownOffset.y);
    }
    return ZWin::OnMouseMove(x, y);
}

bool ZWinDialog::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();
    if (sType == "dlg_ok")
    {
        OnOK();
        return true;
    }
    else if (sType == "dlg_cancel")
    {
        OnCancel();
        return true;
    }


    return ZWin::HandleMessage(message);
}

bool ZWinDialog::OnKeyDown(uint32_t key)
{
    switch (key)
    {
    case VK_ESCAPE:
        OnCancel();
        return true;
    case VK_RETURN:
        OnOK();
        return true;
    }

    return ZWin::OnKeyDown(key);
}

void ZWinDialog::OnOK()
{
    // TODO, post whatever needed on ok
    if (mpParentWin)
        mpParentWin->SetFocus();
    // post close

    gMessageSystem.Post("kill_child", "name", msWinName);

}

void ZWinDialog::OnCancel()
{
    if (mpParentWin)
        mpParentWin->SetFocus();
    gMessageSystem.Post("kill_child", "name", msWinName);
}




