#pragma once

#include "ZWin.h"
#include "ZAnimator.h"

class cXMLNode;
class ZImageWin;


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

	bool	LoadImages(std::list<string>& filenames);

	bool	HandleMessage(const ZMessage& message);

protected:
    bool	Paint();


private:
	void	ProcessImages();

	void	Process_LoadImages();
    void	Process_SelectImage(int32_t nIndex);
    void    Process_ComputeGradient();


    // Spawn threads with jobs to do
    bool    SpawnWork(void(*pProc)(void*), bool bBarrierSyncPoint = false);

    // thread functions
    static void RadiusBlur(void* pContext);
    static void	ComputeContrast(void* pContext);
    static void	StackImages(void* pContext);

    bool BlurBox(int64_t x, int64_t y); // playing around with blurring around mouse cursor based on contrast

    uint32_t    ComputePixelBlur(ZBuffer* pBuffer, int64_t nX, int64_t nY, int64_t nRadius);
	double	ComputePixelContrast(ZBuffer* pBuffer, int64_t nX, int64_t nY, int64_t nRadius);
	void	CopyPixelsInRadius(ZBuffer* pSourceBuffer, ZBuffer* pDestBuffer, int64_t nX, int64_t nY, int64_t nRadius);
	void	ResetResultsBuffer();

    uint32_t ComputeAverageColor(ZBuffer* pBuffer, ZRect rArea);

	int64_t	mnProcessPixelRadius;
    int64_t mnGradientLevels;


	std::vector< std::shared_ptr<ZBuffer> > mImagesToProcess;
	ZRect mrOriginalImagesArea;
    ZRect mrResultImageDest;
//	cCEBuffer*	mpBufferToProcess;

	ZImageWin* mpProcessedImageWin;


    int64_t     mnSelectedImageIndex;
    int64_t     mnSelectedImageW;
    int64_t     mnSelectedImageH;


	std::unique_ptr<ZBuffer>	mpResultBuffer;

    double*                     mpContrastFloatBuffer;
    double                      mfHighestContrast;


    std::list<ZImageWin*>       mChildImageWins;
    ZImageWin*                  mpResultWin;
    string                      msResult;

    uint32_t                    mThreads;

	ZRect mrSliderCaption;

};

