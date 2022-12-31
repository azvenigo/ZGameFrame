#include "3DTestWin.h"
#include "ZRandom.h"
#include "ZFont.h"
#include "ZRasterizer.h"
#include "ZStdTypes.h"
#include "Z3DMath.h"

using namespace std;
using namespace Z3D;

Z3DTestWin::Z3DTestWin()
{
    mIdleSleepMS = 1000;
    mLastTimeStamp = gTimer.GetMSSinceEpoch();
}
   
bool Z3DTestWin::Init()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mbInvalidateParentWhenInvalid = true;


    mCubeVertices.resize(8);

    const float f = 1.0;

    mCubeVertices[0] = Vec3f(-f,  f, -f);
    mCubeVertices[1] = Vec3f( f,  f, -f);
    mCubeVertices[2] = Vec3f( f, -f, -f);
    mCubeVertices[3] = Vec3f(-f, -f, -f);

    mCubeVertices[4] = Vec3f(-f,  f,  f);
    mCubeVertices[5] = Vec3f( f,  f,  f);
    mCubeVertices[6] = Vec3f( f, -f,  f);
    mCubeVertices[7] = Vec3f(-f, -f,  f);

    mSides.resize(6);

    mSides[0].mSide[0] = 0;
    mSides[0].mSide[1] = 1;
    mSides[0].mSide[2] = 2;
    mSides[0].mSide[3] = 3;

    mSides[1].mSide[0] = 1;
    mSides[1].mSide[1] = 5;
    mSides[1].mSide[2] = 6;
    mSides[1].mSide[3] = 2;

    mSides[2].mSide[0] = 5;
    mSides[2].mSide[1] = 4;
    mSides[2].mSide[2] = 7;
    mSides[2].mSide[3] = 6;

    mSides[3].mSide[0] = 4;
    mSides[3].mSide[1] = 0;
    mSides[3].mSide[2] = 3;
    mSides[3].mSide[3] = 7;

    mSides[4].mSide[0] = 4;
    mSides[4].mSide[1] = 5;
    mSides[4].mSide[2] = 1;
    mSides[4].mSide[3] = 0;

    mSides[5].mSide[0] = 6;
    mSides[5].mSide[1] = 7;
    mSides[5].mSide[2] = 3;
    mSides[5].mSide[3] = 2;

    mSides[0].mColor = 0xff0000ff;
    mSides[1].mColor = 0xffff0000;
    mSides[2].mColor = 0xff00ff00;
    mSides[3].mColor = 0xffff00ff;

    mSides[4].mColor = 0xffffffff;
    mSides[5].mColor = 0xff000000;


    mpTexture.reset(new ZBuffer());
    mpTexture.get()->LoadBuffer("res/brick-texture.jpg");


    SetFocus();

    return ZWin::Init();
}

bool Z3DTestWin::OnChar(char key)
{
#ifdef _WIN64
    switch (key)
    {
    case VK_ESCAPE:
        gMessageSystem.Post("quit_app_confirmed");
        break;
    }
#endif
    return ZWin::OnChar(key);
}



void multPointMatrix(const Vec3f& in, Vec3f& out, const Matrix44f& M)
{
    //out = in * M;
    out.x = in.x * M[0][0] + in.y * M[1][0] + in.z * M[2][0] + /* in.z = 1 */ M[3][0];
    out.y = in.x * M[0][1] + in.y * M[1][1] + in.z * M[2][1] + /* in.z = 1 */ M[3][1];
    out.z = in.x * M[0][2] + in.y * M[1][2] + in.z * M[2][2] + /* in.z = 1 */ M[3][2];
    float w = in.x * M[0][3] + in.y * M[1][3] + in.z * M[2][3] + /* in.z = 1 */ M[3][3];

    // normalize if w is different than 1 (convert from homogeneous to Cartesian coordinates)
    if (w != 1) {
        out.x /= w;
        out.y /= w;
        out.z /= w;
    }
}




bool Z3DTestWin::Process()
{
    return true;
}

