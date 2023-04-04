#pragma once

#include <list>
#include "ZTypes.h"
#include "ZMessageSystem.h"
#include "ZTransformable.h"
#include "ZGUIHelpers.h"
#include "ZGUIStyle.h"
#include <mutex>
#ifdef _WIN64
#include "windows.h"		// Virtual Key Defs
#endif


// Forward decl
class ZWin;
class ZBuffer;
class ZXMLNode;
class ZScreenBuffer;

typedef std::list<ZWin*> tWinList;

extern ZWin* gpCaptureWin;
extern ZWin* gpMouseOverWin;
extern ZPoint gLastMouseMove;


////////////////////////////////////////////////////
// class ZWin
class ZWin : public IMessageTarget, public ZTransformable
{
public:
	enum eTransformType
	{
		kNone 		= 0,
		kFade 		= 1,
		kSlideLeft	= 2,
		kSlideRight = 3,
		kSlideUp	= 4,
		kSlideDown	= 5
	};

	ZWin();
	virtual ~ZWin();
	
	virtual	bool		Init();
	virtual	bool		InitFromXML(ZXMLNode* pNode);
	virtual	bool		Shutdown();
	virtual void		SignalShutdown();
	
public:   
    virtual ZWin*       GetTopWindow();
	virtual	ZWin*		GetParentWin() const { return mpParentWin; }
	virtual	bool		SetParentWin(ZWin*);
	
	virtual	bool		ChildAdd(ZWin* child, bool bAutoShow = true);
	virtual	bool		ChildRemove(ZWin* child);
	virtual	bool		ChildDelete(ZWin*	child);
	virtual	void		ChildDeleteAll();
	
	virtual	bool		ChildExists(ZWin* child);
	
	virtual	bool		ChildToFront(ZWin* child); 
	virtual	bool		ChildToBack(ZWin* child);  
	
public:

    virtual void		SetArea(const ZRect& newArea);
    const ZRect&        GetArea() const { return mArea; }
	
//	virtual void		SetSize(int64_t width, int64_t height);
//	virtual void		SetArea(int64_t l,int64_t t,int64_t r,int64_t b);
	
	virtual void		Arrange(ZGUI::ePosition pos, const ZRect &outerRect, int64_t padding = 0);
	
	virtual void		MoveTo(int64_t newX, int64_t newY);
//	virtual void		OffsetTo(int64_t deltaX, int64_t deltaY);
	
	virtual void		ScreenToWindowCoordinates(int64_t& x, int64_t& y);
	virtual void		WindowToScreenCoordinates(int64_t& x, int64_t& y);
	
	virtual bool		IsPointInWindowScreenCoordinates(int64_t screenX, int64_t screenY);
	virtual bool		IsPointInWindowWindowCoordinates(int64_t x, int64_t y);
	
	virtual void		SetMouseDownPos(int64_t mouseX,  int64_t mouseY);
	virtual void		GetMouseDownPos(int64_t& mouseX, int64_t& mouseY);
	
	virtual void		CalcAbsoluteArea();
	
	// IMessageTarget
	virtual bool		ReceiveMessage(const ZMessage& message);
	virtual std::string GetTargetName() { return msWinName; }						// returns a unique target identifier

public:
	virtual void		SetVisible(bool bVisible = true);
    virtual bool		IsVisible() { return mbVisible; }
	
    std::string&        GetWinName() { return msWinName; }
    std::string         GetFullWinName();		// returns the fully qualified name all the way up the parental chain.  Example: A.B.C.D or main.quitdialog.ok
	void				SetWinName(const std::string& sWinName);
	const ZPoint&		GetLastMouseMove();
	
	ZWin*				GetChildWindowByWinName(const std::string& sWinName);
	ZWin*				GetChildWindowByWinNameRecursive(const std::string& sWinName);
	ZWin*				GetChildWindowByPoint( int64_t x, int64_t y );
	
public:   
	virtual bool		ProcessCursorMessage(const ZMessage& message);
	virtual bool		OnChar(char c);
	virtual bool		OnKeyDown(uint32_t key);
	virtual bool		OnKeyUp(uint32_t key);
	
	virtual bool		OnMouseDownL(int64_t x, int64_t y);
	virtual bool		OnMouseUpL(int64_t x, int64_t y);
	
	virtual bool		OnMouseDownR(int64_t x, int64_t y);
	virtual bool		OnMouseUpR(int64_t x, int64_t y);
	
	virtual bool		OnMouseMove(int64_t x, int64_t y);

