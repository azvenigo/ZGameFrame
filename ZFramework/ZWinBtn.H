/////////////////////////////////////////////////////////////////////////////
// class cBtn

#pragma once

#include "ZWin.h"
#include <string>
#include "ZMessageSystem.h"
#include "ZFont.h"
#include <list>

class ZBuffer;
class ZFont;

class ZWinBtn : public ZWin
{
public:
	ZWinBtn();
	~ZWinBtn();

public:
	virtual bool		Init();
	virtual bool		Shutdown(); 

	virtual bool		OnMouseHover(int64_t x, int64_t y);
	virtual bool		OnMouseIn();
	virtual bool		OnMouseOut();

public:
	virtual	bool		SetImage(tZBufferPtr pImage );
	virtual void		SetFont(tZFontPtr pFont) { mpFont = pFont; }

	virtual void		SetText( std::string& text ) { sText = text; }
	virtual bool		SetSrcRect( ZRect rBaseRect );
	virtual bool		SetBaseRect( ZRect rBaseRect );

	virtual void		SetTriggerMessage(const std::string& sMessage)	{ msTriggerMessage = sMessage; }
	virtual void		SetHoverMessage(const std::string& sMessage)		{ msHoverMessage = sMessage; }
	virtual void		SetCursorInMessage(const std::string& sMessage)	{ msCursorInMessage = sMessage; }
	virtual void		SetCursorOutMessage(const std::string& sMessage)	{ msCursorOutMessage = sMessage; }

protected:   
    virtual bool		Paint();

    tZBufferPtr     mpButtonImage;
	ZRect			mrBaseRect;
	ZRect			mrSrcRect;
	std::string		sText;
	tZFontPtr       mpFont;
	IMessageTarget*	mpMessageTarget;     // if this is NULL, the button posts the message to everyone

	// Event specific messages
	std::string		msTriggerMessage;
	std::string		msHoverMessage;
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


	virtual void            SetCaption(const std::string& sCaption);
    std::string             GetCaption() { return msCaption; };
    void                    SetColors(uint32_t nColTop, uint32_t nColBottom);
	void                    SetStyle(ZFont::eStyle style);
    virtual void		    SetFont(tZFontPtr pFont) { mpFont = pFont; }
    tZFontPtr  GetFont()    { return mpFont; }

	virtual void            SetMessage(std::string sMessage) { msButtonMessage = sMessage; }
	virtual std::string     GetMessage() { return msButtonMessage; }

	void                    SetImages(tZBufferPtr pUpEdgeImage, tZBufferPtr pDownEdgeImage, ZRect rUpEdge );

protected:
    virtual bool            Paint();
    std::string             FitCaptionInWidth(int64_t nWidth, const std::string& sCaption, tZFontPtr pFont);		// Calculates an equal number of starting/ending chars with "..." in the middle

	eSizablePushBtnState	mDrawState;
	std::string             msCaption;

	uint32_t                mnTextColorTop;
	uint32_t                mnTextColorBottom;
	ZFont::eStyle           mCaptionStyle;

    tZBufferPtr             mpUpEdgeImage;              // not owned by this class
    tZBufferPtr             mpDownEdgeImage;            // not owned by this class
	ZRect                   mrEdge;

    tZFontPtr               mpFont;

	std::string             msButtonMessage;
};


///////////////////////////////////////////////////////////////////////////////
//Check box button
///////////////////////////////////////////////////////////////////////////////
class ZWinCheck : public ZWinSizablePushBtn
{
public:
	ZWinCheck(bool* pbChecked);
	~ZWinCheck();

public:
	virtual bool  			Init(void);
	virtual bool			OnMouseDownL(int64_t x, int64_t y);
	virtual bool			OnMouseUpL(int64_t x, int64_t y);

	// IMessageTarget
	virtual bool			HandleMessage(const ZMessage& message);

    void                    SetCheckedColors(uint32_t nUncheckedColor, uint32_t nCheckedColor) { mnUncheckedColor = nUncheckedColor; mnCheckedColor = nCheckedColor; }
public:  // ZWinCheck ///////////////////////////////////////////////////////

	bool					GetState() { return *mpbChecked; }
	void					SetState(bool bChecked, bool bPostNotifications = true);
	void					SetMessages(const std::string& sCheckMessage, const std::string sUncheckMessage) { msButtonMessage = sCheckMessage; msUncheckMessage = sUncheckMessage; }
    void                    SetRadioGroup(const std::string& sRadioGroupName); // If part of a radio group, when any other in the same group is checked, automatically unchecks

protected:
    virtual bool  			Paint();
   
    bool*					mpbChecked;
	std::string				msUncheckMessage;
    std::string             msRadioGroupName;
    uint32_t                mnCheckedColor;
    uint32_t                mnUncheckedColor;
};


