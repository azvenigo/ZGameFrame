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
#include "ZGUIStyle.h"
#include <algorithm>
#include "ZWinBtn.H"
#include "helpers/Registry.h"
#include "ZTimer.h"


using namespace std;


template<typename R>
bool is_ready(std::shared_future<R> const& f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}
const int kPoolSize = 8;

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
    mMaxMemoryUsage = 4 * 1024 * 1024 * 1024LL;   // 2GiB
//    mMaxMemoryUsage = 400 * 1024 * 1024LL;
    mMaxCacheReadAhead = 8;
    mnViewingIndex = 0;
    mLastAction = kNone;
    msWinName = "ZWinImageViewer";
    mpPanel = nullptr;
    mpSymbolicFont = nullptr;
    mpFavoritesFont = nullptr;
    mpWinImage = nullptr;
    mpPool = new ThreadPool(kPoolSize);
    mToggleUIHotkey = 0;
    mCachingState = kWaiting;
    mbSubsample = false;
    mFilterState = kAll;
    mpAllFilterButton = nullptr;
    mpFavsFilterButton = nullptr;
    mpDelFilterButton = nullptr;

}
 
ImageViewer::~ImageViewer()
{
    if (mpPool)
        delete mpPool;
}


bool ImageViewer::ShowOpenImageDialog()
{
    string sFilename;
    string sDefaultFolder;
    if (!mCurrentFolder.empty())
        sDefaultFolder = mCurrentFolder.string();
    if (ZWinFileDialog::ShowLoadDialog("Images", "*.jpg;*.jpeg;*.png;*.gif;*.tga;*.bmp;*.psd;*.hdr;*.pic;*.pnm", sFilename, sDefaultFolder))
    {
        mMoveToFolder.clear();
        
        mFilterState = kAll;
        filesystem::path imagePath(sFilename);
        ScanForImagesInFolder(imagePath.parent_path());
        mnViewingIndex = IndexFromPath(imagePath);
        KickCaching();
    }
    return true;

}
void ImageViewer::HandleQuitCommand()
{
    mCachingState = kWaiting;

//    tImageFilenames deletionList = GetImagesFlaggedToBeDeleted();

//    if (deletionList.empty())
    {
        if (mpPool)
        {
            delete mpPool;
            mpPool = nullptr;
        }
        gMessageSystem.Post("quit_app_confirmed");
    }
/*    else
    {
        mnViewingIndex = IndexFromPath(*deletionList.begin());
        ConfirmDeleteDialog* pDialog = ConfirmDeleteDialog::ShowDialog("Please confirm the following files to be deleted", deletionList);
        pDialog->msOnConfirmDelete = ZMessage("delete_confirm", this);
        pDialog->msOnCancel = ZMessage("delete_cancel_and_quit", this);
        pDialog->msOnGoBack = ZMessage("delete_goback", this);
    }*/
}

void ImageViewer::ToggleUI()
{
    if (mpPanel)
    {
        if (!mpPanel->IsVisible())
        {
            mpPanel->SetVisible();
            mpPanel->mbHideOnMouseExit = false;
        }
        else
        {
            mpPanel->SetVisible(false);
            mpPanel->mbHideOnMouseExit = true;
        }

        bool bShowUI = mpPanel->IsVisible() || mImageArray.empty();
        gRegistry["ZImageViewer"]["showui"] = bShowUI;

        UpdateCaptions();

    }
    InvalidateChildren();
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

    if (key == mToggleUIHotkey)
    {
        ToggleUI();
    }


    switch (key)
    {
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
            MoveSelectedFile(mMoveToFolder);
            RemoveImageArrayEntry(mnViewingIndex);
        }
        break;
    case 'o':
    case 'O':
        return ShowOpenImageDialog();
    }


    return true;
}

bool ImageViewer::RemoveImageArrayEntry(int64_t nIndex)
{
    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpTransformTexture.get()->GetMutex());

    if (!ValidIndex(nIndex))
        return false;

    mImageArray.erase(mImageArray.begin() + nIndex);
    limit<int64_t>(mnViewingIndex, 0, mImageArray.size() - 1);
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
    else if (sType == "show_confirm")
    {
        tImageFilenames deletionList = GetImagesFlaggedToBeDeleted();
        ConfirmDeleteDialog* pDialog = ConfirmDeleteDialog::ShowDialog("Please confirm the following files to be deleted", deletionList);
        pDialog->msOnConfirmDelete = ZMessage("delete_confirm", this);
        pDialog->msOnCancel = ZMessage("delete_cancel_and_quit", this);
        pDialog->msOnGoBack = ZMessage("delete_goback", this);
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
        mnViewingIndex = IndexFromPath(message.GetParam("filename"));
        KickCaching();
        return true;
    }
    else if (sType == "set_move_folder")
    {
        HandleMoveCommand();
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
    else if (sType == "invalidate")
    {
        if (mbSubsample && mpWinImage)
            mpWinImage->nSubsampling = 2;
        else
            mpWinImage->nSubsampling = 0;
        InvalidateChildren();
        return true;
    }

    if (mpWinImage && mpWinImage->mpImage)
    {
        if (sType == "rotate_left")
        {
            mpWinImage->mpImage->Rotate(ZBuffer::kLeft);

            if (mpWinImage->IsSizedToWindow())
                mpWinImage->FitImageToWindow();

            Invalidate();
            return true;
        }
        else if (sType == "rotate_right")
        {
            mpWinImage->mpImage->Rotate(ZBuffer::kRight);
            if (mpWinImage->IsSizedToWindow())
                mpWinImage->FitImageToWindow();

            Invalidate();
            return true;
        }
        else if (sType == "flipH")
        {
            mpWinImage->mpImage->Rotate(ZBuffer::kHFlip);
            if (mpWinImage->IsSizedToWindow())
                mpWinImage->FitImageToWindow();

            Invalidate();
            return true;
        }
        else if (sType == "flipV")
        {
            mpWinImage->mpImage->Rotate(ZBuffer::kVFlip);
            if (mpWinImage->IsSizedToWindow())
                mpWinImage->FitImageToWindow();

            Invalidate();
            return true;
        }
   }

    return ZWin::HandleMessage(message);
}

