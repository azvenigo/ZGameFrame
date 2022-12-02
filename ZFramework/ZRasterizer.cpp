#include "ZRasterizer.h"
#include "ZStdTypes.h"

uint64_t	ZRasterizer::mnProcessedVertices;	// for debugging
uint64_t	ZRasterizer::mnDrawnPixels;
uint64_t	ZRasterizer::mnTimeStamp;
uint64_t	ZRasterizer::mnPeakProcessedVertices;
uint64_t	ZRasterizer::mnPeakDrawnPixels;

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


bool ZRasterizer::FindScanlineIntersection(double fScanY, ZUVVertex& v1, ZUVVertex& v2, ZUVVertex& vIntersection)
{
	// if the segment is entirely above or below the scanline, no intersection
	if (v1.mY < fScanY && v2.mY < fScanY ||
		v1.mY > fScanY && v2.mY > fScanY)
		return false;

	// If the line is horizontal
	if (v1.mY == v2.mY)
		return false;

	vIntersection.mY = fScanY;

	// if the line is vertical
	if (v1.mX == v2.mX)
	{
		vIntersection.mX = v1.mX;
	}
	else
	{
		double fSlope = (double)((v1.mY - v2.mY)/(v1.mX - v2.mX));       //Calculate the intersection between the current
		vIntersection.mX = (v2.mX + (fScanY-v2.mY)/fSlope); //line and the current scan-line.
	}

	double fT = (vIntersection.mY - v1.mY)/(v2.mY - v1.mY);

	vIntersection.mU = v1.mU + (v2.mU - v1.mU)*fT;
	vIntersection.mV = v1.mV + (v2.mV - v1.mV)*fT;

	return true;
}

bool ZRasterizer::FindScanlineIntersection(double fScanY, ZColorVertex& v1, ZColorVertex& v2, ZColorVertex& vIntersection)
{
    // if the segment is entirely above or below the scanline, no intersection
    if (v1.mY < fScanY && v2.mY < fScanY ||
        v1.mY > fScanY && v2.mY > fScanY)
        return false;

    // If the line is horizontal
    if (v1.mY == v2.mY)
        return false;

    vIntersection.mY = fScanY;

    // if the line is vertical
    if (v1.mX == v2.mX)
    {
        vIntersection.mX = v1.mX;
    }
    else
    {
        double fSlope = (double)((v1.mY - v2.mY) / (v1.mX - v2.mX));       //Calculate the intersection between the current
        vIntersection.mX = (v2.mX + (fScanY - v2.mY) / fSlope); //line and the current scan-line.
    }

    double fT = (vIntersection.mY - v1.mY) / (v2.mY - v1.mY);

    double fA1 = (double)ARGB_A(v1.mColor);
    double fR1 = (double)ARGB_R(v1.mColor);
    double fG1 = (double)ARGB_G(v1.mColor);
    double fB1 = (double)ARGB_B(v1.mColor);

    double fA2 = (double)ARGB_A(v2.mColor);
    double fR2 = (double)ARGB_R(v2.mColor);
    double fG2 = (double)ARGB_G(v2.mColor);
    double fB2 = (double)ARGB_B(v2.mColor);

    double fA = fA1 + (fA2 - fA1) * fT;
    double fR = fR1 + (fR2 - fR1) * fT;
    double fG = fG1 + (fG2 - fG1) * fT;
    double fB = fB1 + (fB2 - fB1) * fT;

    vIntersection.mColor = ARGB((uint32_t)fA, (uint32_t)fR, (uint32_t)fG, (uint32_t)fB);

    return true;
}


