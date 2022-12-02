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
    sRaw = StringHelpers::FromInt(mPosition.mX) + "," + StringHelpers::FromInt(mPosition.mY) + "," + StringHelpers::FromDouble(mScale) + "," + StringHelpers::FromDouble(mRotation) + "," + StringHelpers::FromInt(mnTimestamp) + "," + StringHelpers::FromInt(mnAlpha) + ",\"" + msCompletionMessage + "\"";

	return sRaw;
}

void ZTransformation::FromString(string sRaw)
{
	string sTemp;
    StringHelpers::SplitToken(sTemp, sRaw, ",");
	mPosition.mX = StringHelpers::ToInt(sTemp);

    StringHelpers::SplitToken(sTemp, sRaw, ",");
	mPosition.mY = StringHelpers::ToInt(sTemp);

    StringHelpers::SplitToken(sTemp, sRaw, ",");
	mScale		= StringHelpers::ToDouble(sTemp);

    StringHelpers::SplitToken(sTemp, sRaw, ",");
	mRotation	= StringHelpers::ToDouble(sTemp);

    StringHelpers::SplitToken(sTemp, sRaw, ",");
	mnTimestamp = StringHelpers::ToInt(sTemp);

    StringHelpers::SplitToken(sTemp, sRaw, ",");
	mnAlpha		= (uint32_t) StringHelpers::ToInt(sTemp);

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
    mpTransformTexture = NULL;
	mTransformState = kNone;
	mbFirstTransformation = false;
	mVerts.resize(4);

	mVerts[0].mU = 0.0;
	mVerts[0].mV = 0.0;

	mVerts[1].mU = 1.0;
	mVerts[1].mV = 0.0;

	mVerts[2].mU = 1.0;
	mVerts[2].mV = 1.0;

	mVerts[3].mU = 0.0;
	mVerts[3].mV = 1.0;
}

ZTransformable::~ZTransformable()
{
	Shutdown();
}

bool ZTransformable::Init(const ZRect& rArea)
{
	ZASSERT(rArea.Width() > 0 && rArea.Height() > 0);

	gTickManager.AddObject(this);

	if (mpTransformTexture.get() && mpTransformTexture.get()->GetArea().Width() == rArea.Width() && mpTransformTexture.get()->GetArea().Height() == rArea.Height())
	{
		return true;
	}

	mpTransformTexture.reset(new ZBuffer());
    mpTransformTexture.get()->Init(rArea.Width(), rArea.Height());

	// Initialize the current transform
	ZTransformation trans(ZPoint(rArea.left, rArea.top));
	SetTransform(trans);

	return true;
}

bool ZTransformable::Init(ZBuffer* pBuffer)
{
    assert(pBuffer);

    ZRect r(pBuffer->GetArea());

    if (!ZTransformable::Init(r))
        return false;

    mpTransformTexture.get()->CopyPixels(pBuffer, r, r);

    return true;
}



bool ZTransformable::Shutdown()
{
    mpTransformTexture.reset();

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

	fX = fOriginX + scale*(x*cosAngle - y*sinAngle);
	fY = fOriginY + scale*(x*sinAngle + y*cosAngle);
}

