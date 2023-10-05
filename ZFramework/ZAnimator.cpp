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
            tAnimObjectList::iterator itNext = it;
            itNext++;

            mAnimObjectList.erase(it);
            delete pObject;
            //		 ZDEBUG_OUT("cCEAnimator::Paint - deleting object:%x\n", (uint32_t) pObject);

            it = itNext;
            continue;
        }
        else
            AddDirtyRects(pObject->mrLastDrawArea, pObject->mrArea);   // compute the areas left behind after animation objects have left

        it++;
    }

   return mAnimObjectList.empty();  // true if there are still objects to animate
}

void ZAnimator::AddDirtyRects(const ZRect& rOldArea, const ZRect& rNewArea)
{
    ZRect rOverlap(rNewArea);
    rOverlap.IntersectRect(rOldArea);

    ZRect rDest(rOldArea);



    	//                              rDest
	//     lw      mw      rw     /
	//  ---^-------^-------^---- /
	//         |       |        |             
	//    TL   |   T   |   TR   > th         
	//         |       |        |              
	//  -------+-------+--------|             
	//         |       |        |              
	//     L   |Overlap|   R    > mh             
	//         |       |        |              
	//  -------+-------+--------|             
	//         |       |        |              
	//    BL   |   B   |   BR   > bh 
	//         |       |        |          






    int64_t lw = rOverlap.left - rDest.left;
    int64_t mw = rOverlap.Width();
    int64_t rw = rDest.right - rOverlap.right;

    int64_t th = rOverlap.top - rDest.top;
    int64_t mh = rOverlap.Height();
    int64_t bh = rDest.bottom - rOverlap.bottom;






    const std::lock_guard<std::mutex> lock(mPostPaintDirtyListMutex);

    if (lw != 0)		// Consider left three?
    {
        if (th != 0)
        {
            // TL
            mPostPaintDirtyList.emplace_back(ZRect(rDest.left, rDest.top, rDest.left + lw, rDest.top + th));
        }

        if (mh != 0)
        {
            // L
            mPostPaintDirtyList.emplace_back(ZRect(rDest.left, rOverlap.top, rDest.left + lw, rOverlap.bottom));
        }

        if (bh != 0)
        {
            // BL
            mPostPaintDirtyList.emplace_back(ZRect(rDest.left, rOverlap.bottom, rOverlap.left, rDest.bottom));
        }
    }

    if (mw != 0)			// consider top and bottom two?
    {
        if (th != 0)
        {
            // T
            mPostPaintDirtyList.emplace_back(ZRect(rOverlap.left, rDest.top, rOverlap.right, rDest.top + th));
        }

        if (bh != 0)
        {
            // B
            mPostPaintDirtyList.emplace_back(ZRect(rOverlap.left, rOverlap.bottom, rOverlap.right, rDest.bottom));
        }
    }

    if (rw != 0)		// consider right three?
    {
        if (th != 0)
        {
            // TR
            mPostPaintDirtyList.emplace_back(ZRect(rOverlap.right, rDest.top, rDest.right, rOverlap.top));
        }

        if (mh != 0)
        {
            // R
            mPostPaintDirtyList.emplace_back(ZRect(rOverlap.right, rOverlap.top, rDest.right, rOverlap.bottom));
        }

        if (bh != 0)
        {
            // BR
            mPostPaintDirtyList.emplace_back(ZRect(rOverlap.right, rOverlap.bottom, rDest.right, rDest.bottom));
        }
    }
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
