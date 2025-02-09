#include "ProcessImageWin.h"
#include "ZBuffer.h"
#include "ZTypes.h"
#include "ZMessageSystem.h"
#include "ZAnimObjects.h"
#include "ZGraphicSystem.h"
#include "ZScreenBuffer.h"
#include "Resources.h"
#include "ZStringHelpers.h"
//#include "ZWinScriptedDialog.h"
#include "ZWinBtn.H"
#include "ZWinSlider.h"
#include "ZWinImage.h"
#include "ZWinControlPanel.h"
#include "ZWinWatchPanel.h"
#include "ZTimer.h"
#include "ZRandom.h"
#include "ZFont.h"
#include "Resources.h"
#include "helpers/StringHelpers.h"
#include "helpers/Registry.h"
#include "ZWinFileDialog.h"
#include "ZWinText.H"
#include "ZGUIStyle.h"


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

    tZBufferPtr pSourceImage;
    tZBufferPtr pDestImage;

    std::thread         worker;
    atomic<int64_t>*    mpBarrierCount;
};

cProcessImageWin::cProcessImageWin()
{
	mbAcceptsFocus = true;
	mbAcceptsCursorMessages = true;
	mpResultBuffer = nullptr;
    mpContrastFloatBuffer = nullptr;
	mnProcessPixelRadius = 2;
    mnGradientLevels = 1;
    mnSubdivisionLevels = 4;
    mrThumbnailSize.SetRect(0, 0, 32, 32);  // initial size
    mpResultWin = nullptr;

    mThreads = 32;

	msWinName = "imageprocesswin";
}

cProcessImageWin::~cProcessImageWin()
{
    delete mpContrastFloatBuffer;
}

bool cProcessImageWin::RemoveImage(const std::string& sFilename)
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());
    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);

    for (auto pWin : mChildImageWins)
    {
        if (pWin->msWinName == sFilename)
        {
            ChildDelete(pWin);
            mChildImageWins.remove(pWin);
            ResetResultsBuffer();
            InvalidateChildren();
            return true;
        }
    }

    return false;
}

bool cProcessImageWin::ClearImages()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());
    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);
    for (auto pWin : mChildImageWins)
    {
        ChildDelete(pWin);
    }
    mChildImageWins.clear();
    ResetResultsBuffer();
    InvalidateChildren();
    return true;
}


bool cProcessImageWin::LoadImages(std::list<string>& filenames)
{
    if (filenames.empty())
        return true;

    ZScreenBuffer* pScreenBuffer = gGraphicSystem.GetScreenBuffer();
    pScreenBuffer->EnableRendering(false);


    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());
//    mImagesToProcess.clear();

    const std::lock_guard<std::recursive_mutex> lock(mChildListMutex);

    int32_t nNumImages = (int32_t)filenames.size();

    for (auto filename : filenames)
    {
        bool bAlreadyLoaded = false;
        // check that this image isn't already loaded
        for (auto pImageWins : mChildImageWins)
        {
            if (pImageWins->msWinName == filename)
            {
                bAlreadyLoaded = true;
                break;
            }
        }

        if (!bAlreadyLoaded)
        {
            ZWinImage* pOriginalImageWin = new ZWinImage();
            pOriginalImageWin->SetArea(mrThumbnailSize);
            if (pOriginalImageWin->LoadImage(filename))
            {
                pOriginalImageWin->msWinName = filename;

                pOriginalImageWin->msMouseUpLMessage = ZMessage("selectimg", "name", filename, "target", "imageprocesswin");
                pOriginalImageWin->msCloseButtonMessage = ZMessage("closeimg", "name", filename, "target", "imageprocesswin");

                pOriginalImageWin->mFillColor = 0xff000000;
                ChildAdd(pOriginalImageWin);

                mChildImageWins.push_back(pOriginalImageWin);
            }
            else
            {
                delete pOriginalImageWin;   // failed to load image....
            }
        }
    }

    tZBufferPtr pImage = (*mChildImageWins.begin())->mpImage;
    if (pImage)
        mrIntersectionWorkArea.SetRect(pImage->GetArea());  // set the work area to the first image to generate the result buffer
    ResetResultsBuffer();
    Process_SelectImage(*filenames.begin());

    pScreenBuffer->EnableRendering(true);

    InvalidateChildren();

	return true;
}

