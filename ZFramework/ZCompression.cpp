#include "ZCompression.h"
#include "ZDebug.h"
#include "ZTimer.h"
#include "ZAssert.h"


const uint32_t kZHUFTag = 0x5A485546;  // hex for "ZHUF"
const uint32_t kTREEENDTAG = 0x11223344;


void Zhuffman::traverse(HuffNodePtr head, std::unordered_map<uint8_t, ZBitBufferPtr>& charKeyMap, ZBitBufferPtr bits)
{
    if (head->left == NULL && head->right == NULL)
    {
        charKeyMap[head->c] = bits;
        return;
    }

    if (head->left)
    {
        ZBitBufferPtr bitsLeft(new ZBitBuffer(*bits));
        bitsLeft->write(false);
        traverse(head->left, charKeyMap, bitsLeft);
    }

    if (head->right)
    {
        ZBitBufferPtr bitsRight(new ZBitBuffer(*bits));
        bitsRight->write(true);
        traverse(head->right, charKeyMap, bitsRight);
    }
}

void Zhuffman::traverse(HuffNodePtr head, std::unordered_map<ZBitBufferPtr, uint8_t>& keyCharMap, ZBitBufferPtr bits)
{
    if (head->left == NULL && head->right == NULL)
    {
        keyCharMap[bits] = head->c;
        return;
    }

    if (head->left)
    {
        ZBitBufferPtr bitsLeft(new ZBitBuffer(*bits));
        bitsLeft->write(false);
        traverse(head->left, keyCharMap, bitsLeft);
    }

    if (head->right)
    {
        ZBitBufferPtr bitsRight(new ZBitBuffer(*bits));
        bitsRight->write(true);
        traverse(head->right, keyCharMap, bitsRight);
    }
}

HuffNodePtr Zhuffman::readTree(ZBitBuffer* pBitBuffer)
{
    ZRELEASE_ASSERT(pBitBuffer);
    if (!pBitBuffer)
        return nullptr;

    bool bNodeType;
    if (!pBitBuffer->read(bNodeType))
        return nullptr;

    if (bNodeType)
    {
        uint8_t c;
        pBitBuffer->read(c);
        HuffNodePtr head(new HuffNode(c, 1));
        return head;
    }

    HuffNodePtr head(new HuffNode('~', 0));
    head->left = readTree(pBitBuffer);
    head->right = readTree(pBitBuffer);
    return head;
}

void Zhuffman::writeTree(ZBitBuffer* pBitBuffer, HuffNodePtr head)
{
    ZRELEASE_ASSERT(pBitBuffer);
    if (!pBitBuffer)
        return;

    if (head->left == NULL && head->right == NULL)
    {
        bool b = true;
        pBitBuffer->write(b);
        uint8_t c = head->c;
        pBitBuffer->write(c);

        return;
    }

    bool b = false;
    pBitBuffer->write(b);

    if (head->left)
        writeTree(pBitBuffer, head->left);
    if (head->right)
        writeTree(pBitBuffer, head->right);
}


