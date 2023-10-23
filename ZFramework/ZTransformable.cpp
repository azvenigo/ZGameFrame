#include "ZTransformable.h"
#include "ZRasterizer.h"
#include "ZTickManager.h"
#include "ZMessageSystem.h"
#include "helpers/StringHelpers.h"
#include "ZTimer.h"
#include <math.h>

extern ZRasterizer	gRasterizer;
extern ZTickManager	gTickManager;
extern ZTimer			gTimer;

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
using namespace std;

ZTransformation::ZTransformation(const ZTransformation& transform)
{
	*this = transform;
}

ZTransformation::ZTransformation(const ZPoint& pos, double scale, double rotation, uint32_t alpha, const string& sCompletionMessage)
{
	mPosition			= pos;
	mScale				= scale;
	mRotation			= rotation;
	mnAlpha				= alpha;
	msCompletionMessage = sCompletionMessage;
}

ZTransformation& ZTransformation::operator=(const ZTransformation& trans)
{
	mPosition			= trans.mPosition;
	mScale				= trans.mScale;
	mRotation			= trans.mRotation;
	mnTimestamp 		= trans.mnTimestamp;
	mnAlpha				= trans.mnAlpha;
	msCompletionMessage = trans.msCompletionMessage;

	return *this;
}

bool ZTransformation::operator==(const ZTransformation& trans)
{
	return (mPosition 	== trans.mPosition &&
			mScale    	== trans.mScale &&
			mRotation 	== trans.mRotation &&
			mnAlpha		== trans.mnAlpha);
}


string ZTransformation::ToString()
{
	string sRaw;
    sRaw = SH::FromInt(mPosition.x) + "," + SH::FromInt(mPosition.y) + "," + SH::FromDouble(mScale) + "," + SH::FromDouble(mRotation) + "," + SH::FromInt(mnTimestamp) + "," + SH::FromInt(mnAlpha) + ",\"" + msCompletionMessage + "\"";

	return sRaw;
}

void ZTransformation::FromString(string sRaw)
{
	string sTemp;
    SH::SplitToken(sTemp, sRaw, ",");
	mPosition.x = SH::ToInt(sTemp);

    SH::SplitToken(sTemp, sRaw, ",");
	mPosition.y = SH::ToInt(sTemp);

    SH::SplitToken(sTemp, sRaw, ",");
	mScale		= SH::ToDouble(sTemp);

    SH::SplitToken(sTemp, sRaw, ",");
	mRotation	= SH::ToDouble(sTemp);

    SH::SplitToken(sTemp, sRaw, ",");
	mnTimestamp = SH::ToInt(sTemp);

    SH::SplitToken(sTemp, sRaw, ",");
	mnAlpha		= (uint32_t) SH::ToInt(sTemp);

	ZASSERT(sTemp[0] == '\"');
	ZASSERT(sTemp[sTemp.length()-1] == '\"');
	msCompletionMessage = sTemp.substr(1, sTemp.length()-2);	//  grab everything but the quotes
}




string ZTransformationList::ToString()
{
	string sRaw;

	for (ZTransformationList::iterator it = begin(); it != end(); it++)
	{
		sRaw += "[" + (*it).ToString() + "]";
	}

	return sRaw;
}

ZTransformable::ZTransformable()
{
	mTransformState = kNone;
	mbFirstTransformation = false;
	mVerts.resize(4);

	mVerts[0].u = 0.0;
	mVerts[0].v = 0.0;

	mVerts[1].u = 1.0;
	mVerts[1].v = 0.0;

	mVerts[2].u = 1.0;
	mVerts[2].v = 1.0;

	mVerts[3].u = 0.0;
	mVerts[3].v = 1.0;
}

ZTransformable::~ZTransformable()
{
	Shutdown();
}

bool ZTransformable::Init(const ZRect& rArea)
{
	gTickManager.AddObject(this);
    mrBaseArea = rArea;

	// Initialize the current transform
	SetTransform(ZTransformation(ZPoint(rArea.left, rArea.top)));

	return true;
}


bool ZTransformable::Shutdown()
{
	gTickManager.RemoveObject(this);
	mTransformState = kNone;
	return true;
}

inline void TransformPoint(double& fX, double& fY, double fOriginX, double fOriginY, double angle, double scale)
{
	const double x(fX - fOriginX);
	const double y(fY - fOriginY);
	const double cosAngle(::cos(angle));
	const double sinAngle(::sin(angle));

	fX = scale*fOriginX + scale*(x*cosAngle - y*sinAngle);
	fY = scale*fOriginY + scale*(x*sinAngle + y*cosAngle);
}

