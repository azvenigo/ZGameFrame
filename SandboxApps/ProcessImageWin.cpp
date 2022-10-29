#include "ProcessImageWin.h"
#include "ZBuffer.h"
#include "ZStdTypes.h"
#include "ZMessageSystem.h"
#include "ZAnimObjects.h"
#include "ZGraphicSystem.h"
#include "ZScreenBuffer.h"
#include "Resources.h"
#include "ZStringHelpers.h"
#include "ZScriptedDialogWin.h"
#include "ZWinBtn.H"
#include "ZSliderWin.h"
#include "ZImageWin.h"
#include "ZWinControlPanel.h"
#include "ZWinWatchPanel.h"
#include "ZTimer.h"
#include "ZRandom.h"
#include "helpers/StringHelpers.h"

#ifdef _WIN64        // for open file dialong
#include <windows.h>
#include <shobjidl.h> 
#endif

using namespace std;

extern ZMessageSystem gMessageSystem;

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


class JobParams
{
public:
    JobParams()
    {
        nRadius = 0;
        pThis = nullptr;
        mpBarrierCount = nullptr;
        pSourceImage = nullptr;
        pDestImage = nullptr;
    }

    ZRect               rArea;
    int64_t             nRadius;

    cProcessImageWin*   pThis;

    ZBuffer* pSourceImage;
    ZBuffer* pDestImage;

    std::thread         worker;
    atomic<int64_t>*    mpBarrierCount;
};

cProcessImageWin::cProcessImageWin()
{
	mbAcceptsFocus = true;
	mbAcceptsCursorMessages = true;
	mpResultBuffer = nullptr;
    mpContrastFloatBuffer = nullptr;
	mnProcessPixelRadius = 1;
    mnGradientLevels = 1;
    mnSelectedImageIndex = 0;

    mThreads = 32;

	msWinName = "imageprocesswin";
}

cProcessImageWin::~cProcessImageWin()
{
    delete mpContrastFloatBuffer;
}

bool cProcessImageWin::LoadImages(std::list<string>& filenames)
{
    ZScreenBuffer* pScreenBuffer = gGraphicSystem.GetScreenBuffer();
    pScreenBuffer->EnableRendering(false);


    const std::lock_guard<std::mutex> surfaceLock(mpTransformTexture->GetMutex());
    mImagesToProcess.clear();

    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    for (auto pImageWins : mChildImageWins)
    {
        pImageWins->Hide();
        ChildDelete(pImageWins);
    }
    mChildImageWins.clear();

	for (auto filename : filenames)
	{
		std::shared_ptr<ZBuffer> pNewBuffer(new ZBuffer());
		pNewBuffer->LoadBuffer(filename);
		mImagesToProcess.push_back(pNewBuffer);
	}

    mpResultBuffer = nullptr;

    int32_t nNumImages = (int32_t) mImagesToProcess.size();

    ZRect rOriginalImage;
    
    if (mImagesToProcess.empty())
    {
        mrOriginalImagesArea.SetRect(0, 0, 0, 0);
    }
    else
    {
        mrOriginalImagesArea.SetRect(mImagesToProcess[0]->GetArea());
        double fScale = 2.0;

        double fImageAspect = (double)mrOriginalImagesArea.Width() / (double)mrOriginalImagesArea.Height();
        int64_t nImageWidth = mAreaToDrawTo.Width() / nNumImages;
        int64_t nImageHeight = (int64_t) (nImageWidth / fImageAspect);

        if (nImageHeight > mAreaToDrawTo.Height() / 4)  // max out at 25% of the window
        {
            nImageHeight = mAreaToDrawTo.Height() / 4;
            nImageWidth = (int64_t) (nImageHeight * fImageAspect);
        }


        rOriginalImage.SetRect(0, 0, nImageWidth, nImageHeight);

        for (int32_t i = 0; i < mImagesToProcess.size(); i++)
        {
            ZImageWin* pOriginalImageWin = new ZImageWin();
            pOriginalImageWin->SetArea(rOriginalImage);
            pOriginalImageWin->SetImage(mImagesToProcess[i].get());

            if (mImagesToProcess.size() > 1)
            {
                string sCaption;
                Sprintf(sCaption, "#%d", i);
                pOriginalImageWin->SetCaption(sCaption, 0, 0x88ffffff, ZFont::kBottomLeft);
            }


            string sMessage;
            Sprintf(sMessage, "type=selectimg;index=%d;target=imageprocesswin", i);
            pOriginalImageWin->SetOnClickMessage( sMessage );
            pOriginalImageWin->SetZoomable(true, 0.01, 2.0);
            pOriginalImageWin->SetFill(0x00000000);
            ChildAdd(pOriginalImageWin);

            mChildImageWins.push_back(pOriginalImageWin);

            rOriginalImage.OffsetRect(rOriginalImage.Width(),0);
        }

    }


    int64_t nResultAreaHeight = mAreaToDrawTo.bottom - rOriginalImage.bottom;

    double fResultAreaAspect = (double) rOriginalImage.Width() / (double) rOriginalImage.Height();

    mrResultImageDest.SetRect(0,rOriginalImage.bottom, (int64_t) (nResultAreaHeight * fResultAreaAspect), mAreaToDrawTo.bottom);
    ResetResultsBuffer();

    pScreenBuffer->EnableRendering(true);

    InvalidateChildren();

	return true;
}