void cProcessImageWin::Process_LoadImages()
{
    std::list<string> filenames;

    if (ZWinFileDialog::ShowMultiLoadDialog("Supported Image Formats", "*.jpg;*.jpeg;*.bmp;*.gif;*.png;*.tiff;*.exif;*.wmf;*.emf", filenames ))
    {
        if (!filenames.empty())
        {
            LoadImages(filenames);
        }
    }
}

void cProcessImageWin::UpdateImageProps(ZBuffer* pBuf)
{
    if (!mpImageProps)
        return;
    mpImageProps->Clear();

    if (!pBuf)
        return;


    mpImageProps->SetScrollable();
}

void cProcessImageWin::Process_SelectImage(string sImageName)
{
    for (auto pWin : mChildImageWins)
    {
        if (pWin->msWinName == sImageName)
        {
            if (pWin->mpImage)
            {
                mrIntersectionWorkArea.SetRect(pWin->mpImage.get()->GetArea());

                ZRect r(mrIntersectionWorkArea);
                mpResultBuffer.get()->GetMutex().lock();
                mpResultBuffer.get()->Init(r.Width(), r.Height());
                mpResultBuffer.get()->Blt(pWin->mpImage.get(), r, r, &r);
                mpResultBuffer.get()->GetMutex().unlock();

                mpResultWin->SetImage(mpResultBuffer);

                UpdateImageProps(pWin->mpImage.get());
            }

            InvalidateChildren();
            return;
        }
    }

    ZASSERT(false); // couldn't find image with that name
}

void cProcessImageWin::Process_SaveResultImage()
{
    if (!mpResultBuffer)
    {
        ZASSERT(false);
        return;
    }

    string sFileName;

    if (ZWinFileDialog::ShowSaveDialog("Supported Image Formats", "*.jpg;*.jpeg;*.bmp;*.gif;*.png;*.tiff;*.tif", sFileName))
    {
        if (!sFileName.empty())
        {
            mpResultBuffer->SaveBuffer(sFileName);
            return;
        }
    }

    ZASSERT(false); // couldn't find image with that name
}

