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
    mIdleSleepMS = 30000;
	return ZWin::Init();
}

///////////////////////////////////////////////////////////////////////////////
bool ZWinBtn::Shutdown() 
{   
	if ( mbInitted ) 
	{
        mpButtonImage.reset();
		return ZWin::Shutdown();
	}
	return true;
}

bool ZWinBtn::Paint()
{
	if (!mbInvalid)
		return false;

    if (!mpTransformTexture.get())
        return false;
   
    ZASSERT( mrSrcRect.Width() > 0 && mrSrcRect.Height() > 0 );

    mpTransformTexture.get()->Blt(mpButtonImage.get(), mrSrcRect, mAreaToDrawTo);

	return ZWin::Paint();
}

bool ZWinBtn::SetImage(tZBufferPtr pImage )
{
	ZASSERT( pImage );

	mpButtonImage = pImage;

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

    ZRect buttonImageRect(mpButtonImage->GetArea());
    buttonImageRect.right = buttonImageRect.Width() / 2;
	SetArea(buttonImageRect);
	mrSrcRect = mrBaseRect;
	mbAcceptsCursorMessages = true;
    mIdleSleepMS = 30000;
    return ZWinBtn::Init();
}

bool  ZWinPushBtn::Shutdown() 
{
	return ZWinBtn::Shutdown();
}

bool  ZWinPushBtn::Paint() 
{
	if (!mbInvalid)
		return false;

    if (!mpTransformTexture.get())
        return false;

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

	// Now draw the caption
	if (sText.length() > 0)
	{
		int64_t nHeightOffset = mAreaToDrawTo.top + (mArea.Height() - mpFont->Height())/2;
		int64_t nWidthOffset = mAreaToDrawTo.left + (mAreaToDrawTo.Width() - mpFont->StringWidth(sText))/2;

		ZRect rText(nWidthOffset, nHeightOffset, mAreaToDrawTo.right, mAreaToDrawTo.bottom);

		if (mDrawState == kDown)
		{
			int64_t nOffset = max((int) mpFont->Height()/16, (int) 1);
			rText.OffsetRect(nOffset,nOffset);
		}

		// Draw it here
		mpFont->DrawText(mpTransformTexture.get(), sText, rText);
	}

	return ZWin::Paint();
}

bool	ZWinPushBtn::SetImage(tZBufferPtr pImage )
{
	if (!ZWinBtn::SetImage(pImage))
		return false;

	// Calculate the rects
	mrBaseRect = ZRect( 0, 0, mpButtonImage->GetArea().Width()/2, mpButtonImage->GetArea().Height() );
	mrSrcRect = mrBaseRect;

	SetArea(mrBaseRect);

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
    ZASSERT(pbChecked);
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
		return false;

    if (!mpTransformTexture.get())
        return false;

	ZASSERT( mAreaToDrawTo.Width() > 0 && mAreaToDrawTo.Height() > 0 );

    if (*mpbChecked)
    {
        mDrawState = kDown;
        mStyle= mCheckedStyle;
    }
    else
    {
        mDrawState = kUp;
        mStyle = mUncheckedStyle;
    }

	return ZWinSizablePushBtn::Paint();
}


bool ZWinCheck::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();
    if (sType == "radio_check")
    {
        string sGroupName = message.GetParam("group");
        if (sGroupName == msRadioGroupName)
        {
            // Part of our group

            string sCheckedName = message.GetParam("checked");
            if (sCheckedName != msWinName && *mpbChecked)
            {
                // Part of our group but not us....and we're checked
                SetState(false, true);
            }
        }
        return true;
    }

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

    if (bChecked && !msRadioGroupName.empty())
    {
        // If we're part of a radio group and we're being checked, send out a message to uncheck all others in the group
//        string sUncheckMessage;
//        Sprintf(sUncheckMessage, "radio_check;group=%s;checked=%s", msRadioGroupName.c_str(), msWinName.c_str());
//        gMessageSystem.Post(sUncheckMessage);
        gMessageSystem.Post("radio_check", "group", msRadioGroupName, "checked", msWinName);
    }


    Invalidate();
}

void ZWinCheck::SetRadioGroup(const std::string& sRadioGroupName)
{
    msRadioGroupName = sRadioGroupName;
    if (!msRadioGroupName.empty())
    {
        gMessageSystem.AddNotification("radio_check", this);
    }
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
	mpUpEdgeImage       = NULL;
	mpDownEdgeImage     = NULL;
	mDrawState          = kUp;
	msButtonMessage     = "btn_pressed";
	mbTransformable     = false;
    mStyle              = gStyleButton;
}

ZWinSizablePushBtn::~ZWinSizablePushBtn()
{
}

bool ZWinSizablePushBtn::Init()
{ 
	mbAcceptsCursorMessages = true;
    mIdleSleepMS = 30000;

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

//	int64_t nFontID = SH::ToInt(pNode->GetAttribute("size"));
    ZFontParams params(pNode->GetAttribute("fontparams"));

    mStyle = ZGUI::Style(pNode->GetAttribute("style"));


	msButtonMessage = pNode->GetAttribute("message");

	string sArea = pNode->GetAttribute("area");
	if (!sArea.empty())
		mArea = StringToRect(sArea);

	return true;
}

bool ZWinSizablePushBtn::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return false;

    if (!mpTransformTexture.get())
        return false;
    // experimentally blt parent's buffer to test transparency
    if (mpParentWin && mpParentWin->IsVisible())
    {
        //        cout << "mpParentWin:" << (uint64_t)mpParentWin;
        tZBufferPtr parentBuffer = mpParentWin->GetTransformTexture();
        mpTransformTexture.get()->Blt(parentBuffer.get(), mAreaToDrawTo, mpTransformTexture.get()->GetArea());
    }
   
	// Draw the button image
    tZBufferPtr pBufferToUse;

	if (mDrawState == kUp)
		pBufferToUse = mpUpEdgeImage;
	else
		pBufferToUse = mpDownEdgeImage;

    
	ZASSERT(pBufferToUse);

    mpTransformTexture.get()->BltEdge(pBufferToUse.get(), mrEdge, mAreaToDrawTo, ZBuffer::kEdgeBltMiddle_Stretch);


	// Draw the text
	int64_t nStrlen = msCaption.length();
	if (nStrlen > 0)
	{
		ZRect rText(mStyle.Font()->GetOutputRect(mAreaToDrawTo, (uint8_t*)msCaption.data(), nStrlen, ZGUI::Center));

		string sCaptionToDraw(msCaption);


		// Offset for text and shadow is based on font size.....but at least of 1
		int64_t nOffset = max((int) mStyle.fp.nHeight/16, (int) 1);
		if (mDrawState == kDown)
			rText.OffsetRect(nOffset,nOffset);

		mStyle.Font()->DrawText(mpTransformTexture.get(), sCaptionToDraw, rText, &mStyle.look);
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


void ZWinSizablePushBtn::SetImages(tZBufferPtr pUpEdgeImage, tZBufferPtr pDownEdgeImage, ZRect rEdge )
{
	ZASSERT(pUpEdgeImage);
	ZASSERT(pDownEdgeImage);

	mpUpEdgeImage = pUpEdgeImage;
	mpDownEdgeImage = pDownEdgeImage;

	mrEdge = rEdge;
}

string ZWinSizablePushBtn::FitCaptionInWidth(int64_t nWidth, const string& sCaption, tZFontPtr pFont)
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

