#include "ZWinScriptedDialog.h"
#include "ZWinSlider.h"
#include "ZWinImage.h"
#include "ZWinBtn.h"
#include "ZWinFormattedText.h"
#include "ZStringHelpers.h"
#include "ZMessageSystem.h"
#include "ZXMLNode.h"
#include "ZGraphicSystem.h"
#include "helpers/StringHelpers.h"
#include "Resources.h"

using namespace std;

extern tZBufferPtr  gDefaultDialogBackground;

extern tZBufferPtr  gStandardButtonUpEdgeImage;
extern tZBufferPtr  gStandardButtonDownEdgeImage;
extern ZRect        grStandardButtonEdge;



const string		ksScriptedDialog("scripteddialog");
const string		ksElement("element");

const string		ksElementDrawBackground("draw_background");
const string		ksElementBackgroundColor("bgcolor");
const string		ksTransformable("transformable");
const string		ksTransformIn("transform_in");
const string		ksTransformOut("transform_out");

const string		ksTransformInTime("transform_in_time");
const string		ksTransformOutTime("transform_out_time");

const string		ksElementWinName("winname");

const string		ksElementImage("image");
const string		ksElementTextWin("textwin");
const string		ksElementTextWinRawFormat("rawformat");
const string		ksCaption("caption");

const string		ksElementButton("btn");

const string		ksAreaTag("area");     // format "l,t,r,b"
const string		ksColorTag("color");      // format "r,g,b"
const string		ksMessageTag("message");
const string        ksFontParamsTag("fontparams");

const int64_t       kButtonMeasure = 32;
const int64_t       kPadding = 4;

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


ZWinScriptedDialog::ZWinScriptedDialog()
{
	//mbTransformable = true;
	mbDrawDefaultBackground = true;
	mbFillBackground = false;
    mnBackgroundColor = 0;
}

ZWinScriptedDialog::~ZWinScriptedDialog()
{
}

void ZWinScriptedDialog::PreInit(const string& sDialogScript)
{
	msDialogScript = sDialogScript;
}

bool ZWinScriptedDialog::Init()
{
	ExecuteScript(msDialogScript);

	//   cCEFont* pFont = gpFontSystem->GetDefaultFont(0);
//	gMessageSystem.AddNotification("kill_window", this);

	tMessageBoxButtonList::iterator it;


	//////////////////////////////////////////////
	// Add the Auto-arranged buttons

	if (mArrangedMessageBoxButtonList.size() > 0)
	{
		// Find the largest button
		int64_t nLargestCaptionSize = 0;
		for (it = mArrangedMessageBoxButtonList.begin(); it != mArrangedMessageBoxButtonList.end(); it++)
		{
			ZWinSizablePushBtn* pBtn = *it;

			string sCaption = pBtn->mCaption.sText;
			int64_t nSize = pBtn->mCaption.style.Font()->StringWidth(sCaption);
			ZASSERT(nSize < mAreaLocal.Width());

			if (nSize > nLargestCaptionSize)
				nLargestCaptionSize = nSize;
		}

		// Calculate the padding between buttons
		//
		//  |      [button1]      [button2]      |
		//     a              b               c
		//
		//  With 2 buttons, there are three spaces, a b & c

		int64_t nButtonWidth = nLargestCaptionSize;

		int64_t nButtonsPerRow = (mAreaInParent.Width()) / (nButtonWidth);

		// If there are fewer buttons than can fit.... use this value
		if (nButtonsPerRow > (int64_t) mArrangedMessageBoxButtonList.size())
			nButtonsPerRow = (int64_t) mArrangedMessageBoxButtonList.size();

		int64_t nActualPaddingRoom = mAreaInParent.Width() - (nButtonsPerRow * nButtonWidth);

		int64_t nPixelsBetweenButtons = nActualPaddingRoom / (nButtonsPerRow+1); // calculate the spaces.... nNumButtons+1


		int64_t nYPos = 0;
		int64_t nXPos = nPixelsBetweenButtons;

		// Create buttons for each entry in our button list
		for (it = mArrangedMessageBoxButtonList.begin(); it != mArrangedMessageBoxButtonList.end(); it++)
		{
			ZWinSizablePushBtn* pBtn = *it;

			int64_t nFontSize = pBtn->mCaption.style.fp.nHeight;

			ZRect rButtonArea(nXPos, nYPos, nXPos + nLargestCaptionSize + nFontSize*kPadding*2, nYPos + kButtonMeasure);
			pBtn ->SetArea(rButtonArea);

			nXPos += rButtonArea.Width() + nPixelsBetweenButtons;     // next x offset

			// If the button won't fit on this line
			if (nXPos + rButtonArea.Width() > mAreaInParent.Width())
			{
				// Advance to the next row of buttons
				nYPos += kButtonMeasure + nFontSize*kPadding;
				nXPos = nPixelsBetweenButtons;

				ZASSERT(nYPos < mAreaInParent.Height() - kButtonMeasure*2);    // assert that it will fit in the window
			}
		}

		// Now move all of the buttons down to the bottom of the window
		int64_t nButtonRows = (mArrangedMessageBoxButtonList.size() + nButtonsPerRow - 1) / nButtonsPerRow;

		int64_t nYOffset = mAreaInParent.Height() - kPadding*2 - kButtonMeasure - (kButtonMeasure) * nButtonRows;
		for (it = mArrangedMessageBoxButtonList.begin(); it != mArrangedMessageBoxButtonList.end(); it++)
		{
			ZWinSizablePushBtn* pBtn = *it;
			ZRect rCurrentRect(pBtn->GetArea());
			rCurrentRect.OffsetRect(0,nYOffset);
			pBtn->SetArea(rCurrentRect);
		}
	}

	mbAcceptsCursorMessages = true;

	if (mbFillBackground)
		mpSurface->Fill(mnBackgroundColor);

	return ZWin::Init();
}

