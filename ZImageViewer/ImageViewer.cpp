#include "ZTypes.h"
#include "ZWin.h"
#include "ImageViewer.h"
#include "ZWinImage.h"
#include "Resources.h"
#include "helpers/StringHelpers.h"
#include "ZStringHelpers.h"
#include "ZWinFileDialog.h"
#include "ConfirmDeleteDialog.h"
#include "ZWinControlPanel.h"
#include "ZWinText.H"
#include "ZGUIStyle.h"
#include <algorithm>
#include "ZWinBtn.H"
#include "helpers/Registry.h"
#include "ImageContest.h"
#include "ZTimer.h"
#include "ImageMeta.h"
#include "WinTopWinners.h"
#include "ZGraphicSystem.h"


using namespace std;


template<typename R>
bool is_ready(std::shared_future<R> const& f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

//#define DEBUG_CACHE

bool ImageEntry::ToBeDeleted()
{
    return filename.parent_path().filename() == ksToBeDeleted;
}

bool ImageEntry::IsFavorite()
{
    return filename.parent_path().filename() == ksFavorites;
}


ImageViewer::ImageViewer()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mIdleSleepMS = 13;
    mMaxMemoryUsage = 4 * 1024 * 1024 * 1024LL;   // 4 GiB
//    mMaxMemoryUsage = 400 * 1024 * 1024LL;
    mMaxCacheReadAhead = 8;
    mLastAction = kNone;
    msWinName = "ZWinImageViewer";
    mpPanel = nullptr;
    mpRotationMenu = nullptr;
    //mpSymbolicFont = nullptr;
    mpFavoritesFont = nullptr;
    mpWinImage = nullptr;
    mpImageLoaderPool = nullptr;
    mpMetadataLoaderPool = nullptr;
    mToggleUIHotkey = 0;
    mCachingState = kWaiting;
    mbSubsample = true;
    mbShowUI = true;
    mFilterState = kAll;
    mpAllFilterButton = nullptr;
    mpFavsFilterButton = nullptr;
    mpDelFilterButton = nullptr;
    mpRankedFilterButton = nullptr;
    mpDeleteMarkedButton = nullptr;
    mOutstandingMetadataCount = 0;
    mpRatedImagesStrip = nullptr;

}
 
ImageViewer::~ImageViewer()
{
    if (mpImageLoaderPool)
        delete mpImageLoaderPool;
}


bool ImageViewer::ShowOpenImageDialog()
{
    string sFilename;
    string sDefaultFolder;
    if (!mCurrentFolder.empty())
        sDefaultFolder = mCurrentFolder.string();
    if (ZWinFileDialog::ShowLoadDialog("Images", "*.jpg;*.jpeg;*.png;*.gif;*.tga;*.bmp;*.psd;*.hdr;*.pic;*.pnm;*.svg", sFilename, sDefaultFolder))
    {
        mMoveToFolder.clear();
        mCopyToFolder.clear();
        mUndoFrom.clear();
        mUndoTo.clear();

        mFilterState = kAll;
        filesystem::path imagePath(sFilename);
        ScanForImagesInFolder(imagePath.parent_path());
        mViewingIndex = IndexFromPath(imagePath);
        KickCaching();
    }
    return true;

}
void ImageViewer::HandleQuitCommand()
{
    mCachingState = kWaiting;

    delete mpImageLoaderPool;
    mpImageLoaderPool = nullptr;

    delete mpMetadataLoaderPool;
    mpMetadataLoaderPool = nullptr;

    gMessageSystem.Post("quit_app_confirmed");
}

void ImageViewer::UpdateUI()
{

    ZDEBUG_OUT("UpdateUI() mbShowUI:", mbShowUI, "\n");
    UpdateControlPanel();

    if (mOutstandingMetadataCount == -1 && !mpRatedImagesStrip && !mRankedImageMetadata.empty())
    {
        mpRatedImagesStrip = new WinTopWinners();
        ChildAdd(mpRatedImagesStrip);
        mpRatedImagesStrip->pMetaList = &mRankedImageMetadata;
    }

    ZRect rImageArea(mAreaToDrawTo);

    if (mpPanel && mpPanel->IsVisible())
        rImageArea.top = mpPanel->GetArea().bottom;

    if (mpRatedImagesStrip)
    {
        if (mFilterState == kRanked)
        {
            ZRect rRatedStrip(rImageArea);
            rRatedStrip.left = rRatedStrip.right - gM * 4;

            mpRatedImagesStrip->SetArea(rRatedStrip);
            mpRatedImagesStrip->SetVisible();
            rImageArea.right = rRatedStrip.left;
        }
        else
            mpRatedImagesStrip->SetVisible(false);
    }

    if (mpWinImage)
        mpWinImage->SetArea(rImageArea);

    UpdateCaptions();
    InvalidateChildren();
}

bool ImageViewer::OnKeyUp(uint32_t key)
{
    if (key == VK_MENU && !mbShowUI)
    {
        UpdateUI();
        return true;
    }


    return ZWin::OnKeyUp(key);
}

bool ImageViewer::OnKeyDown(uint32_t key)
{
#ifdef _WIN64
    if (key == VK_ESCAPE)
    {
        HandleQuitCommand();
        return true;
    }
#endif

    bool bShow = true;
    gRegistry.Get("ZImageViewer", "showui", bShow);
    if (key == mToggleUIHotkey)
    {
        mbShowUI = !mbShowUI;
        UpdateUI();
        gRegistry["ZImageViewer"]["showui"] = mbShowUI;
        return true;
    }
    else if (key == VK_MENU && !mbShowUI)
    {
        UpdateUI();
        return true;
    }


    switch (key)
    {
    case VK_MENU:
        break;
    case VK_UP:
        if (gInput.IsKeyDown(VK_MENU))
            HandleNavigateToParentFolder();
        else
            SetPrevImage();
        break;
    case VK_LEFT:
        SetPrevImage();
        break;
    case VK_DOWN:   // fallthrough to next image
    case VK_RIGHT:
        SetNextImage();
        break;
    case VK_HOME:
        SetFirstImage();
        break;
    case VK_END:
        SetLastImage();
        break;
    case '1':
    {
        ToggleFavorite();
    }
        break;
    case 'D':
    case VK_DELETE:
    {
        ToggleToBeDeleted();
        return true;
    }
    case 'm':
    case 'M':
        if (mMoveToFolder.empty())
        {
            HandleMoveCommand();
        }
        else
        {
            MoveImage(EntryFromIndex(mViewingIndex)->filename, mMoveToFolder);
        }
        break;
    case 'c':
    case 'C':
        if (mCopyToFolder.empty())
        {
            HandleCopyCommand();
        }
        else
        {
            CopyImage(EntryFromIndex(mViewingIndex)->filename, mCopyToFolder);
        }
        break;
    case 'o':
    case 'O':
        return ShowOpenImageDialog();
    }


    return true;
}