bool ZTransformable::Tick()
{
	if (mTransformState == kTransforming)
	{
		ZTransformation oldTrans(mCurTransform);

		int64_t nTimeDelta = gTimer.GetElapsedTime() - mCurTransform.mnTimestamp;
		ZASSERT(nTimeDelta >= 0);

		mCurTransform.mnTimestamp = gTimer.GetElapsedTime();

		if (mEndTransform.mnTimestamp - mCurTransform.mnTimestamp <= 0)
		{
			if (mTransformationList.empty())
			{
				// No more transformations
				EndTransformation();
				return false;
			}
			else
			{
				mbFirstTransformation = mStartTransform.mPosition == mEndTransform.mPosition;
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
			fT = 1.0-(1.0+cos(fRange))/2.0;
//			ZDEBUG_OUT("Easing Both\n");
		}
		else if (bEaseFrom)				// true false
		{
			fRange = pi/2.0 - fPercentage * pi/2.0;
			fT = 1.0-sin(fRange);
//			ZDEBUG_OUT("Easing in.\n");
		}
		else if (bEaseTo)				// false true
		{
			fRange = fPercentage * pi/2.0;
			fT = sin(fRange);
//			ZDEBUG_OUT("Easing out.\n");
		}
		else
		{
			ZASSERT(false); //		!? What other state can there be?
		}

		ZASSERT(fT >= 0.0 && fT <= 1.0);
		//ZDEBUG_OUT("fCurrent:%f fFull:%f  fRange:%f  fT:%f\n", fCurrent, fFullTime, fRange, fT);
		if (fT < -0.0)
			fT = 0.0;
		else if (fT > 1.0)
			fT = 1.0;

/*		for (double fTest = 0.0; fTest < 1.0; fTest += 0.02)
		{
			const double pi = 3.141592653;


			// Straight in..... ease out
//			double fRange = fTest * pi/2.0;
//			double fT = sin(fRange);

			// ease in....... straight out
			double fRange = fTest * pi/2.0;
			double fT = sin(fRange);


//			double fRange = fTest * pi;
//			double fT = 1.0-(1.0+cos(fRange))/2.0;


			ZDEBUG_OUT("fCurrent:%f fFull:%f  fRange:%f  fT:%f\n", fCurrent, fFullTime, fRange, fT);
			for (int64_t j = 0; j < (int64_t) (100.0*fT); j++)
			{
				ZDEBUG_OUT(".");
			}
			ZDEBUG_OUT("\n");
		}

*/





//		double fT = fArcTanTransition;
//		double fT = ((double) mCurTransform.mnTimestamp - (double) mStartTransform.mnTimestamp) / ((double) mEndTransform.mnTimestamp - (double) mStartTransform.mnTimestamp);
		mCurTransform.mPosition.mX = (int64_t) ((double)mStartTransform.mPosition.mX + (double) (mEndTransform.mPosition.mX -mStartTransform.mPosition.mX) * fT);
		mCurTransform.mPosition.mY = (int64_t) ((double)mStartTransform.mPosition.mY + (double) (mEndTransform.mPosition.mY -mStartTransform.mPosition.mY) * fT);
		mCurTransform.mRotation = mStartTransform.mRotation + (mEndTransform.mRotation-mStartTransform.mRotation) * fT;
		mCurTransform.mScale = mStartTransform.mScale + (mEndTransform.mScale-mStartTransform.mScale) * fT;
		mCurTransform.mnAlpha = (uint32_t) (mStartTransform.mnAlpha + (mEndTransform.mnAlpha-mStartTransform.mnAlpha) * fT);

		UpdateVertsAndBounds();
//		ZDEBUG_OUT("fT: %3.2f size: %3.2f\n", fT, mCurTransform.mScale);

		return !(oldTrans == mCurTransform);		// if the old transformation == current, then this tick doesn't need a fast tick
	}

	return false;
}

void ZTransformable::UpdateVertsAndBounds()
{
	mBounds.left = MAXINT32;
	mBounds.right = MININT32;
	mBounds.top = MAXINT32;
	mBounds.bottom = MININT32;

	ZRect rArea = mpTransformTexture.get()->GetArea();
	mVerts[0].mX = (double) mCurTransform.mPosition.mX;
	mVerts[0].mY = (double)mCurTransform.mPosition.mY;

	mVerts[1].mX = (double)mCurTransform.mPosition.mX + rArea.Width();
	mVerts[1].mY = (double)mCurTransform.mPosition.mY;

	mVerts[2].mX = (double)mCurTransform.mPosition.mX + rArea.Width();
	mVerts[2].mY = (double)mCurTransform.mPosition.mY + rArea.Height();

	mVerts[3].mX = (double)mCurTransform.mPosition.mX;
	mVerts[3].mY = (double)mCurTransform.mPosition.mY + rArea.Height();

	double centerX = (double)mCurTransform.mPosition.mX + rArea.Width()/2;
	double centerY = (double)mCurTransform.mPosition.mY + rArea.Height()/2;

	for (int i = 0; i < 4; i++)
	{
		TransformPoint(mVerts[i].mX, mVerts[i].mY, centerX, centerY, mCurTransform.mRotation, mCurTransform.mScale);
		int64_t nX = (int64_t) mVerts[i].mX;
		int64_t nY = (int64_t) mVerts[i].mY;
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

bool ZTransformable::TransformDraw(ZBuffer* pBufferToDrawTo, ZRect* pClip)
{
//	ZDEBUG_OUT("Transform Draw: rect[%d,%d,%d,%d]\n", pClip->left, pClip->top, pClip->right, pClip->bottom);
	ZASSERT(pBufferToDrawTo);
	ZASSERT(mpTransformTexture.get());

	// If no transformation, just blt
	if (mCurTransform.mRotation == 0.0 && mCurTransform.mScale == 1.0)
	{
		ZRect rSource = mpTransformTexture.get()->GetArea();
		int64_t nDestX = mCurTransform.mPosition.mX;
		int64_t nDestY = mCurTransform.mPosition.mY;
		ZRect rFinalDest(nDestX, nDestY, nDestX + rSource.Width(), nDestY + rSource.Height());
		pBufferToDrawTo->BltAlpha(mpTransformTexture.get(), rSource, rFinalDest, mCurTransform.mnAlpha, pClip);
	}
	else
	{
		gRasterizer.RasterizeWithAlpha(pBufferToDrawTo, mpTransformTexture.get(), mVerts, pClip, (uint8_t) mCurTransform.mnAlpha);
	}

	return true;
}

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
	mTransformationList.clear();
	mTransformState = kFinished;
	if (!mEndTransform.msCompletionMessage.empty())
		gMessageSystem.Post(mEndTransform.msCompletionMessage);
}

void ZTransformable::DoTransformation(const string& sRaw)
{
	mTransformationList.clear();

	string sTempList(sRaw);
	ZTransformation temp;
	string sTransTemp;
	bool bFirst = true;
	while (!sTempList.empty() && sTempList[0] == '[')
	{
        StringHelpers::SplitToken(sTransTemp, sTempList, "]");
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
	if (mTransformationList.empty())
		return mEndTransform;

	ZTransformationList::reverse_iterator it = mTransformationList.rbegin();
	ZTransformation& trans = *it;
	return trans;
}
