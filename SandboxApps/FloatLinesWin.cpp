#include "FloatLinesWin.h"
#include "ZBuffer.h"
#include "ZTypes.h"
#include "ZMessageSystem.h"
//#include "ZAnimObjects.h"
#include "ZGraphicSystem.h"
#include "Resources.h"
#include "ZStringHelpers.h"
//#include "ZWinScriptedDialog.h"
#include <iostream>
#include <Zrandom.h>
#include "ZTimer.h"
using namespace std;

extern ZMessageSystem gMessageSystem;



double GetRandExponentialDist(double fMax)
{
    int nMaxAttempts = 10;
    while (nMaxAttempts-- > 0)
    {
        std::exponential_distribution  <double> distribution(15.0);
        double fRand = distribution(gRandGenerator) * fMax;
        if (fRand >= 0.0 && fRand < fMax)
            return fRand;
    }

    return 0.0;
}

/*int64_t GetRandUniformDist(int64_t nMin, int64_t nMax)
{
    std::uniform_int_distribution  <int64_t> distribution(nMin, nMax);
    int64_t nRand = distribution(gRandGenerator);
    return nRand;
}*/


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


ZLine::ZLine()
{
    mfX1 = 0.0f;
    mfY1 = 0.0f;
    mfX2 = 0.0f;
    mfY2 = 0.0f;

    mfdX1 = 0.0f;
    mfdY1 = 0.0f;
    mfdX2 = 0.0f;
    mfdY2 = 0.0f;

    mfR = 0.0f;
    mfG = 0.0f;
    mfB = 0.0f;

    mfdR = 0.0f;
    mfdG = 0.0f;
    mfdB = 0.0f;

    mbGravityV1 = false;
    mbGravityV2 = false;
    mbVariableGravity = false;
    mbV1AttractsV2 = false;
    mbV2AttractsV1 = false;
    mfV1AttractionForceDivisor = 0.0f;
    mfV2AttractionForceDivisor = 0.0f;

    mbExternalGravity = false;
    mExternalGravityPoint.Set(0, 0);

}

void ZLine::SetBounds(ZRect rBounds)
{
    mrBounds = rBounds;
    int64_t nInflate = RANDU64(0, 600);
    mrBounds.Inflate(nInflate, nInflate);
}

ZLine::~ZLine()
{
}


//#define DOCHECKLIMITS

#ifdef DOCHECKLIMITS
#define CHECKLIMITS CheckLimits();
#else
#define CHECKLIMITS
#endif

void ZLine::CheckLimits()
{
    double fLimit = 50.0;

    assert(mfdY1 > -fLimit && mfdY1 < fLimit);
    assert(mfdX1 > -fLimit && mfdX1 < fLimit);
    assert(mfdY2 > -fLimit && mfdY2 < fLimit);
    assert(mfdX2 > -fLimit && mfdX2 < fLimit);

    assert(mfX1 >= mrBounds.left && mfX1 <= mrBounds.right);
    assert(mfY1 >= mrBounds.top && mfY1 <= mrBounds.bottom);
    assert(mfX2 >= mrBounds.left && mfX2 <= mrBounds.right);
    assert(mfY2 >= mrBounds.top && mfY2 <= mrBounds.bottom);


}

