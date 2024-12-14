#pragma once

#include "ZBuffer.h"
#include "ZTypes.h"
#include "ZDebug.h"
#include "helpers/ThreadPool.h"
#include <vector>


class ZRasterizer
{
public:
	bool    Rasterize(ZBuffer* pDestination, ZBuffer* pTexture, tUVVertexArray& vertexArray, ZRect* pClip = NULL);
    bool    Rasterize(ZBuffer* pDestination, tColorVertexArray& vertexArray, ZRect* pClip = NULL);
   
    bool    RasterizeWithAlpha(ZBuffer* pDestination, ZBuffer* pTexture, tUVVertexArray& vertexArray, ZRect* pClip = NULL, uint8_t nAlpha = 255);
    bool    MultiSampleRasterizeWithAlpha(ZBuffer* pDestination, ZBuffer* pTexture, tUVVertexArray& vertexArray, ZRect* pClip, uint32_t nSubsamples, uint8_t nAlpha = 255);

    // helper functions
    bool    RasterizeSimple(ZBuffer* pDestination, ZBuffer* pTexture, ZRect rDest, ZRect rSrc, ZRect* pClip = NULL);
    bool    RasterizeWithAlphaSimple(ZBuffer* pDestination, ZBuffer* pTexture, ZRect rDest, ZRect rSrc, ZRect* pClip = NULL, uint8_t nAlpha = 255);
    ZRect   GetBoundingRect(tUVVertexArray& vertexArray);
    void    RectToVerts(const ZRect& r, tUVVertexArray& vertexArray);     // returns the default vertex array with LTRB and UVs at 0.0-1.0
    void    RectToVerts(const ZRect& r, tColorVertexArray& vertexArray);     // returns the default vertex array with LTRB

    static uint32_t SampleTexture_ZoomedIn(ZBuffer* pTexture, double fTexturePixelU, double fTexturePixelV, double fTexturePixelDU, double fTexturePixelDV, int32_t nSampleSubdivisions);
    static uint32_t SampleTexture_ZoomedOut(ZBuffer* pTexture, double fTexturePixelU, double fTexturePixelV, double fTexturePixelDU, double fTexturePixelDV, int32_t nSampleSubdivisions);
private:
    // UV

    static bool    FindScanlineIntersection(double fScanY, ZUVVertex& v1, ZUVVertex& v2, ZUVVertex& vIntersection);
    static void    SetupRasterization(ZBuffer* pDestination, tUVVertexArray& vertexArray, ZRect& rDest, ZRect* pClip, double& fClipLeft, double& fClipRight, int64_t& nTopScanline, int64_t& nBottomScanline);
    static void    SetupScanline(double fScanLine, double& fClipLeft, double& fClipRight, ZUVVertex& scanLineMin, ZUVVertex& scanLineMax, tUVVertexArray& vertexArray, double& fScanLineLength, double& fTextureU, double& fTextureV, double& fTextureDX, double& fTextureDV);

    // Color
    bool    FindScanlineIntersection(double fScanY, ZColorVertex& v1, ZColorVertex& v2, ZColorVertex& vIntersection);
    void    SetupRasterization(ZBuffer* pDestination, tColorVertexArray& vertexArray, ZRect& rDest, ZRect* pClip, double& fClipLeft, double& fClipRight, int64_t& nTopScanline, int64_t& nBottomScanline);
    void    SetupScanline(double fScanLine, double& fClipLeft, double& fClipRight, ZColorVertex& scanLineMin, ZColorVertex& scanLineMax, tColorVertexArray& vertexArray, double& fScanLineLength, double& fA, double& fR, double& fG, double& fB, double& fDA, double& fDR, double& fDG, double& fDB);

    static bool MultiSampleRasterizeRange(ZBuffer* pTexture, ZBuffer* pDestination, int64_t nTop, int64_t nBottom, double fClipLeft, double fClipRight, tUVVertexArray& vertexArray, bool isZoomedIn, uint32_t nSubsamples, uint8_t nAlpha);


public:

	static uint64_t	mnProcessedVertices;	// for debugging
	static uint64_t	mnDrawnPixels;
	static uint64_t	mnTimeStamp;

	static uint64_t	mnPeakProcessedVertices;
	static uint64_t	mnPeakDrawnPixels;


    ThreadPool  renderPool;
};

extern ZRasterizer gRasterizer;