inline void ZRasterizer::SetupRasterization(ZBuffer* pDestination, ZBuffer* pTexture, tColorVertexArray& vertexArray, ZRect& rDest, ZRect* pClip, double& fClipLeft, double& fClipRight, int64_t& nTopScanline, int64_t& nBottomScanline)
{
    ZASSERT(pDestination);
    ZASSERT(pTexture);

    if (pClip)
        rDest.IntersectRect(pClip);

    // Find the first and last scanline
    nTopScanline = rDest.bottom;			// start with top scan line at the bottom of our draw area
    nBottomScanline = rDest.top;			// start with bottom scan line at the top
    int64_t nNumVertices = vertexArray.size();

    for (int nVertexIndex = 0; nVertexIndex < nNumVertices; nVertexIndex++)
    {
        ZColorVertex& vertex = vertexArray[nVertexIndex];

        int64_t nY = (int64_t)(vertex.mY + 0.5);
        if (nY <= nTopScanline)
            nTopScanline = nY;
        if (nY > nBottomScanline)
            nBottomScanline = nY;
    }

    if (nTopScanline < rDest.top)
        nTopScanline = rDest.top;
    if (nBottomScanline > rDest.bottom)
        nBottomScanline = rDest.bottom;

    fClipLeft = (double)rDest.left;
    fClipRight = (double)rDest.right;
}


inline void ZRasterizer::SetupRasterization(ZBuffer* pDestination, ZBuffer* pTexture, tUVVertexArray& vertexArray, ZRect& rDest, ZRect* pClip, double& fClipLeft, double& fClipRight, int64_t& nTopScanline, int64_t& nBottomScanline)
{
	ZASSERT(pDestination);
	ZASSERT(pTexture);

	if (pClip)
		rDest.IntersectRect(pClip);

	// Find the first and last scanline
	nTopScanline = rDest.bottom;			// start with top scan line at the bottom of our draw area
	nBottomScanline = rDest.top;			// start with bottom scan line at the top
	int64_t nNumVertices = vertexArray.size();

	for (int nVertexIndex = 0; nVertexIndex < nNumVertices; nVertexIndex++)
	{
		ZUVVertex& vertex = vertexArray[nVertexIndex];

		int64_t nY = (int64_t) (vertex.mY + 0.5);
		if (nY <= nTopScanline)
			nTopScanline = nY;
		if (nY > nBottomScanline)
			nBottomScanline = nY;
	}

	if (nTopScanline < rDest.top)
		nTopScanline = rDest.top;
	if (nBottomScanline > rDest.bottom)
		nBottomScanline = rDest.bottom;

	fClipLeft = (double) rDest.left;
	fClipRight = (double) rDest.right;
}

inline void ZRasterizer::SetupScanline(double fScanLine, double& fClipLeft, double& fClipRight, ZUVVertex& scanLineMin, ZUVVertex& scanLineMax, tUVVertexArray& vertexArray, double& fScanLineLength, double& fTextureU, double& fTextureV, double& fTextureDX, double& fTextureDV)
{
	// Calculate min and max intersection on the scanline.
	scanLineMin.mX = fClipRight;
	scanLineMin.mY = fScanLine;

	scanLineMax.mX = fClipLeft;
	scanLineMax.mY = fScanLine;

	int64_t nNumVertices = vertexArray.size();

	// For each line segment
	for (int nVertexIndex = 0; nVertexIndex < nNumVertices; nVertexIndex++)
	{
		ZUVVertex& v1 = vertexArray[nVertexIndex];
		ZUVVertex v2;
		if (nVertexIndex < nNumVertices-1)
			v2 = vertexArray[nVertexIndex+1];
		else
			v2 = vertexArray[0];

		ZUVVertex vIntersection;
		if (FindScanlineIntersection(fScanLine, v1, v2, vIntersection))
		{
			// If the linesegment intersects the scanline && it's < curMin set the curMin vertex to the intersection
			// calculate how far along the linesegment the intersection, and lerp the texture coords along that linesegment
			if (vIntersection.mX < scanLineMin.mX)
				scanLineMin = vIntersection;
			// if the linesegment intersects the scanline && it's > curMax set the curMax vertex to the intersection
			// calculate how far along the linesegment the intersection and lerp the texture coords along that line segment
			if (vIntersection.mX > scanLineMax.mX)
				scanLineMax = vIntersection;
		}
		else if (v1.mY == v2.mY && (int64_t) v1.mY == (int64_t) fScanLine)
		{
			if (v1.mX < v2.mX)
			{
				scanLineMin = v1;
				scanLineMax = v2;
			}
			else
			{
				scanLineMin = v2;
				scanLineMax = v1;
			}
		}
	}

	// Clip left?
	if (scanLineMin.mX < fClipLeft)
	{
		double fAmountToClip = fClipLeft - scanLineMin.mX;
		double fT = (scanLineMax.mX - scanLineMin.mX)/fAmountToClip;

		scanLineMin.mX = fClipLeft;
		scanLineMin.mU = scanLineMin.mU + (scanLineMax.mU-scanLineMin.mU)/fT;
		scanLineMin.mV = scanLineMin.mV + (scanLineMax.mV-scanLineMin.mV)/fT;
	}

	// Clip right?
	if (scanLineMax.mX > fClipRight)
	{
		double fAmountToClip = scanLineMax.mX - fClipRight;
		double fT = (scanLineMax.mX - scanLineMin.mX)/fAmountToClip;

		scanLineMax.mX = fClipRight;
		scanLineMax.mU = scanLineMax.mU - (scanLineMax.mU-scanLineMin.mU)/fT;
		scanLineMax.mV = scanLineMax.mV - (scanLineMax.mV-scanLineMin.mV)/fT;
	}


	// Calculate the texture walker based on lerp from curMin to curMax
	fScanLineLength = ((double) scanLineMax.mX - (double) scanLineMin.mX);

	fTextureU = scanLineMin.mU;
	fTextureV = scanLineMin.mV;
	fTextureDX = (scanLineMax.mU - scanLineMin.mU)/fScanLineLength;
	fTextureDV = (scanLineMax.mV - scanLineMin.mV)/fScanLineLength;
}