void ZLine::Process()
{
    ZFPoint gravityPoint((((double)(mrBounds.left + mrBounds.right)) / 2.0f), (((double)(mrBounds.top + mrBounds.bottom)) / 2.0f));

    double fV1AttractionForceDivisor = mfV1AttractionForceDivisor;
    double fV2AttractionForceDivisor = mfV2AttractionForceDivisor;

    if (mbExternalGravity)
    {
        gravityPoint = mExternalGravityPoint;

//        fV1AttractionForceDivisor = 50000;
//        fV2AttractionForceDivisor = 50000;

    }

    if (mbVariableGravity)
    {
        // Gravity
        if (mbGravityV1)
        {
            CHECKLIMITS;
            mfdY1 += sin((double)mfX1 / 100.0) * (gravityPoint.y - mfY1) / fV1AttractionForceDivisor;
            mfdX1 += sin((double)mfY1 / 100.0) * (gravityPoint.x - mfX1) / fV1AttractionForceDivisor;
            CHECKLIMITS;
        }

        if (mbGravityV2)
        {
            CHECKLIMITS;
            mfdY2 += sin((double)mfX2 / 100.0) * (gravityPoint.y - mfY2) / fV2AttractionForceDivisor;
            mfdX2 += sin((double)mfY2 / 100.0) * (gravityPoint.x - mfX2) / fV2AttractionForceDivisor;
            CHECKLIMITS;
        }

        if (mbV1AttractsV2)
        {
            CHECKLIMITS;
            mfdX2 -= sin((double)mfY2 / 100.0) * (mfX2 - mfX1) / fV1AttractionForceDivisor;
            mfdY2 -= sin((double)mfX2 / 100.0) * (mfY2 - mfY1) / fV1AttractionForceDivisor;
            CHECKLIMITS;
        }

        if (mbV2AttractsV1)
        {
            CHECKLIMITS;
            mfdX1 -= sin((double)mfY1 / 100.0) * (mfX1 - mfX2) / fV2AttractionForceDivisor;
            mfdY1 -= sin((double)mfX1 / 100.0) * (mfY1 - mfY2) / fV2AttractionForceDivisor;
            CHECKLIMITS;
        }
    }
    else
    {
        // Gravity
        if (mbGravityV1)
        {
            CHECKLIMITS;
            mfdY1 += (gravityPoint.y - mfY1) / fV1AttractionForceDivisor;
            mfdX1 += (gravityPoint.x - mfX1) / fV1AttractionForceDivisor;
            CHECKLIMITS;
        }

        if (mbGravityV2)
        {
            CHECKLIMITS;
            mfdY2 += (gravityPoint.y - mfY2) / fV2AttractionForceDivisor;
            mfdX2 += (gravityPoint.x - mfX2) / fV2AttractionForceDivisor;
            CHECKLIMITS;
        }

        if (mbV1AttractsV2)
        {
            CHECKLIMITS;
            mfdX2 -= (mfX2 - mfX1) / fV1AttractionForceDivisor;
            mfdY2 -= (mfY2 - mfY1) / fV1AttractionForceDivisor;
            CHECKLIMITS;
        }

        if (mbV2AttractsV1)
        {
            CHECKLIMITS;
            mfdX1 -= (mfX1 - mfX2) / fV2AttractionForceDivisor;
            mfdY1 -= (mfY1 - mfY2) / fV2AttractionForceDivisor;
            CHECKLIMITS;
        }
    }
    CHECKLIMITS;

    // X1
    CHECKLIMITS;
    mfX1 += mfdX1;
    if (mfX1 > (double)mrBounds.right)
    {
        mfX1 = (double)mrBounds.right;
        mfdX1 = -mfdX1;
    }
    else if (mfX1 < (double)mrBounds.left)
    {
        mfX1 = (double)mrBounds.left;
        mfdX1 = -mfdX1;
    }
    CHECKLIMITS;

    // Y1
    mfY1 += mfdY1;
    if (mfY1 > (double) mrBounds.bottom)
    {
        mfY1 = (double)mrBounds.bottom;
        mfdY1 = -mfdY1;
    }
    else if (mfY1 < (double)mrBounds.top)
    {
        mfY1 = (double)mrBounds.top;
        mfdY1 = -mfdY1;
    }
    CHECKLIMITS;

    // X2
    mfX2 += mfdX2;
    if (mfX2 > (double)  mrBounds.right)
    {
        mfX2 = (double)mrBounds.right;
        mfdX2 = -mfdX2;
    }
    else if (mfX2 < (double)mrBounds.left)
    {
        mfX2 = (double)mrBounds.left;
        mfdX2 = -mfdX2;
    }
    CHECKLIMITS;

    // Y2
    mfY2 += mfdY2;
    if (mfY2 > (double)  mrBounds.bottom)
    {
        mfY2 = (double)mrBounds.bottom;
        mfdY2 = -mfdY2;
    }
    else if (mfY2 < (double)mrBounds.top)
    {
        mfY2 = (double)mrBounds.top;
        mfdY2 = -mfdY2;
    }
    CHECKLIMITS;

    // R
    mfR += mfdR;
    if (mfR > 255.0f)
    {
        mfR = 255.0f;
        mfdR = -mfdR;
    }
    else if (mfR < 0.0f)
    {
        mfR = 0.0f;
        mfdR = -mfdR;
    }

    // G
    mfG += mfdG;
    if (mfG > 255.0f)
    {
        mfG = 255.0f;
        mfdG = -mfdG;
    }
    else if (mfG < 0.0f)
    {
        mfG = 0.0f;
        mfdG = -mfdG;
    }

    // B
    mfB += mfdB;
    if (mfB > 255.0f)
    {
        mfB = 255.0f;
        mfdB = -mfdB;
    }
    else if (mfB < 0.0f)
    {
        mfB = 0.0f;
        mfdB = -mfdB;
    }
}

