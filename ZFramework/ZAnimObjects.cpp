#include "ZAnimObjects.h"
#include "ZBuffer.h"
#include "ZFont.h"
#include "ZTimer.h"
#include "ZDebug.h"
#include "ZGraphicSystem.h"
#include "ZScreenBuffer.h"
#include "ZMessageSystem.h"
#include "ZTransformable.h"
#include "math.h"
#include "ZStringHelpers.h"
#include "ZGUIStyle.h"
#include "ZRandom.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;

ZAnimObject::ZAnimObject()
{
	mState = kNone;
    mpDestination = nullptr;
	mnTimeStamp = gTimer.GetElapsedTime();
}

ZAnimObject::~ZAnimObject()
{
}

tRectList ZAnimObject::ComputeDirtyRects(const ZRect& rOldArea, const ZRect& rNewArea)
{

    tRectList dirtyList;

    ZRect rOverlap(rNewArea);
    rOverlap.IntersectRect(rOldArea);

    ZRect rDest(rOldArea);



    //                              rDest
//     lw      mw      rw     /
//  ---^-------^-------^---- /
//         |       |        |             
//    TL   |   T   |   TR   > th         
//         |       |        |              
//  -------+-------+--------|             
//         |       |        |              
//     L   |Overlap|   R    > mh             
//         |       |        |              
//  -------+-------+--------|             
//         |       |        |              
//    BL   |   B   |   BR   > bh 
//         |       |        |          






    int64_t lw = rOverlap.left - rDest.left;
    int64_t mw = rOverlap.Width();
    int64_t rw = rDest.right - rOverlap.right;

    int64_t th = rOverlap.top - rDest.top;
    int64_t mh = rOverlap.Height();
    int64_t bh = rDest.bottom - rOverlap.bottom;


    if (lw != 0)		// Consider left three?
    {
        if (th != 0)
        {
            // TL
            dirtyList.emplace_back(ZRect(rDest.left, rDest.top, rDest.left + lw, rDest.top + th));
        }

        if (mh != 0)
        {
            // L
            dirtyList.emplace_back(ZRect(rDest.left, rOverlap.top, rDest.left + lw, rOverlap.bottom));
        }

        if (bh != 0)
        {
            // BL
            dirtyList.emplace_back(ZRect(rDest.left, rOverlap.bottom, rOverlap.left, rDest.bottom));
        }
    }

    if (mw != 0)			// consider top and bottom two?
    {
        if (th != 0)
        {
            // T
            dirtyList.emplace_back(ZRect(rOverlap.left, rDest.top, rOverlap.right, rDest.top + th));
        }

        if (bh != 0)
        {
            // B
            dirtyList.emplace_back(ZRect(rOverlap.left, rOverlap.bottom, rOverlap.right, rDest.bottom));
        }
    }

    if (rw != 0)		// consider right three?
    {
        if (th != 0)
        {
            // TR
            dirtyList.emplace_back(ZRect(rOverlap.right, rDest.top, rDest.right, rOverlap.top));
        }

        if (mh != 0)
        {
            // R
            dirtyList.emplace_back(ZRect(rOverlap.right, rOverlap.top, rDest.right, rOverlap.bottom));
        }

        if (bh != 0)
        {
            // BR
            dirtyList.emplace_back(ZRect(rOverlap.right, rOverlap.bottom, rDest.right, rDest.bottom));
        }
    }

    return dirtyList;
}


ZAnimObject_TextMover::ZAnimObject_TextMover(ZGUI::Style _style) : ZAnimObject()
{
	mfX  = 0;
	mfY  = 0;
	mfDX = 0;
	mfDY = 0;
	mnStartAlpha = 0;
	mnEndAlpha = 0;
	mnRemainingAlphaFadeTime = 0;
	mbAlphaFading = false;
    mStyle = _style;
}


