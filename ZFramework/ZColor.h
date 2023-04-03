#pragma once

#include "ZTypes.h"
#include "ZDebug.h"

// 32 bit ARGB
// 8 bits each
#define ARGB_A(nCol) ((nCol&0xff000000)>>24)
#define ARGB_R(nCol) ((nCol&0x00ff0000)>>16)
#define ARGB_G(nCol) ((nCol&0x0000ff00)>>8)
#define ARGB_B(nCol) (nCol&0x000000ff)
#define ARGB(nA, nR, nG, nB) ((nA)<<24)|((nR)<<16)|((nG)<<8)|(nB)
#define TO_ARGB(nCol, nA, nR, nG, nB) { nA = ARGB_A(nCol); nR = ARGB_R(nCol); nG = ARGB_G(nCol); nB = ARGB_B(nCol); }

// 32 bit AHSV
// 2 bits for alpha 10 bits for H S and V
// AAHHHHHHHHHHSSSSSSSSSSVVVVVVVVVV
#define AHSV_A(nCol)  (nCol >> 30)
#define AHSV_H(nCol) ((nCol&0x3FF00000)>>20)
#define AHSV_S(nCol) ((nCol&0x000FFC00)>>10)
#define AHSV_V(nCol)  (nCol&0x000003FF)
#define AHSV(nA, nH, nS, nV) (\
(((uint32_t)nA) << 30)|\
(((uint32_t)nH) << 20)|\
(((uint32_t)nS) << 10)|(nV)\
)



#define TO_AHSV(nCol, nA, nH, nS, nV) { nA = AHSV_A(nCol); nH = AHSV_H(nCol) ; nS = AHSV_S(nCol); nV = AHSV_V(nCol); }

namespace COL
{
    // https://en.wikipedia.org/wiki/YCbCr#ITU-R_BT.601_conversion
    inline void YCbCr_To_RGB(uint8_t Y, uint8_t Cb, uint8_t Cr, uint8_t& R, uint8_t& G, uint8_t& B)
    {
        float fY =  ((float)Y) /255.0f;
        float fCb = (((float)Cb) - 127.0f)/255.0f;
        float fCr = (((float)Cr) - 127.0f)/255.0f;

        float fR = fY + 1.5748f * fCr;
        float fG = fY - 0.1873f * fCb - 0.4681f * fCr;
        float fB = fY + 1.8556f * fCb;

        R = (uint8_t)(fR * 255.0f);
        G = (uint8_t)(fG * 255.0f);
        B = (uint8_t)(fB * 255.0f);
    }

    inline void RGB_To_YCbCr(uint8_t R, uint8_t G, uint8_t B, uint8_t& Y, uint8_t& Cb, uint8_t& Cr)
    {
        float fR = ((float)R) / 255.0f;
        float fG = ((float)G) / 255.0f;
        float fB = ((float)B) / 255.0f;

        float fY = 0.2126f * fR + 0.7152f * fG + 0.0722f * fB;
        float fCb = -0.1146f * fR - 0.3854f * fG + 0.5f * fB;
        float fCr = 0.5f * fR - 0.4552f * fG - 0.0458f * fB;

        Y = (uint8_t)(fY * 255.0f);
        Cb = (uint8_t)((fCb+0.5f) * 255.0f);
        Cr = (uint8_t)((fCr+0.5f) * 255.0f);
    }

#define FMIN2(x, y)     std::min<float>(x, y)
#define FMIN3(x, y, z)  std::min<float>( std::min<float>(x, y), z)
#define FMAX2(x, y)     std::max<float>(x, y)
#define FMAX3(x, y, z)  std::max<float>( std::max<float>(x, y), z)

    inline void ARGB_To_AHSV(uint8_t A, uint8_t R, uint8_t G, uint8_t B, uint8_t& outA, uint32_t& outH, uint32_t& outS, uint32_t& outV)
    {
        float fR = (float)R;
        float fG = (float)G;
        float fB = (float)B;

        float fMin = FMIN3(fR, fG, fB);
        float fMax = FMAX3(fR, fG, fB);
        float fDelta = fMax - fMin;

        float fH = 0;
        float fS;
        float fV = fMax;


        if (fMax == 0.0)
            fS = 0;
        else
            fS = fDelta / fV;

        if (fS != 0.0)
        {
            if (fR == fV)
                fH = (fG - fB) / fDelta;
            else if (fG == fV)
                fH = 2.0f + (fB - fR) / fDelta;
            else
                fH = 4.0f + (fR - fG) / fDelta;

            fH *= 60.0f;
            if (fH < 0.0f)
                fH += 360.0f;
        }

        outA = A >> 7;  // quantize 8 bit down to 2
        outH = (uint32_t)(fH * 1023.0f / 360.0f);     // scaled to 10 bits
        outS = (uint32_t)(fS * 1023.0f);             // scaled to 10 bits
        outV = (uint32_t)(fV * 1023.0f / 255.0f);     // scaled to 10 bits
    }

