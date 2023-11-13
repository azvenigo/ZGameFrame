#pragma once
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Resources include both default assets for ZFramework operation as well as a runtime manager for any custom loaded assets


#include <memory>
#include "ZBuffer.h"
#include "ZFont.h"
#include "ZGUIHelpers.h"
#include "ZGUIStyle.h"
#include "ZMessageSystem.h"

class ZBuffer;


// Default configuration
/*extern uint32_t     gDefaultDialogFill;
extern uint32_t     gDefaultTextAreaFill;


// Resolution Dependent
extern uint32_t     gDefaultSpacer;
extern uint32_t     gnDefaultGroupInlaySize;*/


// Application
extern ZRect		grFullArea;

// Dialog
extern tZBufferPtr  gDefaultDialogBackground;
extern tZBufferPtr  gDimRectBackground;
extern ZRect        grDefaultDialogBackgroundEdgeRect;


// Buttons
extern tZBufferPtr  gStandardButtonUpEdgeImage;
extern tZBufferPtr  gStandardButtonDownEdgeImage;
extern ZRect        grStandardButtonEdge;


// Control Panel
extern ZRect        grControlPanelArea;
extern ZRect        grControlPanelTrigger;
extern int64_t      gnControlPanelButtonHeight;
extern int64_t      gnControlPanelEdge;

// Slider
extern tZBufferPtr  gSliderThumbHorizontal;
extern tZBufferPtr  gSliderThumbVertical;
extern tZBufferPtr  gSliderBackground;
extern ZRect        grSliderBgEdge;
extern ZRect        grSliderThumbEdge;

typedef std::map<std::string, tZBufferPtr >	tBufferResourceMap;

class cResources : public IMessageTarget
{
public:
	cResources();
	~cResources();

	bool                Init(const std::string& sDefaultResourcePath);
	bool                Shutdown();

    tZBufferPtr	        GetBuffer(const std::string& sName);

	tBufferResourceMap  mBufferResourceMap;

    std::string         GetTargetName() { return "ZResources"; }
    bool                ReceiveMessage(const ZMessage& message);


protected:
	bool                AddResource(const std::string& sName, tZBufferPtr buffer);
    bool                ColorizeResources(uint32_t nCol);
};

extern cResources	    gResources;