uint32_t cProcessImageWin::ComputeAverageColor(tZBufferPtr pBuffer, ZRect rArea)
{
    uint64_t nTotalR = 0;
    uint64_t nTotalG = 0;
    uint64_t nTotalB = 0;
    for (int32_t y = (int32_t)rArea.top; y < (int32_t)rArea.bottom; y++)
    {
        for (int32_t x = (int32_t)rArea.left; x < (int32_t)rArea.right; x++)
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

bool cProcessImageWin::ComputeAverageColor(ZFloatColorBuffer* pBuffer, ZRect rArea, ZFColor& fCol)
{
    if (rArea.Width() < 1 || rArea.Height() < 1)
        return false;

    fCol.a = 0.0;
    fCol.r = 0.0;
    fCol.g = 0.0;
    fCol.b = 0.0;

    for (int64_t y = rArea.top; y < rArea.bottom; y++)
    {
        for (int64_t x = rArea.left; x < rArea.right; x++)
        {
            ZFColor c;
            pBuffer->GetPixel(x, y, c);
            fCol += c;
        }
    }

    double fPixels = (double)rArea.Width() * (double)rArea.Height();

    fCol *= (1.0 / fPixels);    // divide by total number of pixels
    fCol.a = 255.0;

    return true;
}


void cProcessImageWin::Process_ComputeGradient()
{
    int64_t nSubdivisions = /*1 <<*/ mnGradientLevels;

    double fSubW = (double) mpResultBuffer->GetArea().Width() / (double) nSubdivisions;
    double fSubH = (double) mpResultBuffer->GetArea().Height() / (double) nSubdivisions;


    uint32_t* subdivisionColorGrid = new uint32_t[nSubdivisions * nSubdivisions];

    for (int64_t nGridY = 0; nGridY < nSubdivisions; nGridY++)
    {
        for (int64_t nGridX = 0; nGridX < nSubdivisions; nGridX++)
        {
            ZRect rSubArea((int64_t)(nGridX * fSubW), (int64_t)(nGridY * fSubH), (int64_t)((nGridX+1)* fSubW), (int64_t)((nGridY+1) * fSubH));

            int64_t nIndex = (nGridY * nSubdivisions) + nGridX;
            subdivisionColorGrid[nIndex] = ComputeAverageColor(mpResultBuffer, rSubArea);
        }
    }

    for (int64_t nGridY = 0; nGridY < nSubdivisions; nGridY++)
    {
        for (int64_t nGridX = 0; nGridX < nSubdivisions; nGridX++)
        {
//            if (nGridX == 1 && nGridY%2 == 1)
            {
                ZRect rSubArea((int64_t)(nGridX * fSubW), (int64_t)(nGridY *fSubH), (int64_t)((nGridX + 1) * fSubW), (int64_t)((nGridY + 1) * fSubH));
//                tColorVertexArray verts;
//                gRasterizer.RectToVerts(rSubArea, verts);

                int64_t nIndex = (nGridY * nSubdivisions) + nGridX; // TL
//                verts[0].mColor = subdivisionColorGrid[nIndex];
//                verts[1].mColor = subdivisionColorGrid[nIndex];
//                verts[2].mColor = subdivisionColorGrid[nIndex];
//                verts[3].mColor = subdivisionColorGrid[nIndex];
//                gRasterizer.Rasterize(mpResultBuffer.get(), mpResultBuffer.get(), verts);

                uint32_t nCol = subdivisionColorGrid[nIndex];

                nCol = 0x80000000 | (nCol & 0x00ffffff); // 50% alpha

                mpResultBuffer.get()->FillAlpha(nCol, &rSubArea);

            }
        }
    }
    delete[] subdivisionColorGrid;
    InvalidateChildren();
}

bool cProcessImageWin::Subdivide_and_Subtract(ZFloatColorBuffer* pBuffer, ZRect rArea, int64_t nMinSize, tFloatAreas& floatAreas)
{
    int64_t nSubW = rArea.Width() / 2;
    int64_t nSubH = rArea.Height() / 2;

    // Too small to sibdivide?
    if (nSubW < nMinSize || nSubH < nMinSize)
        return false;


    cFloatAreaDescriptor tl;
    cFloatAreaDescriptor tr;
    cFloatAreaDescriptor bl;
    cFloatAreaDescriptor br;

    tl.rArea = ZRect(rArea.left, rArea.top, rArea.left + nSubW, rArea.top + nSubH);
    tr.rArea = ZRect(rArea.left + nSubW, rArea.top, rArea.right, rArea.top + nSubH);

    bl.rArea = ZRect(rArea.left, rArea.top + nSubH, rArea.left + nSubW, rArea.bottom);
    br.rArea = ZRect(rArea.left + nSubW, rArea.top + nSubH, rArea.right, rArea.bottom);

    ComputeAverageColor(&mFloatColorBuffer, tl.rArea, tl.fCol);
    ComputeAverageColor(&mFloatColorBuffer, tr.rArea, tr.fCol);
    ComputeAverageColor(&mFloatColorBuffer, bl.rArea, bl.fCol);
    ComputeAverageColor(&mFloatColorBuffer, br.rArea, br.fCol);


    tl.fCol *= -0.25;
    tr.fCol *= -0.25;
    bl.fCol *= -0.25;
    br.fCol *= -0.25;

    mFloatColorBuffer.AddRect(tl.rArea, tl.fCol);
    mFloatColorBuffer.AddRect(tr.rArea, tr.fCol);
    mFloatColorBuffer.AddRect(bl.rArea, bl.fCol);
    mFloatColorBuffer.AddRect(br.rArea, br.fCol);

    floatAreas.push_back(tl);
    floatAreas.push_back(tr);
    floatAreas.push_back(bl);
    floatAreas.push_back(br);

    Subdivide_and_Subtract(pBuffer, tl.rArea, nMinSize, floatAreas);
    Subdivide_and_Subtract(pBuffer, tr.rArea, nMinSize, floatAreas);
    Subdivide_and_Subtract(pBuffer, bl.rArea, nMinSize, floatAreas);
    Subdivide_and_Subtract(pBuffer, br.rArea, nMinSize, floatAreas);

    return true;

}

void cProcessImageWin::Process_FloatColorSandbox()
{
    mFloatColorBuffer.From(mpResultBuffer.get());
    ZRect rArea(mFloatColorBuffer.GetBuffer().get()->GetArea());

/*    double a = 0.0;
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;

    for (int32_t y = 0; y < rArea.Height(); y++)
    {
        for (int32_t x = 0; x < rArea.Width(); x++)
        {
            uint32_t nCol = mpResultBuffer.get()->GetPixel(x, y);
            double da;
            double dr;
            double dg;
            double db;
            mFloatColorBuffer.ColToFloatCol(nCol, da, dr, dg, db);
            double dScale = (da + dr + dg + db) / (4.0 * 256.0);

            a += da * dScale;
            r += dr * dScale;
            g += dg * dScale;
            b += db * dScale;

//            mFloatColorBuffer.SetPixel(x, y, a, r, g, b);
            mFloatColorBuffer.SetPixel(x, y, 255.0, x%256, 255.0*sin((y+x)/314.0), y);
        }
    }*/

//    mFloatColorBuffer.MultRect(rArea, 0.75);
//    mFloatColorBuffer.AddRect(rArea, ZFColor(0xff, -10.0, 0.0, 0.0 ));

    tFloatAreas floatAreas;

    Subdivide_and_Subtract(&mFloatColorBuffer, rArea, rArea.Height() / mnSubdivisionLevels, floatAreas);

#define PRINT_EM

    ZFloatColorBuffer newBuffer;
    newBuffer.Init(rArea.Width(), rArea.Height());
    for (auto a : floatAreas)
    {
#ifdef PRINT_EM
//        ZOUT_LOCKLESS("r[%d,%d,%d,%d] = argb[%f,%f,%f,%f]\n", a.rArea.left, a.rArea.top, a.rArea.right, a.rArea.bottom, a.fCol.a, a.fCol.r, a.fCol.g, a.fCol.b);
        ZOUT_LOCKLESS("r[", a.rArea.left, ",", a.rArea.top, ",", a.rArea.right, ",", a.rArea.bottom, "] = argb[", a.fCol.a, ",", a.fCol.r, ",", a.fCol.g, ",", a.fCol.b, "]");
#endif
        newBuffer.AddRect(a.rArea, -a.fCol);
    }

    ZOUT_LOCKLESS("Rects:", floatAreas.size(), ", Memory:", floatAreas.size() * sizeof(cFloatAreaDescriptor));

//    mpResultBuffer.get()->Blt(&mFloatColorBuffer.GetBuffer(), rArea, rArea);
    mpResultBuffer.get()->Blt(newBuffer.GetBuffer().get(), rArea, rArea);

    InvalidateChildren();
}







bool cProcessImageWin::SpawnWork(void(*pProc)(void*), bool bBarrierSyncPoint)
{
    tZBufferPtr sourceImg(mpResultBuffer);
    ResetResultsBuffer();
    if (mChildImageWins.empty())
    {
//        pScreenBuffer->EnableRendering(true);
        return false;
    }

    int64_t nStartTime = gTimer.GetMSSinceEpoch();

    std::vector<JobParams> workers;
    workers.resize(mThreads);

    std::atomic<int64_t> nBarrierSync = mThreads;
        
    int64_t nTop = mnProcessPixelRadius;        // start radius pixels down
    int64_t nLines = mrIntersectionWorkArea.Height() / mThreads;

    mfHighestContrast = 0.0;

    for (uint32_t i = 0; i < mThreads; i++)
    {
        int64_t nBottom = nTop+nLines;

        // last thread? take the rest of the scanlines
        if (i == mThreads - 1)
            nBottom = mrIntersectionWorkArea.bottom - mnProcessPixelRadius;

        if (nBottom > mrIntersectionWorkArea.bottom - mnProcessPixelRadius)
            nBottom = mrIntersectionWorkArea.bottom - mnProcessPixelRadius;

        workers[i].rArea.SetRect(mrIntersectionWorkArea.left+mnProcessPixelRadius, nTop, mrIntersectionWorkArea.right-mnProcessPixelRadius, nBottom);
        workers[i].nRadius = mnProcessPixelRadius;
        workers[i].pSourceImage = sourceImg;
        workers[i].pDestImage = mpResultBuffer;
        workers[i].pThis = this;
        if (bBarrierSyncPoint)
            workers[i].mpBarrierCount = &nBarrierSync;

        workers[i].worker = std::thread(pProc, (void*)&workers[i]);

        nTop += nLines;
    }

    for (uint32_t i = 0; i < mThreads; i++)
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

    ZBuffer blur;
    blur.Init(pJP->rArea.Width(), pJP->rArea.Height());


    for (int64_t y = pJP->rArea.top; y < pJP->rArea.bottom; y++)
    {
        for (int64_t x = pJP->rArea.left; x < pJP->rArea.right; x++)
        {
            //pJP->pDestImage->SetPixel(x, y, pJP->pThis->ComputePixelBlur(pJP->pSourceImage, x, y, pJP->nRadius));
            int64_t nXTemp = x - pJP->rArea.left;
            int64_t nYTemp = y - pJP->rArea.top;
            blur.SetPixel(nXTemp, nYTemp, pJP->pThis->ComputePixelBlur(pJP->pSourceImage, x, y, pJP->nRadius));
        }
    }

    if (pJP->mpBarrierCount)
    {
        (*pJP->mpBarrierCount)--;
        while (*pJP->mpBarrierCount > 0)
            std::this_thread::sleep_for(std::chrono::microseconds(50));
    }

    pJP->pDestImage->Blt(&blur, blur.GetArea(), pJP->rArea);
}

uint32_t cProcessImageWin::ComputePixelBlur(tZBufferPtr pBuffer, int64_t nX, int64_t nY, int64_t nRadius)
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
                nResultR += (uint32_t)(((nCurR) / fDistance));
                nResultG += (uint32_t)(((nCurG) / fDistance));
                nResultB += (uint32_t)(((nCurB) / fDistance));
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

    nResultR = (uint32_t)(nResultR/fPixelsInRadius);
    nResultG = (uint32_t)(nResultG/fPixelsInRadius);
    nResultB = (uint32_t)(nResultB/fPixelsInRadius);



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

void cProcessImageWin::NegativeImage(void* pContext)
{
    JobParams* pJP = (JobParams*)pContext;

    for (int64_t y = pJP->rArea.top; y < pJP->rArea.bottom; y++)
    {
        for (int64_t x = pJP->rArea.left; x < pJP->rArea.right; x++)
        {
            uint32_t nCol = *(pJP->pDestImage->GetPixels() + (y * pJP->pThis->mrIntersectionWorkArea.Width()) + x);

            uint8_t nR = 255-ARGB_R(nCol);
            uint8_t nG = 255-ARGB_G(nCol);
            uint8_t nB = 255-ARGB_B(nCol);

            *(pJP->pDestImage->GetPixels() + (y * pJP->pThis->mrIntersectionWorkArea.Width()) + x) = ARGB(0xff, nR, nG, nB);
        }
    }

}

void cProcessImageWin::Mono(void* pContext)
{
    JobParams* pJP = (JobParams*)pContext;

    for (int64_t y = pJP->rArea.top; y < pJP->rArea.bottom; y++)
    {
        for (int64_t x = pJP->rArea.left; x < pJP->rArea.right; x++)
        {
            uint32_t nCol = *(pJP->pDestImage->GetPixels() + (y * pJP->pThis->mrIntersectionWorkArea.Width()) + x);

            uint32_t nV = (ARGB_R(nCol) + ARGB_G(nCol) + ARGB_B(nCol)) / 3;

            *(pJP->pDestImage->GetPixels() + (y * pJP->pThis->mrIntersectionWorkArea.Width()) + x) = ARGB(0xff, nV, nV, nV);
        }
    }
}


void cProcessImageWin::StackImages(void* pContext)
{
    JobParams* pJP = (JobParams*)pContext;

    int32_t nImages = (int32_t)pJP->pThis->mChildImageWins.size();

    if (nImages < 1)
        return;

    std::vector<tZBufferPtr> images;
    for (auto pWin : pJP->pThis->mChildImageWins)
    {
        images.push_back(pWin->mpImage);
    }

    std::vector<double> imageContrasts;
    imageContrasts.resize(nImages);

    for (int64_t y = pJP->rArea.top; y < pJP->rArea.bottom; y++)
    {
        for (int64_t x = pJP->rArea.left; x < pJP->rArea.right; x++)
        {
            double fHighestContrast = 0.0;
            double fLowestContrast = 999999999;
//            int64_t nHighestContrastImageIndex = 0;

            int64_t nContrastIndex = 0;

            for (auto pBuffer : images)
            {
                double fContrast = pJP->pThis->ComputePixelContrast(pBuffer, x, y, pJP->nRadius);
                imageContrasts[nContrastIndex++] = fContrast;
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
                pJP->pDestImage->SetPixel(x, y, images[rand()%nImages].get()->GetPixel(x, y));
//                pJP->pDestImage->SetPixel(x, y, 0xffff00ff);
            }
            else
            {
                double fAccumulatedWeights = 0.0;
                for (int32_t i = 0; i < nImages; i++)
                {
                    uint32_t nCol = images[i].get()->GetPixel(x, y);
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



void cProcessImageWin::CopyPixelsInRadius(tZBufferPtr pSourceBuffer, tZBufferPtr pDestBuffer, int64_t nX, int64_t nY, int64_t nRadius)
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


double cProcessImageWin::ComputePixelContrast(tZBufferPtr pBuffer, int64_t nX, int64_t nY, int64_t nRadius)
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
            int64_t nIndex = (y * pJP->pThis->mrIntersectionWorkArea.Width()) + x;
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
            int64_t nIndex = (y * pJP->pThis->mrIntersectionWorkArea.Width()) + x;
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

    int64_t panelW = gM * 10;
    int64_t panelH = gM * 16;
    mrControlPanel.SetRect(grFullArea.right-panelW, grFullArea.bottom-panelH, grFullArea.right, grFullArea.bottom);     // upper right for now

    ZWinControlPanel* pCP = new ZWinControlPanel();
    pCP->SetArea(mrControlPanel);
//    pCP->SetTriggerRect(grControlPanelTrigger);

//    tZFontPtr pBtnFont(gpFontSystem->GetFont(gDefaultButtonFont));


    pCP->Init();

    pCP->Button("loadimages", "Load Image", "{loadimages;target=imageprocesswin}");
    pCP->Button("clearall", "Close All Images", "{clearall;target=imageprocesswin}");

    pCP->AddSpace(32);

    ZWinLabel* pLabel = pCP->Caption("computeradius", "Compute Radius");
    pLabel->msTooltipText= "Sets the radius for the operations below.";
    pLabel->mStyleTooltip = gStyleTooltip;

    pCP->Slider("processpixelradius", &mnProcessPixelRadius, 1, 50, 1, 0.25, "", true, false);

    pCP->Button("radiusblur", "Blur", "{radiusblur;target=imageprocesswin}");
    pCP->Button("stackimages", "Stack", "{stackimages;target=imageprocesswin}");
    pCP->Button("computecontrast", "Contrast", "{computecontrast;target=imageprocesswin}");

    pCP->AddSpace(32);
    pCP->Caption("ops","Ops");
    pCP->Button("negative", "Negative", "{negative;target=imageprocesswin}");
    pCP->Button("mono", "Monochrome", "{mono;target=imageprocesswin}");
    pCP->Button("resize", "resize_256x256", "{resize;target=imageprocesswin}");


    pCP->AddSpace(64);

    pCP->Caption("experiments","Experiments");
    pCP->Button("computegradients", "compute gradients", "{computegradients;target=imageprocesswin}");
    pCP->Slider("gradientlevels", &mnGradientLevels, 1, 50, 1, 0.25, "", true, false);

    pCP->AddSpace(16);

    pCP->Button("floatcolorsandbox", "float color sandbox", "{floatcolorsandbox;target=imageprocesswin}");
    pCP->Slider("subdivisionlevels", &mnSubdivisionLevels, 1, 512, 1, 0.25, "", true, false);


    ChildAdd(pCP);


    mrWatchPanel.SetRect(0,0,mrControlPanel.Width()*2, mrControlPanel.Height());
    mrWatchPanel.MoveRect(mrControlPanel.left-mrWatchPanel.Width() + gSpacer, mrControlPanel.top);
    ZWinWatchPanel* pWP = new ZWinWatchPanel();
    pWP->SetArea(mrWatchPanel);
    pWP->Init();
    pWP->AddItem(WatchType::kLabel, "Image Props", nullptr, ZGUI::ZTextLook(ZGUI::ZTextLook::kEmbossed, 0xff000000, 0xff000000));
    ChildAdd(pWP);

    ZRect rImageProps(gSpacer, gDefaultTextFont.Height()*2, mrWatchPanel.Width() - gSpacer, mrWatchPanel.Height() - gSpacer);

    mpImageProps = new ZWinFormattedDoc();
    mpImageProps->SetArea(rImageProps);
    mpImageProps->mStyle.bgCol = gDefaultTextAreaFill;
    mpImageProps->SetBehavior(ZWinFormattedDoc::kBackgroundFill, true);
    mpImageProps->SetScrollable();

    pWP->ChildAdd(mpImageProps);







    list<string> filenames;

    if (!gRegistry.Get("ProcessImageWin", "images", filenames) || filenames.empty())
    {
        filenames = list<string>{
            "res/414A2616.jpg",
            "res/414A2617.jpg",
            "res/414A2618.jpg",
            "res/414A2619.jpg",
            "res/414A2620.jpg",
            "res/414A2621.jpg",
            "res/414A2622.jpg",
            "res/414A2623.jpg",
            "res/414A2624.jpg"
        };
//        gRegistry.SetDefault("ProcessImageWin", "images", filenames); 
    }
   

    // remove any filenames with missing files
    list<string> checkedFilenames;
    for (auto filename : filenames)
    {
        if (filesystem::exists(filename))
            checkedFilenames.push_back(filename);
    }

    if (!checkedFilenames.empty())
    {
        LoadImages(checkedFilenames);
        Process_SelectImage(*checkedFilenames.begin());
    }

	return ZWin::Init();
}


void cProcessImageWin::ComputeIntersectedWorkArea()
{
    // Compute the intersection area (i.e. smallest rect that all images fit into)
    bool bFirstRect = true;
    for (auto pWin : mChildImageWins)
    {
        if (bFirstRect)
        {
            bFirstRect = false;
            mrIntersectionWorkArea.SetRect(pWin->mpImage->GetArea());
        }
        else
        {
            mrIntersectionWorkArea.IntersectRect(pWin->mpImage->GetArea());
        }
    }
    ZASSERT(mrIntersectionWorkArea.Width() > 0 && mrIntersectionWorkArea.Height() > 0);
}

void cProcessImageWin::ResetResultsBuffer()
{
    if (mChildImageWins.empty())
	{
        if (mpResultWin)
        {
            ChildDelete(mpResultWin);
            mpResultWin = nullptr;
        }
		mpResultBuffer = nullptr;
		return;
	}

    if (mrIntersectionWorkArea.Width() == 0 || mrIntersectionWorkArea.Height() == 0)
        return;

    // Arrange all thumbnails
    double fImageAspect = (double)mrIntersectionWorkArea.Width() / (double)mrIntersectionWorkArea.Height();
    int64_t nImageWidth = mAreaLocal.Width() / mChildImageWins.size();
    int64_t nImageHeight = (int64_t)(nImageWidth / fImageAspect);

    if (nImageHeight > mAreaLocal.Height() / 4)  // max out at 25% of the window
    {
        nImageHeight = mAreaLocal.Height() / 4;
        nImageWidth = (int64_t)(nImageHeight * fImageAspect);
    }


    mrThumbnailSize.SetRect(0, 0, nImageWidth, nImageHeight);
    for (auto pWin : mChildImageWins)
    {
        pWin->SetArea(mrThumbnailSize);
        mrThumbnailSize.OffsetRect(mrThumbnailSize.Width(), 0);

    }
    mrResultImageDest.SetRect(0, mrThumbnailSize.bottom, mrWatchPanel.left, grFullArea.bottom);


    if (!mpResultBuffer || mpResultBuffer->GetArea().Width() != mrIntersectionWorkArea.Width() || mpResultBuffer->GetArea().Height() != mrIntersectionWorkArea.Height())
    {
//        OutputDebugImmediate("new mpResultBuffer. old:0x%x\n", (void*)mpResultBuffer.get());
//        ZDEBUG_OUT_LOCKLESS("new mpResultBuffer. old:0x", std::hex, (void*)mpResultBuffer.get(), std::dec);
        mpResultBuffer.reset(new ZBuffer());
        mpResultBuffer->Init(mrIntersectionWorkArea.Width(), mrIntersectionWorkArea.Height());
    }

    // if there is a result buffer and the dimenstions already match, no need to do anything with it
    if (!mpResultWin || mrResultImageDest != mpResultWin->GetArea())
    {
        if (mpResultWin)
            ChildDelete(mpResultWin);

        mpResultWin = new ZWinImage();
        mpResultWin->SetArea(mrResultImageDest);
        mpResultWin->mFillColor = 0xff222222;
        mpResultWin->SetArea(mrResultImageDest);


        //        mpResultWin->SetShowZoom(gpFontSystem->GetFont(gDefaultTitleFont),ZTextLook(ZTextLook::kNormal, 0x44ffffff, 0x44ffffff), ZGUI::RB, true);
        mpResultWin->mfMinZoom = 0.05;
        mpResultWin->mfMaxZoom = 100.0;

        ChildAdd(mpResultWin);
    }

    mpResultWin->SetImage(mpResultBuffer);


    if (mpContrastFloatBuffer)
        delete[] mpContrastFloatBuffer;

    mpContrastFloatBuffer = new double[mrIntersectionWorkArea.Width() * mrIntersectionWorkArea.Height()];
    memset(mpContrastFloatBuffer, 0, mrIntersectionWorkArea.Width() * mrIntersectionWorkArea.Height() * sizeof(double));

//    pScreenBuffer->EnableRendering(true);
}



bool cProcessImageWin::HandleMessage(const ZMessage& message)
{
	string sType = message.GetType();

	if (sType == "loadimages")
	{
		Process_LoadImages();
        UpdatePrefs();
		return true;
	}
    else if (sType == "clearall")
    {
        ClearImages();
        UpdatePrefs();
        return true;
    }
    else if (sType == "closeimg")
    {
        RemoveImage(message.GetParam("name"));
        UpdatePrefs();
        return true;
    }
    else if (sType == "selectimg")
    {
        Process_SelectImage(message.GetParam("name"));
        return true;
    }
    else if (sType == "saveimg")
    {
        Process_SaveResultImage();
        return true;
    }
    else if (sType == "radiusblur")
    {
        SpawnWork(&RadiusBlur, true);
        return true;
    }
	else if (sType == "stackimages")
	{
        ComputeIntersectedWorkArea();
        SpawnWork(&StackImages, true);
		return true;
	}
    else if (sType == "negative")
    {
        SpawnWork(&NegativeImage, true);
        return true;
    }
    else if (sType == "mono")
    {
        SpawnWork(&Mono, true);
        return true;
    }
    else if (sType == "resize")
    {
        tZBufferPtr image256(new ZBuffer());
        image256->Init(256, 256);
        image256->Fill(0xff000000);

        image256->BltScaled(mpResultWin->mpImage.get());
        mpResultWin->SetImage(image256);
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
    else if (sType == "floatcolorsandbox")
    {
        Process_FloatColorSandbox();
        return true;
    }

	return ZWin::HandleMessage(message);
}

void cProcessImageWin::UpdatePrefs()
{
    std::list<string> filenames;
    for (auto pWin : mChildImageWins)
        filenames.push_back(pWin->msWinName);

    gRegistry["ProcessImageWin"]["images"] = filenames;
}


bool cProcessImageWin::Shutdown()
{
    UpdatePrefs();

	return ZWin::Shutdown();
}

bool cProcessImageWin::BlurBox(int64_t x, int64_t y)
{
    const int kDist = 10;

    // translate mouse into result buffer space
    ZRect rResultImageRect(mpResultBuffer->GetArea());
    int64_t nResultX = ((x - mrResultImageDest.left) * rResultImageRect.Width()) / mrResultImageDest.Width();
    int64_t nResultY = ((y - mrResultImageDest.top) * rResultImageRect.Height()) / mrResultImageDest.Height();

    ZDEBUG_OUT("nResultX:", nResultX, ", nResultY:", nResultY);

    if (rResultImageRect.PtInRect(nResultX - kDist, nResultY - kDist) &&
        rResultImageRect.PtInRect(nResultX + kDist, nResultY - kDist) &&
        rResultImageRect.PtInRect(nResultX + kDist, nResultY + kDist) &&
        rResultImageRect.PtInRect(nResultX - kDist, nResultY + kDist))
    {
        double fContrast = ComputePixelContrast(mpResultBuffer, nResultX, nResultY, 10);
        int nAdjustedRect = (int)(fContrast / 26);

        ZRect r(nResultX - kDist, nResultY - kDist, nResultX + kDist, nResultY + kDist);
        uint32_t nCol = ComputeAverageColor(mpResultBuffer, r);


        ZRect rDraw(nResultX - nAdjustedRect, nResultY - nAdjustedRect, nResultX + nAdjustedRect, nResultY + nAdjustedRect);
        tColorVertexArray verts;
        gRasterizer.RectToVerts(rDraw, verts);


        verts[0].mColor = mpResultBuffer.get()->GetPixel(nResultX - nAdjustedRect, nResultY - nAdjustedRect);
        verts[1].mColor = mpResultBuffer.get()->GetPixel(nResultX + nAdjustedRect, nResultY - nAdjustedRect);
        verts[2].mColor = mpResultBuffer.get()->GetPixel(nResultX + nAdjustedRect, nResultY + nAdjustedRect);
        verts[3].mColor = mpResultBuffer.get()->GetPixel(nResultX - nAdjustedRect, nResultY + nAdjustedRect);

        gRasterizer.Rasterize(mpResultBuffer.get(), verts);

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
            mMouseDownOffset.Set(x, y);
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
		gMessageSystem.Post("{quit_app_confirmed}");
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
    if (!PrePaintCheck())
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

    mpSurface.get()->Fill(0xff000000);

	return ZWin::Paint();
}


void cProcessImageWin::ProcessImages()
{
}