    inline void AHSV_To_ARGB(uint8_t A, uint32_t H, uint32_t S, uint32_t V, uint8_t& outA, uint8_t& outR, uint8_t& outG, uint8_t& outB)
    {
        float fR = 0;
        float fG = 0;
        float fB = 0;

        float fV = (float)V / 1023.0f;    // scaled down from 10 bits to 0-1

        if (S == 0)
        {
            fR = fV;
            fG = fV;
            fB = fV;
        }
        else
        {
            float fH = ((float)H) * 360.0f / 1023.0f;     // scaled down from 10 bits to 0-360
            float fS = ((float)S) / 1023.0f;             // scaled down from 10 bits to 0-1

            if (fH == 360.0f)
                fH = 0;
            else
                fH /= 60.0f;

            int i = (int)std::trunc(fH);
            float fF = fH - i;

            float fP = fV * (1.0f - fS);
            float fQ = fV * (1.0f - (fS * fF));
            float fT = fV * (1.0f - (fS * (1.0f - fF)));

            switch (i)
            {
            case 0:
                fR = fV;
                fG = fT;
                fB = fP;
                break;
            case 1:
                fR = fQ;
                fG = fV;
                fB = fP;
                break;
            case 2:
                fR = fP;
                fG = fV;
                fB = fT;
                break;
            case 3:
                fR = fP;
                fG = fQ;
                fB = fV;
                break;
            case 4:
                fR = fT;
                fG = fP;
                fB = fV;
                break;
            default:
                fR = fV;
                fG = fP;
                fB = fQ;
                break;
            }
        }

        // from quantized to either 100%, 50% or 0%
        if (A > 1)
            outA = 0xff;
        else if (A == 1)
            outA = 0x88;
        else
            outA = 0x00;

        outR = (uint8_t)(fR * 255.0);
        outG = (uint8_t)(fG * 255.0);
        outB = (uint8_t)(fB * 255.0);
    }


    inline uint32_t ARGB_To_AHSV(uint8_t A, uint32_t R, uint32_t G, uint32_t B)
    {
        uint8_t a;
        uint32_t h;
        uint32_t s;
        uint32_t v;
        ARGB_To_AHSV(A, R, G, B, a, h, s, v);
        return AHSV(a, h, s, v);
    }

    inline uint32_t AHSV_To_ARGB(uint8_t A, uint32_t H, uint32_t S, uint32_t V)
    {
        uint8_t a;
        uint8_t r;
        uint8_t g;
        uint8_t b;
        AHSV_To_ARGB(A, H, S, V, a, r, g, b);
        return ARGB(a, r, g, b);
    }

    inline uint32_t ARGB_To_AHSV(uint32_t argb)
    {
        return ARGB_To_AHSV(ARGB_A(argb), ARGB_R(argb), ARGB_G(argb), ARGB_B(argb));
    }

    inline uint32_t AHSV_To_ARGB(uint32_t ahsv)
    {
        return ARGB_To_AHSV(AHSV_A(ahsv), AHSV_H(ahsv), AHSV_S(ahsv), AHSV_V(ahsv));
    }






    inline uint32_t AlphaBlend_Col1Alpha(uint32_t nCol1, uint32_t nCol2, uint32_t nBlend)
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
    inline uint32_t AlphaBlend_Col2Alpha(uint32_t nCol1, uint32_t nCol2, uint32_t nBlend)
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
    inline uint32_t AlphaBlend_AddAlpha(uint32_t nCol1, uint32_t nCol2, uint32_t nBlend)
    {
        nBlend = nBlend * ARGB_A(nCol1) >> 8;
        uint32_t nInverseAlpha = 255 - nBlend;
        uint32_t nFinalA = (ARGB_A(nCol1) + ARGB_A(nCol2));
        if (nFinalA > 0x000000ff)
            nFinalA = 0x000000ff;
        // Blend fAlpha of nCol1 and 1.0-fAlpha of nCol2
        return ARGB(
            nFinalA,
            ((ARGB_R(nCol1) * nBlend + ARGB_R(nCol2) * nInverseAlpha)) >> 8,
            ((ARGB_G(nCol1) * nBlend + ARGB_G(nCol2) * nInverseAlpha)) >> 8,
            ((ARGB_B(nCol1) * nBlend + ARGB_B(nCol2) * nInverseAlpha)) >> 8
        );
    }
    inline uint32_t AlphaBlend_BlendAlpha(uint32_t nCol1, uint32_t nCol2, uint32_t nBlend)
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

    inline uint32_t AddColors(uint32_t nCol1, uint32_t nCol2)
    {
        return ARGB(
            (uint8_t)(((ARGB_A(nCol1) + ARGB_A(nCol2)))),
            (uint8_t)(((ARGB_R(nCol1) + ARGB_R(nCol2)))),
            (uint8_t)(((ARGB_G(nCol1) + ARGB_G(nCol2)))),
            (uint8_t)(((ARGB_B(nCol1) + ARGB_B(nCol2))))
        );

    }
};
