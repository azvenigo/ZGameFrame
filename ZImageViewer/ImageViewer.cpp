#include "ZTypes.h"
#include "ZWin.h"
#include "ImageViewer.h"
#include "ZWinImage.h"
#include "Resources.h"
#include "helpers/StringHelpers.h"
#include "helpers/ThreadPool.h"
#include <algorithm>


using namespace std;


template<typename R>
bool is_ready(std::shared_future<R> const& f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

ThreadPool gPool(4);

ImageViewer::ImageViewer()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mIdleSleepMS = 13;
    mMaxMemoryUsage = 1 * 1024 * 1024 * 1024;   // 1GiB
    mAtomicIndex = 0;
}
 
ImageViewer::~ImageViewer()
{
}


bool ImageViewer::OnKeyDown(uint32_t key)
{
#ifdef _WIN64
    if (key == VK_ESCAPE)
    {
        gMessageSystem.Post("quit_app_confirmed");
    }
#endif

    switch (key)
    {
    case VK_LEFT:
        PrevImage();
        break;
    case VK_RIGHT:
        NextImage();
        break;
    case VK_HOME:
        FirstImage();
        break;
    case VK_END:
        LastImage();
        break;
    }


    return true;
}

bool ImageViewer::OnMouseWheel(int64_t x, int64_t y, int64_t nDelta)
{
    if (nDelta < 0)
        PrevImage();
    else
        NextImage();
    return ZWin::OnMouseWheel(x,y,nDelta);
}

void ImageViewer::FirstImage()
{
    if (mImagesInFolder.empty())
        return;

    mFilenameToLoad = *mImagesInFolder.begin();
    Preload();
}

void ImageViewer::LastImage()
{
    if (mImagesInFolder.empty())
        return;

    mFilenameToLoad = *mImagesInFolder.rbegin();
    Preload();
}

void ImageViewer::PrevImage()
{
    tImageFilenames::iterator selectedImage = std::find(mImagesInFolder.begin(), mImagesInFolder.end(), mSelectedFilename);

    if (selectedImage != mImagesInFolder.end() && selectedImage != mImagesInFolder.begin())
    {
        selectedImage--;
        mFilenameToLoad = *selectedImage;
    }

    Preload();
}

void ImageViewer::NextImage()
{
    tImageFilenames::iterator selectedImage = std::find(mImagesInFolder.begin(), mImagesInFolder.end(), mSelectedFilename);

    if (selectedImage != mImagesInFolder.end())
    {
        selectedImage++;
        if (selectedImage != mImagesInFolder.end())
            mFilenameToLoad = *selectedImage;
    }

    Preload();
}

bool ImageViewer::Init()
{
    if (!mbInitted)
    {
        SetFocus();
        mpWinImage = new ZWinImage();
        mpWinImage->SetArea(mAreaToDrawTo);
        mpWinImage->SetFill(0xff000000);
        mpWinImage->SetManipulationHotkey(VK_CONTROL);
        mpWinImage->SetZoomable(true, 0.05, 10.0);
        mpWinImage->SetShowZoom(gDefaultWinTextEditStyle.Font(), gDefaultWinTextEditStyle.look, ZGUI::CB, true);
        ChildAdd(mpWinImage);
        mpWinImage->SetFocus();
    }

    Preload();

    return ZWin::Init();
}

bool ImageViewer::ViewImage(const std::filesystem::path& filename)
{
    mFilenameToLoad = filename;
    Preload();
    return true;
}


tZBufferPtr ImageViewer::LoadImageProc(std::filesystem::path& imagePath, ImageViewer* pThis)
{
    tZBufferPtr pNewImage(new ZBuffer);
    if (!pNewImage->LoadBuffer(imagePath.string()))
    {
        cerr << "Error loading\n";
        return nullptr;
    }

    return pNewImage;
}


bool ImageViewer::InCache(const std::filesystem::path& imagePath)
{
    for (auto& p : mImageCache)
    {
        if (p.first == imagePath)
            return true;
    }

    return false;
}