bool ZAnimObject_TextMover::Paint()
{
	uint64_t nCurrentTime = gTimer.GetElapsedTime();
	int64_t nDeltaTime = nCurrentTime - mnTimeStamp;
	mnTimeStamp = nCurrentTime;

	double fXPixelsToMoveThisFrame = (mfDX * (double) nDeltaTime) / 1000.0f;
	double fYPixelsToMoveThisFrame = (mfDY * (double) nDeltaTime) / 1000.0f;

	mfX += fXPixelsToMoveThisFrame;
	mfY += fYPixelsToMoveThisFrame;

	int64_t nX = (int64_t) mfX;
	int64_t nY = (int64_t) mfY;

	int64_t nWidth = mrArea.Width();
	int64_t nHeight = mrArea.Height();

	mrArea.SetRect(nX, nY, nX + nWidth, nY + nHeight);

	ZRect rBufferArea(mpDestination->GetArea());

	if (!rBufferArea.Overlaps(&mrArea))    // Have we moved off screen?
	{
		mState = kFinished;
		return true;
	}
	if (mbAlphaFading)
	{
		// Alpha Draw
		mnRemainingAlphaFadeTime -= nDeltaTime;
		if (mnRemainingAlphaFadeTime <= 0)
		{
			mState = kFinished;
			return true;
		}

		ZASSERT(mnTotalFadeTime > 0);
		double fT = (double) (mnTotalFadeTime - mnRemainingAlphaFadeTime) / (double) mnTotalFadeTime;
		int64_t nAlpha = (int64_t) (mnStartAlpha + (mnEndAlpha - mnStartAlpha) * fT);

        ZGUI::Style useStyle(mStyle);

		useStyle.look.colTop = ARGB((uint8_t) nAlpha, ARGB_R(useStyle.look.colTop), ARGB_G(useStyle.look.colTop), ARGB_B(useStyle.look.colTop));
        useStyle.look.colBottom = ARGB((uint8_t) nAlpha, ARGB_R(useStyle.look.colBottom), ARGB_G(useStyle.look.colBottom), ARGB_B(useStyle.look.colBottom));
		useStyle.Font()->DrawText(mpDestination.get(), msText, mrArea, &useStyle.look);

		return true;
	}

	// Plain Draw
    mStyle.Font()->DrawText(mpDestination.get(), msText, mrArea, &mStyle.look);

	return true;
}

// cCEAnimObject_TextMover
void ZAnimObject_TextMover::SetText(const string& sText)
{
	msText = sText;

	int64_t nX = (int64_t) mfX;
	int64_t nY = (int64_t) mfY;

	// Now calculate (or recalculate) our area
	mrArea.SetRect(nX, nY, nX + mStyle.Font()->StringWidth(msText), nY + mStyle.Font()->Height());
}

void ZAnimObject_TextMover::SetLocation(int64_t nX, int64_t nY)
{
	mfX = (double) nX;
	mfY = (double) nY;

	mrArea.SetRect(nX, nY, nX + mStyle.Font()->StringWidth(msText), nY + mStyle.Font()->Height());
}

void ZAnimObject_TextMover::SetPixelsPerSecond(double fDX, double fDY)
{
	mfDX = fDX;
	mfDY = fDY;
}

void ZAnimObject_TextMover::SetAlphaFade(int64_t nStartAlpha, int64_t nEndAlpha, int64_t nMilliseconds)
{
	ZASSERT(nMilliseconds > 0);

	mnStartAlpha = nStartAlpha;
	mnEndAlpha = nEndAlpha;
	mnTotalFadeTime = nMilliseconds;
	mnRemainingAlphaFadeTime = nMilliseconds;
	mbAlphaFading = true;
}



const int64_t kDefaultParticles = 100;
const int64_t kDefaultStreakSize = 50;
const double  kDefaultBrightness = 1.0f;
ZCEAnimObject_Sparkler::ZCEAnimObject_Sparkler() : ZAnimObject()
{
	mnParticles = kDefaultParticles;
	mnMaxStreakSize = kDefaultStreakSize;
	mfMaxBrightness = kDefaultBrightness;
	mpSourceBuffer = NULL;
	mnSourceColor = 0xffffffff;
}

