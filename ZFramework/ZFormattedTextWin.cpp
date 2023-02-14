#include "ZFormattedTextWin.h"
#include "ZStringHelpers.h"
#include "helpers/StringHelpers.h"
#include "ZMessageSystem.h"
#include "ZSliderWin.h"
#include "ZXMLNode.h"
#include "ZGraphicSystem.h"

using namespace std;

extern shared_ptr<ZBuffer>  gSliderThumbVertical;
extern shared_ptr<ZBuffer>  gSliderBackground;
extern ZRect            	grSliderBgEdge;
extern ZRect				grSliderThumbEdge;
extern shared_ptr <ZBuffer> gDimRectBackground;
extern shared_ptr<ZBuffer>  gDimRectBackground;
extern ZRect				grDimRectEdge;

const int64_t 				kDefaultColor = 0;
const int64_t				kSliderWidth = 20;
const ZFont::ePosition	    kDefaultTextPosition = ZFont::kBottomLeft; // we use bottom left by default in case there are multiple size fonts per text line
const ZFont::eStyle		    kDefaultStyle = ZFont::kNormal;


const string				ksLineTag("line");
const string				ksTextTag("text");
const string				ksCaptionTag("caption");
const string				ksFontParamsTag("fontparams");
const string				ksColorTag("color");
const string				ksColor2Tag("color2");
const string				ksPositionTag("position");
const string				ksStyleTag("style");
const string				ksWrapTag("wrap");
const string				ksLinkTag("link");

const string				ksUnderlineLinksTag("underline_links");
const string				ksTextFillTag("text_background_fill");
const string				ksTextFillColor("text_fill_color");
const string				ksScrollable("text_scrollable");
const string				ksTextEdgeTag("text_edge_blt");
const string				ksTextScrollToBottom("text_scroll_to_bottom");


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ZFormattedTextWin::ZFormattedTextWin()
{
	mpSliderWin = NULL;

	mrTextArea.SetRect(0,0,0,0);
	mrTextBorderArea.SetRect(0,0,0,0);

    mCurrentFont                = ZFontParams("Ariel", 12);
	mnCurrentColor				= kDefaultColor;
	mnCurrentColor2				= kDefaultColor;
	mCurrentTextPosition		= kDefaultTextPosition;       
	mCurrentStyle 				= kDefaultStyle;
	mnFullDocumentHeight		= 0;
	mbScrollable				= false;

	mnScrollToOnInit			= 0;
	mnMouseDownSliderVal		= 0;
	mfMouseMomentum				= 0.0;

	mbDrawBorder				= false;
	mbFillBackground			= false;
	mnFillColor					= 0;

	mbAcceptsCursorMessages = true;

}

ZFormattedTextWin::~ZFormattedTextWin()
{
}

bool ZFormattedTextWin::InitFromXML(ZXMLNode* pNode)
{
	ZASSERT(pNode);

	if (ZWin::InitFromXML(pNode))
	{
		ParseDocument(pNode);
		UpdateScrollbar();

		return true;
	}

	return false;
}

bool ZFormattedTextWin::Init()
{
	if (ZWin::Init())
	{
        mIdleSleepMS = 10000;
		UpdateScrollbar();
	}

	return true;
}


void ZFormattedTextWin::SetArea(const ZRect& newArea)
{
	ZWin::SetArea(newArea);
	UpdateScrollbar();
}

