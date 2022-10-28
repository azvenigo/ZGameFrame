#include "ZWinBtn.h"
#include "ZBuffer.h"
#include "ZMessageSystem.h"
#include "ZGraphicSystem.h"
#include "ZFont.h"
#include "ZXMLNode.h"
#include "helpers/StringHelpers.h"
#include "ZStringHelpers.h"


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
using namespace std;


///////////////////////////////////////////////////////////////////////////////
ZWinBtn::ZWinBtn() :
mpButtonImage(NULL),
mbWeOwnButtonImage(false),
mnFontID(0),
mpMessageTarget(NULL),
msTriggerMessage("btn_pressed")
{   
}

///////////////////////////////////////////////////////////////////////////////
ZWinBtn::~ZWinBtn() 
{
	Shutdown();
}

///////////////////////////////////////////////////////////////////////////////
bool ZWinBtn::Init() 
{
	mbAcceptsCursorMessages = true;
    mIdleSleepMS = 10000;
	return ZWin::Init();
}

///////////////////////////////////////////////////////////////////////////////
bool ZWinBtn::Shutdown() 
{   
	if ( mbInitted ) 
	{
		if (mpButtonImage)
		{
			if (mbWeOwnButtonImage)		// if this is a privately owned buffer
			{
				delete mpButtonImage;	// we delete it
				mbWeOwnButtonImage = false;
			}
			mpButtonImage = NULL;
		}
		return ZWin::Shutdown();
	}
	return true;
}

bool ZWinBtn::Paint()
{
	if (!mbInvalid)
		return true;

	ZASSERT( mrSrcRect.Width() > 0 && mrSrcRect.Height() > 0 );

	mpTransformTexture->Blt(mpButtonImage, mrSrcRect, mAreaToDrawTo);

	return ZWin::Paint();
}

bool ZWinBtn::SetImage( ZBuffer* pImage )
{
	ZASSERT( pImage );

	if (mbWeOwnButtonImage)
	{
		ZASSERT( mpButtonImage );
		delete mpButtonImage;
	}
	mpButtonImage = pImage;
	mbWeOwnButtonImage = false;

	return true;
}

bool ZWinBtn::SetSrcRect( ZRect rRect )
{
	mrSrcRect = rRect;
	return true;
}

bool ZWinBtn::SetBaseRect( ZRect rRect )
{
	mrBaseRect = rRect;
	return true;
}

bool ZWinBtn::OnMouseHover(int64_t /*x*/, int64_t /*y*/)
{
	if (!msHoverMessage.empty())
		gMessageSystem.Post(msHoverMessage);

	return true;
}

bool ZWinBtn::OnMouseIn()
{
	if (!msCursorInMessage.empty())
		gMessageSystem.Post(msCursorInMessage);

	return true;
}

bool ZWinBtn::OnMouseOut()
{
	if (!msCursorOutMessage.empty())
		gMessageSystem.Post(msCursorOutMessage);

	return true;
}


ZWinPushBtn::ZWinPushBtn() 
{
	mDrawState = kUp;
}

ZWinPushBtn::~ZWinPushBtn() 
{
	Shutdown();
}

bool  ZWinPushBtn::Init() 
{
	ZASSERT(mpButtonImage);

	SetSize(mpButtonImage->GetArea().Width()/2, mpButtonImage->GetArea().Height());
	mrSrcRect = mrBaseRect;
	mbAcceptsCursorMessages = true;
    mIdleSleepMS = 10000;
    return ZWinBtn::Init();
}

bool  ZWinPushBtn::Shutdown() 
{
	return ZWinBtn::Shutdown();
}

bool  ZWinPushBtn::Paint() 
{
	if (!mbInvalid)
		return true;

	mrSrcRect.left   = 0;
	mrSrcRect.right  = mrBaseRect.Width();

	switch (mDrawState)
	{
	case kUp:
		break;
	case kDown:
		mrSrcRect.OffsetRect(mrBaseRect.Width(), 0);
		break;
	}

	if (!ZWinBtn::Paint())
		return false;

    std::shared_ptr<ZFont> pFont(gpFontSystem->GetDefaultFont((int32_t) mnFontID));

	// Now draw the caption
	if (sText.length() > 0)
	{
		int64_t nHeightOffset = mAreaToDrawTo.top + (mArea.Height() - pFont->FontHeight())/2;
		int64_t nWidthOffset = mAreaToDrawTo.left + (mAreaToDrawTo.Width() - pFont->StringWidth(sText))/2;

		ZRect rText(nWidthOffset, nHeightOffset, mAreaToDrawTo.right, mAreaToDrawTo.bottom);

		if (mDrawState == kDown)
		{
			int64_t nOffset = max((int) pFont->FontHeight()/16, (int) 1);
			rText.OffsetRect(nOffset,nOffset);
		}

		// Draw it here
		pFont->DrawText(mpTransformTexture, sText, rText, 0xffffffff, 0xffffffff, ZFont::kNormal);
	}

	return ZWin::Paint();
}