bool ZTransformable::Tick()
{
	if (mTransformState == kTransforming)
	{
		ZTransformation oldTrans(mCurTransform);

		int64_t nTimeDelta = gTimer.GetElapsedTime() - mCurTransform.mnTimestamp;
		ZASSERT(nTimeDelta >= 0);

		mCurTransform.mnTimestamp = gTimer.GetElapsedTime();

        const std::lock_guard<std::recursive_mutex> lock(mTransformationListMutex);
       
        if (mEndTransform.mnTimestamp - mCurTransform.mnTimestamp <= 0)
		{
            if (!mCurTransform.msCompletionMessage.empty())
                gMessageSystem.Post(mCurTransform.msCompletionMessage);

            
            if (mTransformationList.empty())
			{
				// No more transformations
				EndTransformation();
				return false;
			}
			else
			{
				mbFirstTransformation = mStartTransform == mEndTransform;
				// Begin a transformation from the current to the next in the list

				mStartTransform = mCurTransform;

                mEndTransform = *(mTransformationList.begin());
				mTransformationList.pop_front();
			}
		}

		bool bLastTransform = mTransformationList.empty();
		bool bEaseFrom = mbFirstTransformation;
		bool bEaseTo = bLastTransform || mEndTransform.mPosition == (*mTransformationList.begin()).mPosition;

		// Calculate the current transform

		const double pi = 3.141592653;
		double fRange;
		double fT;
		double fCurrent = ((double) mCurTransform.mnTimestamp - (double) mStartTransform.mnTimestamp);
		double fFullTime = ((double) mEndTransform.mnTimestamp - (double) mStartTransform.mnTimestamp);

        if (fFullTime > 0.0)
        {
            double fPercentage = fCurrent / fFullTime;

            if (!bEaseFrom && !bEaseTo)		//	false false
            {
                // Linear transform
                fT = fCurrent / fFullTime;
                //			ZDEBUG_OUT("Transition linear.\n");
            }
            else if (bEaseFrom && bEaseTo)	// true true
            {
                // Ease Both
                fRange = fPercentage * pi;
                fT = 1.0 - (1.0 + cos(fRange)) / 2.0;
                //			ZDEBUG_OUT("Easing Both\n");
            }
            else if (bEaseFrom)				// true false
            {
                fRange = pi / 2.0 - fPercentage * pi / 2.0;
                fT = 1.0 - sin(fRange);
                //			ZDEBUG_OUT("Easing in.\n");
            }
            else if (bEaseTo)				// false true
            {
                fRange = fPercentage * pi / 2.0;
                fT = sin(fRange);
                //			ZDEBUG_OUT("Easing out.\n");
            }
            else
            {
                ZASSERT(false); //		!? What other state can there be?
            }

            ZASSERT(fT >= 0.0 && fT <= 1.0);
            //ZDEBUG_OUT("fCurrent:%f fFull:%f  fRange:%f  fT:%f\n", fCurrent, fFullTime, fRange, fT);
            limit<double>(fT, 0.0, 1.0);


            mCurTransform.mPosition.x = (int64_t)((double)mStartTransform.mPosition.x + (double)(mEndTransform.mPosition.x - mStartTransform.mPosition.x) * fT);
            mCurTransform.mPosition.y = (int64_t)((double)mStartTransform.mPosition.y + (double)(mEndTransform.mPosition.y - mStartTransform.mPosition.y) * fT);
            mCurTransform.mRotation = mStartTransform.mRotation + (mEndTransform.mRotation - mStartTransform.mRotation) * fT;
            mCurTransform.mScale = mStartTransform.mScale + (mEndTransform.mScale - mStartTransform.mScale) * fT;
            mCurTransform.mnAlpha = (uint32_t)((int32_t)mStartTransform.mnAlpha + ((int32_t)mEndTransform.mnAlpha - (int32_t)mStartTransform.mnAlpha) * fT);
            mCurTransform.msCompletionMessage = mEndTransform.msCompletionMessage;

//            ZDEBUG_OUT("alpha:", mCurTransform.mnAlpha, "\n");

        }
        else
        {
            if (!mCurTransform.msCompletionMessage.empty())
                gMessageSystem.Post(std::move(mCurTransform.msCompletionMessage));

            mCurTransform = mEndTransform;
        }

		UpdateVertsAndBounds();
//		ZDEBUG_OUT("fT: %3.2f size: %3.2f\n", fT, mCurTransform.mScale);

		return !(oldTrans == mCurTransform);		// if the old transformation == current, then this tick doesn't need a fast tick
	}

	return false;
}

