#include "ZTypes.h"
#include "ZWin.h"
#include "ImageViewer.h"
#include "ZWinImage.h"
#include "Resources.h"
#include "helpers/StringHelpers.h"
#include "ZStringHelpers.h"
#include "helpers/ThreadPool.h"
#include "ZWinFileDialog.h"
#include <algorithm>
#include "helpers/Registry.h"


using namespace std;


template<typename R>
bool is_ready(std::shared_future<R> const& f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

ThreadPool gPool(16);

ImageViewer::ImageViewer()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mIdleSleepMS = 13;
    mMaxMemoryUsage = 2 * 1024 * 1024 * 1024LL;   // 2GiB
    mMaxCacheReadAhead = 10;
    mAtomicIndex = 0;
    mLastAction = kNone;
    msWinName = "ZWinImageViewer";
}
 
ImageViewer::~ImageViewer()
{
}


bool ImageViewer::ShowOpenImageDialog()
{
    string sFilename;
    if (ZWinFileDialog::ShowLoadDialog("Images", "*.jpg;*.jpeg;*.png;*.gif;*.tga;*.bmp;*.psd;*.hdr;*.pic;*.pnm", sFilename))
    {
        mFilenameToLoad = sFilename;
        Preload();
    }
    return true;

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
        SetPrevImage();
        break;
    case VK_RIGHT:
        SetNextImage();
        break;
    case VK_HOME:
        SetFirstImage();
        break;
    case VK_END:
        SetLastImage();
        break;
    case 'o':
    case 'O':
        return ShowOpenImageDialog();
        break;
    }


    return true;
}

bool ImageViewer::OnMouseWheel(int64_t x, int64_t y, int64_t nDelta)
{
    if (nDelta < 0)
        SetPrevImage();
    else
        SetNextImage();
    return ZWin::OnMouseWheel(x,y,nDelta);
}

bool ImageViewer::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();

    if (sType == "loadimg")
    {
        ShowOpenImageDialog();
        return true;
    }
    if (sType == "saveimg")
    {
        string sFilename;
        if (ZWinFileDialog::ShowSaveDialog("Images", "*.jpg;*.jpeg;*.png;*.tga;*.bmp;*.hdr", sFilename))
            SaveImage(sFilename);
        return true;
    }

    return ZWin::HandleMessage(message);
}

bool ImageViewer::SaveImage(const std::filesystem::path& filename)
{
    if (filename.empty())
        return false;

    mpWinImage->mpImage.get()->SaveBuffer(filename.string());

}


void ImageViewer::SetFirstImage()
{
    if (mImagesInFolder.empty())
        return;

    mFilenameToLoad = *mImagesInFolder.begin();
    mLastAction = kBeginning;
    Preload();
}

void ImageViewer::SetLastImage()
{
    if (mImagesInFolder.empty())
        return;

    mFilenameToLoad = *mImagesInFolder.rbegin();
    mLastAction = kEnd;
    Preload();
}

void ImageViewer::SetPrevImage()
{
    tImageFilenames::iterator selectedImage = std::find(mImagesInFolder.begin(), mImagesInFolder.end(), mSelectedFilename);

    if (selectedImage != mImagesInFolder.end() && selectedImage != mImagesInFolder.begin())
    {
        selectedImage--;
        mFilenameToLoad = *selectedImage;
    }

    mLastAction = kPrevImage;
    Preload();
}

void ImageViewer::SetNextImage()
{
    tImageFilenames::iterator selectedImage = std::find(mImagesInFolder.begin(), mImagesInFolder.end(), mSelectedFilename);

    if (selectedImage != mImagesInFolder.end())
    {
        selectedImage++;
        if (selectedImage != mImagesInFolder.end())
            mFilenameToLoad = *selectedImage;
    }

    mLastAction = kNextImage;
    Preload();
}

