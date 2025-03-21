#pragma once

#include "ZTypes.h"
#include <string>
#include <vector>
#include <ZMemBuffer.h>
#include "helpers/StringHelpers.h"

ZPoint      StringToPoint(const std::string& sVal);
std::string PointToString(const ZPoint& pointValue);

ZFPoint     StringToFPoint(const std::string& sVal);
std::string FPointToString(const ZFPoint& pointValue);

ZRect       StringToRect(const std::string& sVal);
std::string RectToString(const ZRect& rectValue);

void        StringToInt64Array(const std::string& sVal, std::vector<int64_t>& intArray);
std::string Int64ArrayToString(std::vector<int64_t>& intArray);

void        StringToFloatArray(const std::string& sVal, std::vector<float>& floatArray);
std::string FloatArrayToString(std::vector<float>& floatArray);


//std::string StringMapToString(tKeyValueMap& stringMap);
//void        StringToStringMap(std::string sVal, tKeyValueMap& stringMap);

bool        StringToBinary(const std::string& sString, void* pDest);		// std::string must be in ascii 2 hex values per byte of destination..... so pDest must point to a buffer of sVal.length()/2
bool        BinaryToString(void* pSource, int64_t nLength, std::string& sString);


bool        WriteStringToFile(const std::string& sFilename, const std::string& sString, bool bCompressEncode = false);
bool        ReadStringFromFile(const std::string& sFilename, std::string& sResult);

void        Sprintf(std::string& sOut, const char* format, ...);
void        Sprintf(std::string& sOut, const char* format, va_list args);

// Parsers


enum
{
    kPwr_1b = 0, kPwr_2b, kPwr_4b, kPwr_8b, kPwr_16b, kPwr_32b, kPwr_64b, kPwr_128b, kPwr_256b, kPwr_512b,
    kPwr_1KB = 10, kPwr_2KB, kPwr_4KB, kPwr_8KB, kPwr_16KB, kPwr_32KB, kPwr_64KB, kPwr_128KB, kPwr_256KB, kPwr_512KB,
    kPwr_1MB = 20, kPwr_2MB, kPwr_4MB, kPwr_8MB, kPwr_16MB, kPwr_32MB, kPwr_64MB, kPwr_128MB, kPwr_256MB, kPwr_512MB,
    kPwr_1GB = 30, kPwr_2GB, kPwr_4GB, kPwr_8GB, kPwr_16GB, kPwr_32GB, kPwr_64GB, kPwr_128GB, kPwr_256GB, kPwr_512GB,
    kPwr_1TB = 40, kPwr_2TB, kPwr_4TB, kPwr_8TB, kPwr_16TB, kPwr_32TB, kPwr_64TB, kPwr_128TB, kPwr_256TB, kPwr_512TB,
    kPwr_1PB = 50, kPwr_2PB, kPwr_4PB, kPwr_8PB, kPwr_16PB, kPwr_32PB, kPwr_64PB, kPwr_128PB, kPwr_256PB, kPwr_512PB,
};

inline const char* SizeLabelFromPower(int32_t power)
{
    if (power < 0 || power >= 60)
        return "";

    static const char* labels[] =
    {
         "1b",  "2b",  "4b",  "8b",  "16b",  "32b",  "64b",  "128b",  "256b",  "512b",
        "1KB", "2KB", "4KB", "8KB", "16KB", "32KB", "64KB", "128KB", "256KB", "512KB",
        "1MB", "2MB", "4MB", "8MB", "16MB", "32MB", "64MB", "128MB", "256MB", "512MB",
        "1GB", "2GB", "4GB", "8GB", "16GB", "32GB", "64GB", "128GB", "256GB", "512GB",
        "1TB", "2TB", "4TB", "8TB", "16TB", "32TB", "64TB", "128TB", "256TB", "512TB",
        "1PB", "2PB", "4PB", "8PB", "16PB", "32PB", "64PB", "128PB", "256PB", "512PB",
    };

    return labels[power];
}
