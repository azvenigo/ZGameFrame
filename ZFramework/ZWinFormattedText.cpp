#include "ZWinFormattedText.h"
#include "ZStringHelpers.h"
#include "helpers/StringHelpers.h"
#include "ZMessageSystem.h"
#include "ZWinSlider.h"
#include "ZXMLNode.h"
#include "ZGraphicSystem.h"

using namespace std;

extern shared_ptr<ZBuffer>  gSliderThumbVertical;
extern shared_ptr<ZBuffer>  gSliderBackground;
extern ZRect                grSliderBgEdge;
extern ZRect                grSliderThumbEdge;
extern shared_ptr <ZBuffer> gDimRectBackground;
extern shared_ptr<ZBuffer>  gDimRectBackground;
extern ZRect                grDimRectEdge;

const int64_t               kSliderWidth = 20;
const ZGUI::ePosition       kDefaultTextPosition = ZGUI::LB; // we use bottom left by default in case there are multiple size fonts per text line
const ZGUI::ZTextLook       kDefaultLook = { ZGUI::ZTextLook::kNormal, 0xff000000, 0xff000000 };


const string                ksLineTag("line");
const string                ksTextTag("text");
const string                ksCaptionTag("caption");
const string                ksFontParamsTag("fontparams");
const string                ksColorTag("color");
const string                ksColor2Tag("color2");
const string                ksPositionTag("position");
const string                ksDecoTag("deco");
const string                ksWrapTag("wrap");
const string                ksLinkTag("link");
const string                ksImageTag("img");

const string                ksUnderlineLinksTag("underline_links");
const string                ksTextFillColor("text_fill_color");
const string                ksScrollable("text_scrollable");
const string                ksTextEdgeTag("text_edge_blt");
const string                ksTextScrollToBottom("text_scroll_to_bottom");
const string                ksEvenColumns("even_columns");  // columns sized to widest element in that column 


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static tZBufferPtr gPlaceholderThumb(new ZBuffer());

tZBufferPtr FormattedEntry::Buffer()
{
    if (pBuffer)
        return pBuffer;

    if (text.empty())
        return nullptr;

    pBuffer.reset(new ZBuffer());
    if (pBuffer->LoadBuffer(text))
        return pBuffer;

    return gPlaceholderThumb;
}


ZWinFormattedDoc::ZWinFormattedDoc()
{
    mpWinSlider = NULL;

	mrDocumentArea.SetRect(0,0,0,0);
	mrDocumentBorderArea.SetRect(0,0,0,0);

	mnFullDocumentHeight    = 0;
    mbDocumentInvalid       = true;
//	mbScrollable            = false;

	mnScrollToOnInit        = 0;
	mnMouseDownSliderVal    = 0;
	mfMouseMomentum         = 0.0;

    mBehavior               = kBackgroundFill;
    mStyle                  = gDefaultFormattedDocStyle;

	mbAcceptsCursorMessages = true;

    if (msWinName.empty())
        msWinName = "ZWinFormattedDoc_" + gMessageSystem.GenerateUniqueTargetName();
}

ZWinFormattedDoc::~ZWinFormattedDoc()
{
}

bool ZWinFormattedDoc::Init()
{
	if (ZWin::Init())
	{
        mIdleSleepMS = 10000;
		UpdateDocumentAndScrollbar();
	}

	return true;
}


void ZWinFormattedDoc::SetArea(const ZRect& newArea)
{
	ZWin::SetArea(newArea);
	UpdateDocumentAndScrollbar();
}


