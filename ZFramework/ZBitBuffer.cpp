#include "ZBitBuffer.h"
#include <iostream>
#include "ZRandom.h"
#include "ZAssert.h"

const uint16_t kBitBufferHeader = 0xC6A9;

ZBitBuffer::ZBitBuffer()
{ 
    mAllocatedBits = 0; 
    mCurrentBitWriteOffset = 0;
    mCurrentBitReadOffset = 0;
    mpBuffer = nullptr; 
}

ZBitBuffer::ZBitBuffer(const ZBitBuffer& rhs)
{
    reserve(rhs.mCurrentBitWriteOffset);

    uint32_t nBytesRequired = RequiredBytesForBits(rhs.mCurrentBitWriteOffset);

    memcpy(mpBuffer, rhs.mpBuffer, nBytesRequired);
    mCurrentBitWriteOffset = rhs.mCurrentBitWriteOffset;
}


ZBitBuffer::ZBitBuffer(uint32_t nInitialSize)
{ 
    uint32_t nRequiredStorage = RequiredBytesForBits(nInitialSize);

    nRequiredStorage = (nRequiredStorage + kBufferGrowSize-1) &~(kBufferGrowSize-1);
    mCurrentBitWriteOffset = 0;
    mCurrentBitReadOffset = 0;
    mpBuffer = (uint8_t*) malloc(nRequiredStorage);
    memset(mpBuffer, 0, nRequiredStorage);

    mAllocatedBits = nRequiredStorage << 3;
}

ZBitBuffer::~ZBitBuffer()
{
    reset();
}

void ZBitBuffer::reset()
{
    if (mpBuffer)
        free(mpBuffer);
    mpBuffer = nullptr;
    mAllocatedBits = 0;
    mCurrentBitWriteOffset = 0;
    mCurrentBitReadOffset = 0;
}



void  ZBitBuffer::GrowIfNeeded(uint32_t nBitsToBeAdded)
{
    // Grow if needed
    if (mCurrentBitWriteOffset + nBitsToBeAdded > mAllocatedBits)
    {
        uint32_t nGrowSize = ((mCurrentBitWriteOffset + nBitsToBeAdded - mAllocatedBits) + kBufferGrowSize - 1) & ~(kBufferGrowSize - 1);

        uint32_t nNewRequiredStorage = RequiredBytesForBits(nGrowSize + mAllocatedBits);

        uint32_t nOldAllocatedStorage = mAllocatedBits >> 3;
        mAllocatedBits = nNewRequiredStorage << 3;

        mpBuffer = (uint8_t*)realloc(mpBuffer, nNewRequiredStorage);

        memset(mpBuffer+(mCurrentBitWriteOffset>>3), 0, nNewRequiredStorage-nOldAllocatedStorage);  // zero out new allocation
    }
}

void ZBitBuffer::seekp(uint32_t nOffset)
{
    mCurrentBitWriteOffset = nOffset;
}


bool ZBitBuffer::fromBuffer(ZMemBufferPtr memBuffer)
{
    uint16_t nHeader = *((uint16_t*) memBuffer->read(sizeof(uint16_t)));
    assert(nHeader == kBitBufferHeader);
    if (nHeader != kBitBufferHeader)
    {
        std::cerr << "Error. Header not present when trying to create ZBitBuffer from ZMemBuffer.\n";
        return false;
    }

    uint32_t nBits = *((uint32_t*)memBuffer->read(sizeof(uint32_t)));

    reset();
    reserve(nBits);
    memcpy(mpBuffer, memBuffer->data()+memBuffer->tellg(), RequiredBytesForBits(nBits));
    seekp(nBits);

    return true;
}

bool ZBitBuffer::toBuffer(ZMemBufferPtr memBuffer)
{
    memBuffer->reset();

    memBuffer->write((uint8_t*) &kBitBufferHeader, sizeof(uint16_t));         // write the header
    memBuffer->write((uint8_t*) &mCurrentBitWriteOffset, sizeof(uint32_t)); // write the number of bits
    memBuffer->write(mpBuffer, RequiredBytesForBits(mCurrentBitWriteOffset));

    return true;
}


