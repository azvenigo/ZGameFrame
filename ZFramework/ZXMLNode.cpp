#include "ZDebug.h"
#include "ZXMLNode.h"
#include "ZStringHelpers.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
using namespace std;


ZXML::ZXML()
{
}

ZXML::~ZXML()
{
	for (tXMLNodeList::iterator it = mChildren.begin(); it != mChildren.end(); it++)
	{
		ZXML* pChild = *it;
		delete pChild;
	}
}

bool ZXML::Init(const string& sRaw)
{
	for (tXMLNodeList::iterator it = mChildren.begin(); it != mChildren.end(); it++)
	{
		ZXML* pChild = *it;
		delete pChild;
	}
	mChildren.clear();

	bool bParsed = Parse(sRaw);
	ZASSERT(bParsed);

	return bParsed;
}

void ZXML::SetName(const string& sName)
{
	if (sName[0] == '\"' || sName[0] == '\'')
	{
		if (sName[sName.length()-1] != sName[0])
		{
			ZASSERT(false);
			return;
		}

		msName = sName.substr(1, sName.length()-2);		// Strip the starting and ending chars
		ZASSERT(msName[0] != '\"' && msName[msName.length()-1] != '\"');

		return;
	} 

	msName = sName;
}


string ZXML::GetAttribute(const string& sKey) const
{
	tKeyValueMap::const_iterator it = mAttributes.find(sKey);
	if (it != mAttributes.end())
		return SH::URL_Decode((*it).second);

	return "";
}

bool ZXML::HasAttribute(const string& sKey) const
{
	tKeyValueMap::const_iterator it = mAttributes.find(sKey);
	if (it != mAttributes.end())
		return true;

	return false;
}


void ZXML::SetAttribute(const string& sKey, const string& sVal)
{
	if (sVal[0] == '\"' || sVal[0] == '\'')
	{
		if (sVal[sVal.length()-1] != sVal[0])
		{
			ZASSERT(false);
			return;
		}

		mAttributes[sKey] = SH::URL_Encode(sVal.substr(1, sVal.length()-2));
		return;
	}

	mAttributes[sKey] = SH::URL_Encode(sVal);
}

ZXML* ZXML::GetChild(const string& sName) const
{
	for (tXMLNodeList::const_iterator it = mChildren.begin(); it != mChildren.end(); it++)
	{
		ZXML* pChild = *it;
		if (pChild->GetName() == sName)
			return pChild;
	}

	return NULL;
}

ZXML* ZXML::AddChild(const string& sName, bool bUnique)
{
	ZXML* pChild = NULL;
	if (bUnique)
	{
		pChild = GetChild(sName);
		if (pChild)
			return pChild;
	}

	pChild = new ZXML();
	pChild->SetName(sName);
	mChildren.push_back(pChild);

	return pChild;
}


bool ZXML::DeleteChild(ZXML* pNode)
{
	for (tXMLNodeList::iterator it = mChildren.begin(); it != mChildren.end(); it++)
	{
		ZXML* pChild = *it;
		if (pChild == pNode)
		{
			mChildren.erase(it);
			delete pChild;
			return true;
		}
	}

	return false;
}

void ZXML::GetChildren(const string& sName, tXMLNodeList& nodeList)
{
	nodeList.clear();
	for (tXMLNodeList::iterator it = mChildren.begin(); it != mChildren.end(); it++)
	{
		ZXML* pChild = *it;
		if (pChild->GetName() == sName)
			nodeList.push_back(pChild);
	}
}

void ZXML::GetChildren(tXMLNodeList& nodeList)
{
	nodeList = mChildren;
}