bool ZWinFormattedDoc::OnMouseDownL(int64_t x, int64_t y)
{
	ZRect rEdgeSrc(3,5,21,61);

	// Get a local rect
	ZRect rLocalTextBorderArea(mrDocumentArea);
	rLocalTextBorderArea.InflateRect(6,6);

	ZRect rLocalDocArea(mrDocumentArea);
    rLocalDocArea.DeflateRect(mStyle.pad.h, mStyle.pad.v);


	ZRect rClip(rLocalDocArea);

	int64_t nMouseX = x + rLocalDocArea.left;
	int64_t nMouseY = y + rLocalDocArea.top;

//	int64_t nX = rLocalTextArea.left;
	int64_t nY = rLocalDocArea.top;

	if (mpWinSlider)
	{
		int64_t nMin;
		int64_t nMax;
        mpWinSlider->GetSliderRange(nMin, nMax);

		mnMouseDownSliderVal = mnSliderVal;
		nY -=  mnMouseDownSliderVal;
	}

    std::unique_lock<std::recursive_mutex> lk(mDocumentMutex);

    if (IsBehaviorSet(kEvenColumns) && mColumnWidths.empty())
        ComputeColumnWidths();


	for (tDocument::iterator it = mDocument.begin(); it != mDocument.end(); it++)
	{
		tFormattedLine& line = *it;

        int64_t lineHeight = GetLineHeight(line);
		ZRect rLine(rLocalDocArea.left, nY, rLocalDocArea.right, nY + lineHeight);

		if (rLocalDocArea.Overlaps(&rLine))
		{
            int64_t col = 0;
			for (auto& entry : line)
			{
                if (entry.type == FormattedEntry::Image)
                {
                    ZRect rSrc(entry.Buffer()->GetArea());
                    ZRect rDst(rSrc);
                    rDst = ZGUI::Arrange(rDst, rLine, entry.style.pos);
                    if (rDst.PtInRect(nMouseX, nMouseY) && !entry.link.empty())
                    {
                        gMessageSystem.Post(entry.link);
                        return true;
                    }

                    if (IsBehaviorSet(kEvenColumns))
                        rLine.left += mColumnWidths[col];
                    else
                        rLine.left += rDst.Width();
                }
                else
                {
                    tZFontPtr pFont(entry.style.Font());
                    ZRect rText = pFont->Arrange(rLine, entry.text, entry.style.pos);
                    if (rText.PtInRect(nMouseX, nMouseY) && !entry.link.empty())
                    {
                        gMessageSystem.Post(entry.link);
                        return true;
                    }
                    if (IsBehaviorSet(kEvenColumns))
                        rLine.left += mColumnWidths[col];
                    else
                        rLine.left += rText.Width();
                }
			}
            rLine.left += mStyle.pad.h;
            col++;
		}

		nY += lineHeight;

		// Exit early if we've passed the bottom
		if (nY > rLocalDocArea.bottom)
			break;
	}

    mMouseDownOffset.Set(x, y);
	SetCapture();

	return ZWin::OnMouseDownL(x, y);
}

bool ZWinFormattedDoc::OnMouseUpL(int64_t x, int64_t y)
{
	ReleaseCapture();

	return ZWin::OnMouseUpL(x, y);
}


bool ZWinFormattedDoc::OnMouseWheel(int64_t /*x*/, int64_t /*y*/, int64_t nDelta)
{
	if (mpWinSlider)
	{
		int64_t nMin;
		int64_t nMax;
        mpWinSlider->GetSliderRange(nMin, nMax);
		if (nMax-nMin > 0)
		{
            mpWinSlider->SetSliderValue(mnSliderVal + nDelta);
		}
	}

    Invalidate();

	return true;
}

bool ZWinFormattedDoc::OnMouseMove(int64_t x, int64_t y)
{
	if (AmCapturing() && mpWinSlider)
	{
		int64_t nDelta = mMouseDownOffset.y - y;
		int64_t nScrollDelta = (nDelta)/ mAreaLocal.Height()/5 + mnMouseDownSliderVal;
        mpWinSlider->SetSliderValue(nScrollDelta);
        Invalidate();
	}

	return ZWin::OnMouseMove(x, y);
}

int64_t ZWinFormattedDoc::GetFullDocumentHeight()
{
    if (mbDocumentInvalid)
        UpdateDocumentAndScrollbar();

    return mnFullDocumentHeight;
}

