#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <memory>
#include <assert.h>
#include <vector>
#include "ZMemBuffer.h"

class ZBitBuffer
{
    const uint32_t kBufferGrowSize = 64*8;      // 64B should be one cache line
public:
    ZBitBuffer();
    ZBitBuffer(uint32_t nInitialSize);  // starting size
    ZBitBuffer(const ZBitBuffer& rhs);
    ~ZBitBuffer();


    void reset();   // release memory

    bool write(bool bIn);
    bool write(uint8_t nIn);
    bool write(uint16_t nIn);
    bool write(uint32_t nIn);
    bool write(uint64_t nIn);


    // format when stored to or read from buffer
    // Header:            16 bits = 1100 0110 1010 1001 = 0xC6A9
    // NumBits:           32 bits
    // bitstream:    NumBits bits
    bool fromBuffer(ZMemBufferPtr memBuffer);   
    bool toBuffer(ZMemBufferPtr memBuffer); 


    // Read next N bits into the requested type
    bool read(bool& bOut);
    bool read(uint8_t& nOut);
    bool read(uint16_t& nOut);
    bool read(uint32_t& nOut);
    bool read(uint64_t& nOut);



    void seekg(uint32_t nOffset = 0);
    void seekp(uint32_t nOffset = 0);

    void reserve(uint32_t nBytes);
    void trim();

    uint32_t tellg() { return mCurrentBitReadOffset; }
    uint32_t tellp() { return mCurrentBitWriteOffset; }
    uint32_t size()  { return mCurrentBitWriteOffset; } // same as tellp

    inline bool getBit(uint32_t nOffset)
    {
        assert(nOffset < mAllocatedBits);

        uint32_t nCharOffset = nOffset >> 3;
        uint32_t nBitMaskShift = nOffset & 7; // lowest 3 bits
        uint32_t nBitMask = 1 << nBitMaskShift;

        return mpBuffer[nCharOffset] & nBitMask;
    }

    void operator = (const ZBitBuffer& rhs)
    {
        reset();
        reserve(rhs.mCurrentBitWriteOffset);

        uint32_t nBytesRequired = RequiredBytesForBits(rhs.mCurrentBitWriteOffset);

        memcpy(mpBuffer, rhs.mpBuffer, nBytesRequired);
        mCurrentBitWriteOffset = rhs.mCurrentBitWriteOffset;
    }

    bool operator == (const ZBitBuffer& rhs)
    {
        if (mCurrentBitWriteOffset != rhs.mCurrentBitWriteOffset)
            return false;

        uint32_t nBytesRequired = RequiredBytesForBits(rhs.mCurrentBitWriteOffset);
        return memcmp(mpBuffer, rhs.mpBuffer, nBytesRequired) == 0;
    }


private:

    uint32_t    RequiredBytesForBits(uint32_t nBits) { return (nBits + 7) >> 3; }

    void        GrowIfNeeded(uint32_t nBitsToBeAdded);      // ensures there is enough room in the buffer to add these bits



    uint32_t mAllocatedBits;
    uint32_t mCurrentBitWriteOffset;
    uint32_t mCurrentBitReadOffset;
    uint8_t* mpBuffer;
};

typedef std::shared_ptr<ZBitBuffer> ZBitBufferPtr;