inline void ZRasterizer::SetupScanline(double fScanLine, double& fClipLeft, double& fClipRight, ZColorVertex& scanLineMin, ZColorVertex& scanLineMax, tColorVertexArray& vertexArray, double& fScanLineLength, double& fA, double& fR, double& fG, double& fB, double& fDA, double& fDR, double& fDG, double& fDB)
{
    // Calculate min and max intersection on the scanline.
    scanLineMin.mX = fClipRight;
    scanLineMin.mY = fScanLine;

    scanLineMax.mX = fClipLeft;
    scanLineMax.mY = fScanLine;

    double fA1 = (double)ARGB_A(scanLineMin.mColor);
    double fR1 = (double)ARGB_R(scanLineMin.mColor);
    double fG1 = (double)ARGB_G(scanLineMin.mColor);
    double fB1 = (double)ARGB_B(scanLineMin.mColor);

    double fA2 = (double)ARGB_A(scanLineMax.mColor);
    double fR2 = (double)ARGB_R(scanLineMax.mColor);
    double fG2 = (double)ARGB_G(scanLineMax.mColor);
    double fB2 = (double)ARGB_B(scanLineMax.mColor);




    int64_t nNumVertices = vertexArray.size();

    // For each line segment
    for (int nVertexIndex = 0; nVertexIndex < nNumVertices; nVertexIndex++)
    {
        ZColorVertex& v1 = vertexArray[nVertexIndex];
        ZColorVertex v2;
        if (nVertexIndex < nNumVertices - 1)
            v2 = vertexArray[nVertexIndex + 1];
        else
            v2 = vertexArray[0];

        ZColorVertex vIntersection;
        if (FindScanlineIntersection(fScanLine, v1, v2, vIntersection))
        {
            // If the linesegment intersects the scanline && it's < curMin set the curMin vertex to the intersection
            // calculate how far along the linesegment the intersection, and lerp the texture coords along that linesegment
            if (vIntersection.mX < scanLineMin.mX)
                scanLineMin = vIntersection;
            // if the linesegment intersects the scanline && it's > curMax set the curMax vertex to the intersection
            // calculate how far along the linesegment the intersection and lerp the texture coords along that line segment
            if (vIntersection.mX > scanLineMax.mX)
                scanLineMax = vIntersection;
        }
        else if (v1.mY == v2.mY && (int64_t)v1.mY == (int64_t)fScanLine)
        {
            if (v1.mX < v2.mX)
            {
                scanLineMin = v1;
                scanLineMax = v2;
            }
            else
            {
                scanLineMin = v2;
                scanLineMax = v1;
            }
        }
    }

    // Clip left?
    if (scanLineMin.mX < fClipLeft)
    {
        double fAmountToClip = fClipLeft - scanLineMin.mX;
        double fT = (scanLineMax.mX - scanLineMin.mX) / fAmountToClip;

        scanLineMin.mX = fClipLeft;
//        scanLineMin.mU = scanLineMin.mU + (scanLineMax.mU - scanLineMin.mU) / fT;
//        scanLineMin.mV = scanLineMin.mV + (scanLineMax.mV - scanLineMin.mV) / fT;

        double fA = fA1 + (fA2 - fA1) / fT;
        double fR = fR1 + (fR2 - fR1) / fT;
        double fG = fG1 + (fG2 - fG1) / fT;
        double fB = fB1 + (fB2 - fB1) / fT;

        scanLineMin.mColor = ARGB((uint32_t)fA, (uint32_t)fR, (uint32_t)fG, (uint32_t)fB);
    }

    // Clip right?
    if (scanLineMax.mX > fClipRight)
    {
        double fAmountToClip = scanLineMax.mX - fClipRight;
        double fT = (scanLineMax.mX - scanLineMin.mX) / fAmountToClip;

        scanLineMax.mX = fClipRight;
//        scanLineMax.mU = scanLineMax.mU - (scanLineMax.mU - scanLineMin.mU) / fT;
//        scanLineMax.mV = scanLineMax.mV - (scanLineMax.mV - scanLineMin.mV) / fT;

        double fA = fA2 - (fA2 - fA1) / fT;
        double fR = fR2 - (fR2 - fR1) / fT;
        double fG = fG2 - (fG2 - fG1) / fT;
        double fB = fB2 - (fB2 - fB1) / fT;

        scanLineMax.mColor = ARGB((uint32_t)fA, (uint32_t)fR, (uint32_t)fG, (uint32_t)fB);

    }


    // Calculate the color walker based on lerp from curMin to curMax
    fScanLineLength = ((double)scanLineMax.mX - (double)scanLineMin.mX);

    //    fTextureU = scanLineMin.mU;
    //    fTextureV = scanLineMin.mV;
    fA = (double)ARGB_A(scanLineMin.mColor);
    fR = (double)ARGB_R(scanLineMin.mColor);
    fG = (double)ARGB_G(scanLineMin.mColor);
    fB = (double)ARGB_B(scanLineMin.mColor);

    //fTextureDX = (scanLineMax.mU - scanLineMin.mU) / fScanLineLength;
    //fTextureDV = (scanLineMax.mV - scanLineMin.mV) / fScanLineLength;
    fDA = ((double)ARGB_A(scanLineMax.mColor) - (double)ARGB_A(scanLineMin.mColor)) / fScanLineLength;
    fDR = ((double)ARGB_R(scanLineMax.mColor) - (double)ARGB_R(scanLineMin.mColor)) / fScanLineLength;
    fDG = ((double)ARGB_G(scanLineMax.mColor) - (double)ARGB_G(scanLineMin.mColor)) / fScanLineLength;
    fDB = ((double)ARGB_B(scanLineMax.mColor) - (double)ARGB_B(scanLineMin.mColor)) / fScanLineLength;
}


