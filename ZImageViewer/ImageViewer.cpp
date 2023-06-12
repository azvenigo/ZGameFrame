#include "ZTypes.h"
#include "ZWin.h"
#include "ImageViewer.h"
#include "ZWinImage.h"
#include "Resources.h"
#include "helpers/StringHelpers.h"
#include "ZStringHelpers.h"
#include "helpers/ThreadPool.h"
#include "ZWinFileDialog.h"
#include "ConfirmDeleteDialog.h"
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
    string sDefaultFolder;
    if (!mSelectedFilename.empty())
        sDefaultFolder = mSelectedFilename.parent_path().string();
    if (ZWinFileDialog::ShowLoadDialog("Images", "*.jpg;*.jpeg;*.png;*.gif;*.tga;*.bmp;*.psd;*.hdr;*.pic;*.pnm", sFilename, sDefaultFolder))
    {
        mFilenameToLoad = sFilename;
        mMoveToFolder.clear();
        Preload();
    }
    return true;

}

bool ImageViewer::OnKeyDown(uint32_t key)
{
#ifdef _WIN64
    if (key == VK_ESCAPE)
    {
        if (mDeletionList.empty())
        {
            gMessageSystem.Post("quit_app_confirmed");
        }
        else
        {
            mFilenameToLoad = *mDeletionList.begin();
            ConfirmDeleteDialog* pDialog = ConfirmDeleteDialog::ShowDialog("Please confirm the following files to be deleted", mDeletionList);
            pDialog->msOnConfirmDelete = ZMessage("delete_confirm", this);
            pDialog->msOnCancel = ZMessage("delete_cancel_and_quit", this);
            pDialog->msOnGoBack = ZMessage("delete_goback", this);
        }
    }
#endif

    switch (key)
    {
    case VK_UP:
        if (gInput.IsKeyDown(VK_MENU))
        {
            HandleNavigateToParentFolder();
        }
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
    case 'm':
    case 'M':
        if (mMoveToFolder.empty())
        {
            HandleMoveCommand();
        }
        else
        {
            MoveSelectedFile(mMoveToFolder);
        }
        break;
    case 'o':
    case 'O':
        return ShowOpenImageDialog();
        break;
    case VK_DELETE:
        {
        if (!mSelectedFilename.empty())
        {
            ToggleDeletionList(mSelectedFilename);
        }
        }
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
    if (sType == "delete_single")
    {
        HandleDeleteCommand();
        return true;
    }
    if (sType == "delete_confirm")
    {
        DeleteConfimed();
        gMessageSystem.Post("quit_app_confirmed");
        return true;
    }
    else if (sType == "delete_goback")
    {
        // Anything to do or just close the confirm dialog?
        if (mpWinImage)
            mpWinImage->SetFocus();
        return true;
    }
    else if (sType == "delete_cancel_and_quit")
    {
        gMessageSystem.Post("quit_app_confirmed");
        return true;
    }
    else if (sType == "select")
    {
        mFilenameToLoad = message.GetParam("filename");
        return true;
    }
    else if (sType == "set_move_folder")
    {
        HandleMoveCommand();
        return true;
    }

    return ZWin::HandleMessage(message);
}

void ImageViewer::HandleDeleteCommand()
{
    if (mSelectedFilename.empty())
    {
        ZERROR("No image selected for delete");
        return;
    }

    filesystem::path current = mSelectedFilename;
    SetNextImage();
    DeleteFile(current);

    tImageFilenames::iterator it = find(mDeletionList.begin(), mDeletionList.end(), current);
    if (it != mDeletionList.end())
        it = mDeletionList.erase(it);

    ScanForImagesInFolder(current.parent_path());
}

void ImageViewer::HandleNavigateToParentFolder()
{
    if (mSelectedFilename.empty())
        return;

    ScanForImagesInFolder(mSelectedFilename.parent_path().parent_path());
    SetFirstImage();
}

void ImageViewer::HandleMoveCommand()
{
    if (mSelectedFilename.empty())
    {
        ZERROR("No image selected to move");
        return;
    }

    string sFolder;
    if (ZWinFileDialog::ShowSelectFolderDialog(sFolder))
    {
        if (filesystem::path(sFolder) == mSelectedFilename.parent_path())
        {
            // do not set move to folder to the same as current
            mMoveToFolder.clear();
            return;
        }
        MoveSelectedFile(filesystem::path(sFolder));
    }
    else
    {
        mMoveToFolder.clear();
    }

    if (mpWinImage)
        mpWinImage->SetFocus();
}

void ImageViewer::MoveSelectedFile(std::filesystem::path& newPath)
{
    if (mSelectedFilename.empty() || !std::filesystem::exists(mSelectedFilename))
    {
        ZERROR("No image selected to move");
        return;
    }

    mMoveToFolder = newPath;

    filesystem::path nextImageFilename;
    tImageFilenames::iterator current = std::find(mImagesInFolder.begin(), mImagesInFolder.end(), mSelectedFilename);
    if (current != mImagesInFolder.end())
    {
        current++;
        if (current != mImagesInFolder.end())
            nextImageFilename = *current;
    }

    filesystem::path newName(newPath);
    newName.append(mSelectedFilename.filename().string());
    filesystem::rename(mSelectedFilename, newName);

    RemoveFromCache(mSelectedFilename);

    ScanForImagesInFolder(mFilenameToLoad.parent_path());

    // if we just moved the last image in the folder, need to select the new last one
    if (nextImageFilename.empty())
    {
        SetLastImage();
    }
    else
    {
        mFilenameToLoad = nextImageFilename;
        mSelectedFilename.clear();
    }


    Invalidate();
}


void ImageViewer::DeleteConfimed()
{
    for (auto f : mDeletionList)
    {
        DeleteFile(f);
    }

    mDeletionList.clear();

    ScanForImagesInFolder(mSelectedFilename.parent_path());
}

void ImageViewer::DeleteFile(std::filesystem::path& f)
{
    SHFILEOPSTRUCT fileOp;
    fileOp.hwnd = NULL;
    fileOp.wFunc = FO_DELETE;

    char path[512];
    memset(path, 0, 512);
    strcpy(path, f.string().c_str());

    fileOp.pFrom = path;
    fileOp.pTo = "Recycle Bin";
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOERRORUI | FOF_NOCONFIRMATION | FOF_SILENT;
    int result = SHFileOperationA(&fileOp);

    if (result != 0)
    {
        ZERROR("Error deleting file:", f, "\n");
    }
}


bool ImageViewer::SaveImage(const std::filesystem::path& filename)
{
    if (filename.empty())
        return false;

    return mpWinImage->mpImage.get()->SaveBuffer(filename.string());
}


void ImageViewer::SetFirstImage()
{
    if (mImagesInFolder.empty())
    {
        Clear();
        return;
    }

    mFilenameToLoad = *mImagesInFolder.begin();
    mLastAction = kBeginning;
    Preload();
}

void ImageViewer::SetLastImage()
{
    if (mImagesInFolder.empty())
    {
        Clear();
        return;
    }

    mFilenameToLoad = *mImagesInFolder.rbegin();
    mLastAction = kEnd;
    Preload();
}

void ImageViewer::SetPrevImage()
{
    if (mImagesInFolder.empty())
    {
        Clear();
        return;
    }

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
    if (mImagesInFolder.empty())
    {
        Clear();
        return;
    }

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
        mpWinImage->mFillColor = 0xff000000;
        mpWinImage->mToggleUIHotkey = VK_TAB;
        mpWinImage->mZoomHotkey = VK_MENU;
        mpWinImage->mBehavior |= ZWinImage::kHotkeyZoom|ZWinImage::kUIToggleOnHotkey|ZWinImage::kScrollable|ZWinImage::kShowControlPanel|ZWinImage::kShowLoadButton|ZWinImage::kShowSaveButton;
        Sprintf(mpWinImage->msSaveButtonMessage, "saveimg;target=%s", msWinName.c_str());
        Sprintf(mpWinImage->msLoadButtonMessage, "loadimg;target=%s", msWinName.c_str());


        ZGUI::Style zoomStyle(gDefaultWinTextEditStyle);
        zoomStyle.pos = ZGUI::CB;
        zoomStyle.look = ZTextLook::kShadowed;
        mpWinImage->mZoomStyle = zoomStyle;

        ChildAdd(mpWinImage);
        mpWinImage->SetFocus();
    }

    Preload();
    if (mFilenameToLoad.empty())
        SetFirstImage();

    return ZWin::Init();
}