void ImageViewer::HandleDeleteCommand()
{
    if (!ValidIndex(mnViewingIndex))
    {
        ZERROR("No image selected for delete");
        return;
    }

    int64_t nPrevViewingIndex = mnViewingIndex;

    DeleteFile(mImageArray[mnViewingIndex].filename);

    ScanForImagesInFolder(mCurrentFolder);

    mnViewingIndex = nPrevViewingIndex; // restore
    KickCaching();
    limit<int64_t>(mnViewingIndex, 0, mImageArray.size()-1);
}

void ImageViewer::SetArea(const ZRect& newArea)
{
    mbVisible = false;
    ZWin::SetArea(newArea);

    if (mpPanel)
    {
        int64_t nControlPanelSide = mAreaToDrawTo.Height() / 16;
        limit<int64_t>(nControlPanelSide, 20, 64);

        ZRect rPanelArea(mAreaToDrawTo.left, mAreaToDrawTo.top, mAreaToDrawTo.right, mAreaToDrawTo.top + nControlPanelSide);
        mpPanel->mrTrigger = rPanelArea;
        mpPanel->SetArea(rPanelArea);
    }

    mbVisible = true;
}
void ImageViewer::HandleNavigateToParentFolder()
{
    ScanForImagesInFolder(mCurrentFolder.parent_path());
    SetFirstImage();
}

void ImageViewer::HandleMoveCommand()
{
    if (!ValidIndex(mnViewingIndex) || CountImagesMatchingFilter(mFilterState) == 0)
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
        MoveSelectedFile(filesystem::path(sFolder));
        mMoveToFolder = sFolder;
        RemoveImageArrayEntry(mnViewingIndex);
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
    if (!ValidIndex(mnViewingIndex) || CountImagesMatchingFilter(mFilterState) == 0)
    {
        ZERROR("No image selected to move");
        return;
    }

    filesystem::path oldName = PathFromIndex(mnViewingIndex);
    int64_t oldIndex = mnViewingIndex;

    if (!std::filesystem::exists(oldName))
    {
        ZERROR("No image selected to move");
        return;
    }

    filesystem::create_directories(newPath);

    filesystem::path newName(newPath);
    newName.append(oldName.filename().string());

    try
    {
        filesystem::rename(oldName, newName);
        mImageArray[mnViewingIndex].filename = newName;
    }
    catch (std::filesystem::filesystem_error err)
    {
        ZERROR("Error renaming from \"", oldName, "\" to \"", newName, "\"\ncode:", err.code(), " error:", err.what(), "\n");
    };


//    ScanForImagesInFolder(mCurrentFolder);

//    mnViewingIndex = oldIndex;
//    limit<int64_t>(mnViewingIndex, 0, mImageArray.size() - 1);

    Invalidate();
}