void ZTransformable::UpdateVertsAndBounds()
{
    double fW = (double)mrBaseArea.Width();
    double fH = (double)mrBaseArea.Height();

	mVerts[0].x = (double) mCurTransform.mPosition.x;
	mVerts[0].y = (double) mCurTransform.mPosition.y;

	mVerts[1].x = (double) mCurTransform.mPosition.x + fW;
	mVerts[1].y = (double) mCurTransform.mPosition.y;

	mVerts[2].x = (double) mCurTransform.mPosition.x + fW;
	mVerts[2].y = (double) mCurTransform.mPosition.y + fH;

	mVerts[3].x = (double) mCurTransform.mPosition.x;
	mVerts[3].y = (double) mCurTransform.mPosition.y + fH;

	double centerX = (double) (mCurTransform.mPosition.x + fW) /2.0;
	double centerY = (double) (mCurTransform.mPosition.y + fH) /2.0;


    mBounds.left = MAXINT32;
    mBounds.right = MININT32;
    mBounds.top = MAXINT32;
    mBounds.bottom = MININT32;
	for (int i = 0; i < 4; i++)
	{
		TransformPoint(mVerts[i].x, mVerts[i].y, centerX, centerY, mCurTransform.mRotation, mCurTransform.mScale);
		int64_t nX = (int64_t) mVerts[i].x;
		int64_t nY = (int64_t) mVerts[i].y;
        if (mBounds.left > nX)
            mBounds.left = nX;
        if (mBounds.right < nX)
            mBounds.right = nX;
        if (mBounds.top > nY)
            mBounds.top = nY;
        if (mBounds.bottom < nY)
            mBounds.bottom = nY;
	}
}

/*bool ZTransformable::TransformDraw(ZBuffer* pBufferToDrawTo, ZRect* pClip)
{
//	ZDEBUG_OUT("Transform Draw: rect[%d,%d,%d,%d]\n", pClip->left, pClip->top, pClip->right, pClip->bottom);
	ZASSERT(pBufferToDrawTo);
	ZASSERT(mpTransformTexture.get());

	// If no transformation, just blt
	if (mCurTransform.mRotation == 0.0 && mCurTransform.mScale == 1.0)
	{
		ZRect rSource = mpTransformTexture.get()->GetArea();
		int64_t nDestX = mCurTransform.mPosition.x;
		int64_t nDestY = mCurTransform.mPosition.y;
		ZRect rFinalDest(nDestX, nDestY, nDestX + rSource.Width(), nDestY + rSource.Height());
		pBufferToDrawTo->BltAlpha(mpTransformTexture.get(), rSource, rFinalDest, mCurTransform.mnAlpha, pClip);
	}
	else
	{
		gRasterizer.RasterizeWithAlpha(pBufferToDrawTo, mpTransformTexture.get(), mVerts, pClip, (uint8_t) mCurTransform.mnAlpha);
	}

	return true;
}*/

void ZTransformable::StartTransformation(const ZTransformation& start)
{
	mStartTransform = start;
	mStartTransform.mnTimestamp = gTimer.GetElapsedTime();
	mbFirstTransformation = true;
	mCurTransform = mStartTransform;
	mEndTransform = mStartTransform;

	mTransformState = kTransforming;
	UpdateVertsAndBounds();
}

void ZTransformable::AddTransformation(ZTransformation trans, int64_t nDuration)
{
	ZASSERT(nDuration > 0);
	trans.mnTimestamp = GetLastTransform().mnTimestamp + nDuration;
   
    const std::lock_guard<std::recursive_mutex> lock(mTransformationListMutex);
    mTransformationList.push_back(trans);

	mTransformState = kTransforming;
}

void ZTransformable::SetTransform(const ZTransformation& newTransform) 
{ 
    mEndTransform = newTransform;
    mCurTransform = newTransform; 

	UpdateVertsAndBounds(); 
}

void ZTransformable::EndTransformation()
{
	SetTransform(GetLastTransform());
	mCurTransform.mnTimestamp = gTimer.GetElapsedTime();	// reset the current timestamp
	mEndTransform = mCurTransform;

    gMessageSystem.Post(mEndTransform.msCompletionMessage);

   
    const std::lock_guard<std::recursive_mutex> lock(mTransformationListMutex);
    mTransformationList.clear();
	mTransformState = kFinished;
}

void ZTransformable::DoTransformation(const string& sRaw)
{
    mTransformationListMutex.lock();
    mTransformationList.clear();
    mTransformationListMutex.unlock();

	string sTempList(sRaw);
	ZTransformation temp;
	string sTransTemp;
	bool bFirst = true;
	while (!sTempList.empty() && sTempList[0] == '[')
	{
        SH::SplitToken(sTransTemp, sTempList, "]");
		sTransTemp = sTransTemp.substr(1);	// remove the '['

		temp.FromString(sTransTemp);
		if (bFirst)
		{
			StartTransformation(temp);
			bFirst = false;
		}
		else
		{
			AddTransformation(temp, temp.mnTimestamp);
		}
	}

	ZASSERT(sTempList.empty());		// ensure we've processed everything
}


ZTransformation& ZTransformable::GetLastTransform()
{
    const std::lock_guard<std::recursive_mutex> lock(mTransformationListMutex);

	if (mTransformationList.empty())
		return mEndTransform;

	ZTransformationList::reverse_iterator it = mTransformationList.rbegin();
	ZTransformation& trans = *it;
	return trans;
}
