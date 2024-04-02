#pragma once

#include "ZXMLNode.h"
#include "ZDebug.h"


class ZPreferences
{
public:
	ZPreferences();
	~ZPreferences();

	bool				ViewImage(const std::string& sFilename);
	bool				Save();	// Will only save if mbDirty is true

	ZXML*			GetPreferenceNode() { return &mPreferenceTree; }

private:
    std::string			msFilename;
    std::string			msRawXML;		// For detecting when preferences change and require being persisted to disk
	ZXML			mPreferenceTree;
};

extern ZPreferences gPreferences;