bool Zhuffman::huffmanCompress(ZMemBufferPtr inputBuffer, ZMemBufferPtr outputBuffer)
{
    uint32_t nUncompressedBytes = inputBuffer->size();


    std::vector< HuffNodePtr > charCounts;
    charCounts.resize(256);

    for (uint32_t c = 0; c < 256; c++)
    {
        charCounts[(uint8_t) c].reset(new HuffNode((uint8_t)c, 0));
    }

    for (uint8_t* p = inputBuffer->data(); p < inputBuffer->end(); p++)
        charCounts[*p]->freq++;

//    std::sort(charCounts.begin(), charCounts.end(), [](const HuffNode* a, const HuffNode* b) { return a->freq > b->freq; });


    // Create min priority queue for HuffNode pair
    std::priority_queue<HuffNodePtr, std::vector<HuffNodePtr>, pairComparator> priorityQueue;
    for (auto i : charCounts)
    {
        if (i->freq > 0)       // once we hit nodes of 0 use, we can stop
          priorityQueue.push(i);
    }
    HuffNodePtr head;

    while (!priorityQueue.empty())
    {
        uint32_t freqSum = 0;
        HuffNodePtr first;
        if (!priorityQueue.empty())
        {
            first = priorityQueue.top();
            priorityQueue.pop();
            freqSum += first->freq;
        }

        HuffNodePtr second;
        if (!priorityQueue.empty())
        {
            second = priorityQueue.top();
            priorityQueue.pop();
            freqSum += second->freq;
        }

        HuffNodePtr newPair(new HuffNode('~', freqSum));
        newPair->left = first;
        newPair->right = second;
        priorityQueue.push(newPair);
        if (priorityQueue.size() == 1)
        {
            head = newPair;
            break;
        }
    }

    std::unordered_map<uint8_t, ZBitBufferPtr> charKeyMap;
    assert(head);
    ZBitBufferPtr traverseBitBuffer(new ZBitBuffer());
    traverse(head, charKeyMap, traverseBitBuffer);      // start with empty



    ZBitBuffer bitBuffer;
    bitBuffer.write((uint32_t) kZHUFTag);


    
    // compute the bitstream from the tree
    writeTree(&bitBuffer, head);


    bitBuffer.write((uint32_t) kTREEENDTAG);

    // Write numChars to check file integrity
    bitBuffer.write(nUncompressedBytes);

    //while (reader >> std::noskipws >> ch)
    for (uint8_t* pReader = inputBuffer->data(); pReader < inputBuffer->data()+nUncompressedBytes; pReader++)
    {
        ZBitBufferPtr charBits = charKeyMap[*pReader];
        charBits->seekg();
        for (unsigned int i = 0; i < charBits->size(); i++)
        {
            bool bSet;
            charBits->read(bSet);
            bitBuffer.write(bSet);
        }
    }
    
    bitBuffer.toBuffer(outputBuffer);

    return true;
}

bool Zhuffman::huffmanUncompress(ZMemBufferPtr inputBuffer, ZMemBufferPtr outputBuffer)
{
    // create huffman tree from file
    ZBitBuffer bitBuffer;

    bitBuffer.fromBuffer(inputBuffer);

    uint32_t nTag;
    bitBuffer.read(nTag);
    ZRELEASE_ASSERT( nTag == kZHUFTag ); // verify header

    HuffNodePtr head = readTree(&bitBuffer);
    // Create key char map for decompression
    std::unordered_map<ZBitBufferPtr, uint8_t> keyCharMap;

    ZBitBufferPtr traverseBitBuffer(new ZBitBuffer());
    traverse(head, keyCharMap, traverseBitBuffer);


    bitBuffer.read(nTag);  // kTREEENDTAG
    ZRELEASE_ASSERT(nTag == kTREEENDTAG); 


    // Read total number of characters

    uint32_t nUncompressedBytes;
    bitBuffer.read(nUncompressedBytes);

    ZOUT_LOCKLESS("**** uncompressedbytes:%d\n", nUncompressedBytes);

    while (outputBuffer->size() < nUncompressedBytes)
    {
        HuffNodePtr pNode = head;
        while (pNode->left)     // if either child node is non-null then this is an internal node
        {
            assert(pNode->right);           // if one is null they both must be null
            bool bRight;
            ZRELEASE_ASSERT(bitBuffer.read(bRight));
            if (bRight)
            {
                pNode = pNode->right;
            }
            else
            {
                pNode = pNode->left;
            }
        }

        outputBuffer->write((uint8_t*) &pNode->c, 1);
        pNode = head;
    }

    assert(outputBuffer->size() == nUncompressedBytes);

    return true;
};


//#define UNIT_TEST_ZHUFFMAN
#ifdef UNIT_TEST_ZHUFFMAN

