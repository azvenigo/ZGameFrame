#include "ZHuffman.h"



// ******************************
// 	METHODS DEFINATION
// ******************************

void Zhuffman::traverse(cfp const* head, std::unordered_map<uint8_t, std::string>& charKeyMap, std::string const s)
{
    if (head->left == NULL && head->right == NULL)
    {
        charKeyMap[head->ch] = s;
        return;
    }
    traverse(head->left, charKeyMap, s + "0");
    traverse(head->right, charKeyMap, s + "1");
}

void Zhuffman::traverse(cfp const* head, std::unordered_map<std::string, uint8_t>& keyCharMap, std::string const s)
{
    if (head->left == NULL && head->right == NULL)
    {
        keyCharMap[s] = head->ch;
        return;
    }
    traverse(head->left, keyCharMap, s + "0");
    traverse(head->right, keyCharMap, s + "1");
}

cfp* Zhuffman::readTree(std::vector<bool>& bitstream, int& nIndex)
{
    assert(nIndex < bitstream.size());
    bool bNodeType = bitstream[nIndex++];

    if (bNodeType == true)
    {
        uint8_t c = 0;
        for (int i = 0; i < 8; i++)
        {
            c = c << 1;
            c |= bitstream[nIndex++];
        }

        cfp* head = new cfp(c, 1);
        return head;
    }

    cfp* head = new cfp('~', 0);
    head->left = readTree(bitstream, nIndex);
    head->right = readTree(bitstream, nIndex);
    return head;
}

void Zhuffman::writeTree(std::vector<bool>& bitstream,cfp const* head)
{
    if (head->left == NULL && head->right == NULL)
    {
        bitstream.push_back(1);
        uint8_t c = head->ch;
        for (int i = 0; i < 8; i++)
        {
            bitstream.push_back(c & 0x80);  // push high order bit
            c = c << 1;
        }

        return;
    }

    bitstream.push_back(0);

    writeTree(bitstream, head->left);
    writeTree(bitstream, head->right);
}


bool Zhuffman::huffmanCompress(ZMemBufferPtr inputBuffer, ZMemBufferPtr outputBuffer)
{
    uint32_t nUncompressedBytes = inputBuffer->size();

    outputBuffer->seekp();

    outputBuffer->write((uint8_t*) &"ZHUF", 4);



    std::vector<bool> encodedBitstream;



    std::vector< cfp* > charCounts;
    charCounts.resize(256);

    for (uint32_t i = 0; i < 256; i++)
    {
        charCounts[i] = new cfp(i, 0);
    }

    for (uint32_t i = 0; i < nUncompressedBytes; i++)
    {
        uint8_t c = inputBuffer->data()[i];
        charCounts[c]->freq++;
    }

    std::sort(charCounts.begin(), charCounts.end(), [](const cfp* a, const cfp* b) { return a->freq > b->freq; });


    // Create min priority queue for cfp pair
    std::priority_queue<cfp*, std::vector<cfp*>, pairComparator> freq_sorted;
    for (auto i : charCounts)
    {
        if (i->freq == 0)       // once we hit nodes of 0 use, we can stop
            break;
        freq_sorted.push(i);
    }
    cfp* head = nullptr;
    while (!freq_sorted.empty())
    {
        cfp* first = freq_sorted.top();
        freq_sorted.pop();
        cfp* second = freq_sorted.top();
        freq_sorted.pop();
        cfp* newPair = new cfp('~', first->freq + second->freq);
        newPair->left = first;
        newPair->right = second;
        freq_sorted.push(newPair);
        if (freq_sorted.size() == 1)
        {
            head = newPair;
            break;
        }
    }

    std::unordered_map<uint8_t, std::string> charKeyMap;
    assert(head);
    traverse(head, charKeyMap, "");

    // compute the bitstream from the tree
    writeTree(encodedBitstream, head);
    delete head;

    uint32_t nNumTreeBits = (uint32_t) encodedBitstream.size();
    outputBuffer->write((uint8_t*) &nNumTreeBits, sizeof(uint32_t));

    outputBuffer->append(encodedBitstream);

    // write out the bitstream
/*    uint8_t c = 0;
    uint8_t bitcount = 0;
    for(auto b : encodedBitstream)
    {
        c |= b;
        c = c << 1;
        bitcount++;

        if (bitcount == 8)
        {
            outputBuffer->write(&c, 1);
            c = 0;
            bitcount = 0;
        }        
    }

    if (bitcount > 0)
    {
        // write out partial c
        c = c << (8 - bitcount);
        outputBuffer->write(&c, 1);
    }*/

    outputBuffer->write((uint8_t*) &"end_tree", 8);

    // Write numChars to check file integrity
    outputBuffer->write((uint8_t*) &nUncompressedBytes, sizeof(nUncompressedBytes));


    // Now encode the text data
    encodedBitstream.clear();

    //while (reader >> std::noskipws >> ch)
    for (uint8_t* pReader = inputBuffer->data(); pReader < inputBuffer->data()+nUncompressedBytes; pReader++)
    {
        std::string bin = charKeyMap[*pReader];
        for (unsigned int i = 0; i < bin.length(); i++)
        {
            bool bSet = bin[i] == '1';
            encodedBitstream.push_back(bSet);
        }
    }

    outputBuffer->append(encodedBitstream);

    return true;
}