void cProcessImageWin::Process_LoadImages()
{
    std::list<string> filenames;

#ifdef _WIN64
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            COMDLG_FILTERSPEC rgSpec[] =
            {
                { L"Supported Image Formats", L"*.jpg;*.jpeg;*.bmp;*.gif;*.png;*.tiff;*.exif;*.wmf;*.emf" },
                { L"All Files", L"*.*" }
            };

            pFileOpen->SetFileTypes(2, rgSpec);

            DWORD dwOptions;
            // Specify multiselect.
            hr = pFileOpen->GetOptions(&dwOptions);

            if (SUCCEEDED(hr))
            {
                hr = pFileOpen->SetOptions(dwOptions | FOS_ALLOWMULTISELECT);
            }

            // Show the Open dialog box.
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItemArray* pItems;
                hr = pFileOpen->GetResults(&pItems);
                if (SUCCEEDED(hr))
                {
                    DWORD nCount = 0;
                    pItems->GetCount(&nCount);
                    for (DWORD i = 0; i < nCount; i++)
                    {
                        IShellItem* pItem;
                        pItems->GetItemAt(i, &pItem);

                        PWSTR pszFilePath;
                        pItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszFilePath);

                        wstring wideFilename(pszFilePath);
                        filenames.push_back(StringHelpers::wstring2string(wideFilename));
                        pItem->Release();
                    }
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }

    if (!filenames.empty())
    {
        LoadImages(filenames);
        Process_SelectImage(0);
    }
#endif
}

void cProcessImageWin::Process_SelectImage(int32_t nIndex)
{
    if (nIndex >= 0 && nIndex < mImagesToProcess.size())
    {
        mnSelectedImageIndex = nIndex;
        ZRect r(mpResultBuffer.get()->GetArea());
        mpResultBuffer.get()->Blt(mImagesToProcess[nIndex].get(), r, r, &r);

        mnSelectedImageW = mImagesToProcess[nIndex]->GetArea().Width();
        mnSelectedImageH = mImagesToProcess[nIndex]->GetArea().Height();

        Sprintf(msResult, "Image:%d", nIndex);
        InvalidateChildren();
    }
}

uint32_t cProcessImageWin::ComputeAverageColor(ZBuffer* pBuffer, ZRect rArea)
{
    uint64_t nTotalR = 0;
    uint64_t nTotalG = 0;
    uint64_t nTotalB = 0;
    for (int32_t y = rArea.top; y < rArea.bottom; y++)
    {
        for (int32_t x = rArea.left; x < rArea.right; x++)
        {
            uint32_t nCol = pBuffer->GetPixel(x, y);
            nTotalR += ARGB_R(nCol);
            nTotalG += ARGB_G(nCol);
            nTotalB += ARGB_B(nCol);
        }
    }

    uint64_t nPixels = rArea.Width() * rArea.Height();
    uint32_t nAverageR = (uint32_t)(nTotalR / nPixels);
    uint32_t nAverageG = (uint32_t)(nTotalG / nPixels);
    uint32_t nAverageB = (uint32_t)(nTotalB / nPixels);

    return ARGB(0xff, nAverageR, nAverageG, nAverageB);
}

