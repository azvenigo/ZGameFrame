#pragma once
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Resources include both default assets for ZFramework operation as well as a runtime manager for any custom loaded assets


#include <memory>

class ZBuffer;


// Default configuration
extern uint32_t     gDefaultDialogFill;


// Application
extern ZRect		grFullArea;
extern bool			gbFullScreen;

// Dialog
extern std::shared_ptr<ZBuffer>		gDefaultDialogBackground;
extern std::shared_ptr<ZBuffer>		gDimRectBackground;
extern ZRect		grTextArea;


// Buttons
extern std::shared_ptr<ZBuffer>		gStandardButtonUpEdgeImage;
extern std::shared_ptr<ZBuffer>		gStandardButtonDownEdgeImage;
extern ZRect		                grStandardButtonEdge;


// Control Panel
extern ZRect        grControlPanelArea;
extern ZRect        grControlPanelTrigger;
extern int64_t      gnControlPanelButtonHeight;
extern int64_t      gnControlPanelEdge;

// Slider
extern std::shared_ptr<ZBuffer>		gSliderThumb;
extern std::shared_ptr<ZBuffer>		gSliderBackground;
extern ZRect                        grSliderBgEdge;
extern ZRect                        grSliderThumbEdge;


typedef std::map<std::string, std::shared_ptr<ZBuffer> >	tBufferResourceMap;

class cResources
{
public:
	cResources();
	~cResources();

	bool		                Init(const std::string& sDefaultResourcePath);
	bool		                Shutdown();

    std::shared_ptr<ZBuffer>	GetBuffer(const std::string& sName);

	tBufferResourceMap		    mBufferResourceMap;

protected:
	bool		                AddResource(const std::string& sName, std::shared_ptr<ZBuffer> buffer);
};

extern cResources	gResources;