void ZWinFormattedDoc::UpdateDocumentAndScrollbar()
{
    if (!mbDocumentInvalid)
        return;

    mbDocumentInvalid = false;

    std::unique_lock<std::recursive_mutex> lk(mDocumentMutex);
    CalculateFullDocumentHeight();

    if (IsBehaviorSet(kEvenColumns))
        ComputeColumnWidths();

    mrDocumentBorderArea.SetRect(mAreaLocal);

    mrDocumentArea.SetRect(mrDocumentBorderArea);

    if (IsBehaviorSet(kDrawBorder))
    {
        mrDocumentArea.left += grDimRectEdge.left;
        mrDocumentArea.top += grDimRectEdge.top;
        mrDocumentArea.right -= (gDimRectBackground->GetArea().Width() - grDimRectEdge.right);
        mrDocumentArea.bottom -= (gDimRectBackground->GetArea().Height() - grDimRectEdge.bottom);
    }


	if (IsBehaviorSet(kScrollable) && mnFullDocumentHeight > mrDocumentArea.Height())
	{
        if (!mpWinSlider)
        {
            mpWinSlider = new ZWinSlider(&mnSliderVal);
            mpWinSlider->mBehavior = ZWinSlider::kInvalidateOnMove;
            mpWinSlider->Init();
            ChildAdd(mpWinSlider);
        }

        mrDocumentBorderArea.right -= kSliderWidth;
        mrDocumentArea.right -= kSliderWidth;

        mpWinSlider->SetArea(ZRect(mrDocumentArea.right, mrDocumentArea.top, mrDocumentArea.right+kSliderWidth, mrDocumentArea.bottom));

        double fThumbRatio = (double)mrDocumentArea.Height()/(double)mnFullDocumentHeight;
        mpWinSlider->SetSliderRange(0, mnFullDocumentHeight -mrDocumentArea.Height(), 1, fThumbRatio);

		if (mnScrollToOnInit > 0)
		{
			int64_t nLine = 0;
			int64_t nHeight = 0;

			for (tDocument::iterator it = mDocument.begin(); it != mDocument.end() && nLine < mnScrollToOnInit; it++, nLine++)
			{
				tFormattedLine& textLine = *it;

				nHeight += GetLineHeight(textLine);
			}

            mpWinSlider->SetSliderValue(nHeight);

		}
	}
	else
	{
		if (mpWinSlider)
		{
			ChildDelete(mpWinSlider);
            mpWinSlider = NULL;
		}
	}
}