void cProcessImageWin::Process_ComputeGradient()
{
    int32_t nSubdivisions = /*1 <<*/ mnGradientLevels;

    int32_t nSubW = mpResultBuffer->GetArea().Width() / nSubdivisions;
    int32_t nSubH = mpResultBuffer->GetArea().Height() / nSubdivisions;


    uint32_t* subdivisionColorGrid = new uint32_t[nSubdivisions * nSubdivisions];

    for (int32_t nGridY = 0; nGridY < nSubdivisions; nGridY++)
    {
        for (int32_t nGridX = 0; nGridX < nSubdivisions; nGridX++)
        {
            ZRect rSubArea(nGridX * nSubW, nGridY * nSubH, (nGridX+1)* nSubW, (nGridY+1)* nSubH);

            int32_t nIndex = (nGridY * nSubdivisions) + nGridX;
            subdivisionColorGrid[nIndex] = ComputeAverageColor(mpResultBuffer.get(), rSubArea);
        }
    }

    for (int32_t nGridY = 0; nGridY < nSubdivisions-1; nGridY++)
    {
        for (int32_t nGridX = 0; nGridX < nSubdivisions-1; nGridX++)
        {
//            if (nGridX == 1 && nGridY%2 == 1)
            {
                ZRect rSubArea(nGridX * nSubW, nGridY * nSubH, (nGridX + 1) * nSubW, (nGridY + 1) * nSubH);
                tColorVertexArray verts;
                gRasterizer.RectToVerts(rSubArea, verts);

                int32_t nIndex = (nGridY * nSubdivisions) + nGridX; // TL
                verts[0].mColor = subdivisionColorGrid[nIndex];

                nIndex++;   // TR

                verts[1].mColor = subdivisionColorGrid[nIndex];

                nIndex += nSubW; // BR
                verts[2].mColor = subdivisionColorGrid[nIndex];

                nIndex--;       // BL
                verts[3].mColor = subdivisionColorGrid[nIndex];

                gRasterizer.Rasterize(mpResultBuffer.get(), mpResultBuffer.get(), verts);
            }
        }
    }
    delete[] subdivisionColorGrid;
    InvalidateChildren();
}

/*void cProcessImageWin::Process_ComputeGradient()
{
    ZRect rDrawArea(mpResultBuffer->GetArea());
    rDrawArea.DeflateRect(32, 32);

    tColorVertexArray verts;
    gRasterizer.RectToVerts(rDrawArea, verts);
    verts[0].mColor = ARGB(0xff, 0xff, 0, 0);
    verts[1].mColor = ARGB(0x00, 0x00, 0xff, 0);
    verts[2].mColor = ARGB(0x00, 0, 0, 0xff);
    verts[3].mColor = ARGB(0xff, 0xff, 0, 0xff);
    gRasterizer.Rasterize(mpResultBuffer.get(), mpResultBuffer.get(), verts);

    Invalidate();
}*/








bool cProcessImageWin::SpawnWork(void(*pProc)(void*), bool bBarrierSyncPoint)
{
//    ZScreenBuffer* pScreenBuffer = gGraphicSystem.GetScreenBuffer();
//    pScreenBuffer->EnableRendering(false);

    ZBuffer sourceImg(mpResultBuffer.get());
    ResetResultsBuffer();
    if (mImagesToProcess.empty())
    {
//        pScreenBuffer->EnableRendering(true);
        return false;
    }

    int64_t nStartTime = gTimer.GetMSSinceEpoch();

    std::vector<JobParams> workers;
    workers.resize(mThreads);

    std::atomic<int64_t> nBarrierSync = mThreads;
        
    int64_t nTop = mnProcessPixelRadius;        // start radius pixels down
    int64_t nLines = mrOriginalImagesArea.Height() / mThreads;

    mfHighestContrast = 0.0;

    for (int i = 0; i < mThreads; i++)
    {
        int64_t nBottom = nTop+nLines;
        if (nBottom > mrOriginalImagesArea.bottom - mnProcessPixelRadius)
            nBottom = mrOriginalImagesArea.bottom - mnProcessPixelRadius;

        workers[i].rArea.SetRect(mrOriginalImagesArea.left+mnProcessPixelRadius, nTop, mrOriginalImagesArea.right-mnProcessPixelRadius, nBottom);
        workers[i].nRadius = mnProcessPixelRadius;
        workers[i].pSourceImage = &sourceImg;
        workers[i].pDestImage = mpResultBuffer.get();
        workers[i].pThis = this;
        if (bBarrierSyncPoint)
            workers[i].mpBarrierCount = &nBarrierSync;

        workers[i].worker = std::thread(pProc, (void*)&workers[i]);

        nTop += nLines;
    }

    for (int i = 0; i < mThreads; i++)
    {
        workers[i].worker.join();
    }

    InvalidateChildren();
//    pScreenBuffer->EnableRendering(true);
    return true;
}