bool ZFormattedTextWin::OnMouseDownL(int64_t x, int64_t y)
{
	ZRect rEdgeSrc(3,5,21,61);

	// Get a local rect
	ZRect rLocalTextBorderArea(mrTextArea);
	rLocalTextBorderArea.InflateRect(6,6);

	ZRect rLocalTextArea(mrTextArea);

	ZRect rClip(rLocalTextArea);

	int64_t nMouseX = x + rLocalTextArea.left;
	int64_t nMouseY = y + rLocalTextArea.top;

//	int64_t nX = rLocalTextArea.left;
	int64_t nY = rLocalTextArea.top;

	if (mpSliderWin)
	{
		int64_t nMin;
		int64_t nMax;
		mpSliderWin->GetSliderRange(nMin, nMax);

		mnMouseDownSliderVal = mpSliderWin->GetSliderValue();
		nY -=  mnMouseDownSliderVal;
	}

    std::unique_lock<std::mutex> lk(mDocumentMutex);
	for (tDocument::iterator it = mDocument.begin(); it != mDocument.end(); it++)
	{
		tTextLine& textLine = *it;

		ZRect rLine(rLocalTextArea.left, nY, rLocalTextArea.right, nY + GetLineHeight(textLine));

		if (rLocalTextArea.Overlaps(&rLine))
		{
			for (tTextLine::iterator lineIt = textLine.begin(); lineIt != textLine.end(); lineIt++)
			{
				sTextEntry& entry = *lineIt;

                tZFontPtr pFont(gpFontSystem->GetFont(entry.fontParams));
				ZRect rText = pFont->GetOutputRect(rLine, entry.sText.data(), entry.sText.length(), entry.position);
				if (rText.PtInRect(nMouseX, nMouseY))
				{
					if (!entry.sLink.empty())
					{
						gMessageSystem.Post(entry.sLink);
						return true;
					}
				}
			}
		}

		nY += GetLineHeight(textLine);

		// Exit early if we've passed the bottom
		if (nY > rLocalTextArea.bottom)
			break;
	}

	SetMouseDownPos(x, y);
	SetCapture();

	return ZWin::OnMouseDownL(x, y);
}

bool ZFormattedTextWin::OnMouseUpL(int64_t x, int64_t y)
{
	ReleaseCapture();

	return ZWin::OnMouseUpL(x, y);
}


bool ZFormattedTextWin::OnMouseWheel(int64_t /*x*/, int64_t /*y*/, int64_t nDelta)
{
	if (mpSliderWin)
	{
		int64_t nMin;
		int64_t nMax;
		mpSliderWin->GetSliderRange(nMin, nMax);
		if (nMax-nMin > 0)
		{
			mpSliderWin->SetSliderValue(mpSliderWin->GetSliderValue() + nDelta);
		}
	}

    Invalidate();

	return true;
}

bool ZFormattedTextWin::OnMouseMove(int64_t x, int64_t y)
{
	if (AmCapturing() && mpSliderWin)
	{
		int64_t nDownX;
		int64_t nDownY;
		GetMouseDownPos(nDownX, nDownY);
		int64_t nDelta = nDownY - y;
		int64_t nScrollDelta = (nDelta)/ mAreaToDrawTo.Height()/5 + mnMouseDownSliderVal;
		mpSliderWin->SetSliderValue(nScrollDelta);
        Invalidate();
        //ZDEBUG_OUT("DownY:%d, y:%d, nDelta: %d  \n", nDownY, y, mfScrollDelta);
	}

	return ZWin::OnMouseMove(x, y);
}

void ZFormattedTextWin::UpdateScrollbar()
{
	int64_t nFullTextHeight = GetFullDocumentHeight();

    mrTextBorderArea.SetRect(mAreaToDrawTo);

    if (mbScrollable)
        mrTextBorderArea.right -= kSliderWidth;

    mrTextArea.SetRect(mrTextBorderArea);
    if (mbDrawBorder)
        mrTextArea.DeflateRect(6, 6);


	if (mbScrollable && nFullTextHeight > mrTextArea.Height())
	{
        if (!mpSliderWin)
        {
            mpSliderWin = new ZSliderWin(&mnSliderVal);
            mpSliderWin->Init(gSliderThumbVertical, grSliderThumbEdge, gSliderBackground, grSliderBgEdge);
            mpSliderWin->SetDrawSliderValueFlag(false, false, gpFontSystem->GetFont(mCurrentFont));
            mpSliderWin->SetArea(ZRect(mArea.Width() - kSliderWidth, 0, mArea.Width(), mArea.Height()));
            ChildAdd(mpSliderWin);
        }

		mpSliderWin->SetSliderRange(0, nFullTextHeight-mrTextArea.Height());

		if (mnScrollToOnInit > 0)
		{
			int64_t nLine = 0;
			int64_t nHeight = 0;

            std::unique_lock<std::mutex> lk(mDocumentMutex);
			for (tDocument::iterator it = mDocument.begin(); it != mDocument.end() && nLine < mnScrollToOnInit; it++, nLine++)
			{
				tTextLine& textLine = *it;

				nHeight += GetLineHeight(textLine);
			}

			mpSliderWin->SetSliderValue(nHeight);
		}
	}
	else
	{
		if (mpSliderWin)
		{
			ChildDelete(mpSliderWin);
			mpSliderWin = NULL;
		}
	}
    Invalidate();
}

