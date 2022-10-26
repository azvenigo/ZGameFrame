#include "ZMemBuffer.h"

ZMemBuffer::ZMemBuffer()
{ 
    mAllocated = 0; 
    mCurrentWriteOffset = 0;
    mCurReadOffset = 0;
    mpBuffer = nullptr; 
}

ZMemBuffer::ZMemBuffer(const ZMemBuffer& rhs)
{
    write(rhs.mpBuffer, rhs.mCurrentWriteOffset);
}


ZMemBuffer::ZMemBuffer(uint32_t nInitialSize)
{ 
    mAllocated = (nInitialSize + kBufferGrowSize-1) &~(kBufferGrowSize-1);
    mCurrentWriteOffset = 0;
    mCurReadOffset = 0;
    mpBuffer = (uint8_t*) malloc(mAllocated);
}

ZMemBuffer::~ZMemBuffer()
{
    reset();
}


void ZMemBuffer::reset()
{
    if (mpBuffer)
        free(mpBuffer);
    mpBuffer = nullptr;
    mAllocated = 0;
    mCurReadOffset = 0;
    mCurrentWriteOffset = 0;
}




bool ZMemBuffer::write(uint8_t* pBytes, uint32_t nBytes)
{
    // Grow if needed
    if (mCurrentWriteOffset + nBytes > mAllocated)
    {
        uint32_t nGrowSize = ((mCurrentWriteOffset + nBytes - mAllocated) + kBufferGrowSize - 1) & ~(kBufferGrowSize - 1);

        mAllocated += nGrowSize;
        mpBuffer = (uint8_t*) realloc(mpBuffer, mAllocated);
    }

    memcpy((void*) (mpBuffer + mCurrentWriteOffset), pBytes, nBytes);
    mCurrentWriteOffset += nBytes;

    return true;
}

void ZMemBuffer::seekp(uint32_t nOffset)
{
    mCurrentWriteOffset = nOffset;
}


uint8_t* ZMemBuffer::read(uint32_t nBytes)      // return pointer to current read offset and advance read pointer
{
    assert(mCurReadOffset + nBytes < mAllocated);

    uint8_t* pRead = mpBuffer + mCurReadOffset;
    mCurReadOffset += nBytes;
    return pRead;
}

bool ZMemBuffer::read(uint8_t* pDest, uint32_t nOffset, uint32_t nBytes)
{
    assert(nOffset + nBytes < mAllocated);
    memcpy((void*) pDest, mpBuffer + nOffset, nBytes);
    mCurReadOffset += nBytes;

    return true;
}

void ZMemBuffer::seekg(uint32_t nOffset)
{
    mCurReadOffset = nOffset;
}




void ZMemBuffer::reserve(uint32_t nBytes)
{
    if (mAllocated < nBytes)
    {
        mpBuffer = (uint8_t*)realloc(mpBuffer, nBytes);
        mAllocated = nBytes;
    }
}

void ZMemBuffer::trim()
{
    if (mAllocated > mCurrentWriteOffset)
    {
        mpBuffer = (uint8_t*)realloc(mpBuffer, mCurrentWriteOffset);
        mAllocated = mCurrentWriteOffset;
    }
}



//#define UNITTEST_ZMEM
#ifdef UNITTEST_ZMEM

#include <string>

class ZMemUnitTest
{
public:
    ZMemUnitTest()  // run tests in the constructor
    {

        std::string sTestStringToEncode("This is a string.");
        uint32_t nLength = (uint32_t) sTestStringToEncode.length();

        ZMemBuffer testBuffer((uint8_t*) sTestStringToEncode.data(), nLength);

        ZMemBuffer testBuffer2(testBuffer);

        assert(testBuffer2.size() == nLength);

        // verify testBuffer2 data copied
        assert( memcmp(testBuffer2.data(), sTestStringToEncode.data(), sTestStringToEncode.length()) == 0 );

        testBuffer2[12] = 5;

        assert(*(testBuffer2.data() + 12) == 5);

        testBuffer2.trim();


        testBuffer2[nLength-1] = -1; // should not assert
        //testBuffer2[nLength] = 1; // should assert

        testBuffer.resetwrite();



    }
}

static ZMemUnitTest;



#endif