void Z3DTestWin::RenderPoly(vector<Vec3f>& worldVerts, Matrix44f& mtxProjection, Matrix44f& mtxWorldToCamera, uint32_t nCol)
{
    tColorVertexArray screenVerts;
    screenVerts.resize(worldVerts.size());


/*    Vec3f faceX(worldVerts[0].x - worldVerts[1].x, worldVerts[0].y - worldVerts[1].y, 1);
    Vec3f faceY(worldVerts[0].x - worldVerts[3].x, worldVerts[0].y - worldVerts[3].y, 1);
    Vec3f faceNormal = faceX.cross(faceY);
    faceNormal.normalize();
    Vec3f lightDirection(5, 5, 1);
    lightDirection.normalize();

    double fScale = lightDirection.dot(-faceNormal);
    if (fScale < 0)
        fScale = 0;
    if (fScale > 1.0)
        fScale = 1.0;

    double fR = ARGB_R(nCol) * fScale;
    double fG = ARGB_G(nCol) * fScale;
    double fB = ARGB_B(nCol) * fScale;

    nCol = ARGB(0xff, (uint8_t)fR, (uint8_t)fG, (uint8_t)fB);
    */


    for (int i = 0; i < worldVerts.size(); i++)
    {
        Vec3f v = worldVerts[i];

        Vec3f vertCamera;
        Vec3f vertProjected;

        multPointMatrix(v, vertCamera, mtxWorldToCamera);
        multPointMatrix(vertCamera, vertProjected, mtxProjection);

/*        if (vertProjected.x < -1 || vertProjected.x > 1 || vertProjected.y < -1 || vertProjected.y > 1)
            continue;*/

        screenVerts[i].mX = mAreaToDrawTo.Width()/2 + (int64_t)(vertProjected.x * 4000);
        screenVerts[i].mY = mAreaToDrawTo.Height()/2 + (int64_t)(vertProjected.y * 4000);

        screenVerts[i].mColor = nCol;
    }

    Vec3f planeX(screenVerts[1].mX- screenVerts[0].mX, screenVerts[1].mY - screenVerts[0].mY, 1);
    Vec3f planeY(screenVerts[0].mX - screenVerts[3].mX, screenVerts[0].mY - screenVerts[3].mY, 1);
    Vec3f normal = planeX.cross(planeY);
    if (normal.z > 0)
        return;

    gRasterizer.Rasterize(mpTransformTexture.get(), screenVerts);
}

void Z3DTestWin::RenderPoly(vector<Vec3f>& worldVerts, Matrix44f& mtxProjection, Matrix44f& mtxWorldToCamera, tZBufferPtr pTexture)
{
    tUVVertexArray screenVerts;
    screenVerts.resize(worldVerts.size());

    for (int i = 0; i < worldVerts.size(); i++)
    {
        Vec3f v = worldVerts[i];

        Vec3f vertCamera;
        Vec3f vertProjected;

        multPointMatrix(v, vertCamera, mtxWorldToCamera);
        multPointMatrix(vertCamera, vertProjected, mtxProjection);

        /*        if (vertProjected.x < -1 || vertProjected.x > 1 || vertProjected.y < -1 || vertProjected.y > 1)
                    continue;*/

        screenVerts[i].mX = mAreaToDrawTo.Width() / 2 + (int64_t)(vertProjected.x * 4000);
        screenVerts[i].mY = mAreaToDrawTo.Height() / 2 + (int64_t)(vertProjected.y * 4000);
    }

    Vec3f planeX(screenVerts[1].mX - screenVerts[0].mX, screenVerts[1].mY - screenVerts[0].mY, 1);
    Vec3f planeY(screenVerts[0].mX - screenVerts[3].mX, screenVerts[0].mY - screenVerts[3].mY, 1);
    Vec3f normal = planeX.cross(planeY);
    if (normal.z > 0)
        return;


    screenVerts[0].mU = 0.0;
    screenVerts[0].mV = 0.0;

    screenVerts[1].mU = 1.0;
    screenVerts[1].mV = 0.0;

    screenVerts[2].mU = 1.0;
    screenVerts[2].mV = 1.0;

    screenVerts[3].mU = 0.0;
    screenVerts[3].mV = 1.0;

    gRasterizer.Rasterize(mpTransformTexture.get(), pTexture.get(), screenVerts);
}



