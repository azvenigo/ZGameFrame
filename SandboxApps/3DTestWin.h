#pragma once

#include "ZTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZTimer.h"
#include <vector>
#include "Z3DMath.h"
#include "ZD3D.h"

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

//#define RENDER_TEAPOT
#define RENDER_SPHERES


class Z3DTestWin : public ZWin
{
public:
    Z3DTestWin();

    bool    Init();
    bool    Shutdown();
    bool    Paint();
    bool    Process();
    bool    OnChar(char key);

    void    SetControlPanelEnabled(bool bEnabled = true) { mbControlPanelEnabled = bEnabled; }

    void    RenderPoly(std::vector<Z3D::Vec3d>& worldVerts, Z3D::Matrix44d& mtxProjection, Z3D::Matrix44d& mtxWorldToCamera, uint32_t nCol);
    void    RenderPoly(std::vector<Z3D::Vec3d>& worldVerts, Z3D::Matrix44d& mtxProjection, Z3D::Matrix44d& mtxWorldToCamera, ZD3D::tDynamicTexturePtr pTexture);
    bool	HandleMessage(const ZMessage& message);

private:

    std::vector<Z3D::Vec3d> mCubeVertices;
    std::vector< CubeSide> mSides;

    Z3D::Matrix44d mObjectToWorld;
    Z3D::Matrix44d mWorldToObject;

    tZBufferPtr mpTexture;
    ZD3D::tDynamicTexturePtr mpDynTexture;
    bool mbControlPanelEnabled;

    size_t mFramePrimCount;
    std::vector<ZD3D::ScreenSpacePrimitive*> mReservedPrims;

#ifdef RENDER_TEAPOT
    tZBufferPtr mpTeapotRender;
    void    RenderTeapot();
#endif

#ifdef RENDER_SPHERES
    tZBufferPtr mpSpheresRender;
    void UpdateSphereCount();
    void RenderSpheres(tZBufferPtr pSurface);
//    static Z3D::Vec3d   TraceSpheres(const Z3D::Vec3d& rayorig, const Z3D::Vec3d& raydir, const int& depth, const std::vector<class Sphere>& spheres);

    std::vector<class Sphere> mSpheres;
    int64_t     mnTargetSphereCount;
    int64_t     mnMinSphereSizeTimes100;
    int64_t     mnMaxSphereSizeTimes100;
    int64_t     mnRotateSpeed;
    int64_t     mnRayDepth;
    int64_t     mnFOVTime100;
    double      mfBaseAngle;

    bool        mbRenderCube;
    bool        mbRenderSpheres;

    bool        mbOuterSphere;
    bool        mbCenterSphere;

    int64_t     mnRenderSize;
#endif

    uint64_t mLastTimeStamp;

};

