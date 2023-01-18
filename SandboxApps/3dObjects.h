#pragma once

#include "ZStdTypes.h"
#include <vector>
#include "Z3DMath.h"

class Z3DSurfaceIntersection
{
    Z3DSurfaceIntersection(float dist = 0, const Vec3f& point = 0, const Vec3f col = 0, int64_t id = 0)
    {
        mDistance = dist;
        mPoint = point;
        mColor = col;
        mObjectID = id;
    }

    float mDistance;
    Vec3f mPoint;
    Vec3f mColor;
    int64_t mObjectID;
    
};

class Z3DObject
{
public:
    Z3DObject();
    virtual ~Z3DObject();

    virtual bool intersect(const Vec3f& rayOrigin, const Vec3f& rayDirection, Z3DSurfaceIntersection& result) const = 0;
    virtual void SetPos(const Vec3f& pos);
    virtual void SetOrientation(const Vec3f& orientation);


protected:
    Vec3f mPos;
    Vec3f mOrientation;

    Matrix44f mObject2World;
    Matrix44f mWorld2Object;
};

typedef std::list<Z3DObject> tZ3DObjectList;

class Z3DSphere : public Z3DObject
{
    Z3DSphere() : Z3DObject(), mRadius(0), mRadius2(0), mTransparency(0), mReflection(0) {}
    bool Set(const Vec3f& pos, float fRadius, float fTransparency = 0, float fReflection = 1, const Vec3f& surfaceCol = 1, const Vec3f& emissionCol = 0);
    virtual bool intersect(const Vec3f& from, const Vec3f& to, const tZ3DObjectList& objects, Z3DSurfaceIntersection& result);

protected:
    float mRadius, mRadius2;                  /// sphere radius and radius^2
    Vec3f mSurfaceColor, mEmissionColor;      /// surface color and emission (light)
    float mTransparency, mReflection;         /// surface transparency and reflectivity
};