// MIT License
// Copyright 2019 Alex Zvenigorodsky
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "ZZipAPI.h"
#include <iostream>
#include "zlibAPI.h"


void ZZipAPI::Obfuscate(unsigned char* pData, int64_t nLength)
{
    // obfuscate the data further
    unsigned char cMod = (202 + nLength) % 256;
    for (int64_t i = 0; i < nLength; i++)
    {
        *pData = *pData ^ cMod;
        pData++;
        cMod += 13;
    }
}

bool ZZipAPI::Compress(ZMemBufferPtr uncompressed, ZMemBufferPtr compressed, bool bObfuscate)
{
    ZCompressor compressor;
    compressor.Init(6); // default compression level
    compressor.InitStream(uncompressed->data(), uncompressed->size());
    int32_t nStatus = Z_OK;
    int32_t nOutIndex = 0;
    while (compressor.HasMoreOutput())
    {
        if (nStatus == Z_OK)
        {
            nStatus = compressor.Compress(true);    // true for final block since we're giving it the entire buffer
            uint32_t nCompressedBytes = (uint32_t)compressor.GetCompressedBytes();
            uint32_t nNumWritten = 0;
            compressed->write(compressor.GetCompressedBuffer(), nCompressedBytes);
        }

        if (nStatus != Z_OK)
        {
            break;
        }
    }

    if (!(nStatus == Z_OK || nStatus == Z_STREAM_END))
    {
        cerr << "Compress Error #:" << nStatus << "\n";
        return false;
    }

    if (bObfuscate)
        Obfuscate(compressed->data(), compressed->size());

    return true;
}



bool ZZipAPI::Decompress(ZMemBufferPtr compressed, ZMemBufferPtr uncompressed, bool bObfuscate)
{
    // deobfuscate
    if (bObfuscate)
        Obfuscate(compressed->data(), compressed->size());


    ZDecompressor decompressor;
    decompressor.Init();
    decompressor.InitStream(compressed->data(), compressed->size());

    int32_t nStatus;
    do
    {
        nStatus = decompressor.Decompress();
        if (nStatus < 0)
            break;

        if ((nStatus == Z_OK || nStatus == Z_STREAM_END) && decompressor.HasMoreOutput())
        {
            uint32_t nDecompressedBytes = (uint32_t)decompressor.GetDecompressedBytes();
            uncompressed->write(decompressor.GetDecompressedBuffer(), nDecompressedBytes);
        }

    }
    while (decompressor.HasMoreOutput());

    if (nStatus != Z_STREAM_END)
    {
        cerr << "Decompress Error #:" << nStatus << "\n";
        return false;
    }

    return true;
}