bool ZWinFormattedDoc::Paint()
{
    if (!PrePaintCheck())
        return false;

    std::unique_lock<std::recursive_mutex> lk(mDocumentMutex);

    if (mbDocumentInvalid)
        UpdateDocumentAndScrollbar();


	// Get a local rect
	ZRect rLocalDocBorderArea(mrDocumentArea);
	//   rLocalTextBorderArea.OffsetRect(mAreaToDrawTo.left, mAreaToDrawTo.top);

	ZRect rLocalDocArea(mrDocumentArea);

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

    if (IsBehaviorSet(kBackgroundFromParent))
    {
        //PaintFromParent();
        GetTopWindow()->RenderToBuffer(mpSurface, mAreaAbsolute, mAreaLocal, this);
    }
    if (IsBehaviorSet(kBackgroundFill))
    {
        mpSurface.get()->FillAlpha(mStyle.bgCol, &mrDocumentArea);
    }
    if (IsBehaviorSet(kDrawBorder))
    {
        mpSurface.get()->BltEdge(gDimRectBackground.get(), grDimRectEdge, mAreaLocal, ZBuffer::kEdgeBltMiddle_None);
    }

	ZRect rClip(rLocalDocArea);

	int64_t nY = rLocalDocArea.top;

	if (mpWinSlider)
	{
		int64_t nMin;
		int64_t nMax;
        mpWinSlider->GetSliderRange(nMin, nMax);

		nY -= mnSliderVal;
	}

    for (tDocument::iterator it = mDocument.begin(); it != mDocument.end(); it++)
	{
		tFormattedLine& line = *it;

        int64_t lineHeight = GetLineHeight(line);
		ZRect rLine(rLocalDocArea.left, nY, rLocalDocArea.right, nY +lineHeight);

		if (rLocalDocArea.Overlaps(&rLine))
		{
            int64_t col = 0;
			for (auto& entry : line)
			{
                if (entry.type == FormattedEntry::Image)
                {
                    ZRect rSrc(entry.Buffer()->GetArea());
                    ZRect rDst(rSrc);
                    rDst = ZGUI::Arrange(rDst, rLine, entry.style.pos);
                    mpSurface->Blt(entry.Buffer().get(), rSrc, rDst);

                    if (IsBehaviorSet(kEvenColumns))
                        rLine.left += mColumnWidths[col];
                    else
                        rLine.left += rDst.Width();
                }
                else
                {
                    if (!entry.text.empty())
                    {
                        tZFontPtr pFont(entry.style.Font());

                        string sVisibleText(entry.text.substr(0, pFont->CalculateLettersThatFitOnLine(rLine.Width(), (const uint8_t*)entry.text.c_str(), entry.text.length())));
                        ZRect rText = pFont->Arrange(rLine, sVisibleText, entry.style.pos);

                        int64_t nShadowOffset = max((int)pFont->Height() / 16, (int)1);

                        pFont->DrawText(mpSurface.get(), sVisibleText, rText, &entry.style.look, &rClip);

                        if (IsBehaviorSet(kUnderlineLinks) && !entry.link.empty())
                        {
                            ZRect rUnderline(rText);
                            rUnderline.top = rUnderline.bottom - nShadowOffset;
                            rUnderline.IntersectRect(&rClip);
                            mpSurface.get()->Fill(entry.style.look.colBottom, &rUnderline);

                            if (entry.style.look.decoration == ZGUI::ZTextLook::kShadowed)
                            {
                                rUnderline.OffsetRect(nShadowOffset, nShadowOffset);
                                rUnderline.IntersectRect(&rClip);
                                mpSurface.get()->Fill(0xff000000, &rUnderline);
                            }
                        }

                        if (IsBehaviorSet(kEvenColumns))
                            rLine.left += mColumnWidths[col];
                        else
                            rLine.left += rText.Width();
                    }
                }
                col++;
                rLine.left += mStyle.pad.h;
			}
		}

		nY += lineHeight;

		// Exit early if we've passed the bottom
		if (nY > rLocalDocArea.bottom)
			break;
	}

	return ZWin::Paint();
}

int64_t ZWinFormattedDoc::GetLineHeight(tFormattedLine& line)
{
    int64_t nLargest = 0;
	for (auto& entry : line)
	{
        if (entry.type == FormattedEntry::Image && entry.pBuffer && entry.pBuffer->GetArea().Height() > nLargest)
            nLargest = entry.pBuffer->GetArea().Height();
		else if (entry.style.fp.Height() > nLargest)
            nLargest = entry.style.fp.Height();
	}

    return nLargest + mStyle.pad.v;
}

void ZWinFormattedDoc::CalculateFullDocumentHeight()
{
	mnFullDocumentHeight = 0;

    std::unique_lock<std::recursive_mutex> lk(mDocumentMutex);
    for (tDocument::iterator it = mDocument.begin(); it != mDocument.end(); it++)
	{
		tFormattedLine& textLine = *it;

		mnFullDocumentHeight += GetLineHeight(textLine);
	}
}

void ZWinFormattedDoc::Clear()
{
    std::unique_lock<std::recursive_mutex> lk(mDocumentMutex);
    mDocument.clear();
    mbDocumentInvalid = true;
}