void cProcessImageWin::RadiusBlur(void* pContext)
{
    JobParams* pJP = (JobParams*) pContext;

    for (int64_t y = pJP->rArea.top; y < pJP->rArea.bottom; y++)
    {
        for (int64_t x = pJP->rArea.left; x < pJP->rArea.right; x++)
        {
            pJP->pDestImage->SetPixel(x, y, pJP->pThis->ComputePixelBlur(pJP->pSourceImage, x, y, pJP->nRadius));
        }
    }
}

uint32_t cProcessImageWin::ComputePixelBlur(ZBuffer* pBuffer, int64_t nX, int64_t nY, int64_t nRadius)
{
    if (nRadius < 1)
        return 0xff;

    // Set boundaries to max
    uint32_t nResultR = 0;
    uint32_t nResultG = 0;
    uint32_t nResultB = 0;
    double fMinDistanceSquared = (double)nRadius * nRadius;

    double fPixelsInRadius = 0.0;

    // These are all of the pixels encompasing the circle (middle of circle to center of any square side being the radius)
    for (int64_t y = nY - nRadius; y < nY + nRadius; y++)
    {
        for (int64_t x = nX - nRadius; x < nX + nRadius; x++)
        {
            double fDistanceSquared = (double)(x - nX) * (double)(x - nX) + (double)(y - nY) * (double)(y - nY);

            if (fDistanceSquared > fMinDistanceSquared)	// ensures this is within a circular radius
                continue;

            //nPixels++;

            double fDistance = sqrt(fDistanceSquared);

            uint32_t nCol = pBuffer->GetPixel(x, y);

            uint8_t nCurR = ARGB_R(nCol);
            uint8_t nCurG = ARGB_G(nCol);
            uint8_t nCurB = ARGB_B(nCol);

            if (fDistance > 0.0)
            {
                nResultR += ((nCurR) / fDistance);
                nResultG += ((nCurG) / fDistance);
                nResultB += ((nCurB) / fDistance);
                fPixelsInRadius += 1/fDistance;
            }
            else
            {
                nResultR += nCurR;
                nResultG += nCurG;
                nResultB += nCurB;
                fPixelsInRadius += 1;
            }
        }
    }

    nResultR = nResultR/fPixelsInRadius;
    nResultG = nResultG/fPixelsInRadius;
    nResultB = nResultB/fPixelsInRadius;



    uint32_t nBlurred = ARGB(0xff, nResultR, nResultG, nResultB);

    return nBlurred;
}

template<typename A, typename B>
std::pair<B, A> flip_pair(const std::pair<A, B>& p)
{
    return std::pair<B, A>(p.second, p.first);
}

template<typename A, typename B>
std::multimap<B, A> flip_map(const std::map<A, B>& src)
{
    std::multimap<B, A> dst;
    std::transform(src.begin(), src.end(), std::inserter(dst, dst.begin()),
        flip_pair<A, B>);
    return dst;
}

