#pragma once

#include "ZBuffer.h"
#include "ZGraphicSystem.h"
#include "ZD3D.h"


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


#pragma pack()  // Reset to default alignment

class ZScreenBuffer : public ZBuffer
{
public:
	ZScreenBuffer();
	virtual ~ZScreenBuffer();
	bool	Init(int64_t nWidth, int64_t nHeight, ZGraphicSystem* pGraphicSystem);
	bool	Shutdown();

    void    EnableRendering(bool bEnable = true);   // for pausing rendering temporarily while creating/destroying surfaces

    void    BeginRender();

    void    EndRender();


	// Visibility Related
	void	ResetVisibilityList() { mScreenRectList.clear(); }
	bool	AddScreenRectAndComputeVisibility(const ZScreenRect& screenRect);
    void	SetVisibilityComputingFlag(bool bSet);
	bool	DoesVisibilityNeedComputing() { return mbVisibilityNeedsComputing; }
	size_t	GetVisibilityCount() { return mScreenRectList.size(); }

	int32_t	RenderVisibleRects();   // returns number of rects that needed rendering

    bool    RenderBuffer(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst);
    bool    PaintToSystem(const ZRect& rClip);

    bool    PaintToSystem();    // final transfer from internal surface

protected:
	ZGraphicSystem*     mpGraphicSystem;

	tScreenRectList     mScreenRectList;
    std::mutex          mScreenRectListMutex;
	bool                mbVisibilityNeedsComputing;
    bool                mbRenderingEnabled; 
    bool                mbCurrentlyRendering;

#ifdef _WIN64
    PAINTSTRUCT         mPS;
    HDC                 mDC;

    ZD3D::ScreenSpacePrimitive* mpSSPrim;
#endif
};

