#pragma once

#include "ZTypes.h"
#include "ZDebug.h"
#include <map>
#include "helpers/StringHelpers.h"
#include <list>


class ZXML;
typedef std::list<ZXML*> tXMLNodeList;

class ZXML
{
public:
	ZXML();
	//cCEXMLNode(const string& sRaw);
	~ZXML();

	bool                Init(const std::string& sRaw);

    std::string         GetName() { return msName; }
	void                SetName(const std::string& sName);

    std::string         GetText() { return msText; }
	void                SetText(const std::string& sText) { msText = sText; }

    std::string         GetAttribute(const std::string& sKey) const;
	void                SetAttribute(const std::string& sKey, const std::string& sVal);
	bool                HasAttribute(const std::string& sKey) const;

	ZXML*               GetChild(const std::string& sName) const;
	ZXML*               AddChild(const std::string& sName, bool bUnique = false);			// if bUnique is true, it will only add a child if one by that name does not exist.  It will return the existing child if it already exists
	bool                DeleteChild(ZXML* pNode);

	void                GetChildren(tXMLNodeList& nodeList);
	void                GetChildren(const std::string& sName, tXMLNodeList& nodeList);

    std::string         ToString(int64_t nDepth = 0);

    static int64_t      FindSubstring(const std::string& sStringToSearch, const std::string& sSubstring, int64_t nStartOffset = 0, bool bIgnoreQuotedPortions = false);
    static bool         GetField(const std::string& sText, const std::string& sKey, std::string& sOutput);
    static std::string  Encode(const std::string& sRaw) { return SH::URL_Encode(sRaw); }
    static std::string  Decode(const std::string& sEncoded) { return SH::URL_Decode(sEncoded); }

protected:
	bool                Parse(const std::string& sRaw);
	void                ParseStartTag(const std::string& sStartTag);
	void                ParseKeyValuePair(const std::string& sKeyValuePair);

	std::list<std::string> SplitAttributes(std::string sAttributes);


    std::string         msName;
    std::string         msText;
	tKeyValueMap        mAttributes;
	tXMLNodeList        mChildren;
};