void cProcessImageWin::StackImages(void* pContext)
{
    JobParams* pJP = (JobParams*)pContext;

    int32_t nImages = (int32_t)pJP->pThis->mImagesToProcess.size();

    if (nImages < 1)
        return;

    std::vector<double> imageContrasts;
    imageContrasts.resize(nImages);

    for (int64_t y = pJP->rArea.top; y < pJP->rArea.bottom; y++)
    {
        for (int64_t x = pJP->rArea.left; x < pJP->rArea.right; x++)
        {
            double fHighestContrast = 0.0;
            double fLowestContrast = 999999999;
//            int64_t nHighestContrastImageIndex = 0;

            for (int32_t i = 0; i < nImages; i++)
            {
                ZBuffer* pBuffer = pJP->pThis->mImagesToProcess[i].get();
                double fContrast = pJP->pThis->ComputePixelContrast(pBuffer, x, y, pJP->nRadius);
                imageContrasts[i] = fContrast;
                if (fHighestContrast < fContrast)
                    fHighestContrast = fContrast;
                if (fLowestContrast > fContrast)
                    fLowestContrast = fContrast;
/*                if (fContrast > 64.0 && fContrast > fHighestContrast)
                {
                    fHighestContrast = fContrast;
                    nHighestContrastImageIndex = i;
                }*/
            }

            double fContrastRange = fHighestContrast - fLowestContrast;

            double fAccumulatedR = 0.0;
            double fAccumulatedG = 0.0;
            double fAccumulatedB = 0.0;

            if (fContrastRange < 1.0)
            {
                // if no contrast, just use.what? image 0?
                pJP->pDestImage->SetPixel(x, y, pJP->pThis->mImagesToProcess[rand()%nImages].get()->GetPixel(x, y));
//                pJP->pDestImage->SetPixel(x, y, 0xffff00ff);
            }
            else
            {
                double fAccumulatedWeights = 0.0;
                for (int32_t i = 0; i < nImages; i++)
                {
                    uint32_t nCol = pJP->pThis->mImagesToProcess[i].get()->GetPixel(x, y);
                    uint32_t nSourceR = ARGB_R(nCol);
                    uint32_t nSourceG = ARGB_G(nCol);
                    uint32_t nSourceB = ARGB_B(nCol);

                    double fContrast = imageContrasts[i];
                    double fWeight = sqrt((fContrast - fLowestContrast)*(fContrast - fLowestContrast)) / fContrastRange;

                    // play with some randomization in weight
//                    fWeight += RANDDOUBLE(0.0, 2.0);

                    fAccumulatedR += ((double)nSourceR) * fWeight;
                    fAccumulatedG += ((double)nSourceG) * fWeight;
                    fAccumulatedB += ((double)nSourceB) * fWeight;
                    fAccumulatedWeights += fWeight;
                }

                fAccumulatedR /= (fAccumulatedWeights);
                fAccumulatedG /= (fAccumulatedWeights);
                fAccumulatedB /= (fAccumulatedWeights);

                uint32_t nFinalCol = ARGB(0xff, (uint8_t)fAccumulatedR, (uint8_t)fAccumulatedG, (uint8_t)fAccumulatedB);

                pJP->pDestImage->SetPixel(x, y, nFinalCol);
            }
        }

    }
}



void cProcessImageWin::CopyPixelsInRadius(ZBuffer* pSourceBuffer, ZBuffer* pDestBuffer, int64_t nX, int64_t nY, int64_t nRadius)
{
	if (nRadius < 1)
		return;

	double fMinDistanceSquared = (double)nRadius * nRadius;

	// These are all of the pixels encompasing the circle (middle of circle to center of any square side being the radius)
	for (int64_t y = nY - nRadius; y < nY + nRadius; y++)
	{
		for (int64_t x = nX - nRadius; x < nX + nRadius; x++)
		{
/*            if (x == nX && y == nY)
            {
                pDestBuffer->SetPixel(x, y, pSourceBuffer->GetPixel(x, y));
                continue;
            }
*/
			double fDistanceSquared = (double)(x - nX) * (double)(x - nX) + (double)(y - nY) * (double)(y - nY);

			if (fDistanceSquared > fMinDistanceSquared)	// ensures this is within a circular radius
				continue;

			double fWeight = sqrt(fDistanceSquared) / (double)nRadius;

			uint32_t nSourceCol = pSourceBuffer->GetPixel(x, y);
			uint32_t nDestCol = pDestBuffer->GetPixel(x, y);

			uint32_t nSourceR = ARGB_R(nSourceCol);
			uint32_t nSourceG = ARGB_G(nSourceCol);
			uint32_t nSourceB = ARGB_B(nSourceCol);

			uint32_t nDestR = ARGB_R(nDestCol);
			uint32_t nDestG = ARGB_G(nDestCol);
			uint32_t nDestB = ARGB_B(nDestCol);


			uint32_t nResultR = (uint32_t) (((double)nDestR* fWeight) + ((double)nSourceR * (1.0-fWeight))/1.0);
			uint32_t nResultG = (uint32_t) (((double)nDestG* fWeight) + ((double)nSourceG * (1.0-fWeight))/1.0);
			uint32_t nResultB = (uint32_t) (((double)nDestB* fWeight) + ((double)nSourceB * (1.0-fWeight))/1.0);

			pDestBuffer->SetPixel(x, y, ARGB(0xff, nResultR, nResultG, nResultB));
		}
	}
}


