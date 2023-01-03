#pragma once

#include "ZStdTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZTimer.h"
#include <vector>
#include "Z3DMath.h"

/////////////////////////////////////////////////////////////////////////
// 

class CubeSide
{
public:
    CubeSide()
    {
        mSide[0] = 0;
        mSide[1] = 0;
        mSide[2] = 0;
        mSide[3] = 0;

        mColor = 0xff000000;
    }

    CubeSide(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint32_t nCol)
    {
        mSide[0] = a;
        mSide[1] = b;
        mSide[2] = c;
        mSide[3] = d;

        mColor = nCol;
    }

    uint8_t mSide[4];
    uint32_t mColor;
};

#define RENDER_SPHERES


class Z3DTestWin : public ZWin
{
public:
    Z3DTestWin();

    bool    Init();
    bool    Paint();
    bool    Process();
    bool    OnChar(char key);

    void    RenderPoly(std::vector<Z3D::Vec3f>& worldVerts, Z3D::Matrix44f& mtxProjection, Z3D::Matrix44f& mtxWorldToCamera, uint32_t nCol);
    void    RenderPoly(std::vector<Z3D::Vec3f>& worldVerts, Z3D::Matrix44f& mtxProjection, Z3D::Matrix44f& mtxWorldToCamera, tZBufferPtr pTexture);
    bool	HandleMessage(const ZMessage& message);

private:

    std::vector<Z3D::Vec3f> mCubeVertices;
    std::vector< CubeSide> mSides;

    Z3D::Matrix44f mObjectToWorld;
    Z3D::Matrix44f mWorldToObject;

    tZBufferPtr mpTexture;

#ifdef RENDER_TEAPOT
    tZBufferPtr mpTeapotRender;
    void    RenderTeapot();
#endif

#ifdef RENDER_SPHERES
    tZBufferPtr mpSpheresRender;
    void UpdateSphereCount();
    void Z3DTestWin::RenderSpheres(tZBufferPtr mpSurface);
//    static Z3D::Vec3f   TraceSpheres(const Z3D::Vec3f& rayorig, const Z3D::Vec3f& raydir, const int& depth, const std::vector<class Sphere>& spheres);

    std::vector<class Sphere> mSpheres;
    int64_t     mnTargetSphereCount;
    int64_t     mnMinSphereSizeTimes100;
    int64_t     mnMaxSphereSizeTimes100;
    int64_t     mnRotateSpeed;
    double      mfBaseAngle;

    bool        mbRenderCube;
    bool        mbRenderSpheres;

    bool        mbOuterSphere;
    bool        mbCenterSphere;

    int64_t     mnRenderSize;
#endif

    uint64_t mLastTimeStamp;

};