bool ZCEAnimObject_Sparkler::Paint()
{
	ProcessSparkles();

	uint32_t* pDest = mpDestination->GetPixels();
	int64_t  nStride = mpDestination->GetArea().Width();
    ZRect* pClip = &mpDestination->GetArea();
	for (tSparkleList::iterator it = mSparkleList.begin(); it != mSparkleList.end(); it++)
	{
		sSparkle& sparkle = *it;
        double fBrightness = sparkle.fBrightness;

		for (tPointList::iterator pointIt = sparkle.streakPoints.begin(); pointIt != sparkle.streakPoints.end(); pointIt++)
		{
			ZFPoint& point = *pointIt;

			int64_t nX = (int64_t) point.x;
			int64_t nY = (int64_t) point.y;
			if (pClip->PtInRect(nX, nY))
			{
				uint32_t* pBits = pDest + nY * nStride + nX;
				uint8_t nB1 = (uint8_t) ((fBrightness) * 255.0f);
				*pBits = COL::AlphaBlend_AddAlpha(0x88882222, *pBits, nB1);
			}

			if (pClip->PtInRect(nX+1, nY))
			{
				uint32_t* pBits = pDest + nY * nStride + nX+1;
				uint8_t nB1 = (uint8_t) ((fBrightness) * 255.0f);
				*pBits = COL::AlphaBlend_AddAlpha(0x88882222, *pBits, nB1);
			}

			if (pClip->PtInRect(nX-1, nY))
			{
				uint32_t* pBits = pDest + nY * nStride + nX-1;
				uint8_t nB1 = (uint8_t) ((fBrightness) * 255.0f);
				*pBits = COL::AlphaBlend_AddAlpha(0x88882222, *pBits, nB1);
			}

			if (pClip->PtInRect(nX, nY-1))
			{
				uint32_t* pBits = pDest + ((nY-1) * nStride) + nX;
				uint8_t nB1 = (uint8_t) ((fBrightness) * 255.0f);
				*pBits = COL::AlphaBlend_AddAlpha(0x88882222, *pBits, nB1);
			}

			if (pClip->PtInRect(nX, nY+1))
			{
				uint32_t* pBits = pDest + ((nY+1) * nStride) + nX;
				uint8_t nB1 = (uint8_t) ((fBrightness) * 255.0f);
				*pBits = COL::AlphaBlend_AddAlpha(0x8800FFFF, *pBits, nB1);
			}

			fBrightness *= 0.998f;
		}

	}

	return true;
}

// ZCEAnimObject_Sparkler 
void ZCEAnimObject_Sparkler::SetSource(tZBufferPtr pSource, const ZRect& rBounds, uint32_t nSourceColor)
{
	mrBounds = rBounds;
	mnSourceColor = nSourceColor;
	mpSourceBuffer = pSource;
	ZASSERT(pSource);
	mState = kAnimating;
}

void ZCEAnimObject_Sparkler::CreateSparkle()
{
	sSparkle sparkle;
	sparkle.fBrightness = mfMaxBrightness;
	sparkle.fDX = RANDDOUBLE(-1.0,1.0);//((float) ((rand()%1000) - 500)) / 500.0f; 
	sparkle.fDY = RANDDOUBLE(-1.0,1.0);//((float) ((rand()%1000) - 500)) / 500.0f; 

	sparkle.nLife = RANDU64(0,50);//rand()%50;

	ZFPoint newSparklePoint(-1.0f, -1.0f);

	const int64_t kMaxSparkleSearchIterations = 500;
	for (int64_t i = 0; i < kMaxSparkleSearchIterations; i++)
	{
		int64_t nRandX = RANDI64(mrBounds.left, mrBounds.Width());
		int64_t nRandY = RANDI64(mrBounds.top, mrBounds.Height());
		uint32_t nSourceCol = mpSourceBuffer->GetPixel(nRandX, nRandY);
		if ((nSourceCol & 0x00ffffff) == (mnSourceColor & 0x00ffffff))
		{
			newSparklePoint.x = (double) nRandX;
			newSparklePoint.y = (double) nRandY;
			break;
		}
	}

	if (newSparklePoint.x < 0)
	{
		ZDEBUG_OUT("Couldn't find a sparkle point.");
		return;
	}


	// Give the new point a random head start
	/*   int64_t nRand = rand()%5;
	for (int64_t j = 0; j < nRand; j++)
	newSparklePoint.Offset(sparkle.fDX*3, sparkle.fDY*3);*/


	for (int64_t i = 0; i < mnMaxStreakSize; i++)
	{
		newSparklePoint.Offset(sparkle.fDX, sparkle.fDY);
		sparkle.streakPoints.push_front(newSparklePoint);
	}

	mSparkleList.push_back(sparkle);
}

void ZCEAnimObject_Sparkler::ProcessSparkles()
{
//	for (int64_t i = 0; i < 10; i++)
	{
		if (mSparkleList.size() < (uint32_t) mnParticles /*&& rand()%100 > 20*/)
			CreateSparkle();
	}

	// For every sparkle....  
	tSparkleList::iterator it;
	for (it = mSparkleList.begin(); it != mSparkleList.end(); it++)
	{
		sSparkle& sparkle = *it;

		// adjust the sparkle brightness
		sparkle.fBrightness *= 0.95f;

		// Remove the tail point if we've reached max streak size
		sparkle.streakPoints.pop_back();

		// Add a new head point
		ZFPoint newSparklePoint;
		tPointList::iterator pointIt = sparkle.streakPoints.begin();
		if (pointIt != sparkle.streakPoints.end())
		{
			newSparklePoint = *pointIt;
		}
		else
		{
			//newSparklePoint = mSource;
			continue;
		}

		newSparklePoint.Offset(sparkle.fDX, sparkle.fDY);
		sparkle.fDY += .1f;     // a little bit 'o gravity

		sparkle.streakPoints.push_front(newSparklePoint);
	}

	// Remove any sparkles that have gotten too dim
	for (it = mSparkleList.begin(); it != mSparkleList.end();)
	{
		sSparkle& sparkle = *it;
		sparkle.nLife--;

		if (sparkle.nLife < 0 || sparkle.fBrightness < 0.09f)
		{
			tSparkleList::iterator nextIt = it;;
			nextIt++;
			mSparkleList.erase(it);

			it = nextIt;
			continue;
		}
		it++;
	}
}