class UnitTestZHuffMan
{
public:
    UnitTestZHuffMan()
    {
        ZOUT_LOCKLESS("ZHuffman UnitTest Running\n");

        Zhuffman h;
        
        const int kSize = 1*1024;

        ZMemBufferPtr original(new ZMemBuffer(kSize));
        ZMemBufferPtr compressed(new ZMemBuffer());
        ZMemBufferPtr uncompressed(new ZMemBuffer());


        // fixed string test
        //string sTestBuf("abc1234defghijklmn123opqrstuvwxyz12343");
        //string sTestBuf("ababcbababaa");
        string sTestBuf("aaaaaaaaaaeeeeeeeeeeeeeeeiiiiiiiiiiiiooouuuussssssssssssst");
        original->write((uint8_t*)sTestBuf.data(), sTestBuf.length());
        original->seekp(sTestBuf.length()); // wrote this much data
        ZRELEASE_ASSERT(h.huffmanCompress(original, compressed));

        ZRELEASE_ASSERT(h.huffmanUncompress(compressed, uncompressed));


        ZRELEASE_ASSERT(uncompressed->size() == original->size());
        ZRELEASE_ASSERT(memcmp(original->data(), uncompressed->data(), original->size()) == 0);






        for (int i = 0; i < kSize; i++)
            *(original->data()+i) = i%256;

        original->seekp(kSize); // wrote this much data




        h.huffmanCompress(original, compressed);

        ZOUT_LOCKLESS("original size:%ld  compressed size:%ld\n", original->size(), compressed->size());

        uncompressed->reset();
        h.huffmanUncompress(compressed, uncompressed);

        ZOUT_LOCKLESS("compressed size:%ld  uncompressed size:%ld\n", compressed->size(), uncompressed->size());

        bool bSizesMatch = uncompressed->size() == original->size();
        ZASSERT(bSizesMatch);

        ZOUT_LOCKLESS("Size Match: ");
        if (bSizesMatch)
            ZOUT_LOCKLESS("Success\n");
        else
            ZOUT_LOCKLESS("FAIL!\n");

        bool bDataMatches = memcmp(original->data(), uncompressed->data(), original->size()) == 0;
        ZASSERT(bDataMatches);
        ZOUT_LOCKLESS("Memcmp:" );
        if (bDataMatches)
            ZOUT_LOCKLESS("Success\n");
        else
            ZOUT_LOCKLESS("FAIL!\n");




        ZOUT_LOCKLESS("ZHuffman UnitTest Complete\n");

    }
};

UnitTestZHuffMan gHuffmanUnitTestInstance;


#endif


inline uint32_t Hash(uint8_t* pStart, uint32_t nLength)
{
    uint32_t h = 0;
    while (nLength-- > 0)
    {
        h += (*pStart);
        pStart++;
    }

    return h;
}


