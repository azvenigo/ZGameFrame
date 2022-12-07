#pragma once

#ifdef _WIN64
#include "windows.h"
#endif

#include <string>
#include "ZAssert.h"
#include <list>
#include <mutex>
#include <iostream>
#include <stdio.h>
#include <stdarg.h>

extern std::list<std::string>   gDebugOutQueue;
extern std::mutex               gDebugOutMutex;


// To be called once per frame by main queue
static void FlushDebugOutQueue()
{
    if (gDebugOutQueue.empty())
        return;

    const std::lock_guard<std::mutex> lock(gDebugOutMutex);
    for (auto s : gDebugOutQueue)
    {
#ifdef _WIN64
        OutputDebugStringA(s.c_str());
#else
        std::cout << s.c_str();
#endif
    }

    gDebugOutQueue.clear();
}


static void OutputDebug(const std::string& sOutput)
{
#ifdef _WIN64
    OutputDebugStringA(sOutput.c_str());
#else
    std::cout << sOutput.c_str();
#endif
}

static void OutputDebug(const char* format, ...)
{
    va_list args;
    static char  buf[2048];

    va_start(args, format);
    vsprintf_s(buf, format, args);

    //OutputDebugStringA(buf);
    const std::lock_guard<std::mutex> lock(gDebugOutMutex);
    gDebugOutQueue.emplace_back(std::string(buf));
}

static void OutputDebugLockless(const char* format, ...)
{
    va_list args;
    static char  buf[2048];

    va_start(args, format);
    vsprintf_s(buf, format, args);

    OutputDebugStringA(buf);
}


#define ZOUT OutputDebug
#define ZOUT_LOCKLESS OutputDebugLockless

#ifdef _DEBUG
    #define ZDEBUG_OUT 	OutputDebug
    #define ZDEBUG_OUT_LOCKLESS OutputDebugLockless
#else
    #define ZDEBUG_OUT
    #define ZDEBUG_OUT_LOCKLESS 
#endif