bool ImageViewer::RemoveImageArrayEntry(const ViewingIndex& vi)
{
    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpTransformTexture.get()->GetMutex());

    if (!ValidIndex(vi))
        return false;

    filesystem::path curViewingImagePath = EntryFromIndex(mViewingIndex)->filename;
    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
    mImageArray.erase(mImageArray.begin() + vi.absoluteIndex);

    mViewingIndex = IndexFromPath(curViewingImagePath);

    UpdateFilteredView(mFilterState);
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
    if (sType == "quit")
    {
        HandleQuitCommand();
        return true;
    }
    else if (sType == "loadimg")
    {
        ShowOpenImageDialog();
        return true;
    }
    else if (sType == "saveimg")
    {
        string sFilename;
        if (ZWinFileDialog::ShowSaveDialog("Images", "*.jpg;*.jpeg;*.png;*.tga;*.bmp;*.hdr", sFilename))
            SaveImage(sFilename);
        return true;
    }
    else if (sType == "openfolder")
    {
#ifdef _WIN32
        string sPath;
        Sprintf(sPath, "/select,%s", EntryFromIndex(mViewingIndex)->filename.string().c_str());
        ShellExecute(0, "open", "explorer", sPath.c_str(), 0,  SW_SHOWNORMAL);
        return true;
#endif
    }
    else if (sType == "show_confirm")
    {
//        tImageFilenames deletionList = GetImagesFlaggedToBeDeleted();
        if (!mToBeDeletedImageArray.empty())
        {
            tImageFilenames deletionList;
            for (auto& i : mToBeDeletedImageArray)
                deletionList.push_back(i->filename);
            ConfirmDeleteDialog* pDialog = ConfirmDeleteDialog::ShowDialog("Please confirm the following files to be deleted", deletionList);
            pDialog->msOnConfirmDelete = ZMessage("delete_confirm", this);
            pDialog->msOnCancel = ZMessage("delete_cancel_and_quit", this);
            pDialog->msOnGoBack = ZMessage("delete_goback", this);
        }
        return true;
    }
    else if (sType == "delete_confirm")
    {
        DeleteConfimed();
        return true;
    }
    else if (sType == "delete_goback")
    {
        // Anything to do or just close the confirm dialog?
        if (mpWinImage)
            mpWinImage->SetFocus();
        return true;
    }
    else if (sType == "select")
    {
        mViewingIndex = IndexFromPath(message.GetParam("filename"));
        KickCaching();
        return true;
    }
    else if (sType == "set_move_folder")
    {
        HandleMoveCommand();
        return true;
    }
    else if (sType == "set_copy_folder")
    {
        HandleCopyCommand();
        return true;
    }
    else if (sType == "filter_all")
    {
        UpdateFilteredView(kAll);
        return true;
    }
    else if (sType == "filter_del")
    {
        UpdateFilteredView(kToBeDeleted);
        return true;
    }
    else if (sType == "filter_favs")
    {
        UpdateFilteredView(kFavs);
        return true;
    }
    else if (sType == "filter_ranked")
    {
        UpdateFilteredView(kRanked);
        return true;
    }
    else if (sType == "invalidate")
    {
        if (mbSubsample && mpWinImage)
            mpWinImage->nSubsampling = 2;
        else
            mpWinImage->nSubsampling = 0;
        InvalidateChildren();
        return true;
    }
    else if (sType == "show_contest")
    {
        ImageContest* pWin = new ImageContest();
        pWin->SetArea(mArea);
        GetTopWindow()->ChildAdd(pWin);
        if (mFilterState == kFavs)
            pWin->ScanFolder(FavoritesPath());
        else
            pWin->ScanFolder(mCurrentFolder);
        SetVisible(false);
        return true;
    }
    else if (sType == "image_selection")
    {
        ZRect rSelection = StringToRect(message.GetParam("r"));

        ZOUT("Rect Selected:", message.GetParam("r"), "\n");
        string sFolder;
        gRegistry.Get("ZImageViewer", "selectionsave", sFolder);

        filesystem::path fileName(EntryFromIndex(mViewingIndex)->filename.filename());
        fileName.replace_extension(".png");
        string sFilename = fileName.string();

        if (ZWinFileDialog::ShowSaveDialog("Images", "*.png;*.jpg;*.jpeg;*.tga;*.bmp;*.hdr", sFilename, sFolder))
        {
            ZRect rSourceImage(mpWinImage->mpImage->GetArea());

            ZBuffer imageSelection;
            imageSelection.Init(rSelection.Width(), rSelection.Height());
            imageSelection.Blt(mpWinImage->mpImage.get(), rSelection, imageSelection.GetArea(), 0, ZBuffer::kAlphaSource);

#define RESIZE
#ifdef RESIZE

            const int kSize = 1024;

            ZBuffer resizedimage;
            resizedimage.Init(kSize, kSize);
            resizedimage.Fill(0xff000000, &resizedimage.GetArea());

            resizedimage.BltScaled(&imageSelection);

            resizedimage.SaveBuffer(sFilename);
#else
            imageSelection.SaveBuffer(sFilename);
#endif
            filesystem::path folder(sFilename);
            gRegistry.Set("ZImageViewer", "selectionsave", folder.parent_path().string());
        }

        InvalidateChildren();
        return true;
    }

    if (mpWinImage && mpWinImage->mpImage)
    {
        double fOldZoom = mpWinImage->GetZoom();
        if (sType == "rotate_left")
        {
            mpWinImage->mpImage->Rotate(ZBuffer::kLeft);

            if (mpWinImage->IsSizedToWindow())
                mpWinImage->FitImageToWindow();
            else
                mpWinImage->SetZoom(fOldZoom);

            InvalidateChildren();
            return true;
        }
        else if (sType == "rotate_right")
        {
            mpWinImage->mpImage->Rotate(ZBuffer::kRight);
            if (mpWinImage->IsSizedToWindow())
                mpWinImage->FitImageToWindow();
            else
                mpWinImage->SetZoom(fOldZoom);

            InvalidateChildren();
            return true;
        }
        else if (sType == "flipH")
        {
            mpWinImage->mpImage->Rotate(ZBuffer::kHFlip);
            if (mpWinImage->IsSizedToWindow())
                mpWinImage->FitImageToWindow();
            else
                mpWinImage->SetZoom(fOldZoom);

            InvalidateChildren();
            return true;
        }
        else if (sType == "flipV")
        {
            mpWinImage->mpImage->Rotate(ZBuffer::kVFlip);
            if (mpWinImage->IsSizedToWindow())
                mpWinImage->FitImageToWindow();
            else
                mpWinImage->SetZoom(fOldZoom);

            InvalidateChildren();
            return true;
        }
        else if (sType == "show_rotation_menu")
        {
            if (mpRotationMenu)
            {
                if (mpRotationMenu->IsVisible())
                {
                    mpRotationMenu->SetVisible(false);
                }
                else
                {
                    mpRotationMenu->mStyle.paddingH = (int32_t)gSpacer;
                    mpRotationMenu->mStyle.paddingV = (int32_t)gSpacer;

                    ZRect r = StringToRect(message.GetParam("r"));
                    r.InflateRect(gSpacer *2, gSpacer *2);
                    r.bottom = r.top + r.Height() * 4 + gM;

                    mpRotationMenu->SetArea(r);
                    mpRotationMenu->ArrangeWindows(mpRotationMenu->GetChildrenInGroup("Rotate"), ZRect(r.Width(),r.Height()), mpRotationMenu->mStyle, 1, -1);
                    mpRotationMenu->SetVisible();
                    ChildToFront(mpRotationMenu);
                }
            }
            return true;
        }
        else if (sType == "undo")
        {
            if (!mUndoFrom.empty() && !mUndoTo.empty())
            {
                MoveImage(mUndoTo, mUndoFrom);   // move it back
                ViewingIndex vi = IndexFromPath(mUndoTo);  // if moving an image into our tree
                if (ValidIndex(vi))
                    mViewingIndex = vi;
                UpdateFilteredView(mFilterState);
            }
            return true;
        }
        else if (sType == "show_help")
        {
            ShowHelpDialog();
            return true;
        }
   }

    return ZWin::HandleMessage(message);
}

void ImageViewer::ShowHelpDialog()
{
    ZWinDialog* pHelp = new ZWinDialog();
    pHelp->msWinName = "ImageViewerHelp";
    pHelp->mbAcceptsCursorMessages = true;
    pHelp->mbAcceptsFocus = true;

    ZRect r(1600, 1300);
    pHelp->SetArea(r);
    pHelp->mBehavior = ZWinDialog::Draggable | ZWinDialog::OKButton;
    pHelp->mStyle = gDefaultDialogStyle;


    ZRect rCaption(r);
    ZWinLabel* pLabel = new ZWinLabel();
    pLabel->msText = "ZView Help";
    pLabel->SetArea(rCaption);
    pLabel->mStyle = gStyleCaption;
    pLabel->mStyle.pos = ZGUI::CT;
    pLabel->mStyle.fp.nHeight = 72;
    pLabel->mStyle.fp.nWeight = 500;
    pLabel->mStyle.look = ZGUI::ZTextLook::kEmbossed;
    pLabel->mStyle.look.colTop = 0xff999999;
    pLabel->mStyle.look.colBottom = 0xff777777;
    pHelp->ChildAdd(pLabel);



    ZWinFormattedDoc* pForm = new ZWinFormattedDoc();

    ZGUI::Style text(gStyleGeneralText);
    ZGUI::Style sectionText(gStyleGeneralText);
    sectionText.fp.nWeight = 800;
    sectionText.look.colTop = 0xffaaaaaa;
    sectionText.look.colBottom = 0xffaaaaaa;

    ZRect rForm(1400, 1100);
    rForm = ZGUI::Arrange(rForm, r, ZGUI::C);
    pForm->SetArea(rForm);
    pForm->mbScrollable = true;
    pForm->mDialogStyle.bgCol = 0;
    pForm->mbAcceptsCursorMessages = false;
    pHelp->ChildAdd(pForm);
    ChildAdd(pHelp);
    pHelp->Arrange(ZGUI::C, mAreaToDrawTo);

    pForm->AddMultiLine("The main idea for ZView is to open and sort through images as fast as possible.\nThe app will read ahead/behind so that the next or previous image is already loaded when switching.\n\n", text);
    pForm->AddMultiLine("Loading, quickly zooming and exiting are prioritized.", text);
    pForm->AddMultiLine("\nAnother key feature is to sort through which images are favorites, and which should be deleted.", text);
    pForm->AddMultiLine("So that you have time to change your mind, images are moved into subfolders.", text);


    pForm->AddMultiLine("\nViewing", sectionText);
    pForm->AddMultiLine("Associate image types with ZView or open an image from the toolbar or press 'O' to open a new image.", text);
    pForm->AddMultiLine("Once an image is loaded, all images in the same folder are scanned and available to sort through.", text);
    pForm->AddMultiLine("Use Left/Right or Mouse wheel to flip through images.", text);
    pForm->AddMultiLine("Use Home/End to go to the beginning/end of the list.", text);

    pForm->AddMultiLine("Use toolbar rotation buttons to rotate/flip", text);

    pForm->AddMultiLine("\nZoom/Scroll", sectionText);
    pForm->AddMultiLine("Right-click on a portion of the image to quickly switch between 100% zoom and fit to window..", text);
    pForm->AddMultiLine("Hold ALT and use Mouse wheel zoom in and out.", text);

    pForm->AddMultiLine("\nUI", sectionText);
    pForm->AddMultiLine("Use TAB to toggle UI visibility", text);
    pForm->AddMultiLine("Hold ALT show UI (when hidden)", text);
    pForm->AddMultiLine("Use 'F' to switch windowed/fullscreen", text);
    pForm->AddMultiLine("Move mouse to the top of the screen to show toolbar even with UI hidden.", text);
    
    pForm->AddMultiLine("\nManaging Images", sectionText);
    pForm->AddMultiLine("Use '1' to toggle an image as favorite. (It will be moved into a 'favorites' subfolder.", text);
    pForm->AddMultiLine("Use 'DEL' to toggle an image as to-be-deleted. (It will be moved into a 'ZZ_TO_BE_DELETED_ZZ' subfolder.", text);
    pForm->AddMultiLine("Use the DELETE button in the toolbar to bring up a confirmation dialog with the list of photos marked for deletion.", text);

    pForm->AddMultiLine("Use the filter buttons in the toolbar to view either all images, just favorites, or just images marked for deletion.", text);


    pForm->AddMultiLine("Use 'UNDO' to undo the previous toggle or move.", text);

    pForm->AddMultiLine("\nYou can also use 'M' to quickly set a destination folder. Afterward 'M' will instantly move the image to that destination. (UNDO is available for this as well.)", text);

    pForm->AddMultiLine("\nAI Training Images", sectionText);
    pForm->AddMultiLine("Hold SHIFT and left drag a selection rectangle to crop/resize to 256x256 and save the result. This is for quickly generating training data for Dreambooth....maybe others.", text);



    pForm->Invalidate();


}

void ImageViewer::HandleNavigateToParentFolder()
{
    ScanForImagesInFolder(mCurrentFolder.parent_path());
    SetFirstImage();
}

void ImageViewer::HandleMoveCommand()
{
    if (!ValidIndex(mViewingIndex) || CountImagesMatchingFilter(mFilterState) == 0)
    {
        ZERROR("No image selected to move");
        return;
    }

    string sFolder;
    if (ZWinFileDialog::ShowSelectFolderDialog(sFolder, mCurrentFolder.string()))
    {
        if (filesystem::path(sFolder) == mCurrentFolder)
        {
            // do not set move to folder to the same as current
            mMoveToFolder.clear();
            return;
        }
        if (MoveImage(EntryFromIndex(mViewingIndex)->filename, filesystem::path(sFolder)))
            mMoveToFolder = sFolder;
        else
            mMoveToFolder.clear();
    }
    else
    {
        mMoveToFolder.clear();

    }

    if (mpWinImage)
        mpWinImage->SetFocus();
}

