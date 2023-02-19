#include "ZStringHelpers.h"
#include "ZStdDebug.h"
#include <string>
#include <locale>
#include <fstream>
#include <stdio.h>
#include "ZCompression.h"
#include "ZXMLNode.h"
#include "ZBuffer.h"
#include "ZZipAPI.h"

const char* ksEncodedStringTag = "ESbyAZ";


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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


void StringToInt64Array(const string& sVal, std::vector<int64_t>& intArray)
{
    char* pNext;

#ifdef _WIN64
    char* pVal = strtok_s((char*)sVal.c_str(), &StringHelpers::kCharSplitToken, &pNext);
    while (pVal)
    {
        intArray.push_back(strtol(pVal, nullptr, 10));
        pVal = strtok_s(nullptr, &StringHelpers::kCharSplitToken, &pNext);
    }
#else
    rsize_t nRemainingChars = sVal.length();
    char* pVal = strtok_s((char*) sVal.data(), &nRemainingChars, &StringHelpers::kCharSplitToken, &pNext);
    while (pVal)
    {
        intArray.push_back(strtol(pVal, nullptr, 10));
        pVal = strtok_s(nullptr, 0, &StringHelpers::kCharSplitToken, &pNext);
    }
#endif
}


string Int64ArrayToString(std::vector<int64_t>& intArray)
{
	string sValue;
	char buf[32];

	for (uint32_t i = 0; i < intArray.size(); i++)
	{
		sprintf_s(buf, "%lld%c", intArray[i], StringHelpers::kCharSplitToken);

		sValue.append(buf);
	}

	if (!sValue.empty())
		sValue = sValue.substr(0, sValue.length()-1 );	// remove the trailing kCharSplitToken

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



bool WriteStringToFile(const string& sFilename, const string& sString, bool bCompressEncode)
{
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
    inputBuffer->seekp(nLength);

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
	sOut.assign(buf, len-1);    // minus '\0' terminator

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

/*std::string SanitizeAscii(const std::string& sText)
{
    std::string sSanitized(sText);
    
    for (size_t i = 0; i < sSanitized.length(); i++)
    {
        if (sSanitized[i] > 127 || sSanitized[i] < 0)
            sSanitized[i] = '_';
    }
    return sSanitized;
}*/
