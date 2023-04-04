#pragma once

#include "ZWin.h"
#include <string>
#include "ZFont.h"
#include "ZBuffer.h"
#include <list>

class ZWinSlider;
class ZFont;

struct sTextEntry
{
    std::string         sText;
	ZFontParams         fontParams;
    ZTextLook           look;
    ZGUI::ePosition     pos;
    std::string         sLink;
};

typedef std::list<sTextEntry>	tTextLine;
typedef std::list<tTextLine>	tDocument;

class ZWinFormattedText : public ZWin
{
public:
	ZWinFormattedText();
	~ZWinFormattedText();

	virtual bool		Init();
	virtual bool		InitFromXML(ZXMLNode* pNode);
	virtual bool 		Paint();

    virtual void		SetScrollable(bool bScrollable = true);
    virtual void        SetDrawBorder(bool bDraw = true) { mbDrawBorder = bDraw; }
    virtual void        SetUnderlineLinks(bool bUnderline = true) { mbUnderlineLinks = bUnderline; }
	virtual void		Clear();
    virtual void        SetFill(uint32_t nCol, bool bEnable = true) { mnFillColor = nCol; mbFillBackground = bEnable && ARGB_A(mnFillColor) > 5; }

    virtual void		AddLineNode(std::string sLine);
    virtual void		AddMultiLine(std::string sLine, ZFontParams fontParams, const ZTextLook& look = {}, ZGUI::ePosition = ZGUI::LB, const std::string& sLink = "");
    int64_t   			GetFullDocumentHeight() { return mnFullDocumentHeight; }

	void				ScrollTo(int64_t nSliderValue);		 // normalized 0.0 to 1.0
	void				UpdateScrollbar();					// Creates a scrollbar if one is needed


	virtual void		SetArea(const ZRect& newArea);
	virtual bool		OnMouseDownL(int64_t x, int64_t y);
	virtual bool		OnMouseUpL(int64_t x, int64_t y);
	virtual bool		OnMouseMove(int64_t x, int64_t y);
	virtual bool		OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);

/*#ifdef _DEBUG
	virtual  bool		OnMouseMove(int64_t x, int64_t y);
#endif*/

private:

	void				CalculateFullDocumentHeight();
	int64_t   			GetLineHeight(tTextLine& textLine);     // returns the font height of the largest font on the line

	bool 				ParseDocument(ZXMLNode* pNode);
	bool 				ProcessLineNode(ZXMLNode* pLineNode);
	void				ExtractTextParameters(ZXMLNode* pTextNode);


	ZWinSlider*         mpWinSlider;
	int64_t				mnSliderVal;
	//cCEFont*     		mpFont[3];

	ZRect        		mrTextArea;
	ZRect        		mrTextBorderArea;

	tDocument    		mDocument;     // parsed document
    std::mutex          mDocumentMutex;		// when held, document may not be modified


	// Storage for text parameters while parsing the document
	ZFontParams		    mCurrentFont;
    ZTextLook           mCurrentLook;
	ZGUI::ePosition	    mCurrentTextPosition;
	bool				mbScrollable;
    std::string         msLink;

	int64_t				mnMouseDownSliderVal;
	double				mfMouseMomentum;

	bool  				mbDrawBorder;
	bool				mbUnderlineLinks;
	bool  				mbFillBackground;
	uint32_t			mnFillColor;

	int64_t				mnFullDocumentHeight;

	int64_t				mnScrollToOnInit;
};