bool ZLZ77::FindLongestMatch(uint8_t* pSearchStart, uint8_t* pSearchEnd, uint8_t* pSearchFor, uint32_t nMax, uint8_t** ppFound, uint32_t& nFound)
{
/*    uint32_t nLongestFound = 0;
    uint8_t* pLongestFoundStart = nullptr;

    uint32_t nSearchAreaLength = pSearchEnd-pSearchStart;


    // search from longest possible down to shortest (3, at the moment)
    for (uint32_t nSearchLength = nMax; nSearchLength >= 3; nSearchLength--)
    {
        uint32_t nMatchStringHash = Hash(pSearchFor, nSearchLength);
//        ZOUT_LOCKLESS("Search length:%d - hash:%d\n", nSearchLength, nMatchStringHash);

        uint32_t nHash = Hash(pSearchStart, nSearchLength);
        for (uint8_t* pSearchIteration = pSearchStart; pSearchIteration < pSearchEnd - nSearchLength + 1; pSearchIteration++)
        {
            if (*pSearchIteration == *pSearchFor && nHash == nMatchStringHash)
            {
                // hash matches, check string
  //              ZOUT_LOCKLESS("Hash matches...checking string...");
                if (memcmp(pSearchIteration, pSearchFor, nSearchLength) == 0)
                {
//                    ZOUT_LOCKLESS("matches.\n");
                    *ppFound = pSearchIteration;
                    nFound = nSearchLength;
                    return true;
                }
                else
                {
//                    static int n = 0;
//                    ZOUT_LOCKLESS("hash collision:%d\n", n++);
                }
            }
            else
            {
//                ZOUT_LOCKLESS("no match. Continuing.\n");
            }

            nHash = nHash - (*pSearchIteration) + *(pSearchIteration+nSearchLength);
        }

    }

    */



    uint32_t nLongestFound = 0;
    uint8_t* pLongestFoundStart = nullptr;

    for (uint8_t* pSearchIteration = pSearchStart; pSearchIteration < pSearchEnd && (pSearchIteration-pSearchStart) < nMax; pSearchIteration++)
    {
        uint32_t nIterationMatch = 0;

        for (uint8_t* pSearch = pSearchIteration; pSearch < pSearchEnd && nIterationMatch < nMax; pSearch++)
        {
            if (*pSearch != *(pSearchFor + nIterationMatch))
                break;

            nIterationMatch++;
        }

        if (nIterationMatch > nLongestFound)
        {
            nLongestFound = nIterationMatch;
            pLongestFoundStart = pSearchIteration;
        }
    }

    if (nLongestFound > 1)
    {
        *ppFound = pLongestFoundStart;
        nFound = nLongestFound;
        return true;
    }
    
    return false;
}

uint32_t ZLZ77::CountRepeatingChars(uint8_t c, uint8_t* pSearchStart, uint8_t* pSearchEnd, uint32_t nMax)
{
    uint32_t nCount = 0;
    while (c == *pSearchStart && pSearchStart++ < pSearchEnd && nCount < nMax)
        nCount++;

    return nCount;
}