void ZLine::RandomizeLine()
{
    int64_t nWidth = mrBounds.Width();
    int64_t nHeight = mrBounds.Height();


    mfX1 = RANDDOUBLE(mrBounds.left, mrBounds.right);
    mfX2 = RANDDOUBLE(mrBounds.left, mrBounds.right);
    mfY1 = RANDDOUBLE(mrBounds.top, mrBounds.bottom);
    mfY2 = RANDDOUBLE(mrBounds.top, mrBounds.bottom);

    double fLineDivisor = 2.0f + (double)RANDU64(0, 40);
    //double fLineDivisor = 1.0f;
    mfdX1 = RANDI64(0, 30) / fLineDivisor;
    mfdY1 = RANDI64(0, 30) / fLineDivisor;
    mfdX2 = RANDI64(0, 30) / fLineDivisor;
    mfdY2 = RANDI64(0, 30) / fLineDivisor;

    CHECKLIMITS;


    mfR = (double)RANDI64(0, 255);
    mfG = (double)RANDI64(0, 255);
    mfB = (double)RANDI64(0, 255);

    //double fColorDivisor = 1.0f + ((double) (rand()%200));
    double fColorDivisor = 1.0f + 200.0 - GetRandExponentialDist(200.0);

    mfdR = (double)RANDI64(0, 15) / fColorDivisor;
    mfdG = (double)RANDI64(0, 15) / fColorDivisor;
    mfdB = (double)RANDI64(0, 15) / fColorDivisor;

    mbExternalGravity = RANDPERCENT(25);
    mbVariableGravity = RANDPERCENT(20);
    mExternalGravityPoint.Set((double) RANDI64(0, mrBounds.Width()), (double) RANDI64(0, mrBounds.Height()));


    mbGravityV1 = RANDPERCENT(50);
    mbGravityV2 = RANDPERCENT(50);
    mbV1AttractsV2 = RANDPERCENT(50);
    mbV2AttractsV1 = RANDPERCENT(50);

    //mbV1AttractsV2 = true;
    //mbV2AttractsV1 = true;

    mfV1AttractionForceDivisor = (double)RANDI64(200, 100000);
    mfV2AttractionForceDivisor = (double)RANDI64(200, 100000);
}


cFloatLinesWin::cFloatLinesWin()
{
    mpFloatBuffer = NULL;
    mbDrawTail = true;
    mnMaxTailLength = 10;
    mnNumDrawn = 0;
    mbXMirror = false;
    mbYMirror = false;
    mbReset = true;
    mnProcessPerFrame = 1;
    mfLineAlpha = 1.0f;

    mbLightDecay = false;
    mnDecayScanLine = 0;
    mfDecayAmount = 1.0f;

    mbAcceptsFocus = true;
    mbAcceptsCursorMessages = true;

    mnMSBetweenResets = RANDU64(15000, 45000);

    mbVariableLineThickness = true;

    Sprintf(msWinName, "floatlineswin_%lld", (int64_t)this);

    //	msWinName = "floatlineswin";
}



