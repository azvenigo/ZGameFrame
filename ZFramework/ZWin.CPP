#include "ZWin.H"
#include "ZBuffer.h"
#include "ZStdDebug.h"
#include "ZGraphicSystem.h"
#include "helpers/StringHelpers.h"
#include "ZAnimObjects.h"
#include "ZAnimator.h"
#include "ZXMLNode.h"
#include "ZTimer.h"
#include <iostream>
#include "ZScreenBuffer.h"

extern ZAnimator		gAnimator;
extern ZTimer			gTimer;
extern ZPoint			gLastMouseMove;

ZWin* ZWin::mpFocusWin	= NULL;
extern bool gbApplicationExiting;

const int64_t kDefaultTransformTime = 500;
const int64_t kDefaultIdleTime = 32;

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
using namespace std;

ZWin::ZWin() : mArea(0,0,0,0),
mAreaToDrawTo(0,0,0,0),
mAreaAbsolute(0,0,0,0),
mAreaToBltTo(0,0,0,0),
mMouseDownOffset(0,0),
mpParentWin(0),
mbInitted(false),
mbVisible(true),
mbAcceptsCursorMessages(false),
mbAcceptsFocus(false),
mbInvalid(true),
mbInvalidateParentWhenInvalid(false),
//mbTransformable(false),
mTransformIn(kNone),
mTransformOut(kNone),
mnTransformInTime(kDefaultTransformTime),
mnTransformOutTime(kDefaultTransformTime),
mbShutdownFlag(false),
mIdleSleepMS(kDefaultIdleTime)
{
    Sprintf(msWinName, "win_%x", this); // default to unique window name
}

ZWin::~ZWin() 
{
	gAnimator.KillContextObjects(this);

	if (mpFocusWin == this)
		mpFocusWin = NULL;

	if (gpCaptureWin == this)
		gpCaptureWin = NULL;
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

		if (mArea.Width() > 0 && mArea.Height() > 0)
		{
			ZTransformable::Init(mAreaAbsolute);
			TransformIn();
		}

		gMessageSystem.RegisterTarget(this);

        Paint();        // TBD this may be desired since our texture may be requested before the threadproc has a chance to call paint

		mbShutdownFlag = false;
		mThread = std::thread(&WindowThreadProc, (void*)this);

		mbInvalid = true;
		mbInitted = true;
	}
	return true;
}  

bool ZWin::Shutdown()
{
	if (mbInitted)
	{
		mbShutdownFlag = true;
		mThread.join();

		const std::lock_guard<std::mutex> lock(mShutdownMutex);	// prevent shutdown until this returns
		TransformOut();

		gMessageSystem.RemoveAllNotifications(this);

		if (!msWinName.empty())
			gMessageSystem.UnregisterTarget(this);

        ChildDeleteAll();

		if (gpMouseOverWin == this)
			gpMouseOverWin = NULL;

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
		ZTransformation start(ZPoint(mAreaAbsolute.left, mAreaAbsolute.top), 1.0, 0, 255);
		ZTransformation end(ZPoint(mAreaAbsolute.left, mAreaAbsolute.top), 1.0, 0, 255);

		switch (mTransformIn)
		{
		case kFade:			start.mnAlpha = 0;					break;
		case kSlideLeft:	start.mPosition.mX += mArea.Width();		break;
		case kSlideRight:	start.mPosition.mX -= mArea.Width();		break;
		case kSlideUp:		start.mPosition.mY += mArea.Height();		break;
		case kSlideDown:	start.mPosition.mY -= mArea.Height();		break;
		}


		StartTransformation(start);
		AddTransformation(end, mnTransformInTime);
	}
}

void ZWin::TransformOut()
{
	if (mTransformOut != kNone)
	{
		ZAnimObject_TransformingImage* pImage = new ZAnimObject_TransformingImage(mpTransformTexture.get());
		ZTransformation trans(GetTransform());

		pImage->StartTransformation(trans);

		switch (mTransformOut)
		{
		case kFade:			trans.mnAlpha = 0;					break;
		case kSlideLeft:	trans.mPosition.mX -= mArea.Width();		break;
		case kSlideRight:	trans.mPosition.mX += mArea.Width();		break;
		case kSlideUp:		trans.mPosition.mY -= mArea.Height();		break;
		case kSlideDown:	trans.mPosition.mY += mArea.Height();		break;
		}

		pImage->AddTransformation(trans,mnTransformOutTime);

		gAnimator.AddObject(pImage);
	}
}


