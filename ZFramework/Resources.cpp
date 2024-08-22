#include "ZTypes.h"
#include "Resources.h"
#include "ZBuffer.h"
#include "helpers/Registry.h"
#include "helpers/StringHelpers.h"
#include "ZWinPaletteDialog.h"
#include "lunasvg.h"
#include "ZColor.h"


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
ZRect			grDimRectEdge(4, 4, 60, 60);
ZRect			grStandardButtonEdge(5, 5, 50, 50);
ZRect           grDefaultDialogBackgroundEdgeRect(3, 3, 53, 52);


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

    // If there is a persisted palette, retrieve it
    ZGUI::Palette persisted;
    if (gRegistry.Contains("resources", "palette"))
        persisted = ZGUI::Palette(gRegistry.GetValue("resources", "palette"));

    // if number of colors persisted doesn't match baked in, then toss out persisted and readd
    if (persisted.mColorMap.size() != gAppPalette.mColorMap.size())
    {
        gRegistry.Remove(string("resources"), string("palette"));
        gRegistry.SetDefault("resources", "palette", (string)gAppPalette);
    }
    else
        gAppPalette = persisted;



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

    uint32_t nCol = 0;
    if (gAppPalette.GetByName("Dialog Fill", nCol))
        ColorizeResources(nCol);


    gMessageSystem.AddNotification("colorize", this);
    gMessageSystem.AddNotification("show_app_palette", this);
    
	return bResult;
}

bool cResources::Shutdown()
{
    gMessageSystem.RemoveAllNotifications(this);
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

bool cResources::ColorizeResources(uint32_t nCol)
{
    uint32_t nHSV = COL::ARGB_To_AHSV(nCol);
    uint32_t nH = AHSV_H(nHSV);
    uint32_t nS = AHSV_S(nHSV);

    for (auto& resource : mBufferResourceMap)
    {
        resource.second->Colorize(nH, nS);
    }
    return true;
}


bool cResources::ReceiveMessage(const ZMessage& message)
{
    string sType = message.GetType();

    if (sType == "colorize")
    {
        ColorizeResources((uint32_t)SH::ToInt(message.GetParam("col")));
        return true;
    }
    else if (sType == "show_app_palette")
    {
        ZWinPaletteDialog::ShowPaletteDialog("Application Color Palette", &gAppPalette.mColorMap, ZMessage("app_restart"));
        return true;
    }

    return false;
}

