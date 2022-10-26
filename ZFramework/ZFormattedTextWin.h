#pragma once

#include "ZWin.h"
#include <string>
#include "ZFont.h"
#include <list>

class ZSliderWin;
class ZFont;

struct sTextEntry
{
	string              sText;
	int64_t             nFontID;
	uint32_t            nColor;
	uint32_t            nColor2;
	ZFont::ePosition    position;
	ZFont::eStyle       style;
	string              sLink;
};

typedef std::list<sTextEntry>	tTextLine;
typedef std::list<tTextLine>	tDocument;

class ZFormattedTextWin : public ZWin
{
public:
	ZFormattedTextWin();
	~ZFormattedTextWin();

	virtual bool		Init();
	virtual bool		InitFromXML(ZXMLNode* pNode);
	virtual bool 		Paint();

	virtual void		SetScrollable(bool bScrollable = true) { mbScrollable = bScrollable; }
	virtual void		Clear();
    virtual void        SetFill(uint32_t nCol, bool bEnable = true) { mnFillColor = nCol; mbFillBackground = bEnable; }
	virtual void		AddTextLine(string sLine, int64_t nFontID, uint32_t nCol1, uint32_t nCol2, ZFont::eStyle style = ZFont::kNormal, ZFont::ePosition = ZFont::kBottomLeft, bool bWrap = true, const string& sLink = "");
	int64_t   			GetFullDocumentHeight() { return mnFullDocumentHeight; }

	void				ScrollTo(int64_t nSliderValue);		 // normalized 0.0 to 1.0
	void				UpdateScrollbar();					// Creates a scrollbar if one is needed


	virtual void		SetArea(int64_t l, int64_t t, int64_t r, int64_t b);
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


	ZSliderWin*			mpSliderWin;
	int64_t				mnSliderVal;
	//cCEFont*     		mpFont[3];

	ZRect        		mrTextArea;
	ZRect        		mrTextBorderArea;

	tDocument    		mDocument;     // parsed document

	// Storage for text parameters while parsing the document
	int64_t				mnCurrentFontID;
	uint32_t			mnCurrentColor;
	uint32_t			mnCurrentColor2;
	ZFont::ePosition	mCurrentTextPosition;
	ZFont::eStyle		mCurrentStyle;
	bool				mbScrollable;
	string              msLink;

	int64_t				mnMouseDownSliderVal;
	double				mfMouseMomentum;

	bool  				mbDrawBorder;
	bool				mbUnderlineLinks;
	bool  				mbFillBackground;
	uint32_t			mnFillColor;

	int64_t				mnFullDocumentHeight;

	int64_t				mnScrollToOnInit;
};