bool ZXML::Parse(const string& sRaw)
{
	// Find first non whitespace char.
	int64_t nFindOffset = 0;
	while (std::isspace(sRaw[nFindOffset]))
		nFindOffset++;

	// If this is a single named node:
	if (sRaw[nFindOffset] != '<')
	{
		SetText(sRaw);
		return true;
	}

	bool bFinished = false;

	while (!bFinished)
	{
//		int64_t nEndStartTag = sRaw.find(">", nFindOffset);		// find end of <START>
		int64_t nEndStartTag = FindSubstring(sRaw, ">", nFindOffset, true);		// find the '>' ignoring quoted sections
		ZASSERT_MESSAGE(nEndStartTag >= 0, string("Couldn't find end of the start tag: " + sRaw).c_str());

		ZXML* pNewChild = new ZXML();

		string sStartTag = sRaw.substr(nFindOffset+1, nEndStartTag-nFindOffset-1);		// skip the initial '<'
		if (sStartTag.substr(sStartTag.length()-1) == "/")
		{
			// Attribute only node
			sStartTag = sStartTag.substr(0, sStartTag.length()-1);
			pNewChild->ParseStartTag(sStartTag);

			nFindOffset = nEndStartTag+1;
		}
		else
		{
			pNewChild->ParseStartTag(sStartTag);

			// Find End Tag

			// If the node name contains a space, it must be surrounded by quotes
			string sName = pNewChild->GetName();
			if (SH::ContainsWhitespace(sName))
				sName = "\"" + sName + "\"";

			string sEndTag = "</" + sName + ">";
			//int64_t nStartEndTag = sRaw.find(sEndTag);
			int64_t nStartEndTag = FindSubstring(sRaw, sEndTag, nEndStartTag, true);		// find the start of the endtag ignoring quoted sections
			ZASSERT_MESSAGE(nStartEndTag >= 0, string("Couldn't find end tag:" + sEndTag).c_str());

			// Parse everything inside recursively
			string sContents = sRaw.substr(nEndStartTag+1, nStartEndTag - nEndStartTag-1);

			pNewChild->Parse(sContents);

			nFindOffset = nStartEndTag + sEndTag.length();
		}

		mChildren.push_back(pNewChild);

		// Look for another element
//		nFindOffset = sRaw.find("<", nFindOffset);
		nFindOffset = FindSubstring(sRaw, "<", nFindOffset, true);	// find another element, ignoring quoted sections
		if (nFindOffset < 0)
			bFinished = true;
	}

	return true;
}

std::list<string> ZXML::SplitAttributes(string sAttributes)
{
	std::list<string> attributeList;

	uint32_t nIndex = 0;
	bool bDone = false;

	while (!bDone)
	{
		// skip whitespaces
		while (std::isspace(sAttributes[nIndex]) && nIndex < sAttributes.length())
			nIndex++;

		sAttributes = sAttributes.substr(nIndex);
		nIndex = 0;

		// All done?
		if (sAttributes.empty())
			return attributeList;


		while (!std::isspace(sAttributes[nIndex]) && sAttributes[nIndex] != '\"' && nIndex < sAttributes.length())
		{
			nIndex++;
		}

		if (sAttributes[nIndex] == '\"')
		{
			// Skip the quote
			nIndex++;

			// find the end quote
			while (sAttributes[nIndex] != '\"' && nIndex < sAttributes.length())
				nIndex++;

			ZASSERT_MESSAGE(sAttributes[nIndex] == '\"', string("End '\"' not found in attributes:\"" + sAttributes + "\"" ).c_str());
			// Skip the quote
			if (sAttributes[nIndex == '\"'])
				nIndex ++;
		}

        if (sAttributes[nIndex] == '\'')
        {
            // Skip the quote
            nIndex++;

            // find the end quote
            while (sAttributes[nIndex] != '\'' && nIndex < sAttributes.length())
                nIndex++;

            ZASSERT_MESSAGE(sAttributes[nIndex] == '\'', string("End '\'' not found in attributes:\"" + sAttributes + "\"").c_str());
            // Skip the quote
            if (sAttributes[nIndex == '\''])
                nIndex++;
        }



		string sKeyValuePair = sAttributes.substr(0, nIndex); // Left
		attributeList.push_back(sKeyValuePair);

		sAttributes = sAttributes.substr(nIndex);	// ClipLeft
		nIndex = 0;

		if (sAttributes.empty())
			bDone = true;
	}

	return attributeList;
}

void ZXML::ParseStartTag(const string& sStartTag)
{
	std::list<string> attributeList = SplitAttributes(sStartTag);

	ZASSERT_MESSAGE(attributeList.size() >= 1, string("Start tag:\"" + sStartTag + "\" doesn't contain at least one attribute").c_str());

	std::list<string>::iterator it = attributeList.begin();
	SetName(*it);

	for (it++; it != attributeList.end(); it++)
	{
		ParseKeyValuePair(*it);
	}
}

void ZXML::ParseKeyValuePair(const string& sKeyValuePair)
{
	int64_t nEquals = sKeyValuePair.find('=');
	ZASSERT_MESSAGE(nEquals > 0, string("Parsing Key/Value pair failed to find '=': " + sKeyValuePair).c_str());
	string sKey = sKeyValuePair.substr(0, nEquals);
	//string sValue = sKeyValuePair.Right(sKeyValuePair.length() - nEquals - 1);
	string sValue = sKeyValuePair.substr(nEquals+1);  // test that it grabs everything after equals

	ZASSERT(!StartsWith(sKey, " ") && 
		     !EndsWith(sKey, " ") &&
			 !StartsWith(sValue, " ") &&
			 !EndsWith(sValue, " "));

	if (SH::StartsWith(sValue, "\""))
	{
		ZASSERT_MESSAGE(EndsWith(sValue, "\""), "Key value pair has no ending quote.");
		sValue = sValue.substr(1, sValue.length()-2);		// clip left and right quotes

		ZASSERT(sValue[0] != '\"' && sValue[sValue.length()-1] != '\"');
	}

	ZASSERT_MESSAGE(mAttributes.find(sKey) == mAttributes.end(), string("Attribute for key:" + sKey + " already mapped.").c_str());
	mAttributes[sKey] = sValue;
}

