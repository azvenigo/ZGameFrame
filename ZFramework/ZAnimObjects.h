#pragma once

#include "ZAnimObjects.h"
#include "ZRasterizer.h"
#include "ZTransformable.h"
#include "ZFont.h"
#include <list>
#include <string>
#include "ZGUIStyle.h"
#include "ZTypes.h"

class ZBuffer;

class ZAnimObject
{
public:
	enum eState
	{
		kNone = 0,
		kAnimating = 1,
		kHidden = 2,
		kFinished = 3
	};

									ZAnimObject();
   virtual							~ZAnimObject();

   virtual bool                  	Paint() { return false; }
   virtual eState   	            GetState() { return mState; }
   virtual void                  	SetState(eState state) { mState = state; }

   virtual void						SetContext(void* pContext) { mpContext = pContext; }
   virtual void*					GetContext() { return mpContext; }

   virtual void                     SetDestination(tZBufferPtr pDestination) { mpDestination = pDestination; }


   ZRect                			mrArea;
   ZRect                            mrLastDrawArea;

protected:
   eState  			                mState;
   int64_t               			mnTimeStamp;
   tZBufferPtr                      mpDestination;
   void*							mpContext;			// Contextual owner.
};

// Moves text
class ZAnimObject_TextMover : public ZAnimObject
{
public:
    ZAnimObject_TextMover(ZGUI::Style _style = {});

    virtual bool    Paint();

    void            SetText(const std::string& sText);
    void           	SetLocation(int64_t nX, int64_t nY);
    void           	SetPixelsPerSecond(double fDX, double fDY);
    void           	SetAlphaFade(int64_t nStartAlpha, int64_t nEndAlpha, int64_t nMilliseconds);

protected:
    std::string     msText;
    ZGUI::Style     mStyle;
    int64_t			mnStartAlpha;
    int64_t			mnEndAlpha;
    int64_t         mnTotalFadeTime;
    int64_t         mnRemainingAlphaFadeTime;
    bool           	mbAlphaFading;

    // we keep our location in floating point values for smooth animating
    double          mfX;           
    double          mfY;
    double          mfDX;
    double          mfDY;
};


typedef std::list<ZFPoint>       tPointList;

struct sSparkle
{
   tPointList  streakPoints;
   double       fDX;
   double       fDY;
   double       fBrightness;
   int64_t      nLife;
};

typedef std::list<sSparkle>   tSparkleList;

class ZCEAnimObject_Sparkler : public ZAnimObject
{
public:
   // cCEAnimObject
   ZCEAnimObject_Sparkler();

   virtual bool		Paint();

   // ZCEAnimObject_Sparkler 
   void           	SetSource(tZBufferPtr pSource, const ZRect& rBounds, uint32_t nSourceColor);
   void           	SetNumParticles(int64_t nParticles) { mnParticles = nParticles; }
   void           	SetMaxStreakSize(int64_t nMaxStreakSize) { mnMaxStreakSize = nMaxStreakSize; }
   void           	SetMaxBrightness(double fMaxBrightness) { mfMaxBrightness = fMaxBrightness; }

private:
   void           	CreateSparkle();
   void           	ProcessSparkles();

   tSparkleList   	mSparkleList;
   int64_t         	mnParticles;      // the desired number of particles
   int64_t         	mnMaxStreakSize;  // How long a streak should be
   double          	mfMaxBrightness;  // How "hot" it burns

   ZRect			mrBounds;
   tZBufferPtr		mpSourceBuffer;
   uint32_t			mnSourceColor;
};

// Alpha Pulses text
class ZAnimObject_TextPulser : public ZAnimObject
{
public:
   // cCEAnimObject
    ZAnimObject_TextPulser(ZGUI::Style _style = {});

   virtual bool Paint();

   // cCEAnimObject_TextPulser
   void			SetText(const std::string& sText);
   void			SetPulse(int64_t nMinAlpha, int64_t nMaxAlpha, double fPeriod);

protected:
    std::string msText;
    ZGUI::Style mStyle;

    double		mfPeriod;
    int64_t		mnMinAlpha;
    int64_t		mnMaxAlpha;
};

// Transformer
class ZAnimObject_Transformer : public ZAnimObject
{
public:
	ZAnimObject_Transformer(ZTransformable* pTransformer);
	virtual ~ZAnimObject_Transformer();

	virtual bool  Paint();

protected:
	ZTransformable* mpTransformer;
};

// TransformingImage
class ZAnimObject_TransformingImage : public ZAnimObject, public ZTransformable
{
public:
	ZAnimObject_TransformingImage(tZBufferPtr pImage, ZRect* pArea = nullptr);
	virtual ~ZAnimObject_TransformingImage();

	virtual bool  Paint();

    tZBufferPtr mpImage;
};



class cShatterQuad
{
public:
	cShatterQuad() { Reset(); }

	void		Reset()
	{
		mVertices.resize(4);
		for (int i = 0; i < 4; i++)
		{
			mVertices[i].x = 0.0f;
			mVertices[i].y = 0.0f;
			mVertices[i].u = 0.0f;
			mVertices[i].v = 0.0f;
		}

		fdX = 0.0f;
		fdY	= 0.0f;
		fRotation = 0.0f;
		fdRotation = 0.0f;
		fScale = 1.0f;
		fdScale = 0.0f;
	}
	
	tUVVertexArray	mVertices;

	double		fdX;
	double		fdY;
	double		fRotation;
	double		fdRotation;

	double		fScale;
	double		fdScale;
};

typedef std::list<cShatterQuad> tShatterQuadList;

class ZAnimObject_BitmapShatterer : public ZAnimObject
{
public:
					ZAnimObject_BitmapShatterer();
   virtual			~ZAnimObject_BitmapShatterer();
   virtual bool		Paint();

   void				SetBitmapToShatter(ZBuffer* pBufferToShatter, ZRect& rSrc, ZRect& rStartingDst, int64_t nSubdivisions);

protected:
   void  			CreateShatterListFromBuffer(ZBuffer* pBuffer, ZRect& rSrc, int64_t nSubdivisions);
   void  			Process(ZRect& rBoundingArea);

   tShatterQuadList mShatterQuadList;
   tZBufferPtr		mpTexture;
   ZRasterizer	mRasterizer;
};


/*class ZAnimObject_BouncyLine : public ZAnimObject
{
public:

   // Can be a combination of these flags
   enum eDrawType
   {
      kNormal     = 0,
      kXMirror    = 1,
      kYMirror    = 2,
      kAAttractsB = 4,
      kBAttractsA = 8
   };

				ZAnimObject_BouncyLine();
   virtual		~ZAnimObject_BouncyLine();
   virtual bool	Paint(ZBuffer* pBufferToDrawTo, ZRect* pClip);

   void			SetParams(ZRect rExtents, int64_t nTailLength, uint8_t nType);

protected:
   void  		RandomizeLine();
   void  		Process();

   int64_t   	mnMaxTailLength;
   cFRect    	mrExtents;
   bool     	mbXMirror;
   bool     	mbYMirror;
   bool     	mbAAttractsB;
   bool     	mbBAttractsA;


   // Line Data

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
   int64_t mnR;
   int64_t mnG;
   int64_t mnB;
   
   int64_t mndR;
   int64_t mndG;
   int64_t mndB;
};
*/