void ImageViewer::HandleCopyCommand()
{
    if (!ValidIndex(mViewingIndex) || CountImagesMatchingFilter(mFilterState) == 0)
    {
        ZERROR("No image selected to copy");
        return;
    }

    string sFolder;
    if (ZWinFileDialog::ShowSelectFolderDialog(sFolder, mCurrentFolder.string()))
    {
        filesystem::path newPath(sFolder);

        bool bNewPathInTree = newPath.parent_path() == mCurrentFolder || newPath.parent_path() == FavoritesPath() || newPath.parent_path() == ToBeDeletedPath();

        if (bNewPathInTree)
        {
            // do not set copy to folder into our tree
            mCopyToFolder.clear();
            return;
        }
        if (CopyImage(EntryFromIndex(mViewingIndex)->filename, filesystem::path(sFolder)))
            mCopyToFolder = sFolder;
        else
            mCopyToFolder.clear();
    }
    else
    {
        mCopyToFolder.clear();

    }

    if (mpWinImage)
        mpWinImage->SetFocus();
}


bool ImageViewer::MoveImage(std::filesystem::path oldPath, std::filesystem::path newPath)
{
    if (oldPath.empty() || newPath.empty())
        return false;

    if (!std::filesystem::exists(oldPath))
    {
        ZERROR("No image selected to move");
        return false;
    }

    // if we were passed a folder name only, use the filename of the source
    if (!newPath.has_extension())
        newPath.append(oldPath.filename().string());

    // if the file is moving in/out of current tree, rescan
    bool bRequireRescan = false;

    bool bOldPathInTree = oldPath.parent_path() == mCurrentFolder || oldPath.parent_path() == ToBeDeletedPath() || oldPath.parent_path() == FavoritesPath();
    bool bNewPathInTree = newPath.parent_path() == mCurrentFolder || newPath.parent_path() == ToBeDeletedPath() || newPath.parent_path() == FavoritesPath();

    bRequireRescan = bOldPathInTree^bNewPathInTree;     // xor

    ZOUT("Moving ", oldPath, " -> ", newPath, "\n");

    filesystem::create_directories(newPath.parent_path());

    try
    {
        filesystem::rename(oldPath, newPath);
        const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
        mImageArray[mViewingIndex.absoluteIndex]->filename = newPath;

        mUndoFrom = oldPath;
        mUndoTo = newPath;
    }
    catch (std::filesystem::filesystem_error err)
    {
        ZERROR("Error renaming from \"", oldPath, "\" to \"", newPath, "\"\ncode:", err.code(), " error:", err.what(), "\n");
        return false;
    };


    if (bRequireRescan)
        ScanForImagesInFolder(mCurrentFolder);

//    mnViewingIndex = oldIndex;
//    limit<int64_t>(mnViewingIndex, 0, mImageArray.size() - 1);

    Invalidate();

    return true;
}


bool ImageViewer::CopyImage(std::filesystem::path curPath, std::filesystem::path newPath)
{
    if (curPath.empty() || newPath.empty())
        return false;

    if (!std::filesystem::exists(curPath))
    {
        ZERROR("No image selected to copy");
        return false;
    }

    // if we were passed a folder name only, use the filename of the source
    if (!newPath.has_extension())
        newPath.append(curPath.filename().string());

    ZOUT("Copying ", curPath, " -> ", newPath, "\n");

    filesystem::create_directories(newPath.parent_path());

    try
    {
        filesystem::copy_file(curPath, newPath);
        const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
        mImageArray[mViewingIndex.absoluteIndex]->filename = newPath;

        mUndoFrom.clear();
        mUndoTo.clear();
    }
    catch (std::filesystem::filesystem_error err)
    {
        ZERROR("Error copying from \"", curPath, "\" to \"", newPath, "\"\ncode:", err.code(), " error:", err.what(), "\n");
        return false;
    };

    Invalidate();

    return true;
}



void ImageViewer::DeleteConfimed()
{
//    tImageFilenames deletionList = GetImagesFlaggedToBeDeleted();
    for (auto& f : mToBeDeletedImageArray)
    {
        DeleteFile(f->filename);
    }

    filesystem::path toBeDeleted = mCurrentFolder;
    toBeDeleted.append(ksToBeDeleted);
    if (std::filesystem::is_empty(toBeDeleted))
        std::filesystem::remove(toBeDeleted);

    ScanForImagesInFolder(mCurrentFolder);
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
    if (!mImageArray.empty())
    {
        const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
        if (mFilterState == kRanked)
        {
            if (mRankedArray.empty())
                return;
            mViewingIndex = IndexFromPath(GetRankedFilename(1));
        }
        else
        {

            mViewingIndex = IndexFromPath(mImageArray[0]->filename);
            if (!ImageMatchesCurFilter(mViewingIndex))
            {
                if (!FindImageMatchingCurFilter(mViewingIndex, 1))
                {
                    UpdateFilteredView(mFilterState);
                }
            };
        }
    }
    else
        mViewingIndex = {};

    mLastAction = kBeginning;
    mCachingState = kReadingAhead;
    FreeCacheMemory();
    KickCaching();
    InvalidateChildren();
}
 

void ImageViewer::SetLastImage()
{
    if (!mImageArray.empty())
    {
        const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
        if (mFilterState == kRanked)
        {
            if (mRankedArray.empty())
                return;
            mViewingIndex = IndexFromPath(GetRankedFilename(mRankedArray.size()));
        }
        else
        {
            mViewingIndex = IndexFromPath(mImageArray[mImageArray.size() - 1]->filename);
            if (!ImageMatchesCurFilter(mViewingIndex))
            {
                if (!FindImageMatchingCurFilter(mViewingIndex, -1))
                {
                    UpdateFilteredView(mFilterState);
                }
            };
        }
    }
    else
        mViewingIndex = {};


    mLastAction = kEnd;
    mCachingState = kReadingAhead;
    FreeCacheMemory();
    KickCaching();
    InvalidateChildren();
}

void ImageViewer::SetPrevImage()
{
    if (!FindImageMatchingCurFilter(mViewingIndex, -1))
    {
//        UpdateFilteredView(mFilterState);
        ZDEBUG_OUT("SetPrevImage() - No prev image found.");
    }

    mLastAction = kPrevImage;
    mCachingState = kReadingAhead;
    FreeCacheMemory();
    KickCaching();
    InvalidateChildren();
}

void ImageViewer::SetNextImage()
{
    if (!FindImageMatchingCurFilter(mViewingIndex, 1))
    {
//        UpdateFilteredView(mFilterState);
        ZDEBUG_OUT("SetNextImage() - No next image found.");
    }

    mLastAction = kNextImage;
    mCachingState = kReadingAhead;
    FreeCacheMemory();
    KickCaching();
    InvalidateChildren();
}

bool ImageViewer::FindImageMatchingCurFilter(ViewingIndex& vi, int64_t offset)
{
    assert(offset != 0);    // need a search direction
    if (offset == 0)
    {
        return false;
    }

    if (mFilterState == kRanked)
    {
        int64_t nCurRank = GetRank(mImageArray[vi.absoluteIndex]->filename.string());
        int64_t nNewRank = nCurRank + offset;
        if (nNewRank < 1 || nNewRank > (int64_t)mRankedArray.size())
            return false;

        vi = IndexFromPath( GetRankedFilename(nNewRank));
        return true;
    }



    int64_t nAbsIndex = vi.absoluteIndex + offset;

    while (nAbsIndex >= 0 && nAbsIndex < (int64_t)mImageArray.size())
    {
        const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
        if (mFilterState == kAll)
        {
            vi = IndexFromPath(mImageArray[nAbsIndex]->filename);   // fill out ViewingIndex
            return true;
        }
        else if (mFilterState == kToBeDeleted && mImageArray[nAbsIndex]->ToBeDeleted())
        {
            vi = IndexFromPath(mImageArray[nAbsIndex]->filename);   // fill out ViewingIndex
            return true;
        }
        else if (mFilterState == kFavs && mImageArray[nAbsIndex]->IsFavorite())
        {
            vi = IndexFromPath(mImageArray[nAbsIndex]->filename);   // fill out ViewingIndex
            return true;
        }

        nAbsIndex += offset;
    }

    return false;
}


/*bool ImageViewer::FindNextImageMatchingFilter(ViewingIndex& vi)
{
    if (!ValidIndex(vi))
        return false;

    filesystem::path nextImagePath;

    if (mFilterState == kToBeDeleted && vi.delIndex < mToBeDeletedImageArray.size() - 1)
        nextImagePath = mToBeDeletedImageArray[vi.delIndex + 1]->filename;
    else if (mFilterState == kFavs && vi.favIndex < mFavImageArray.size() - 1)
        nextImagePath = mFavImageArray[vi.favIndex + 1]->filename;
    else if (vi.absoluteIndex < mImageArray.size() - 1)
        nextImagePath = mImageArray[vi.absoluteIndex + 1]->filename;

    ViewingIndex newIndex = IndexFromPath(nextImagePath);
    if (ValidIndex(newIndex))
    {
        vi = newIndex;
        return true;
    }

    return false;
}

bool ImageViewer::FindPrevImageMatchingFilter(ViewingIndex& vi)
{
    if (!ValidIndex(vi))
        return false;

    filesystem::path prevImagePath;

    if (mFilterState == kToBeDeleted && vi.delIndex > 0)
        prevImagePath = mToBeDeletedImageArray[vi.delIndex - 1]->filename;
    else if (mFilterState == kFavs && vi.favIndex > 0)
        prevImagePath = mFavImageArray[vi.favIndex - 1]->filename;
    else if (vi.absoluteIndex > 0)
        prevImagePath = mImageArray[vi.absoluteIndex - 1]->filename;

    ViewingIndex newIndex = IndexFromPath(prevImagePath);
    if (ValidIndex(newIndex))
    {
        vi = newIndex;
        return true;
    }

    return false;
}*/


bool ImageViewer::ImageMatchesCurFilter(const ViewingIndex& vi)
{
    if (mFilterState == kAll)
        return true;

    if (!ValidIndex(vi.absoluteIndex))
        return false;

    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);

    if (mFilterState == kRanked)
        return mImageArray[vi.absoluteIndex]->mMeta.elo > 0;

    if (mFilterState == kToBeDeleted)
        return mImageArray[vi.absoluteIndex]->ToBeDeleted();

    assert(mFilterState == kFavs);
    return mImageArray[vi.absoluteIndex]->IsFavorite();
}

int64_t ImageViewer::CountImagesMatchingFilter(eFilterState state)
{
    if (state == kToBeDeleted)
        return mToBeDeletedImageArray.size();
    else if (state == kFavs)
        return mFavImageArray.size();
    else if (state == kRanked)
        return mRankedArray.size();

    return mImageArray.size();
}