// Reader functions

bool ZBitBuffer::read(bool& bOut)
{
    if (mCurrentBitReadOffset < mCurrentBitWriteOffset)
    {
        bOut = getBit(mCurrentBitReadOffset++);

        return true;
    }

    return false;
}

bool ZBitBuffer::read(uint8_t& nOut)
{
    if (mCurrentBitReadOffset + (sizeof(uint8_t) << 3) <= mAllocatedBits)
    { 
        nOut = 0;
        // swizzle out the next 8 bits
        nOut =  
          (uint8_t)getBit(mCurrentBitReadOffset    )
        | (uint8_t)getBit(mCurrentBitReadOffset + 1) << 1
        | (uint8_t)getBit(mCurrentBitReadOffset + 2) << 2
        | (uint8_t)getBit(mCurrentBitReadOffset + 3) << 3
        | (uint8_t)getBit(mCurrentBitReadOffset + 4) << 4
        | (uint8_t)getBit(mCurrentBitReadOffset + 5) << 5
        | (uint8_t)getBit(mCurrentBitReadOffset + 6) << 6
        | (uint8_t)getBit(mCurrentBitReadOffset + 7) << 7;

        mCurrentBitReadOffset += sizeof(uint8_t) << 3;
        return true;
    }

    return false;
}

bool ZBitBuffer::read(uint16_t& nOut)
{
    if (mCurrentBitReadOffset + (sizeof(uint16_t) << 3) <= mAllocatedBits)
    {
        nOut = 0;
        // swizzle out the next 16 bits
        nOut =
              (uint16_t)getBit(mCurrentBitReadOffset)
            | (uint16_t)getBit(mCurrentBitReadOffset + 1) << 1
            | (uint16_t)getBit(mCurrentBitReadOffset + 2) << 2
            | (uint16_t)getBit(mCurrentBitReadOffset + 3) << 3
            | (uint16_t)getBit(mCurrentBitReadOffset + 4) << 4
            | (uint16_t)getBit(mCurrentBitReadOffset + 5) << 5
            | (uint16_t)getBit(mCurrentBitReadOffset + 6) << 6
            | (uint16_t)getBit(mCurrentBitReadOffset + 7) << 7
            | (uint16_t)getBit(mCurrentBitReadOffset + 8) << 8
            | (uint16_t)getBit(mCurrentBitReadOffset + 9) << 9
            | (uint16_t)getBit(mCurrentBitReadOffset + 10) << 10
            | (uint16_t)getBit(mCurrentBitReadOffset + 11) << 11
            | (uint16_t)getBit(mCurrentBitReadOffset + 12) << 12
            | (uint16_t)getBit(mCurrentBitReadOffset + 13) << 13
            | (uint16_t)getBit(mCurrentBitReadOffset + 14) << 14
            | (uint16_t)getBit(mCurrentBitReadOffset + 15) << 15;

        mCurrentBitReadOffset += sizeof(uint16_t) << 3;
        return true;
    }

    return false;
}