void ImageViewer::DeleteConfimed()
{
    tImageFilenames deletionList = GetImagesFlaggedToBeDeleted();
    for (auto& f : deletionList)
    {
        DeleteFile(f);
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
    mnViewingIndex = 0;
    if (!ImageMatchesCurFilter(mnViewingIndex))
    {
        if (!FindNextImageMatchingFilter(mnViewingIndex))
        {
            UpdateFilteredView(mFilterState);
        }
    };


    mLastAction = kBeginning;
    limit<int64_t>(mnViewingIndex, 0, mImageArray.size() - 1);
    mCachingState = kReadingAhead;
    FreeCacheMemory();
    KickCaching();
    InvalidateChildren();
}
 

void ImageViewer::SetLastImage()
{
    mnViewingIndex = mImageArray.size() - 1;
    if (ValidIndex(mnViewingIndex) && !ImageMatchesCurFilter(mnViewingIndex))
    {
        if (!FindPrevImageMatchingFilter(mnViewingIndex))
        {
            UpdateFilteredView(mFilterState);
        }
    };

    mLastAction = kEnd;
    limit<int64_t>(mnViewingIndex, 0, mImageArray.size() - 1);
    mCachingState = kReadingAhead;
    FreeCacheMemory();
    KickCaching();
    InvalidateChildren();
}

void ImageViewer::SetPrevImage()
{
    if (!FindPrevImageMatchingFilter(mnViewingIndex))
    {
        UpdateFilteredView(mFilterState);
    }

    mLastAction = kPrevImage;
    limit<int64_t>(mnViewingIndex, 0, mImageArray.size() - 1);
    mCachingState = kReadingAhead;
    FreeCacheMemory();
    KickCaching();
    InvalidateChildren();
}

void ImageViewer::SetNextImage()
{
    if (!FindNextImageMatchingFilter(mnViewingIndex))
    {
        UpdateFilteredView(mFilterState);
    }

    mLastAction = kNextImage;
    limit<int64_t>(mnViewingIndex, 0, mImageArray.size() - 1);
    mCachingState = kReadingAhead;
    FreeCacheMemory();
    KickCaching();
    InvalidateChildren();
}
bool ImageViewer::FindNextImageMatchingFilter(int64_t& nIndex)
{
    int64_t nSearch = mnViewingIndex;
    do
    {
        nSearch++;
    } while (ValidIndex(nSearch) && !ImageMatchesCurFilter(nSearch));

    if (ValidIndex(nSearch))
    {
        nIndex = nSearch;
        return true;
    }

    return false;
}

bool ImageViewer::FindPrevImageMatchingFilter(int64_t& nIndex)
{
    int64_t nSearch = mnViewingIndex;
    do
    {
        nSearch--;
    } while (ValidIndex(nSearch) && !ImageMatchesCurFilter(nSearch));

    if (ValidIndex(nSearch))
    {
        nIndex = nSearch;
        return true;
    }

    return false;
}


bool ImageViewer::ImageMatchesCurFilter(int64_t nIndex)
{
    if (mFilterState == kAll)
        return true;

    if (!ValidIndex(nIndex))
        return false;

    if (mFilterState == kToBeDeleted)
        return mImageArray[nIndex].ToBeDeleted();

    assert(mFilterState == kFavs);
    return mImageArray[nIndex].IsFavorite();
}

int64_t ImageViewer::CountImagesMatchingFilter(eFilterState state)
{
    if (state == kAll)
        return mImageArray.size();
       
    int64_t nToBeDeleted = 0;
    int64_t nFavorites = 0;
    for (auto& i : mImageArray)
    {
        if (i.ToBeDeleted())
            nToBeDeleted++;
        else if (i.IsFavorite())
            nFavorites++;
    }

    if (state == kToBeDeleted)
        return nToBeDeleted;

    // else viewing favorites
    return nFavorites;
}



bool ImageViewer::Init()
{
    if (!mbInitted)
    {
        SetFocus();
        mpWinImage = new ZWinImage();
        ZRect rImageArea(mAreaToDrawTo);
//        rImageArea.left += 64;  // thumbs
        mpWinImage->SetArea(rImageArea);
        mpWinImage->mFillColor = 0xff000000;
        mpWinImage->mZoomHotkey = VK_MENU;
        mpWinImage->mBehavior |= ZWinImage::kHotkeyZoom|ZWinImage::kScrollable;

//        mpWinImage->mCaptionMap["zoom"].style.paddingV = gStyleCaption.fp.nHeight;

        mpWinImage->mpTable = new ZGUI::ZTable();
        ChildAdd(mpWinImage);
        mpWinImage->SetFocus();


        string sPersistedViewPath;
        if (gRegistry.Get("ZImageViewer", "image", sPersistedViewPath))
        {
            ScanForImagesInFolder(filesystem::path(sPersistedViewPath).parent_path());
            mnViewingIndex = IndexFromPath(sPersistedViewPath);
            limit<int64_t>(mnViewingIndex, 0, mImageArray.size() - 1);
            mCachingState = kReadingAhead;
            KickCaching();
        }

        ResetControlPanel();

        mToggleUIHotkey = VK_TAB;
        UpdateFilteredView(kAll);
    }

    if (!ValidIndex(mnViewingIndex))
        SetFirstImage();

    return ZWin::Init();
}


void ImageViewer::ResetControlPanel()
{
    int64_t nControlPanelSide = mAreaToDrawTo.Height() / 24;
    limit<int64_t>(nControlPanelSide, 64, 128);


    bool bShow = false;
    gRegistry.Get("ZImageViewer","showui", bShow);
    if (mpPanel)
    {
        // if the panel is already the correct size, don't recreate it
//        if (mpPanel->GetArea().Width() == mAreaToDrawTo.Width() && mpPanel->GetArea().Height() == nControlPanelSide)
//            return;

        bShow = mpPanel->IsVisible();
        ChildDelete(mpPanel);
        mpPanel = nullptr;
    }



    mpPanel = new ZWinControlPanel();
        
    int64_t nGroupSide = nControlPanelSide - gnDefaultGroupInlaySize * 2;



    ZGUI::Style unicodeStyle = ZGUI::Style(ZFontParams("Arial", nGroupSide * 3 /4, 200, 0, 0, false, true), ZGUI::ZTextLook{}, ZGUI::C, 0);
    ZGUI::Style wingdingsStyle = ZGUI::Style(ZFontParams("Wingdings", nGroupSide /2, 200, 0, 0, false, true), ZGUI::ZTextLook{}, ZGUI::C);

    mpSymbolicFont = gpFontSystem->CreateFont(unicodeStyle.fp);
    ((ZDynamicFont*) mpSymbolicFont.get())->GenerateSymbolicGlyph('<', 11119); // rotate left
    ((ZDynamicFont*)mpSymbolicFont.get())->GenerateSymbolicGlyph('>', 11118); // rotate right

    ((ZDynamicFont*)mpSymbolicFont.get())->GenerateSymbolicGlyph('-', 11108); // flip H
    ((ZDynamicFont*)mpSymbolicFont.get())->GenerateSymbolicGlyph('|', 11109); // flip V

    ((ZDynamicFont*)mpSymbolicFont.get())->GenerateSymbolicGlyph('F', 0x2750);
    ((ZDynamicFont*)mpSymbolicFont.get())->GenerateSymbolicGlyph('Q', 0x0F1C);  // quality rendering

    ZGUI::Style favorites = ZGUI::Style(ZFontParams("Arial", nGroupSide*4, 400, 0, 0, false, true), ZGUI::ZTextLook{}, ZGUI::C, 0);
    mpFavoritesFont = gpFontSystem->CreateFont(favorites.fp);
    ((ZDynamicFont*)mpFavoritesFont.get())->GenerateSymbolicGlyph('C', 0x2655);  // crown



//    ((ZDynamicFont*)wingdingsStyle.Font().get())->GenerateSymbolicGlyph('F', 0x2922);



    ZRect rPanelArea(mAreaToDrawTo.left, mAreaToDrawTo.top, mAreaToDrawTo.right, mAreaToDrawTo.top + nControlPanelSide);
    mpPanel->mbHideOnMouseExit = true; // if UI is toggled on, then don't hide panel on mouse out
    mpPanel->SetArea(rPanelArea);
    mpPanel->mrTrigger = rPanelArea;
    ChildAdd(mpPanel, bShow);
    mpPanel->mbHideOnMouseExit = !bShow; 

    ZWinSizablePushBtn* pBtn;

    ZRect rGroup(gnDefaultGroupInlaySize, gnDefaultGroupInlaySize, -1, nControlPanelSide - gnDefaultGroupInlaySize);  // start the first grouping

    ZRect rButton(rGroup.left + gnDefaultGroupInlaySize, rGroup.top + gnDefaultGroupInlaySize, rGroup.left + nGroupSide, rGroup.bottom - gnDefaultGroupInlaySize / 2);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("X");
    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("quit", this));
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width() + gnDefaultGroupInlaySize, 0);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("1");  // wingdings open folder
    pBtn->SetArea(rButton);
    pBtn->mStyle = wingdingsStyle;
    pBtn->SetMessage(ZMessage("loadimg", this));
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width(), 0);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("<"); // wingdings save
    pBtn->mStyle = wingdingsStyle;
    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("saveimg", this));
    mpPanel->ChildAdd(pBtn);

    rGroup.right = rButton.right + gnDefaultGroupInlaySize;
    mpPanel->AddGrouping("File", rGroup);

    string sMessage;


    // Management
    rButton.OffsetRect(rButton.Width() + gnDefaultGroupInlaySize * 4, 0);
    rButton.right = rButton.left + rButton.Width() * 2.5;     // wider buttons for management

    rGroup.left = rButton.left - gnDefaultGroupInlaySize;


    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("Move");
    pBtn->mStyle = gDefaultGroupingStyle;
    //        pBtn->mStyle.look.colTop = 0xffff0000;
    //        pBtn->mStyle.look.colBottom = 0xffff0000;
    pBtn->mStyle.fp.nHeight = nGroupSide / 2;
    pBtn->mStyle.pos = ZGUI::C;
    pBtn->SetArea(rButton);
    Sprintf(sMessage, "set_move_folder;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);



    rButton.OffsetRect(rButton.Width(), 0);
    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("Delete");
    pBtn->mStyle = gDefaultGroupingStyle;
    pBtn->mStyle.look.colTop = 0xffff0000;
    pBtn->mStyle.look.colBottom = 0xffff0000;
    pBtn->mStyle.fp.nHeight = nGroupSide / 2;
    pBtn->mStyle.pos = ZGUI::C;
    pBtn->SetArea(rButton);
    Sprintf(sMessage, "show_confirm;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);




    rGroup.right = rButton.right + gnDefaultGroupInlaySize;
    mpPanel->AddGrouping("Manage", rGroup);





    // Rotation Group
    rButton.OffsetRect(rButton.Width() + gnDefaultGroupInlaySize * 8, 0);
    rGroup.left = rButton.left - gnDefaultGroupInlaySize;   // start new grouping

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("<"); // unicode rotate left
    pBtn->mStyle = unicodeStyle;
    pBtn->SetArea(rButton);
    Sprintf(sMessage, "rotate_left;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width(), 0);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption(">"); // unicode rotate right
    pBtn->mStyle = unicodeStyle;
    pBtn->SetArea(rButton);
    Sprintf(sMessage, "rotate_right;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width(), 0);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("-"); // unicode flip H
    pBtn->mStyle = unicodeStyle;
    pBtn->SetArea(rButton);
    Sprintf(sMessage, "flipH;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width(), 0);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("|"); // unicode flip V
    pBtn->mStyle = unicodeStyle;
    pBtn->SetArea(rButton);
    Sprintf(sMessage, "flipV;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);



    rGroup.right = rButton.right + gnDefaultGroupInlaySize;
    mpPanel->AddGrouping("Rotate", rGroup);







    // Filter group
    rButton.OffsetRect(rButton.Width() + gnDefaultGroupInlaySize * 4, 0);
    rButton.right = rButton.left + rButton.Width();     // wider buttons for management

    rGroup.left = rButton.left - gnDefaultGroupInlaySize;

    ZGUI::Style filterButtonStyle = gDefaultGroupingStyle;
    filterButtonStyle.look.decoration = ZGUI::ZTextLook::kEmbossed;
    filterButtonStyle.fp.nHeight = nGroupSide / 3;
    filterButtonStyle.pos = ZGUI::C;
    filterButtonStyle.look.colTop = 0xff888888;
    filterButtonStyle.look.colBottom = 0xff888888;


    // All
    ZWinCheck* pCheck = new ZWinCheck();
    pCheck->SetState(true, false);
    pCheck->SetMessages(ZMessage("filter_all", this), "");
    pCheck->SetRadioGroup("filter_group");
    pCheck->SetCaption("all");
    pCheck->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);

    pCheck->mCheckedStyle = filterButtonStyle;
    pCheck->mCheckedStyle.look.colTop = 0xffffffff;
    pCheck->mCheckedStyle.look.colBottom = 0xffffffff;

    pCheck->mUncheckedStyle = filterButtonStyle;

    pCheck->SetArea(rButton);
    pCheck->SetTooltip("All images");
    mpPanel->ChildAdd(pCheck);
    mpAllFilterButton = pCheck;

    // ToBeDeleted
    rButton.OffsetRect(rButton.Width(), 0);
    pCheck = new ZWinCheck();
    pCheck->SetMessages(ZMessage("filter_del", this), "");
    pCheck->SetRadioGroup("filter_group");
    pCheck->SetCaption("del");
    pCheck->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);

    pCheck->mCheckedStyle = filterButtonStyle;
    pCheck->mCheckedStyle.look.colTop = 0xffff4444;
    pCheck->mCheckedStyle.look.colBottom = 0xffff4444;

    pCheck->mUncheckedStyle = filterButtonStyle;

    pCheck->SetArea(rButton);
    pCheck->SetTooltip("Images flagged for deletion");
    mpPanel->ChildAdd(pCheck);
    mpDelFilterButton = pCheck;

    // Favorites
    rButton.OffsetRect(rButton.Width(), 0);

    pCheck = new ZWinCheck();
    pCheck->SetMessages(ZMessage("filter_favs", this), "");
    pCheck->SetRadioGroup("filter_group");
    pCheck->SetCaption("favs");
    pCheck->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);

    pCheck->mCheckedStyle = filterButtonStyle;
    pCheck->mCheckedStyle.look.colTop = 0xffe1b131;
    pCheck->mCheckedStyle.look.colBottom = 0xffe1b131;

    pCheck->mUncheckedStyle = filterButtonStyle;

    pCheck->SetArea(rButton);
    pCheck->SetTooltip("Favorites");
    mpPanel->ChildAdd(pCheck);
    mpFavsFilterButton = pCheck;
  






    rGroup.right = rButton.right + gnDefaultGroupInlaySize;
    mpPanel->AddGrouping("Filter", rGroup);























    rButton = ZGUI::Arrange(rButton, rPanelArea, ZGUI::RC, gnDefaultGroupInlaySize);

    rGroup.right = rButton.right;
    rButton.OffsetRect(-gnDefaultGroupInlaySize, 0);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("F");
    pBtn->mStyle = unicodeStyle;
    pBtn->mStyle.pos = ZGUI::C;
    pBtn->mStyle.paddingV = -pBtn->mStyle.fp.nHeight/10;
    pBtn->SetArea(rButton);
    Sprintf(sMessage, "toggle_fullscreen");
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(-rButton.Width(), 0);
    pCheck = new ZWinCheck(&mbSubsample);
    pCheck->SetMessages(ZMessage("invalidate", this), ZMessage("invalidate", this));
    pCheck->SetCaption("Q");
    pCheck->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);

    pCheck->mCheckedStyle = unicodeStyle;
    pCheck->mCheckedStyle.look.decoration = ZGUI::ZTextLook::kEmbossed;
    pCheck->mCheckedStyle.look.colTop = 0xff88ff88;
    pCheck->mCheckedStyle.look.colBottom = 0xff88ff88;
    pCheck->mCheckedStyle.pos = ZGUI::CB;
    pCheck->mCheckedStyle.paddingV = unicodeStyle.fp.nHeight / 10;

    pCheck->mUncheckedStyle = unicodeStyle;
    pCheck->mUncheckedStyle.look.decoration = ZGUI::ZTextLook::kEmbossed;
    pCheck->mUncheckedStyle.pos = ZGUI::CB;
    pCheck->mUncheckedStyle.paddingV = unicodeStyle.fp.nHeight / 10;

    pCheck->SetArea(rButton);
    pCheck->SetTooltip("Sub-pixel sampling");
    mpPanel->ChildAdd(pCheck);



    rGroup.left = rButton.left- gnDefaultGroupInlaySize;
    mpPanel->AddGrouping("view", rGroup);
}

bool ImageViewer::OnParentAreaChange()
{
    if (!mpTransformTexture)
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpTransformTexture.get()->GetMutex());
    SetArea(mpParentWin->GetArea());

    ResetControlPanel();
    ZWin::OnParentAreaChange();
    UpdateCaptions();

//    static int count = 0;
//    ZOUT("ImageViewer::OnParentAreaChange\n", count++);

    return true;
}


