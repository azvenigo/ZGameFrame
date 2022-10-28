#pragma once

#include "ZStdTypes.h"
#include <string>
#include "ZStringHelpers.h"
#include "ZStdDebug.h"
#include <chrono>


class ZTimer
{
public:
	ZTimer(bool bStart = false);

	void			Start();
	void			Stop();
	void			Reset();
	inline int64_t	GetElapsedTime();

	inline int64_t	GetMSSinceEpoch();
	inline int64_t	GetUSSinceEpoch();
    inline double   GetElapsedSecondsSinceEpoch();
protected:
	bool			mbRunning;
	int64_t			mnTimeAccumulator;
	int64_t			mnLastTimeStamp;
};


extern ZTimer      gTimer;      // Application should define a global timer


inline int64_t ZTimer::GetMSSinceEpoch()
{
	return std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
}

inline int64_t ZTimer::GetUSSinceEpoch()
{
	return std::chrono::system_clock::now().time_since_epoch() / std::chrono::microseconds(1);
}


inline
int64_t ZTimer::GetElapsedTime()
{
	if (mbRunning)
	{
		int64_t nCurrentTime = GetMSSinceEpoch();
		int64_t nDelta = nCurrentTime - mnLastTimeStamp;
		ZASSERT(nDelta >= 0);
		mnLastTimeStamp = nCurrentTime;
		mnTimeAccumulator += nDelta;
	}

	return mnTimeAccumulator;
}


inline double ZTimer::GetElapsedSecondsSinceEpoch()
{
    return (double) (std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1)) / 1000.0;
}



class ZSpeedRecord
{
public:
    ZSpeedRecord() { Clear(); }

    inline void Clear() { nSamples = 0; nBytes = 0; fUSSpent = 0.0; }

    friend std::ostream& operator<<(std::ostream& os, const ZSpeedRecord& sr);
    ZSpeedRecord& operator+=(const ZSpeedRecord& sr)
    {
        nBytes += sr.nBytes;
        fUSSpent = fUSSpent + sr.fUSSpent;
        nSamples += sr.nSamples;
        return *this;
    }

    void AddSample(int64_t bytes, double msSpent)
    {
        nBytes += bytes;
        fUSSpent = fUSSpent + msSpent;
        nSamples++;
    }

    double GetBytesPerSecond()  { return (1000000.0*(double)nBytes)                           / fUSSpent; }    // times 1000.0 to scale up and then divide by us to get rate in seconds
    double GetKBPerSecond()     { return (1000000.0*(double)nBytes/1024.0)                    / fUSSpent; }    // times 1000.0 to scale up and then divide by us to get rate in seconds
    double GetMBPerSecond()     { return (1000000.0*(double)nBytes/(1024.0*1024.0))           / fUSSpent; }    // times 1000.0 to scale up and then divide by us to get rate in seconds
    double GetGBPerSecond()     { return (1000000.0*(double)nBytes/(1024.0*1024.0*1024.0))    / fUSSpent; }    // times 1000.0 to scale up and then divide by us to get rate in seconds

    std::atomic<int64_t> nSamples;
    std::atomic<int64_t> nBytes;
    std::atomic<double>  fUSSpent;
};

inline std::ostream& operator<<(std::ostream& os, const ZSpeedRecord& sr)
{
    os << "samples: " << sr.nSamples << " bytes: " << sr.nBytes << " timeUS:" << sr.fUSSpent;
    return os;
}






#ifdef _DEBUG
class cDebugTimer
{
public:
	cDebugTimer(std::string sName);
	~cDebugTimer();

	void Enter();
	void Exit();

protected:
	ZTimer	    mTimer;
	int64_t		mnCount;
	int64_t		mnTotalTime;
    std::string	msName;
};

inline
cDebugTimer::cDebugTimer(std::string sName)
{
	msName = sName;
	mnCount = 0;
	mnTotalTime = 0;
}

inline
cDebugTimer::~cDebugTimer()
{
    std::string sTemp;
	if (mnCount > 0)
		Sprintf(sTemp, "%s: Times called:%ld  Total time:%ld  us per call:%ld\n", msName.c_str(), mnCount, mnTotalTime, mnTotalTime/mnCount);
	else
		Sprintf(sTemp, "%s: Never called\n", msName.c_str());
	//	DEBUG_OUT(sTemp.c_str());
}

inline
void cDebugTimer::Enter()
{
	mTimer.Reset();
	mTimer.Start();
}

inline
void cDebugTimer::Exit()
{
	mnTotalTime += mTimer.GetElapsedTime();
	mnCount++;
}





#define DEBUG_TIME_DECLARE(name) static cDebugTimer name(#name);
#define DEBUG_TIME_ENTER(name) name.Enter();
#define DEBUG_TIME_EXIT(name) name.Exit();
#else
#define DEBUG_TIME_DECLARE(name)
#define DEBUG_TIME_ENTER(name)
#define DEBUG_TIME_EXIT(name)
#endif

#define TIME_SECTION_START(name) uint64_t InlineSectionStartTime = gTimer.GetUSSinceEpoch();
#define TIME_SECTION_END(name) uint64_t InlineSectionEndTime = gTimer.GetUSSinceEpoch(); OutputDebug("Time Section %s took: %lld us\n", #name, InlineSectionEndTime - InlineSectionStartTime);