bool	ZWinPushBtn::SetImage( ZBuffer* pImage )
{
	if (!ZWinBtn::SetImage(pImage))
		return false;

	// Calculate the rects
	mrBaseRect = ZRect( 0, 0, mpButtonImage->GetArea().Width()/2, mpButtonImage->GetArea().Height() );
	mrSrcRect = mrBaseRect;

	SetSize( mrBaseRect.Width(), mrBaseRect.Height() );

	return true;
}

bool  ZWinPushBtn::OnMouseDownL(int64_t /*x*/, int64_t /*y*/)
{
	SetCapture();
	mDrawState = kDown;
	Invalidate();
	return true;
}

bool  ZWinPushBtn::OnMouseUpL(int64_t x, int64_t y)
{
	if (AmCapturing())
	{
		ReleaseCapture();
		if ( x > 0 && x < mArea.Width() && y > 0 && y < mArea.Height() )
		{
			gMessageSystem.Post(msTriggerMessage);
		}
		mDrawState = kUp;
		Invalidate();
	}

	return true;
}

bool ZWinPushBtn::OnMouseMove(int64_t x, int64_t y) 
{
	if (AmCapturing())
	{
		// Mouse button is down
		if ( x > 0 && x < mArea.Width() && y > 0 && y < mArea.Height() )
		{
			// Mouse button is down and cursor is within button
			if (mDrawState != kDown)
			{
				// Button picture isn't down so fix it
				mDrawState = kDown;
				Invalidate();
			}
		}
		else
		{
			// Mouse button is down but outside of our rect
			if (mDrawState == kDown)
			{
				// if our state is down and captured fix it
				mDrawState = kUp;
				Invalidate();
			}
		}
	}

	return true;
}



///////////////////////////////////////////////////////////////////////////////
ZWinCheck::ZWinCheck(bool* pbChecked)
{
	mpbChecked = pbChecked;
}

///////////////////////////////////////////////////////////////////////////////
ZWinCheck::~ZWinCheck() 
{
	Shutdown();
}

///////////////////////////////////////////////////////////////////////////////
bool ZWinCheck::Init() 
{  
	// Important note!
	// Init must be called after image, font, and captions are set.
	ZWinSizablePushBtn::Init();

	ZASSERT(!msWinName.empty())

	return true;
}

bool ZWinCheck::Paint()
{
	if (!mbInvalid)
		return true;

	ZASSERT( mAreaToDrawTo.Width() > 0 && mAreaToDrawTo.Height() > 0 );

	mDrawState = *mpbChecked ? kDown : kUp;

	return ZWinSizablePushBtn::Paint();
}


bool ZWinCheck::HandleMessage(const ZMessage& message)
{
	return ZWinSizablePushBtn::HandleMessage(message);
}


void ZWinCheck::SetState(bool bChecked, bool bPostNotifications)
{
	*mpbChecked = bChecked;

	// Post messages
	if (bPostNotifications)
	{
		if (*mpbChecked)
		{
			gMessageSystem.Post(msButtonMessage);
		}
		else
		{
			gMessageSystem.Post(msUncheckMessage);
		}
	}

    Invalidate();
}

bool ZWinCheck::OnMouseUpL(int64_t, int64_t)
{
	return true;
}

bool ZWinCheck::OnMouseDownL(int64_t, int64_t)
{
	SetState(!*mpbChecked);
	return true;
}


ZWinSizablePushBtn::ZWinSizablePushBtn()
{
	mpUpEdgeImage		= NULL;
	mpDownEdgeImage		= NULL;
	mnFontID			= 0;
	mDrawState			= kUp;
	mCaptionStyle		= ZFont::kNormal;
	mnTextColor			= 0xff000000;
	mnTextColor2		= 0xff000000;
	msButtonMessage		= "btn_pressed";
	mbTransformable = false;
}

ZWinSizablePushBtn::~ZWinSizablePushBtn()
{
}

bool ZWinSizablePushBtn::Init()
{ 
	mbAcceptsCursorMessages = true;
    mIdleSleepMS = 10000;

	return ZWin::Init();
}

bool ZWinSizablePushBtn::Shutdown()
{
	if (mbInitted)
	{
//		PrivateBuffer(false);
	}

	return ZWin::Shutdown();
}

bool ZWinSizablePushBtn::InitFromXML(ZXMLNode* pNode)
{
	msCaption = pNode->GetText();
	msWinName = pNode->GetAttribute("winname");

	mnFontID = StringHelpers::ToInt(pNode->GetAttribute("size"));

	string sStyle = pNode->GetAttribute("style");
	if (sStyle == "shadowed")
		mCaptionStyle = ZFont::kShadowed;
	else if (sStyle == "embossed")
		mCaptionStyle = ZFont::kEmbossed;

	string sColor = pNode->GetAttribute("color");
	if (!sColor.empty())
		mnTextColor = StringHelpers::ToInt(sColor);

	string sColor2 = pNode->GetAttribute("color2");
	if (!sColor2.empty())
		mnTextColor2 = StringHelpers::ToInt(sColor2);

	msButtonMessage = pNode->GetAttribute("message");

	string sArea = pNode->GetAttribute("area");
	if (!sArea.empty())
		mArea = StringToRect(sArea);

	return true;
}


