#include "ZStringHelpers.h"
#include "ZStdDebug.h"
#include <string>
#include <locale>
#include <fstream>
#include <stdio.h>
#include "ZCompression.h"
#include "ZBuffer.h"
#include "ZZipAPI.h"

const char* ksEncodedStringTag = "ESbyAZ";


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void MakeLower(string& sVal)
{
	char* pChar = (char*)sVal.data();

	for (size_t i = 0; i < sVal.length() && pChar; i++)
		if (pChar[i] && pChar[i] >= 'A' && pChar[i] <= 'Z')
			pChar[i] = pChar[i] + 'a' - 'A';
}

void MakeUpper(string& sVal)
{
	char* pChar = (char*)sVal.data();

	for (size_t i = 0; i < sVal.length() && pChar; i++)
		if (pChar[i] && pChar[i] >= 'a' && pChar[i] <= 'z')
			pChar[i] = pChar[i] + 'A' - 'a';
}


bool	StringToBool(string sVal)
{
	MakeLower(sVal);
	return sVal == "1" || sVal == "true" || sVal == "on" || sVal == "yes" || sVal == "y";
}


string	BoolToString(bool bVal)
{
	if (bVal)
		return "1";

	return "0";
}

int64_t		StringToInt(const string& sVal)
{
	return atoll(sVal.c_str());
}

string	IntToString(int64_t nVal)
{
	char buf[32];
	sprintf_s(buf, "%lld", nVal);

	return string(buf);
}


uint32_t StringToHex(const string& sVal)
{
	return strtoul(sVal.c_str(), NULL, 16);
}

string HexToString(uint32_t nVal)
{
	char buf[32];
	sprintf_s(buf, "%x", nVal);

	return string(buf);
}

double StringToDouble(const string& sVal)
{
	return strtod(sVal.c_str(), NULL);
}

string	DoubleToString(double fVal)
{
	char buf[32];
	sprintf_s(buf, "%f", (float) fVal);

	return string(buf);
}

ZPoint StringToPoint(const string& sVal)
{
	std::vector<int64_t> intArray;
	StringToInt64Array(sVal, intArray);
	ZASSERT_MESSAGE(intArray.size() == 2, string("Can't extract exactly 2 values from:\"" + sVal + "\"").c_str());

	return ZPoint(intArray[0], intArray[1]);
}

string PointToString(const ZPoint& pointValue)
{
	std::vector<int64_t> intArray;
	intArray.reserve(2);
	intArray.push_back(pointValue.mX);
	intArray.push_back(pointValue.mY);

	return Int64ArrayToString(intArray);
}

ZRect StringToRect(const string& sVal)
{
	std::vector<int64_t> intArray;
	StringToInt64Array(sVal, intArray);
	ZASSERT_MESSAGE(intArray.size() == 4, string("Can't extract exactly 4 values from:\"" + sVal + "\"").c_str());

	return ZRect(intArray[0], intArray[1], intArray[2], intArray[3]);
}

string RectToString(const ZRect& rectValue)
{
	std::vector<int64_t> intArray;
	intArray.push_back(rectValue.left);
	intArray.push_back(rectValue.top);
	intArray.push_back(rectValue.right);
	intArray.push_back(rectValue.bottom);

	return Int64ArrayToString(intArray);
}


void StringToStringArray(string sVal, std::vector<string>& stringArray)
{
	std::size_t current, previous = 0;

	current = sVal.find(',');
	while (current != std::string::npos) 
	{
		stringArray.push_back(sVal.substr(previous, current - previous));
		previous = current + 1;
		current = sVal.find(',', previous);
	}
	stringArray.push_back(sVal.substr(previous, current - previous));
}


string StringArrayToString(std::vector<string>& stringArray)
{
	string sValue;
	for (uint32_t i = 0; i < stringArray.size(); i++)
	{
		ZASSERT_MESSAGE( stringArray[i].find(',') == -1, "Converting to a string array doesn't support strings with commas right now.");
		if (!stringArray[i].empty())
			sValue += stringArray[i] + ",";
	}

	if (!sValue.empty())
		sValue = sValue.substr( sValue.length()-1 );	// remove the trailing ','

	return sValue;
}


void StringToStringMap(string stringToSplit, tKeyValueMap& stringMap)
{
	std::size_t current, previous = 0;

	current = stringToSplit.find(',');
	while (current != std::string::npos)
	{
		std::size_t equalIndex = stringToSplit.find('=', previous);
		if (equalIndex != std::string::npos)
		{
			string sKey(stringToSplit.substr(previous, equalIndex));
			string sVal(stringToSplit.substr(equalIndex + 1, current));
			stringMap[sKey] = sVal;
		}

		previous = current + 1;
		current = stringToSplit.find(',', previous);
	}

	// final value
	std::size_t equalIndex = stringToSplit.find('=', previous);
	if (equalIndex != std::string::npos)
	{
		string sKey(stringToSplit.substr(previous, equalIndex));
		string sVal(stringToSplit.substr(equalIndex + 1, current));
		stringMap[sKey] = sVal;
	}
}


