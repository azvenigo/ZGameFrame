/////////////////////////////////////////////////////////////////////////////
// class cCEBuffer                    

#pragma once

#include "ZStdTypes.h"
#include "ZStdDebug.h"
#include <string>
#include <mutex>

#define ARGB_A(nCol) ((nCol&0xff000000)>>24)
#define ARGB_R(nCol) ((nCol&0x00ff0000)>>16)
#define ARGB_G(nCol) ((nCol&0x0000ff00)>>8)
#define ARGB_B(nCol) (nCol&0x000000ff)
#define ARGB(nA, nR, nG, nB) ((nA)<<24)|((nR)<<16)|((nG)<<8)|(nB)

class ZBuffer
{
public:
	enum eAlphaBlendType
	{
		kAlphaDest		= 0,
		kAlphaSource	= 1
	};

    enum eRenderFlags
    {
        kRenderFlag_None            = 0,
        kRenderFlag_ReadyToRender   = 1
    };

    enum eBltEdgeMiddleType
    {
        kEdgeBltMiddle_None         = 0,
        kEdgeBltMiddle_Tile         = 1,
        kEdgeBltMiddle_Stretch      = 2
    };

	ZBuffer();
    ZBuffer(const ZBuffer* pSrc);
	virtual ~ZBuffer();

	virtual bool            Init(int64_t nWidth, int64_t nHeight);
	virtual bool            Shutdown();
												
	virtual bool            Fill(ZRect& rDst, uint32_t nCol);			// fills a rect with nCol, forcing alpha to ARGB_A(nCol)
	virtual bool            FillAlpha(ZRect& rDst, uint32_t nCol);	// fills a rect with nCol, blending based on ARGB_A(nCol)


	virtual bool            CopyPixels(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, ZRect* pClip = NULL);		// like BLT but copies pixel values directly.... including alpha

	virtual bool            BltNoClip(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, eAlphaBlendType type = kAlphaDest);
	virtual bool            BltAlphaNoClip(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, uint32_t nAlpha, eAlphaBlendType type = kAlphaDest);
											
	virtual bool            Blt(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, ZRect* pClip = NULL, eAlphaBlendType type = kAlphaDest);
	virtual bool            BltAlpha(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, uint32_t nAlpha, ZRect* pClip = NULL, eAlphaBlendType type = kAlphaDest);

	virtual bool            BltTiled(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, int64_t nStartX, int64_t nStartY, ZRect* pClip = NULL, eAlphaBlendType type = kAlphaDest);
	virtual bool            BltEdge(ZBuffer* pSrc, ZRect& rEdgeRect, ZRect& rDst, eBltEdgeMiddleType middleType = kEdgeBltMiddle_Tile, ZRect* pClip = NULL, eAlphaBlendType type = kAlphaDest);

	virtual bool            BltRotated(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, double fAngle, double fScale, ZRect* pClip = NULL);

	virtual void            DrawAlphaLine(const ZColorVertex& v1, const ZColorVertex& v2, ZRect* pClip = NULL);

	virtual uint32_t        GetPixel(int64_t x, int64_t y);
	virtual void            SetPixel(int64_t x, int64_t y, uint32_t nCol);

	virtual ZRect&          GetArea() { return mSurfaceArea; }

	virtual bool            LoadBuffer(const std::string& sName);
#ifdef _WIN64
	virtual bool            LoadBuffer(uint32_t nResourceID);
#endif

	virtual uint32_t*       GetPixels() { return mpPixels; }

    virtual std::mutex&     GetMutex() { return mMutex; }
    virtual bool            GetRenderFlag(eRenderFlags flag) { return mRenderFlags & flag; }
    virtual void            SetRenderFlag(eRenderFlags flag) { mRenderFlags = mRenderFlags|flag; }
    virtual void            ClearRenderFlag(eRenderFlags flag) { mRenderFlags = mRenderFlags&~flag; }



	// Alpha Blending Functions
	static uint32_t        AddColors(uint32_t nCol1, uint32_t nCol2);
    static uint32_t        AlphaBlend_Col1Alpha(uint32_t nCol1, uint32_t nCol2, uint32_t nBlend);
    static uint32_t        AlphaBlend_Col2Alpha(uint32_t nCol1, uint32_t nCol2, uint32_t nBlend);
    static uint32_t        AlphaBlend_AddAlpha(uint32_t nCol1, uint32_t nCol2, uint32_t nBlend);
    static uint32_t        AlphaBlend_BlendAlpha(uint32_t nCol1, uint32_t nCol2, uint32_t nBlend);

    static bool            Clip(const ZRect& fullDstRect, ZRect& srcRect, ZRect& dstRect);
    static uint32_t        ConvertRGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b);
    static void            ConvertToRGB(uint32_t col, uint8_t& a, uint8_t& r, uint8_t& g, uint8_t& b);

