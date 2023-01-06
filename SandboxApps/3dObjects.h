#pragma once

#include "ZStdTypes.h"
#include <vector>
#include "Z3DMath.h"

class Z3DObject
{
public:
    Z3DObject();
    virtual ~Z3DObject();

    virtual bool intersect(const Vec3f&, const Vec3f&, float&, uint32_t&, Vec2f&) const = 0;

};