bool ImageViewer::ViewImage(const std::filesystem::path& filename)
{
    if (mCurrentFolder != filename.parent_path())
        ScanForImagesInFolder(filename.parent_path());

    mnViewingIndex = IndexFromPath(filename);
    limit<int64_t>(mnViewingIndex, 0, mImageArray.size());
    return true;
}

void ImageViewer::LoadExifProc(std::filesystem::path& imagePath, ImageEntry* pEntry)
{
    if (!pEntry)
        return;

    ZOUT("Loading EXIF:", imagePath, "\n");

    if (ZBuffer::ReadEXIFFromFile(imagePath.string(), pEntry->mEXIF))
        pEntry->mState = ImageEntry::kExifReady;
    else
        pEntry->mState = ImageEntry::kNoExifAvailable;
}

void ImageViewer::FlushLoads()
{
    delete mpPool;  // will join
    mpPool = new ThreadPool(kPoolSize);
}



void ImageViewer::LoadImageProc(std::filesystem::path& imagePath, ImageEntry* pEntry)
{
    if (!pEntry)
        return;

    ZOUT("Loading:", imagePath, "\n");

    tZBufferPtr pNewImage(new ZBuffer);
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
    if (!ValidIndex(mnViewingIndex))    // also handles empty array case
        return nullptr;

    if (!ImageMatchesCurFilter(mnViewingIndex))
        return nullptr;

    return mImageArray[mnViewingIndex].pImage;
}