bool ImageViewer::Init()
{
    if (!mbInitted)
    {
        gRegistry.Get("ZImageViewer", "showui", mbShowUI);


        int64_t nGroupSide = (gM * 2) - gSpacer * 4;

        mSymbolicStyle = ZGUI::Style(ZFontParams("Arial", nGroupSide, 200, 0, 0, false, true), ZGUI::ZTextLook{}, ZGUI::C, 0);
        //        mpSymbolicFont = gpFontSystem->CreateFont(unicodeStyle.fp);
        ZDynamicFont* pFont = (ZDynamicFont*)mSymbolicStyle.Font().get();

        pFont->GenerateSymbolicGlyph('<', 11119); // rotate left
        pFont->GenerateSymbolicGlyph('>', 11118); // rotate right

        pFont->GenerateSymbolicGlyph('-', 11108); // flip H
        pFont->GenerateSymbolicGlyph('|', 11109); // flip V

        pFont->GenerateSymbolicGlyph('u', 0x238c); // undo


        pFont->GenerateSymbolicGlyph('F', 0x2750);
        pFont->GenerateSymbolicGlyph('Q', 0x0F1C);  // quality rendering

        SetFocus();
        mpWinImage = new ZWinImage();
        ZRect rImageArea(mAreaToDrawTo);
//        rImageArea.left += 64;  // thumbs
        mpWinImage->SetArea(rImageArea);
        mpWinImage->mFillColor = 0xff000000;
        mpWinImage->mZoomHotkey = VK_MENU;
        mpWinImage->mBehavior |= ZWinImage::kHotkeyZoom|ZWinImage::kScrollable|ZWinImage::kSelectableArea|ZWinImage::kLaunchGeolocation;

//        mpWinImage->mCaptionMap["zoom"].style.paddingV = gStyleCaption.fp.nHeight;

        mpWinImage->mpTable = new ZGUI::ZTable();
        ChildAdd(mpWinImage);
        mpWinImage->SetFocus();

        mToggleUIHotkey = VK_TAB;

        Clear();

        string sPersistedViewPath;
        if (gRegistry.Get("ZImageViewer", "image", sPersistedViewPath))
        {
            ViewImage(sPersistedViewPath);
        }


        string sMessage;
        mpRotationMenu = new ZWinControlPanel();

        string sAppPath = gRegistry["apppath"];
        Sprintf(sMessage, "show_rotation_menu;target=%s", GetTargetName().c_str());
        ZWinSizablePushBtn* pBtn = mpRotationMenu->AddSVGButton(sAppPath + "/res/rotate.svg", sMessage);
        pBtn->msWinGroup = "Rotate";

        Sprintf(sMessage, "rotate_left;target=%s", GetTargetName().c_str());
        pBtn = mpRotationMenu->AddButton("<", sMessage);
        pBtn->mCaption.style = mSymbolicStyle;
        pBtn->msWinGroup = "Rotate";

        Sprintf(sMessage, "rotate_right;target=%s", GetTargetName().c_str());
        pBtn = mpRotationMenu->AddButton(">", sMessage);
        pBtn->mCaption.style = mSymbolicStyle;
        pBtn->msWinGroup = "Rotate";

        Sprintf(sMessage, "flipH;target=%s", GetTargetName().c_str());
        pBtn = mpRotationMenu->AddButton("-", sMessage);
        pBtn->mCaption.style = mSymbolicStyle;
        pBtn->msWinGroup = "Rotate";

        Sprintf(sMessage, "flipV;target=%s", GetTargetName().c_str());
        pBtn = mpRotationMenu->AddButton("|", sMessage);
        pBtn->mCaption.style = mSymbolicStyle;
        pBtn->msWinGroup = "Rotate";

        mpRotationMenu->mbHideOnMouseExit = true;
        ChildAdd(mpRotationMenu, false);
    }

    if (!ValidIndex(mViewingIndex))
    {
        SetFirstImage();
        UpdateControlPanel();
        UpdateCaptions();
        mpPanel->SetVisible();
    }

    return ZWin::Init();
}


void ImageViewer::LimitIndex()
{
    limit<int64_t>(mViewingIndex.absoluteIndex, 0, mImageArray.size());
    limit<int64_t>(mViewingIndex.delIndex,      0, mToBeDeletedImageArray.size());
    limit<int64_t>(mViewingIndex.favIndex,      0, mFavImageArray.size());
    limit<int64_t>(mViewingIndex.rankedIndex,   0, mRankedArray.size());
}

