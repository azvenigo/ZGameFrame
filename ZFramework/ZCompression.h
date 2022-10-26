// The MIT License (MIT)

// Copyright (c) 2020 BHUMIJ GUPTA

//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.

/* code */

// [1] See Wiki for more info
#pragma once


#include <iostream>
#include <iomanip>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <string>
#include <bitset>
#include <chrono>
#include "ZMemBuffer.h"
#include "ZBitBuffer.h"



class HuffNode;
typedef std::shared_ptr<HuffNode> HuffNodePtr;

class HuffNode
{
public:
    uint8_t c;
    uint32_t freq;

    HuffNodePtr left;
    HuffNodePtr right;

    HuffNode()
    {
        c = 0;
        freq = 0;
        left = nullptr;
        right = nullptr;
    };

    HuffNode(uint8_t const& ch, int freq) : c(ch), freq(freq)
    {
        left = nullptr;
        right = nullptr;
    };

    ~HuffNode()
    {
    }
};


class pairComparator
{
public:
    int operator()(HuffNodePtr a, HuffNodePtr b)
    {
        return a->freq > b->freq;
    }
};


class Zhuffman
{
public:
    static bool huffmanCompress(ZMemBufferPtr inputBuffer, ZMemBufferPtr outputBuffer);
    static bool huffmanUncompress(ZMemBufferPtr inputBuffer, ZMemBufferPtr outputBuffer);

protected:
    static void traverse(HuffNodePtr, std::unordered_map<uint8_t, ZBitBufferPtr>&, ZBitBufferPtr bits);
    static void traverse(HuffNodePtr, std::unordered_map<ZBitBufferPtr, uint8_t>&, ZBitBufferPtr bits);
    static HuffNodePtr readTree(ZBitBuffer* pBitBuffer);
    static void writeTree(ZBitBuffer* pBitBuffer, HuffNodePtr);

};



class ZLZ77
{
public:
    static bool lz77Encode(ZMemBufferPtr inputBuffer, ZMemBufferPtr outputBuffer);
    static bool lz77Decode(ZMemBufferPtr inputBuffer, ZMemBufferPtr outputBuffer);

    static bool FindLongestMatch(uint8_t* pSearchStart, uint8_t* pSearchEnd, uint8_t* pSearchFor, uint32_t nMax, uint8_t** ppFound, uint32_t& nFound);
    static uint32_t CountRepeatingChars(uint8_t c, uint8_t* pSearchStart, uint8_t* pSearchEnd, uint32_t nMax);
};