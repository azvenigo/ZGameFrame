#include "ZFloatColorBuffer.h"

ZFloatColorBuffer::ZFloatColorBuffer()
{
    mpFloatPixels = nullptr;
}

ZFloatColorBuffer::~ZFloatColorBuffer()
{
    Shutdown();
}

bool ZFloatColorBuffer::From(ZBuffer* pSrc)
{
    ZRect rSrc(pSrc->GetArea());
    ZRect rOur(mBuffer.GetArea());

    if (rSrc != rOur)
    {
        if (!Init(rSrc.Width(), rSrc.Height()))
        {
            assert(false);
            return false;
        }
        if (!mBuffer.CopyPixels(pSrc, rSrc, rSrc))
        {
            assert(false);
            return false;
        }
    }
    uint32_t* pSrcCol = pSrc->GetPixels();
    ZFColor* pDstPixels = mpFloatPixels;

    int64_t nPixels = rSrc.Width() * rSrc.Height();

    for (int64_t i = 0; i < nPixels; i++)
    {
        uint32_t nCol = *pSrcCol;
        *pDstPixels = ZFColor((double)ARGB_A(nCol), (double)ARGB_R(nCol), (double)ARGB_G(nCol), (double)ARGB_B(nCol));

        pSrcCol++;
        pDstPixels++;
    }


    return true;
}

bool ZFloatColorBuffer::Init(int64_t nWidth, int64_t nHeight)
{
    if (mpFloatPixels)
        delete[] mpFloatPixels;
    mpFloatPixels = new ZFColor[nWidth * nHeight];
    return mBuffer.Init(nWidth, nHeight);
}

bool ZFloatColorBuffer::Shutdown()
{
    delete[] mpFloatPixels;
    mpFloatPixels = nullptr;
    mBuffer.Shutdown();
    return true;
}

void ZFloatColorBuffer::MultRect(ZRect& rArea, double fMult)
{
    ZRect rSrcArea(mBuffer.GetArea());

    if (mBuffer.Clip(rSrcArea, rSrcArea, rArea))
    {
        for (int64_t y = rArea.top; y < rArea.bottom; y++)
        {
            for (int64_t x = rArea.left; x < rArea.right; x++)
            {
                ZFColor* pCol = mpFloatPixels + (y * rSrcArea.Width() + x);
                *pCol = (*pCol) * fMult;
            }
        }
    }
}

void ZFloatColorBuffer::AddRect(ZRect& rArea, ZFColor col)
{
    ZRect rSrcArea(mBuffer.GetArea());

    if (mBuffer.Clip(rSrcArea, rSrcArea, rArea))
    {
        for (int64_t y = rArea.top; y < rArea.bottom; y++)
        {
            for (int64_t x = rArea.left; x < rArea.right; x++)
            {
                ZFColor* pCol = mpFloatPixels + (y * rSrcArea.Width() + x);
                ZFColor totalCol = *pCol + col;
                totalCol.a = 255.0; // force full alpha for now
                *pCol = totalCol;
                mBuffer.SetPixel(x, y, FloatColToCol(totalCol));
            }
        }
    }
}


										
void ZFloatColorBuffer::GetPixel(int64_t x, int64_t y, ZFColor& col)
{
    col = *(mpFloatPixels + (y * mBuffer.GetArea().Width() + x));
}

void ZFloatColorBuffer::SetPixel(int64_t x, int64_t y, ZFColor col)
{
    *(mpFloatPixels + (y * mBuffer.GetArea().Width() + x)) = col;
    mBuffer.SetPixel(x, y, ARGB((uint8_t)col.a, (uint8_t)col.r, (uint8_t)col.g, (uint8_t)col.b));
}

uint32_t ZFloatColorBuffer::FloatColToCol(ZFColor fCol)
{
    return ARGB((uint8_t)fCol.a, (uint8_t)fCol.r, (uint8_t)fCol.g, (uint8_t)fCol.b);
}

ZFColor ZFloatColorBuffer::ColToFloatCol(uint32_t nCol)
{
    return ZFColor((double)ARGB_A(nCol), (double)ARGB_R(nCol), (double)ARGB_G(nCol), (double)ARGB_B(nCol));
}
