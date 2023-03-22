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

const int64_t               kSliderWidth = 20;
const ZGUI::ePosition       kDefaultTextPosition = ZGUI::LB; // we use bottom left by default in case there are multiple size fonts per text line
const ZTextLook		        kDefaultLook = { ZTextLook::kNormal, 0xff000000, 0xff000000 };


const string				ksLineTag("line");
const string				ksTextTag("text");
const string				ksCaptionTag("caption");
const string				ksFontParamsTag("fontparams");
const string				ksColorTag("color");
const string				ksColor2Tag("color2");
const string				ksPositionTag("position");
const string				ksDecoTag("deco");
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
    mCurrentLook                = kDefaultLook;
	mCurrentTextPosition		= kDefaultTextPosition;       
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

void ZFormattedTextWin::SetScrollable(bool bScrollable) 
{ 
    mbScrollable = bScrollable;
    if (mbScrollable)
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
				ZRect rText = pFont->GetOutputRect(rLine, (uint8_t*)entry.sText.data(), entry.sText.length(), entry.pos);
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
		return false;

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
				ZRect rText = pFont->GetOutputRect(rLine, (uint8_t*)entry.sText.data(), entry.sText.length(), entry.pos);

				int64_t nShadowOffset = max((int) pFont->Height()/16, (int) 1);

				pFont->DrawText(mpTransformTexture.get(), entry.sText, rText, entry.look, &rClip);

				if (mbUnderlineLinks && !entry.sLink.empty())
				{
					ZRect rUnderline(rText);
					rUnderline.top = rUnderline.bottom-nShadowOffset;
					rUnderline.IntersectRect(&rClip);
                    mpTransformTexture.get()->Fill(rUnderline, entry.look.colBottom);

					if (entry.look.decoration == ZTextLook::kShadowed)
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
    mCurrentLook = kDefaultLook;
    mCurrentTextPosition	= kDefaultTextPosition;
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

/*void ZFormattedTextWin::AddLineNode(string sLine, ZFontParams fontParams, const ZTextLook& look, ZGUI::ePosition pos, const string& sLink)
{
    // Treat the line as a line node
    ZXMLNode lineNode;
    lineNode.Init(sLine);
    mCurrentLook = look;
    mCurrentFont = fontParams;
    mCurrentTextPosition = pos;
    msLink = sLink;

    ProcessLineNode(lineNode.GetChild("line"));
}*/


void ZFormattedTextWin::AddLineNode(string sLine)
{
    // Treat the line as a line node
    ZXMLNode lineNode;
    lineNode.Init(sLine);
    ProcessLineNode(lineNode.GetChild("line"));
}


void ZFormattedTextWin::AddMultiLine(string sLine, ZFontParams fontParams, const ZTextLook& look, ZGUI::ePosition pos, const string& sLink)
{
    tTextLine textLine;
    // Insert as much text on each line as will fit
    while (sLine.length() > 0)
    {
        tZFontPtr pFont(gpFontSystem->GetFont(fontParams));
        assert(pFont);
        int64_t nChars = pFont->CalculateWordsThatFitOnLine(mrTextArea.Width(), (uint8_t*)sLine.data(), sLine.length());

        sTextEntry textEntry;
        textEntry.sText = sLine.substr(0, nChars);
        textEntry.look = look;
        textEntry.fontParams = fontParams;
        textEntry.pos = pos;
        textEntry.sLink = sLink;

        textLine.push_back(textEntry);

        // Add it to the document, and on to the next line
        std::unique_lock<std::mutex> lk(mDocumentMutex);
        mDocument.push_back(textLine);
        mnFullDocumentHeight += GetLineHeight(textLine);
        textLine.clear();

        sLine = sLine.substr((int)nChars);
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
			AddMultiLine(sCaption, mCurrentFont, mCurrentLook, mCurrentTextPosition, msLink);
		}
		else
		{
			sTextEntry textEntry;
			textEntry.sText         = sCaption;
            textEntry.look          = mCurrentLook;
			textEntry.fontParams    = mCurrentFont;
			textEntry.pos           = mCurrentTextPosition;
			textEntry.sLink         = msLink;

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
        mCurrentLook.colTop = (uint32_t)StringHelpers::ToInt(sParam);
	}

	sParam = pTextNode->GetAttribute(ksColor2Tag);
	if (!sParam.empty())
	{
        mCurrentLook.colBottom = (uint32_t)StringHelpers::ToInt(sParam);
	}
	else
	{
        mCurrentLook.colBottom = mCurrentLook.colTop;
	}

	sParam = pTextNode->GetAttribute(ksPositionTag);
    mCurrentTextPosition = ZGUI::FromString(sParam);
    ZASSERT(mCurrentTextPosition != ZGUI::Unknown);

/*	if (!sParam.empty())
	{
		StringHelpers::makelower(sParam);

		if (sParam == "lt")         mCurrentTextPosition = ZGUI::LT;
		else if (sParam == "ct")    mCurrentTextPosition = ZGUI::CT;
		else if (sParam == "rt")    mCurrentTextPosition = ZGUI::RT;
		else if (sParam == "lc")    mCurrentTextPosition = ZGUI::LC;
		else if (sParam == "c")     mCurrentTextPosition = ZGUI::C;
		else if (sParam == "rc")    mCurrentTextPosition = ZGUI::RC;
		else if (sParam == "lb")    mCurrentTextPosition = ZGUI::LB;
		else if (sParam == "cb")    mCurrentTextPosition = ZGUI::CB;
		else if (sParam == "rb")    mCurrentTextPosition = ZGUI::RB;
		else
		{
			ZASSERT(false);  // unknown position type
		}
	}*/

	sParam = pTextNode->GetAttribute(ksDecoTag);
	if (!sParam.empty())
	{
        StringHelpers::makelower(sParam);

		if (sParam == "normal")        mCurrentLook.decoration = ZTextLook::kNormal;
		else if (sParam == "shadowed") mCurrentLook.decoration = ZTextLook::kShadowed;
		else if (sParam == "embossed") mCurrentLook.decoration = ZTextLook::kEmbossed;
		else
		{
			ZASSERT(false);  // unknown decoration type
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