bool ZFormattedTextWin::Paint()
{
	if (!mbInvalid)
		return true;

    if (!mpTransformTexture.get())
        return false;

	ZRect rEdgeSrc(3,5,21,61);

	// Get a local rect
	ZRect rLocalTextBorderArea(mrTextArea);
	rLocalTextBorderArea.InflateRect(6,6);
	//   rLocalTextBorderArea.OffsetRect(mAreaToDrawTo.left, mAreaToDrawTo.top);

	ZRect rLocalTextArea(mrTextArea);
	//   rLocalTextArea.OffsetRect(mAreaToDrawTo.left, mAreaToDrawTo.top);

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());

	if (mbFillBackground)
        mpTransformTexture.get()->Fill(rLocalTextBorderArea, mnFillColor);
    else if (mbDrawBorder)
        mpTransformTexture.get()->BltEdge(gDimRectBackground.get(), grDimRectEdge, rLocalTextBorderArea, ZBuffer::kEdgeBltMiddle_Tile, &mArea);

	ZRect rClip(rLocalTextArea);


//	if (mbDrawBorder)
//		pBufferToDrawTo->BltEdge(&gSliderBackground, rEdgeSrc, mrTextArea, true, pClip);

//	int64_t nX = rLocalTextArea.left;
	int64_t nY = rLocalTextArea.top;

	if (mpSliderWin)
	{
		int64_t nMin;
		int64_t nMax;
		mpSliderWin->GetSliderRange(nMin, nMax);

		nY -=  (int64_t) mpSliderWin->GetSliderValue();
	}

    std::unique_lock<std::mutex> lk(mDocumentMutex);
    for (tDocument::iterator it = mDocument.begin(); it != mDocument.end(); it++)
	{
		tTextLine& textLine = *it;

		ZRect rLine(rLocalTextArea.left, nY, rLocalTextArea.right, nY + GetLineHeight(textLine));

		if (rLocalTextArea.Overlaps(&rLine))
		{
			for (tTextLine::iterator lineIt = textLine.begin(); lineIt != textLine.end(); lineIt++)
			{
				sTextEntry& entry = *lineIt;

                tZFontPtr pFont(gpFontSystem->GetFont(entry.fontParams));
				ZRect rText = pFont->GetOutputRect(rLine, entry.sText.data(), entry.sText.length(), entry.position);

				int64_t nShadowOffset = max((int) pFont->Height()/16, (int) 1);

				pFont->DrawText(mpTransformTexture.get(), entry.sText, rText, entry.nColor, entry.nColor2, entry.style, &rClip);

				if (mbUnderlineLinks && !entry.sLink.empty())
				{
					ZRect rUnderline(rText);
					rUnderline.top = rUnderline.bottom-nShadowOffset;
					rUnderline.IntersectRect(&rClip);
                    mpTransformTexture.get()->Fill(rUnderline, entry.nColor2);

					if (entry.style == ZFont::kShadowed)
					{
						rUnderline.OffsetRect(nShadowOffset, nShadowOffset);
						rUnderline.IntersectRect(&rClip);
                        mpTransformTexture.get()->Fill(rUnderline, 0xff000000);
					}
				}
			}
		}

		nY += GetLineHeight(textLine);

		// Exit early if we've passed the bottom
		if (nY > rLocalTextArea.bottom)
			break;
	}

	return ZWin::Paint();
}

int64_t ZFormattedTextWin::GetLineHeight(tTextLine& textLine)
{
	ZFontParams largestFont;
	for (tTextLine::iterator it = textLine.begin(); it != textLine.end(); it++)
	{
		sTextEntry& textEntry = *it;
		if (textEntry.fontParams.nHeight > largestFont.nHeight)
            largestFont = textEntry.fontParams;
	}

    tZFontPtr pLargestFont(gpFontSystem->GetFont(largestFont));
    assert(pLargestFont);

    return pLargestFont->Height();
}