bool ZRasterizer::RasterizeWithAlpha(ZBuffer* pDestination, ZBuffer* pTexture, tUVVertexArray& vertexArray, ZRect* pClip, uint8_t nAlpha)
{
	ZRect rDest = pDestination->GetArea();
	int64_t nDestStride = pDestination->GetArea().Width();
	double fTextureW = (double) pTexture->GetArea().Width() - 0.5;
	double fTextureH = (double) pTexture->GetArea().Height() - 0.5;
	uint32_t* pSourcePixels = pTexture->GetPixels();
	int64_t nTextureStride = pTexture->GetArea().Width();
	double fClipLeft;
	double fClipRight;
	int64_t nTopScanLine;
	int64_t nBottomScanLine;

    ZASSERT(pDestination->GetPixels() != nullptr);
	SetupRasterization(pDestination, pTexture, vertexArray, rDest, pClip, fClipLeft, fClipRight, nTopScanLine, nBottomScanLine);

	// For each scanline
	for (int64_t nScanLine = nTopScanLine; nScanLine < nBottomScanLine; nScanLine++)
	{
		ZUVVertex scanLineMin;
		ZUVVertex scanLineMax;

		double fScanLineLength;

		double fTextureU;
		double fTextureV;
		double fTextureDX;
		double fTextureDV;

		SetupScanline((double) nScanLine, fClipLeft, fClipRight, scanLineMin, scanLineMax, vertexArray, fScanLineLength, fTextureU, fTextureV, fTextureDX, fTextureDV);

		// Rasterize the scanline using textureWalker as the source color
		int64_t nStartX = (int64_t) scanLineMin.mX;
		int64_t nScanLinePixels = (int64_t) scanLineMax.mX - (int64_t) scanLineMin.mX;
		uint32_t* pDestPixels = pDestination->GetPixels() + nScanLine*nDestStride + nStartX;


//		int64_t nRand = pDestination->GetArea().Width() * pDestination->GetArea().Height();

		ZASSERT(nStartX + nScanLinePixels <= rDest.right);

        ZASSERT(pDestination->GetPixels() != nullptr);

		for (int64_t nCount = 0; nCount < nScanLinePixels; nCount++)
		{
            int64_t nTextureX = (int64_t) (fTextureU * fTextureW);
			int64_t nTextureY = (int64_t) (fTextureV * fTextureH);
			uint32_t nSourceCol = *(pSourcePixels + nTextureY*nTextureStride + nTextureX);
			if (ARGB_A(nSourceCol) > 0)
				*pDestPixels = pDestination->AlphaBlend_Col2Alpha(nSourceCol, *pDestPixels, nAlpha);
			pDestPixels++;
			fTextureU += fTextureDX;
			fTextureV += fTextureDV;
		}

		mnDrawnPixels += nScanLinePixels;
	}

	mnProcessedVertices += vertexArray.size();
	return true;
}