bool cFloatLinesWin::RandomizeSettings()
{
    mbDrawTail = RANDBOOL;
    mbLightDecay = RANDBOOL;
    mfDecayAmount = 0.1f + GetRandExponentialDist(20.0);/*((float) (rand()%1000)) / 100.0f ;*/
    //mnMaxTailLength = 100 + rand()%kTailLength;
    mnMaxTailLength = RANDU64(100, 8000);
    mnNumDrawn = 0;
    mbXMirror = RANDPERCENT(25);
    mbYMirror = RANDPERCENT(15);

    mnProcessPerFrame = RANDI64(1, 30);
    //mfLineAlpha = ((double) mnProcessPerFrame) / (1.0f + ((double) (rand()%50)));
    mfLineAlpha = 1.0f + GetRandExponentialDist(55);

    ZRect rBounds(mAreaLocal);
    if (!mbYMirror && !mbXMirror)
    {
        int64_t nWidth = rBounds.Width();
        int64_t nHeight = rBounds.Height();
        rBounds.left -= RANDU64(0, nWidth);
        rBounds.top -= RANDU64(0, nHeight);
        rBounds.right += RANDU64(0, nWidth);
        rBounds.bottom += RANDU64(0, nHeight);
    }

    mHeadLine.SetBounds(rBounds);
    mHeadLine.RandomizeLine();
    mTailLine = mHeadLine;
    //mfLineThickness = 1.0f + (double) (rand()%500);
    mfLineThickness = 1.0f + GetRandExponentialDist(100);

    mbVariableLineThickness = RANDPERCENT(25);

    mbReset = false;
    //mnTimeStampOfReset = ::GetTickCount();
    mnTimeStampOfReset = gTimer.GetMSSinceEpoch();

    return true;
}



bool cFloatLinesWin::Init()
{
    if (mpFloatBuffer)
        delete[] mpFloatBuffer;

    int64_t nWidth = mAreaLocal.Width();
    int64_t nHeight = mAreaLocal.Height();
    mnDecayScanLine = 0;

    int64_t nBufSize = nWidth * nHeight * 3;
    mpFloatBuffer = new double[nBufSize];

    SetFocus();

    return ZWin::Init();
}

bool cFloatLinesWin::Shutdown()
{
    ZWin::Shutdown();
    if (mpFloatBuffer)
    {
        delete[] mpFloatBuffer;
        mpFloatBuffer = NULL;
    }

    return true;
}


void cFloatLinesWin::ClearBuffer(ZBuffer* pBufferToDrawTo)
{

    int64_t nWidth = mAreaLocal.Width();
    int64_t nHeight = mAreaLocal.Height();

    int64_t nBufSize = nWidth * nHeight * 3;
    // Fill with blackness
    for (int64_t i = 0; i < nBufSize; i++)
        mpFloatBuffer[i] = 0.0f;

    pBufferToDrawTo->Fill(0xff000000);
}

bool cFloatLinesWin::OnMouseDownL(int64_t x, int64_t y)
{
    mbReset = true;
    return true;
}

bool cFloatLinesWin::OnChar(char)
{
    mbReset = true;
    return true;
}

bool cFloatLinesWin::OnKeyDown(uint32_t key)
{
#ifdef _WIN64
    if (key == VK_ESCAPE)
    {
        gMessageSystem.Post("{quit_app_confirmed}");
    }
#endif

    return true;
}