bool ZWinScriptedDialog::Shutdown()
{
	return ZWin::Shutdown();
};

bool ZWinScriptedDialog::Paint()
{
	if (!mbInvalid)
		return false;
    if (!mpSurface)
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface->GetMutex());
   
    // Draw the dialog edge
	if (mbDrawDefaultBackground)
		mpSurface->Fill(gDefaultDialogFill);
	return ZWin::Paint();
}


bool ZWinScriptedDialog::ExecuteScript(string sDialogScript)
{
	string sElement;

	ZXMLNode dialogTree;
	dialogTree.Init(sDialogScript);

	ZXMLNode* pDialogNode = dialogTree.GetChild(ksScriptedDialog);
	ZASSERT(pDialogNode);
	if (!pDialogNode)
		return false;

	// Window Attributes
	msWinName = pDialogNode->GetAttribute(ksElementWinName);
//	if (pDialogNode->HasAttribute(ksTransformable))
//		mbTransformable	= SH::ToBool(pDialogNode->GetAttribute(ksTransformable));

	if (pDialogNode->HasAttribute(ksTransformIn))
	{
		string sTransformType = pDialogNode->GetAttribute(ksTransformIn);
		if (sTransformType == "none" || sTransformType == "0")
			mTransformIn = kNone;
		else if (sTransformType == "fade" || sTransformType == "1")
			mTransformIn = kFade;
		else if (sTransformType == "slideleft" || sTransformType == "2")
			mTransformIn = kSlideLeft;
		else if (sTransformType == "slideright" || sTransformType == "3")
			mTransformIn = kSlideRight;
		else if (sTransformType == "slideup" || sTransformType == "4")
			mTransformIn = kSlideUp;
		else if (sTransformType == "slidedown" || sTransformType == "5")
			mTransformIn = kSlideDown;
	}

	if (pDialogNode->HasAttribute(ksTransformInTime))
		mnTransformInTime = SH::ToInt( pDialogNode->GetAttribute(ksTransformInTime));

	if (pDialogNode->HasAttribute(ksTransformOut))
	{
		string sTransformType = pDialogNode->GetAttribute(ksTransformOut);
		if (sTransformType == "none" || sTransformType == "0")
			mTransformOut = kNone;
		else if (sTransformType == "fade" || sTransformType == "1")
			mTransformOut = kFade;
		else if (sTransformType == "slideleft" || sTransformType == "2")
			mTransformOut = kSlideLeft;
		else if (sTransformType == "slideright" || sTransformType == "3")
			mTransformOut = kSlideRight;
		else if (sTransformType == "slideup" || sTransformType == "4")
			mTransformOut = kSlideUp;
		else if (sTransformType == "slidedown" || sTransformType == "5")
			mTransformOut = kSlideDown;
	}

	if (pDialogNode->HasAttribute(ksTransformOutTime))
		mnTransformOutTime = SH::ToInt( pDialogNode->GetAttribute(ksTransformOutTime));

	if (pDialogNode->HasAttribute(ksElementDrawBackground))
		mbDrawDefaultBackground = SH::ToBool(pDialogNode->GetAttribute(ksElementDrawBackground));

	if (pDialogNode->HasAttribute(ksElementBackgroundColor))
	{
		mnBackgroundColor = (uint32_t) SH::ToInt(pDialogNode->GetAttribute(ksElementBackgroundColor));
		mbFillBackground = true;
	}

	// Children
	tXMLNodeList elementNodeList;
	pDialogNode->GetChildren(elementNodeList);

	for (tXMLNodeList::iterator it = elementNodeList.begin(); it != elementNodeList.end(); it++)
	{
		ZXMLNode* pChild = *it;
		ProcessNode(pChild);
	}

	return true;
}

bool ZWinScriptedDialog::ProcessNode(ZXMLNode* pNode)
{
	ZASSERT(pNode);

	// Find the next component
	string sComponent = pNode->GetName();

	// Set the window name if there is one
    msWinName = pNode->GetAttribute(ksElementWinName);

	if (sComponent == ksElementTextWin)
	{
		ZWinFormattedDoc* pWin = new ZWinFormattedDoc();
		pWin->InitFromXML(pNode);
		ChildAdd(pWin);
	}
	else if (sComponent == ksElementButton)
	{
		ZWinSizablePushBtn* pBtn = new ZWinSizablePushBtn();
		pBtn->InitFromXML(pNode);
		ChildAdd(pBtn);
		bool bArrangedButton = pNode->GetAttribute(ksAreaTag).empty();	// if there is no area specified, then this button is auto-arranged
		if (bArrangedButton)
			mArrangedMessageBoxButtonList.push_back(pBtn);
	}
	else if (sComponent == ksElementImage)
	{
		ZWinImage* pWinImage = new ZWinImage();
		if (pWinImage->InitFromXML(pNode))
		{
			return ChildAdd(pWinImage);
		}
		else
		{
			ZASSERT(false);
			delete pWinImage;
		}
	}
	else
	{
		ZASSERT_MESSAGE(false, string("Unknown Component \"" + sComponent + "\"").c_str() );     // unknown command encountered
		return false;      
	}

	return false;
}

bool ZWinScriptedDialog::HandleMessage(const ZMessage& message)
{
	return ZWin::HandleMessage(message);
}
