#include "ZTimer.h"


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ZTimer::ZTimer(bool bStart)
{
	mnTimeAccumulator = 0;
	if (bStart)
		Start();
}

void ZTimer::Reset()
{
	mnTimeAccumulator = 0;
	mnLastTimeStamp = GetMSSinceEpoch();
}

void ZTimer::Start()
{
	mbRunning = true;
	mnLastTimeStamp = GetMSSinceEpoch();
}

void ZTimer::Stop()
{
	if (mbRunning)
	{
		int64_t nCurrentTime = GetMSSinceEpoch();
		int64_t nDelta = (int64_t) nCurrentTime - (int64_t) mnLastTimeStamp;
		ZASSERT(nDelta >= 0);
		mnLastTimeStamp = nCurrentTime;
		mnTimeAccumulator += nDelta;
	}
	mbRunning = false;
}