bool ZBitBuffer::read(uint32_t& nOut)
{
    if (mCurrentBitReadOffset + (sizeof(uint32_t) << 3) <= mAllocatedBits)
    {
        nOut = 0;
        // swizzle out the next 16 bits
        nOut =
              (uint32_t)getBit(mCurrentBitReadOffset)
            | (uint32_t)getBit(mCurrentBitReadOffset + 1) << 1
            | (uint32_t)getBit(mCurrentBitReadOffset + 2) << 2
            | (uint32_t)getBit(mCurrentBitReadOffset + 3) << 3
            | (uint32_t)getBit(mCurrentBitReadOffset + 4) << 4
            | (uint32_t)getBit(mCurrentBitReadOffset + 5) << 5
            | (uint32_t)getBit(mCurrentBitReadOffset + 6) << 6
            | (uint32_t)getBit(mCurrentBitReadOffset + 7) << 7
            | (uint32_t)getBit(mCurrentBitReadOffset + 8) << 8
            | (uint32_t)getBit(mCurrentBitReadOffset + 9) << 9
            | (uint32_t)getBit(mCurrentBitReadOffset + 10) << 10
            | (uint32_t)getBit(mCurrentBitReadOffset + 11) << 11
            | (uint32_t)getBit(mCurrentBitReadOffset + 12) << 12
            | (uint32_t)getBit(mCurrentBitReadOffset + 13) << 13
            | (uint32_t)getBit(mCurrentBitReadOffset + 14) << 14
            | (uint32_t)getBit(mCurrentBitReadOffset + 15) << 15
            | (uint32_t)getBit(mCurrentBitReadOffset + 16) << 16 
            | (uint32_t)getBit(mCurrentBitReadOffset + 17) << 17
            | (uint32_t)getBit(mCurrentBitReadOffset + 18) << 18
            | (uint32_t)getBit(mCurrentBitReadOffset + 19) << 19
            | (uint32_t)getBit(mCurrentBitReadOffset + 20) << 20
            | (uint32_t)getBit(mCurrentBitReadOffset + 21) << 21
            | (uint32_t)getBit(mCurrentBitReadOffset + 22) << 22
            | (uint32_t)getBit(mCurrentBitReadOffset + 23) << 23
            | (uint32_t)getBit(mCurrentBitReadOffset + 24) << 24
            | (uint32_t)getBit(mCurrentBitReadOffset + 25) << 25
            | (uint32_t)getBit(mCurrentBitReadOffset + 26) << 26
            | (uint32_t)getBit(mCurrentBitReadOffset + 27) << 27
            | (uint32_t)getBit(mCurrentBitReadOffset + 28) << 28
            | (uint32_t)getBit(mCurrentBitReadOffset + 29) << 29
            | (uint32_t)getBit(mCurrentBitReadOffset + 30) << 30
            | (uint32_t)getBit(mCurrentBitReadOffset + 31) << 31;

        mCurrentBitReadOffset += sizeof(uint32_t) << 3;
        return true;
    }

    return false;
}