bool ImageViewer::OnParentAreaChange()
{
    if (!mpTransformTexture)
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpTransformTexture.get()->GetMutex());
    SetArea(mpParentWin->GetArea());
    UpdateCaptions();
    ZWin::OnParentAreaChange();

    return true;
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
    const std::lock_guard<std::recursive_mutex> lock(mImageCacheMutex);

    for (auto& p : mImageCache)
    {
        if (p.first == imagePath)
            return true;
    }

    return false;
}

bool ImageViewer::ImagePreloading()
{
    const std::lock_guard<std::recursive_mutex> lock(mImageCacheMutex);

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
    const std::lock_guard<std::recursive_mutex> lock(mImageCacheMutex);

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
    const std::lock_guard<std::recursive_mutex> lock(mImageCacheMutex);

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

    const std::lock_guard<std::recursive_mutex> lock(mImageCacheMutex);

    if (InCache(imagePath))
        return true;

    mImageCache.emplace_front( tNamedImagePair(imagePath, gPool.enqueue(&ImageViewer::LoadImageProc, imagePath, this)));
    //ZOUT("Adding to cache: ", imagePath, " total in cache:", mImageCache.size(), "\n");
    return true;
}

bool ImageViewer::RemoveFromCache(std::filesystem::path imagePath)
{
    if (imagePath.empty())
        return false;

    const std::lock_guard<std::recursive_mutex> lock(mImageCacheMutex);

    mImageCache.remove_if([&](auto& p) {return p.first == imagePath; });

    return false;
}