// cCEAnimObject
ZAnimObject_TextPulser::ZAnimObject_TextPulser(ZGUI::Style _style) : ZAnimObject()
{
	mnMaxAlpha = 255;
	mnMinAlpha = 0;
	mfPeriod = 10.0;
    mStyle = _style;
}

bool ZAnimObject_TextPulser::Paint()
{
	ZRect rBufferArea(mpDestination->GetArea());

	double pulse = 0.5 * (1.0 + (double) sin(((double) gTimer.GetElapsedTime()) / mfPeriod));
	int64_t nAlpha = (int64_t) ((double) mnMinAlpha + ((double)mnMaxAlpha - (double)mnMinAlpha) * pulse );


    ZGUI::Style useStyle(mStyle);

	useStyle.look.colTop = ARGB((uint8_t) nAlpha, ARGB_R(useStyle.look.colTop), ARGB_G(useStyle.look.colTop), ARGB_B(useStyle.look.colTop));
    useStyle.look.colBottom = ARGB((uint8_t)nAlpha, ARGB_R(useStyle.look.colBottom), ARGB_G(useStyle.look.colBottom), ARGB_B(useStyle.look.colBottom));
    useStyle.pos = ZGUI::C;


    mStyle.Font()->DrawTextParagraph(mpDestination.get(), msText, mrArea, &useStyle);
    mrLastDrawArea = mrArea;
	return true;

}

// cCEAnimObject_TextPulser
void ZAnimObject_TextPulser::SetText(const string& sText)
{
	msText = sText;

	// Now calculate (or recalculate) our area
	mrArea.bottom = mrArea.top + mStyle.Font()->Height();
	mrArea.left   = mrArea.left + mStyle.Font()->StringWidth(msText);
}

void ZAnimObject_TextPulser::SetPulse(int64_t nMinAlpha, int64_t nMaxAlpha, double fPeriod)
{ 
	mnMinAlpha = nMinAlpha;
	mnMaxAlpha = nMaxAlpha;
	mfPeriod = fPeriod;

	ZASSERT(mfPeriod > 0.0f);
}


ZAnimObject_BitmapShatterer::ZAnimObject_BitmapShatterer() : ZAnimObject()
{
	mpTexture = NULL;
}

ZAnimObject_BitmapShatterer::~ZAnimObject_BitmapShatterer()
{
}

void ZAnimObject_BitmapShatterer::SetBitmapToShatter(ZBuffer* pBufferToShatter, ZRect& rSrc, ZRect& rStartingDst, int64_t nSubdivisions)
{
	mrArea = rStartingDst;
	CreateShatterListFromBuffer(pBufferToShatter, rSrc, nSubdivisions);

	ZRect rDst(0,0,rSrc.Width(),rSrc.Height());
	mpTexture.reset(new ZBuffer());
	mpTexture->Init(rSrc.Width(), rSrc.Height());

	// Instead of BLT we're doing a direct pixel translation to get all the pixel values (ignore alpha)
	for (int64_t y = 0; y < rSrc.Height(); y++)
	{
		for (int64_t x = 0; x < rSrc.Width(); x++)
		{
			mpTexture->SetPixel(x, y, pBufferToShatter->GetPixel(x + rSrc.left, y + rSrc.top));
		}
	}

	mState = kAnimating;
}

bool ZAnimObject_BitmapShatterer::Paint()
{
	for (tShatterQuadList::iterator it = mShatterQuadList.begin(); it != mShatterQuadList.end(); it++)
	{
		cShatterQuad& quad = *it;
		cShatterQuad transformedQuad = quad;

		double fCenterX = (quad.mVertices[0].x + quad.mVertices[1].x)/2.0f;
		double fCenterY = (quad.mVertices[0].y + quad.mVertices[2].y)/2.0f;

		for (int64_t i = 0; i < 4; i++)
		{
			const double x(quad.mVertices[i].x - fCenterX);
			const double y(quad.mVertices[i].y - fCenterY);
			const double cosAngle(::cos(quad.fRotation));
			const double sinAngle(::sin(quad.fRotation));

			transformedQuad.mVertices[i].x = fCenterX + transformedQuad.fScale*(x*cosAngle - y*sinAngle);
			transformedQuad.mVertices[i].y = fCenterY + transformedQuad.fScale*(x*sinAngle + y*cosAngle);
		}

        ZRect* pClip = &mpDestination->GetArea();
		mRasterizer.RasterizeWithAlpha(mpDestination.get(), mpTexture.get(), transformedQuad.mVertices, pClip, 128);
	}

	Process(mpDestination->GetArea());

	return true;
}

