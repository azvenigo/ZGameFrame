/////////////////////////////////////////////////////////////////////////////
// class cBtn

#pragma once

#include "ZWin.h"
#include <string>
#include "ZMessageSystem.h"
#include "ZFont.h"
#include "ZGUIStyle.h"
#include "ZGUIElements.h"
#include <list>

class ZBuffer;
class ZFont;

/*class ZWinBtn : public ZWin
{
public:
	ZWinBtn();
	~ZWinBtn();

public:
	virtual bool    Init();
	virtual bool    Shutdown(); 

	virtual bool    OnMouseHover(int64_t x, int64_t y);
	virtual bool    OnMouseIn();
	virtual bool    OnMouseOut();

public:
	virtual	bool    SetImage(tZBufferPtr pImage );
    virtual void    SetFont(tZFontPtr pFont, uint32_t nCol1, uint32_t nCol2) { mpFont = pFont; mnTextColorTop = nCol1; mnTextColorBottom = nCol2; }

	virtual void    SetTriggerMessage(const std::string& sMessage)	{ msTriggerMessage = sMessage; }
	virtual void    SetCursorInMessage(const std::string& sMessage)	{ msCursorInMessage = sMessage; }
	virtual void    SetCursorOutMessage(const std::string& sMessage)	{ msCursorOutMessage = sMessage; }

    ZRect			mrBaseRect;
    ZRect			mrSrcRect;
    std::string		sText;
    std::string		msTooltip;

protected:   
    virtual bool    Paint();

    tZBufferPtr     mpButtonImage;
    tZFontPtr       mpFont;
    uint32_t        mnTextColorTop;
    uint32_t        mnTextColorBottom;
    bool            mbEnabled;

	IMessageTarget*	mpMessageTarget;     // if this is NULL, the button posts the message to everyone

	// Event specific messages
	std::string		msTriggerMessage;
	std::string		msCursorInMessage;
	std::string		msCursorOutMessage;
};


///////////////////////////////////////////////////////////////////////////////
// Standard push button
///////////////////////////////////////////////////////////////////////////////
class ZWinPushBtn : public ZWinBtn
{
public:
	enum tPushBtnState
	{
		kUp = 0,
		kDown
	};

public:
	ZWinPushBtn();
	~ZWinPushBtn();

public:
	virtual bool    Init();
	virtual bool    Shutdown(); 

	virtual bool    OnMouseDownL(int64_t x, int64_t y);
	virtual bool    OnMouseUpL(int64_t x, int64_t y);
	virtual bool    OnMouseMove(int64_t x, int64_t y);
	bool            SetImage(tZBufferPtr pImage );

protected:
    virtual bool    Paint();
   
    tPushBtnState	mDrawState;
};

*/

///////////////////////////////////////////////////////////////////////////////
// Sizable push button
///////////////////////////////////////////////////////////////////////////////
class ZWinSizablePushBtn : public ZWin
{
public:
	enum eSizablePushBtnState
	{
		kUp   = 0,
		kDown = 1
	};

public:
	ZWinSizablePushBtn();
	~ZWinSizablePushBtn();

	virtual bool            Init();
	virtual bool            Shutdown();

	virtual bool            InitFromXML(ZXMLNode* pNode);

public:
	virtual bool            OnMouseDownL(int64_t x, int64_t y);
	virtual bool            OnMouseUpL(int64_t x, int64_t y);
	virtual bool            OnMouseMove(int64_t x, int64_t y);

	virtual void            SetMessage(std::string sMessage) { msButtonMessage = sMessage; }
	virtual std::string     GetMessage() { return msButtonMessage; }

    ZGUI::TextBox           mCaption;
    ZGUI::SVGImageBox       mSVGImage;

    ZGUI::Style             mBtnStyle;
    bool                    mbEnabled;


    tZBufferPtr             mpUpEdgeImage;
    tZBufferPtr             mpDownEdgeImage;
    ZRect                   mrEdge;


protected:
    virtual bool            Paint();
    std::string             FitCaptionInWidth(int64_t nWidth, const std::string& sCaption, tZFontPtr pFont);		// Calculates an equal number of starting/ending chars with "..." in the middle

	eSizablePushBtnState	mDrawState;

	std::string             msButtonMessage;
};


///////////////////////////////////////////////////////////////////////////////
//Check box button
///////////////////////////////////////////////////////////////////////////////
class ZWinCheck : public ZWinSizablePushBtn
{
public:
	ZWinCheck(bool* pbChecked = nullptr);
	~ZWinCheck();

public:
	virtual bool  			Init(void);
	virtual bool			OnMouseDownL(int64_t x, int64_t y);
	virtual bool			OnMouseUpL(int64_t x, int64_t y);

	// IMessageTarget
	virtual bool			HandleMessage(const ZMessage& message);

public:  // ZWinCheck ///////////////////////////////////////////////////////

	bool					GetState() { return *mpbChecked; }
	void					SetState(bool bChecked, bool bPostNotifications = true);
	void					SetMessages(const std::string& sCheckMessage, const std::string sUncheckMessage) { msButtonMessage = sCheckMessage; msUncheckMessage = sUncheckMessage; }

    // Style
    ZGUI::Style             mCheckedStyle;
    ZGUI::Style             mUncheckedStyle;
    std::string             msRadioGroup;   // only one in the group shoudl be checked

protected:
    virtual bool  			Paint();
   
    bool*					mpbChecked;
    bool                    mbInternal;     // if control has no external bool, keep it internal
	std::string				msUncheckMessage;
};