bool Zhuffman::huffmanUncompress(ZMemBufferPtr inputBuffer, ZMemBufferPtr outputBuffer)
{
    // create huffman tree from file
    assert( memcmp("ZHUFF", inputBuffer->data(), 4) == 0 ); // verify header

    uint32_t nTreeBits = *((uint32_t*) (inputBuffer->data()+4));  // next 4 bytes are the number of tree bits

    std::vector<bool> bitstream(inputBuffer->ToBitstream(8));   // 4 byte header + 4 byte tree bit count above

    for (uint8_t* pC = inputBuffer->data(); pC < inputBuffer->data() + inputBuffer->size(); pC++)
    {
        uint8_t c = *pC;
        for (int i = 0; i < 8; i++)
        {
            bool b = c & 0x80;  // high order bit
            c = c << 1;
            bitstream.push_back(b);
        }
    }

    bitstream.resize(nTreeBits);

    assert(bitstream.size() == nTreeBits);  // this will assert if bitstream isn't aligned to a byte....assert just to check for now, later trim

    int nIndex = 0;
    cfp* head = readTree(bitstream, nIndex);
    // Create key char map for decompression
    std::unordered_map<std::string, uint8_t> keyCharMap;
    traverse(head, keyCharMap, "");
//    delete head;


    uint32_t nOffsetPastTree = 8 + ((nTreeBits+7)&~7)/8;   // past the 8 initial bytes and rounded up to the bytes past the tree
    inputBuffer->seekg(nOffsetPastTree);

    // Read total number of characters






    uint8_t* pTag = inputBuffer->read(8);

    assert(memcmp(pTag, &"end_tree", 8) == 0);

    uint32_t nUncompressedBytes = *((uint32_t*) inputBuffer->read(sizeof(uint32_t)));

    std::vector<bool> encoded(inputBuffer->ToBitstream(inputBuffer->tellg()));
    uint32_t nReadIndex = 0;


    while (outputBuffer->size() < nUncompressedBytes)
    {
        assert(nReadIndex < encoded.size());
        cfp* pNode = head;
        while (pNode->left)     // if either child node is non-null then this is an internal node
        {
            assert(pNode->right);           // if one is null they both must be null
            if (encoded[nReadIndex])
            {
                pNode = pNode->right;
            }
            else
            {
                pNode = pNode->left;
            }
            nReadIndex++;
        }

        outputBuffer->write((uint8_t*) &pNode->ch, 1);
        pNode = head;
    }

    assert(outputBuffer->size() == nUncompressedBytes);

    delete head;
    return true;
};

