#pragma once
#include "ZStdTypes.h"
#define _USE_MATH_DEFINES
#include <math.h>

namespace Z3D
{
    void LookAt(const Vec3f& from, const Vec3f& to, const Vec3f& up, Matrix44f& m)
    {
        Vec3f forward = (from - to);
        forward.normalize();
        Vec3f right = up.cross(forward);
        right.normalize();


        Vec3f newup = forward.cross(right);

        m[0][0] = right.x;      m[0][1] = right.y;      m[0][2] = right.z;
        m[1][0] = newup.x;      m[1][1] = newup.y;      m[1][2] = newup.z;
        m[2][0] = forward.x;    m[2][1] = forward.y;    m[2][2] = forward.z;
        m[3][0] = from.x;       m[3][1] = from.y;       m[3][2] = from.z;
    }

    void setProjectionMatrix(const double& angleOfView, const double& fNear, const double& fFar, Matrix44f& M)
    {
        // set the basic projection matrix
        float scale = 1 / tan(angleOfView * 0.5 * M_PI / 180);
        M[0][0] = scale;  //scale the x coordinates of the projected point 
        M[1][1] = scale;  //scale the y coordinates of the projected point 
        M[2][2] = -fFar / (fFar - fNear);  //used to remap z to [0,1] 
        M[3][2] = -fFar * fNear / (fFar - fNear);  //used to remap z [0,1] 
        M[2][3] = -1;  //set w = -z 
        M[3][3] = 0;
    }

    void setOrientationMatrix(float fX, float fY, float fZ, Matrix44f& M)
    {
        Matrix44f MX;
        MX[1][1] =  cos(fX);   MX[2][1] =  sin(fX);
        MX[1][2] = -sin(fX);   MX[2][2] =  cos(fX);

        Matrix44f MY;
        MY[0][0] =  cos(fY);   MY[2][0] = -sin(fY);
        MY[0][2] =  sin(fY);   MY[2][2] =  cos(fY);

        Matrix44f MZ;
        MZ[0][0] =  cos(fZ);   MZ[1][0] =  sin(fZ);
        MZ[0][1] = -sin(fZ);   MZ[1][1] =  cos(fZ);


        M = MZ * MY * MX;
    }


};