int64_t ImageViewer::ImageIndexInFolder(std::filesystem::path imagePath)
{
    int64_t i = 1;
    for (auto& image : mImagesInFolder)
    {
        if (image == imagePath)
            return i;
        i++;
    }

    return 0;
}


tImageFuture* ImageViewer::GetCached(const std::filesystem::path& imagePath)
{
    for (auto& p : mImageCache)
    {
        if (p.first == imagePath)
            return &p.second;
    }

    return nullptr;
}


bool ImageViewer::AddToCache(std::filesystem::path imagePath)
{
    if (imagePath.empty())
        return false;

    if (InCache(imagePath))
        return true;

//    auto newPair = tNamedImagePair(imagePath, tImageFuture(std::move(std::async([=]() { ImageViewer::LoadImageProc(mSelectedFilename, this); }))));

    mImageCache.emplace_front( tNamedImagePair(imagePath, gPool.enqueue(&ImageViewer::LoadImageProc, imagePath, this)));
    return true;
}


int64_t ImageViewer::CurMemoryUsage()
{
    int64_t nLoaded = 0;
    for (auto& p : mImageCache)
    {
        if (is_ready(p.second))
        {
            ZRect rArea = p.second.get().get()->GetArea();
            nLoaded += rArea.Width() * rArea.Height() * 4;
        }
    }

    return nLoaded;
}


bool ImageViewer::Preload()
{
    if (mScannedFolder != mFilenameToLoad.parent_path())
    {
        mScannedFolder = mFilenameToLoad.parent_path();
        ScanForImagesInFolder(mScannedFolder);
    }

    tImageFilenames::iterator selectedImage = std::find(mImagesInFolder.begin(), mImagesInFolder.end(), mFilenameToLoad);

    if (selectedImage != mImagesInFolder.end())
    {
        if (AddToCache(*selectedImage))
        {


            // now preload the next image
            selectedImage++;
            if (selectedImage != mImagesInFolder.end())
            {
                AddToCache(*selectedImage);
            }
        }
    }

    while (CurMemoryUsage() > mMaxMemoryUsage && mImagesInFolder.size() > 1)
    {
        mImageCache.pop_back();
        ZDEBUG_OUT("Images:", mImageCache.size(), " memory:", CurMemoryUsage(), "\n");
    }

    return true;
}


bool ImageViewer::AcceptedExtension(std::string sExt)
{
    if (sExt.empty())
        return false;
    SH::makelower(sExt);
    if (sExt[0] == '.')
        sExt = sExt.substr(1);  // remove leading . if it's tehre

    for (auto s : mAcceptedExtensions)
    {
        if (sExt == s)
            return true;
    }

    return false;
}


bool ImageViewer::ScanForImagesInFolder(const std::filesystem::path& folder)
{
    bool bFolderScan = std::filesystem::is_directory(folder);

    mImagesInFolder.clear();

    for (auto filePath : std::filesystem::directory_iterator(folder))
    {
        if (filePath.is_regular_file() && AcceptedExtension(filePath.path().extension().string()))
        {
            mImagesInFolder.push_back(filePath);
            ZDEBUG_OUT("Found image:", filePath, "\n");
        }
    }

    return true;
}

bool ImageViewer::Process()
{
    if (mpWinImage && mFilenameToLoad != mSelectedFilename)
    {
        tImageFuture* pFuture = GetCached(mFilenameToLoad);
        if (pFuture && is_ready(*pFuture))
        {
            mSelectedFilename = mFilenameToLoad;
            mpWinImage->SetImage(pFuture->get());

            string sCaption = SH::FromInt(ImageIndexInFolder(mSelectedFilename)) + "/" + SH::FromInt(mImagesInFolder.size()) + " " + mSelectedFilename.string();

            mpWinImage->SetCaption(sCaption, gDefaultWinTextEditStyle.Font(), gDefaultWinTextEditStyle.look, ZGUI::LB);
        }
    }
    return ZWin::Process();
}

bool ImageViewer::Paint()
{
/*    if (!mbInvalid)
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());

    mpTransformTexture.get()->Fill(mAreaToDrawTo, 0xff003300);*/

    return ZWin::Paint();
}
   