bool ImageViewer::Init()
{
    if (!mbInitted)
    {
        string sLoaded;
        if (gRegistry.Get("ZImageViewer", "image", sLoaded))
        {
            mFilenameToLoad = sLoaded;
        }

        SetFocus();
        mpWinImage = new ZWinImage();
        ZRect rImageArea(mAreaToDrawTo);
//        rImageArea.left += 64;  // thumbs
        mpWinImage->SetArea(rImageArea);
        mpWinImage->SetFill(0xff000000);
        mpWinImage->mManipulationHotkey = VK_CONTROL;
        mpWinImage->mBehavior |= ZWinImage::kHotkeyZoom|ZWinImage::kShowOnHotkey|ZWinImage::kScrollable|ZWinImage::kShowControlPanel|ZWinImage::kShowLoadButton|ZWinImage::kShowSaveButton;
        Sprintf(mpWinImage->msSaveButtonMessage, "saveimg;target=%s", msWinName.c_str());
        Sprintf(mpWinImage->msLoadButtonMessage, "loadimg;target=%s", msWinName.c_str());


        ZGUI::Style zoomStyle(gDefaultWinTextEditStyle);
        zoomStyle.pos = ZGUI::CB;
        mpWinImage->SetShowZoom(zoomStyle);

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

bool ImageViewer::ImagePreloading()
{
    for (auto& p : mImageCache)
    {
        if (p.second.valid() && !is_ready(p.second))
        {
            return true;
        }
    }

//    ZDEBUG_OUT_LOCKLESS("not preloading...\n");

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
        {
            if (is_ready(p.second))
                return &p.second;
            return nullptr;
        }
    }

    return nullptr;
}


bool ImageViewer::AddToCache(std::filesystem::path imagePath)
{
    if (imagePath.empty())
        return false;

    if (InCache(imagePath))
        return true;

    mImageCache.emplace_front( tNamedImagePair(imagePath, gPool.enqueue(&ImageViewer::LoadImageProc, imagePath, this)));
    //ZOUT("Adding to cache: ", imagePath, " total in cache:", mImageCache.size(), "\n");
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

    if (mFilenameToLoad != mSelectedFilename)
    {
        if (selectedImage != mImagesInFolder.end())
        {
            if (AddToCache(*selectedImage))
            {
                /*            // now preload the next image
                            selectedImage++;
                            if (selectedImage != mImagesInFolder.end())
                            {
                                AddToCache(*selectedImage);
                            }*/
            }
        }
    }
    else
    {
        // if there's room in the cache and no images are loading, preload another
        if (!ImagePreloading() && CurMemoryUsage() < mMaxMemoryUsage)
        {
            if (mLastAction == kNone || mLastAction == kNextImage)
            {
                for (int readAhead = 0; readAhead < mMaxCacheReadAhead && selectedImage != mImagesInFolder.end(); readAhead++)
                {
                    selectedImage++;
                    if (selectedImage != mImagesInFolder.end() && !InCache(*selectedImage))
                    {
                        AddToCache(*selectedImage);
                        break;
                    }
                }
            }
            else if (mLastAction == kPrevImage)
            {
                for (int readAhead = 0; readAhead < mMaxCacheReadAhead; readAhead++)
                {
                    if (selectedImage != mImagesInFolder.end() && selectedImage != mImagesInFolder.begin())
                    {
                        selectedImage--;
                        if (!InCache(*selectedImage))
                        {
                            AddToCache(*selectedImage);
                            break;
                        }
                    }
                }
            }
        }
    }

    while (CurMemoryUsage() > mMaxMemoryUsage && mImagesInFolder.size() > 1)
    {
        mImageCache.pop_back();
        //ZDEBUG_OUT("Images:", mImageCache.size(), " memory:", CurMemoryUsage(), "\n");
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
            //ZDEBUG_OUT("Found image:", filePath, "\n");
        }
    }

    return true;
}

bool ImageViewer::Process()
{
    if (mpWinImage && mFilenameToLoad != mSelectedFilename)
    {
        tImageFuture* pFuture = GetCached(mFilenameToLoad);
        if (pFuture)
        {
            mSelectedFilename = mFilenameToLoad;
            mpWinImage->SetImage(pFuture->get());

            gRegistry["ZImageViewer"]["image"] = mSelectedFilename.string();


            string sCaption = SH::FromInt(ImageIndexInFolder(mSelectedFilename)) + "/" + SH::FromInt(mImagesInFolder.size()) + " " + mSelectedFilename.string();

            ZGUI::Style captionStyle(gDefaultWinTextEditStyle);
            captionStyle.pos = ZGUI::LB;
            mpWinImage->SetCaption(sCaption, captionStyle);
        }
    }
    else
    {
        Preload();
    }
    return ZWin::Process();
}

bool ImageViewer::Paint()
{
/*    if (!mpTransformTexture)
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpTransformTexture.get()->GetMutex());

    if (!mbVisible)
        return false;

    if (!mbInvalid)
        return false;


    ZRect rThumb(0, 0, 64, mArea.Height() / mImagesInFolder.size());

    int i = 0;
    for (auto& imgPath : mImagesInFolder)
    {
        tImageFuture* pImgFuture = GetCached((const std::filesystem::path&)imgPath);
        if (pImgFuture)
        {
            tZBufferPtr pImg = pImgFuture->get();

            mpTransformTexture->Blt(pImg.get(), pImg->GetArea(), rThumb);
        }
        else
        {
            mpTransformTexture->Fill(rThumb, ARGB(0xff, (i*13)%256, (i*17)%256, (i*29)%256));
        }
        i++;
        rThumb.OffsetRect(0, rThumb.Height());
    }

    */

    return ZWin::Paint();
}
   