bool Z3DTestWin::Paint()
{
    if (!mbInvalid)
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    mpTransformTexture->FillAlpha(mAreaToDrawTo, 0xff000088);


    uint64_t nTime = gTimer.GetMSSinceEpoch();
    string sTime;
    Sprintf(sTime, "fps: %f", 1000.0 / (nTime - mLastTimeStamp));
    gpFontSystem->GetDefaultFont()->DrawText(mpTransformTexture.get(), sTime, mAreaToDrawTo);
    mLastTimeStamp = nTime;

/*    tColorVertexArray verts;

    verts.resize(5);
    verts[0].mX = 100;
    verts[0].mY = 100;
    verts[0].mColor = 0xffff00ff;

    verts[1].mX = 500;
    verts[1].mY = 100;
    verts[1].mColor = 0xffffff00;

    verts[2].mX = 250;
    verts[2].mY = 500;
    verts[2].mColor = 0xffffffff;

    verts[3].mX = 200;
    verts[3].mY = 800;
    verts[3].mColor = 0x00fffff;

    verts[4].mX = 800;
    verts[4].mY = 600;
    verts[4].mColor = 0xff000000;

    gRasterizer.Rasterize(mpTransformTexture.get(), verts);*/


    Matrix44f mtxProjection;
    Matrix44f mtxWorldToCamera;

//    Z3D::LookAt(Vec3f(10*sin(gTimer.GetMSSinceEpoch() / 1000.0), 0, -10-10*cos(gTimer.GetMSSinceEpoch()/1000.0)), Vec3f(0, 0, 0), Vec3f(0, 1, 0), mtxWorldToCamera);
//    Z3D::LookAt(Vec3f(0, 1, -20 - 10 * cos(gTimer.GetMSSinceEpoch() / 1000.0)), Vec3f(0, 0, 0), Vec3f(0, 10, 0), mtxWorldToCamera);
    Z3D::LookAt(Vec3f(0, 1, -20), Vec3f(0, 0, 0), Vec3f(0, 10, 0), mtxWorldToCamera);
    
    double fFoV = 90;
    double fNear = 0.1;
    double fFar = 100;

    setProjectionMatrix(fFoV, fNear, fFar, mtxProjection);


    vector<Vec3f> worldVerts;
    worldVerts.resize(4);

    int i = 1;
//    for (; i < 100; i++)
    {
//        setOrientationMatrix((float)sin(i)*gTimer.GetElapsedTime() / 1050.0, (float)gTimer.GetElapsedTime() / 8000.0, (float)gTimer.GetElapsedTime() / 1000.0, mObjectToWorld);
        setOrientationMatrix(4.0, 0.0, (float)gTimer.GetElapsedTime() / 1000.0, mObjectToWorld);

        std::vector<Vec3f> cubeWorldVerts;
        cubeWorldVerts.resize(8);
        for (int i = 0; i < 8; i++)
            multPointMatrix(mCubeVertices[i], cubeWorldVerts[i], mObjectToWorld);

        for (int i = 0; i < 6; i++)
        {
            worldVerts[0] = cubeWorldVerts[mSides[i].mSide[0]];
            worldVerts[1] = cubeWorldVerts[mSides[i].mSide[1]];
            worldVerts[2] = cubeWorldVerts[mSides[i].mSide[2]];
            worldVerts[3] = cubeWorldVerts[mSides[i].mSide[3]];

#define RENDER_TEXTURE

#ifdef RENDER_TEXTURE
            RenderPoly(worldVerts, mtxProjection, mtxWorldToCamera, mpTexture);
#else
            RenderPoly(worldVerts, mtxProjection, mtxWorldToCamera, mSides[i].mColor);
#endif
        }
    }

    ZWin::Paint();

    mbInvalid = true;
    return true;
}
   