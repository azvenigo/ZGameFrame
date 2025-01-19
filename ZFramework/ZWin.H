#pragma once

#include <list>
#include "ZTypes.h"
#include "ZMessageSystem.h"
#include "ZTransformable.h"
#include "ZGUIHelpers.h"
#include "ZGUIStyle.h"
#include "ZInput.h"
#include <mutex>
#ifdef _WIN64
#include "windows.h"		// Virtual Key Defs
#endif


// Forward decl
class ZWin;
class ZBuffer;
class ZXML;
class ZScreenBuffer;
class ZMainWin;

typedef std::list<ZWin*> tWinList;

typedef std::map< std::string, tWinList > tGroupNameToWinList;

extern ZMainWin* gpMainWin;

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
		kSlideDown	= 5,
        kSpinCW     = 6,
        kSpinCCW    = 7,
        kToOrFrom   = 8
	};

	ZWin();
	virtual ~ZWin();
	
	virtual	bool        Init();
	virtual	bool        Shutdown();
	virtual void        SignalShutdown();
	
public:   
    virtual ZWin*       GetTopWindow() { return (ZWin*)gpMainWin; }
	virtual	ZWin*       GetParentWin() const { return mpParentWin; }
	virtual	bool        SetParentWin(ZWin*);
	
	virtual	bool        ChildAdd(ZWin* child, bool bAutoShow = true);
	virtual	bool        ChildRemove(ZWin* child);
	virtual	bool        ChildDelete(ZWin* child);
	virtual	void        ChildDeleteAll();
	
	virtual	bool        ChildExists(ZWin* child);
	
	virtual	bool        ChildToFront(ZWin* child); 
	virtual	bool        ChildToBack(ZWin* child);  
	
public:

    virtual void        SetArea(const ZRect& newArea);
    const ZRect&        GetArea() const { return mAreaInParent; }
	
	virtual void        Arrange(ZGUI::ePosition pos, const ZRect &outerRect, int64_t padding = 0);
    static  void        ArrangeWindows(tWinList windows, ZRect rArea, ZGUI::Style& style, int64_t nCols = -1, int64_t nRows = -1);    // -1 means unlocked

	virtual void        MoveTo(int64_t newX, int64_t newY);
	
    // Frame of reference helpers
    virtual ZRect       AbsToLocal(const ZRect& rAbsolute);
    virtual ZRect       LocalToAbs(const ZRect& rLocal);

    virtual ZRect       ParentToLocal(const ZRect& rParent);
    virtual ZRect       LocalToParent(const ZRect& rLocal);

    virtual void        AbsToLocal(int64_t& x, int64_t& y);
    virtual void        LocalToAbs(int64_t& x, int64_t& y);

	
	virtual bool        IsPointInWindowScreenCoordinates(int64_t screenX, int64_t screenY);
	virtual bool        IsPointInWindowWindowCoordinates(int64_t x, int64_t y);
	
	// IMessageTarget
	virtual bool        ReceiveMessage(const ZMessage& message);
	virtual std::string GetTargetName() { return msWinName; }						// returns a unique target identifier

public:
	virtual void        SetVisible(bool bVisible = true);
	
    std::string         GetFullWinName();		// returns the fully qualified name all the way up the parental chain.  Example: A.B.C.D or main.quitdialog.ok
	const ZPoint&       GetLastMouseMove();

    tWinList            GetChildrenInGroup(const std::string& sGroup);
    tGroupNameToWinList GetChildGroups();        // returns a map of group name to child windows in those groups (does not return any children with no groups)
    ZRect               GetBounds(const tWinList& list);


	ZWin*               GetChildWindowByWinName(const std::string& sWinName);
	ZWin*               GetChildWindowByWinNameRecursive(const std::string& sWinName);
	ZWin*               GetChildWindowByPoint( int64_t x, int64_t y );

public:
	virtual bool        ProcessCursorMessage(const ZMessage& message);
	virtual bool        OnChar(char c);
	virtual bool        OnKeyDown(uint32_t key);
	virtual bool        OnKeyUp(uint32_t key);

	virtual bool        OnMouseDownL(int64_t x, int64_t y);
	virtual bool        OnMouseUpL(int64_t x, int64_t y);

	virtual bool        OnMouseDownR(int64_t x, int64_t y);
	virtual bool        OnMouseUpR(int64_t x, int64_t y);

	virtual bool        OnMouseMove(int64_t x, int64_t y);

	virtual bool        OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);
	virtual bool        OnMouseHover(int64_t x, int64_t y);

	virtual bool        OnMouseIn();
	virtual bool        OnMouseOut();

	bool                SetFocus();

	virtual bool        SetCapture();
	virtual bool        ReleaseCapture();
	virtual bool        AmCapturing();
	
