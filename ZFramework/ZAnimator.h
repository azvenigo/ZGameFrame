#pragma once

#include <list>
#include "ZTypes.h"
#include "ZAnimObjects.h"

class ZBuffer;

typedef std::list<ZAnimObject*> tAnimObjectList;

class ZAnimator
{
public:
    ZAnimator();
    ~ZAnimator();

    bool Paint();
    bool AddObject(ZAnimObject* pObject, void* pContext = NULL);     // Once the object is added, the cCEAnimator is the owner and will delete the object when necessary
    bool KillObject(ZAnimObject* pObject);    // Manually kill the object
    bool KillContextObjects(void* pContext);
    bool KillAllObjects();
    
    bool HasActiveObjects() { return !mAnimObjectList.empty(); }
    bool HasFullScreenDrawObjects();

    bool GetDirtyRects(tRectList& outList); // std::moves the list out

protected:
    tRectList mPostPaintDirtyList;        // rects that need to be redrawn after animation object has moved away
    std::mutex mPostPaintDirtyListMutex;


    void AddDirtyRect(const ZRect& rDirty);
    tAnimObjectList mAnimObjectList;
};

extern ZAnimator        gAnimator;
