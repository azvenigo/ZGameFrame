#pragma once

#ifdef _WIN64
#include "windows.h"
#endif

#include <string>
#include <sstream>
#include "ZAssert.h"
#include <list>
#include <mutex>
#include <iostream>
#include <stdio.h>
#include <stdarg.h>

struct sDbgMsg
{
    enum eLvl : uint32_t
    {
        kDefault    = 0xffffffff,       // white
        kWarning    = 0xffd48715,       // orange
        kError      = 0xffff0000,       // red
        kDebug      = 0xff0000ff        // green
    };

    sDbgMsg(std::string line, uint32_t level = kDefault) { sLine = line; nLevel = level; }

    uint32_t     nLevel;
    std::string sLine;
};


class ZDebug
{
    friend class ZWinDebugConsole;
public:
    ZDebug(size_t maxHistory = 512) : mMaxHistory(maxHistory), mHistoryCounter(0) {}

    template <typename T, typename...Types>
    inline void ImmediateOut(uint32_t level, T arg, Types...more)
    {
        AddRow(level, arg, more...);
        Flush();
    }

    template <typename T, typename...Types>
    inline void AddRow(uint32_t level, T arg, Types...more)
    {
        std::string sLine;
        ToString(sLine, arg, more...);

        mHistory.push_back(sDbgMsg(sLine, level));
        mHistoryCounter++;
        TrimHistory();

        const std::lock_guard<std::mutex> lock(mOutQueueMutex);
        mOutQueue.push_back(sLine);
    }

    template <typename S, typename...SMore>
    inline void ToString(std::string& sLine, S arg, SMore...moreargs)
    {
        std::stringstream ss;
        ss << arg;
        sLine += ss.str();
        return ToString(sLine, moreargs...);
    }

    inline void ToString(std::string&) {}   // needed for the variadic with no args

    inline void Flush() // to be called regularly (once per frame) by application to do actual output
    {
        if (mOutQueue.empty())
            return;

        const std::lock_guard<std::mutex> lock(mOutQueueMutex);
        for (auto s : mOutQueue)
        {
//#ifdef _WIN64
            //OutputDebugStringA(s.c_str());
//#else
            std::cout << s.c_str();
//#endif
        }

        mHistoryCounter++;
        mOutQueue.clear();
        TrimHistory();
    }

    size_t Counter() { return mHistoryCounter; }

private:
    void    TrimHistory()
    {
        size_t nCurSize = mHistory.size();

        if (nCurSize > mMaxHistory)
        {
            const std::lock_guard<std::mutex> lock(mHistoryMutex);

            auto first = mHistory.begin();
            auto last = first;
            std::advance(last, nCurSize - mMaxHistory);
            mHistory.erase(first, last);
            mHistoryCounter++;
        }
    }

    size_t                  mHistoryCounter;
    size_t                  mMaxHistory;
    std::list<sDbgMsg>      mHistory;
    std::mutex              mHistoryMutex;

    std::list<std::string>  mOutQueue;
    std::mutex              mOutQueueMutex;
};

extern ZDebug gDebug;


#define ZOUT(...) gDebug.AddRow(sDbgMsg::kDefault, __VA_ARGS__)
#define ZOUT_LOCKLESS(...) gDebug.ImmediateOut(sDbgMsg::kDefault, __VA_ARGS__)

#define ZWARNING(...) gDebug.AddRow(sDbgMsg::kWarning, __VA_ARGS__)
#define ZWARNING_LOCKLESS(...) gDebug.ImmediateOut(sDbgMsg::kWarning, __VA_ARGS__)

#define ZERROR(...) gDebug.AddRow(sDbgMsg::kError, __VA_ARGS__)
#define ZERROR_LOCKLESS(...) gDebug.ImmediateOut(sDbgMsg::kError, __VA_ARGS__)


#ifdef _DEBUG
    #define ZDEBUG_OUT(...) 	gDebug.AddRow(sDbgMsg::kDebug, __VA_ARGS__)
    #define ZDEBUG_OUT_LOCKLESS(...) gDebug.ImmediateOut(sDbgMsg::kDebug, __VA_ARGS__)
#else
    #define ZDEBUG_OUT
    #define ZDEBUG_OUT_LOCKLESS 
#endif


