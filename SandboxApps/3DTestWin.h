#pragma once

#include "ZStdTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZTimer.h"
#include <vector>

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

class Z3DTestWin : public ZWin
{
public:
    Z3DTestWin();

    bool    Init();
    bool    Paint();
    bool    Process();
    bool    OnChar(char key);

    void    RenderPoly(std::vector<Vec3f>& worldVerts, Matrix44f& mtxProjection, Matrix44f& mtxWorldToCamera, uint32_t nCol);
    void    RenderPoly(std::vector<Vec3f>& worldVerts, Matrix44f& mtxProjection, Matrix44f& mtxWorldToCamera, tZBufferPtr pTexture);

private:

    std::vector<Vec3f> mCubeVertices;
    std::vector< CubeSide> mSides;

    Matrix44f mObjectToWorld;
    Matrix44f mWorldToObject;

    tZBufferPtr mpTexture;

    uint64_t mLastTimeStamp;

};

