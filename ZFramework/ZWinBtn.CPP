#include "ZWinBtn.h"
#include "ZBuffer.h"
#include "ZMessageSystem.h"
#include "ZGraphicSystem.h"
#include "ZFont.h"
#include "ZXMLNode.h"
#include "helpers/StringHelpers.h"
#include "ZStringHelpers.h"
#include "Resources.h"
#include "ZWinText.H"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
using namespace std;

///////////////////////////////////////////////////////////////////////////////
ZWinCheck::ZWinCheck(bool* pbChecked)
{
   
    mCheckedStyle = gStyleToggleChecked;
    mUncheckedStyle = gStyleToggleUnchecked;

    mbInternal = false;

    if (!pbChecked)
        mpbChecked = &mbInternal;
    else
	    mpbChecked = pbChecked;

    if (msWinName.empty())
        msWinName = "ZWinCheck_" + gMessageSystem.GenerateUniqueTargetName();
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
    gMessageSystem.AddNotification("radio_check", this);

	ZASSERT(!msWinName.empty())

	return true;
}

bool ZWinCheck::Paint()
{
    if (!PrePaintCheck())
        return false;

    if (*mpbChecked)
    {
        mDrawState = kDown;
        mCaption.style = mCheckedStyle;
    }
    else
    {
        mDrawState = kUp;
        mCaption.style = mUncheckedStyle;
    }

	ZWinSizablePushBtn::Paint();

    if (mbEnabled && mDrawState == kDown)
    {
        mpSurface->FillAlpha(0x88000000|(gStyleToggleChecked.bgCol&0x00ffffff));    // 50% alpha
    }

    return true;
}


bool ZWinCheck::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();
    if (sType == "radio_check")
    {
        string sGroupName = message.GetParam("group");
        if (sGroupName == msRadioGroup)
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

        if (bChecked && !msRadioGroup.empty())
        {
            // If we're part of a radio group and we're being checked, send out a message to uncheck all others in the group
            gMessageSystem.Post("{radio_check}", "group", msRadioGroup, "checked", msWinName);
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
    if (!mbEnabled)
        return true;

    // if we're part of a radio group and area already checked, do nothing
    if (!msRadioGroup.empty() && *mpbChecked)
        return true;

    SetState(!*mpbChecked);
	return true;
}


ZWinSizablePushBtn::ZWinSizablePushBtn()
{
	mpUpEdgeImage       = NULL;
	mpDownEdgeImage     = NULL;
	mDrawState          = kUp;
    mbEnabled           = true;
	msButtonMessage     = "{btn_pressed}";
    mBtnStyle           = gStyleButton;
    mCaption.style      = gStyleCaption;
    mSVGImage.style     = gStyleCaption;

    mpUpEdgeImage = gStandardButtonUpEdgeImage;
    mpDownEdgeImage = gStandardButtonDownEdgeImage;

    mrEdge = grStandardButtonEdge;

    if (msWinName.empty())
        msWinName = "ZWinSizablePushBtn_" + gMessageSystem.GenerateUniqueTargetName();

//	mbTransformable     = false;
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
	return ZWin::Shutdown();
}

bool ZWinSizablePushBtn::Paint()
{
    if (!PrePaintCheck())
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

    if (mAreaLocal.right == 0 || mAreaLocal.bottom == 0)
        return false;

    if (mpParentWin && !mpParentWin->mbVisible)
        return true;

    PaintFromParent();
   
	// Draw the button image
    tZBufferPtr pBufferToUse;

	if (mDrawState == kUp)
		pBufferToUse = mpUpEdgeImage;
	else
		pBufferToUse = mpDownEdgeImage;

    
	ZASSERT(pBufferToUse);

    mpSurface.get()->BltEdge(pBufferToUse.get(), mrEdge, mAreaLocal, ZBuffer::kEdgeBltMiddle_Stretch|ZBuffer::kEdgeBltSides_Stretch);

    int64_t nOffset = max((int)mCaption.style.fp.nHeight / 16, (int)1);
    ZRect rCaption = mAreaLocal;
    if (mDrawState == kDown)
        rCaption.OffsetRect(nOffset, nOffset);

    mSVGImage.area = rCaption;
    if (!mSVGImage.Paint(mpSurface.get()))
    {
//        cout << "mSVGImage.Paint failed\n" << std::flush;
        return false;
    }

    mCaption.area = rCaption;
    if (!mCaption.Paint(mpSurface.get()))
    {
//        cout << "mCaption.Paint failed\n" << std::flush;
        return false;
    }

    if (!mbEnabled)
    {
        mpSurface->FillAlpha(0x88333333); 
        mpSurface->FillAlpha(0x88888888);     
//        mpSurface->Colorize(0, 0);             // gray
    }

	return ZWin::Paint();
}

bool ZWinSizablePushBtn::OnMouseDownL(int64_t /*x*/, int64_t /*y*/)
{
    if (mbEnabled)
    {
        SetCapture();
        mDrawState = kDown;
    	Invalidate();
    }

	return true;
}

bool ZWinSizablePushBtn::OnMouseUpL(int64_t x, int64_t y)
{
	if (AmCapturing())
	{
		ReleaseCapture();
		if ( x > 0 && x < mAreaInParent.Width() && y > 0 && y < mAreaInParent.Height() )
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
		if ( x > 0 && x < mAreaInParent.Width() && y > 0 && y < mAreaInParent.Height() )
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

