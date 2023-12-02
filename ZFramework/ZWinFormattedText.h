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

    tZBufferPtr         Buffer();       // lazy loads buffer with filename in text member

    tEntry              type;
    std::string         text;

    ZGUI::Style         style;
    std::string         link;
    tZBufferPtr         pBuffer;
};

typedef std::list<FormattedEntry>	tFormattedLine;
typedef std::list<tFormattedLine>	tDocument;

class ZWinFormattedDoc : public ZWin
{
public:
    enum eBehavior : uint32_t
    {
        kNone                   = 0,
        kBackgroundFill         = 1,
        kBackgroundEdgeBlt      = 2,
        kBackgroundFromParent   = 4,
        kDrawBorder             = 8,
        kUnderlineLinks         = 16,
        kEvenColumns            = 32,
        kScrollable             = 64
    };


	ZWinFormattedDoc();
	~ZWinFormattedDoc();

	virtual bool		Init();
	virtual bool		InitFromXML(ZXMLNode* pNode);
	virtual bool 		Paint();

	virtual void		Clear();

    virtual void		AddLineNode(std::string sLine);
    virtual void		AddMultiLine(std::string sLine, ZGUI::Style style = {}, const std::string& sLink = "");
    int64_t   			GetFullDocumentHeight();

    void                SetScrollable(bool enable = true) 
    { 
        Set(kScrollable, enable);
        mbAcceptsCursorMessages |= enable; 
        UpdateScrollbar(); 
    }
    void				ScrollTo(int64_t nSliderValue);		 // normalized 0.0 to 1.0
	void				UpdateScrollbar();					// Creates a scrollbar if one is needed

    void                Set(uint32_t flag, bool set)
    {
        if (set)
            mBehavior |= flag;
        else
            mBehavior &= ~flag;
    }

    bool                IsSet(uint32_t flag) { return (mBehavior & flag) > 0; }

	virtual void		SetArea(const ZRect& newArea);
	virtual bool		OnMouseDownL(int64_t x, int64_t y);
	virtual bool		OnMouseUpL(int64_t x, int64_t y);
	virtual bool		OnMouseMove(int64_t x, int64_t y);
	virtual bool		OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);

/*#ifdef _DEBUG
	virtual  bool		OnMouseMove(int64_t x, int64_t y);
#endif*/


/*    bool  				mbDrawBorder;
    bool				mbUnderlineLinks;
    bool                mbEvenColumns;*/
    uint32_t            mBehavior;
    int64_t				mnScrollToOnInit;
    ZGUI::Style         mStyle;

private:

	void				CalculateFullDocumentHeight();
	int64_t   			GetLineHeight(tFormattedLine& line);     // returns the font height of the largest font on the line

	bool 				ParseDocument(ZXMLNode* pNode);
	bool 				ProcessLineNode(ZXMLNode* pLineNode);
	void				ExtractParameters(ZXMLNode* pNode);
    void                ComputeColumnWidths();


	ZWinSlider*         mpWinSlider;
	int64_t				mnSliderVal;

	ZRect        		mrDocumentArea;
	ZRect        		mrDocumentBorderArea;

	tDocument    		mDocument;     // parsed document
    std::mutex          mDocumentMutex;		// when held, document may not be modified


	// Storage for text parameters while parsing the document

	int64_t				mnMouseDownSliderVal;
	double				mfMouseMomentum;
//    bool				mbScrollable;

    std::vector<int64_t>mColumnWidths;

	int64_t				mnFullDocumentHeight;

};