bool ZWinFormattedDoc::ParseDocument(ZXML* pNode)
{
	Clear();

	// Get the TextWin Attributes
	string sAttribute;
	sAttribute = pNode->GetAttribute(ksTextEdgeTag);
    if (!sAttribute.empty())
        SetBehavior(kDrawBorder, SH::ToBool(sAttribute));

	sAttribute = pNode->GetAttribute(ksTextFillColor);
	if (!sAttribute.empty())
        mStyle.bgCol = (uint32_t) SH::ToInt(sAttribute);

	sAttribute = pNode->GetAttribute(ksTextScrollToBottom);
	if (!sAttribute.empty())
		mnScrollToOnInit = (uint32_t)SH::ToInt(sAttribute);

	sAttribute = pNode->GetAttribute(ksScrollable);
	if (!sAttribute.empty())
		SetBehavior(kScrollable, SH::ToBool(sAttribute));

	sAttribute = pNode->GetAttribute(ksUnderlineLinksTag);
	if (!sAttribute.empty())
		SetBehavior(kUnderlineLinks, SH::ToBool(sAttribute));

        sAttribute = pNode->GetAttribute(ksEvenColumns);
    if (!sAttribute.empty())
        SetBehavior(kEvenColumns, SH::ToBool(sAttribute));


	tXMLNodeList elementNodeList;
	pNode->GetChildren("line", elementNodeList);

	for (tXMLNodeList::iterator it = elementNodeList.begin(); it != elementNodeList.end(); it++)
	{
		ZXML* pChild = *it;
		ProcessLineNode(pChild);
	}

    mbDocumentInvalid = true;

	return true;
}

void ZWinFormattedDoc::ComputeColumnWidths()
{
    mColumnWidths.clear();

    for (auto& line : mDocument)
    {
        // ensure array is sized to fit all lines
        if (line.size() > mColumnWidths.size())
            mColumnWidths.resize(line.size());

        int64_t col = 0;
        for (auto& entry : line)
        {
            int64_t width = 0;
            if (entry.type == FormattedEntry::Image)
            {
                if (entry.Buffer())
                    width = entry.Buffer()->GetArea().Width();
            }
            else
            {
                width = entry.style.Font()->StringWidth(entry.text);
            }
            if (mColumnWidths[col] < width)
                mColumnWidths[col] = width;
            col++;
        }
    }
}



void ZWinFormattedDoc::AddLineNode(string sLine)
{
    // Treat the line as a line node
    ZXML lineNode;
    lineNode.Init(sLine);
    ProcessLineNode(lineNode.GetChild("line"));
    mbDocumentInvalid = true;
}


void ZWinFormattedDoc::AddMultiLine(string sLine, ZGUI::Style style, const string& sLink)
{
    if (style.Uninitialized())
        style = gDefaultFormattedDocStyle;

    tZFontPtr pFont(style.Font());
    assert(pFont);

    // Insert as much text on each line as will fit
    tFormattedLine line;
    while (sLine.length() > 0)
    {
        int64_t nChars = pFont->CalculateWordsThatFitInWidth(mrDocumentArea.Width(), (uint8_t*)sLine.data(), sLine.length());

        FormattedEntry entry;
        entry.type = FormattedEntry::Text;
        entry.text = sLine.substr(0, nChars);
        entry.style = style;
        entry.link = sLink;

        line.push_back(entry);

        // Add it to the document, and on to the next line
        std::unique_lock<std::recursive_mutex> lk(mDocumentMutex);
        mDocument.push_back(line);
        line.clear();

        sLine = sLine.substr((int)nChars);
    }
    mbDocumentInvalid = true;
}