string		StringMapToString(tKeyValueMap& stringMap)
{
	string sReturn;
	for (tKeyValueMap::iterator it = stringMap.begin(); it != stringMap.end(); it++)
	{
		string sKey = (*it).first;
		string sValue = (*it).second;

		ZASSERT_MESSAGE( sKey.find(',') == -1, "Converting to a string array doesn't support strings with commas right now.");
		ZASSERT_MESSAGE( sValue.find(',') == -1, "Converting to a string array doesn't support strings with commas right now.");
		ZASSERT_MESSAGE( sKey.find('=') == -1, "Converting to a string array doesn't support strings with equals right now.");
		ZASSERT_MESSAGE( sValue.find('=') == -1, "Converting to a string array doesn't support strings with equals right now.");
		if (!sKey.empty())
			sReturn += sKey + "=" + sValue + ",";
	}

	if (!sReturn.empty())
		sReturn = sReturn.substr( sReturn.length()-1 );	// remove the trailing ','

	return sReturn;
}


void StringToInt64Array(const string& sVal, std::vector<int64_t>& intArray)
{
    char* pNext;

#ifdef _WIN64
    char* pVal = strtok_s((char*)sVal.c_str(), ",", &pNext);
    while (pVal)
    {
        intArray.push_back(strtol(pVal, nullptr, 10));
        pVal = strtok_s(nullptr, ",", &pNext);
    }
#else
    rsize_t nRemainingChars = sVal.length();
    char* pVal = strtok_s((char*) sVal.data(), &nRemainingChars,  ",", &pNext);
    while (pVal)
    {
        intArray.push_back(strtol(pVal, nullptr, 10));
        pVal = strtok_s(nullptr, 0, ",", &pNext);
    }
#endif
}


string Int64ArrayToString(std::vector<int64_t>& intArray)
{
	string sValue;
	char buf[32];

	for (uint32_t i = 0; i < intArray.size(); i++)
	{
		sprintf_s(buf, "%lld,", intArray[i]);

		sValue.append(buf);
	}

	if (!sValue.empty())
		sValue = sValue.substr(0, sValue.length()-1 );	// remove the trailing ','

	return sValue;
}

bool StringToBinary(const string& sString, void* pDest)
{
	// string must be of even length
	if (sString.length()%2 != 0)
	{
		ZASSERT(false);
		return false;
	}

	int64_t nBinaryLength = sString.length()/2;

	for (int64_t i = 0; i < nBinaryLength; i++)
	{
		uint8_t b1 = sString[i*2] - 'A';
		uint8_t b2 = sString[i*2+1] - 'A';

		if (b1 >= 16)
		{
			ZASSERT(false);
			return false;
		}

		if (b2 >= 16)
		{
			ZASSERT(false);
			return false;
		}

		uint8_t val = (b1 << 4 | b2) ^ 149;
		((uint8_t*) pDest)[i] = val;
	}

	return true;
}

bool BinaryToString(void* pSource, int64_t nLength, string& sString)
{
	sString.clear();
	for (int64_t i = 0; i < nLength; i++)
	{
		uint8_t val = (*((uint8_t*) pSource + i)) ^ 149;
		uint8_t b1 = (val>>4) + 'A';
		uint8_t b2 = (val&0x0f) + 'A';

		sString += (char) b1 + (char) b2;
	}

	return true;
}

#pragma warning(disable:4244)

string	WideToAscii(const std::wstring& sVal)
{
    return string(  sVal.begin(), sVal.end() );
}

std::wstring AsciiToWide(const string& sVal)
{
    return wstring( sVal.begin(), sVal.end() );
}



int64_t FindSubstring(const string& sStringToSearch, const string& sSubstring, int64_t nStartOffset, bool bIgnoreQuotedPortions)
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

bool GetField(const string& sText, const string& sKey, string& sOutput)
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

bool WriteStringToFile(const string& sFilename, const string& sString, bool bCompressEncode)
{
#ifdef _DEBUG
	int64_t nSub = FindSubstring(sString, "<<", 0, true);
	ZASSERT(nSub < 0);
#endif

//	HANDLE hFile = CreateFile(AsciiToWide(sFilename).c_str(), GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    std::ofstream stringFile(sFilename.c_str(), ios::out | ios::binary | ios::trunc);
    if (!stringFile.is_open())
    {
        ZOUT("Failed to open file \"%s\"\n", sFilename.c_str());
        return false;
    }


//	CEASSERT_MESSAGE(hFile != INVALID_HANDLE_VALUE, string(string("Couldn't open file \"") + sFilename + string("\" for writing.  Error:") + IntToString(GetLastError())).c_str());
    unsigned char* pData = (unsigned char*) sString.data();
	uint32_t nLength = (uint32_t) sString.length();

    ZMemBufferPtr stringBuf(new ZMemBuffer(nLength));
    stringBuf->write(pData, nLength);
    

	if (bCompressEncode)
	{
        ZMemBufferPtr compressedBuf(new ZMemBuffer());

		ZZipAPI::Compress(stringBuf, compressedBuf, true);
//		WriteFile(hFile, ksEncodedStringTag, 6, &nNumWritten, NULL);
        stringFile.write(ksEncodedStringTag, 6);
        stringFile.write((const char*) compressedBuf->data(), compressedBuf->size());
        return true;
	}


//	WriteFile(hFile, pData, (DWORD) nLength, &nNumWritten, NULL);
    stringFile.write((char*) pData, nLength);


	return true;
}