double cProcessImageWin::ComputePixelContrast(ZBuffer* pBuffer, int64_t nX, int64_t nY, int64_t nRadius)
{
	if (nRadius < 1)
		return 0xff;

	// Set boundaries to max
	uint8_t nLowestR = 0xff;
	uint8_t nLowestG = 0xff;
	uint8_t nLowestB = 0xff;
	uint8_t nHighestR = 0;
	uint8_t nHighestG = 0;
	uint8_t nHighestB = 0;

	double fMinDistanceSquared = (double) nRadius*nRadius;

	// These are all of the pixels encompasing the circle (middle of circle to center of any square side being the radius)
	for (int64_t y = nY - nRadius; y <= nY + nRadius; y++)
	{
		for (int64_t x = nX - nRadius; x <= nX + nRadius; x++)
		{
			double fDistanceSquared = (double) (x-nX) * (double) (x-nX ) + (double) (y-nY) * (double) (y-nY);

			if (fDistanceSquared > fMinDistanceSquared)	// ensures this is within a circular radius
				continue;

			uint32_t nCol = pBuffer->GetPixel(x, y);

			uint8_t nCurR = ARGB_R(nCol);
			uint8_t nCurG = ARGB_G(nCol);
			uint8_t nCurB = ARGB_B(nCol);

			if (nCurR < nLowestR)
				nLowestR = nCurR;
			if (nCurG < nLowestG)
				nLowestG = nCurG;
			if (nCurB < nLowestB)
				nLowestB = nCurB;

			if (nCurR > nHighestR)
				nHighestR = nCurR;
			if (nCurG > nHighestG)
				nHighestG = nCurG;
			if (nCurB > nHighestB)
				nHighestB = nCurB;
		}
	}

	double fRDelta = nHighestR - nLowestR;
	double fGDelta = nHighestG - nLowestG;
	double fBDelta = nHighestB - nLowestB;



	double fContrast = sqrt(fRDelta*fRDelta + fGDelta*fGDelta + fBDelta*fBDelta);
/*	{
		ZDEBUG_OUT("contrast:%f\n", fContrast);
	}*/

	return fContrast;
}

void  cProcessImageWin::ComputeContrast(void* pContext)
{
    JobParams* pJP = (JobParams*)pContext;

    for (int64_t y = pJP->rArea.top; y < pJP->rArea.bottom; y++)
    {
        for (int64_t x = pJP->rArea.left; x < pJP->rArea.right; x++)
        {
            double fContrast = pJP->pThis->ComputePixelContrast(pJP->pSourceImage, x, y, pJP->nRadius);
            int64_t nIndex = (y * pJP->pThis->mrOriginalImagesArea.Width()) + x;
            pJP->pThis->mpContrastFloatBuffer[nIndex] = fContrast;

            if (fContrast > pJP->pThis->mfHighestContrast)
                pJP->pThis->mfHighestContrast = fContrast;
		}
	}

    // Is there a sync point?
    if (pJP->mpBarrierCount)
    {
        (*pJP->mpBarrierCount)--;
        while (*pJP->mpBarrierCount > 0)
            std::this_thread::sleep_for(std::chrono::microseconds(50));
    }




    for (int64_t y = pJP->rArea.top; y < pJP->rArea.bottom; y++)
    {
        for (int64_t x = pJP->rArea.left; x < pJP->rArea.right; x++)
        {
            int64_t nIndex = (y * pJP->pThis->mrOriginalImagesArea.Width()) + x;
            double fContrast = (255.0 * pJP->pThis->mpContrastFloatBuffer[nIndex] / pJP->pThis->mfHighestContrast);
            uint8_t nContrast = (uint8_t)fContrast;
            uint32_t nResultColor = ARGB(0xff, nContrast, nContrast, nContrast);
            pJP->pDestImage->SetPixel(x, y, nResultColor);
        }
    }
}

