#pragma once

#include "ZWin.h"
#include <string>
#include "ZFont.h"
#include "ZBuffer.h"
#include <list>

class ZWinSlider;
class ZFont;

class FormattedEntry
{
public:
    enum tEntry
    {
        Text = 1,
        Image = 2
    };

    FormattedEntry(tEntry _type = Text, const std::string _text = "", tZBufferPtr _buffer = nullptr, const ZGUI::Style& _style = {}, const std::string _link = "")
    {
        type = _type;
        text = _text;
        pBuffer = _buffer;
        style = _style;
        link = _link;
    }


    tEntry              type;
    std::string         text;
    tZBufferPtr         pBuffer;

    ZGUI::Style         style;
    std::string         link;
};

typedef std::list<FormattedEntry>	tFormattedLine;
typedef std::list<tFormattedLine>	tDocument;

class ZWinFormattedDoc : public ZWin
{
public:
	ZWinFormattedDoc();
	~ZWinFormattedDoc();

	virtual bool		Init();
	virtual bool		InitFromXML(ZXMLNode* pNode);
	virtual bool 		Paint();

    virtual void		SetScrollable(bool bScrollable = true);
    virtual void        SetDrawBorder(bool bDraw = true) { mbDrawBorder = bDraw; }
    virtual void        SetUnderlineLinks(bool bUnderline = true) { mbUnderlineLinks = bUnderline; }
	virtual void		Clear();
    virtual void        SetFill(uint32_t nCol, bool bEnable = true) { mnFillColor = nCol; mbFillBackground = bEnable && ARGB_A(mnFillColor) > 5; }

    virtual void		AddLineNode(std::string sLine);
    virtual void		AddMultiLine(std::string sLine, ZGUI::Style style = {}, const std::string& sLink = "");
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
	int64_t   			GetLineHeight(tFormattedLine& line);     // returns the font height of the largest font on the line

	bool 				ParseDocument(ZXMLNode* pNode);
	bool 				ProcessLineNode(ZXMLNode* pLineNode);
	void				ExtractParameters(ZXMLNode* pNode);


	ZWinSlider*         mpWinSlider;
	int64_t				mnSliderVal;

	ZRect        		mrDocumentArea;
	ZRect        		mrDocumentBorderArea;

	tDocument    		mDocument;     // parsed document
    std::mutex          mDocumentMutex;		// when held, document may not be modified


	// Storage for text parameters while parsing the document
	bool				mbScrollable;

	int64_t				mnMouseDownSliderVal;
	double				mfMouseMomentum;

	bool  				mbDrawBorder;
	bool				mbUnderlineLinks;
	bool  				mbFillBackground;
	uint32_t			mnFillColor;

	int64_t				mnFullDocumentHeight;

	int64_t				mnScrollToOnInit;
};