bool ReadStringFromFile(const string& sFilename, string& sResult)
{
//	HANDLE hFile = CreateFile(AsciiToWide(sFilename).c_str(), GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    std::ifstream stringFile(sFilename.c_str(), ios::in | ios::binary | ios::ate );
    if (!stringFile.is_open())
    {
        ZOUT("Failed to open file \"%s\"\n", sFilename.c_str());
        return false;
    }

    uint32_t nLength = (uint32_t) stringFile.tellg();
    stringFile.seekg(0);

    ZMemBufferPtr inputBuffer(new ZMemBuffer(nLength));
    stringFile.read((char*)inputBuffer->data(), nLength);

	if (nLength > 6 && memcmp(inputBuffer->data(), ksEncodedStringTag, 6) == 0)
	{
        inputBuffer->read(6);   // skip the encoded tag

        ZMemBufferPtr outputBuffer(new ZMemBuffer());
		bool bSuccess = ZZipAPI::Decompress(inputBuffer, outputBuffer);
		ZASSERT_MESSAGE(bSuccess, "Failed to decode string.");
		if (bSuccess)
		{
			sResult.assign((char*) outputBuffer->data(), outputBuffer->size());;
		}
	}
	else
	{
		sResult.assign((char*) inputBuffer->data(), inputBuffer->size());
	}

	return true;
}

bool IsAlpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}


bool IsWhiteSpace(char c)
{
	return	c == ' ' || 
		c == '\t' ||
		c == '\r' || 
		c == '\n';
}

bool ContainsWhitespace(const string& sVal)
{
	return sVal.find_first_of(" \t\r\n") != -1;
}

void TrimWhitespace(string& sVal)
{
	int64_t nFirstNonWhiteSpace = 0;
	while (nFirstNonWhiteSpace < (int64_t) sVal.length() && IsWhiteSpace(sVal[nFirstNonWhiteSpace]))
		nFirstNonWhiteSpace++;

	int64_t nLastNonWhiteSpace = (int64_t) sVal.length();
	while (nLastNonWhiteSpace > nFirstNonWhiteSpace && IsWhiteSpace(sVal[nLastNonWhiteSpace]))
		nLastNonWhiteSpace--;

	sVal = sVal.substr(nFirstNonWhiteSpace, nLastNonWhiteSpace-nFirstNonWhiteSpace);
}

string GetHoursMinutesSecondsString(int64_t nSeconds)
{
	int64_t nHours = 0;
	int64_t nMinutes = 0;

	nHours = nSeconds/3600;

	nSeconds -= nHours*3600;

	nMinutes = nSeconds/60;

	nSeconds -= nMinutes*60;

	char buf[32];
	sprintf_s(buf, "%02d:%02d:%02d", (uint32_t) nHours, (uint32_t) nMinutes, (uint32_t) nSeconds);

	return string(buf);
}

void Sprintf(string& sOut, const char* lpszFormat, ...)
{
	va_list args;
	int len;
	char* buf = nullptr;

	va_start(args, lpszFormat);
	len = vsnprintf(buf, 0, (char*) lpszFormat, args) + 1; // terminating '\0'
	buf = new char[len];
	vsprintf_s(buf, len, lpszFormat, args);
	sOut.assign(buf, len);

	delete[] buf;
}


bool StartsWith(const string& sConsider, const string& starts)
{
	return sConsider.substr(0, sConsider.length()) == starts;
}

bool EndsWith(const string& sConsider, const string& ends)
{
	return sConsider.substr(sConsider.length() - ends.length()) == ends;
}


void SplitToken(string& sBefore, string& sAfter, const string& token)
{
	size_t pos = sAfter.find(token);
	if (pos == string::npos)
	{
		sBefore = sAfter;
		sAfter = "";
		return;
	}

	sBefore = sAfter.substr(0, pos).c_str();
	sAfter = sAfter.substr(pos+token.length()).c_str();
}


bool ContainsNonAscii(const string& sString)
{
    for (auto c = sString.begin(); c != sString.end(); c++)
        if (*c <= 0)
            return true;

    return false;
}