int64_t ImageViewer::CurMemoryUsage()
{
    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);

    int64_t nUsage = 0;
    for (auto& i : mImageArray)
    {
        if (i.pImage)
        {
            ZRect rArea = i.pImage->GetArea();
            nUsage += rArea.Width() * rArea.Height() * 4;
        }
        else if (i.mState == ImageEntry::kLoadInProgress)
        {
            int64_t nBytesPending = (i.mEXIF.ImageWidth * i.mEXIF.ImageHeight) * 4;
            nUsage += nBytesPending;
        }
    }

    return nUsage;
}

int64_t ImageViewer::GetLoadsInProgress()
{
    int64_t nCount = 0;
    for (auto& i : mImageArray)
    {
        if (i.mState == ImageEntry::kLoadInProgress)
            nCount++;
    }

    return nCount;
}


// check ahead in the direction of last action....two forward, one back
int64_t ImageViewer::NextImageToCache()
{
    int64_t dir = 1;
    if (mLastAction == kPrevImage || mnViewingIndex == mImageArray.size()-1)
        dir = -1;
    

    for (int64_t distance = 0; distance < mMaxCacheReadAhead; distance++)
    {
        int64_t index = (int64_t)mnViewingIndex + distance * dir;
        if (ValidIndex(index) && ImageMatchesCurFilter(index))
        {
            ImageEntry& entry = mImageArray[index];
            if (entry.mState < ImageEntry::kLoadInProgress)
                return index;
        }

        index = (int64_t)mnViewingIndex + distance * -dir;
        if (ValidIndex(index) && ImageMatchesCurFilter(index))
        {
            ImageEntry& entry = mImageArray[index];
            if (entry.mState < ImageEntry::kLoadInProgress)
                return index;
        }
    }

    return -1;
}