void ImageViewer::UpdateControlPanel()
{
    int64_t nControlPanelSide = gM * 2.5;
    limit<int64_t>(nControlPanelSide, 40, 88);


    const std::lock_guard<std::recursive_mutex> panelLock(mPanelMutex);
    
    if (mpPanel && mpPanel->GetArea().Width() == mAreaToDrawTo.Width() && mpPanel->GetArea().Height() == nControlPanelSide)
    {
        bool bShow = mbShowUI;
        if (gInput.IsKeyDown(VK_MENU))
            bShow = true;

        mpPanel->mbHideOnMouseExit = !bShow;
        mpPanel->SetVisible(bShow);
        return;
    }

    // panel needs to be created or is wrong dimensions
    if (mpPanel)
    {
        ZDEBUG_OUT("Deleting Control Panel\n");
        ChildDelete(mpPanel);
        mpPanel = nullptr;
    }


    mpPanel = new ZWinControlPanel();
        
    int64_t nGroupSide = nControlPanelSide - gSpacer * 4;



    ZGUI::Style wingdingsStyle = ZGUI::Style(ZFontParams("Wingdings", nGroupSide /2, 200, 0, 0, false, true), ZGUI::ZTextLook{}, ZGUI::C);

/*    if (!mpSymbolicFont)
    {
        mpSymbolicFont = gpFontSystem->CreateFont(unicodeStyle.fp);
        ((ZDynamicFont*)mpSymbolicFont.get())->GenerateSymbolicGlyph('<', 11119); // rotate left
        ((ZDynamicFont*)mpSymbolicFont.get())->GenerateSymbolicGlyph('>', 11118); // rotate right

        ((ZDynamicFont*)mpSymbolicFont.get())->GenerateSymbolicGlyph('-', 11108); // flip H
        ((ZDynamicFont*)mpSymbolicFont.get())->GenerateSymbolicGlyph('|', 11109); // flip V

        ((ZDynamicFont*)mpSymbolicFont.get())->GenerateSymbolicGlyph('u', 0x238c); // undo


        ((ZDynamicFont*)mpSymbolicFont.get())->GenerateSymbolicGlyph('F', 0x2750);
        ((ZDynamicFont*)mpSymbolicFont.get())->GenerateSymbolicGlyph('Q', 0x0F1C);  // quality rendering
    }*/

    ZGUI::Style favorites = ZGUI::Style(ZFontParams("Arial", nGroupSide*4, 400, 0, 0, false, true), ZGUI::ZTextLook{}, ZGUI::C, 0);
    if (!mpFavoritesFont)
    {
        mpFavoritesFont = gpFontSystem->CreateFont(favorites.fp);
        ((ZDynamicFont*)mpFavoritesFont.get())->GenerateSymbolicGlyph('C', 0x2655);  // crown
    }


//    ((ZDynamicFont*)wingdingsStyle.Font().get())->GenerateSymbolicGlyph('F', 0x2922);



    ZRect rPanelArea(mAreaToDrawTo.left, mAreaToDrawTo.top, mAreaToDrawTo.right, mAreaToDrawTo.top + nControlPanelSide);
    mpPanel->mbHideOnMouseExit = true; // if UI is toggled on, then don't hide panel on mouse out
    mpPanel->SetArea(rPanelArea);
    mpPanel->mrTrigger = rPanelArea;
    mpPanel->mrTrigger.bottom = mpPanel->mrTrigger.top + mpPanel->mrTrigger.Height() / 4;
    ChildAdd(mpPanel, mbShowUI);
    mpPanel->mbHideOnMouseExit = !mbShowUI;

    ZWinSizablePushBtn* pBtn;

    ZRect rButton(nGroupSide, nGroupSide);
    rButton.OffsetRect(gSpacer * 2, gSpacer * 2);

    string sAppPath = gRegistry["apppath"];

    int32_t nButtonPadding = (int32_t)(rButton.Width() / 8);
    
    pBtn = new ZWinSizablePushBtn();
    pBtn->mSVGImage.Load(sAppPath + "/res/openfile.svg");
    pBtn->SetTooltip("Load Image");
    pBtn->msWinGroup = "File";
    pBtn->mSVGImage.style.paddingH = nButtonPadding;
    pBtn->mSVGImage.style.paddingV = nButtonPadding;

    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("loadimg", this));
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width(), 0);

    pBtn = new ZWinSizablePushBtn();
    pBtn->mSVGImage.Load(sAppPath + "/res/save.svg");
    pBtn->msWinGroup = "File";
    pBtn->SetTooltip("Save Image");

    pBtn->mSVGImage.style.paddingH = nButtonPadding;
    pBtn->mSVGImage.style.paddingV = nButtonPadding;
    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("saveimg", this));
    pBtn->msWinGroup = "File";
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width(), 0);

    pBtn = new ZWinSizablePushBtn();
    pBtn->mSVGImage.Load(sAppPath + "/res/openfolder.svg");
    pBtn->SetTooltip("Go to Folder");
    pBtn->mSVGImage.style.paddingH = nButtonPadding;
    pBtn->mSVGImage.style.paddingV = nButtonPadding;

    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("openfolder", this));
    pBtn->msWinGroup = "File";
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width(), 0);


    string sMessage;


    // Management
    rButton.OffsetRect(rButton.Width() + gSpacer * 2, 0);
    rButton.right = rButton.left + (int64_t) (rButton.Width() * 2.5);     // wider buttons for management

    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = "undo";  // undo glyph
    pBtn->mCaption.style = gDefaultGroupingStyle;
    pBtn->mCaption.style.fp.nHeight = (int64_t) (nGroupSide / 1.5);
    pBtn->mCaption.style.pos = ZGUI::C;
    pBtn->SetArea(rButton);
    pBtn->msWinGroup = "Manage";
    Sprintf(sMessage, "undo;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width(), 0);
    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = "move";
    pBtn->mCaption.style = gDefaultGroupingStyle;
    pBtn->mCaption.style.fp.nHeight = (int64_t) (nGroupSide / 1.5);
    pBtn->mCaption.style.pos = ZGUI::C;
    pBtn->SetArea(rButton);
    pBtn->msWinGroup = "Manage";
    Sprintf(sMessage, "set_move_folder;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width(), 0);
    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = "copy";
    pBtn->mCaption.style = gDefaultGroupingStyle;
    pBtn->mCaption.style.fp.nHeight = (int64_t)(nGroupSide / 1.5);
    pBtn->mCaption.style.pos = ZGUI::C;
    pBtn->SetArea(rButton);
    pBtn->msWinGroup = "Manage";
    Sprintf(sMessage, "set_copy_folder;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);




    rButton.OffsetRect(rButton.Width(), 0);
    mpDeleteMarkedButton = new ZWinSizablePushBtn();
    mpDeleteMarkedButton->mCaption.sText = "delete\nmarked";
    mpDeleteMarkedButton->mCaption.style = gDefaultGroupingStyle;
    mpDeleteMarkedButton->mCaption.style.look.colTop = 0xffff0000;
    mpDeleteMarkedButton->mCaption.style.look.colBottom = 0xffff0000;
    mpDeleteMarkedButton->mCaption.style.fp.nHeight = nGroupSide / 2;
    mpDeleteMarkedButton->mCaption.style.wrap = true;
    mpDeleteMarkedButton->mCaption.style.pos = ZGUI::C;
    mpDeleteMarkedButton->SetArea(rButton);
    mpDeleteMarkedButton->msWinGroup = "Manage";
    Sprintf(sMessage, "show_confirm;target=%s", GetTargetName().c_str());
    mpDeleteMarkedButton->SetMessage(sMessage);
    mpPanel->ChildAdd(mpDeleteMarkedButton);





    // Transformation (rotation, flip, etc.)
    rButton.OffsetRect(rButton.Width() + gSpacer * 4, 0);
    rButton.right = rButton.left + rButton.Height();    // square button


    pBtn = new ZWinSizablePushBtn();
//    ZWinSizablePushBtn* pBtn = mpRotationMenu->AddButton("Transform", sMessage);


    rButton.OffsetRect(rButton.Width() + gSpacer * 2, 0);
    rButton.right = rButton.left + (int64_t)(rButton.Width()*1.25 );     // wider buttons for management

//    rButton.right = rButton.left + rButton.Width();     // wider buttons for management

    bool bRet = pBtn->mSVGImage.Load(sAppPath +"/res/rotate.svg");
    assert(bRet);
    Sprintf(sMessage, "show_rotation_menu;target=%s;r=%s", GetTargetName().c_str(), RectToString(rButton).c_str());
    pBtn->SetMessage(sMessage);
    pBtn->SetArea(rButton);
    pBtn->msWinGroup = "Rotate";
    mpPanel->ChildAdd(pBtn);


    rButton.right = rButton.left + (int64_t)(rButton.Width() * 1.5);     // wider buttons for management

    rButton.OffsetRect(rButton.Width() + gSpacer * 2, 0);
/*
    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = ">"; // unicode rotate right
    pBtn->mCaption.style = unicodeStyle;
    pBtn->SetArea(rButton);
    Sprintf(sMessage, "rotate_right;target=%s", GetTargetName().c_str());

//    Sprintf(sMessage, "show_rotation_menu;target=%s", GetTargetName().c_str());

    pBtn->SetMessage(sMessage);
    pBtn->msWinGroup = "Rotate";
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width(), 0);

    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = "-"; // unicode flip H
    pBtn->mCaption.style = unicodeStyle;
    pBtn->SetArea(rButton);
    Sprintf(sMessage, "flipH;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    pBtn->msWinGroup = "Rotate";
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width(), 0);

    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = "|"; // unicode flip V
    pBtn->mCaption.style = unicodeStyle;
    pBtn->SetArea(rButton);
    Sprintf(sMessage, "flipV;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    pBtn->msWinGroup = "Rotate";
    mpPanel->ChildAdd(pBtn);


    */


    // Filter group

    ZGUI::Style filterButtonStyle = gDefaultGroupingStyle;
    filterButtonStyle.look.decoration = ZGUI::ZTextLook::kEmbossed;
    filterButtonStyle.fp.nHeight = nGroupSide / 3;
    filterButtonStyle.pos = ZGUI::C;
    filterButtonStyle.wrap = true;
    filterButtonStyle.look.colTop = 0xff888888;
    filterButtonStyle.look.colBottom = 0xff888888;


    // All
    ZWinCheck* pCheck = new ZWinCheck();
    pCheck->SetState(true, false);
    pCheck->SetMessages(ZMessage("filter_all", this), "");
    pCheck->mCaption.sText = "all";
    pCheck->mCheckedStyle = filterButtonStyle;
    pCheck->mCheckedStyle.look.colTop = 0xffffffff;
    pCheck->mCheckedStyle.look.colBottom = 0xffffffff;

    pCheck->mUncheckedStyle = filterButtonStyle;

    pCheck->SetArea(rButton);
    pCheck->SetTooltip("All images");
    pCheck->msWinGroup = "Filter";
    pCheck->msRadioGroup = "FilterGroup";
    mpPanel->ChildAdd(pCheck);
    mpAllFilterButton = pCheck;

    // ToBeDeleted
    rButton.OffsetRect(rButton.Width(), 0);
    pCheck = new ZWinCheck();
    pCheck->SetMessages(ZMessage("filter_del", this), "");
    pCheck->mCaption.sText = "del";
    pCheck->mCheckedStyle = filterButtonStyle;
    pCheck->mCheckedStyle.look.colTop = 0xffff4444;
    pCheck->mCheckedStyle.look.colBottom = 0xffff4444;

    pCheck->mUncheckedStyle = filterButtonStyle;

    pCheck->SetArea(rButton);
    pCheck->SetTooltip("Images marked for deletion");
    pCheck->msWinGroup = "Filter";
    pCheck->msRadioGroup = "FilterGroup";
    mpPanel->ChildAdd(pCheck);
    mpDelFilterButton = pCheck;

    // Favorites
    rButton.OffsetRect(rButton.Width(), 0);

    pCheck = new ZWinCheck();
    pCheck->SetMessages(ZMessage("filter_favs", this), "");
    pCheck->mCaption.sText = "favs";
    pCheck->mCheckedStyle = filterButtonStyle;
    pCheck->mCheckedStyle.look.colTop = 0xffe1b131;
    pCheck->mCheckedStyle.look.colBottom = 0xffe1b131;

    pCheck->mUncheckedStyle = filterButtonStyle;

    pCheck->SetArea(rButton);
    pCheck->SetTooltip("Favorites");
    pCheck->msWinGroup = "Filter";
    pCheck->msRadioGroup = "FilterGroup";
    mpPanel->ChildAdd(pCheck);
    mpFavsFilterButton = pCheck;

    // Ranked
    rButton.OffsetRect(rButton.Width(), 0);

    pCheck = new ZWinCheck();
    pCheck->SetMessages(ZMessage("filter_ranked", this), "");
    pCheck->mCaption.sText = "ranked";
    pCheck->mCheckedStyle = filterButtonStyle;
    pCheck->mCheckedStyle.look.colTop = 0xffe1b131;
    pCheck->mCheckedStyle.look.colBottom = 0xffe1b131;

    pCheck->mUncheckedStyle = filterButtonStyle;

    pCheck->SetArea(rButton);
    pCheck->SetTooltip("Ranked");
    pCheck->msWinGroup = "Filter";
    pCheck->msRadioGroup = "FilterGroup";
    mpPanel->ChildAdd(pCheck);
    mpRankedFilterButton = pCheck;






  
    mpFavsFilterButton->SetState(mFilterState == kFavs, false);
    mpDelFilterButton->SetState(mFilterState == kToBeDeleted, false);
    mpAllFilterButton->SetState(mFilterState == kAll, false);
    mpRankedFilterButton->SetState(mFilterState == kRanked, false);









    if (gGraphicSystem.mbFullScreen)
    {
        ZRect rExit(nControlPanelSide, nControlPanelSide);
        rExit = ZGUI::Arrange(rExit, mAreaToDrawTo, ZGUI::RT);
        pBtn = new ZWinSizablePushBtn();
        pBtn->mSVGImage.Load(sAppPath + "/res/exit.svg");
        pBtn->SetTooltip("Exit");
        pBtn->SetArea(rExit);
        pBtn->SetMessage(ZMessage("quit", this));
        mpPanel->ChildAdd(pBtn);

    }

    rButton.SetRect(0, 0, nGroupSide, nGroupSide);
    rButton = ZGUI::Arrange(rButton, rPanelArea, ZGUI::RC, gSpacer / 2);
    rButton.OffsetRect(-gSpacer * 2, 0);
    rButton.OffsetRect(-nControlPanelSide, 0);


    pBtn = new ZWinSizablePushBtn();
    pBtn->mSVGImage.Load(sAppPath + "/res/fullscreen.svg");
    pBtn->msWinGroup = "View";
    pBtn->SetArea(rButton);
    pBtn->SetTooltip("Toggle Fullscreen");
    pBtn->SetMessage("toggle_fullscreen");
    pBtn->msWinGroup = "View";
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(-rButton.Width(), 0);

    pCheck = new ZWinCheck(&mbSubsample);
    pCheck->SetMessages(ZMessage("invalidate", this), ZMessage("invalidate", this));
    pCheck->mSVGImage.Load(sAppPath + "/res/subpixel.svg");
    pCheck->SetArea(rButton);
    pCheck->SetTooltip("Quality Render");
    pCheck->msWinGroup = "View";
    mpPanel->ChildAdd(pCheck);


    rButton.OffsetRect(-rButton.Width(), 0);
    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = "?"; // unicode flip H
    pBtn->mCaption.style = filterButtonStyle;
    pBtn->mCaption.style.fp.nHeight = nGroupSide / 2;
    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("show_help", this));
    pBtn->msWinGroup = "View";
    mpPanel->ChildAdd(pBtn);


    // Contest Button


    rButton.right = rButton.left + rButton.Width() * 4;
    rButton.OffsetRect(-rButton.Width()-gSpacer*2, 0);
    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = "Contest";
    pBtn->mCaption.style = gStyleButton;
    pBtn->mCaption.style.pos = ZGUI::C;
    pBtn->SetTooltip("Rank images in pairs");
    pBtn->SetArea(rButton);
    Sprintf(sMessage, "show_contest;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);


}

bool ImageViewer::OnParentAreaChange()
{
    if (!mpTransformTexture)
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpTransformTexture.get()->GetMutex());
    SetArea(mpParentWin->GetArea());

    ZWin::OnParentAreaChange();
    UpdateUI();

//    static int count = 0;
//    ZOUT("ImageViewer::OnParentAreaChange\n", count++);

    return true;
}


bool ImageViewer::ViewImage(const std::filesystem::path& filename)
{
    if (!filename.empty() && filesystem::exists(filename))
    {
        if (mCurrentFolder != filename.parent_path())
            ScanForImagesInFolder(filename.parent_path());
    }

    mToggleUIHotkey = VK_TAB;
    UpdateFilteredView(kAll);   // set filter to all and fill out del and fav arrays

    mViewingIndex = IndexFromPath(filename);

    if (mViewingIndex.favIndex >= 0) // if current image is fav, set that as the filter
        UpdateFilteredView(kFavs);
    else if (mViewingIndex.delIndex >= 0)   // same with del
        UpdateFilteredView(kToBeDeleted);

    UpdateControlPanel();
    mCachingState = kReadingAhead;
    KickCaching();

    return true;
}

void ImageViewer::LoadMetadataProc(std::filesystem::path& imagePath, shared_ptr<ImageEntry> pEntry, std::atomic<int64_t>* pnOutstanding)
{
    if (!pEntry || !pnOutstanding)
        return;

//    ZOUT("Loading EXIF:", imagePath, "\n");

    pEntry->mMeta = gImageMeta.Entry(imagePath.string());

    if (ZBuffer::ReadEXIFFromFile(imagePath.string(), pEntry->mEXIF))
    {
        pEntry->mState = ImageEntry::kMetadataReady;
    }
    else
        pEntry->mState = ImageEntry::kNoExifAvailable;

    (*pnOutstanding)--;
    int64_t n = *pnOutstanding;
    ZDEBUG_OUT("outstanding:", n, "\n");
}

void ImageViewer::FlushLoads()
{
    delete mpImageLoaderPool;  // will join
    mpImageLoaderPool = new ThreadPool(std::thread::hardware_concurrency() / 4);

    delete mpMetadataLoaderPool;
    mpMetadataLoaderPool = new ThreadPool(1);
}



void ImageViewer::LoadImageProc(std::filesystem::path& imagePath, shared_ptr<ImageEntry> pEntry)
{
    if (!pEntry)
        return;

//    ZOUT("Loading:", imagePath, "\n");

    tZBufferPtr pNewImage(new ZBuffer);

    if (imagePath.extension() == ".svg")
        pNewImage->Init(100, 100);

    if (!pNewImage->LoadBuffer(imagePath.string()))
    {
        pNewImage->Init(1920, 1080);
        string sError;
        Sprintf(sError, "Error loading image:\n%s", imagePath.string().c_str());

        ZGUI::Style errorStyle(gStyleCaption);
        errorStyle.look.colTop = 0xffff0000;
        errorStyle.look.colBottom = 0xffff0000;
        errorStyle.pos = ZGUI::C;
        errorStyle.Font()->DrawTextParagraph(pNewImage.get(), sError, pNewImage->GetArea(), &errorStyle);

        cerr << "Error loading\n";
//        return nullptr;
    }

    pEntry->pImage = pNewImage;
    pEntry->mState = ImageEntry::kLoaded;
}

tZBufferPtr ImageViewer::GetCurImage()
{
    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
    if (!ValidIndex(mViewingIndex))    // also handles empty array case
        return nullptr;

    return mImageArray[mViewingIndex.absoluteIndex]->pImage;
}

int64_t ImageViewer::CurMemoryUsage()
{
    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);

    int64_t nUsage = 0;
    for (auto& i : mImageArray)
    {
        if (i->pImage)
        {
            ZRect rArea = i->pImage->GetArea();
            nUsage += rArea.Width() * rArea.Height() * 4;
        }
        else if (i->mState == ImageEntry::kLoadInProgress)
        {
            int64_t nBytesPending = (i->mEXIF.ImageWidth * i->mEXIF.ImageHeight) * 4;
            nUsage += nBytesPending;
        }
    }

    return nUsage;
}

