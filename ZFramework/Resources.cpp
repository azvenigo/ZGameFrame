#include "ZTypes.h"
#include "Resources.h"
#include "ZBuffer.h"
#include "helpers/Registry.h"

using namespace std;

cResources	gResources;

tZBufferPtr		gDefaultDialogBackground;
tZBufferPtr		gSliderThumbHorizontal;
tZBufferPtr		gSliderThumbVertical;
tZBufferPtr		gSliderBackground;
tZBufferPtr		gDimRectBackground;
tZBufferPtr		gStandardButtonUpEdgeImage;
tZBufferPtr		gStandardButtonDownEdgeImage;






/*
uint32_t        gDefaultDialogFill(0xff575757);
uint32_t        gDefaultTextAreaFill(0xff888888);
uint32_t        gDefaultSpacer(16);
uint32_t        gnDefaultGroupInlaySize(8);
ZRect           grDefaultDialogBackgroundEdgeRect(3,3,53,52);
*/


ZRect			grSliderBgEdge(16, 16, 16 *3, 16 *3);
ZRect			grSliderThumbEdge(9, 8, 43, 44);
//ZRect			grDimRectEdge(4, 4, 102, 103);
ZRect			grDimRectEdge(16, 16, 16 *3, 16 *3);
ZRect			grStandardButtonEdge(5, 5, 50, 50);

ZRect           grControlPanelArea;
ZRect           grControlPanelTrigger;
int64_t         gnControlPanelButtonHeight;
int64_t         gnControlPanelEdge;

extern ZGUI::Palette gAppPalette;

cResources::cResources()
{
}

cResources::~cResources()
{
}

bool cResources::Init(const string& sDefaultResourcePath)
{
	bool bResult = true;


    // Try and set the default palette....if false, it has already been persisted
    if (gRegistry.Contains("resources", "palette"))
        gAppPalette = ZGUI::Palette(gRegistry.GetValue("resources", "palette"));
    else
        gRegistry.SetDefault("resources", "palette", (string)gAppPalette);


    // Adjust font sizes based on screen resolution


    gDefaultDialogBackground.reset(new ZBuffer());
    bResult &= AddResource(sDefaultResourcePath + "dialog_edge.png", gDefaultDialogBackground);
    assert(bResult);


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


	return bResult;
}

bool cResources::Shutdown()
{
    gRegistry["resources"]["palette"] = (string)gAppPalette;

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

    ZOUT_LOCKLESS("cResources::Shutdown() complete");

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