int64_t ImageViewer::CurMemoryUsage()
{
    const std::lock_guard<std::recursive_mutex> lock(mImageCacheMutex);

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

    if (mImagesInFolder.empty())
    {
        Clear();
        return true;
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

    const std::lock_guard<std::recursive_mutex> lock(mImageCacheMutex);

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

void ImageViewer::Clear()
{
    const std::lock_guard<std::recursive_mutex> lock(mImageCacheMutex);
    mImageCache.clear();
    mImagesInFolder.clear();
    mSelectedFilename.clear();
    if (mpWinImage)
        mpWinImage->Clear();

    UpdateCaptions();
    Invalidate();
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
            mpWinImage->mCaptionMap["no_image"].Clear();

            gRegistry["ZImageViewer"]["image"] = mSelectedFilename.string();

            UpdateCaptions();
        }
    }
    else
    {
        Preload();
    }
    return ZWin::Process();
}

void ImageViewer::UpdateCaptions()
{
    if (mpWinImage)
    {
        string sCaption = SH::FromInt(ImageIndexInFolder(mSelectedFilename)) + "/" + SH::FromInt(mImagesInFolder.size()) + " " + mSelectedFilename.string();
        ZGUI::Style captionStyle(gDefaultWinTextEditStyle);
        captionStyle.pos = ZGUI::LB;
        captionStyle.look = ZTextLook::kShadowed;
        mpWinImage->mCaptionMap["filename"].sText = sCaption;
        mpWinImage->mCaptionMap["filename"].style = captionStyle;


        bool bShowCapitons = (mpWinImage->mBehavior & ZWinImage::kShowCaption)!=0;
        if (InDeletionList(mSelectedFilename))
        {
            bShowCapitons = true;
            mpWinImage->mCaptionMap["for_delete"].sText = mSelectedFilename.filename().string() + "\nMARKED FOR DELETE";
            mpWinImage->mCaptionMap["for_delete"].style = ZGUI::Style(ZFontParams("Ariel Bold", 100, 400), ZTextLook(ZTextLook::kShadowed, 0xffff0000, 0xffff0000), ZGUI::C, 0, 0x88000000, true);
        }
        else
        {
            mpWinImage->mCaptionMap["for_delete"].Clear();
        }

        if (!mDeletionList.empty())
        {
            bShowCapitons = true;
            Sprintf(mpWinImage->mCaptionMap["deletion_summary"].sText, "%d Files marked for deletion", mDeletionList.size());
            mpWinImage->mCaptionMap["deletion_summary"].style = ZGUI::Style(ZFontParams("Ariel Bold", 28, 400), ZTextLook(ZTextLook::kShadowed, 0xffff0000, 0xffff0000), ZGUI::RB, 0, 0x88000000, true);
        }
        else
        {
            mpWinImage->mCaptionMap["deletion_summary"].Clear();
        }

        if (!mMoveToFolder.empty())
        {
            bShowCapitons = true;
            Sprintf(mpWinImage->mCaptionMap["move_to_folder"].sText, "'M' -> Move to: %s", mMoveToFolder.string().c_str());
            mpWinImage->mCaptionMap["move_to_folder"].style = ZGUI::Style(ZFontParams("Ariel Bold", 28, 400), ZTextLook(ZTextLook::kShadowed, 0xffff88ff, 0xffff88ff), ZGUI::RT, 32, 0x88000000, true);
        }
        else
        {
            mpWinImage->mCaptionMap["move_to_folder"].Clear();
        }

        if (mImagesInFolder.empty())
        {
            bShowCapitons = true;
            mpWinImage->mCaptionMap["no_image"].sText = "No images\nPress 'O' or TAB";
            mpWinImage->mCaptionMap["no_image"].style = gStyleCaption;
        }




        if (bShowCapitons)
            mpWinImage->mBehavior |= ZWinImage::kShowCaption;
        else
            mpWinImage->mBehavior &= ~ZWinImage::kShowCaption;



        mpWinImage->Invalidate();
    }
}

bool ImageViewer::InDeletionList(std::filesystem::path& imagePath)
{
    tImageFilenames::iterator findit = std::find(mDeletionList.begin(), mDeletionList.end(), imagePath);
    return findit != mDeletionList.end();
}

void ImageViewer::ToggleDeletionList(std::filesystem::path& imagePath)
{
    tImageFilenames::iterator findit = std::find(mDeletionList.begin(), mDeletionList.end(), imagePath);
    if (findit == mDeletionList.end())
        mDeletionList.push_back(imagePath);
    else
        mDeletionList.erase(findit);

    UpdateCaptions();
    Invalidate();
}


bool ImageViewer::Paint()
{
    if (!mpTransformTexture)
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpTransformTexture.get()->GetMutex());

    if (!mbVisible)
        return false;

    if (!mbInvalid)
        return false;


    if (mImagesInFolder.empty())
    {
        gStyleCaption.Font()->DrawTextParagraph(mpTransformTexture.get(), "No images", mAreaToDrawTo, gStyleCaption.look, ZGUI::C);
    }


/*    ZRect rThumb(0, 0, 64, mArea.Height() / mImagesInFolder.size());

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
   