int64_t ImageViewer::GetLoadsInProgress()
{
    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
    int64_t nCount = 0;
    for (auto& i : mImageArray)
    {
        if (i->mState == ImageEntry::kLoadInProgress)
            nCount++;
    }

    return nCount;
}


// check ahead in the direction of last action....two forward, one back
tImageEntryPtr ImageViewer::NextImageToCache()
{
    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
    if (!ValidIndex(mViewingIndex))
        return nullptr;

    int64_t dir = 1;
    if (mLastAction == kPrevImage || mViewingIndex.absoluteIndex == mImageArray.size()-1)
        dir = -1;
    
    // First priority is caching current view
    if (mImageArray[mViewingIndex.absoluteIndex]->mState < ImageEntry::kLoadInProgress)
    {
        return mImageArray[mViewingIndex.absoluteIndex];
    }


    for (int64_t distance = 1; distance < mMaxCacheReadAhead; distance++)
    {
        ViewingIndex vi = mViewingIndex;

        int64_t offset = dir * distance;
        if (FindImageMatchingCurFilter(vi, offset))
        {
            tImageEntryPtr entry = EntryFromIndex(vi);
            if (entry && entry->mState < ImageEntry::kLoadInProgress)
                return entry;
        }

        offset = -dir * distance;

        vi = mViewingIndex;
        if (FindImageMatchingCurFilter(vi, offset))
        {
            tImageEntryPtr entry = EntryFromIndex(vi);
            if (entry && entry->mState < ImageEntry::kLoadInProgress)
                return entry;
        }
    }

    return nullptr;
}

bool ImageViewer::FreeCacheMemory()
{

    const int64_t kUnloadViewDistance = mMaxCacheReadAhead;

    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
    if (CurMemoryUsage() > mMaxMemoryUsage)
    {
        // unload everything not matching current filter
        if (mFilterState == kToBeDeleted)
        {
            for (auto& i : mImageArray)
            {
                if (!i->ToBeDeleted())
                    i->Unload();
            }
        }
        else if (mFilterState == kFavs)
        {
            for (auto& i : mImageArray)
            {
                if (!i->IsFavorite())
                    i->Unload();
            }
        }
    }

    tImageEntryArray* pArrayToScan = &mImageArray;
    if (mFilterState == kFavs)
        pArrayToScan = &mFavImageArray;
    else if (mFilterState == kToBeDeleted)
        pArrayToScan = &mToBeDeletedImageArray;
    else if (mFilterState == kRanked)
        pArrayToScan = &mRankedArray;

    int64_t nCurIndex = IndexInCurMode();


    // unload everything over mMaxCacheReadAhead
    for (int64_t i = 0; i < (int64_t)pArrayToScan->size(); i++)
    {
        if (std::abs(i - nCurIndex) > kUnloadViewDistance && (*pArrayToScan)[i]->mState == ImageEntry::kLoaded)
        {
//            ZOUT("Unloading viewed:", i, "\n");
            (*pArrayToScan)[i]->Unload();
        }
    }

    // progressively look for closer images to unload until under budget
    for (int64_t nDistance = kUnloadViewDistance-1; nDistance > 1 && CurMemoryUsage() > mMaxMemoryUsage; nDistance--)
    {
        // make memory available if over budget
        for (int64_t i = 0; i < (int64_t)pArrayToScan->size() && CurMemoryUsage() > mMaxMemoryUsage; i++)
        {
            if (std::abs(i - nCurIndex) > nDistance && (*pArrayToScan)[i]->mState == ImageEntry::kLoaded)
            {
//                ZOUT("Unloading viewed:", i, "\n");
                (*pArrayToScan)[i]->Unload();
            }
        }
    }

    return true;
}

bool ImageViewer::KickMetadataLoading()
{
    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
    mOutstandingMetadataCount = mImageArray.size();;
    static int64_t enqueued = 0;

    // kick off exif reading if necessary
    for (auto& entry : mImageArray)
    {
        if (entry->mState == ImageEntry::kInit)
        {
            entry->mState = ImageEntry::kLoadingMetadata;
            mpImageLoaderPool->enqueue(&ImageViewer::LoadMetadataProc, entry->filename, entry, &mOutstandingMetadataCount);
            enqueued++;
        }
    }

    return true;
}

bool ImageViewer::KickCaching()
{
    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);

    // loading current image is top priority
    tImageEntryPtr entry = EntryFromIndex(mViewingIndex);
    if (entry && entry->mState < ImageEntry::kLoadInProgress)
    {
        entry->mState = ImageEntry::kLoadInProgress;
        mpImageLoaderPool->enqueue(&ImageViewer::LoadImageProc, entry->filename, entry);
    }

    if (GetLoadsInProgress() > (int64_t) mpImageLoaderPool->size())
        return false;

    if (CurMemoryUsage() < mMaxMemoryUsage)
    {
        tImageEntryPtr entry = NextImageToCache();

        if (!entry)
        {
            mCachingState = kWaiting;   // wait for any additional actions before resuming caching
            return false;
        }

        if (entry->mState < ImageEntry::kLoadInProgress)
        {
//            ZOUT("caching image ", entry->filename, "\n");
            entry->mState = ImageEntry::kLoadInProgress;
            mpImageLoaderPool->enqueue(&ImageViewer::LoadImageProc, entry->filename, entry);
        }
    }
    else
        mCachingState = kWaiting;   // wait for any additional actions before resuming caching

    return true;
}


bool ImageViewer::AcceptedExtension(std::string sExt)
{
    if (sExt.empty())
        return false;
    SH::makelower(sExt);
    if (sExt[0] == '.')
        sExt = sExt.substr(1);  // remove leading . if it's tehre

    for (auto& s : mAcceptedExtensions)
    {
        if (sExt == s)
            return true;
    }

    return false;
}

void ImageViewer::Clear()
{
    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);

    FlushLoads();
    mImageArray.clear();
    mCurrentFolder.clear();
    mRankedArray.clear();
    mToBeDeletedImageArray.clear();
    mFavImageArray.clear();
    mRankedImageMetadata.clear();
    mViewingIndex = {};
    mLastAction = kNone;
    if (mpWinImage)
        mpWinImage->Clear();

    if (mpRatedImagesStrip)
    {
        ChildDelete(mpRatedImagesStrip);
        mpRatedImagesStrip = nullptr;
    }

    UpdateCaptions();
    Invalidate();
}