bool ImageViewer::FreeCacheMemory()
{

    const int64_t kUnloadViewDistance = mMaxCacheReadAhead;
    // unload everything over mMaxCacheReadAhead
    for (int64_t i = 0; i < mImageArray.size(); i++)
    {
        if (std::abs(i - mnViewingIndex) > kUnloadViewDistance && mImageArray[i].mState == ImageEntry::kLoaded)
        {
            ZOUT("Unloading viewed:", i, "\n");
            mImageArray[i].pImage = nullptr;
            if (mImageArray[i].mEXIF.ImageWidth > 0 && mImageArray[i].mEXIF.ImageHeight > 0)
                mImageArray[i].mState = ImageEntry::kExifReady;
            else
                mImageArray[i].mState = ImageEntry::kInit;
        }
    }

    // progressively look for closer images to unload until under budget
    for (int64_t nDistance = kUnloadViewDistance-1; nDistance > 1 && CurMemoryUsage() > mMaxMemoryUsage; nDistance--)
    {
        // make memory available if over budget
        for (int64_t i = 0; i < mImageArray.size() && CurMemoryUsage() > mMaxMemoryUsage; i++)
        {
            if (std::abs(i - mnViewingIndex) > nDistance && mImageArray[i].mState == ImageEntry::kLoaded)
            {
                ZOUT("Unloading viewed:", i, "\n");
                mImageArray[i].pImage = nullptr;
                if (mImageArray[i].mEXIF.ImageWidth > 0 && mImageArray[i].mEXIF.ImageHeight > 0)
                    mImageArray[i].mState = ImageEntry::kExifReady;
                else
                    mImageArray[i].mState = ImageEntry::kInit;
            }
        }
    }

    return true;
}



