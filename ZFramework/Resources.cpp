#include "ZStdTypes.h"
#include "Resources.h"
#include "ZBuffer.h"

using namespace std;

cResources	gResources;

std::shared_ptr<ZBuffer>		gDefaultDialogBackground;
std::shared_ptr<ZBuffer>		gSliderThumb;
std::shared_ptr<ZBuffer>		gSliderBackground;
std::shared_ptr<ZBuffer>		gDimRectBackground;
std::shared_ptr<ZBuffer>		gStandardButtonUpEdgeImage;
std::shared_ptr<ZBuffer>		gStandardButtonDownEdgeImage;


ZRect			grTextArea;

ZRect			grSliderBgEdge(16, 16, 48, 48);
ZRect			grSliderThumbEdge(16, 16, 48, 48);
//ZRect			grDimRectEdge(4, 4, 102, 103);
ZRect			grDimRectEdge(16, 16, 48, 48);
ZRect			grStandardButtonEdge(16, 16, 48, 48);

ZRect           grControlPanelArea;
ZRect           grControlPanelTrigger;
int64_t         gnControlPanelButtonHeight;
int64_t         gnControlPanelEdge;


cResources::cResources()
{
}

cResources::~cResources()
{
}

bool cResources::Init(const string& sDefaultResourcePath)
{
	bool bResult = true;
    gSliderBackground.reset(new ZBuffer());
	bResult &= AddResource(sDefaultResourcePath+"slider_bg.png",gSliderBackground);
    assert(bResult);

    gSliderThumb.reset(new ZBuffer());
	bResult &= AddResource(sDefaultResourcePath+"slider_thumb.png",gSliderThumb);
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

	grTextArea.SetRect(16, 16, grFullArea.right - 48, grFullArea.bottom - 96);


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
        std::shared_ptr<ZBuffer> pBuffer = bufferIt.second;
        pBuffer->Shutdown();
	}

	mBufferResourceMap.clear();


    gSliderBackground = nullptr;

    gSliderThumb = nullptr;

    gStandardButtonUpEdgeImage = nullptr;

    gStandardButtonDownEdgeImage = nullptr;


	return true;
}

bool cResources::AddResource(const string& sName, std::shared_ptr<ZBuffer> buffer)
{
	bool bResult = buffer->LoadBuffer(sName);
	ZASSERT(bResult);

    mBufferResourceMap[sName] = buffer;

	return bResult;
}

std::shared_ptr<ZBuffer> cResources::GetBuffer(const string& sName)
{
	tBufferResourceMap::iterator it = mBufferResourceMap.find(sName);
	if (it != mBufferResourceMap.end())
	{
		return (*it).second;
	}

	ZASSERT_MESSAGE(false, "No buffer mapped");
	return NULL;
}