bool ZRasterizer::Rasterize(ZBuffer* pDestination, ZBuffer* pTexture, tUVVertexArray& vertexArray, ZRect* pClip)
{
	ZRect rDest = pDestination->GetArea();
	int64_t nDestStride = pDestination->GetArea().Width();
	double fTextureW = (double) pTexture->GetArea().Width() - 0.5;
	double fTextureH = (double) pTexture->GetArea().Height() - 0.5;
	uint32_t* pSourcePixels = pTexture->GetPixels();
	int64_t nTextureStride = pTexture->GetArea().Width();
	double fClipLeft;
	double fClipRight;
	int64_t nTopScanLine;
	int64_t nBottomScanLine;

	SetupRasterization(pDestination, pTexture, vertexArray, rDest, pClip, fClipLeft, fClipRight, nTopScanLine, nBottomScanLine);

	// For each scanline
	for (int64_t nScanLine = nTopScanLine; nScanLine < nBottomScanLine; nScanLine++)
	{
		ZUVVertex scanLineMin;
		ZUVVertex scanLineMax;

		double fScanLineLength;

		double fTextureU;
		double fTextureV;
		double fTextureDX;
		double fTextureDV;

		SetupScanline((double) nScanLine, fClipLeft, fClipRight, scanLineMin, scanLineMax, vertexArray, fScanLineLength, fTextureU, fTextureV, fTextureDX, fTextureDV);

		// Rasterize the scanline using textureWalker as the source color
		int64_t nStartX = (int64_t) scanLineMin.mX;
		int64_t nScanLinePixels = (int64_t) scanLineMax.mX - (int64_t) scanLineMin.mX;
		uint32_t* pDestPixels = pDestination->GetPixels() + nScanLine*nDestStride + nStartX;

		ZASSERT(nStartX + nScanLinePixels <= rDest.right);

		for (int64_t nCount = 0; nCount < nScanLinePixels; nCount++)
		{
			int64_t nTextureX = (int64_t) (fTextureU * fTextureW);
			int64_t nTextureY = (int64_t) (fTextureV * fTextureH);
			uint32_t nSourceCol = *(pSourcePixels + nTextureY*nTextureStride + nTextureX);
			*pDestPixels = nSourceCol;
			pDestPixels++;

			fTextureU += fTextureDX;
			fTextureV += fTextureDV;
		}

		mnDrawnPixels += nScanLinePixels;
	}

	mnProcessedVertices += vertexArray.size();
	return true;
}

