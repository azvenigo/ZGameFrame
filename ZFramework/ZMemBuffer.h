#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <memory>
#include <assert.h>
#include <vector>

class ZMemBuffer
{
    const uint32_t kBufferGrowSize = 1024;
public:
    ZMemBuffer();
    ZMemBuffer(const ZMemBuffer& rhs);  // copy construct
    ZMemBuffer(uint32_t nInitialSize);  // starting size
    ZMemBuffer(uint8_t* pBytes, uint32_t nBytes) { write(pBytes, nBytes); } // copy from existing memory

    ~ZMemBuffer();

    void reset();

    bool write(uint8_t* pBytes, uint32_t nBytes);
    void append(const ZMemBuffer& rhs)
    {
        write(rhs.mpBuffer, rhs.mCurrentWriteOffset);
    }

    uint8_t* read(uint32_t nBytes);
    bool read(uint8_t* pDest, uint32_t nOffset, uint32_t nBytes);

    void seekg(uint32_t nOffset = 0);
    void seekp(uint32_t nOffset = 0);

    void reserve(uint32_t nBytes);
    void trim();

    // direct access
    uint8_t* data() { return mpBuffer; }
    uint8_t* end()  { return mpBuffer + mCurrentWriteOffset; }

    uint32_t tellg() { return mCurReadOffset; }
    uint32_t tellp() { return mCurrentWriteOffset; }
    uint32_t size() { return mCurrentWriteOffset; } // same as tellp... regardless of how much memory is allocated for this buffer

    uint8_t& operator[](uint32_t nOffset)
    {
        assert(nOffset < mAllocated);
        return mpBuffer[nOffset];
    }

    ZMemBuffer& operator += (const ZMemBuffer& rhs)
    {
        append(rhs);
        return *this;
    }

private:
    uint32_t mAllocated;
    uint32_t mCurrentWriteOffset;
    uint32_t mCurReadOffset;
    uint8_t* mpBuffer;
};

typedef std::shared_ptr<ZMemBuffer> ZMemBufferPtr;