#pragma once

#include "ZBuffer.h"
#include "ZGraphicSystem.h"

#ifdef _WIN64
#include <DirectXMath.h>
struct Vertex {
    DirectX::XMFLOAT3 position; // Already in screen space (x, y, z)
    DirectX::XMFLOAT2 uv;       // Texture coordinates
};
#endif

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

    void    EndRender();


	// Visibility Related
	void	ResetVisibilityList() { mScreenRectList.clear(); }
	bool	AddScreenRectAndComputeVisibility(const ZScreenRect& screenRect);
	void	SetVisibilityComputingFlag(bool bSet) { mbVisibilityNeedsComputing = bSet; }
	bool	DoesVisibilityNeedComputing() { return mbVisibilityNeedsComputing; }
	size_t	GetVisibilityCount() { return mScreenRectList.size(); }

	int32_t	RenderVisibleRects();   // returns number of rects that needed rendering
    void    RenderScreenSpaceTriangle(ID3D11ShaderResourceView* textureSRV, const std::array<Vertex, 3>& triangleVertices);
//    int32_t RenderVisibleRectsToBuffer(ZBuffer* pDst, const ZRect& rClip);  // shouldn't be needed any longer since screenbuffer will collect all visible...this can just be a blt

    bool    RenderBuffer(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst);
    bool    PaintToSystem(const ZRect& rClip);

    bool    PaintToSystem();    // final transfer from internal surface

protected:
	ZGraphicSystem*		mpGraphicSystem;

	tScreenRectList		mScreenRectList;
    std::mutex          mScreenRectListMutex;
	bool				mbVisibilityNeedsComputing;
    bool                mbRenderingEnabled; 
    bool                mbCurrentlyRendering;

#ifdef _WIN64
    PAINTSTRUCT			mPS;
    HDC					mDC;

    ID3D11Texture2D* mDynamicTexture;
#endif
};