bool ZBitBuffer::read(uint64_t& nOut)
{
    if (mCurrentBitReadOffset + (sizeof(uint64_t) << 3) <= mAllocatedBits)
    {
        nOut = 0;
        // swizzle out the next 16 bits
        nOut =
              (uint64_t)getBit(mCurrentBitReadOffset)
            | (uint64_t)getBit(mCurrentBitReadOffset + 1) << 1
            | (uint64_t)getBit(mCurrentBitReadOffset + 2) << 2
            | (uint64_t)getBit(mCurrentBitReadOffset + 3) << 3
            | (uint64_t)getBit(mCurrentBitReadOffset + 4) << 4
            | (uint64_t)getBit(mCurrentBitReadOffset + 5) << 5
            | (uint64_t)getBit(mCurrentBitReadOffset + 6) << 6
            | (uint64_t)getBit(mCurrentBitReadOffset + 7) << 7
            | (uint64_t)getBit(mCurrentBitReadOffset + 8) << 8
            | (uint64_t)getBit(mCurrentBitReadOffset + 9) << 9
            | (uint64_t)getBit(mCurrentBitReadOffset + 10) << 10
            | (uint64_t)getBit(mCurrentBitReadOffset + 11) << 11
            | (uint64_t)getBit(mCurrentBitReadOffset + 12) << 12
            | (uint64_t)getBit(mCurrentBitReadOffset + 13) << 13
            | (uint64_t)getBit(mCurrentBitReadOffset + 14) << 14
            | (uint64_t)getBit(mCurrentBitReadOffset + 15) << 15
            | (uint64_t)getBit(mCurrentBitReadOffset + 16) << 16
            | (uint64_t)getBit(mCurrentBitReadOffset + 17) << 17
            | (uint64_t)getBit(mCurrentBitReadOffset + 18) << 18
            | (uint64_t)getBit(mCurrentBitReadOffset + 19) << 19
            | (uint64_t)getBit(mCurrentBitReadOffset + 20) << 20
            | (uint64_t)getBit(mCurrentBitReadOffset + 21) << 21
            | (uint64_t)getBit(mCurrentBitReadOffset + 22) << 22
            | (uint64_t)getBit(mCurrentBitReadOffset + 23) << 23
            | (uint64_t)getBit(mCurrentBitReadOffset + 24) << 24
            | (uint64_t)getBit(mCurrentBitReadOffset + 25) << 25
            | (uint64_t)getBit(mCurrentBitReadOffset + 26) << 26
            | (uint64_t)getBit(mCurrentBitReadOffset + 27) << 27
            | (uint64_t)getBit(mCurrentBitReadOffset + 28) << 28
            | (uint64_t)getBit(mCurrentBitReadOffset + 29) << 29
            | (uint64_t)getBit(mCurrentBitReadOffset + 30) << 30
            | (uint64_t)getBit(mCurrentBitReadOffset + 31) << 31
            | (uint64_t)getBit(mCurrentBitReadOffset + 32) << 32
            | (uint64_t)getBit(mCurrentBitReadOffset + 33) << 33
            | (uint64_t)getBit(mCurrentBitReadOffset + 34) << 34
            | (uint64_t)getBit(mCurrentBitReadOffset + 35) << 35
            | (uint64_t)getBit(mCurrentBitReadOffset + 36) << 36
            | (uint64_t)getBit(mCurrentBitReadOffset + 37) << 37
            | (uint64_t)getBit(mCurrentBitReadOffset + 38) << 38
            | (uint64_t)getBit(mCurrentBitReadOffset + 39) << 39
            | (uint64_t)getBit(mCurrentBitReadOffset + 40) << 40
            | (uint64_t)getBit(mCurrentBitReadOffset + 41) << 41
            | (uint64_t)getBit(mCurrentBitReadOffset + 42) << 42
            | (uint64_t)getBit(mCurrentBitReadOffset + 43) << 43
            | (uint64_t)getBit(mCurrentBitReadOffset + 44) << 44
            | (uint64_t)getBit(mCurrentBitReadOffset + 45) << 45
            | (uint64_t)getBit(mCurrentBitReadOffset + 46) << 46
            | (uint64_t)getBit(mCurrentBitReadOffset + 47) << 47
            | (uint64_t)getBit(mCurrentBitReadOffset + 48) << 48
            | (uint64_t)getBit(mCurrentBitReadOffset + 49) << 49
            | (uint64_t)getBit(mCurrentBitReadOffset + 50) << 50
            | (uint64_t)getBit(mCurrentBitReadOffset + 51) << 51
            | (uint64_t)getBit(mCurrentBitReadOffset + 52) << 52
            | (uint64_t)getBit(mCurrentBitReadOffset + 53) << 53
            | (uint64_t)getBit(mCurrentBitReadOffset + 54) << 54
            | (uint64_t)getBit(mCurrentBitReadOffset + 55) << 55
            | (uint64_t)getBit(mCurrentBitReadOffset + 56) << 56
            | (uint64_t)getBit(mCurrentBitReadOffset + 57) << 57
            | (uint64_t)getBit(mCurrentBitReadOffset + 58) << 58
            | (uint64_t)getBit(mCurrentBitReadOffset + 59) << 59
            | (uint64_t)getBit(mCurrentBitReadOffset + 60) << 60
            | (uint64_t)getBit(mCurrentBitReadOffset + 61) << 61
            | (uint64_t)getBit(mCurrentBitReadOffset + 62) << 62
            | (uint64_t)getBit(mCurrentBitReadOffset + 63) << 63;

        mCurrentBitReadOffset += sizeof(uint64_t) << 3;
        return true;
    }

    return false;
}

bool ZBitBuffer::write(bool bIn)
{
    GrowIfNeeded(sizeof(bool));

    uint32_t nCharOffset = mCurrentBitWriteOffset >> 3;     
    uint32_t nBitMaskShift = mCurrentBitWriteOffset & 7;    // lowest 3 bits
    uint32_t nBitMask = 1 << nBitMaskShift;

    if (bIn)
        mpBuffer[nCharOffset] |= nBitMask;
    else
        mpBuffer[nCharOffset] &= ~nBitMask;

    mCurrentBitWriteOffset++;

    return true;
}