bool cFloatLinesWin::Paint()
{
    if (!PrePaintCheck())
        return false;

    ZRect r(mpSurface->GetArea());

//    double dX = 10.0 * (1.0 + cos(gTimer.GetMSSinceEpoch() / 10000.0));
//    double dY = 10.0 * (1.0 + cos(gTimer.GetMSSinceEpoch() / 9000.0));
//    r.Offset(dX, dY);
//    r.Offset(1, 1);
//    mpSurface->Blt(mpSurface.get(), r, mpSurface->GetArea());

/*    size_t bytes = r.Width() * 4;
    for (int64_t y = r.top; y < r.bottom - 1; y++)
    {
        uint8_t* pSrc = (uint8_t*)mpSurface->mpPixels + (y+1) * bytes;
        uint8_t* pDst = (uint8_t*)mpSurface->mpPixels + y * bytes;
        memcpy(pDst, pSrc, bytes);
    }
    memset(mpSurface->mpPixels + (r.bottom-2) * bytes, 0, bytes);
    */


    for (int64_t i = 0; i < mnProcessPerFrame; i++)
    {
        if (mbReset)
        {
            ClearBuffer(mpSurface.get());
            RandomizeSettings();
        }

        ComputeFloatLines();

        ZFloatVertex v1(mHeadLine.mfX1, mHeadLine.mfY1, mfLineAlpha, mHeadLine.mfR, mHeadLine.mfG, mHeadLine.mfB);
        ZFloatVertex v2(mHeadLine.mfX2, mHeadLine.mfY2, mfLineAlpha, mHeadLine.mfR, mHeadLine.mfG, mHeadLine.mfB);


        /* start test code*/
/*		mfLineThickness = 5.0f;
        ZFloatVertex v1(100, 100, 0.6f, 255, 255, 0);
        ZFloatVertex v2(200, 200, 0.2f, 255, 0, 255);
        mbXMirror = false;
        mbYMirror = false;
        mbVariableLineThickness = false;
        /* end test code*/


        DrawAlphaLine(mpSurface.get(), v2, v1, &mAreaLocal);

        if (mbXMirror)
        {
            ZFloatVertex vx1(v1);
            ZFloatVertex vx2(v2);

            vx1.mx = mAreaLocal.right - vx1.mx;
            vx2.mx = mAreaLocal.right - vx2.mx;

            DrawAlphaLine(mpSurface.get(), vx1, vx2, &mAreaLocal);
        }

        // Now Y mirror
        if (mbYMirror)
        {
            ZFloatVertex vy1(v1);
            ZFloatVertex vy2(v2);

            vy1.my = mAreaLocal.bottom - vy1.my;
            vy2.my = mAreaLocal.bottom - vy2.my;

            DrawAlphaLine(mpSurface.get(), vy1, vy2, &mAreaLocal);


            if (mbXMirror)
            {
                ZFloatVertex vxy1(vy1);
                ZFloatVertex vxy2(vy2);

                vxy1.mx = mAreaLocal.right - vxy1.mx;
                vxy2.mx = mAreaLocal.right - vxy2.mx;

                DrawAlphaLine(mpSurface.get(), vxy1, vxy2, &mAreaLocal);
            }
        }


        if (mbDrawTail && mnNumDrawn > mnMaxTailLength)
        {
            // Subtract
            ZFloatVertex vs1(mTailLine.mfX1, mTailLine.mfY1, -mfLineAlpha, mTailLine.mfR, mTailLine.mfG, mTailLine.mfB);
            ZFloatVertex vs2(mTailLine.mfX2, mTailLine.mfY2, -mfLineAlpha, mTailLine.mfR, mTailLine.mfG, mTailLine.mfB);

            DrawAlphaLine(mpSurface.get(), vs2, vs1, &mAreaLocal);

            if (mbXMirror)
            {
                ZFloatVertex vsx1(vs1);
                ZFloatVertex vsx2(vs2);

                vsx1.mx = mAreaLocal.right - vsx1.mx;
                vsx2.mx = mAreaLocal.right - vsx2.mx;

                DrawAlphaLine(mpSurface.get(), vsx1, vsx2, &mAreaLocal);
            }

            // Now Y mirror
            if (mbYMirror)
            {
                ZFloatVertex vsy1(vs1);
                ZFloatVertex vsy2(vs2);

                vsy1.my = mAreaLocal.bottom - vsy1.my;
                vsy2.my = mAreaLocal.bottom - vsy2.my;

                DrawAlphaLine(mpSurface.get(), vsy1, vsy2, &mAreaLocal);


                if (mbXMirror)
                {
                    ZFloatVertex vsxy1(vsy1);
                    ZFloatVertex vsxy2(vsy2);

                    vsxy1.mx = mAreaLocal.right - vsxy1.mx;
                    vsxy2.mx = mAreaLocal.right - vsxy2.mx;

                    DrawAlphaLine(mpSurface.get(), vsxy1, vsxy2, &mAreaLocal);
                }
            }


            /*      //pBufferToDrawTo->SetPixel(mTailLine.mfX2, mTailLine.mfY2, 0);
            DrawAlphaLine(vs1, vs2, &mAreaToDrawTo);

            if (mbXMirror)
            {
            vs1.mx = mAreaToDrawTo.right - vs1.mx;
            vs2.mx = mAreaToDrawTo.right - vs2.mx;

            DrawAlphaLine(vs1, vs2, &mAreaToDrawTo);
            }*/

        }


        if (mbLightDecay)
        {
            ZRect rArea = mAreaLocal;
            for (int64_t i = 0; i < 20; i++)
            {
                double* pDest = mpFloatBuffer + (mnDecayScanLine)*mAreaInParent.Width() * 3;
                FillInSpan(mpSurface.get(), pDest, rArea.Width(), 255, 255, 255, -mfDecayAmount, rArea.left, mnDecayScanLine);

                mnDecayScanLine++;
                if (mnDecayScanLine > mAreaLocal.bottom - 1)
                    mnDecayScanLine = 0;
            }
        }
    }

    mAnimator.Paint();

    ZWin::Paint();
    mbInvalid = true;
    return true;
}