string MakeTabs(int64_t nTabs)
{
	string sReturned;
	while (nTabs-- > 0)
		sReturned += "\t";

	return sReturned;
}

string ZXML::ToString(int64_t nDepth)
{
	string sReturned;

	if (msName.empty())
	{
		for (tXMLNodeList::iterator childrenIt = mChildren.begin(); childrenIt != mChildren.end(); childrenIt++)
		{
			sReturned += (*childrenIt)->ToString(nDepth);
		}

		return sReturned;
	}

	string sNameToString;
	if (SH::ContainsWhitespace(msName))
		sNameToString = "\"" + msName + "\"";
	else
		sNameToString = msName;


	bool bHaveChildren = mChildren.size() > 0;
	if (bHaveChildren)
	{
		sReturned += "<" + sNameToString + " ";

		for (tKeyValueMap::iterator it = mAttributes.begin(); it != mAttributes.end(); it++)
		{
			string sKey = (*it).first;
			string sValue = (*it).second;
			if (SH::ContainsWhitespace(sValue))
				sReturned += sKey + "=\"" + sValue + "\" ";		// quotes around the value
			else
				sReturned +=  sKey + "=" + sValue + " ";		// no quotes
		}

		sReturned = sReturned.substr(sReturned.length()-1);		// clip the last ' ';

		if (msText.empty())
		{
			sReturned += ">\r\n";
		}
		else
		{
			sReturned += ">\r\n" + MakeTabs(nDepth+1) + msText + "\r\n";
		}

		for (tXMLNodeList::iterator childrenIt = mChildren.begin(); childrenIt != mChildren.end(); childrenIt++)
		{
			sReturned += MakeTabs(nDepth+1) + (*childrenIt)->ToString(nDepth+1) + "\r\n";
		}

		sReturned += MakeTabs(nDepth) + "</" + sNameToString + ">";
	}
	else
	{
		sReturned += "<" + sNameToString + " ";

		for (tKeyValueMap::iterator it = mAttributes.begin(); it != mAttributes.end(); it++)
		{
			string sKey = (*it).first;
			string sValue = (*it).second;
			if (SH::ContainsWhitespace(sValue))
				sReturned += sKey + "=\"" + sValue + "\" ";		// quotes around the value
			else
				sReturned +=  sKey + "=" + sValue + " ";		// no quotes
		}

		sReturned = sReturned.substr(sReturned.length() - 1);		// clip the last ' ';

		if (msText.empty())
			sReturned += "/>";
		else
			sReturned += ">" + msText + "</" + sNameToString + ">";;
	}

	return sReturned;
}

int64_t ZXML::FindSubstring(const string& sStringToSearch, const string& sSubstring, int64_t nStartOffset, bool bIgnoreQuotedPortions)
{
    const char* pSearch = sStringToSearch.data() + nStartOffset;
    const char* pSub = sSubstring.data();

    int64_t nLengthToSearch = sStringToSearch.length() - nStartOffset;
    int64_t nSubstringLength = sSubstring.length();

    const char* pEnd = pSearch + nLengthToSearch - nSubstringLength + 1;

    if (bIgnoreQuotedPortions)
    {
        while (pSearch < pEnd)
        {
            if (*pSearch == '\"')
            {
                pSearch++;		// skip the opening quote
                while (*pSearch != '\"' && pSearch < pEnd)		// find the ending quote
                    pSearch++;
            }
            else
            {
                if (memcmp(pSearch, pSub, nSubstringLength) == 0)
                    return pSearch - sStringToSearch.data();
            }

            pSearch++;
        }
    }
    else
    {
        while (pSearch < pEnd)
        {
            if (memcmp(pSearch, pSub, nSubstringLength) == 0)
                return pSearch - sStringToSearch.data();

            pSearch++;
        }
    }

    return -1;
}

bool ZXML::GetField(const string& sText, const string& sKey, string& sOutput)
{
    string sStartKey("<" + sKey + ">");
    string sEndKey("</" + sKey + ">");

    int64_t nStartOffset = FindSubstring(sText, sStartKey);
    if (nStartOffset == -1)
        return false;

    nStartOffset += sStartKey.length();		// skip the start key

    int64_t nEndOffset = FindSubstring(sText, sEndKey, nStartOffset);
    ZASSERT(nEndOffset != -1);		// start key without an end key
    if (nEndOffset == -1)
        return false;

    sOutput.assign(sText.data() + nStartOffset, nEndOffset - nStartOffset);
    return true;
}