bool ZRasterizer::Rasterize(ZBuffer* pDestination, ZBuffer* pTexture, tColorVertexArray& vertexArray, ZRect* pClip)
{
    ZRect rDest = pDestination->GetArea();
    int64_t nDestStride = pDestination->GetArea().Width();
    double fClipLeft;
    double fClipRight;
    int64_t nTopScanLine;
    int64_t nBottomScanLine;

    SetupRasterization(pDestination, pTexture, vertexArray, rDest, pClip, fClipLeft, fClipRight, nTopScanLine, nBottomScanLine);

    // For each scanline
    for (int64_t nScanLine = nTopScanLine; nScanLine < nBottomScanLine; nScanLine++)
    {
        ZColorVertex scanLineMin;
        ZColorVertex scanLineMax;

        double fScanLineLength;
        double fA, fR, fG, fB;
        double fDA, fDR, fDG, fDB;

        SetupScanline((double)nScanLine, fClipLeft, fClipRight, scanLineMin, scanLineMax, vertexArray, fScanLineLength, fA, fR, fG, fB, fDA, fDR, fDG, fDB);

        // Rasterize the scanline using textureWalker as the source color
        int64_t nStartX = (int64_t)scanLineMin.mX;
        int64_t nScanLinePixels = (int64_t)scanLineMax.mX - (int64_t)scanLineMin.mX;
        uint32_t* pDestPixels = pDestination->GetPixels() + nScanLine * nDestStride + nStartX;

        ZASSERT(nStartX + nScanLinePixels <= rDest.right);

        for (int64_t nCount = 0; nCount < nScanLinePixels; nCount++)
        {
            *pDestPixels = ARGB((uint32_t) fA, (uint32_t) fR, (uint32_t) fG, (uint32_t) fB);
            pDestPixels++;

            fA += fDA;
            fR += fDR;
            fG += fDG;
            fB += fDB;
        }

        mnDrawnPixels += nScanLinePixels;
    }

    mnProcessedVertices += vertexArray.size();
    return true;
}


ZRect ZRasterizer::GetBoundingRect(tUVVertexArray& vertexArray)
{
	ZRect rBounds(MAXINT64, MAXINT64, MININT64, MININT64);
	for (uint32_t nIndex = 0; nIndex < vertexArray.size(); nIndex++)
	{
		int64_t nVertX = (int64_t) vertexArray[nIndex].mX;
		int64_t nVertY = (int64_t) vertexArray[nIndex].mY;
		if (rBounds.left > nVertX)
			rBounds.left = nVertX;
		if (rBounds.right < nVertX)
			rBounds.right = nVertX;
		if (rBounds.top > nVertY)
			rBounds.top = nVertY;
		if (rBounds.bottom < nVertY)
			rBounds.bottom = nVertY;
	}

	return rBounds;
}

void ZRasterizer::RectToVerts(const ZRect& r, tUVVertexArray& vertexArray)
{
    vertexArray.resize(4);

    vertexArray[0].mX = (double) r.left;
    vertexArray[0].mY = (double) r.top;
    vertexArray[0].mU = 0.0;
    vertexArray[0].mV = 0.0;


    vertexArray[1].mX = (double) r.right;
    vertexArray[1].mY = (double) r.top;
    vertexArray[1].mU = 1.0;
    vertexArray[1].mV = 0.0;

    vertexArray[2].mX = (double) r.right;
    vertexArray[2].mY = (double) r.bottom;
    vertexArray[2].mU = 1.0;
    vertexArray[2].mV = 1.0;

    vertexArray[3].mX = (double) r.left;
    vertexArray[3].mY = (double) r.bottom;
    vertexArray[3].mU = 0.0;
    vertexArray[3].mV = 1.0;
}
void ZRasterizer::RectToVerts(const ZRect& r, tColorVertexArray& vertexArray)
{
    vertexArray.resize(4);

    vertexArray[0].mX = (double)r.left;
    vertexArray[0].mY = (double)r.top;

    vertexArray[1].mX = (double)r.right;
    vertexArray[1].mY = (double)r.top;

    vertexArray[2].mX = (double)r.right;
    vertexArray[2].mY = (double)r.bottom;

    vertexArray[3].mX = (double)r.left;
    vertexArray[3].mY = (double)r.bottom;
}