bool ZWinFormattedDoc::ProcessLineNode(ZXML* pTextNode)
{
    if (!pTextNode)
        return false;

	tFormattedLine formattedLine;

	bool bWrapLine = SH::ToBool(pTextNode->GetAttribute(ksWrapTag));

	tXMLNodeList lineElements;
	pTextNode->GetChildren(lineElements);

	//ZASSERT(mrDocumentArea.Width() > 0);

	for (auto& element : lineElements)
	{
        ZGUI::Style style(mStyle);
        if (element->HasAttribute(ksFontParamsTag))
            style.fp = ZFontParams(SH::URL_Decode(element->GetAttribute(ksFontParamsTag)));
        if (element->HasAttribute(ksColorTag))
            style.look.colTop = (uint32_t)SH::ToInt(element->GetAttribute(ksColorTag));
        if (element->HasAttribute(ksColor2Tag))
            style.look.colBottom = (uint32_t)SH::ToInt(element->GetAttribute(ksColor2Tag));
        if (element->HasAttribute(ksPositionTag))
            style.pos = ZGUI::FromString(element->GetAttribute(ksPositionTag));
        if (element->HasAttribute(ksDecoTag))
        {
            string sParam = element->GetAttribute(ksDecoTag);
            SH::makelower(sParam);
            if (sParam == "normal")        style.look.decoration = ZGUI::ZTextLook::kNormal;
            else if (sParam == "shadowed") style.look.decoration = ZGUI::ZTextLook::kShadowed;
            else if (sParam == "embossed") style.look.decoration = ZGUI::ZTextLook::kEmbossed;
        }
        string sLink = element->GetAttribute(ksLinkTag);

        if (element->GetName() == "img")
        {
            string sPath = element->GetText();
            formattedLine.push_back(FormattedEntry(FormattedEntry::Image, sPath, nullptr, style, sLink));
        }
        else if (element->GetName() == "text")
        {
            if (bWrapLine)
            {
                AddMultiLine(element->GetText(), style, sLink);
            }
            else
            {
                formattedLine.push_back(FormattedEntry(FormattedEntry::Text, element->GetText(), nullptr, style, sLink));
            }
        }
        else
        {
            // unknown element
            assert(false);
        }

	}

	if (formattedLine.size() > 0)
	{
        std::unique_lock<std::recursive_mutex> lk(mDocumentMutex);
		mDocument.push_back(formattedLine);
        mbDocumentInvalid = true;
	}

	return false;
}

/*void ZWinFormattedDoc::ExtractParameters(ZXMLNode* pTextNode)
{
	string sParam;

	sParam = pTextNode->GetAttribute(ksFontParamsTag);
	if (!sParam.empty())
	{
        mCurrentFont = ZFontParams(SH::URL_Decode(sParam));
	}

	sParam = pTextNode->GetAttribute(ksColorTag);
	if (!sParam.empty())
	{
        mCurrentLook.colTop = (uint32_t)SH::ToInt(sParam);
	}

	sParam = pTextNode->GetAttribute(ksColor2Tag);
	if (!sParam.empty())
	{
        mCurrentLook.colBottom = (uint32_t)SH::ToInt(sParam);
	}
	else
	{
        mCurrentLook.colBottom = mCurrentLook.colTop;
	}

	sParam = pTextNode->GetAttribute(ksPositionTag);
    if (!sParam.empty())
    {
        mCurrentTextPosition = ZGUI::FromString(sParam);
        ZASSERT(mCurrentTextPosition != ZGUI::Unknown);
    }


	sParam = pTextNode->GetAttribute(ksDecoTag);
	if (!sParam.empty())
	{
        SH::makelower(sParam);

		if (sParam == "normal")        mCurrentLook.decoration = ZGUI::ZTextLook::kNormal;
		else if (sParam == "shadowed") mCurrentLook.decoration = ZGUI::ZTextLook::kShadowed;
		else if (sParam == "embossed") mCurrentLook.decoration = ZGUI::ZTextLook::kEmbossed;
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

    sParam = pTextNode->GetAttribute(ksImageTag);
    if (!sParam.empty())
    {
        mImage.reset(new ZBuffer());
        if (!mImage->LoadBuffer(sParam))
        {
            ZERROR("Failed to load image element:", sParam, "\n");
            mImage.reset();
        }
    }

}
*/

void ZWinFormattedDoc::ScrollTo(int64_t nSliderValue)
{
    if (mpWinSlider)
    {
        mpWinSlider->SetSliderValue(nSliderValue);
    }
    Invalidate();
}