void cFloatLinesWin::ComputeFloatLines()
{
    mHeadLine.Process();
    mnNumDrawn++;

    if (mnNumDrawn > mnMaxTailLength)
    {
        mTailLine.Process();
    }

    //	uint32_t nCurTime = ::GetTickCount();
    uint64_t nCurTime = gTimer.GetMSSinceEpoch();

    if (nCurTime - mnTimeStampOfReset > mnMSBetweenResets)
    {
        mbReset = true;
    }
}

inline
bool cFloatLinesWin::FloatScanLineIntersection(double fScanLine, const ZFloatVertex& v1, const ZFloatVertex& v2, double& fIntersection, double& fR, double& fG, double& fB, double& fA)
{
    if (v1.my == v2.my || v1.mx == v2.mx)
    {
        fIntersection = v1.mx;
        fA = v1.mfA;
        fR = v1.mfR;
        fG = v1.mfG;
        fB = v1.mfB;
        return false;
    }

    // Calculate line intersection
    double fM = (double)((v1.my - v2.my) / (v1.mx - v2.mx)); // slope
    fIntersection = (v2.mx + (fScanLine - v2.my) / fM);

    double fT = (fScanLine - v1.my) / (v2.my - v1.my);  // how far along line (0.0 - 1.0)

    fA = (double)(v1.mfA + (v2.mfA - v1.mfA) * fT);
    fR = (double)(v1.mfR + (v2.mfR - v1.mfR) * fT);
    fG = (double)(v1.mfG + (v2.mfG - v1.mfG) * fT);
    fB = (double)(v1.mfB + (v2.mfB - v1.mfB) * fT);

    return true;
}

inline
void cFloatLinesWin::FillInSpan(ZBuffer* pBufferToDrawTo, double* pDest, int64_t nNumPixels, double fR, double fG, double fB, double fA, int64_t nX, int64_t nY)
{
    //	ZDEBUG_OUT("NumPixels:%d\n", nNumPixels);
    while (nNumPixels > 0)
    {
        double fCurR = *pDest;
        double fCurG = *(pDest + 1);
        double fCurB = *(pDest + 2);

        double fDestR = fCurR + fR * fA / 255.0f;
        double fDestG = fCurG + fG * fA / 255.0f;
        double fDestB = fCurB + fB * fA / 255.0f;

        if (fDestR < 0.0f)
            fDestR = 0.0f;
        //      else if (fDestR > 255.0f)
        //         fDestR = 255.0f;
        if (fDestG < 0.0f)
            fDestG = 0.0f;
        //      else if (fDestG > 255.0f)
        //         fDestG = 255.0f;
        if (fDestB < 0.0f)
            fDestB = 0.0f;
        //      else if (fDestB > 255.0f)
        //         fDestB = 255.0f;

        /*		float fDestR = (fR * fA + (fCurR * (255.0f-fA)))/255.0f;
        float fDestG = (fG * fA + (fCurG * (255.0f-fA)))/255.0f;
        float fDestB = (fB * fA + (fCurB * (255.0f-fA)))/255.0f;*/

        *pDest = fDestR;
        *(pDest + 1) = fDestG;
        *(pDest + 2) = fDestB;

        // Copy the pixels to the blittable buffer
        int64_t nR = (int64_t)fDestR;
        int64_t nG = (int64_t)fDestG;
        int64_t nB = (int64_t)fDestB;

        if (nR > 255)
            nR = 255;
        else if (nR < 0)
            nR = 0;
        if (nG > 255)
            nG = 255;
        else if (nG < 0)
            nG = 0;
        if (nB > 255)
            nB = 255;
        else if (nB < 0)
            nB = 0;

        uint32_t nCol = ((uint8_t)nR) << 16 | ((uint8_t)nG) << 8 | ((uint8_t)nB);

        if (nCol > 0)
        {
            nCol |= 0xff000000;
            int stophere = 5;
        }

        pBufferToDrawTo->SetPixel(nX, nY, nCol);

        nX++;
        pDest += 3;
        nNumPixels--;
    }
}