public:
	virtual void        Invalidate();		// sets invalid flag used for windows that may not require redrawing every frame
    virtual void        InvalidateChildren();

	virtual void        ComputeAreas();
	
	virtual bool        ComputeVisibility();		// Add my own area to ZScreenBuffer's ScreenRect container, then call all children (that will be on top of me) recursively to do the same

    virtual bool        OnParentAreaChange();       // Called recursively for any window that needs to rearrange itself due to parent area changing
    virtual void        RenderToBuffer(tZBufferPtr pTexture, const ZRect& rAbsSrc, const ZRect& rDst, ZWin* pThis = nullptr); // special function for drawing window and all children to a destination.... not part of normal rendering loop. pThis indicates window requesting paint which should be excluded if in child list
    virtual bool        PaintFromParent();          // copies pixels from parent texture into this one's for transparency support
    virtual void        TransformIn();
    virtual void        TransformOut();
    virtual void        TransformTo(ZTransformation& to, int64_t nDuration);

protected:
    virtual bool        Process();              // for doing any computation/simulation/etc. Returns true if anything was done
	virtual bool        Paint();
    inline  bool        PrePaintCheck()
    {
        return mbInvalid && mbVisible && mbInitted && mbPaints && mpSurface;
    }


	// Window Threading
	static bool             WindowThreadProc(void* pContext);
	std::thread             mThread;
	std::mutex              mMessageQueueMutex;
	std::mutex              mShutdownMutex;		// when held, this window is not allowed to shut down
    std::recursive_mutex    mChildListMutex;
	bool                    mbShutdownFlag;
    std::condition_variable mWorkToDoCV;


	virtual bool            HandleMessage(const ZMessage& message);



protected:
    tZBufferPtr             mpSurface;
	ZPoint                  mMouseDownOffset;
	tWinList                mChildList;
	ZWin*                   mpParentWin;
	bool                    mbInvalid;
    bool                    mbInvalidateParentWhenInvalid;

	tMessageList            mMessages;

public:
    std::string             msWinName;
    std::string             msWinGroup; // optional for organizational purposes

    std::string             msTooltip;
    ZGUI::Style             mTooltipStyle;

    ZRect                   mAreaLocal;
    ZRect                   mAreaInParent;
    ZRect                   mAreaAbsolute;

    
    // Behavior
    bool                    mbInitted;
    bool                    mbVisible;
	bool                    mbAcceptsCursorMessages;

    bool                    mbTransformable;
	eTransformType          mTransformIn;
	eTransformType          mTransformOut;
    ZTransformation         mToOrFrom;
	int64_t                 mnTransformInTime;
	int64_t                 mnTransformOutTime;
    int64_t                 mIdleSleepMS;
    bool                    mbAcceptsFocus;
    bool                    mbPaints;   // whether this window has its own surface and painting or only children
};

typedef std::shared_ptr<ZWin> tZWinPtr;




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

    virtual	bool    Init();
    virtual bool    Shutdown();
    virtual bool    OnMouseDownL(int64_t x, int64_t y);
    virtual bool    OnMouseUpL(int64_t x, int64_t y);
    virtual bool    OnMouseMove(int64_t x, int64_t y);
    virtual bool    HandleMessage(const ZMessage& message);

    virtual bool    OnKeyDown(uint32_t key);

protected:
    virtual bool    Paint();
    virtual void    OnOK();
    virtual void    OnCancel();


public:
    uint8_t     mBehavior;
    std::string msOnOKMessage;
    std::string msPreviousFocusWin;
    ZGUI::Style mStyle;
};




inline
void ZWin::LocalToAbs(int64_t& nXPosition, int64_t& nYPosition)
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
void ZWin::AbsToLocal(int64_t& x, int64_t& y)
{
    int64_t nXFactor = 0;
    int64_t nYFactor = 0;
    
    LocalToAbs(nXFactor, nYFactor);
    x -= nXFactor;
    y -= nYFactor;
}

inline
ZRect ZWin::LocalToAbs(const ZRect& rLocal)
{
    ZRect rAbsolute(rLocal);
    rAbsolute.OffsetRect(mAreaAbsolute.left, mAreaAbsolute.top);
    return rAbsolute;
}


inline
ZRect ZWin::AbsToLocal(const ZRect& rAbsolute)
{
    ZRect rLocal(rAbsolute);
    rLocal.OffsetRect(-mAreaAbsolute.left, -mAreaAbsolute.top);
    return rLocal;
}

inline
ZRect ZWin::ParentToLocal(const ZRect& rParent)
{
    ZRect rLocal(rParent);
    rLocal.OffsetRect(-mAreaInParent.left, -mAreaInParent.top);
    return rLocal;
}

inline
ZRect ZWin::LocalToParent(const ZRect& rLocal)
{
    ZRect rParent(rLocal);
    rParent.OffsetRect(mAreaInParent.left, mAreaInParent.top);
    return rParent;
}


inline
bool ZWin::IsPointInWindowWindowCoordinates(int64_t x, int64_t y) 
{
	return mAreaInParent.PtInRect( x, y );
}

inline
bool ZWin::IsPointInWindowScreenCoordinates(int64_t screenX, int64_t screenY) 
{
	return mAreaAbsolute.PtInRect(screenX, screenY);
}

