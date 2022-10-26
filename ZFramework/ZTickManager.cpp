#include "ZTickManager.h"
#include "ZTransformable.h"
#include <algorithm>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


ZTickManager::ZTickManager()
{
}

ZTickManager::~ZTickManager()
{
}

bool ZTickManager::AddObject(ZTransformable* pObject)
{
//	ZDEBUG_OUT("cCETickManager::Tick - AddObject:%s\n", pObject->msDebugName.c_str());
	//std::lock_guard<std::mutex> lock(mMutex);

	if (mTransformableObjectList.empty())		// special case if list is empty
	{
		mTransformableObjectList.push_back(pObject);
		mTickNextIterator = mTransformableObjectList.begin();
	}
	else
	{
		tTransformableObjectList::iterator it = std::find(mTransformableObjectList.begin(), mTransformableObjectList.end(), pObject);
		if (it == mTransformableObjectList.end())
		{
			mTransformableObjectList.push_back(pObject);
		}
	}

	return true;
}

bool ZTickManager::RemoveObject(ZTransformable* pObject)
{
//	ZDEBUG_OUT("cCETickManager::Tick - RemoveObject:%s\n", pObject->msDebugName.c_str());
	//std::lock_guard<std::mutex> lock(mMutex);

	tTransformableObjectList::iterator it = std::find(mTransformableObjectList.begin(), mTransformableObjectList.end(), pObject);
	if (it != mTransformableObjectList.end())
	{
		// If within a callback to tick, ensure we're preserving the mTickNextIterator by advancing it
		if (mTickNextIterator == it)
			mTickNextIterator++;

		mTransformableObjectList.erase(it);
		return true;
	}

	return false;
}

bool ZTickManager::Tick()
{
	bool bHasMoreTicks = false;
	for (tTransformableObjectList::iterator it = mTransformableObjectList.begin(); it != mTransformableObjectList.end();)
	{
		// Using itNext in case an iterator is invalidated by being erased from a callback
		mTickNextIterator = it;
		mTickNextIterator++;

		ZTransformable* pTransformable = *it;
		bool bTick = pTransformable->Tick();
		bHasMoreTicks |= bTick;
//		if (bTick)
//		{
//			ZDEBUG_OUT("%s returned more ticks\n", pTransformable->msDebugName.c_str());
//		}

		it = mTickNextIterator;
	}
//	ZDEBUG_OUT("cCETickManager::Tick - end\n");

	return bHasMoreTicks;
}
