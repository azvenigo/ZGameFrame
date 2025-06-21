#pragma once

#include "ZTypes.h"
#include "ZDebug.h"
#include "ZColor.h"
#include <string>
#include <mutex>
#include "easyexif/exif.h"
#include "Z3DMath.h"

typedef std::shared_ptr<class ZBuffer> tZBufferPtr;

class ZBuffer
{
public:
	enum eAlphaBlendType
	{
		kAlphaDest      = 0,
		kAlphaSource    = 1,
        kAlphaBlend     = 2
	};

    enum eRenderState : uint32_t
    {
        kFreeToModify    = 0,
        kReadyToRender   = 1,
        kBusy_SkipRender = 2
    };

    enum eBltEdgeFlags
    {
        kEdgeBltMiddle_None         = 0,
        kEdgeBltMiddle_Tile         = 1,
        kEdgeBltMiddle_Stretch      = 2,
        kEdgeBltSides_Stretch       = 4,
    };


    // Orientations defined as in EXIF spec
    enum eOrientation
    {
        kUnknown        = 0,
        kNone           = 1,
        kHFlip          = 2,
        k180            = 3,
        kVFlip          = 4,
        kLeftAndVFlip   = 5,
        kLeft           = 6,
        kRightAndVFlip  = 7,
        kRight          = 8,
    };

	ZBuffer();
    ZBuffer(ZBuffer* pSrc);
	virtual ~ZBuffer();

	virtual bool            Init(int64_t nWidth, int64_t nHeight);
	virtual bool            Shutdown();

    // Accessors and Manipulation
    virtual bool            CopyPixels(ZBuffer* pSrc);      // if dimensions are identical, copy everything wholesale
    virtual bool            CopyPixels(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, ZRect* pClip = NULL);		// like BLT but copies pixel values directly.... including alpha
    virtual uint32_t        GetPixel(int64_t x, int64_t y);
    virtual void            SetPixel(int64_t x, int64_t y, uint32_t nCol);
    virtual bool            Rotate(eOrientation rotation);


    virtual ZRect&          GetArea() { return mSurfaceArea; }

    virtual uint32_t*       GetPixels() { return mpPixels; }

    virtual easyexif::EXIFInfo& GetEXIF() { return mEXIF; }
    static bool             ReadEXIFFromFile(const std::string& sName, easyexif::EXIFInfo& info);

    // Drawing
    virtual bool            Fill(uint32_t nCol, ZRect* pRect = nullptr);			// fills a rect with nCol, forcing alpha to ARGB_A(nCol)
	virtual bool            FillAlpha(uint32_t nCol, ZRect* pRect = nullptr);	// fills a rect with nCol, blending based on ARGB_A(nCol)
    virtual bool            FillGradient(uint32_t nCol[4], ZRect* pRect = nullptr);     // fills gradient...color order is TL->TR->BR->BL

    virtual bool            Colorize(uint32_t nH, uint32_t nS, ZRect* pRect = nullptr);        // changes hue to color
    virtual void            Blur(float radius, float falloff = 1.0f, ZRect* pRect = nullptr);


	virtual bool            BltNoClip(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, eAlphaBlendType type = kAlphaDest);
	virtual bool            BltAlphaNoClip(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, uint32_t nAlpha, eAlphaBlendType type = kAlphaDest);
											
	virtual bool            Blt(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, ZRect* pClip = NULL, eAlphaBlendType type = kAlphaDest);
	virtual bool            BltAlpha(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, uint32_t nAlpha, ZRect* pClip = NULL, eAlphaBlendType type = kAlphaDest);

	virtual bool            BltTiled(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, int64_t nStartX, int64_t nStartY, ZRect* pClip = NULL, eAlphaBlendType type = kAlphaDest);
	virtual bool            BltEdge(ZBuffer* pSrc, ZRect& rEdgeRect, ZRect& rDst, uint32_t flags = kEdgeBltMiddle_Tile, ZRect* pClip = NULL, eAlphaBlendType type = kAlphaDest);

	virtual bool            BltRotated(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, double fAngle, double fScale, ZRect* pClip = NULL);

    virtual bool            BltScaled(ZBuffer* pSrc);


	virtual void            DrawAlphaLine(const ZColorVertex& v1, const ZColorVertex& v2, double thickness = 2.0, ZRect* pClip = NULL);
    virtual void            DrawRectAlpha(uint32_t nCol, ZRect rDst, eAlphaBlendType type = kAlphaDest);

    virtual void            DrawCircle(ZPoint center, int64_t radius, uint32_t col);
    virtual void            DrawSphere(ZPoint center, int64_t radius, const Z3D::Vec3f& lightPos, const Z3D::Vec3f& viewPos, const Z3D::Vec3f& ambient, const Z3D::Vec3f& diffuse, const Z3D::Vec3f& specular, float shininess);


    // Load/Save
	virtual bool            LoadBuffer(const std::string& sName);
    virtual bool            SaveBuffer(const std::string& sName);
#ifdef _WIN64
//	virtual bool            LoadBuffer(uint32_t nResourceID);
#endif

    // Thread Safety
    virtual std::recursive_mutex& GetMutex() { return mMutex; }

    bool                    Clip(ZRect& dstRect);       // clips dst into mSurfaceArea
    static bool             Clip(const ZRect& fullDstRect, ZRect& srcRect, ZRect& dstRect);
    static bool             Clip(const ZBuffer* pSrc, const ZBuffer* pDst, ZRect& rSrc, ZRect& rDst);
    static bool             Clip(const ZRect& rSrcSurface, const ZRect& rDstSurface, ZRect& rSrc, ZRect& rDst);

#ifdef _DEBUG
    std::string             sDebugOwner;
#endif

protected:
    bool                    FloatScanLineIntersection(double fScanLine, const ZColorVertex& v1, const ZColorVertex& v2, double& fIntersection, double& fR, double& fG, double& fB, double& fA);
    void                    FillInSpan(uint32_t* pDest, int64_t nNumPixels, double fR, double fG, double fB, double fA);

    bool                    LoadFromSVG(const std::string& sName);
    uint32_t                ComputePixelBlur(ZBuffer* pBuffer, int64_t nX, int64_t nY, int64_t nRadius);
    ZRect                   FindContentBounds(const ZRect& searchArea);

public:
	uint32_t*                   mpPixels;        // The color data
	ZRect                       mSurfaceArea;
    easyexif::EXIFInfo          mEXIF;
    std::recursive_mutex        mMutex;
    bool                        mbHasAlphaPixels;
    std::atomic<eRenderState>   mRenderState;
};