void ZFormattedTextWin::CalculateFullDocumentHeight()
{
	mnFullDocumentHeight = 0;

    std::unique_lock<std::mutex> lk(mDocumentMutex);
    for (tDocument::iterator it = mDocument.begin(); it != mDocument.end(); it++)
	{
		tTextLine& textLine = *it;

		mnFullDocumentHeight += GetLineHeight(textLine);
	}
}

void ZFormattedTextWin::Clear()
{
    std::unique_lock<std::mutex> lk(mDocumentMutex);
    mDocument.clear();
	mnFullDocumentHeight = 0;

	// Restore the defaults
	mCurrentFont			= ZFontParams("Arial", 12);
	mnCurrentColor			= kDefaultColor;
	mnCurrentColor2			= kDefaultColor;
	mCurrentTextPosition	= kDefaultTextPosition;       
	mCurrentStyle			= kDefaultStyle;
}

bool ZFormattedTextWin::ParseDocument(ZXMLNode* pNode)
{
	Clear();

	// Get the TextWin Attributes
	string sAttribute;
	sAttribute = pNode->GetAttribute(ksTextEdgeTag);
	if (!sAttribute.empty())
		mbDrawBorder = StringHelpers::ToBool(sAttribute);

	sAttribute = pNode->GetAttribute(ksTextFillTag);
	if (!sAttribute.empty())
		mbFillBackground = StringHelpers::ToBool(sAttribute);

	sAttribute = pNode->GetAttribute(ksTextFillColor);
	if (!sAttribute.empty())
		mnFillColor = (uint32_t) StringHelpers::ToInt(sAttribute);

	sAttribute = pNode->GetAttribute(ksTextScrollToBottom);
	if (!sAttribute.empty())
		mnScrollToOnInit = (uint32_t)StringHelpers::ToInt(sAttribute);

	sAttribute = pNode->GetAttribute(ksScrollable);
	if (!sAttribute.empty())
		mbScrollable = StringHelpers::ToBool(sAttribute);

	sAttribute = pNode->GetAttribute(ksUnderlineLinksTag);
	if (!sAttribute.empty())
		mbUnderlineLinks = StringHelpers::ToBool(sAttribute);



	tXMLNodeList elementNodeList;
	pNode->GetChildren("line", elementNodeList);

	for (tXMLNodeList::iterator it = elementNodeList.begin(); it != elementNodeList.end(); it++)
	{
		ZXMLNode* pChild = *it;
		ProcessLineNode(pChild);
	}

	return true;
}

void ZFormattedTextWin::AddTextLine(string sLine, ZFontParams fontParams, uint32_t nCol1, uint32_t nCol2, ZFont::eStyle style, ZFont::ePosition position, bool bWrap, const string& sLink)
{
	tTextLine textLine;
	if (bWrap)
	{
		// Insert as much text on each line as will fit
		while (sLine.length() > 0)
		{
            tZFontPtr pFont(gpFontSystem->GetFont(fontParams));
            assert(pFont);
			int64_t nChars = pFont->CalculateWordsThatFitOnLine(mrTextArea.Width(), sLine.data(), sLine.length());

			sTextEntry textEntry;
			textEntry.sText		= sLine.substr(0, nChars);
			textEntry.nColor	= nCol1;
			textEntry.nColor2	= nCol2;
			textEntry.style		= style;
			textEntry.fontParams	= fontParams;
			textEntry.position	= position;
			textEntry.sLink		= sLink;

			textLine.push_back(textEntry);

			// Add it to the document, and on to the next line
            std::unique_lock<std::mutex> lk(mDocumentMutex);
            mDocument.push_back(textLine);
			mnFullDocumentHeight += GetLineHeight(textLine);
			textLine.clear();

			sLine = sLine.substr((int) nChars);
		}
	}
	else
	{
		// Treat the line as a line node
		ZXMLNode lineNode;
		lineNode.Init(sLine);
		mnCurrentColor = nCol1;
		mnCurrentColor2 = nCol2;
		mCurrentFont = fontParams;
		mCurrentStyle = style;
		mCurrentTextPosition = position;
		msLink = sLink;

        ProcessLineNode(lineNode.GetChild("line"));
	}
}

