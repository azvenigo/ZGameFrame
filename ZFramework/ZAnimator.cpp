#include "ZAnimator.h"
#include <string>
#include "ZAnimObjects.h"
#include <algorithm>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


ZAnimator::ZAnimator()
{
}

ZAnimator::~ZAnimator()
{
   // Clean up all objects
   for (tAnimObjectList::iterator it = mAnimObjectList.begin(); it != mAnimObjectList.end(); it++)
   {
      ZAnimObject* pObject = *it;
      delete pObject;
   }   
}

bool ZAnimator::Paint()
{
    if (mAnimObjectList.empty())
        return false;

    tAnimObjectList::iterator it;

   // Paint all the objects
    for (it = mAnimObjectList.begin(); it != mAnimObjectList.end(); it++)
    {
        ZAnimObject* pObject = *it;
        if (pObject->GetState() != ZAnimObject::kHidden)
            pObject->Paint();
    }

    // Remove any objects that are in kFinished state
    for (it = mAnimObjectList.begin(); it != mAnimObjectList.end();)
    {
        ZAnimObject* pObject = *it;
        

        if (pObject->GetState() == ZAnimObject::kFinished)
        {
            AddDirtyRect(pObject->mrLastDrawArea);
            tAnimObjectList::iterator itNext = it;
            itNext++;

            mAnimObjectList.erase(it);
            delete pObject;
            //		 ZDEBUG_OUT("cCEAnimator::Paint - deleting object:%x\n", (uint32_t) pObject);

            it = itNext;
            continue;
        }

        it++;
    }

   return mAnimObjectList.empty();  // true if there are still objects to animate
}


void ZAnimator::AddDirtyRect(const ZRect& rDirty)
{
    const std::lock_guard<std::mutex> lock(mPostPaintDirtyListMutex);
    mPostPaintDirtyList.push_back(rDirty);
}

bool ZAnimator::GetDirtyRects(tRectList& outList)
{
    const std::lock_guard<std::mutex> lock(mPostPaintDirtyListMutex);
    if (mPostPaintDirtyList.empty())
        return false;

    outList = std::move(mPostPaintDirtyList);
    return true;
}

bool ZAnimator::AddObject(ZAnimObject* pObject, void* pContext)
{
//	ZDEBUG_OUT("cCEAnimator::AddObject\n");
   mAnimObjectList.push_back(pObject);
   pObject->SetContext(pContext);
   return true;
}

bool ZAnimator::KillObject(ZAnimObject* pObject)
{
//	ZDEBUG_OUT("cCEAnimator::KillObject\n");
   tAnimObjectList::iterator it = std::find(mAnimObjectList.begin(), mAnimObjectList.end(), pObject);
   if (it != mAnimObjectList.end())
   {
      ZAnimObject* pObject = *it;

      mAnimObjectList.erase(it);
      delete pObject;

      return true;
   }

   return false;
}

bool ZAnimator::KillContextObjects(void* pContext)
{
//	ZDEBUG_OUT("cCEAnimator::KillContextObjects\n");
	for (tAnimObjectList::iterator it = mAnimObjectList.begin(); it != mAnimObjectList.end();)
	{
		tAnimObjectList::iterator itNext = it;
		itNext++;

		ZAnimObject* pObject = *it;
		if (pObject->GetContext() == pContext)
		{
			mAnimObjectList.erase(it);
			delete pObject;
		}

		it = itNext;
	}

	return true;
}

bool ZAnimator::KillAllObjects()
{
//	ZDEBUG_OUT("cCEAnimator::KillAllObjects\n");
	for (tAnimObjectList::iterator it = mAnimObjectList.begin(); it != mAnimObjectList.end(); it++)
	{
		ZAnimObject* pObject = *it;
		delete pObject;
	}
	mAnimObjectList.clear();

	return true;
}