bool ZLZ77::lz77Encode(ZMemBufferPtr inputBuffer, ZMemBufferPtr outputBuffer)
{
    const uint32_t kSearchBufferSize = 32 * 1024;

    uint32_t statsNumMatches = 0;
    uint32_t statsCharsMatched = 0;

    ZBitBuffer bitBuffer;

    uint8_t lastWrittenChar = 0;
    bool bLastWrittenCharValid = false;


#ifdef _DEBUG
    static bool bVerbose = false;

    if (bVerbose)
    {
        ZOUT_LOCKLESS("pNext: chars:\"");
        for (uint32_t i = 0; i < inputBuffer->size(); i++)
            ZOUT_LOCKLESS("%c", (uint8_t) * (inputBuffer->data() + i));
        ZOUT_LOCKLESS("\"\n");
    }
#endif



    for (uint32_t nIndex = 0; nIndex < inputBuffer->size(); nIndex++)
    {
        uint8_t* pNext = inputBuffer->data() + nIndex;

        // Set up search window
        uint8_t* pSearchStart = pNext - kSearchBufferSize;
        // clip at the beginning
        if (pSearchStart < inputBuffer->data())
            pSearchStart = inputBuffer->data();


        uint8_t* pSearchEnd = pSearchStart + kSearchBufferSize;
        // stop at our current location
        if (pSearchEnd > pNext)
            pSearchEnd = pNext;

        int32_t nSearchSize = (int32_t) (pSearchEnd - pSearchStart);

#ifdef _DEBUG
        if (bVerbose)
            if (nSearchSize > 1)
            {
                //            ZOUT_LOCKLESS("Searching at index:%d searchStart:%d searchEnd:%d searchSize:%d\n", nIndex, pNext - pSearchStart, pNext - pSearchEnd, nSearchSize);
                ZOUT_LOCKLESS("searching through: chars at %d:\"", nIndex);
                for (int32_t i = 0; i < nSearchSize; i++)
                    ZOUT_LOCKLESS("%c", (uint8_t) * (pSearchStart + i));
                ZOUT_LOCKLESS("\"\n");
            }
#endif


//        uint32_t nMaxToMatch = kSearchBufferSize;
        uint32_t nMaxToMatch = 32;
        if (nIndex + nMaxToMatch > inputBuffer->size())
            nMaxToMatch = inputBuffer->size() - nIndex;

        // find longest match for string at current position
        uint32_t nMatchedChars = 0;
        uint8_t* pLongestMatchStart = nullptr;
        bool bReference = false;

        // Test if next 3+ bytes are the same (encode RLE)
        // search from pNext as far as end of input data
        // 3 minimum bytes for improvement? 
        uint32_t nRepeats = 0;
        
        if (bLastWrittenCharValid)
            nRepeats = CountRepeatingChars(lastWrittenChar, pNext, inputBuffer->end(), 64*1024-1); // limit to 64k

        if (nRepeats >= 3 && bLastWrittenCharValid)
        {
#ifdef _DEBUG
            if (bVerbose)
            {
                ZOUT_LOCKLESS("Found %d repeating chars.\n", nRepeats);
            }
#endif

            bitBuffer.write(true); // now reference the previous char N-1 times
            bitBuffer.write((uint16_t) 1); // distance

            assert(nRepeats < 64*1024);

            bitBuffer.write((uint16_t) nRepeats);
            pNext += nRepeats;
            nIndex += nRepeats;

            if (nIndex < inputBuffer->size())      // need to check in case last bytes were all referenced
            {
                bitBuffer.write(*pNext);
                lastWrittenChar = *pNext;
                bLastWrittenCharValid = true;
            }

        }
        else if (FindLongestMatch(pSearchStart, pSearchEnd, pNext, nMaxToMatch, &pLongestMatchStart, nMatchedChars))
        {
#ifdef _DEBUG
            if (bVerbose)
            {
                ZOUT_LOCKLESS("Longest match:%d,%d chars:\"", (pLongestMatchStart - pSearchStart), nMatchedChars);
                for (uint32_t nMatch = 0; nMatch < nMatchedChars; nMatch++)
                    ZOUT_LOCKLESS("%c", (uint8_t) * (pLongestMatchStart + nMatch));
                ZOUT_LOCKLESS("\"\n");
            }
#endif
            // output triple
            bReference = true;
            bitBuffer.write(bReference);    // one bit signifying this is a reference


            assert(pNext - pLongestMatchStart < 65536);    // fit into 16bit value
            assert(nMatchedChars < 65536);  // fit into a 16bit value

            uint16_t nDistance = (uint16_t) (pNext - pLongestMatchStart);
            uint16_t nEncodedLength = (uint16_t)nMatchedChars;
            bitBuffer.write(nDistance);
            bitBuffer.write(nEncodedLength);

            pNext += nMatchedChars;
            // advanced past the match
            nIndex += nMatchedChars;

            if (nIndex < inputBuffer->size())      // need to check in case last bytes were all referenced
            {
                bitBuffer.write(*pNext);
                lastWrittenChar = *pNext;
                bLastWrittenCharValid = true;
            }


            // stats
            statsNumMatches++;
            statsCharsMatched += nMatchedChars;
        }
        else
        {
            bitBuffer.write(bReference);
            bitBuffer.write(*pNext);

            lastWrittenChar = *pNext;
            bLastWrittenCharValid = true;
        }
    }

    bitBuffer.toBuffer(outputBuffer);

#ifdef _DEBUG
    ZOUT_LOCKLESS("lz77Encode: before:%db after:%db. NumMatches:%d BytesMatched:%d \n", inputBuffer->size(), outputBuffer->size(), statsNumMatches, statsCharsMatched);
#endif


    return true;
}


