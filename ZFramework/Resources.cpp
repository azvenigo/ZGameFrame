#include "ZTypes.h"
#include "Resources.h"
#include "ZBuffer.h"

using namespace std;

cResources	gResources;

tZBufferPtr		gDefaultDialogBackground;
tZBufferPtr		gSliderThumbHorizontal;
tZBufferPtr		gSliderThumbVertical;
tZBufferPtr		gSliderBackground;
tZBufferPtr		gDimRectBackground;
tZBufferPtr		gStandardButtonUpEdgeImage;
tZBufferPtr		gStandardButtonDownEdgeImage;


ZRect			grTextArea;
uint32_t        gDefaultDialogFill(0xff575757);
uint32_t        gDefaultTextAreaFill(0xff888888);
uint32_t        gDefaultSpacer(16);

ZRect			grSliderBgEdge(gDefaultSpacer, gDefaultSpacer, gDefaultSpacer*3, gDefaultSpacer*3);
ZRect			grSliderThumbEdge(9, 8, 43, 44);
//ZRect			grDimRectEdge(4, 4, 102, 103);
ZRect			grDimRectEdge(gDefaultSpacer, gDefaultSpacer, gDefaultSpacer*3, gDefaultSpacer*3);
ZRect			grStandardButtonEdge(5, 5, 50, 50);

ZRect           grControlPanelArea;
ZRect           grControlPanelTrigger;
int64_t         gnControlPanelButtonHeight;
int64_t         gnControlPanelEdge;

//Fonts
ZFontParams     gDefaultTitleFont("Gadugi", 40);
//ZFontParams     gDefaultCaptionFont("Gadugi", 30, 400);
ZFontParams     gDefaultTextFont("Gadugi", 20);

ZGUI::Style     gStyleTooltip(ZFontParams("Verdana", 30), ZTextLook(ZTextLook::kShadowed, 0xff000000, 0xff000000), ZGUI::C, 0, gDefaultTextAreaFill);
ZGUI::Style     gStyleCaption(ZFontParams("Gadugi", 36, 400), ZTextLook(ZTextLook::kShadowed, 0xffffffff, 0xffffffff), ZGUI::C, 0, gDefaultDialogFill);
ZGUI::Style     gStyleButton(ZFontParams("Verdana", 30, 600), ZTextLook(ZTextLook::kEmbossed, 0xffffffff, 0xffffffff), ZGUI::C, 0, gDefaultDialogFill);
ZGUI::Style     gStyleToggleChecked(ZFontParams("Verdana", 30, 600), ZTextLook(ZTextLook::kEmbossed, 0xff00ff00, 0xff008800), ZGUI::C, 0, gDefaultDialogFill);
ZGUI::Style     gStyleToggleUnchecked(ZFontParams("Verdana", 30, 600), ZTextLook(ZTextLook::kEmbossed, 0xffffffff, 0xff888888), ZGUI::C, 0, gDefaultDialogFill);
ZGUI::Style     gStyleGeneralText(ZFontParams("Verdana", 30), ZTextLook(ZTextLook::kNormal, 0xffffffff, 0xffffffff), ZGUI::LT, 0, 0, true);
ZGUI::Style     gDefaultDialogStyle(gDefaultTextFont, ZTextLook(), ZGUI::LT, gDefaultSpacer, gDefaultDialogFill, true);


cResources::cResources()
{
}

cResources::~cResources()
{
}

bool cResources::Init(const string& sDefaultResourcePath)
{
	bool bResult = true;

    // Adjust font sizes based on screen resolution
    gDefaultSpacer = grFullArea.Height() / 125;

    gStyleButton.fp.nHeight = grFullArea.Height() / 72;
    gStyleToggleChecked.fp.nHeight = grFullArea.Height() / 72;
    gStyleToggleUnchecked.fp.nHeight = grFullArea.Height() / 72;
    gStyleTooltip.fp.nHeight = grFullArea.Height() / 72;
    gStyleCaption.fp.nHeight = grFullArea.Height() / 60;


    gDefaultTitleFont.nHeight = grFullArea.Height() / 54;
//    gDefaultCaptionFont.nHeight = grFullArea.Height() / 60;
    gDefaultTextFont.nHeight = grFullArea.Height() / 108;

    gSliderBackground.reset(new ZBuffer());
	bResult &= AddResource(sDefaultResourcePath+"slider_bg.png",gSliderBackground);
    assert(bResult);

    gSliderThumbHorizontal.reset(new ZBuffer());
	bResult &= AddResource(sDefaultResourcePath+"slider_thumb_h.png",gSliderThumbHorizontal);
    assert(bResult);

    gSliderThumbVertical.reset(new ZBuffer());
    bResult &= AddResource(sDefaultResourcePath + "slider_thumb_v.png", gSliderThumbVertical);
    assert(bResult);


    gStandardButtonUpEdgeImage.reset(new ZBuffer());
	bResult &= AddResource(sDefaultResourcePath+"btnsizable_up.png", gStandardButtonUpEdgeImage);
    assert(bResult);

    gStandardButtonDownEdgeImage.reset(new ZBuffer());
	bResult &= AddResource(sDefaultResourcePath+"btnsizable_down.png", gStandardButtonDownEdgeImage);
    assert(bResult);

    gDimRectBackground.reset(new ZBuffer());
    bResult &= AddResource(sDefaultResourcePath+"slider_bg.png", gDimRectBackground);
    assert(bResult);

	grTextArea.SetRect(gDefaultSpacer, gDefaultSpacer, grFullArea.right - gDefaultSpacer*3, grFullArea.bottom - gDefaultSpacer*6);


    int64_t controlW = grFullArea.Width() / 10;
    int64_t controlH = grFullArea.Height() / 20;

    grControlPanelArea.SetRect(grFullArea.right - controlW, 0, grFullArea.right, controlH);

    int64_t controlTriggerW = 64;
    int64_t controlTriggerH = 64;

    grControlPanelTrigger.SetRect(grFullArea.right - controlTriggerW, 0, grFullArea.right, controlTriggerH);
    gnControlPanelButtonHeight = grFullArea.Height()/50;
    gnControlPanelEdge = grFullArea.Height()/100;

    
	ZASSERT(bResult);

	return bResult;
}

bool cResources::Shutdown()
{
    for (auto bufferIt : mBufferResourceMap)
	{
        tZBufferPtr pBuffer = bufferIt.second;
        pBuffer->Shutdown();
	}

	mBufferResourceMap.clear();


    gSliderBackground = nullptr;

    gSliderThumbHorizontal = nullptr;
    gSliderThumbVertical = nullptr;

    gStandardButtonUpEdgeImage = nullptr;

    gStandardButtonDownEdgeImage = nullptr;


	return true;
}

bool cResources::AddResource(const string& sName, tZBufferPtr pBuffer)
{
	bool bResult = pBuffer->LoadBuffer(sName);
	ZASSERT(bResult);

    mBufferResourceMap[sName] = pBuffer;

	return bResult;
}

tZBufferPtr cResources::GetBuffer(const string& sName)
{
	tBufferResourceMap::iterator it = mBufferResourceMap.find(sName);
	if (it != mBufferResourceMap.end())
	{
		return (*it).second;
	}

	ZASSERT_MESSAGE(false, "No buffer mapped");
	return NULL;
}

