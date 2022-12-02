#pragma once

#include "ZBuffer.h"
#include "ZGraphicSystem.h"

class ZScreenRect
{
public:
	ZScreenRect();
	ZScreenRect(tZBufferPtr pSourceBuffer, ZRect rDest, ZPoint sourcePt) 
	{ 
		mpSourceBuffer = pSourceBuffer; 
		mrDest = rDest;
		mSourcePt = sourcePt;
	}

	ZScreenRect(const ZScreenRect& rhs)
	{
		mpSourceBuffer = rhs.mpSourceBuffer;
		mrDest = rhs.mrDest;
		mSourcePt = rhs.mSourcePt;
	}

public:
    tZBufferPtr	mpSourceBuffer;
	ZRect		mrDest;
	ZPoint		mSourcePt;
};


typedef std::list<ZScreenRect> tScreenRectList;





class ZScreenBuffer : public ZBuffer
{
public:
	ZScreenBuffer();
	virtual ~ZScreenBuffer();
	bool	Init(int64_t nWidth, int64_t nHeight, ZGraphicSystem* pGraphicSystem);
	bool	Shutdown();

    void    EnableRendering(bool bEnable = true);   // for pausing rendering temporarily while creating/destroying surfaces

    void    BeginRender();

	void	Render(tZBufferPtr pTexture, ZRect& rAreaToDrawTo);

    void    EndRender();


	// Visibility Related
	void	ResetVisibilityList() { mScreenRectList.clear(); }
	bool	AddScreenRectAndComputeVisibility(const ZScreenRect& screenRect);
	void	SetVisibilityComputingFlag(bool bSet) { mbVisibilityNeedsComputing = bSet; }
	bool	DoesVisibilityNeedComputing() { return mbVisibilityNeedsComputing; }
	size_t	GetVisibilityCount() { return mScreenRectList.size(); }

	int32_t	RenderVisibleRects();   // returns number of rects that needed rendering

protected:
	ZGraphicSystem*		mpGraphicSystem;

	tScreenRectList		mScreenRectList;
	bool				mbVisibilityNeedsComputing;
    bool                mbRenderingEnabled; 
    bool                mbCurrentlyRendering;


#ifdef USE_D3D
	D3DSURFACE_DESC		mSurfaceDesc;
	D3DLOCKED_RECT		mLockedRect;
#endif

#ifdef _WIN64
    PAINTSTRUCT			mPS;
    HDC					mDC;
#endif
};