void cFloatLinesWin::DrawAlphaLine(ZBuffer* pBufferToDrawTo, ZFloatVertex& v1, ZFloatVertex& v2, ZRect* pClip)
{
    if (v1.my > v2.my)
    {
        // Swap them
        ZFloatVertex temp = v2;
        v2 = v1;
        v1 = temp;
    }


    ZRect rDest(*pClip);

    ZRect rLineRect;

    rLineRect.top = (int64_t)min(v1.my, v2.my);
    rLineRect.bottom = (int64_t)max(v1.my, v2.my);

    if (mbVariableLineThickness)
    {
        //double fLineWidth = (double)(rLineRect.Height()) / (4.0f * sin(mHeadLine.mfdX1));
        double fLineWidth = 1.0f + sin(mHeadLine.mfdX1) * 40.0f * sin(mHeadLine.mfdY1);
        rLineRect.left = (int64_t)min(v1.mx - fLineWidth, v2.mx - fLineWidth);
        rLineRect.right = (int64_t)max(v1.mx + fLineWidth, v2.mx + fLineWidth);
    }
    else
    {
        rLineRect.left = (int64_t)min(v1.mx - mfLineThickness / 2.0f, v2.mx - mfLineThickness / 2.0f);
        rLineRect.right = (int64_t)max(v1.mx + mfLineThickness / 2.0f, v2.mx + mfLineThickness / 2.0f);
    }

    rLineRect.Intersect(/*&rLineRect, */&rDest);

    double* pSurface = mpFloatBuffer;
    int64_t nStride = mAreaInParent.Width() * 3;

    double fScanLine = (double)rLineRect.top;
    double fIntersection;
    double fR;
    double fG;
    double fB;
    double fA;

    double fPrevScanLineIntersection;
    if (!FloatScanLineIntersection(fScanLine, v1, v2, fPrevScanLineIntersection, fR, fG, fB, fA) && v1.my == v2.my)
    {
        // Horizontal line is a special case:
        for (int64_t nScanLine = rLineRect.top; nScanLine < rLineRect.bottom; nScanLine++)
        {
            double* pDest = pSurface + (nScanLine)*nStride + (rLineRect.left) * 3;
            FillInSpan(pBufferToDrawTo, pDest, rLineRect.Width(), fR, fG, fB, fA, rLineRect.left, nScanLine);
        }
    }

    for (int64_t nScanLine = rLineRect.top; nScanLine < rLineRect.bottom; nScanLine++)
    {
        FloatScanLineIntersection(fScanLine, v1, v2, fIntersection, fR, fG, fB, fA);

        int64_t nStartPixel = (int64_t)min(fIntersection - mfLineThickness / 2.0f, fPrevScanLineIntersection - mfLineThickness / 2.0f);
        nStartPixel = max(nStartPixel, rLineRect.left);		// clip left
        nStartPixel = min(nStartPixel, rLineRect.right);

        int64_t nEndPixel = (int64_t)max(fIntersection + mfLineThickness / 2.0f, fPrevScanLineIntersection + mfLineThickness / 2.0f);
        nEndPixel = min(nEndPixel, rLineRect.right);
        nEndPixel = max(nEndPixel, rLineRect.left);

        //      int64_t nStartPixel = 20;
        //      int64_t nEndPixel = 40;

        double* pDest = pSurface + nScanLine * nStride + (nStartPixel) * 3;

        int64_t nLineWidth = nEndPixel - nStartPixel;
        FillInSpan(pBufferToDrawTo, pDest, nLineWidth, fR, fG, fB, fA, nStartPixel, nScanLine);

        fPrevScanLineIntersection = fIntersection;
        fScanLine += 1.0f;
    }
}

