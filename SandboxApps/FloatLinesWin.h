#pragma once

#include "ZWin.h"
#include "ZAnimator.h"

class ZXMLNode;

class ZFloatVertex
{
public:
	ZFloatVertex();
	ZFloatVertex(double x, double y, double a, double r, double g, double b);
	ZFloatVertex(const ZFloatVertex& p);

	// Operations
	void Offset(double dx, double dy);
	void operator+=(ZFloatVertex& p);
	void operator-=(ZFloatVertex& p);

	// Operators returning cFPoint values
	ZFloatVertex operator-() const;
	ZFloatVertex operator+(ZFloatVertex& p) const;

public:
	double	mx;
	double	my;
	double   mfA;
	double   mfR;
	double   mfG;
	double   mfB;
};

inline bool operator==(const ZFloatVertex& a,const ZFloatVertex& b)
{
	return (a.mx == b.mx && a.my == b.my);
}

inline bool operator!=(const ZFloatVertex& a,const ZFloatVertex& b)
{
	return (!(a==b));
}

inline ZFloatVertex::ZFloatVertex()
{  
}

inline ZFloatVertex::ZFloatVertex(double x, double y, double a, double r, double g, double b)
{ 
	mx = x; 
	my = y; 

	mfA = a;
	mfR = r;
	mfG = g;
	mfB = b;
}

inline ZFloatVertex::ZFloatVertex(const ZFloatVertex& p)
{
	*this = p; 
}

inline void ZFloatVertex::Offset(double dx, double dy)
{ 
	mx += dx; 
	my += dy; 
}

inline void ZFloatVertex::operator+=(ZFloatVertex& p)
{ 
	mx += p.mx; 
	my += p.my; 
}

inline void ZFloatVertex::operator-=(ZFloatVertex& p)
{ 
	mx -= p.mx; 
	my -= p.my; 
}

inline ZFloatVertex ZFloatVertex::operator-() const
{ 
	return ZFloatVertex(-mx, -my, mfA, mfR, mfG, mfB);
}

inline ZFloatVertex ZFloatVertex::operator+(ZFloatVertex& p) const
{ 
	return ZFloatVertex(mx + p.mx, my + p.my, mfA, mfR, mfG, mfB);
}






class ZLine
{
public:
	ZLine();
	~ZLine();

	void SetBounds(ZRect rBoundry);
	void Process();
	void RandomizeLine();
    void CheckLimits();

	// Endpoints
	double mfX1;
	double mfY1;
	double mfX2;
	double mfY2;

	// Direction
	double mfdX1;
	double mfdY1;
	double mfdX2;
	double mfdY2;

	// Color
	double mfR;
	double mfG;
	double mfB;

	double mfdR;
	double mfdG;
	double mfdB;

	ZRect mrBounds;
	bool  mbGravityV1;
	bool  mbGravityV2;
	bool  mbV1AttractsV2;
	bool  mbV2AttractsV1;
	double mfV1AttractionForceDivisor;
	double mfV2AttractionForceDivisor;

	bool mbExternalGravity;
	ZFPoint mExternalGravityPoint;
    bool mbVariableGravity;
};

/////////////////////////////////////////////////////////////////////////
// 
class cFloatLinesWin : public ZWin
{
public:
	cFloatLinesWin();

	bool			Init();
	bool			Shutdown();
	bool			OnMouseDownL(int64_t x, int64_t y);
	virtual bool	OnChar(char c);
	virtual bool	OnKeyDown(uint32_t key);

	void			ShowPopupWin(ZWin* pWin, const ZRect& rArea);
	void			ShowTestWin();


protected:
    bool			Paint();


private:
    void            ComputeFloatLines();
	bool			FloatScanLineIntersection(double fScanLine, const ZFloatVertex& v1, const ZFloatVertex& v2, double& fIntersection, double& fR, double& fG, double& fB, double& fA);
	void			FillInSpan(ZBuffer* pBufferToDrawTo, double* pDest, int64_t nNumPixels, double fR, double fG, double fB, double fA, int64_t nX, int64_t nY);
	void			DrawAlphaLine(ZBuffer* pBufferToDrawTo, ZFloatVertex& v1, ZFloatVertex& v2, ZRect* pClip);
	bool			RandomizeSettings();
	void			ClearBuffer(ZBuffer* pBufferToDrawTo);

	ZAnimator		mAnimator;

	double* 			mpFloatBuffer;
	int64_t			mnProcessPerFrame;
	double			mfLineAlpha;

	bool			mbDrawTail;

	bool			mbLightDecay;
	int64_t			mnDecayScanLine;
	double			mfDecayAmount;

	int64_t 			mnMaxTailLength;
	int64_t 			mnNumDrawn;
	bool   			mbXMirror;
	bool   			mbYMirror;

	ZLine  			mHeadLine;
	ZLine  			mTailLine;
	bool 			mbReset;
	double  			mfLineThickness;
	uint64_t			mnTimeStampOfReset;
    uint64_t			mnMSBetweenResets;

	bool			mbVariableLineThickness;

};


