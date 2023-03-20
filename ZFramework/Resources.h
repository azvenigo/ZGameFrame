#pragma once
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Resources include both default assets for ZFramework operation as well as a runtime manager for any custom loaded assets


#include <memory>
#include "ZBuffer.h"
#include "ZFont.h"

class ZBuffer;


// Default configuration
extern uint32_t     gDefaultDialogFill;
extern uint32_t     gDefaultTextAreaFill;


// Resolution Dependent
extern uint32_t     gDefaultSpacer;


// Application
extern ZRect		grFullArea;
extern bool			gbFullScreen;

// Dialog
extern tZBufferPtr  gDefaultDialogBackground;
extern tZBufferPtr  gDimRectBackground;
extern ZRect		grTextArea;


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

// Fonts
extern ZFontParams  gDefaultButtonFont;
extern ZFontParams  gDefaultTitleFont;
extern ZFontParams  gDefaultCaptionFont;
extern ZFontParams  gDefaultTextFont;

extern ZFontParams  gDefaultTooltipFont;
extern ZTextLook    gDefaultToolitipLook;
extern uint32_t     gDefaultTooltipFill;



typedef std::map<std::string, tZBufferPtr >	tBufferResourceMap;

class cResources
{
public:
	cResources();
	~cResources();

	bool                Init(const std::string& sDefaultResourcePath);
	bool                Shutdown();

    tZBufferPtr	        GetBuffer(const std::string& sName);

	tBufferResourceMap  mBufferResourceMap;

protected:
	bool                AddResource(const std::string& sName, tZBufferPtr buffer);
};

extern cResources	    gResources;