void ZAnimObject_BitmapShatterer::Process(ZRect& rBoundingArea)
{
	uint64_t nCurrentTime = gTimer.GetElapsedTime();
	int64_t nDeltaTime = nCurrentTime - mnTimeStamp;
	mnTimeStamp = nCurrentTime;

	double fTimeScale = ((double) nDeltaTime / 10.0);
	//ZDEBUG_OUT("timescale:%f\n", fTimeScale);

//	ZRect rArea = mpBufferToDrawTo->GetArea();

	for (tShatterQuadList::iterator it = mShatterQuadList.begin(); it != mShatterQuadList.end();)
	{
		cShatterQuad& quad = *it;
		tShatterQuadList::iterator nextIt = it;
		nextIt++;

		ZRect rBounds = mRasterizer.GetBoundingRect(quad.mVertices);

		if (rBoundingArea.Overlaps(&rBounds) || quad.fScale < 0.01f)
		{
			if (quad.fdX == 0.0f && quad.fdY == 0.0f && quad.fdRotation == 0.0f && quad.fdScale == 0.0f)
			{
				// % chance to start this quad moving
//				if (rand() % 1000 < 100)
                if (RANDPERCENT(10))
				{
					quad.fdX = RANDDOUBLE(-1.0,1.0);//(double) ((double) (rand()%500 - 250)/250.0f);
					quad.fdY = RANDDOUBLE(-1.0, 1.0);// (double) ((double) (rand()%500 - 400)/250.0f);
					quad.fdRotation = RANDDOUBLE(-0.1,0.1);//(double) ((double) (rand()%500 - 250)/2450.0f);
					quad.fdScale = 0.998;//(double) -(rand()%500)/1000.0f;
				}
			}
			else
			{
				quad.fRotation += quad.fdRotation * fTimeScale;
				quad.fdY += 0.02f * fTimeScale;	// gravity

				quad.fScale *= quad.fdScale ;
			}

			for (int64_t i = 0; i < 4; i++)
			{
				quad.mVertices[i].x += quad.fdX * fTimeScale;
				quad.mVertices[i].y += quad.fdY * fTimeScale;
			}
		}
		else
		{
			mShatterQuadList.erase(it);
		}


		it = nextIt;
	}

	// No more pixels?
	if (mShatterQuadList.empty())
	{
		mState = kFinished;
	}
}

void ZAnimObject_BitmapShatterer::CreateShatterListFromBuffer(ZBuffer* pBuffer, ZRect& /*rSrc*/, int64_t nSubdivisions)
{
	ZASSERT(pBuffer);

	// Clear all pixels
	mShatterQuadList.clear();

	double fQuadWidth = (double) mrArea.Width() / (double) nSubdivisions;
	double fQuadHeight = (double) mrArea.Height() / (double) nSubdivisions;

	double fUQuad = 1.0f / nSubdivisions;

	for (int64_t y = 0; y < nSubdivisions; y++)
	{
		double fVTop = y * fUQuad;
		double fVBottom = (y+1) * fUQuad;

		for (int64_t x = 0; x < nSubdivisions; x++)
		{
			double fULeft = x * fUQuad;
			double fURight = (x+1) * fUQuad;

			cShatterQuad newPixel;
			newPixel.mVertices[0].x = mrArea.left + x * fQuadWidth;
			newPixel.mVertices[0].y = mrArea.top + y * fQuadHeight;
			newPixel.mVertices[0].u = fULeft;
			newPixel.mVertices[0].v = fVTop;

			newPixel.mVertices[1].x = mrArea.left + (x+1) * fQuadWidth;
			newPixel.mVertices[1].y = mrArea.top + y * fQuadHeight;
			newPixel.mVertices[1].u = fURight;
			newPixel.mVertices[1].v = fVTop;

			newPixel.mVertices[2].x = mrArea.left + (x+1) * fQuadWidth;
			newPixel.mVertices[2].y = mrArea.top + (y+1) * fQuadHeight;
			newPixel.mVertices[2].u = fURight;
			newPixel.mVertices[2].v = fVBottom;

			newPixel.mVertices[3].x = mrArea.left + x * fQuadWidth;
			newPixel.mVertices[3].y = mrArea.top + (y+1) * fQuadHeight;
			newPixel.mVertices[3].u = fULeft;
			newPixel.mVertices[3].v = fVBottom;

			mShatterQuadList.push_back(newPixel);
		}
	}
}