protected:
	bool                    FloatScanLineIntersection(double fScanLine, const ZColorVertex& v1, const ZColorVertex& v2, double& fIntersection, double& fR, double& fG, double& fB, double& fA);
	void                    FillInSpan(uint32_t* pDest, int64_t nNumPixels, double fR, double fG, double fB, double fA);

	uint32_t*               mpPixels;        // The color data
	ZRect                   mSurfaceArea;
    std::mutex              mMutex;
    uint32_t                mRenderFlags;
};


inline uint32_t ZBuffer::AlphaBlend_Col1Alpha(uint32_t nCol1, uint32_t nCol2, uint32_t nBlend)
{
	nBlend = nBlend * ARGB_A(nCol1) >> 8;
	uint32_t nInverseAlpha = 255 - nBlend;

	// Use Alpha value for the destination color (nCol1)
	// Blend fAlpha of nCol1 and 1.0-fAlpha of nCol2
	return ARGB(
		ARGB_A(nCol1),
		((ARGB_R(nCol1) * nBlend + ARGB_R(nCol2) * nInverseAlpha)) >> 8,
		((ARGB_G(nCol1) * nBlend + ARGB_G(nCol2) * nInverseAlpha)) >> 8,
		((ARGB_B(nCol1) * nBlend + ARGB_B(nCol2) * nInverseAlpha)) >> 8
		);
}

inline uint32_t ZBuffer::AlphaBlend_Col2Alpha(uint32_t nCol1, uint32_t nCol2, uint32_t nBlend)
{
	nBlend = nBlend * ARGB_A(nCol1) >> 8;
	uint32_t nInverseAlpha = 255 - nBlend;

	// Use Alpha value for the destination color (nCol2)
	// Blend fAlpha of nCol1 and 1.0-fAlpha of nCol2
	return ARGB(
		ARGB_A(nCol2),	
		((ARGB_R(nCol1) * nBlend + ARGB_R(nCol2) * nInverseAlpha)) >> 8,
		((ARGB_G(nCol1) * nBlend + ARGB_G(nCol2) * nInverseAlpha)) >> 8,
		((ARGB_B(nCol1) * nBlend + ARGB_B(nCol2) * nInverseAlpha)) >> 8
		);
}

inline uint32_t ZBuffer::AlphaBlend_AddAlpha(uint32_t nCol1, uint32_t nCol2, uint32_t nBlend)
{
	nBlend = nBlend * ARGB_A(nCol1) >> 8;

	uint32_t nInverseAlpha = 255 - nBlend;

	uint32_t nFinalA = (ARGB_A(nCol1) + ARGB_A(nCol2));

	if (nFinalA > 0x000000ff)
		nFinalA = 0x000000ff;

	// Use Alpha value for the destination color (nCol2)
	// Blend fAlpha of nCol1 and 1.0-fAlpha of nCol2
	return ARGB(
		nFinalA,	
		((ARGB_R(nCol1) * nBlend + ARGB_R(nCol2) * nInverseAlpha)) >> 8,
		((ARGB_G(nCol1) * nBlend + ARGB_G(nCol2) * nInverseAlpha)) >> 8,
		((ARGB_B(nCol1) * nBlend + ARGB_B(nCol2) * nInverseAlpha)) >> 8
		);
}

inline uint32_t ZBuffer::AlphaBlend_BlendAlpha(uint32_t nCol1, uint32_t nCol2, uint32_t nBlend)
{
	nBlend = nBlend * ARGB_A(nCol1) >> 8;
	uint32_t nInverseAlpha = 255 - nBlend;

	// Use Alpha value for the destination color (nCol2)
	// Blend fAlpha of nCol1 and 1.0-fAlpha of nCol2
	return ARGB(
		((ARGB_A(nCol1) * nBlend + ARGB_A(nCol2) * nInverseAlpha)) >> 8,	
		((ARGB_R(nCol1) * nBlend + ARGB_R(nCol2) * nInverseAlpha)) >> 8,
		((ARGB_G(nCol1) * nBlend + ARGB_G(nCol2) * nInverseAlpha)) >> 8,
		((ARGB_B(nCol1) * nBlend + ARGB_B(nCol2) * nInverseAlpha)) >> 8
		);
}


inline uint32_t ZBuffer::AddColors(uint32_t nCol1, uint32_t nCol2)
{
	return ARGB(
		(uint8_t) (((ARGB_A(nCol1) + ARGB_A(nCol2))) ),	
		(uint8_t) (((ARGB_R(nCol1) + ARGB_R(nCol2))) ),
		(uint8_t) (((ARGB_G(nCol1) + ARGB_G(nCol2))) ),
		(uint8_t) (((ARGB_B(nCol1) + ARGB_B(nCol2))) )
		);

}