bool ZWinSizablePushBtn::Paint()
{
	if (!mbInvalid)
		return true;

	// Draw the button image
	ZBuffer* pBufferToUse;

	if (mDrawState == kUp)
		pBufferToUse = mpUpEdgeImage;
	else
		pBufferToUse = mpDownEdgeImage;


	ZASSERT(pBufferToUse);

	mpTransformTexture->BltEdge(pBufferToUse, mrEdge, mAreaToDrawTo, true);


	// Draw the text
	int64_t nStrlen = msCaption.length();
	if (nStrlen > 0)
	{
        std::shared_ptr<ZFont> pFont(gpFontSystem->GetDefaultFont((int32_t) mnFontID));
		ZRect rText(pFont->GetOutputRect(mAreaToDrawTo, msCaption.data(), nStrlen, ZFont::kMiddleCenter));

		string sCaptionToDraw(msCaption);


		// Offset for text and shadow is based on font size.....but at least of 1
		int64_t nOffset = max((int) pFont->FontHeight()/16, (int) 1);
		if (mDrawState == kDown)
			rText.OffsetRect(nOffset,nOffset);

		pFont->DrawText(mpTransformTexture, sCaptionToDraw, rText, mnTextColor, mnTextColor2, mCaptionStyle);
	}

	return ZWin::Paint();
}

bool ZWinSizablePushBtn::OnMouseDownL(int64_t /*x*/, int64_t /*y*/)
{
	SetCapture();
	mDrawState = kDown;
	Invalidate();

	return true;
}

bool ZWinSizablePushBtn::OnMouseUpL(int64_t x, int64_t y)
{
	if (AmCapturing())
	{
		ReleaseCapture();
		if ( x > 0 && x < mArea.Width() && y > 0 && y < mArea.Height() )
		{
			gMessageSystem.Post(msButtonMessage);
		}

		mDrawState = kUp;
		Invalidate();

		return true;
	}

	return ZWin::OnMouseUpL(x, y);
}

bool ZWinSizablePushBtn::OnMouseMove(int64_t x, int64_t y)
{
	if (AmCapturing())
	{
		// Mouse button is down
		if ( x > 0 && x < mArea.Width() && y > 0 && y < mArea.Height() )
		{
			// Mouse button is down and cursor is within button
			if (mDrawState != kDown)
			{
				// Button picture isn't down so fix it
				mDrawState = kDown;
				Invalidate();
			}
		}
		else
		{
			// Mouse button is down but outside of our rect
			if (mDrawState == kDown)
			{
				// if our state is down and captured fix it
				mDrawState = kUp;
				Invalidate();
			}
		}
		return true;
	}

	return ZWin::OnMouseMove(x, y);
}

void ZWinSizablePushBtn::SetCaption(const string& sCaption) 
{ 
	msCaption = sCaption; 
}

void ZWinSizablePushBtn::SetColor(uint32_t nCol) 
{ 
	mnTextColor = nCol; 
}

void ZWinSizablePushBtn::SetColor2(uint32_t nCol) 
{ 
	mnTextColor2 = nCol; 
}

void ZWinSizablePushBtn::SetStyle(ZFont::eStyle style) 
{ 
	mCaptionStyle = style; 
}

void ZWinSizablePushBtn::SetFontID(int64_t nFontID) 
{ 
	mnFontID = nFontID; 
}


void ZWinSizablePushBtn::SetImages( ZBuffer* pUpEdgeImage, ZBuffer* pDownEdgeImage, ZRect rEdge )
{
	ZASSERT(pUpEdgeImage);
	ZASSERT(pDownEdgeImage);

	mpUpEdgeImage = pUpEdgeImage;
	mpDownEdgeImage = pDownEdgeImage;

	mrEdge = rEdge;
}

string ZWinSizablePushBtn::FitCaptionInWidth(int64_t nWidth, const string& sCaption, ZFont* pFont)
{
	int64_t nFrontChars = 0;
	int64_t nBackChars = 0;
	int64_t nTotalLength = sCaption.length();
	int64_t nWidthSoFar = pFont->StringWidth("...");
	string sFront;
	string sBack;

	bool bFront = true;
	while (nWidthSoFar < nWidth && nFrontChars+nBackChars < nTotalLength)
	{
		if (bFront)
		{
			char c = sCaption[nFrontChars++];
			sFront += c;
			nWidthSoFar += pFont->CharWidth( c );
		}
		else
		{
			char c = sCaption[ nTotalLength - nBackChars++ ];
			sBack = c + sBack;
			nWidthSoFar += pFont->CharWidth( c );
		}

		bFront = !bFront;
	}

	// The whole string fits.
	if (nFrontChars + nBackChars == nTotalLength)
		return sCaption;

	return sFront + "..." + sBack;
}