bool ZBitBuffer::write(uint8_t nIn)
{
    for (int i = 0; i < sizeof(uint8_t) << 3; i++)
    {
        bool bSet = nIn & 1;    // lowest order bit
        if (!write(bSet))
            return false;

        nIn = nIn >> 1;
    }

    return true;
}

bool ZBitBuffer::write(uint16_t nIn)
{
    for (int i = 0; i < sizeof(uint16_t) << 3; i++)
    {
        bool bSet = nIn & 1;    // lowest order bit
        if (!write(bSet))
            return false;

        nIn = nIn >> 1;
    }

    return true;
}

bool ZBitBuffer::write(uint32_t nIn)
{
    for (int i = 0; i < sizeof(uint32_t) << 3; i++)
    {
        bool bSet = nIn & 1;    // lowest order bit
        if (!write(bSet))
            return false;

        nIn = nIn >> 1;
    }

    return true;
}

bool ZBitBuffer::write(uint64_t nIn)
{
    for (int i = 0; i < sizeof(uint64_t) << 3; i++)
    {
        bool bSet = nIn & 1;    // lowest order bit
        if (!write(bSet))
            return false;

        nIn = nIn >> 1;
    }

    return true;
}






void ZBitBuffer::seekg(uint32_t nOffset)
{
    mCurrentBitReadOffset = nOffset;
}




void ZBitBuffer::reserve(uint32_t nBits)
{
    if (mAllocatedBits < nBits)
    {
        uint32_t nRequiredStorage = RequiredBytesForBits(nBits);

        uint32_t nOldAllocatedBytes = mAllocatedBits >> 3;
        mpBuffer = (uint8_t*)realloc(mpBuffer, nRequiredStorage);
        mAllocatedBits = nRequiredStorage << 3;

        uint32_t nGrowSize = nRequiredStorage-nOldAllocatedBytes;

        memset(mpBuffer, nOldAllocatedBytes, nGrowSize);


    }
}

void ZBitBuffer::trim()
{

    if (mAllocatedBits > mCurrentBitWriteOffset)
    {
        uint32_t nAllocatedBytes = RequiredBytesForBits(mCurrentBitWriteOffset);

        mpBuffer = (uint8_t*)realloc(mpBuffer, nAllocatedBytes);
        mAllocatedBits = nAllocatedBytes << 3;
    }
}



//#define UNITTEST_ZBitBuffer
#ifdef UNITTEST_ZBitBuffer

#include <string>