/*ZAnimObject_BouncyLine::ZAnimObject_BouncyLine() : ZAnimObject()
{
	mnMaxTailLength = 0;
	mrExtents.SetRect(0,0,0,0);
	mbXMirror = false;
	mbYMirror = false;
	mbAAttractsB = false;
	mbBAttractsA = false;
}

ZAnimObject_BouncyLine::~ZAnimObject_BouncyLine()
{
}

void ZAnimObject_BouncyLine::RandomizeLine()
{
	mfX1 = RANDDOUBLE(mrExtents.left, mrExtents.Width());//(double) (mrExtents.left + rand()%(int64_t)mrExtents.Width());
	mfX2 = RANDDOUBLE(mrExtents.left, mrExtents.Width());//(double) (mrExtents.left + rand()% (int64_t)mrExtents.Width());
	mfY1 = RANDDOUBLE(mrExtents.top, mrExtents.Height()); //(double) (mrExtents.top  + rand()% (int64_t)mrExtents.Height());
	mfY2 = RANDDOUBLE(mrExtents.top, mrExtents.Height()); //(double) (mrExtents.top  + rand()% (int64_t)mrExtents.Height());

	mfdX1 = RANDDOUBLE(-15.0, 15.0); //(double) (rand()% (int64_t)mrExtents.Width()/30.0f - 15.0f);
	mfdY1 = RANDDOUBLE(-15.0, 15.0); //(double) (rand()% (int64_t)mrExtents.Height()/30.0f - 15.0f);
	mfdX2 = RANDDOUBLE(-15.0, 15.0); //(double) (rand()% (int64_t)mrExtents.Width()/30.0f - 15.0f);
	mfdY2 = RANDDOUBLE(-15.0, 15.0); //(double) (rand()% (int64_t)mrExtents.Height()/30.0f - 15.0f);

	mnR = RANDU64(0,255);//rand()%255;
	mnG = RANDU64(0, 255);//rand()%255;
	mnB = RANDU64(0, 255);//rand()%255;

	mndR = RANDU64(0, 15);//rand()%15;
	mndG = RANDU64(0, 15);//rand()%15;
	mndB = RANDU64(0, 15);//rand()%15;
}


bool ZAnimObject_BouncyLine::Paint(ZBuffer* pBufferToDrawTo, ZRect* pClip)
{
	Process();

	// Grab the member variables for our own iteration
	// Endpoints
	double fCurX1 = mfX1;
	double fCurY1 = mfY1;
	double fCurX2 = mfX2;
	double fCurY2 = mfY2;

	// Direction
	double fCurdX1 = mfdX1;
	double fCurdY1 = mfdY1;
	double fCurdX2 = mfdX2;
	double fCurdY2 = mfdY2;

	// Color
	int64_t nCurR = mnR;
	int64_t nCurG = mnG;
	int64_t nCurB = mnB;

	int64_t nCurdR = mndR;
	int64_t nCurdG = mndG;
	int64_t nCurdB = mndB;

	for (int64_t i = 0; i < mnMaxTailLength; i++)
	{
		// X1
		fCurX1 += fCurdX1;
		if (fCurX1 > mrExtents.right)
		{
			fCurX1 = mrExtents.right;
			fCurdX1 = -fCurdX1;
		}
		else if (fCurX1 < mrExtents.left)
		{
			fCurX1 = mrExtents.left;
			fCurdX1 = -fCurdX1;
		}

		// Y1
		fCurY1 += fCurdY1;
		if (fCurY1 > mrExtents.right)
		{
			fCurY1 = mrExtents.right;
			fCurdY1 = -fCurdY1;
		}
		else if (fCurY1 < mrExtents.left)
		{
			fCurY1 = mrExtents.left;
			fCurdY1 = -fCurdY1;
		}

		// X2
		fCurX2 += fCurdX2;
		if (fCurX2 > mrExtents.right)
		{
			fCurX2 = mrExtents.right;
			fCurdX2 = -fCurdX2;
		}
		else if (fCurX2 < mrExtents.left)
		{
			fCurX2 = mrExtents.left;
			fCurdX2 = -fCurdX2;
		}

		// Y2
		fCurY2 += fCurdY2;
		if (fCurY2 > mrExtents.right)
		{
			fCurY2 = mrExtents.right;
			fCurdY2 = -fCurdY2;
		}
		else if (fCurY2 < mrExtents.left)
		{
			fCurY2 = mrExtents.left;
			fCurdY2 = -fCurdY2;
		}

		// R
		nCurR += nCurdR;
		if (nCurR > 255)
		{
			nCurR = 255;
			nCurdR = -nCurdR;
		}
		else if (nCurR < 0)
		{
			nCurR = 0;
			nCurdR = -nCurdR;
		}

		// G
		nCurG += nCurdG;
		if (nCurG > 255)
		{
			nCurG = 255;
			nCurdG = -nCurdG;
		}
		else if (nCurG < 0)
		{
			nCurG = 0;
			nCurdG = -nCurdG;
		}

		// B
		nCurB += nCurdB;
		if (nCurB > 255)
		{
			nCurB = 255;
			nCurdB = -nCurdB;
		}
		else if (nCurB < 0)
		{
			nCurB = 0;
			nCurdB = -nCurdB;
		}

		uint32_t nCol = 0xFF << 24 | (uint32_t) nCurR << 16 | (uint32_t) nCurG << 8 | (uint32_t)nCurB;
		ZVertex v1(fCurX1, fCurY1, nCol);
		ZVertex v2(fCurX2, fCurY2, nCol);

		pBufferToDrawTo->DrawAlphaLine(v1, v2);

		if (mbXMirror)
		{
			ZVertex v1(mrExtents.right - fCurX1, fCurY1, nCol);
			ZVertex v2(mrExtents.right - fCurX2, fCurY2, nCol);
			pBufferToDrawTo->DrawAlphaLine(v1, v2, pClip);
		}

		if (mbYMirror)
		{
			ZVertex v1(fCurX1, mrExtents.bottom - fCurY1, nCol);
			ZVertex v2(fCurX2, mrExtents.bottom - fCurY2, nCol);

			pBufferToDrawTo->DrawAlphaLine(v1, v2, pClip);

			if (mbXMirror)
			{
				ZVertex v1(mrExtents.right - fCurX1, mrExtents.bottom - fCurY1, nCol);
				ZVertex v2(mrExtents.right - fCurX2, mrExtents.bottom - fCurY2, nCol);
				pBufferToDrawTo->DrawAlphaLine(v1, v2, pClip);
			}
		}
	}

	return true;
}

void ZAnimObject_BouncyLine::Process()
{
	// X1
	mfX1 += mfdX1;
	if (mfX1 > mrExtents.right)
	{
		mfX1 = mrExtents.right;
		mfdX1 = -mfdX1;
	}
	else if (mfX1 < mrExtents.left)
	{
		mfX1 = mrExtents.left;
		mfdX1 = -mfdX1;
	}

	// Y1
	mfY1 += mfdY1;
	if (mfY1 > mrExtents.right)
	{
		mfY1 = mrExtents.right;
		mfdY1 = -mfdY1;
	}
	else if (mfY1 < mrExtents.left)
	{
		mfY1 = mrExtents.left;
		mfdY1 = -mfdY1;
	}

	// X2
	mfX2 += mfdX2;
	if (mfX2 > mrExtents.right)
	{
		mfX2 = mrExtents.right;
		mfdX2 = -mfdX2;
	}
	else if (mfX2 < mrExtents.left)
	{
		mfX2 = mrExtents.left;
		mfdX2 = -mfdX2;
	}

	// Y2
	mfY2 += mfdY2;
	if (mfY2 > mrExtents.right)
	{
		mfY2 = mrExtents.right;
		mfdY2 = -mfdY2;
	}
	else if (mfY2 < mrExtents.left)
	{
		mfY2 = mrExtents.left;
		mfdY2 = -mfdY2;
	}

	// R
	mnR += mndR;
	if (mnR > 255)
	{
		mnR = 255;
		mndR = -mndR;
	}
	else if (mnR < 0)
	{
		mnR = 0;
		mndR = -mndR;
	}

	// G
	mnG += mndG;
	if (mnG > 255)
	{
		mnG = 255;
		mndG = -mndG;
	}
	else if (mnG < 0)
	{
		mnG = 0;
		mndG = -mndG;
	}

	// B
	mnB += mndB;
	if (mnB > 255)
	{
		mnB = 255;
		mndB = -mndB;
	}
	else if (mnB < 0)
	{
		mnB = 0;
		mndB = -mndB;
	}
}

void ZAnimObject_BouncyLine::SetParams(ZRect rExtents, int64_t nTailLength, uint8_t nType)
{
	mrExtents.SetRect((double) rExtents.left, (double) rExtents.top, (double) rExtents.right, (double) rExtents.bottom);
	mnMaxTailLength = nTailLength;
	mbXMirror = (nType & kXMirror) != 0;
	mbYMirror = (nType & kYMirror) != 0;
	mbAAttractsB = (nType & kAAttractsB) != 0;
	mbBAttractsA = (nType & kBAttractsA) != 0;

	RandomizeLine();
}
*/