	virtual bool		OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);
	virtual bool		OnMouseHover(int64_t x, int64_t y);

	virtual bool		OnMouseIn();
	virtual bool		OnMouseOut();
	
	bool				SetFocus();
	ZWin*				GetFocus() { return mpFocusWin; }

	virtual bool		SetCapture();
	virtual bool		ReleaseCapture();
	virtual bool		AmCapturing();
	
public:
	virtual void		Invalidate();		// sets invalid flag used for windows that may not require redrawing every frame
    virtual void        InvalidateChildren();

	virtual void		SetAreaToDrawTo();
	virtual void		SetAreaToDrawToRecursive();
	
	virtual bool		ComputeVisibility();		// Add my own area to ZScreenBuffer's ScreenRect container, then call all children (that will be on top of me) recursively to do the same

protected:
    virtual bool        Process();              // for doing any computation/simulation/etc. Returns true if anything was done
	virtual bool		Paint();
	virtual void		TransformIn();
	virtual void		TransformOut();


	// Window Threading
	static bool			    WindowThreadProc(void* pContext);
	std::thread			    mThread;
	std::mutex			    mMessageQueueMutex;
	std::mutex			    mShutdownMutex;		// when held, this window is not allowed to shut down
    std::recursive_mutex    mChildListMutex;
	bool				    mbShutdownFlag;
    int64_t                 mIdleSleepMS;
    std::condition_variable mWorkToDoCV;


	virtual bool		    HandleMessage(const ZMessage& message);



protected:

    std::string						msWinName;
	ZRect   						mAreaAbsolute;
	ZRect   						mAreaToDrawTo;
	ZPoint  						mMouseDownOffset;
	tWinList						mChildList;
	ZWin*   						mpParentWin;
	bool  							mbInvalid;
    bool                            mbInvalidateParentWhenInvalid;
	ZRect							mArea;
	ZRect							mAreaToBltTo;
	bool							mbInitted;

	tMessageList					mMessages;

public:
	// Flags
	bool							mbVisible;
	bool							mbAcceptsCursorMessages;
	bool							mbTransformable;
	eTransformType					mTransformIn;
	eTransformType					mTransformOut;
	int64_t							mnTransformInTime;
	int64_t							mnTransformOutTime;
	bool							mbAcceptsFocus;

private:
	static ZWin*					mpFocusWin;	
	static ZWin*					mpMouseOverWin;
};




class ZWinDialog : public ZWin
{
public:
    enum eBehavior : uint8_t
    {
        None = 0,
        Draggable           = 1,        // 1   
        OKButton            = 1 << 1,   // 2
        CancelButton        = 1 << 2,   // 4
    };

    ZWinDialog();

    virtual	bool		Init();
    virtual bool		OnMouseDownL(int64_t x, int64_t y);
    virtual bool		OnMouseUpL(int64_t x, int64_t y);
    virtual bool		OnMouseMove(int64_t x, int64_t y);
    virtual bool        HandleMessage(const ZMessage& message);

    virtual bool		OnKeyDown(uint32_t key);

protected:
    virtual bool		Paint();
    virtual void        OnOK();
    virtual void        OnCancel();

//    virtual bool        Process();


public:
    uint8_t     mBehavior;
    ZGUI::Style mStyle;
};




inline
void ZWin::WindowToScreenCoordinates(int64_t& nXPosition, int64_t& nYPosition)
{
    ZWin* pWin = this;
    while(pWin)
    {
        nXPosition += pWin->GetArea().left;
        nYPosition += pWin->GetArea().top;
        pWin = pWin->GetParentWin();
    }
}

inline
void ZWin::ScreenToWindowCoordinates(int64_t& x, int64_t& y)
{
    int64_t nXFactor = 0;
    int64_t nYFactor = 0;
    
    WindowToScreenCoordinates(nXFactor, nYFactor);
    x -= nXFactor;
    y -= nYFactor;
}


inline
bool ZWin::IsPointInWindowWindowCoordinates(int64_t x, int64_t y) 
{
	return mArea.PtInRect( x, y );
}

inline
bool ZWin::IsPointInWindowScreenCoordinates(int64_t screenX, int64_t screenY) 
{
	return mAreaAbsolute.PtInRect(screenX, screenY);
}

inline
void ZWin::GetMouseDownPos(int64_t& mouseX, int64_t& mouseY) 
{
    mouseX = mMouseDownOffset.x;
    mouseY = mMouseDownOffset.y;
}

inline
void ZWin::SetMouseDownPos(int64_t mouseX, int64_t mouseY) {   
	mMouseDownOffset.x = mouseX;
	mMouseDownOffset.y = mouseY;
}

