#include "ZPreferences.h"
#include "ZStringHelpers.h"


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ZPreferences gPreferences;

ZPreferences::ZPreferences()
{
}

ZPreferences::~ZPreferences()
{
}

bool ZPreferences::Load(const string& sFilename)
{
	msFilename = sFilename;

	string sRaw;
#ifdef _DEBUG
	if (ReadStringFromFile(msFilename + ".txt", sRaw))
		return mPreferenceTree.Init(sRaw);
#endif


	if (ReadStringFromFile(msFilename, sRaw))
		return mPreferenceTree.Init(sRaw);

	return false;
}

bool ZPreferences::Save()
{
	string sToString = mPreferenceTree.ToString();
	if (sToString != msRawXML)
	{
		msRawXML = sToString;
#ifdef _DEBUG
		WriteStringToFile(msFilename + ".txt", msRawXML);
#endif
		return WriteStringToFile(msFilename, msRawXML, true);
	}

	return true;
}