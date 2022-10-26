#pragma once

#include <list>
#include <mutex>

class ZTransformable;

typedef std::list<ZTransformable*> tTransformableObjectList;

class ZTickManager
{
public:
	ZTickManager();
	~ZTickManager();

	bool AddObject(ZTransformable* pObject);
	bool RemoveObject(ZTransformable* pObject);

	bool Tick();		// returns true if there are still objects to be ticked

protected:
	tTransformableObjectList			mTransformableObjectList;
	std::mutex							mMutex;
private:
	tTransformableObjectList::iterator	mTickNextIterator;
};

