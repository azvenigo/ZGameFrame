#pragma once

#include "ZTypes.h"
#include "ZRasterizer.h"
#include "ZBuffer.h"


class ZTransformation
{
public:
	ZTransformation(const ZPoint& pos = ZPoint(0,0), double scale = 1.0, double rotation = 0.0, uint32_t alpha = 255, const std::string& sCompletionMessage = "");
	ZTransformation(const ZTransformation& transform);

	ZTransformation&	operator=(const ZTransformation& trans);
	bool				operator==(const ZTransformation& trans);

    std::string         ToString();
	void				FromString(std::string sRaw);

	ZPoint				mPosition;
	double				mScale;
	double				mRotation;
	uint32_t			mnAlpha;
	int64_t				mnTimestamp;
    std::string         msCompletionMessage;
};

//typedef std::list<cCETransformation> tTransformationList;

class ZTransformationList : public std::list<ZTransformation>
{
public:
    std::string ToString();
//	void FromString(const string& sRaw);
};

class ZTransformable 
{
public:
	enum eTransformState
	{
		kNone			= 0,
		kTransforming	= 1,
		kFinished		= 2
	};

	ZTransformable();
	~ZTransformable();

	virtual bool			Init(const ZRect& rArea);
    virtual bool            Init(ZBuffer* pBuffer);
	virtual bool			Shutdown();

	virtual void			StartTransformation(const ZTransformation& start);
	virtual void			AddTransformation(ZTransformation trans, int64_t nDuration);
	virtual void			EndTransformation();	// Snaps to end

	virtual void			DoTransformation(const std::string& sRaw);	// parses the string and creates the transformation queue

	eTransformState			GetState()			{ return mTransformState; }
	ZTransformation&		GetStartTransform() { return mStartTransform; }
	ZTransformation&		GetEndTransform()	{ return mStartTransform; }


	void					SetTransform(const ZTransformation& newTransform);

	const ZTransformation&	GetTransform() { return mCurTransform; }

	virtual bool			Tick();

	virtual tZBufferPtr		GetTransformTexture() { return mpTransformTexture; }
	virtual bool			TransformDraw(ZBuffer* pBufferToDrawTo, ZRect* pClip = NULL);

	ZRect&					GetBounds()			{ return mBounds; }

#ifdef _DEBUG
    std::string				msDebugName;
#endif
protected:
	void					UpdateVertsAndBounds();

    tZBufferPtr				mpTransformTexture;

	eTransformState			mTransformState;
	ZTransformation			mStartTransform;
	ZTransformation			mEndTransform;

	tUVVertexArray			mVerts;
	ZRect					mBounds;

private:

	ZTransformation&		GetLastTransform();

	bool					mbFirstTransformation;
	ZTransformation			mCurTransform;
	ZTransformationList		mTransformationList;


};