bool ImageViewer::ScanForImagesInFolder(std::filesystem::path folder)
{
    // if passing in a filename, scan the containing folder
    if (filesystem::is_regular_file(folder))
        folder = folder.parent_path();

    if (!std::filesystem::exists(folder))
        return false;


    string sTail = folder.filename().string();

    // if loading either favorites or to_be_deleted images, scan the parent folder
    if (sTail == ksFavorites || sTail == ksToBeDeleted)
        folder = folder.parent_path();


//    if (folder == mCurrentFolder)
//        return true;




    Clear();

    mCurrentFolder = folder;

    bool bErrors = false;

    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
    for (auto filePath : std::filesystem::directory_iterator(mCurrentFolder))
    {
        if (filePath.is_regular_file() && AcceptedExtension(filePath.path().extension().string()))
        {
            mImageArray.emplace_back(new ImageEntry(filePath));
            //ZDEBUG_OUT("Found image:", filePath, "\n");
        }
    }

    if (filesystem::exists(FavoritesPath()))
    {
        for (auto filePath : std::filesystem::directory_iterator(FavoritesPath()))
        {
            if (filePath.is_regular_file() && AcceptedExtension(filePath.path().extension().string()))
            {
/*                if (ValidIndex(IndexFromPath(filePath)))
                {
                    // Duplicate image in both favorites and non-favorites folder
                    ZERROR("Duplicate image in both favorites and regular folder! Please ensure it's in one or the other:", filePath.path().string().c_str());
                    bErrors = true;
                }
                else*/
                    mImageArray.emplace_back(new ImageEntry(filePath));
                //ZDEBUG_OUT("Found image:", filePath, "\n");
            }
        }
    }

    if (filesystem::exists(ToBeDeletedPath()))
    {
        for (auto filePath : std::filesystem::directory_iterator(ToBeDeletedPath()))
        {
            if (filePath.is_regular_file() && AcceptedExtension(filePath.path().extension().string()))
            {
/*                if (ValidIndex(IndexFromPath(filePath)))
                {
                    // Duplicate image in both to be deleted and non-favorites folder
                    ZERROR("Duplicate image in both to_be_deleted and regular folder! Please ensure it's in one or the other:", filePath.path().string().c_str());
                    bErrors = true;
                }
                else*/
                    mImageArray.emplace_back(new ImageEntry(filePath));
                //ZDEBUG_OUT("Found image:", filePath, "\n");
            }
        }
    }

    if (bErrors)
    {
        gMessageSystem.Post("toggleconsole");
        return false;
    }


    std::sort(mImageArray.begin(), mImageArray.end(), [](const shared_ptr<ImageEntry>& a, const shared_ptr<ImageEntry>& b) -> bool { return a->filename.filename().string() < b->filename.filename().string(); });

    KickMetadataLoading();

    if (!ValidIndex(mViewingIndex))
        SetFirstImage();

    UpdateUI();

    return true;
}

ViewingIndex ImageViewer::IndexFromPath(const std::filesystem::path& imagePath)
{
    ViewingIndex index;
    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
    for (int i = 0; i < mImageArray.size(); i++)
    {
        if (mImageArray[i]->filename.filename() == imagePath.filename())
        {
            index.absoluteIndex = i;
            break;
        }
    }

    for (int i = 0; i < mToBeDeletedImageArray.size(); i++)
    {
        if (mToBeDeletedImageArray[i]->filename.filename() == imagePath.filename())
        {
            index.delIndex = i;
            break;
        }
    }

    for (int i = 0; i < mFavImageArray.size(); i++)
    {
        if (mFavImageArray[i]->filename.filename() == imagePath.filename())
        {
            index.favIndex = i;
            break;
        }
    }

    for (int i = 0; i < mRankedArray.size(); i++)
    {
        if (mRankedArray[i]->filename.filename() == imagePath.filename())
        {
            index.rankedIndex = i;
            break;
        }
    }

    return index;
}


filesystem::path ImageViewer::ToBeDeletedPath() 
{ 
    filesystem::path tbd(mCurrentFolder);
    tbd.append(ksToBeDeleted);
    return tbd;
}

filesystem::path ImageViewer::FavoritesPath() 
{ 
    filesystem::path fav(mCurrentFolder);
    fav.append(ksFavorites);
    return fav;
}



bool ImageViewer::ValidIndex(const ViewingIndex& vi)
{
    return vi.absoluteIndex >= 0 && vi.absoluteIndex < (int64_t)mImageArray.size();
}

tImageEntryPtr ImageViewer::EntryFromIndex(const ViewingIndex& vi)
{
    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
    if (!ValidIndex(vi))
        return nullptr;

    return mImageArray[vi.absoluteIndex];
}

int64_t ImageViewer::IndexInCurMode()
{
    if (mFilterState == kToBeDeleted)
        return mViewingIndex.delIndex;
    else if (mFilterState == kFavs)
        return mViewingIndex.favIndex;
    else if (mFilterState == kRanked)
        return mViewingIndex.rankedIndex;

    return mViewingIndex.absoluteIndex;
}

int64_t ImageViewer::CountInCurMode()
{
    if (mFilterState == kToBeDeleted)
        return mToBeDeletedImageArray.size();
    else if (mFilterState == kFavs)
        return mFavImageArray.size();
    else if (mFilterState == kRanked)
        return mRankedArray.size();

    return mImageArray.size();;
}



#ifdef _DEBUG

#ifdef DEBUG_CACHE

string ImageViewer::LoadStateString()
{
    string state;
    for (int i = 0; i < mImageArray.size(); i++)
    {
        if (mImageArray[i]->pImage)
            state = state + " [" + SH::FromInt(i) + "]";
        else
            state = state + " " + SH::FromInt(i);
    }

    return state;
}
#endif

#endif

int64_t ImageViewer::GetRank(const std::string& sFilename)
{
    int64_t nRank = 1;
    for (auto& i : mRankedImageMetadata)
    {
        if (i.filename == sFilename)
            return nRank;
        nRank++;
    }

    return -1;
}

std::string ImageViewer::GetRankedFilename(int64_t nRank)
{
    if (nRank > (int64_t)mRankedImageMetadata.size())
        return "";

    tImageMetaList::iterator it = mRankedImageMetadata.begin();
    while (nRank-- > 1) // 1 not 0 because ranks are 1 through N
        it++;

    return (*it).filename;
}


bool ImageViewer::Process()
{
    if (mpWinImage && !mImageArray.empty())
    {
        if (mOutstandingMetadataCount == 0)
        {
            mOutstandingMetadataCount = -1;

            for (auto& i : mImageArray)
            {
                if (i->mMeta.elo > 0)
                    mRankedArray.emplace_back(i);
            }

            std::sort(mRankedArray.begin(), mRankedArray.end(), [](const shared_ptr<ImageEntry>& a, const shared_ptr<ImageEntry>& b) -> bool { return a->mMeta.elo > b->mMeta.elo; });

            mRankedImageMetadata.clear();
            for (auto& i : mRankedArray)
            {
                ImageMetaEntry rankedEntry(i->mMeta);
                mRankedImageMetadata.emplace_back(std::move(rankedEntry));
            }

            UpdateUI();

        }


        tZBufferPtr curImage = GetCurImage();

        if (curImage && mpWinImage->mpImage.get() != curImage.get())
        {
            mpWinImage->SetImage(curImage);
            ZDEBUG_OUT("Setting image:", curImage->GetEXIF().DateTime, "\n");

            mpWinImage->mCaptionMap["no_image"].Clear();

            const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
            gRegistry["ZImageViewer"]["image"] = mImageArray[mViewingIndex.absoluteIndex]->filename.string();

            UpdateCaptions();
            InvalidateChildren();
        }

/*        if (!curImage)
        {
            mpWinImage->mCaptionMap["no_image"].sText = "No images\nPress 'O' or TAB";
            mpWinImage->mCaptionMap["no_image"].style = gStyleCaption;
            mpWinImage->mCaptionMap["no_image"].visible = true;
            InvalidateChildren();
        }*/

        if (mCachingState == kReadingAhead)
            KickCaching();
    }

#ifdef DEBUG_CACHE

#ifdef _DEBUG
    static int64_t nLastReport = 0;

    int64_t nTime = gTimer.GetElapsedTime();
    if (nTime - nLastReport > 1000)
    {
        ZOUT("state:   ", LoadStateString(), "\n\n");
        nLastReport = nTime;
    }
#endif

#endif

    return ZWin::Process();
}


void ImageViewer::UpdateFilteredView(eFilterState state)
{
    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);

    // remember currently viewed image
    filesystem::path currentViewPicturePath;
    if (ValidIndex(mViewingIndex))
        currentViewPicturePath = mImageArray[mViewingIndex.absoluteIndex]->filename;

    mToBeDeletedImageArray.clear();
    mFavImageArray.clear();

    for (auto i : mImageArray)
    {
        if (i->ToBeDeleted())
            mToBeDeletedImageArray.push_back(i);
        else if (i->IsFavorite())
            mFavImageArray.push_back(i);
    }


    mFilterState = state;
    if (CountImagesMatchingFilter(mFilterState) == 0)
    {
        mViewingIndex = {};
        mpWinImage->Clear();
    }
    else
    {
        mpWinImage->mCaptionMap["no_image"].Clear();

        if (currentViewPicturePath.empty())
            SetFirstImage();
        else
            mViewingIndex = IndexFromPath(currentViewPicturePath);

        // If the previosly viewed image doesn't match the current filter, find the nearest one that does (first forward then back)
        if (!ImageMatchesCurFilter(mViewingIndex))
        {
            // try setting next image that matches filter
            SetNextImage();

            // if no next image, try setting previous.... 
            if (!ImageMatchesCurFilter(mViewingIndex))
                SetPrevImage();
        }
    }

    UpdateUI();
//    UpdateCaptions();
//    InvalidateChildren();
}