bool ZLZ77::lz77Decode(ZMemBufferPtr inputBuffer, ZMemBufferPtr outputBuffer)
{

    ZBitBuffer bitBuffer;
    if (!bitBuffer.fromBuffer(inputBuffer))
    {
        ZOUT("ZLZ77::lz77Decode couldn't extract bitBuffer from inputBuffer!\n");
        return false;
    }

    while (bitBuffer.tellg() < bitBuffer.tellp())
    {
        bool bReference = false;

        ZRELEASE_ASSERT(bitBuffer.read(bReference));

        if (!bReference)
        {
            uint8_t c;
            bitBuffer.read(c);
            outputBuffer->write(&c, sizeof(uint8_t));
        }
        else
        {
            uint16_t nDistance;
            uint16_t nMatchedChars;
            bitBuffer.read(nDistance);
            bitBuffer.read(nMatchedChars);

            // copy nMatchedChars from earlier in the buffer
            assert(nDistance <= outputBuffer->size());
            uint32_t nOutputOffset = outputBuffer->size() - nDistance;
            while (nMatchedChars > 0)
            {
                // The following references data already in the output buffer which can be reallocated on writing when necessary
                uint8_t c = *(outputBuffer->data() + nOutputOffset);
                outputBuffer->write(&c, sizeof(uint8_t));
                nOutputOffset++;
                nMatchedChars--;
            }
            uint8_t c;
            if (bitBuffer.read(c))
                outputBuffer->write(&c, sizeof(uint8_t));
        }
    }

#ifdef _DEBUG
    ZOUT_LOCKLESS("lz77Decode: before:%db after:%db.\n", inputBuffer->size(), outputBuffer->size());
#endif


    return true;
}



//#define UNIT_TEST_ZLZ77
#ifdef UNIT_TEST_ZLZ77

class UnitTestZLZ77
{
public:

    bool LoadFile(const string& filename, ZMemBufferPtr memBuffer)
    {
        std::ifstream file(filename.c_str(), ios::in | ios::binary | ios::ate);    // open and set pointer at end
        if (!file.is_open())
        {
            ZOUT("Failed to open file \"%s\"\n", filename.c_str());
            return false;
        }

        uint32_t nFileSize = (uint32_t)file.tellg();
        file.seekg(0, ios::beg);

        memBuffer->reset();
        memBuffer->reserve(nFileSize);

        file.read((char*) memBuffer->data(), nFileSize);
        memBuffer->seekp(nFileSize);

        return true;
    }

