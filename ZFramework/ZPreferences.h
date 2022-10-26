#pragma once

#include "ZXMLNode.h"
#include "ZStdDebug.h"


class ZPreferences
{
public:
	ZPreferences();
	~ZPreferences();

	bool				Load(const std::string& sFilename);
	bool				Save();	// Will only save if mbDirty is true

	ZXMLNode*			GetPreferenceNode() { return &mPreferenceTree; }

private:
    std::string			msFilename;
    std::string			msRawXML;		// For detecting when preferences change and require being persisted to disk
	ZXMLNode			mPreferenceTree;
};

extern ZPreferences gPreferences;