bool cProcessImageWin::Init()
{
	SetFocus();

    int64_t panelW = grFullArea.Width() / 10;
    int64_t panelH = grFullArea.Height() / 4;
    ZRect rControlPanel(grFullArea.right-panelW, grFullArea.bottom-panelH, grFullArea.right, grFullArea.bottom);     // upper right for now

    ZWinControlPanel* pCP = new ZWinControlPanel();
    pCP->SetArea(rControlPanel);
//    pCP->SetTriggerRect(grControlPanelTrigger);

    pCP->Init();

    pCP->AddButton("Load Images", "type=loadimages;target=imageprocesswin");
    pCP->AddButton("Radius Blur", "type=radiusblur;target=imageprocesswin");
    pCP->AddButton("Stack", "type=stackimages;target=imageprocesswin");
    pCP->AddButton("compute contrast", "type=computecontrast;target=imageprocesswin");
    pCP->AddSlider(&mnProcessPixelRadius, 1, 50, 1, "", true, false, 1);

    pCP->AddButton("compute gradients", "type=computegradients;target=imageprocesswin");
    pCP->AddSlider(&mnGradientLevels, 1, 50, 1, "", true, false, 1);

    ChildAdd(pCP);


    ZRect rWatchPanel(rControlPanel);
    rWatchPanel.OffsetRect(-rWatchPanel.Width() + 8, 0);
    ZWinWatchPanel* pWP = new ZWinWatchPanel();
    pWP->SetArea(rWatchPanel);
    pWP->Init();
    pWP->AddItem(WatchType::kLabel, "Watch Panel", nullptr, 3, 0xff000000, 0xff000000, ZFont::kEmbossed);
    pWP->AddItem(WatchType::kInt64, "Width", (void*)&mnSelectedImageW, 2, 0xff333333, 0xff333333);
    pWP->AddItem(WatchType::kInt64, "Height", (void*)&mnSelectedImageH, 2, 0xff333333, 0xff333333);
    ChildAdd(pWP);

    Process_SelectImage(0);
	return ZWin::Init();
}

void cProcessImageWin::ResetResultsBuffer()
{
	if (mImagesToProcess.empty())
	{
		mpResultBuffer = nullptr;
		return;
	}

//    ZScreenBuffer* pScreenBuffer = gGraphicSystem.GetScreenBuffer();
//    pScreenBuffer->EnableRendering(false);


	ZASSERT(mrOriginalImagesArea.Width() > 0 && mrOriginalImagesArea.Height() > 0);
    // if there is a result buffer and the dimenstions already match, no need to do anything with it
    if (!mpResultBuffer || mpResultBuffer->GetArea().Width() != mrOriginalImagesArea.Width() || mpResultBuffer->GetArea().Height() != mrOriginalImagesArea.Height())
    {
        mpResultBuffer.reset(new ZBuffer());
        mpResultBuffer->Init(mrOriginalImagesArea.Width(), mrOriginalImagesArea.Height());

        if (mpResultWin)
            ChildDelete(mpResultWin);

        mpResultWin = new ZImageWin();
        mpResultWin->SetArea(mrResultImageDest);
        mpResultWin->SetImage(mpResultBuffer.get());
        mpResultWin->SetShowZoom(4, 0x44ffffff, ZFont::kBottomRight, true);
        mpResultWin->SetImage(mpResultBuffer.get());
        mpResultWin->SetFill(0xff222222);
        mpResultWin->SetArea(mrResultImageDest);
        mpResultWin->SetZoomable(true, 0.05, 100.0);
        ChildAdd(mpResultWin);

    }


    if (mpContrastFloatBuffer)
        delete[] mpContrastFloatBuffer;

    mpContrastFloatBuffer = new double[mrOriginalImagesArea.Width() * mrOriginalImagesArea.Height()];
    memset(mpContrastFloatBuffer, 0, mrOriginalImagesArea.Width() * mrOriginalImagesArea.Height() * sizeof(double));

//    pScreenBuffer->EnableRendering(true);
}



bool cProcessImageWin::HandleMessage(const ZMessage& message)
{
	string sType = message.GetType();

	if (sType == "loadimages")
	{
		Process_LoadImages();
		return true;
	}
    else if (sType == "selectimg")
    {
        Process_SelectImage((int32_t) StringHelpers::ToInt(message.GetParam("index")));
        return true;
    }
    else if (sType == "radiusblur")
    {
        SpawnWork(&RadiusBlur);
        return true;
    }
	else if (sType == "stackimages")
	{
        SpawnWork(&StackImages, true);
		return true;
	}
	else if (sType == "computecontrast")
	{
        SpawnWork(&ComputeContrast, true);
        return true;
	}
    else if (sType == "computegradients")
    {
        Process_ComputeGradient();
        return true;
    }

	return ZWin::HandleMessage(message);
}