    void RunTest(ZMemBufferPtr original, const string& name)
    {
        ZLZ77 lz;
        Zhuffman hf;

        ZMemBufferPtr lz77Compressed(new ZMemBuffer());


        double fStart;
        double fEnd;
        ZSpeedRecord record77Encode;
        ZSpeedRecord recordHuffmanEncode;
        ZSpeedRecord recordBothEncode;

        // Time compression
        fStart = gTimer.GetUSSinceEpoch();
        ZRELEASE_ASSERT(lz.lz77Encode(original, lz77Compressed));
        fEnd = gTimer.GetUSSinceEpoch();

        record77Encode.AddSample(original->size(), fEnd-fStart );


        // Now verify original data
        ZMemBufferPtr lz77Decoded(new ZMemBuffer());
        lz.lz77Decode(lz77Compressed, lz77Decoded);
        ZRELEASE_ASSERT(lz77Decoded->size() == original->size());
        ZRELEASE_ASSERT(memcmp(lz77Decoded->data(), original->data(), original->size()) == 0);





        ZMemBufferPtr huffmanCompressed(new ZMemBuffer());
        fStart = gTimer.GetUSSinceEpoch();
        ZRELEASE_ASSERT(hf.huffmanCompress(original, huffmanCompressed));
        fEnd = gTimer.GetUSSinceEpoch();
        recordHuffmanEncode.AddSample(original->size(), fEnd-fStart);


        ZMemBufferPtr lz77AndHuffmanCompressed(new ZMemBuffer());
        fStart = gTimer.GetUSSinceEpoch();
        ZRELEASE_ASSERT(hf.huffmanCompress(lz77Compressed, lz77AndHuffmanCompressed));
        fEnd = gTimer.GetUSSinceEpoch();
        recordBothEncode.AddSample(original->size(), fEnd-fStart);



        ZMemBufferPtr huffmanDecompressed(new ZMemBuffer());
        ZRELEASE_ASSERT(hf.huffmanUncompress(lz77AndHuffmanCompressed, huffmanDecompressed)); // back to just lz77

        // check buffer sizes and contents match
        ZRELEASE_ASSERT(huffmanDecompressed->size() == lz77Compressed->size());
        ZRELEASE_ASSERT(memcmp(huffmanDecompressed->data(), lz77Compressed->data(), lz77Compressed->size()) == 0);

        ZMemBufferPtr uncompressed(new ZMemBuffer());
        ZRELEASE_ASSERT(lz.lz77Decode(huffmanDecompressed, uncompressed));
        // check buffer sizes and contents match
        ZRELEASE_ASSERT(uncompressed->size() == original->size());
        ZRELEASE_ASSERT(memcmp(uncompressed->data(), original->data(), original->size()) == 0);

        ZOUT_LOCKLESS("Test:%s Results\n", name.c_str());
        ZOUT_LOCKLESS("Original  Size: %lld\n", original->size());
        ZOUT_LOCKLESS("LZ77      Size: %lld  ratio:%1.4f    Time:%.6fs   MiB/s:%f\n", lz77Compressed->size(),            (double) lz77Compressed->size() / (double) original->size(),          (double) record77Encode.fUSSpent,        record77Encode.GetMBPerSecond());
        ZOUT_LOCKLESS("Huffman   Size: %lld  ratio:%1.4f    Time:%.6fs   MiB/s:%f\n", huffmanCompressed->size(),         (double) huffmanCompressed->size() / (double)original->size(),        (double) recordHuffmanEncode.fUSSpent,   recordHuffmanEncode.GetMBPerSecond());
        ZOUT_LOCKLESS("LZ77+Huff Size: %lld  ratio:%1.4f    Time:%.6fs   MiB/s:%f\n", lz77AndHuffmanCompressed->size(),  (double) lz77AndHuffmanCompressed->size() / (double)original->size(), (double) recordBothEncode.fUSSpent,      recordBothEncode.GetMBPerSecond());
    }

