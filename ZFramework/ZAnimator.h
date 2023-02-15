#ifndef CEANIMATOR_H
#define CEANIMATOR_H

#include <list>
#include "ZStdTypes.h"
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

protected:
   tAnimObjectList mAnimObjectList;
};


#endif