bool cProcessImageWin::Shutdown()
{
	return ZWin::Shutdown();
}

bool cProcessImageWin::BlurBox(int64_t x, int64_t y)
{
    const int kDist = 10;

    // translate mouse into result buffer space
    ZRect rResultImageRect(mpResultBuffer->GetArea());
    int64_t nResultX = ((x - mrResultImageDest.left) * rResultImageRect.Width()) / mrResultImageDest.Width();
    int64_t nResultY = ((y - mrResultImageDest.top) * rResultImageRect.Height()) / mrResultImageDest.Height();

    OutputDebugLockless("nResultX:%d, nResultY:%d\n", nResultX, nResultY);

    if (rResultImageRect.PtInRect(nResultX - kDist, nResultY - kDist) &&
        rResultImageRect.PtInRect(nResultX + kDist, nResultY - kDist) &&
        rResultImageRect.PtInRect(nResultX + kDist, nResultY + kDist) &&
        rResultImageRect.PtInRect(nResultX - kDist, nResultY + kDist))
    {
        double fContrast = ComputePixelContrast(mpResultBuffer.get(), nResultX, nResultY, 10);
        int nAdjustedRect = fContrast / 26;

        ZRect r(nResultX - kDist, nResultY - kDist, nResultX + kDist, nResultY + kDist);
        uint32_t nCol = ComputeAverageColor(mpResultBuffer.get(), r);


        ZRect rDraw(nResultX - nAdjustedRect, nResultY - nAdjustedRect, nResultX + nAdjustedRect, nResultY + nAdjustedRect);
        tColorVertexArray verts;
        gRasterizer.RectToVerts(rDraw, verts);


        verts[0].mColor = mpResultBuffer.get()->GetPixel(nResultX - nAdjustedRect, nResultY - nAdjustedRect);
        verts[1].mColor = mpResultBuffer.get()->GetPixel(nResultX + nAdjustedRect, nResultY - nAdjustedRect);
        verts[2].mColor = mpResultBuffer.get()->GetPixel(nResultX + nAdjustedRect, nResultY + nAdjustedRect);
        verts[3].mColor = mpResultBuffer.get()->GetPixel(nResultX - nAdjustedRect, nResultY + nAdjustedRect);


        /*            verts[0].mColor = nCol;
                    verts[1].mColor = nCol;
                    verts[2].mColor = nCol;
                    verts[3].mColor = nCol;*/

        gRasterizer.Rasterize(mpResultBuffer.get(), mpResultBuffer.get(), verts);

        InvalidateChildren();
    }
    return true;
}

bool cProcessImageWin::OnMouseMove(int64_t x, int64_t y)
{
//        BlurBox(x,y);

    return ZWin::OnMouseMove(x,y);
}

bool cProcessImageWin::OnMouseUpL(int64_t x, int64_t y)
{
    ReleaseCapture();

    return ZWin::OnMouseUpL(x,y);
}

bool cProcessImageWin::OnMouseDownL(int64_t x, int64_t y)
{
    if (mrResultImageDest.PtInRect(x, y))
    {
        if (SetCapture())
        {
            SetMouseDownPos(x,y);
            return true;
        }
    }
	return ZWin::OnMouseDownL(x,y);
}

bool cProcessImageWin::OnChar(char)
{
	return true;
}

bool cProcessImageWin::OnKeyDown(uint32_t key)
{
	if (key == VK_ESCAPE)
	{
		gMessageSystem.Post("quit_app_confirmed");
	}

	return true;
}


double LimitUV(double f, double fMin, double fMax)
{
    if (f < fMin)
        return fMin;
    else if (f > fMax)
        return fMax;

    return f;
}

bool cProcessImageWin::Paint()
{
    if (!mbInvalid)
        return true;

    const std::lock_guard<std::mutex> surfaceLock(mpTransformTexture->GetMutex());

    mpTransformTexture->Fill(mAreaToDrawTo, 0xff000000);

    std::shared_ptr<ZFont> pFont(gpFontSystem->GetDefaultFont(4));
	pFont->DrawText(mpTransformTexture, msResult, mrResultImageDest, 0x88ffffff, 0x88888888);


	return ZWin::Paint();
}


void cProcessImageWin::ProcessImages()
{
}

