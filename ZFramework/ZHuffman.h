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


class cfp
{
public:
    uint8_t ch;
    int freq;

    cfp* left;
    cfp* right;

    cfp()
    {
        ch = 0;
        freq = 0;
        left = nullptr;
        right = nullptr;
    };

    cfp(uint8_t const& ch, int const& freq) : ch(ch), freq(freq)
    {
        left = nullptr;
        right = nullptr;
    };

    ~cfp()
    {
        delete left;
        delete right;
    }
};

class pairComparator
{
public:
    // [6]
    int operator()(cfp* const& a, cfp* const& b)
    {
        return a->freq > b->freq;
    }
};


// ******************************
// 	CLASS DECLARATION
// ******************************

/**
 * @brief This class contains methods for file compression and decompression
 *
 */
class Zhuffman
{
public:
    static bool huffmanCompress(ZMemBufferPtr inputBuffer, ZMemBufferPtr outputBuffer);
    static bool huffmanUncompress(ZMemBufferPtr inputBuffer, ZMemBufferPtr outputBuffer);

protected:
    static void traverse(cfp const*, std::unordered_map<uint8_t, std::string>&, std::string const);
    static void traverse(cfp const*, std::unordered_map<std::string, uint8_t>&, std::string const);
    static cfp* readTree(std::vector<bool>& bitstream, int& nIndex);
    static void writeTree(std::vector<bool>& bitstream, cfp const*);

};

