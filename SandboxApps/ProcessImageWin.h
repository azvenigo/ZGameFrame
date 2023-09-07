#pragma once

#include "ZWin.h"
#include "ZAnimator.h"
#include "ZFloatColorBuffer.h"
#include "ZWinFormattedText.h"

class cXMLNode;
class ZWinImage;


class cFloatAreaDescriptor
{
public:
    ZFColor fCol;
    ZRect rArea;
};

typedef std::list<cFloatAreaDescriptor> tFloatAreas;


/////////////////////////////////////////////////////////////////////////
// 
class cProcessImageWin : public ZWin
{
public:
	cProcessImageWin();
    ~cProcessImageWin();

	bool	Init();
	bool	Shutdown();

    bool	OnMouseUpL(int64_t x, int64_t y);
    bool	OnMouseDownL(int64_t x, int64_t y);
    bool    OnMouseMove(int64_t x, int64_t y);

    bool	OnChar(char c);
	bool	OnKeyDown(uint32_t key);

	bool	LoadImages(std::list<std::string>& filenames);

    bool    RemoveImage(const std::string& sFilename);
    bool    ClearImages();

	bool	HandleMessage(const ZMessage& message);

protected:
    bool	Paint();


private:
	void	ProcessImages();

	void	Process_LoadImages();
    void    Process_SaveResultImage();
    void	Process_SelectImage(std::string sImageName);
    void    Process_ComputeGradient();
    void    Process_FloatColorSandbox();

    bool    Subdivide_and_Subtract(ZFloatColorBuffer* pBuffer, ZRect rArea, int64_t nMinSize, tFloatAreas& floatAreas);   // Adds subtracted to floatAreas

    void    UpdatePrefs();
    void    UpdateImageProps(ZBuffer* pBuf);

    // Spawn threads with jobs to do
    bool    SpawnWork(void(*pProc)(void*), bool bBarrierSyncPoint = false);

    // thread functions
    static void RadiusBlur(void* pContext);
    static void	ComputeContrast(void* pContext);
    static void	StackImages(void* pContext);
    static void NegativeImage(void* pContext);
    static void Mono(void* pContext);


    bool BlurBox(int64_t x, int64_t y); // playing around with blurring around mouse cursor based on contrast

    uint32_t    ComputePixelBlur(tZBufferPtr pBuffer, int64_t nX, int64_t nY, int64_t nRadius);
	double	ComputePixelContrast(tZBufferPtr pBuffer, int64_t nX, int64_t nY, int64_t nRadius);
	void	CopyPixelsInRadius(tZBufferPtr pSourceBuffer, tZBufferPtr pDestBuffer, int64_t nX, int64_t nY, int64_t nRadius);
	void	ResetResultsBuffer();

    void ComputeIntersectedWorkArea();

    uint32_t ComputeAverageColor(tZBufferPtr pBuffer, ZRect rArea);
    bool    ComputeAverageColor(ZFloatColorBuffer* pBuffer, ZRect rArea, ZFColor& fCol);

	int64_t	mnProcessPixelRadius;
    int64_t mnGradientLevels;
    int64_t mnSubdivisionLevels;


//	std::vector< tZBufferPtr > mImagesToProcess;
	ZRect mrIntersectionWorkArea;
    ZRect mrResultImageDest;
    ZRect mrWatchPanel;
    ZRect mrControlPanel;
    ZRect mrThumbnailSize;
//	cCEBuffer*	mpBufferToProcess;

	ZWinImage*                  mpProcessedImageWin;
    ZWinFormattedDoc*          mpImageProps;

    ZFloatColorBuffer           mFloatColorBuffer;

    tZBufferPtr	mpResultBuffer;

    double*                     mpContrastFloatBuffer;
    double                      mfHighestContrast;


    std::list<ZWinImage*>       mChildImageWins;

    ZWinImage*                  mpResultWin;
    uint32_t                    mThreads;

	ZRect mrSliderCaption;

};