// cCEAnimObject
ZAnimObject_Transformer::ZAnimObject_Transformer(ZTransformable* pTransformable) : ZAnimObject()
{
	mpTransformer = pTransformable;
}

ZAnimObject_Transformer::~ZAnimObject_Transformer()
{
	if (mpTransformer)
	{
		mpTransformer->EndTransformation();
		mState = kFinished;
		mpTransformer = NULL;
	}
}


bool ZAnimObject_Transformer::Paint()
{
	if (mpTransformer)
	{
		if (mpTransformer->GetState() == ZTransformable::kFinished)
		{
			mState = kFinished;
			mpTransformer = NULL;
		}
//		else
//			mpTransformer->TransformDraw(mpDestination.get(), &mpDestination->GetArea());
	}
	return true;
}

ZAnimObject_TransformingImage::ZAnimObject_TransformingImage(tZBufferPtr pImage, tZBufferPtr pBackground, ZRect* pArea)
{
	ZASSERT(pImage);

	if (pArea)
		mrArea.SetRect(*pArea);
	else
		mrArea = pImage->GetArea();

	ZTransformable::Init(mrArea);
    mpImage = pImage;


    mpBackground = pBackground;

    mpWorkingBuffer.reset(new ZBuffer());
    mpWorkingBuffer->Init(mpBackground->GetArea().Width(), mpBackground->GetArea().Height());

#ifdef _DEBUG
	Sprintf(msDebugName, "TransformingImage:%d x %d", mpImage->GetArea().Width(), mpImage->GetArea().Height());
#endif
}