bool ImageViewer::KickCaching()
{
    const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);

    // loading current image is top priority
    if (ValidIndex(mnViewingIndex))
    {
        ImageEntry& entry = mImageArray[mnViewingIndex];
        if (entry.mState < ImageEntry::kLoadInProgress)
        {
            entry.mState = ImageEntry::kLoadInProgress;
            mpPool->enqueue(&ImageViewer::LoadImageProc, entry.filename, &entry);
        }
    }

    if (GetLoadsInProgress() > kPoolSize/2)
        return false;

    // kick off exif reading if necessary
    for (auto& entry : mImageArray)
    {
        if (entry.mState == ImageEntry::kInit)
        {
            entry.mState = ImageEntry::kLoadingExif;
            mpPool->enqueue(&ImageViewer::LoadExifProc, entry.filename, &entry);
            break;
        }
    }

    if (CurMemoryUsage() < mMaxMemoryUsage)
    {
        int64_t nextToCache = NextImageToCache();

        if (nextToCache < 0)
            return false;

        ImageEntry& entry = mImageArray[nextToCache];
        if (entry.mState < ImageEntry::kLoadInProgress)
        {
            ZOUT("caching image ", nextToCache, "\n");
            entry.mState = ImageEntry::kLoadInProgress;
            mpPool->enqueue(&ImageViewer::LoadImageProc, entry.filename, &entry);
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
    mnViewingIndex = 0;
    mLastAction = kNone;
    if (mpWinImage)
        mpWinImage->Clear();

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

    for (auto filePath : std::filesystem::directory_iterator(mCurrentFolder))
    {
        if (filePath.is_regular_file() && AcceptedExtension(filePath.path().extension().string()))
        {
            mImageArray.emplace_back(ImageEntry(filePath));
            //ZDEBUG_OUT("Found image:", filePath, "\n");
        }
    }

    filesystem::path favoritesPath = mCurrentFolder;
    favoritesPath.append(ksFavorites);
    if (filesystem::exists(favoritesPath))
    {
        for (auto filePath : std::filesystem::directory_iterator(favoritesPath))
        {
            if (filePath.is_regular_file() && AcceptedExtension(filePath.path().extension().string()))
            {
                mImageArray.emplace_back(ImageEntry(filePath));
                //ZDEBUG_OUT("Found image:", filePath, "\n");
            }
        }
    }

    filesystem::path toBeDeletedPath = mCurrentFolder;
    toBeDeletedPath.append(ksToBeDeleted);
    if (filesystem::exists(toBeDeletedPath))
    {
        for (auto filePath : std::filesystem::directory_iterator(toBeDeletedPath))
        {
            if (filePath.is_regular_file() && AcceptedExtension(filePath.path().extension().string()))
            {
                mImageArray.emplace_back(ImageEntry(filePath));
                //ZDEBUG_OUT("Found image:", filePath, "\n");
            }
        }
    }

    std::sort(mImageArray.begin(), mImageArray.end(), [](const ImageEntry& a, const ImageEntry& b) -> bool { return a.filename.filename().string() < b.filename.filename().string(); });


    UpdateCaptions();

    return true;
}

int64_t ImageViewer::IndexFromPath(std::filesystem::path imagePath)
{
    for (int i = 0; i < mImageArray.size(); i++)
    {
        if (mImageArray[i].filename == imagePath)
            return i;
    }

    return -1;
}

std::filesystem::path ImageViewer::PathFromIndex(int64_t index)
{
    if (!ValidIndex(index))
        return {};

    return mImageArray[index].filename;
}


bool ImageViewer::ValidIndex(int64_t index)
{
    return index >= 0 && index < mImageArray.size();
}

tImageFilenames ImageViewer::GetImagesFlaggedToBeDeleted()
{
    tImageFilenames filenames;

    for (auto& i : mImageArray)
    {
        if (i.ToBeDeleted())
            filenames.push_back(i.filename);
    }

    return filenames;
}

#ifdef _DEBUG

#ifdef DEBUG_CACHE

string ImageViewer::LoadStateString()
{
    string state;
    for (int i = 0; i < mImageArray.size(); i++)
    {
        if (mImageArray[i].pImage)
            state = state + " [" + SH::FromInt(i) + "]";
        else
            state = state + " " + SH::FromInt(i);
    }

    return state;
}
#endif

#endif

bool ImageViewer::Process()
{
    if (mpWinImage && !mImageArray.empty())
    {
        tZBufferPtr curImage = GetCurImage();

        if (curImage && mpWinImage->mpImage != curImage)
        {
            mpWinImage->SetImage(curImage);
            ZOUT("Setting image:", curImage->GetEXIF().DateTime, "\n");

            mpWinImage->mCaptionMap["no_image"].Clear();

            gRegistry["ZImageViewer"]["image"] = mImageArray[mnViewingIndex].filename.string();

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
    mFilterState = state;
    if (CountImagesMatchingFilter(mFilterState) == 0)
    {
        mpWinImage->Clear();
        if (mpPanel && !mpPanel->IsVisible())   // if no images and the UI is hidden, bring it up
            ToggleUI();
    }
    else
    {
        mpWinImage->mCaptionMap["no_image"].Clear();
        if (!ImageMatchesCurFilter(mnViewingIndex))
            SetFirstImage();
    }

    UpdateCaptions();
    InvalidateChildren();
}

void ImageViewer::UpdateCaptions()
{
    bool bShow = false;
    gRegistry.Get("ZImageViewer", "showui", bShow);

    mpWinImage->mCaptionMap["favorite"].visible = false;
    mpWinImage->mCaptionMap["for_delete"].visible = false;
    mpWinImage->mCaptionMap["no_image"].visible = false;
    mpWinImage->mCaptionMap["image_count"].visible = false;


    // count up favorites and to be deleted
    int64_t nToBeDeleted = 0;
    int64_t nFavorites = 0;
    for (auto& i : mImageArray)
    {
        if (i.ToBeDeleted())
            nToBeDeleted++;
        else if (i.IsFavorite())
            nFavorites++;
    }

    string sCaption;
    Sprintf(sCaption, "%d favs", nFavorites);
    if (mpFavsFilterButton)
        mpFavsFilterButton->SetCaption(sCaption);

    Sprintf(sCaption, "%d del", nToBeDeleted);
    if (mpDelFilterButton)
        mpDelFilterButton->SetCaption(sCaption);

    Sprintf(sCaption, "%d images", mImageArray.size());
    if (mpAllFilterButton)
        mpAllFilterButton->SetCaption(sCaption);


    if (mpWinImage)
    {
        int64_t topPadding = 0; // in case panel is visible
        if (mpPanel)
            topPadding = mpPanel->GetArea().Height();


        ZGUI::Style folderStyle(gStyleButton);
        folderStyle.pos = ZGUI::LT;
        folderStyle.look = ZGUI::ZTextLook::kShadowed;


        if (CountImagesMatchingFilter(mFilterState) == 0)
        {
            mpWinImage->mCaptionMap["no_image"].sText = "No images\nPress 'O' or TAB";
            mpWinImage->mCaptionMap["no_image"].style = gStyleCaption;
            mpWinImage->mCaptionMap["no_image"].visible = true;
        }
        else
        {
            if (bShow)
            {
                mpWinImage->mpTable->mCellStyle = gStyleCaption;
                mpWinImage->mpTable->mCellStyle.pos = ZGUI::LC;
                mpWinImage->mpTable->mCellStyle.paddingH = gDefaultSpacer / 2;
                mpWinImage->mpTable->mCellStyle.paddingV = gDefaultSpacer / 2;

                mpWinImage->mpTable->mTableStyle.pos = ZGUI::RB;
                mpWinImage->mpTable->mTableStyle.bgCol = 0x88000000;
                mpWinImage->mpTable->mTableStyle.paddingH = gDefaultSpacer;
                mpWinImage->mpTable->mTableStyle.paddingV = gDefaultSpacer;

                string sImageCount = "Viewing: [" + SH::FromInt(mnViewingIndex + 1) + "/" + SH::FromInt(mImageArray.size()) + "]";
                mpWinImage->mCaptionMap["image_count"].sText = sImageCount;
                mpWinImage->mCaptionMap["image_count"].style = folderStyle;
                mpWinImage->mCaptionMap["image_count"].style.paddingV = topPadding + folderStyle.fp.nHeight;
                mpWinImage->mCaptionMap["image_count"].style.pos = ZGUI::LT;
                mpWinImage->mCaptionMap["image_count"].visible = true;

                string sFilename = PathFromIndex(mnViewingIndex).filename().string();
                ZGUI::Style filenameStyle(gStyleButton);
                filenameStyle.pos = ZGUI::CB;
                filenameStyle.paddingV = folderStyle.fp.nHeight;
                filenameStyle.look = ZGUI::ZTextLook::kShadowed;
                mpWinImage->mCaptionMap["filename"].sText = sFilename;
                mpWinImage->mCaptionMap["filename"].style = filenameStyle;
                mpWinImage->mCaptionMap["filename"].visible = true;

                if (mFilterState == kAll)
                {
                    if (ValidIndex(mnViewingIndex) && mImageArray[mnViewingIndex].IsFavorite() && mpFavoritesFont)
                    {
                        mpWinImage->mCaptionMap["favorite"].sText = "C";
                        mpWinImage->mCaptionMap["favorite"].style = ZGUI::Style(mpFavoritesFont->GetFontParams(), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xffe1b131, 0xffe1b131), ZGUI::RT, filenameStyle.fp.nHeight * 2, filenameStyle.fp.nHeight * 2, 0x88000000, true);
                        mpWinImage->mCaptionMap["favorite"].visible = true;
                    }
                }

                if (ValidIndex(mnViewingIndex) && mImageArray[mnViewingIndex].ToBeDeleted())
                {
                    mpWinImage->mCaptionMap["for_delete"].sText = /*mImageArray[mnViewingIndex].filename.filename().string() +*/ "\nMARKED FOR DELETE";
                    mpWinImage->mCaptionMap["for_delete"].style = ZGUI::Style(ZFontParams("Ariel Bold", 100, 400), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xffff0000, 0xffff0000), ZGUI::CB, 0, 100, 0x88000000, true);
                    mpWinImage->mCaptionMap["for_delete"].visible = true;
                    bShow = true;
                }
            }


        }


        mpWinImage->mCaptionMap["folder"].sText = mCurrentFolder.string();
        mpWinImage->mCaptionMap["folder"].style = folderStyle;
        mpWinImage->mCaptionMap["folder"].style.paddingV = topPadding;
        mpWinImage->mCaptionMap["folder"].visible = bShow;



        if (!mMoveToFolder.empty())
        {
            Sprintf(mpWinImage->mCaptionMap["move_to_folder"].sText, "'M'ove destination: %s", mMoveToFolder.string().c_str());
            mpWinImage->mCaptionMap["move_to_folder"].style = ZGUI::Style(ZFontParams("Ariel Bold", 28, 400), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xff0088ff, 0xff0088ff), ZGUI::RT, 32, topPadding+32, 0x88000000, true);
            mpWinImage->mCaptionMap["move_to_folder"].visible = bShow;
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
            style.fp.nHeight *= 1.2;

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
    if (!ValidIndex(mnViewingIndex))
        return;

    filesystem::path toBeDeleted = mCurrentFolder;
    toBeDeleted.append(ksToBeDeleted);

    if (mImageArray[mnViewingIndex].ToBeDeleted())
    {
        // move from subfolder up
        MoveSelectedFile(mCurrentFolder);

        if (std::filesystem::is_empty(toBeDeleted))
            std::filesystem::remove(toBeDeleted);
    }
    else
    {
        // move into to be deleted subfolder
        MoveSelectedFile(toBeDeleted);
    }

    UpdateFilteredView(mFilterState);
    Invalidate();
}

void ImageViewer::ToggleFavorite()
{
    if (!ValidIndex(mnViewingIndex))
        return;

    filesystem::path favorites = mCurrentFolder;
    favorites.append(ksFavorites);

    if (mImageArray[mnViewingIndex].IsFavorite())
    {
        // move from subfolder up
        MoveSelectedFile(mCurrentFolder);

        if (std::filesystem::is_empty(favorites))
            std::filesystem::remove(favorites);
    }
    else
    {
        // move into favorites subfolder
        MoveSelectedFile(favorites);
    }

    UpdateFilteredView(mFilterState);
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
   
