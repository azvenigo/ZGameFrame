#include "Z3DObjects.h"

void Z3DSphere::Set(const Vec3f& pos, float fRadius, float fTransparency, float fReflection, const Vec3f& surfaceCol, const Vec3f& emissionCol)
{
    mPos = pos;
    mRadius = fRadius;
    mRadius2 = fRadius * fRadius;
    mTransparency = fTransparency;
    mReflection = fReflection;
    mSurfaceColor = surfaceCol;
    mEmissionColor = emmission;
}

bool Z3DSphere::intersect(const Vec3f& rayOrigin, const Vec3f& rayDirection, Z3DSurfaceIntersection& result)
{
    Vec3f l = mPos - rayOrigin;
    float tca = l.dotProduct(rayDirection);
    if (tca < 0) return false;
    float d2 = l.dotProduct(l) - tca * tca;
    if (d2 > mRadius2) return false;
    float thc = sqrt(mRadius2 - d2);
    t0 = tca - thc;
    t1 = tca + thc;

    if (t0 < 0)
        result.mDistance = t1;
    else
        result.mDistance = t0;
    result.

    return true;
}
