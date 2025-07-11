#include "ZRasterizer.h"
#include "ZTypes.h"

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
	if (v1.y < fScanY && v2.y < fScanY ||
		v1.y > fScanY && v2.y > fScanY)
		return false;

	// If the line is horizontal
	if (v1.y == v2.y)
		return false;

	vIntersection.y = fScanY;

	// if the line is vertical
	if (v1.x == v2.x)
	{
		vIntersection.x = v1.x;
	}
	else
	{
		double fSlope = (double)((v1.y - v2.y)/(v1.x - v2.x));       //Calculate the intersection between the current
		vIntersection.x = (v2.x + (fScanY-v2.y)/fSlope); //line and the current scan-line.
	}

	double fT = (vIntersection.y - v1.y)/(v2.y - v1.y);

	vIntersection.u = v1.u + (v2.u - v1.u)*fT;
	vIntersection.v = v1.v + (v2.v - v1.v)*fT;

	return true;
}

bool ZRasterizer::FindScanlineIntersection(double fScanY, ZColorVertex& v1, ZColorVertex& v2, ZColorVertex& vIntersection)
{
    // if the segment is entirely above or below the scanline, no intersection
    if (v1.y < fScanY && v2.y < fScanY ||
        v1.y > fScanY && v2.y > fScanY)
        return false;

    // If the line is horizontal
    if (v1.y == v2.y)
        return false;

    vIntersection.y = fScanY;

    // if the line is vertical
    if (v1.x == v2.x)
    {
        vIntersection.x = v1.x;
    }
    else
    {
        double fSlope = (double)((v1.y - v2.y) / (v1.x - v2.x));       //Calculate the intersection between the current
        vIntersection.x = (v2.x + (fScanY - v2.y) / fSlope); //line and the current scan-line.
    }

    double fT = (vIntersection.y - v1.y) / (v2.y - v1.y);

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


inline void ZRasterizer::SetupRasterization(ZBuffer* pDestination, tColorVertexArray& vertexArray, ZRect& rDest, ZRect* pClip, double& fClipLeft, double& fClipRight, int64_t& nTopScanline, int64_t& nBottomScanline)
{
    ZASSERT(pDestination);

    if (pClip)
        rDest.Intersect(pClip);

    // Find the first and last scanline
    nTopScanline = rDest.bottom;			// start with top scan line at the bottom of our draw area
    nBottomScanline = rDest.top;			// start with bottom scan line at the top
    int64_t nNumVertices = vertexArray.size();

    for (int nVertexIndex = 0; nVertexIndex < nNumVertices; nVertexIndex++)
    {
        ZColorVertex& vertex = vertexArray[nVertexIndex];

        int64_t nY = (int64_t)(vertex.y + 0.5);
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


inline void ZRasterizer::SetupRasterization(ZBuffer* pDestination, tUVVertexArray& vertexArray, ZRect& rDest, ZRect* pClip, double& fClipLeft, double& fClipRight, int64_t& nTopScanline, int64_t& nBottomScanline)
{
	ZASSERT(pDestination);

	if (pClip)
		rDest.Intersect(pClip);

	// Find the first and last scanline
	nTopScanline = rDest.bottom;			// start with top scan line at the bottom of our draw area
	nBottomScanline = rDest.top;			// start with bottom scan line at the top
	int64_t nNumVertices = vertexArray.size();

	for (int nVertexIndex = 0; nVertexIndex < nNumVertices; nVertexIndex++)
	{
		ZUVVertex& vertex = vertexArray[nVertexIndex];

		int64_t nY = (int64_t) (vertex.y + 0.5);
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
	scanLineMin.x = fClipRight;
	scanLineMin.y = fScanLine;

	scanLineMax.x = fClipLeft;
	scanLineMax.y = fScanLine;

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
			if (vIntersection.x < scanLineMin.x)
				scanLineMin = vIntersection;
			// if the linesegment intersects the scanline && it's > curMax set the curMax vertex to the intersection
			// calculate how far along the linesegment the intersection and lerp the texture coords along that line segment
			if (vIntersection.x > scanLineMax.x)
				scanLineMax = vIntersection;
		}
		else if (v1.y == v2.y && (int64_t) v1.y == (int64_t) fScanLine)
		{
			if (v1.x < v2.x)
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
	if (scanLineMin.x < fClipLeft)
	{
		double fAmountToClip = fClipLeft - scanLineMin.x;
		double fT = (scanLineMax.x - scanLineMin.x)/fAmountToClip;

		scanLineMin.x = fClipLeft;
		scanLineMin.u = scanLineMin.u + (scanLineMax.u-scanLineMin.u)/fT;
		scanLineMin.v = scanLineMin.v + (scanLineMax.v-scanLineMin.v)/fT;
	}

	// Clip right?
	if (scanLineMax.x > fClipRight)
	{
		double fAmountToClip = scanLineMax.x - fClipRight;
		double fT = (scanLineMax.x - scanLineMin.x)/fAmountToClip;

		scanLineMax.x = fClipRight;
		scanLineMax.u = scanLineMax.u - (scanLineMax.u-scanLineMin.u)/fT;
		scanLineMax.v = scanLineMax.v - (scanLineMax.v-scanLineMin.v)/fT;
	}


	// Calculate the texture walker based on lerp from curMin to curMax
	fScanLineLength = ((double) scanLineMax.x - (double) scanLineMin.x);

	fTextureU = scanLineMin.u;
	fTextureV = scanLineMin.v;
	fTextureDX = (scanLineMax.u - scanLineMin.u)/fScanLineLength;
	fTextureDV = (scanLineMax.v - scanLineMin.v)/fScanLineLength;
}


inline void ZRasterizer::SetupScanline(double fScanLine, double& fClipLeft, double& fClipRight, ZColorVertex& scanLineMin, ZColorVertex& scanLineMax, tColorVertexArray& vertexArray, double& fScanLineLength, double& fA, double& fR, double& fG, double& fB, double& fDA, double& fDR, double& fDG, double& fDB)
{
    // Calculate min and max intersection on the scanline.
    scanLineMin.x = fClipRight;
    scanLineMin.y = fScanLine;

    scanLineMax.x = fClipLeft;
    scanLineMax.y = fScanLine;

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
            if (vIntersection.x < scanLineMin.x)
                scanLineMin = vIntersection;
            // if the linesegment intersects the scanline && it's > curMax set the curMax vertex to the intersection
            // calculate how far along the linesegment the intersection and lerp the texture coords along that line segment
            if (vIntersection.x > scanLineMax.x)
                scanLineMax = vIntersection;
        }
        else if (v1.y == v2.y && (int64_t)v1.y == (int64_t)fScanLine)
        {
            if (v1.x < v2.x)
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
    if (scanLineMin.x < fClipLeft)
    {
        double fAmountToClip = fClipLeft - scanLineMin.x;
        double fT = (scanLineMax.x - scanLineMin.x) / fAmountToClip;

        scanLineMin.x = fClipLeft;
//        scanLineMin.u = scanLineMin.u + (scanLineMax.u - scanLineMin.u) / fT;
//        scanLineMin.v = scanLineMin.v + (scanLineMax.v - scanLineMin.v) / fT;

        double fA = fA1 + (fA2 - fA1) / fT;
        double fR = fR1 + (fR2 - fR1) / fT;
        double fG = fG1 + (fG2 - fG1) / fT;
        double fB = fB1 + (fB2 - fB1) / fT;

        scanLineMin.mColor = ARGB((uint32_t)fA, (uint32_t)fR, (uint32_t)fG, (uint32_t)fB);
    }

    // Clip right?
    if (scanLineMax.x > fClipRight)
    {
        double fAmountToClip = scanLineMax.x - fClipRight;
        double fT = (scanLineMax.x - scanLineMin.x) / fAmountToClip;

        scanLineMax.x = fClipRight;
//        scanLineMax.u = scanLineMax.u - (scanLineMax.u - scanLineMin.u) / fT;
//        scanLineMax.v = scanLineMax.v - (scanLineMax.v - scanLineMin.v) / fT;

        double fA = fA2 - (fA2 - fA1) / fT;
        double fR = fR2 - (fR2 - fR1) / fT;
        double fG = fG2 - (fG2 - fG1) / fT;
        double fB = fB2 - (fB2 - fB1) / fT;

        scanLineMax.mColor = ARGB((uint32_t)fA, (uint32_t)fR, (uint32_t)fG, (uint32_t)fB);

    }


    // Calculate the color walker based on lerp from curMin to curMax
    fScanLineLength = ((double)scanLineMax.x - (double)scanLineMin.x);

    //    fTextureU = scanLineMin.u;
    //    fTextureV = scanLineMin.v;
    fA = (double)ARGB_A(scanLineMin.mColor);
    fR = (double)ARGB_R(scanLineMin.mColor);
    fG = (double)ARGB_G(scanLineMin.mColor);
    fB = (double)ARGB_B(scanLineMin.mColor);

    //fTextureDX = (scanLineMax.u - scanLineMin.u) / fScanLineLength;
    //fTextureDV = (scanLineMax.v - scanLineMin.v) / fScanLineLength;
    fDA = ((double)ARGB_A(scanLineMax.mColor) - (double)ARGB_A(scanLineMin.mColor)) / fScanLineLength;
    fDR = ((double)ARGB_R(scanLineMax.mColor) - (double)ARGB_R(scanLineMin.mColor)) / fScanLineLength;
    fDG = ((double)ARGB_G(scanLineMax.mColor) - (double)ARGB_G(scanLineMin.mColor)) / fScanLineLength;
    fDB = ((double)ARGB_B(scanLineMax.mColor) - (double)ARGB_B(scanLineMin.mColor)) / fScanLineLength;
}


bool ZRasterizer::RasterizeWithAlpha(ZBuffer* pDestination, ZBuffer* pTexture, tUVVertexArray& vertexArray, ZRect* pClip, uint8_t nAlpha)
{
    if (nAlpha < 8)
        return true;

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
	SetupRasterization(pDestination, vertexArray, rDest, pClip, fClipLeft, fClipRight, nTopScanLine, nBottomScanLine);

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
		int64_t nStartX = (int64_t) scanLineMin.x;
		int64_t nScanLinePixels = (int64_t) scanLineMax.x - (int64_t) scanLineMin.x;
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
				*pDestPixels = COL::AlphaBlend_Col2Alpha(nSourceCol, *pDestPixels, nAlpha);
			pDestPixels++;
			fTextureU += fTextureDX;
			fTextureV += fTextureDV;
		}

		mnDrawnPixels += nScanLinePixels;
	}

	mnProcessedVertices += vertexArray.size();
	return true;
}




uint32_t ZRasterizer::SampleTexture_ZoomedIn(ZBuffer* pTexture, double fTexturePixelU, double fTexturePixelV, double fTexturePixelDU, double fTexturePixelDV, uint32_t nSampleSubdivisions)
{
    uint32_t* pSourcePixels = pTexture->GetPixels();
    int64_t nTextureStride = pTexture->GetArea().Width();
    double fTextureW = (double)pTexture->GetArea().Width() - 1.0;
    double fTextureH = (double)pTexture->GetArea().Height() - 1.0;








    double fSubPixelDU = (1.0 / fTextureW) / (double)nSampleSubdivisions;
    double fSubPixelDV = (1.0 / fTextureH) / (double)nSampleSubdivisions;

    uint64_t nFinalA = 0;
    uint64_t nFinalR = 0;
    uint64_t nFinalG = 0;
    uint64_t nFinalB = 0;

    double fSampleV = fTexturePixelV;
    for (uint32_t y = 0; y < nSampleSubdivisions; y++)



    {
        double fSampleU = fTexturePixelU;
        for (uint32_t x = 0; x < nSampleSubdivisions; x++)
        {





            int64_t nTextureX = (int64_t)(fSampleU * fTextureW);
            int64_t nTextureY = (int64_t)(fSampleV * fTextureH);


            uint32_t nSourceCol = *(pSourcePixels + nTextureY * nTextureStride + nTextureX);


            nFinalA += ARGB_A(nSourceCol);
            nFinalR += ARGB_R(nSourceCol);
            nFinalG += ARGB_G(nSourceCol);
            nFinalB += ARGB_B(nSourceCol);

            fSampleU += fSubPixelDV;
        }
        fSampleV += fSubPixelDV;
    }


    uint32_t nSamples = nSampleSubdivisions * nSampleSubdivisions;
    uint32_t nFinalCol = ARGB((uint32_t)(nFinalA / nSamples), (uint32_t)(nFinalR / nSamples), (uint32_t)(nFinalG / nSamples), (uint32_t)(nFinalB / nSamples));






    return nFinalCol;
}

uint32_t ZRasterizer::SampleTexture_ZoomedOut(ZBuffer* pTexture, double fTexturePixelU, double fTexturePixelV, double fTexturePixelDU, double fTexturePixelDV, uint32_t nSampleSubdivisions)
{
    uint32_t* pSourcePixels = pTexture->GetPixels();
    int64_t nTextureStride = pTexture->GetArea().Width();
    double fTextureW = (double)pTexture->GetArea().Width()-1.0;
    double fTextureH = (double)pTexture->GetArea().Height()-1.0;


    double fSubDU = fTexturePixelDU / (double)nSampleSubdivisions;
    double fSubDV = fTexturePixelDV / (double)nSampleSubdivisions;

    double fSubDUy = -fSubDV;
    double fsubDVy = fSubDU;


    uint64_t nFinalA = 0;
    uint64_t nFinalR = 0;
    uint64_t nFinalG = 0;
    uint64_t nFinalB = 0;

    double minIndex = (-(double)nSampleSubdivisions)/2.0;
    double maxIndex = (double)nSampleSubdivisions/2.0;

    // Loop over the subdivisions to accumulate samples
    uint32_t nSamples = 0;
    for (double y = minIndex; y < maxIndex; y += 1.0)
    {
        for (double x = minIndex; x < maxIndex; x += 1.0)
        {
            // Compute the UV offsets for the current sample
//            double fSampleU = fTexturePixelU + x * fSubDU + y * fSubDV;
//            double fSampleV = fTexturePixelV + x * fSubDUy + y * fsubDVy;

            double fSampleU = fTexturePixelU + x * fSubDU;
            double fSampleV = fTexturePixelV + y * fSubDV;

            if (fSampleU < 0.0 || fSampleU > 1.0 || fSampleV < 0.0 || fSampleV > 1.0)
                continue;

            // Convert UV to texture coordinates (integer pixel coordinates)
            int64_t nTextureX = (int64_t)(fSampleU * fTextureW);
            int64_t nTextureY = (int64_t)(fSampleV * fTextureH);

            // Fetch the pixel color from the texture
            uint32_t nSourceCol = *(pSourcePixels + nTextureY * nTextureStride + nTextureX);

            // Accumulate the ARGB values
            nFinalA += ARGB_A(nSourceCol);
            nFinalR += ARGB_R(nSourceCol);
            nFinalG += ARGB_G(nSourceCol);
            nFinalB += ARGB_B(nSourceCol);
            nSamples++;
        }
    }

    // Compute the average color from all samples
    uint32_t nFinalCol = ARGB(
        (uint32_t)(nFinalA / nSamples),
        (uint32_t)(nFinalR / nSamples),
        (uint32_t)(nFinalG / nSamples),
        (uint32_t)(nFinalB / nSamples)
    );

    return nFinalCol;
}

bool ZRasterizer::MultiSampleRasterizeRange(ZBuffer* pTexture, ZBuffer* pDestination, int64_t nTop, int64_t nBottom, double fClipLeft, double fClipRight, tUVVertexArray& vertexArray, bool isZoomedIn, uint32_t nSubsamples, uint8_t nAlpha)
{
    // For each scanline
    for (int64_t nScanLine = nTop; nScanLine < nBottom; nScanLine++)
    {
        ZUVVertex scanLineMin;
        ZUVVertex scanLineMax;

        double fScanLineLength;

        double fTextureU;
        double fTextureV;
        double fTextureDU;
        double fTextureDV;

        SetupScanline((double)nScanLine, fClipLeft, fClipRight, scanLineMin, scanLineMax, vertexArray, fScanLineLength, fTextureU, fTextureV, fTextureDU, fTextureDV);

        // Rasterize the scanline using textureWalker as the source color
        int64_t nStartX = (int64_t)scanLineMin.x;
        int64_t nScanLinePixels = (int64_t)scanLineMax.x - (int64_t)scanLineMin.x;
        int64_t nDestStride = pDestination->GetArea().Width();
        uint32_t* pDestPixels = pDestination->GetPixels() + nScanLine * nDestStride + nStartX;


        //		int64_t nRand = pDestination->GetArea().Width() * pDestination->GetArea().Height();

        //ZASSERT(nStartX + nScanLinePixels <= rDest.right);

        ZASSERT(pDestination->GetPixels() != nullptr);


        if (isZoomedIn)
        {
            for (int64_t nCount = 0; nCount < nScanLinePixels; nCount++)
            {
                uint32_t nSampled = SampleTexture_ZoomedIn(pTexture, fTextureU, fTextureV, fTextureDU, fTextureDV, nSubsamples);
                if (ARGB_A(nSampled) > 0)
                    *pDestPixels = COL::AlphaBlend_Col2Alpha(nSampled, *pDestPixels, nAlpha);
                pDestPixels++;
                fTextureU += fTextureDU;
                fTextureV += fTextureDV;
            }
        }
        else
        {
            for (int64_t nCount = 0; nCount < nScanLinePixels; nCount++)
            {
                uint32_t nSampled = SampleTexture_ZoomedOut(pTexture, fTextureU, fTextureV, fTextureDU, fTextureDV, nSubsamples);
                if (ARGB_A(nSampled) > 0)
                    *pDestPixels = COL::AlphaBlend_Col2Alpha(nSampled, *pDestPixels, nAlpha);
                pDestPixels++;
                fTextureU += fTextureDU;
                fTextureV += fTextureDV;
            }
        }

        mnDrawnPixels += nScanLinePixels;
    }

    return true;
}

bool ZRasterizer::MultiSampleRasterizeWithAlpha(ZBuffer* pDestination, ZBuffer* pTexture, tUVVertexArray& vertexArray, ZRect* pClip, uint32_t nSubsamples, uint8_t nAlpha)
{
    if (nAlpha < 8)
        return true;

    ZRect rDest = pDestination->GetArea();
    int64_t nDestStride = pDestination->GetArea().Width();
    double fTextureW = (double)pTexture->GetArea().Width() - 0.5;
    double fTextureH = (double)pTexture->GetArea().Height() - 0.5;
    uint32_t* pSourcePixels = pTexture->GetPixels();
    int64_t nTextureStride = pTexture->GetArea().Width();
    double fClipLeft;
    double fClipRight;
    int64_t nTopScanLine;
    int64_t nBottomScanLine;

    ZASSERT(pDestination->GetPixels() != nullptr);
    SetupRasterization(pDestination, vertexArray, rDest, pClip, fClipLeft, fClipRight, nTopScanLine, nBottomScanLine);


    float screenDist = (float)sqrt(
        pow(vertexArray[0].x - vertexArray[1].x, 2) +
        pow(vertexArray[0].y - vertexArray[1].y, 2)
    );

    float textureDist = (float)fTextureW * (float)sqrt(
        pow(vertexArray[0].u - vertexArray[1].u, 2) +
        pow(vertexArray[0].v - vertexArray[1].v, 2)
    );

    // Determine if we're zoomed in or zoomed out
    bool isZoomedIn = (screenDist > textureDist);

    int64_t threadCount = renderPool.size();
    int64_t scanLinesPerThread = (nBottomScanLine - nTopScanLine) / threadCount;

    std::vector< std::shared_future<bool> > threadRenderResults;

    for (uint32_t i = 0; i < threadCount; i++)
    {
        int64_t nTop = nTopScanLine + i * scanLinesPerThread;
        int64_t nBottom = nTop + scanLinesPerThread;

        // last thread? take the rest of the scanlines
        if (i == threadCount - 1)
            nBottom = nBottomScanLine;

        ZASSERT(nBottom <= pDestination->GetArea().bottom);
        threadRenderResults.emplace_back(renderPool.enqueue(&MultiSampleRasterizeRange, pTexture, pDestination, nTop, nBottom, fClipLeft, fClipRight, vertexArray, isZoomedIn, nSubsamples, nAlpha));
    }

    for (const auto& result : threadRenderResults)
    {
        result.get();
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

	SetupRasterization(pDestination, vertexArray, rDest, pClip, fClipLeft, fClipRight, nTopScanLine, nBottomScanLine);

	// For each scanline
	for (int64_t nScanLine = nTopScanLine; nScanLine < nBottomScanLine; nScanLine++)
	{
		ZUVVertex scanLineMin;
		ZUVVertex scanLineMax;

		double fScanLineLength;

		double fTextureU;
		double fTextureV;
		double fTextureDU;
		double fTextureDV;

		SetupScanline((double) nScanLine, fClipLeft, fClipRight, scanLineMin, scanLineMax, vertexArray, fScanLineLength, fTextureU, fTextureV, fTextureDU, fTextureDV);

		// Rasterize the scanline using textureWalker as the source color
		int64_t nStartX = (int64_t) scanLineMin.x;
		int64_t nScanLinePixels = (int64_t) scanLineMax.x - (int64_t) scanLineMin.x;
		uint32_t* pDestPixels = pDestination->GetPixels() + nScanLine*nDestStride + nStartX;

		ZASSERT(nStartX + nScanLinePixels <= rDest.right);

		for (int64_t nCount = 0; nCount < nScanLinePixels; nCount++)
		{
			int64_t nTextureX = (int64_t) (fTextureU * fTextureW);
			int64_t nTextureY = (int64_t) (fTextureV * fTextureH);
			uint32_t nSourceCol = *(pSourcePixels + nTextureY*nTextureStride + nTextureX);
			*pDestPixels = (*pDestPixels&0xff000000)|(nSourceCol&0x00ffffff);
			pDestPixels++;

			fTextureU += fTextureDU;
			fTextureV += fTextureDV;
		}

		mnDrawnPixels += nScanLinePixels;
	}

	mnProcessedVertices += vertexArray.size();
	return true;
}

bool ZRasterizer::Rasterize(ZBuffer* pDestination, tColorVertexArray& vertexArray, ZRect* pClip)
{
    ZRect rDest = pDestination->GetArea();
    int64_t nDestStride = pDestination->GetArea().Width();
    double fClipLeft;
    double fClipRight;
    int64_t nTopScanLine;
    int64_t nBottomScanLine;

    SetupRasterization(pDestination, vertexArray, rDest, pClip, fClipLeft, fClipRight, nTopScanLine, nBottomScanLine);

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
        int64_t nStartX = (int64_t)scanLineMin.x;
        int64_t nScanLinePixels = (int64_t)scanLineMax.x - (int64_t)scanLineMin.x;
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

bool ZRasterizer::RasterizeSimple(ZBuffer* pDestination, ZBuffer* pTexture, ZRect rDest, ZRect rSrc, ZRect* pClip)
{
    tUVVertexArray verts;
    gRasterizer.RectToVerts(rDest, verts);

    double w = (double)pTexture->GetArea().Width();
    double h = (double)pTexture->GetArea().Height();

    verts[0].u = (double)rSrc.left / w;
    verts[0].v = (double)rSrc.top / h;

    verts[1].u = (double)rSrc.right / w;
    verts[1].v = (double)rSrc.top /h;

    verts[2].u = (double)rSrc.right / w;
    verts[2].v = (double)rSrc.bottom / h;

    verts[3].u = (double)rSrc.left / w;
    verts[3].v = (double)rSrc.bottom / h;

    return gRasterizer.Rasterize(pDestination, pTexture, verts, pClip);
}

bool ZRasterizer::RasterizeWithAlphaSimple(ZBuffer* pDestination, ZBuffer* pTexture, ZRect rDest, ZRect rSrc, ZRect* pClip, uint8_t nAlpha)
{
    tUVVertexArray verts;
    gRasterizer.RectToVerts(rDest, verts);

    double w = (double)pTexture->GetArea().Width();
    double h = (double)pTexture->GetArea().Height();

    verts[0].u = (double)rSrc.left / w;
    verts[0].v = (double)rSrc.top / h;

    verts[1].u = (double)rSrc.right / w;
    verts[1].v = (double)rSrc.top / h;

    verts[2].u = (double)rSrc.right / w;
    verts[2].v = (double)rSrc.bottom / h;

    verts[3].u = (double)rSrc.left / w;
    verts[3].v = (double)rSrc.bottom / h;

    return gRasterizer.RasterizeWithAlpha(pDestination, pTexture, verts, pClip, nAlpha);
}



ZRect ZRasterizer::GetBoundingRect(tUVVertexArray& vertexArray)
{
	ZRect rBounds(MAXINT64, MAXINT64, MININT64, MININT64);
	for (uint32_t nIndex = 0; nIndex < vertexArray.size(); nIndex++)
	{
		int64_t nVertX = (int64_t) vertexArray[nIndex].x;
		int64_t nVertY = (int64_t) vertexArray[nIndex].y;
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

    vertexArray[0].x = (double) r.left;
    vertexArray[0].y = (double) r.top;
    vertexArray[0].u = 0.0;
    vertexArray[0].v = 0.0;


    vertexArray[1].x = (double) r.right;
    vertexArray[1].y = (double) r.top;
    vertexArray[1].u = 1.0;
    vertexArray[1].v = 0.0;

    vertexArray[2].x = (double) r.right;
    vertexArray[2].y = (double) r.bottom;
    vertexArray[2].u = 1.0;
    vertexArray[2].v = 1.0;

    vertexArray[3].x = (double) r.left;
    vertexArray[3].y = (double) r.bottom;
    vertexArray[3].u = 0.0;
    vertexArray[3].v = 1.0;
}
void ZRasterizer::RectToVerts(const ZRect& r, tColorVertexArray& vertexArray)
{
    vertexArray.resize(4);

    vertexArray[0].x = (double)r.left;
    vertexArray[0].y = (double)r.top;

    vertexArray[1].x = (double)r.right;
    vertexArray[1].y = (double)r.top;

    vertexArray[2].x = (double)r.right;
    vertexArray[2].y = (double)r.bottom;

    vertexArray[3].x = (double)r.left;
    vertexArray[3].y = (double)r.bottom;
}