class ZBitBufferUnitTest
{
public:
    ZBitBufferUnitTest()  // run tests in the constructor
    {

        ZBitBuffer test;


        for (int nTestSize = 1; nTestSize < 2 * 1024; nTestSize++)
        {
            test.reset();

            bool bSet = 0;
            for (int i = 0; i < nTestSize; i++)
            {
                test.write(bSet);
                bSet = !bSet;
            }

            ZMemBufferPtr store(new ZMemBuffer(nTestSize));
            test.toBuffer(store);

            ZBitBuffer test2;
            test2.fromBuffer(store);

            // check that each N bit is set
            bSet = 0;
            for (int i = 0; i < nTestSize; i++)
            {
                ZRELEASE_ASSERT(test2.getBit(i) == bSet);
                bSet = !bSet;
            }

            ZRELEASE_ASSERT(test2.tellp() == test.tellp());
        }


        const int kTestSize = 17;
   //     test.reserve(kTestSize);
        test.reset();

        // set N bits
        bool bSet = 0;
        for (int i = 0; i < kTestSize; i++)
        {
            test.write(bSet);
            bSet = !bSet;
        }


        // check that each N bit is set
        bSet = 0;
        for (int i = 0; i < kTestSize; i++)
        {
            ZRELEASE_ASSERT(test.getBit(i) == bSet);
            bSet = !bSet;
        }

        // try reading bit at a time while advancing read index
        bSet = 0;
        for (int i = 0; i < kTestSize; i++)
        {
            bool bNextRead;
            ZRELEASE_ASSERT(test.read(bNextRead));
            ZRELEASE_ASSERT(bNextRead == bSet);
            bSet = !bSet;
        }

        // verify no more can be read
        bool bReadShouldFail;
        ZRELEASE_ASSERT(!test.read(bReadShouldFail));

        uint8_t nTestSet8 = 0x9b;    // 10011011
        test.write(nTestSet8);
        uint8_t nTestGet8;
        test.read(nTestGet8);
        ZRELEASE_ASSERT(nTestGet8 == nTestSet8);


        uint16_t nTestSet16 = 0xB693;   // 1011011010010011
        test.write(nTestSet16);
        uint16_t nTestGet16;
        test.read(nTestGet16);
        ZRELEASE_ASSERT(nTestGet16 == nTestSet16);


        uint32_t nTestSet32 = RANDU64(0, 0xffffffff);   
        test.write(nTestSet32);
        uint32_t nTestGet32;
        test.read(nTestGet32);
        ZRELEASE_ASSERT(nTestGet32 == nTestSet32);

        uint64_t nTestSet64 = RANDU64(0, 0xffffffffffffffff);
        test.write(nTestSet64);
        uint64_t nTestGet64;
        test.read(nTestGet64);
        ZRELEASE_ASSERT(nTestGet64 == nTestSet64);


        const int kRandomBitsBetweenIterations = 100;

        for (int i = 0; i < kRandomBitsBetweenIterations; i++)
        {

            // insert a 32bit value before
            uint32_t nTestSet32a = RANDU64(0, 0xffffffff);
            test.write(nTestSet32a);


            // insert a random number of bits
            int32_t nInsertRandom = RANDU64(0, 200);
            for (int j = 0; j < nInsertRandom; j++)
                test.write(RANDBOOL);

            // insert a 32 bit value after
            uint32_t nTestSet32b = RANDU64(0, 0xffffffff);
            test.write(nTestSet32b);


            // now get the previously set values
            uint32_t nTestGet32a;
            uint32_t nTestGet32b;

            test.read(nTestGet32a);

            // skip the random bits
            test.seekg( test.tellg() + nInsertRandom );

            ZRELEASE_ASSERT(test.read(nTestGet32b));

            ZRELEASE_ASSERT(nTestGet32a == nTestSet32a);
            ZRELEASE_ASSERT(nTestGet32b == nTestSet32b);
        }


        // Test converting to and from membuffer

        ZMemBufferPtr memBuffer(new ZMemBuffer());
        ZRELEASE_ASSERT( test.toBuffer(memBuffer) );

        ZBitBuffer fromTest;
        ZRELEASE_ASSERT( fromTest.fromBuffer(memBuffer) );





        test.reset();
        ZRELEASE_ASSERT(test.tellg() == 0);
        ZRELEASE_ASSERT(test.tellp() == 0);
        
        nTestSet16 = 0xB693;   // 1011 0110 1001 0011
        test.write(nTestGet16);
        test.trim();
        ZRELEASE_ASSERT(test.tellg() == 0);
        ZRELEASE_ASSERT(test.tellp() == 16);

        bool bOut;
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 1 && test.tellg() == 1);
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 1 && test.tellg() == 2);
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 0 && test.tellg() == 3);
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 0 && test.tellg() == 4);

        ZRELEASE_ASSERT(test.read(bOut) && bOut == 1 && test.tellg() == 5);
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 0 && test.tellg() == 6);
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 0 && test.tellg() == 7);
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 1 && test.tellg() == 8);
       
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 0 && test.tellg() == 9);
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 1 && test.tellg() == 10);
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 1 && test.tellg() == 11);
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 0 && test.tellg() == 12);

        ZRELEASE_ASSERT(test.read(bOut) && bOut == 1 && test.tellg() == 13);
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 1 && test.tellg() == 14);
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 0 && test.tellg() == 15);
        ZRELEASE_ASSERT(test.read(bOut) && bOut == 1 && test.tellg() == 16);


//        test.reset();





    }
}

static ZBitBufferUnitTest;



#endif