void ImageViewer::UpdateCaptions()
{
    bool bShow = mbShowUI;
    if (gInput.IsKeyDown(VK_MENU))
        bShow = true;

    mpWinImage->mCaptionMap["favorite"].Clear();
    mpWinImage->mCaptionMap["for_delete"].Clear();
    mpWinImage->mCaptionMap["no_image"].Clear();
    mpWinImage->mCaptionMap["image_count"].Clear();
    mpWinImage->mCaptionMap["rank"].Clear();

    string sCaption;
    Sprintf(sCaption, "Favorites\n(%d)", mFavImageArray.size());

    // need to make sure panel isn't being reset while updating these
    mPanelMutex.lock();
    if (mpFavsFilterButton)
    {
        mpFavsFilterButton->mCaption.sText = sCaption;
        mpFavsFilterButton->mbEnabled = !mFavImageArray.empty();
    }

    Sprintf(sCaption, "To Be Del\n(%d)", mToBeDeletedImageArray.size());
    if (mpDelFilterButton)
    {
        mpDelFilterButton->mCaption.sText = sCaption;
        mpDelFilterButton->mbEnabled = !mToBeDeletedImageArray.empty();
    }

 
    Sprintf(sCaption, "All\n(%d)", mImageArray.size());
    if (mpAllFilterButton)
        mpAllFilterButton->mCaption.sText = sCaption;

/*    Sprintf(sCaption, "%d ranked", mRankedArray.size());
    if (mpRankedFilterButton)
        mpRankedFilterButton->mCaption.sText = sCaption;*/
    if (mpRankedFilterButton)
    {
        mpRankedFilterButton->mCaption.sText = "ranked";
        mpRankedFilterButton->mbEnabled = !mRankedArray.empty();
    }

    if (mpDeleteMarkedButton)
    {
        mpDeleteMarkedButton->mbEnabled = !mToBeDeletedImageArray.empty();
    }


    mPanelMutex.unlock();


    if (mpWinImage)
    {
        int64_t topPadding = 0; // in case panel is visible
        if (mpPanel)
            topPadding = mpPanel->GetArea().Height();


        ZGUI::Style folderStyle(gStyleButton);
        folderStyle.pos = ZGUI::LT;
        folderStyle.look = ZGUI::ZTextLook::kShadowed;
        folderStyle.paddingH = (int32_t) (gSpacer / 2);
        folderStyle.paddingV = (int32_t)(gSpacer / 2);


        if (CountImagesMatchingFilter(mFilterState) == 0)
        {
            mpWinImage->mCaptionMap["no_image"].sText = "No images\nPress 'O' or TAB";
            mpWinImage->mCaptionMap["no_image"].style = gStyleCaption;
            mpWinImage->mCaptionMap["no_image"].visible = true;
            bShow = true;
        }
        else
        {
            if (bShow)
            {
                mpWinImage->mpTable->mCellStyle = gStyleCaption;
                mpWinImage->mpTable->mCellStyle.pos = ZGUI::LC;
                mpWinImage->mpTable->mCellStyle.paddingH = (int32_t)(gSpacer / 2);
                mpWinImage->mpTable->mCellStyle.paddingV = (int32_t)(gSpacer / 2);

                mpWinImage->mpTable->mTableStyle.pos = ZGUI::RB;
                mpWinImage->mpTable->mTableStyle.bgCol = 0x88000000;
                mpWinImage->mpTable->mTableStyle.paddingH = (int32_t)gSpacer;
                mpWinImage->mpTable->mTableStyle.paddingV = (int32_t)gSpacer;

                string sImageCount = "[" + SH::FromInt(IndexInCurMode() + 1) + "/" + SH::FromInt(CountInCurMode()) + "]";
                mpWinImage->mCaptionMap["image_count"].sText = sImageCount;
                mpWinImage->mCaptionMap["image_count"].style = folderStyle;
                mpWinImage->mCaptionMap["image_count"].style.pos = ZGUI::LB;
                mpWinImage->mCaptionMap["image_count"].visible = true;

                if (ValidIndex(mViewingIndex))
                {
                    string sFilename = EntryFromIndex(mViewingIndex)->filename.filename().string();
                    ZGUI::Style filenameStyle(gStyleButton);
                    filenameStyle.pos = ZGUI::LT;
                    filenameStyle.paddingV += (int32_t)(folderStyle.fp.nHeight);
                    filenameStyle.look = ZGUI::ZTextLook::kShadowed;
                    mpWinImage->mCaptionMap["filename"].sText = sFilename;
                    mpWinImage->mCaptionMap["filename"].style = filenameStyle;
                    mpWinImage->mCaptionMap["filename"].visible = true;

                    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
                    if (mImageArray[mViewingIndex.absoluteIndex]->IsFavorite() && mpFavoritesFont)
                    {
                        mpWinImage->mCaptionMap["favorite"].sText = "C";
                        mpWinImage->mCaptionMap["favorite"].style = ZGUI::Style(mpFavoritesFont->GetFontParams(), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xffe1b131, 0xffe1b131), ZGUI::RT, (int32_t)filenameStyle.fp.nHeight * 2, (int32_t)filenameStyle.fp.nHeight * 2, 0x88000000, true);
                        mpWinImage->mCaptionMap["favorite"].visible = true;
                    }

                    if (mImageArray[mViewingIndex.absoluteIndex]->ToBeDeleted())
                    {
                        mpWinImage->mCaptionMap["for_delete"].sText = /*mImageArray[mnViewingIndex].filename.filename().string() +*/ "\nMARKED FOR DELETE";
                        mpWinImage->mCaptionMap["for_delete"].style = ZGUI::Style(ZFontParams("Ariel Bold", 100, 400), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xffff0000, 0xffff0000), ZGUI::CB, (int32_t)(gSpacer / 2), 100, 0x88000000, true);
                        mpWinImage->mCaptionMap["for_delete"].visible = true;
                        bShow = true;
                    }

                    ImageMetaEntry& meta = mImageArray[mViewingIndex.absoluteIndex]->mMeta;
//                    if (meta.elo > 1000)
                    {
                        int64_t nRank = GetRank(meta.filename);
                        if (nRank > 0)
                        {
                            ZGUI::Style eloStyle(gStyleCaption);
                            eloStyle.fp.nHeight = gM * 2;
                            eloStyle.look.colTop = 0xffffd800;
                            eloStyle.look.colBottom = 0xff9d8500;
                            eloStyle.look.decoration = ZGUI::ZTextLook::kShadowed;
                            eloStyle.pos = ZGUI::RT;
                            eloStyle.paddingH += (int32_t)eloStyle.fp.nHeight;
                            Sprintf(mpWinImage->mCaptionMap["rank"].sText, "#%d\n%d", nRank, meta.elo);
                            mpWinImage->mCaptionMap["rank"].visible = true;
                            mpWinImage->mCaptionMap["rank"].style = eloStyle;
                        }
                    }
                }
            }


        }


        mpWinImage->mCaptionMap["folder"].sText = mCurrentFolder.string();
        mpWinImage->mCaptionMap["folder"].style = folderStyle;
        mpWinImage->mCaptionMap["folder"].visible = bShow;



        if (!mMoveToFolder.empty())
        {
            Sprintf(mpWinImage->mCaptionMap["move_to_folder"].sText, "'M' -> move to:\n%s", mMoveToFolder.string().c_str());
            mpWinImage->mCaptionMap["move_to_folder"].style = ZGUI::Style(ZFontParams("Ariel Bold", folderStyle.fp.nHeight, 400), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xff0088ff, 0xff0088ff), ZGUI::LT, (int32_t)(gSpacer / 2), (int32_t)(gSpacer / 2 + folderStyle.fp.nHeight*8), 0x88000000, true);
            mpWinImage->mCaptionMap["move_to_folder"].visible = bShow;
        }
        else
        {
            mpWinImage->mCaptionMap["move_to_folder"].Clear();
        }

        if (!mCopyToFolder.empty())
        {
            Sprintf(mpWinImage->mCaptionMap["copy_to_folder"].sText, "'C' -> copy to:\n%s", mCopyToFolder.string().c_str());
            mpWinImage->mCaptionMap["copy_to_folder"].style = ZGUI::Style(ZFontParams("Ariel Bold", folderStyle.fp.nHeight, 400), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xff0088ff, 0xff0088ff), ZGUI::LT, (int32_t)(gSpacer / 2), (int32_t)(gSpacer / 2 + folderStyle.fp.nHeight * 12), 0x88000000, true);
            mpWinImage->mCaptionMap["copy_to_folder"].visible = bShow;
        }
        else
        {
            mpWinImage->mCaptionMap["move_to_folder"].Clear();
        }

        if (mpWinImage->mpImage)
        {
            easyexif::EXIFInfo& exif = mpWinImage->mpImage->GetEXIF();
            string sF;
            if (std::fmod(exif.FNumber, 1.0) == 0)
                Sprintf(sF, "%d", (int)exif.FNumber);
            else
                Sprintf(sF, "%.1f", exif.FNumber);

            ZRect rArea(mpWinImage->mpImage->GetArea());
            string sExposure;
            if (exif.FocalLength > 0)
                Sprintf(sExposure, "%dmm  1/%ds  f%s  ISO:%d", (int)exif.FocalLength, (int)(1.0 / exif.ExposureTime), sF.c_str(), exif.ISOSpeedRatings);


            string sSize;
            Sprintf(sSize, "%dx%d", mpWinImage->mpImage.get()->GetArea().Width(), mpWinImage->mpImage.get()->GetArea().Height());

            const std::lock_guard<std::recursive_mutex> lock(mpWinImage->mpTable->mTableMutex);

            mpWinImage->mpTable->Clear();

            if (!exif.LensInfo.Model.empty())
                mpWinImage->mpTable->AddRow("Lens", exif.LensInfo.Model);

            if (!exif.DateTime.empty())
                mpWinImage->mpTable->AddRow("Date", exif.DateTime);

            if (!sExposure.empty())
                mpWinImage->mpTable->AddRow("Exposure", sExposure);

            if (exif.GeoLocation.Latitude != 0.0)
            {
                string sGeo;
                Sprintf(sGeo, "lat:%0.3f lng:%0.3f", exif.GeoLocation.Latitude, exif.GeoLocation.Longitude);
                mpWinImage->mpTable->AddRow("Location", sGeo);
            }

            mpWinImage->mpTable->AddRow("Size", sSize);

            ZGUI::Style style(mpWinImage->mpTable->mCellStyle);
            style.fp.nWeight = 800;
            style.fp.nHeight = (int64_t) (style.fp.nHeight*1.2);

            mpWinImage->mpTable->SetColStyle(0, style);
        }
        else
        {
            const std::lock_guard<std::recursive_mutex> lock(mpWinImage->mpTable->mTableMutex);
            mpWinImage->mpTable->Clear();
        }

        if (bShow)
            mpWinImage->mBehavior |= ZWinImage::kShowCaption;
        else
            mpWinImage->mBehavior &= ~ZWinImage::kShowCaption;



        mpWinImage->Invalidate();
    }
}

void ImageViewer::ToggleToBeDeleted()
{
    if (!ValidIndex(mViewingIndex))
        return;

    filesystem::path toBeDeleted = mCurrentFolder;
    toBeDeleted.append(ksToBeDeleted);

    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
    if (mImageArray[mViewingIndex.absoluteIndex]->ToBeDeleted())
    {
        // move from subfolder up
        MoveImage(EntryFromIndex(mViewingIndex)->filename, mCurrentFolder);

        if (std::filesystem::is_empty(toBeDeleted))
            std::filesystem::remove(toBeDeleted);
    }
    else
    {
        // move into to be deleted subfolder
        MoveImage(EntryFromIndex(mViewingIndex)->filename, toBeDeleted);
    }

    UpdateFilteredView(mFilterState);
}

void ImageViewer::ToggleFavorite()
{
    if (!ValidIndex(mViewingIndex))
        return;

    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
    if (mImageArray[mViewingIndex.absoluteIndex]->IsFavorite())
    {
        // move from subfolder up
        MoveImage(EntryFromIndex(mViewingIndex)->filename, mCurrentFolder);

        if (std::filesystem::is_empty(FavoritesPath()))
            std::filesystem::remove(FavoritesPath());
    }
    else
    {
        // move into favorites subfolder
        MoveImage(EntryFromIndex(mViewingIndex)->filename, FavoritesPath());
    }

    UpdateFilteredView(mFilterState);
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


    if (mImageArray.empty())
    {
        gStyleCaption.Font()->DrawTextParagraph(mpTransformTexture.get(), "No images", mAreaToDrawTo, &gStyleCaption);
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
   