bool ZFormattedTextWin::ProcessLineNode(ZXMLNode* pTextNode)
{
    if (!pTextNode)
        return false;

	tTextLine textLine;

	string sParam;
	bool bWrapLine = StringHelpers::ToBool(pTextNode->GetAttribute(ksWrapTag));

	tXMLNodeList elementNodeList;
	pTextNode->GetChildren("text", elementNodeList);

	ZASSERT(mrTextArea.Width() > 0);

	for (tXMLNodeList::iterator it = elementNodeList.begin(); it != elementNodeList.end(); it++)
	{
		ZXMLNode* pTextChild = *it;
		ExtractTextParameters(pTextChild);

		string sCaption = pTextChild->GetText();

		if (bWrapLine)
		{
			AddTextLine(sCaption, mCurrentFont, mnCurrentColor, mnCurrentColor2, mCurrentStyle, mCurrentTextPosition, bWrapLine, msLink);
		}
		else
		{
			sTextEntry textEntry;
			textEntry.sText			= sCaption;
			textEntry.nColor		= mnCurrentColor;
			textEntry.nColor2		= mnCurrentColor2;
			textEntry.style			= mCurrentStyle;
			textEntry.fontParams	= mCurrentFont;
			textEntry.position		= mCurrentTextPosition;
			textEntry.sLink			= msLink;

			textLine.push_back(textEntry);
		}
	}

	if (textLine.size() > 0)
	{
        std::unique_lock<std::mutex> lk(mDocumentMutex);
		mDocument.push_back(textLine);
		mnFullDocumentHeight += GetLineHeight(textLine);
	}

	return false;
}

void ZFormattedTextWin::ExtractTextParameters(ZXMLNode* pTextNode)
{
	string sParam;

	sParam = pTextNode->GetAttribute(ksFontParamsTag);
	if (!sParam.empty())
	{
        mCurrentFont = ZFontParams(StringHelpers::URL_Decode(sParam));
	}

	sParam = pTextNode->GetAttribute(ksColorTag);
	if (!sParam.empty())
	{
		mnCurrentColor = (uint32_t)StringHelpers::ToInt(sParam);
	}

	sParam = pTextNode->GetAttribute(ksColor2Tag);
	if (!sParam.empty())
	{
		mnCurrentColor2 = (uint32_t)StringHelpers::ToInt(sParam);
	}
	else
	{
		mnCurrentColor2 = mnCurrentColor;
	}

	sParam = pTextNode->GetAttribute(ksPositionTag);
	if (!sParam.empty())
	{
		StringHelpers::makelower(sParam);

		if (sParam == "topleft")            mCurrentTextPosition = ZFont::kTopLeft;
		else if (sParam == "topcenter")     mCurrentTextPosition = ZFont::kTopCenter;
		else if (sParam == "topright")      mCurrentTextPosition = ZFont::kTopRight;
		else if (sParam == "middleleft")    mCurrentTextPosition = ZFont::kMiddleLeft;
		else if (sParam == "middlecenter")  mCurrentTextPosition = ZFont::kMiddleCenter;
		else if (sParam == "middleright")   mCurrentTextPosition = ZFont::kMiddleRight;
		else if (sParam == "bottomleft")    mCurrentTextPosition = ZFont::kBottomLeft;
		else if (sParam == "bottomcenter")  mCurrentTextPosition = ZFont::kBottomCenter;
		else if (sParam == "bottomright")   mCurrentTextPosition = ZFont::kBottomRight;
		else
		{
			ZASSERT(false);  // unknown position type
		}
	}

	sParam = pTextNode->GetAttribute(ksStyleTag);
	if (!sParam.empty())
	{
        StringHelpers::makelower(sParam);

		if (sParam == "normal")        mCurrentStyle = ZFont::kNormal;
		else if (sParam == "shadowed") mCurrentStyle = ZFont::kShadowed;
		else if (sParam == "embossed") mCurrentStyle = ZFont::kEmbossed;
		else
		{
			ZASSERT(false);  // unknown style type
		}
	}

	sParam = pTextNode->GetAttribute(ksLinkTag);
	if (!sParam.empty())
	{
		msLink = sParam;
	}
}


void ZFormattedTextWin::ScrollTo(int64_t nSliderValue)
{
	if (mpSliderWin)
		mpSliderWin->SetSliderValue(nSliderValue);
    Invalidate();
}