void ZWin::SetWinName(const string& sWinName) 
{
	ZASSERT_MESSAGE(!mbInitted, "Window name must be set before init for message target registration.");
	msWinName = sWinName; 
}


void  ZWin::CalcAbsoluteArea() 
{
	ZRect tempArea( mArea );
	ZWin* currParent = mpParentWin;

	while (currParent) 
	{
		tempArea.OffsetRect( currParent->mArea.left, currParent->mArea.top ); 
		currParent = currParent->GetParentWin();     
	}

	mAreaAbsolute = tempArea;

    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    for (tWinList::iterator it = mChildList.begin(); it != mChildList.end(); it++)
	{
		(*it)->CalcAbsoluteArea();
	}   

	gpGraphicSystem->GetScreenBuffer()->SetVisibilityComputingFlag(true);
}

void ZWin::CenterWindowInRect(const ZRect &outerRect) 
{
	const int64_t tempX = outerRect.left + ( (outerRect.Width() - mArea.Width() ) / 2 );
	const int64_t tempY = outerRect.top  + ( (outerRect.Height()- mArea.Height() ) / 2 );
	SetArea( tempX, tempY, tempX + mArea.Width(), tempY + mArea.Height());
}

bool  ZWin::SetParentWin(ZWin* pNewParent) 
{
	mpParentWin = pNewParent;
	return true;
}

bool ZWin::ChildAdd(ZWin* child)
{
	ZASSERT(child);
//	ZDEBUG_OUT("ChildAdd:%d %s\n", (uint32_t) child, child->GetFullWinName().c_str());

	child->SetParentWin(this);

    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    ZASSERT_MESSAGE(find(mChildList.begin(), mChildList.end(), child) == mChildList.end(), "Child already added!");
    mChildList.push_front(child);

	child->CalcAbsoluteArea();
	child->SetAreaToDrawTo();
//	child->SetBufferToDrawTo();

	if (!child->Init())
	{
		ZASSERT(false);
		return false;
	}

	child->Show();

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

			child->SetParentWin(NULL);
			return true;
		}
	}   
	return false;
}

void ZWin::ChildDeleteAll() 
{
    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);

	for (auto pChild : mChildList)		// signal all to shutdown first
		pChild->SignalShutdown();

	while (!mChildList.empty())			// ChildDelete modifies the list so we can't use regular iteration
		ChildDelete(*mChildList.begin());

	mChildList.clear();
}

bool ZWin::ChildDelete(ZWin* pChild) 
{
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
		if (mpParentWin)
			mpParentWin->ChildToFront(this);

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
		if ((*it)->GetWinName() == sWinName)
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

bool ZWin::SetFocus()
{
//    ZDEBUG_OUT("SetFocus: mWorkToDoCV\n");
    mWorkToDoCV.notify_one();
    if (mbAcceptsFocus)
	{
		mpFocusWin = this;
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
		nX = StringHelpers::ToInt(message.GetParam("x"));
	int64_t nY = 0;
	if (message.HasParam("y"))
		nY = StringHelpers::ToInt(message.GetParam("y"));

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
			int64_t nDelta = StringHelpers::ToInt(message.GetParam("delta"));
			return OnMouseWheel(nX, nY, nDelta);
		}
		else if (sSubType == "hover")
		{
			return OnMouseHover(nX, nY);
		}
	}

	return false;
}

bool  ZWin::Paint() 
{
    if (!mbInitted || !mbInvalid)
        return false;

    mpTransformTexture.get()->SetRenderFlag(ZBuffer::kRenderFlag_ReadyToRender);
	mbInvalid = false;
	return true;
}               


bool ZWin::ComputeVisibility()
{
	if (!mbVisible)
		return true;

//	if (mbVisible)
	{
		ZScreenRect sr(mpTransformTexture, mAreaAbsolute, ZPoint(0, 0));
        gpGraphicSystem->GetScreenBuffer()->AddScreenRectAndComputeVisibility(sr);
	}

    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    for (tWinList::reverse_iterator it = mChildList.rbegin(); it != mChildList.rend(); it++)
	{
		ZWin* pWin = *it;

		//ZDEBUG_OUT("ChildPaint:0x%x\n", uint32_t(pWin));

		pWin->ComputeVisibility();
	}

	return true;
}



