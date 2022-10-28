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
	virtual	bool		SetImage( ZBuffer* pImage );
	virtual void		SetFont(int64_t nFontID) { mnFontID = nFontID; }

	virtual void		SetText( std::string& text ) { sText = text; }
	virtual bool		SetSrcRect( ZRect rBaseRect );
	virtual bool		SetBaseRect( ZRect rBaseRect );

	virtual void		SetTriggerMessage(const std::string& sMessage)	{ msTriggerMessage = sMessage; }
	virtual void		SetHoverMessage(const std::string& sMessage)		{ msHoverMessage = sMessage; }
	virtual void		SetCursorInMessage(const std::string& sMessage)	{ msCursorInMessage = sMessage; }
	virtual void		SetCursorOutMessage(const std::string& sMessage)	{ msCursorOutMessage = sMessage; }

protected:   
    virtual bool		Paint();

	ZBuffer*			mpButtonImage;
	ZRect				mrBaseRect;
	ZRect				mrSrcRect;
	std::string			sText;
	int64_t				mnFontID;
	IMessageTarget*	    mpMessageTarget;     // if this is NULL, the button posts the message to everyone

	// Event specific messages
	std::string			msTriggerMessage;
	std::string			msHoverMessage;
	std::string			msCursorInMessage;
	std::string			msCursorOutMessage;

	bool				mbWeOwnButtonImage;
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
	virtual bool  Init();
	virtual bool  Shutdown(); 

	virtual bool  OnMouseDownL(int64_t x, int64_t y);
	virtual bool  OnMouseUpL(int64_t x, int64_t y);
	virtual bool  OnMouseMove(int64_t x, int64_t y);
	bool	SetImage( ZBuffer* pImage );

protected:
    virtual bool  Paint();
   
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


	virtual std::string     GetCaption() { return msCaption; }
	virtual void            SetCaption(const std::string& sCaption);

	uint32_t                GetColor() { return mnTextColor; }
	void                    SetColor(uint32_t nCol);

	uint32_t                GetColor2() { return mnTextColor2; }
	void                    SetColor2(uint32_t nCol2);

	ZFont::eStyle           GetStyle() { return mCaptionStyle; }
	void                    SetStyle(ZFont::eStyle style);

	void                    SetFontID(int64_t nFontID);
	int64_t                 GetFontID() { return mnFontID; }

	virtual void            SetMessage(std::string sMessage) { msButtonMessage = sMessage; }
	virtual std::string     GetMessage() { return msButtonMessage; }

	void                    SetImages(ZBuffer* pUpEdgeImage, ZBuffer* pDownEdgeImage, ZRect rUpEdge );  

protected:
    virtual bool            Paint();
    std::string             FitCaptionInWidth(int64_t nWidth, const std::string& sCaption, ZFont* pFont);		// Calculates an equal number of starting/ending chars with "..." in the middle

	eSizablePushBtnState	mDrawState;
	std::string             msCaption;

	uint32_t                mnTextColor;
	uint32_t                mnTextColor2;
	ZFont::eStyle           mCaptionStyle;

	ZBuffer*                mpUpEdgeImage;              // not owned by this class
	ZBuffer*                mpDownEdgeImage;            // not owned by this class
	ZRect                   mrEdge;

	int64_t                 mnFontID;

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

public:  // cCEWinCheck ///////////////////////////////////////////////////////

	bool					GetState() { return *mpbChecked; }
	void					SetState(bool bChecked, bool bPostNotifications = true);
	void					SetMessages(const std::string& sCheckMessage, const std::string sUncheckMessage) { msButtonMessage = sCheckMessage; msUncheckMessage = sUncheckMessage; }

protected:
    virtual bool  			Paint();
   
    bool*					mpbChecked;
	std::string				msUncheckMessage;
};