ZAnimObject_TransformingImage::~ZAnimObject_TransformingImage()
{
}

bool ZAnimObject_TransformingImage::Paint()
{
    tRectList dirtyRects;
    if (mTransformState == ZTransformable::kFinished)
	{
		mState = ZAnimObject::kFinished;
        //mrArea.SetRect(mCurTransform.mPosition.x, mCurTransform.mPosition.y, mCurTransform.mPosition.x + mrBaseArea.Width(), mCurTransform.mPosition.y + mrBaseArea.Height());
        mrArea = mBounds;
    }
    else
    {
        
        // for now I'm drawing directly to screen buffer...... I may want to re-enable drawing to arbitrary destinations with either blt or rasterization
        // maybe have a screenbuffer blt that does GDI and or rasterization to a temp buffer then transfer via mask?
        bool bFirstDraw = false;
        if (mrLastDrawArea.Width() == 0 || mrLastDrawArea.Height() == 0)
            bFirstDraw = true;

        mrLastDrawArea = mrArea;
//        mrArea.SetRect(mCurTransform.mPosition.x, mCurTransform.mPosition.y, mCurTransform.mPosition.x + mrBaseArea.Width(), mCurTransform.mPosition.y + mrBaseArea.Height());
        mrArea = mBounds;

        // if this is the first time drawing, re-set
        if (bFirstDraw)
            mrLastDrawArea = mrArea;

        if (mCurTransform.mScale == 1.0 && mCurTransform.mRotation == 0 && mCurTransform.mnAlpha > 0xf0)
        {
            if (mpDestination)
                mpDestination->Blt(mpImage.get(), mpImage->GetArea(), mrArea);
            else
                gpGraphicSystem->GetScreenBuffer()->RenderBuffer(mpImage.get(), mpImage->GetArea(), mrArea);
        }
        else
        {
            mpWorkingBuffer->CopyPixels(mpBackground.get());

            if (mpDestination)
                mpDestination->Blt(mpImage.get(), mpImage->GetArea(), mrArea);
            else
            {
                gRasterizer.RasterizeWithAlpha(mpWorkingBuffer.get(), mpImage.get(), mVerts, nullptr, mCurTransform.mnAlpha);
                gpGraphicSystem->GetScreenBuffer()->RenderBuffer(mpWorkingBuffer.get(), mBounds, mBounds);
            }
        }
    }

    dirtyRects = ComputeDirtyRects(mrLastDrawArea, mrArea);

    // render the area that's left
    for (auto& r : dirtyRects)
    {
        gpGraphicSystem->GetScreenBuffer()->RenderBuffer(mpBackground.get(), r, r);
    }

	return true;
}