void ZWin::SetAreaToDrawTo() 
{
	mAreaToDrawTo.SetRect(0, 0, mArea.Width(), mArea.Height() );

	if (mpTransformTexture.get())
	{
		ZWin* pCurrParent = mpParentWin;
		mAreaToBltTo = mArea;
		while (pCurrParent) 
		{
			if (pCurrParent->mpTransformTexture.get())
				break;

			mAreaToBltTo.OffsetRect(pCurrParent->mArea.left, pCurrParent->mArea.top);

			pCurrParent = pCurrParent->GetParentWin();
		}
	} 
}

void ZWin::SetAreaToDrawToRecursive() 
{
	SetAreaToDrawTo();
    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    tWinList::iterator it(mChildList.begin());
	while(it != mChildList.end())
	{
		(*it)->SetAreaToDrawToRecursive();
		it++;
	}
}
bool ZWin::SetCapture()
{
	if (gpCaptureWin)
		return false;
	gpCaptureWin = this;
	return true;
}

bool ZWin::ReleaseCapture()
{
	if (gpCaptureWin != this)
		return false;
	gpCaptureWin = NULL;
	return true;
}

bool ZWin::AmCapturing()
{
	return (gpCaptureWin == this);
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

const ZPoint& ZWin::GetLastMouseMove()
{
	return gLastMouseMove;
}

string ZWin::GetFullWinName()
{
	string sFullName(msWinName);

	// Walk up all parents and prepend their name
	ZWin* pParent = mpParentWin;
	while (pParent)
	{
		string sCur = sFullName;
		sFullName = pParent->GetWinName();
		sFullName += string(".");
		sFullName += sCur;

		pParent = pParent->GetParentWin();
	}

	return sFullName;
}

bool ZWin::ReceiveMessage(const ZMessage& message)
{
	const std::lock_guard<std::mutex> lock(mMessageQueueMutex);

	mMessages.push_back(std::move(message));

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
	else if (sType == "kill_window")
	{
		if (message.GetParam("target") == GetTargetName())
		{
			ZASSERT(mpParentWin);
			mpParentWin->ChildDelete(this);
			return true;
		}
	}

#ifdef _DEBUG
	// If the message is targeted, this had better be the target window
	if (message.HasTarget() && message.GetTarget() == GetTargetName())
	{
		ZASSERT_MESSAGE(false, string(string("Targeted message:\"") + string(message.ToString()) + string("\" was not handled!")).c_str());
	}
#endif

	return false;
}

void ZWin::Show()
{
    if (!mbVisible)
    {
        mbVisible = true;
        Invalidate();
        gpGraphicSystem->GetScreenBuffer()->SetVisibilityComputingFlag(true);
    }
}

void ZWin::Hide()
{
    if (mbVisible)
    {
        mbVisible = false;
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
    for (auto pChild : mChildList)		// signal all to shutdown first
        pChild->InvalidateChildren();
}


void ZWin::SetArea(int64_t l,int64_t t,int64_t r,int64_t b) 
{
	mArea.SetRect(l, t, r, b);

	//	if (mpBuffer && ( (r - l) != mArea.Width()) || ((b - t)!= mArea.Height()))       
	//		PrivateBuffer(true);
//	if (mbTransformable)
		ZTransformable::Init(mArea);
	SetAreaToDrawToRecursive();
	CalcAbsoluteArea();
}

void ZWin::SetSize( int64_t width, int64_t height ) 
{
	SetArea( mArea.left, mArea.top, mArea.left + width, mArea.top + height );
}

void ZWin::SetArea( const ZRect& area ) 
{
	SetArea(area.left, area.top, area.right, area.bottom);
}

void  ZWin::MoveTo(int64_t newX, int64_t newY) 
{
	SetArea(newX, newY, newX+mArea.Width(), newY+mArea.Height());
}

void  ZWin::OffsetTo(int64_t deltaX, int64_t deltaY) 
{
	SetArea( mArea.left + deltaX, mArea.top + deltaY, mArea.right + deltaX, mArea.bottom + deltaY );
}


bool ZWin::WindowThreadProc(void* pContext)
{
	ZWin* pThis = (ZWin*)pContext;

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

        bool bActive = pThis->Process();
        bActive |= pThis->Paint();

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