    UnitTestZLZ77()
    {
        ZOUT_LOCKLESS("UnitTestZLZ77 Running\n");


        // Test RabinKarp search
/*        string sTestBuf("abc1234defghijklmn123opqrstuvwxyz12343");
        ZLZ77 lz;
        uint8_t* pStart = (uint8_t*)sTestBuf.c_str();
        uint8_t* pEnd = (uint8_t*)sTestBuf.c_str() + 18;
        uint8_t* pSearchFor = pEnd;  // "123"

        uint8_t* pFound = nullptr;
        uint32_t nLength = 0;

        if (lz.FindLongestMatch(pStart, pEnd, pEnd, 4, &pFound, nLength))
        {
            ZOUT_LOCKLESS("found");
        }

  */      






        const int kSize = 128 *  1024;  

        ZMemBufferPtr original(new ZMemBuffer(kSize));

        
        // fixed string test
        //string sTestBuf("abc1234defghijklmn123opqrstuvwxyz12343");
        string sTestBuf("ababcbababaa");
        original->write((uint8_t*) sTestBuf.data(), sTestBuf.length());
        original->seekp(sTestBuf.length()); // wrote this much data
        RunTest(original, "short string");



        // sequential data
        original->reset();
        original->reserve(kSize);
        for (int i = 0; i < kSize; i++)
            *(original->data() + i) = i % 256;
        original->seekp(kSize); // wrote this much data
        RunTest(original, "sequential 8bit");
        
        

        
        // slow test to verify encoding lengths work
/*        for (int kSize = 1; kSize < 128 * 1024; kSize++)
        {
            ZMemBufferPtr original(new ZMemBuffer(kSize));
            for (int i = 0; i < kSize; i++)
                *(original->data() + i) = i % 256;
            original->seekp(kSize); // wrote this much data

            ZMemBufferPtr enc(new ZMemBuffer(kSize));
            ZMemBufferPtr dec(new ZMemBuffer(kSize));

            ZLZ77 lz;
            lz.lz77Encode(original, enc);
            lz.lz77Decode(enc, dec);

            ZRELEASE_ASSERT(original->size() == dec->size());
            ZRELEASE_ASSERT(memcmp(original->data(), dec->data(), dec->size()) == 0);

        }
  */      
        






        
        

        
        // 32bit ints
        for (int i = 0; i < kSize/4; i++)
            *((uint32_t*) original->data() + i) = i;
        original->seekp(kSize); // wrote this much data
        RunTest(original, "sequential 32bit");


        //64bit ints
        for (int i = 0; i < kSize / 8; i++)
            *((uint64_t*)original->data() + i) = i;
        original->seekp(kSize); // wrote this much data
        RunTest(original, "sequential 64bit");

        

        // all 0s
        for (int i = 0; i < kSize; i++)
            *(original->data() + i) = 0;
        original->seekp(kSize); // wrote this much data
        RunTest(original, "All 0s");


/*        for (int i = 0; i < kSize; i++)
            *(original->data() + i) = 'a' + rand()%2;
        original->seekp(kSize); // wrote this much data*/

        /*

        ZMemBufferPtr compressed(new ZMemBuffer());
        lz.lz77Encode(original, compressed);

        ZOUT_LOCKLESS("old        original size:%ld  compressed size:%ld\n", original->size(), compressed->size());

        ZMemBufferPtr uncompressed(new ZMemBuffer());
        lz.lz77Decode(compressed, uncompressed);

        ZOUT_LOCKLESS("compressed size:%ld  uncompressed size:%ld\n", compressed->size(), uncompressed->size());

        bool bSizesMatch = uncompressed->size() == original->size();
        ZASSERT(bSizesMatch);

        ZOUT_LOCKLESS("Size Match: ");
        if (bSizesMatch)
            ZOUT_LOCKLESS("Success\n");
        else
            ZOUT_LOCKLESS("FAIL!\n");

        bool bDataMatches = memcmp(original->data(), uncompressed->data(), original->size()) == 0;
        ZASSERT(bDataMatches);
        ZOUT_LOCKLESS("Memcmp:");
        if (bDataMatches)
            ZOUT_LOCKLESS("Success\n");
        else
            ZOUT_LOCKLESS("FAIL!\n");
            
            





        /*

        ZMemBufferPtr lz77andhuffman(new ZMemBuffer());
        Zhuffman h;
        h.huffmanCompress(compressed, lz77andhuffman);

        ZOUT_LOCKLESS("lz77 and huffman lz77 compressed size:%ld  both size:%ld\n", compressed->size(), lz77andhuffman->size());


        */
        
/*        ZMemBufferPtr testCompressBMP(new ZMemBuffer());
        assert(LoadFile("./res/testcompress.bmp", testCompressBMP));

        ZMemBufferPtr compressed(new ZMemBuffer());
        compressed->reset();
        lz.lz77Encode(testCompressBMP, compressed);

        ZOUT_LOCKLESS("testCompressBMP bitbuffer  original size:%ld  lz77 compressed size:%ld\n", testCompressBMP->size(), compressed->size());

        ZMemBufferPtr lz77andhuffman(new ZMemBuffer());
        Zhuffman h;
        h.huffmanCompress(compressed, lz77andhuffman);
        ZOUT_LOCKLESS("testCompressBMP  lz77 and huffman lz77 compressed size:%ld  both size:%ld\n", compressed->size(), lz77andhuffman->size());
        */
        

        ZMemBufferPtr testCompressBigText(new ZMemBuffer());
        ZRELEASE_ASSERT(LoadFile("./res/testcompress.txt", testCompressBigText));
        RunTest(testCompressBigText, "./res/testcompress.txt");


        ZOUT_LOCKLESS("UnitTestZLZ77 UnitTest Complete\n");


    }
};

UnitTestZLZ77 gZLZ77UnitTestInstance;


#endif

