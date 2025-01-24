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
#include "ZWinFolderSelector.h"
#include "ZWinText.H"
#include "ZGUIStyle.h"
#include <algorithm>
#include "ZXMLNode.h"
#include "ZWinBtn.H"
#include "ZWinPanel.h"
#include "helpers/Registry.h"
#include "ImageContest.h"
#include "ZTimer.h"
#include "ImageMeta.h"
#include "WinTopWinners.h"
#include "ZGraphicSystem.h"
#include "ZScreenBuffer.h"
#include "ZAnimator.h"


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
    //mpRotationMenu = nullptr;
    //mpManageMenu = nullptr;
    //mpSymbolicFont = nullptr;
    //mpFavoritesFont = nullptr;
    mpWinImage = nullptr;
    mpImageLoaderPool = nullptr;
    mpMetadataLoaderPool = nullptr;
    mToggleUIHotkey = 0;
    mCachingState = kWaiting;
    mbSubsample = true;
    mbShowUI = true;
    //mbShowFavOrDelState = true;
    mFilterState = kAll;
/*    mpAllFilterButton = nullptr;
    mpFavsFilterButton = nullptr;
    mpDelFilterButton = nullptr;
    mpRankedFilterButton = nullptr;
    mpDeleteMarkedButton = nullptr;
    mpShowContestButton = nullptr;*/
    mOutstandingMetadataCount = 0;
    mpRatedImagesStrip = nullptr;
    mpFolderLabel = nullptr;

    assert(gGraphicSystem.GetScreenBuffer());
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
        mpWinImage->FitImageToWindow();
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

    gMessageSystem.Post(ZMessage("quit_app_confirmed"));
}

void ImageViewer::UpdateUI()
{
    //ZDEBUG_OUT("UpdateUI() mbShowUI:", mbShowUI, "\n");

    if (!mRankedImageMetadata.empty())
    {
        if (mOutstandingMetadataCount == -1)
        {
            if (!mpRatedImagesStrip)
            {
                mpRatedImagesStrip = new WinTopWinners();
                ChildAdd(mpRatedImagesStrip);
            }

            mpRatedImagesStrip->pMetaList = &mRankedImageMetadata;
        }
    }

    ZRect rImageArea(mAreaLocal);

    if (mpPanel && mbShowUI)
        rImageArea.top = mpPanel->GetArea().Height();

    if (mpFolderLabel)
    {
        mpFolderLabel->SetVisible(mbShowUI && !mCurrentFolder.empty());
    }

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

    if (mpFolderLabel)
    {
        ZRect rSelector(mpFolderLabel->GetArea());
        rSelector.MoveRect(0, mpPanel->GetArea().bottom);
        mpFolderLabel->SetArea(rSelector);
    }

//    gGraphicSystem.GetScreenBuffer()->EnableRendering(false);
    if (mpWinImage && mbInitted)
        mpWinImage->SetArea(rImageArea);
//    gGraphicSystem.GetScreenBuffer()->EnableRendering(true);

    UpdateControlPanel();
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
        // If a image selection panel is up, escape should just close it
        ZWin* pTop = GetTopWindow();
        if (pTop)
        {
            ZWinPanel* pWinPanel = (ZWinPanel*)pTop->GetChildWindowByWinName("image_selection_panel");
            if (pWinPanel)
            {
                gMessageSystem.Post(ZMessage("clear_selection", mpWinImage));
                pTop->ChildDelete(pWinPanel);
                return true;
            }
        }

        HandleQuitCommand();
        return true;
    }
#endif

    bool bShow = true;
    gRegistry.Get("ZImageViewer", "showui", bShow);
    if (key == mToggleUIHotkey)
    {
        mbShowUI = !mbShowUI;
        //mbShowFavOrDelState = mbShowUI;    // when UI gets hidden, also hide this indicator
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
    case 'p':
    case 'P':
        gMessageSystem.Post(ZMessage("show_app_palette"));
        return true;
    case 'r':
    case 'R':
        if (gInput.IsKeyDown(VK_CONTROL))
        {
            LaunchCR3(FindCR3());
            return true;
        }
        break;
    case 'c':
    case 'C':
        if (gInput.IsKeyDown(VK_CONTROL))
        {
            gMessageSystem.Post(ZMessage("copylink", this));
            return true;
        }
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
    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpSurface.get()->GetMutex());

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
        if (mpWinImage && mpWinImage->mpImage)
        {
            string sFilename;
            if (ZWinFileDialog::ShowSaveDialog("Images", "*.jpg;*.jpeg;*.png;*.tga;*.bmp;*.hdr", sFilename))
                SaveImage(sFilename);
        }
        return true;
    }
    else if (sType == "opencr3")
    {
        LaunchCR3(FindCR3());
        return true;
    }
    else if (sType == "gotofolder")
    {
#ifdef _WIN32
        if (mpWinImage && mpWinImage->mpImage)
        {
            string sPath;
            Sprintf(sPath, "/select,%s", EntryFromIndex(mViewingIndex)->filename.string().c_str());
            ShellExecute(0, "open", "explorer", sPath.c_str(), 0, SW_SHOWNORMAL);
        }
        return true;
#endif
    }
    else if (sType == "copylink")
    {
        if (ValidIndex(mViewingIndex))
        {
            gInput.SetClipboard(EntryFromIndex(mViewingIndex)->filename.string());

            ShowTooltipMessage("Copied", 0x8800ff00);
        }

        return true;
    }
    else if (sType == "download")
    {
        string sURL;
        if (gRegistry.Get("app", "newurl", sURL))
        {
            ShellExecute(0, "open", "explorer", sURL.c_str(), 0, SW_SHOWNORMAL);
            HandleQuitCommand();
        }
        return true;
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
            mpWinImage->nSubsampling = 4;
        else
            mpWinImage->nSubsampling = 0;
        InvalidateChildren();
        return true;
    }
    else if (sType == "rank_favorites")
    {
        ImageContest* pWin = new ImageContest();
        pWin->SetArea(mAreaInParent);
        GetTopWindow()->ChildAdd(pWin);
        pWin->ScanFolder(FavoritesPath());
        SetVisible(false);
        return true;
    }
    else if (sType == "image_selection")
    {
        ZRect rSelection = StringToRect(message.GetParam("r"));
        ZOUT("Rect Selected:", message.GetParam("r"), "\n");
        ShowImageSelectionPanel(rSelection);

        return true;
    }
    else if (sType == "save_selection")
    {
        SaveSelection(StringToRect(message.GetParam("r")));
        return true;
    }
    else if (sType == "copy_selection")
    {
        CopySelection(StringToRect(message.GetParam("r")));
        return true;
    }
    else if (sType == "show_help")
    {
        ToggleShowHelpDialog();
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
/*        else if (sType == "show_rotation_menu")
        {
            if (mpRotationMenu)
            {
                if (mpRotationMenu->mbVisible)
                {
                    mpRotationMenu->SetVisible(false);
                }
                else
                {
                    mpRotationMenu->mStyle.pad.h = (int32_t)gSpacer;
                    mpRotationMenu->mStyle.pad.v = (int32_t)gSpacer;

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
        }*/
/*        else if (sType == "show_manage_menu")
        {
            if (mpManageMenu)
            {
                if (mpManageMenu->mbVisible)
                {
                    mpManageMenu->SetVisible(false);
                }
                else
                {
                    mpManageMenu->mStyle.pad.h = (int32_t)gSpacer;
                    mpManageMenu->mStyle.pad.v = (int32_t)gSpacer;

                    ZRect r = StringToRect(message.GetParam("r"));
                    r.InflateRect(gSpacer * 2, gSpacer * 2);
                    r.bottom = r.top + r.Height() * 4 + gM;

                    mpManageMenu->SetArea(r);
                    mpManageMenu->ArrangeWindows(mpManageMenu->GetChildrenInGroup("Manage"), ZRect(r.Width(), r.Height()), mpManageMenu->mStyle, 1, -1);
                    mpManageMenu->SetVisible();
                    ChildToFront(mpManageMenu);
                }
            }
            return true;
        }*/
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
   }

    return ZWin::HandleMessage(message);
}

void ImageViewer::ShowTooltipMessage(const string& msg, uint32_t col)
{
    ZGUI::Style messageStyle(gStyleCaption);
    messageStyle.bgCol = col;
    messageStyle.pad.h = (int32_t)gM;

    gInput.ShowTooltip(msg, messageStyle);
}

void ImageViewer::ShowImageSelectionPanel(ZRect rSelection)
{

    ZWin* pTop = GetTopWindow();
    assert(pTop);

    ZWinPanel* pWinPanel = (ZWinPanel*)pTop->GetChildWindowByWinName("image_selection_panel");
    if (pWinPanel)
    {
        pTop->ChildDelete(pWinPanel);
    }

    string sClearMsg = "{clear_selection;target=" + mpWinImage->GetTargetName() + "}";
    string sPanelLayout;
    sPanelLayout = "<panel close_on_button=1 border=1 spacers=1><row>";
    sPanelLayout += "<button id=save svgpath=$apppath$/res/save.svg msg=\"{save_selection;r=" + RectToString(rSelection) + ";target=" + GetTargetName() + "}" + sClearMsg + "\" tooltip=\"Save selection\"/>";
    sPanelLayout += "<button id=copy svgpath=$apppath$/res/copy.svg msg=\"{copy_selection;r=" + RectToString(rSelection) + ";target=" + GetTargetName() + "}" + sClearMsg + "\" tooltip=\"Copy selection to clipboard\"/>";
    sPanelLayout += "<button id=cancel svgpath=$apppath$/res/x.svg msg=" + sClearMsg + " tooltip=\"Cancel\"/>";
    sPanelLayout += "</row></panel>";

    pWinPanel = new ZWinPanel();
    pWinPanel->msWinName = "image_selection_panel";
    pWinPanel->mPanelLayout = sPanelLayout;
    ZRect rPanel(gM * 6, gM * 2);
    rPanel.OffsetRect(gInput.lastMouseMove);
    rPanel.OffsetRect(-rPanel.Width(), 0);
    pWinPanel->SetArea(rPanel);

    pTop->ChildAdd(pWinPanel);
}


void ImageViewer::CopySelection(ZRect rSelection)
{
    ZBuffer imageSelection;
    imageSelection.Init(rSelection.Width(), rSelection.Height());
    imageSelection.Blt(mpWinImage->mpImage.get(), rSelection, imageSelection.GetArea(), 0, ZBuffer::kAlphaSource);
    if (gInput.SetClipboard(&imageSelection))
        ShowTooltipMessage("Copied", 0x88008800);
}

void ImageViewer::SaveSelection(ZRect rSelection)
{
    string sFolder;
    gRegistry.Get("ZImageViewer", "selectionsave", sFolder);

    filesystem::path fileName(EntryFromIndex(mViewingIndex)->filename.filename());
    fileName.replace_extension(".png");
    string sFilename = fileName.string();

    if (ZWinFileDialog::ShowSaveDialog("Images", "*.png;*.jpg;*.jpeg;*.tga;*.bmp;*.hdr", sFilename, sFolder))
    {
        ZBuffer imageSelection;
        imageSelection.Init(rSelection.Width(), rSelection.Height());
        imageSelection.Blt(mpWinImage->mpImage.get(), rSelection, imageSelection.GetArea(), 0, ZBuffer::kAlphaSource);

//#define RESIZE
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
}

void ImageViewer::ToggleShowHelpDialog()
{
    ZWinDialog* pHelp = (ZWinDialog*)GetChildWindowByWinName("ImageViewerHelp");
    if (pHelp)
    {
        gMessageSystem.Post(ZMessage("kill_child", "name", "ImageViewerHelp"));
        return;
    }

    pHelp = new ZWinDialog();
    pHelp->msWinName = "ImageViewerHelp";
    pHelp->mbAcceptsCursorMessages = true;
    pHelp->mbAcceptsFocus = true;
//    pHelp->mTransformIn = ZWin::kToOrFrom;
//    pHelp->mTransformOut = ZWin::kToOrFrom;
//    pHelp->mToOrFrom = ZTransformation(ZPoint(mAreaAbsolute.right - gM*5, mAreaAbsolute.top+gM), 0.0, 0.0);

    ZRect r((int64_t) (grFullArea.Width() * 0.8), (int64_t)(grFullArea.Height() * 0.8));
    //ZRect r(mAreaLocal);
    pHelp->SetArea(r);
    pHelp->mBehavior = ZWinDialog::Draggable | ZWinDialog::OKButton;
    pHelp->mStyle = gDefaultDialogStyle;
    pHelp->mStyle.pad.h = (int32_t)gM;
    pHelp->mStyle.pad.v = (int32_t)gM;



    ZRect rCaption(r);
    rCaption.bottom = rCaption.top + gM * 3;
    rCaption.DeflateRect(gSpacer, gSpacer);
    ZWinLabel* pLabel = new ZWinLabel();
    pLabel->msText = "ZView Help";
    pLabel->SetArea(rCaption);
    pLabel->mStyle = gStyleCaption;
    pLabel->mStyle.pos = ZGUI::CT;
    pLabel->mStyle.pad.v = (int32_t)gM * 2;
    pLabel->mStyle.fp.nScalePoints = 2000;
    pLabel->mStyle.fp.nWeight = 800;
    pLabel->mStyle.look = ZGUI::ZTextLook::kShadowed;
    pLabel->mStyle.look.colTop = 0xff999999;
    pLabel->mStyle.look.colBottom = 0xff777777;
    pHelp->ChildAdd(pLabel);



    ZWinFormattedDoc* pForm = new ZWinFormattedDoc();

    ZGUI::Style text(gStyleGeneralText);
    text.fp.nScalePoints = 1000;
    text.pad.h = (int32_t)gM;

    ZGUI::Style sectionText(gStyleGeneralText);
    sectionText.fp.nWeight = 800;
    sectionText.fp.nScalePoints = 1000;
    sectionText.look.colTop = 0xffaaaaaa;
    sectionText.look.colBottom = 0xffaaaaaa;

    ZRect rForm((int64_t)(r.Width() * 0.48), (int64_t)(r.Height() * 0.8));
    rForm = ZGUI::Arrange(rForm, rCaption, ZGUI::ILOB, gSpacer, gSpacer);
    pForm->SetArea(rForm);
    pForm->SetScrollable();
    pHelp->ChildAdd(pForm);
    pHelp->Arrange(ZGUI::C, mAreaLocal);


    string sCurBuild;
    gRegistry.Get("app", "version", sCurBuild);
    pForm->AddMultiLine("\nAbout", sectionText);
    pForm->AddMultiLine("Written by Alex Zvenigorodsky\nBuild:\"" + sCurBuild + "\"\n\n", text);

    pForm->AddMultiLine("\nOverview", sectionText);
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
    pForm->AddMultiLine("Use 'DEL' to toggle an image as to-be-deleted. (It will be moved into a '_MARKED_TO_BE_DELETED_' subfolder.", text);
    pForm->AddMultiLine("Use the DELETE button in the toolbar to bring up a confirmation dialog with the list of photos marked for deletion.", text);
    pForm->AddMultiLine("CTRL-'C' copies the path of the current image to the clipboard.", text);
    pForm->AddMultiLine("CTRL-'R' look for the RAW (.cr3) version of the current image. (current folder, 'raw' subfolder, or '../raw' folders.)", text);


    pForm->AddMultiLine("Use the filter buttons in the toolbar to view either all images, just favorites, or just images marked for deletion.", text);


    pForm->AddMultiLine("Use 'UNDO' to undo the previous toggle or move.", text);

    pForm->AddMultiLine("\nYou can also use 'M' to quickly set a destination folder. Afterward 'M' will instantly move the image to that destination. (UNDO is available for this as well.)", text);

    pForm->AddMultiLine("\nAI Training Images", sectionText);
    pForm->AddMultiLine("Hold SHIFT and left drag a selection rectangle to crop/resize to 256x256 and save the result. This is for quickly generating training data for Dreambooth....maybe others.\n\n", text);
    pForm->InvalidateChildren();



    pForm = new ZWinFormattedDoc();

//    rForm.OffsetRect(rForm.Width() + gSpacer * 4, 0);
//    rForm = ZGUI::Arrange(rForm, r, ZGUI::RT, gSpacer * 4, pLabel->mStyle.fp.Height() + gSpacer);
    rForm = ZGUI::Arrange(rForm, rCaption, ZGUI::IROB, gSpacer, gSpacer);

    pForm->SetArea(rForm);
    pForm->SetScrollable();
//    pForm->mDialogStyle = text;
    pForm->SetBehavior(ZWinFormattedDoc::kBackgroundFill, true);
    pForm->mStyle.look.colTop = 0xffffffff;
    pForm->mStyle.look.colBottom = 0xffffffff;
    pHelp->ChildAdd(pForm);
    pHelp->Arrange(ZGUI::C, mAreaLocal);


    pForm->AddMultiLine("\nQuick Reference\n\n", sectionText);
    pForm->SetBehavior(ZWinFormattedDoc::kEvenColumns|ZWinFormattedDoc::kBackgroundFill, true);
    pForm->AddLineNode("<line><text>TAB</text><text>Toggle UI</text></line>");

    string sFormat = "<line wrap=0><text position=lb>%s</text><text position=lb>%s</text></line>";

    string sLine;
//    pForm->AddLineNode("\n");
    Sprintf(sLine, sFormat.c_str(), "CTRL+F", "Toggle windowed/fullscreen");    pForm->AddLineNode(sLine);
    Sprintf(sLine, sFormat.c_str(), "'O'", "Open Image Dialog");    pForm->AddLineNode(sLine);
    Sprintf(sLine, sFormat.c_str(), "LEFT/RIGHT or Mouse wheel", "Flip through images in current folder");    pForm->AddLineNode(sLine);
    Sprintf(sLine, sFormat.c_str(), "HOME/END", "Go to the first or last image");    pForm->AddLineNode(sLine);
    Sprintf(sLine, sFormat.c_str(), "ALT+Wheel", "Zoom");    pForm->AddLineNode(sLine);
    Sprintf(sLine, sFormat.c_str(), "Right-click", "Instant zoom between 100% and fit");    pForm->AddLineNode(sLine);
    Sprintf(sLine, sFormat.c_str(), "'1'", "Toggle Favorite Mark");    pForm->AddLineNode(sLine);
    Sprintf(sLine, sFormat.c_str(), "DEL", "Toggle Marked-for-Deletion");    pForm->AddLineNode(sLine);
    Sprintf(sLine, sFormat.c_str(), "'M'", "Set folder for move-to.");    pForm->AddLineNode(sLine);
    Sprintf(sLine, sFormat.c_str(), " ", "(After a folder is set, further presses move image instantly.)");    pForm->AddLineNode(sLine);

    ChildAdd(pHelp);
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
    if (ZWinFileDialog::ShowSelectFolderDialog(sFolder, mCurrentFolder.string(), "Select Move Destination"))
    {
        if (filesystem::path(sFolder) == mCurrentFolder)
        {
            // do not set move to folder to the same as current
            mMoveToFolder.clear();
            return;
        }
        if (MoveImage(EntryFromIndex(mViewingIndex)->filename, filesystem::path(sFolder)))
        {
            ShowTooltipMessage("moved", 0xff888800);
            mMoveToFolder = sFolder;
        }
        else
        {
            ShowTooltipMessage("move error", 0xffff0000);
            mMoveToFolder.clear();
        }
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
    if (ZWinFileDialog::ShowSelectFolderDialog(sFolder, mCurrentFolder.string(), "Select Copy Destination"))
    {
        filesystem::path newPath(sFolder);

        bool bForbiddenFolder = newPath == mCurrentFolder || newPath.parent_path() == FavoritesPath() || newPath.parent_path() == ToBeDeletedPath();

        if (bForbiddenFolder)
        {
            // do not set copy to folder into our tree
            mCopyToFolder.clear();
            ShowTooltipMessage(string("Cannot copy image to ") + sFolder, 0xffff0000);
            return;
        }

        if (CopyImage(EntryFromIndex(mViewingIndex)->filename, filesystem::path(sFolder)))
        {
            ShowTooltipMessage(string("Copied image to ") + newPath.string(), 0xff008800);
            mCopyToFolder = sFolder;
        }
        else
        {
            ShowTooltipMessage(string("Error copying image to ") + newPath.string(), 0xffff0000);
            mCopyToFolder.clear();
        }
    }
    else
    {
        mCopyToFolder.clear();
    }

    if (mpWinImage)
        mpWinImage->SetFocus();
    UpdateCaptions();
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
        //mbShowFavOrDelState = mbShowUI;


//        int64_t nGroupSide = (gM * 2) - gSpacer * 4;

//        mSymbolicStyle = ZGUI::Style(ZFontParams("Arial", 1500, 200, 0, 0, false, true), ZGUI::ZTextLook{}, ZGUI::C, 0);
        //        mpSymbolicFont = gpFontSystem->CreateFont(unicodeStyle.fp);


//        ZGUI::Style favorites = ZGUI::Style(ZFontParams("Arial", 8000, 400, 0, 0, false, true), ZGUI::ZTextLook{}, ZGUI::C, 0);
//        mpFavoritesFont = gpFontSystem->CreateFont(favorites.fp);
//        mpFavoritesFont.get()->GenerateSymbolicGlyph('C', 0x2655);  // crown



        SetFocus();
        mpWinImage = new ZWinImage();
        ZRect rImageArea(mAreaLocal);
//        rImageArea.left += 64;  // thumbs
        mpWinImage->SetArea(rImageArea);
        mpWinImage->mFillColor = 0xff000000;
        mpWinImage->mZoomHotkey = VK_MENU;
        if (mbSubsample)
            mpWinImage->nSubsampling = 4;
        mpWinImage->mBehavior |= ZWinImage::kHotkeyZoom|ZWinImage::kScrollable|ZWinImage::kSelectableArea|ZWinImage::kLaunchGeolocation|ZWinImage::kShowCaptionOnMouseOver;

//        mpWinImage->mCaptionMap["zoom"].style.pad.v = gStyleCaption.fp.nHeight;

        mpWinImage->mpTable = new ZGUI::ZTable();
        mpWinImage->mIconMap["favorite"].imageFilename = "res/star.svg";
        mpWinImage->mIconMap["favorite"].visible = false;

        ChildAdd(mpWinImage);
        mpWinImage->SetFocus();

        mToggleUIHotkey = VK_TAB;

        Clear();




        mpFavoriteIcon.reset(new ZBuffer());
        mpFavoriteIcon->LoadBuffer("res/star.svg");


        mpFolderLabel = new ZWinFolderLabel();
        mpFolderLabel->mBehavior = ZWinFolderLabel::kCollapsable;
        mpFolderLabel->mCurPath = mCurrentFolder;
        mpFolderLabel->mStyle.bgCol = 0;
        mpFolderLabel->mStyle.pos = ZGUI::LC;
        mpFolderLabel->mStyle.pad.h = (int32_t)gSpacer;
        mpFolderLabel->mStyle.pad.v = (int32_t)gSpacer;
        mpFolderLabel->mStyle.fp.nScalePoints = 800;
        mpFolderLabel->SetArea(ZRect(0, 0, mAreaLocal.Width() / 4, gM));
        ChildAdd(mpFolderLabel, !mCurrentFolder.empty());


        ZGUI::Style style(gDefaultPanelStyle);
        style.pos = ZGUI::CT;
//        style.pad.h = (int32_t)gSpacer/2;
//        style.pad.v = (int32_t)gSpacer/2;
        style.pad.h = 0;
        style.pad.v = 0;


        ZGUI::Style spacestyle(gDefaultPanelStyle);
        spacestyle.pos = ZGUI::CT;
        spacestyle.pad.h = (int32_t)gSpacer;
        spacestyle.pad.v = (int32_t)gSpacer;


        ZGUI::RA_Descriptor rad(ZRect(0, 0, grFullArea.right, gM * 2), "full", ZGUI::L|ZGUI::T|ZGUI::R, 1.0, 0.05f, -1, 64, -1, 128);


        mpPanel = new ZWinPanel();
        mpPanel->mPanelLayout = "<panel show_init=1 show_on_mouse_enter=1 style=\"" + SH::URL_Encode((string)style) + "\" rel_area_desc=\"" + SH::URL_Encode((string)rad) + "\"><row>";

        mpPanel->mrTrigger = mAreaLocal;
        mpPanel->mrTrigger.bottom = mAreaLocal.top + 20;

        mpPanel->mPanelLayout += ZWinPanel::MakeButton("openfile", "File", "", "$apppath$/res/openfile.svg", "Load Image", ZMessage("loadimg", this), 1.0, style, {});
        
        // file group
        string sFileGroupLayout = "<panel hide_on_button=1 hide_on_mouse_exit=1 border=1 spacers=1>";
        sFileGroupLayout += "<row>" + ZWinPanel::MakeButton("savefile", "", "", "$apppath$/res/save.svg", "Save Image", ZMessage("saveimg", this), 1.0, style, {}) + "</row>";
        sFileGroupLayout += "<row>" + ZWinPanel::MakeButton("copyfile", "", "", "$apppath$/res/copy.svg", "Select a Copy Folder for quick-move with 'C'", ZMessage("set_copy_folder", this), 1.0, style, {}) + "</row>";
        sFileGroupLayout += "<row>" + ZWinPanel::MakeButton("movefile", "", "", "$apppath$/res/move.svg", "Select a Move Folder for quick-move with 'M'", ZMessage("set_move_folder", this), 1.0, style, {}) + "</row>";
        sFileGroupLayout += "</panel>";

        ZGUI::RA_Descriptor fileManageRAD({}, "file_manage_menu", ZGUI::VOutside | ZGUI::HC | ZGUI::T, 1.25, 6.0);
        mpPanel->mPanelLayout += ZWinPanel::MakeSubmenu("file_manage_menu", "File", "", "$apppath$/res/manage.svg", "Manage File", sFileGroupLayout, fileManageRAD, 1.0, style);



        mpPanel->mPanelLayout += ZWinPanel::MakeButton("gotofolder", "File", "", "$apppath$/res/gotofolder.svg", "Go to Image Folder", ZMessage("gotofolder", this), 1.0, style, {});
        mpPanel->mPanelLayout += ZWinPanel::MakeButton("copylink", "File", "", "$apppath$/res/linkcopy.svg", "Copy path to image to clipboard", ZMessage("copylink", this), 1.0, style, {});
        mpPanel->mPanelLayout += ZWinPanel::MakeButton("psraw", "File", "", "$apppath$/res/ps_raw.svg", "Open CR3 in Photoshop", ZMessage("opencr3", this), 1.0, style, {});

        string sRotateGroupLayout = "<panel hide_on_mouse_exit=1 border=1 spacers=0>";
        sRotateGroupLayout += "<row>" + ZWinPanel::MakeButton("rot_left", "", "", "$apppath$/res/rot_left.svg", "Left", ZMessage("rotate_left", this), 1.0, style, {}) + "</row>";
        sRotateGroupLayout += "<row>" + ZWinPanel::MakeButton("rot_right", "", "", "$apppath$/res/rot_right.svg", "Right", ZMessage("rotate_right", this), 1.0, style, {}) + "</row>";
        sRotateGroupLayout += "<row>" + ZWinPanel::MakeButton("flip_h", "", "", "$apppath$/res/fliph.svg", "Flip Horizontal", ZMessage("flipH", this), 1.0, style, {}) + "</row>";
        sRotateGroupLayout += "<row>" + ZWinPanel::MakeButton("flip_v", "", "", "$apppath$/res/flipv.svg", "Flip Vertical", ZMessage("flipV", this), 1.0, style, {}) + "</row>";
        sRotateGroupLayout += "</panel>";

        
        ZGUI::RA_Descriptor rotateRAD({}, "rotate_menu", ZGUI::VOutside | ZGUI::HC | ZGUI::T, 1.0, 4.0);
        mpPanel->mPanelLayout += ZWinPanel::MakeSubmenu("rotate_menu", "Transform", "", "$apppath$/res/rotate.svg", "Transform", sRotateGroupLayout, rotateRAD, 1.0, style);

        mpPanel->mPanelLayout += "<space style=\"" + ZXML::Encode((string)spacestyle) + "\"/>";

        // Filter group
        ZGUI::Style checked(gStyleToggleChecked);
        ZGUI::Style unchecked(gStyleToggleUnchecked);
        checked.fp.nScalePoints = gM * 20;
        unchecked.fp.nScalePoints = gM * 20;

        mpPanel->mPanelLayout += ZWinPanel::MakeRadio("filterall", "Filter", "FilterGroup", "All", "", "All images", ZMessage("filter_all", this), "", 3.0, checked, unchecked);
        mpPanel->mPanelLayout += ZWinPanel::MakeRadio("filterfavs", "Filter", "FilterGroup", "Favorites", "", "Favorites (hit '1' to toggle current image)", ZMessage("filter_favs", this), "", 3.0, checked, unchecked);
        mpPanel->mPanelLayout += ZWinPanel::MakeRadio("filterranked", "Filter", "FilterGroup", "Ranked", "", "Images that have been ranked", ZMessage("filter_ranked", this), "", 3.0, checked, unchecked);
        mpPanel->mPanelLayout += ZWinPanel::MakeRadio("filterdel", "Filter", "FilterGroup", "To Be Deleted", "", "Flagged for deletion", ZMessage("filter_del", this), "", 3.0, checked, unchecked);
        mpPanel->mPanelLayout += ZWinPanel::MakeButton("deletemarked", "", "", "$apppath$/res/trash.svg", "Delete images marked for deletion (with confirmation)", ZMessage("show_confirm", this), 1.0, style, {});




        // Window Controls
        ZGUI::Style rightAligned(gStyleCaption);
        rightAligned.pos = ZGUI::R;
        mpPanel->mPanelLayout += ZWinPanel::MakeButton("rank_favorites", "", "Rank Favorites", "", "Rank favorites", ZMessage("rank_favorites", this), 3.0, rightAligned, gStyleCaption);
        mpPanel->mPanelLayout += "<space style=\"" + ZXML::Encode((string)spacestyle) + "\"/>";


        ZGUI::Style bigFontStyle(gStyleCaption);
        bigFontStyle.fp.nScalePoints = 2000;
        bigFontStyle.pos = ZGUI::C;
        mpPanel->mPanelLayout += ZWinPanel::MakeButton("show_help", "", "?", "", "Help", ZMessage("show_help", this), 1.0, rightAligned, bigFontStyle);
        mpPanel->mPanelLayout += ZWinPanel::MakeButton("toggle_fullscreen", "", "", "$apppath$/res/fullscreen.svg", "Fullscreen/Windowed", ZMessage("toggle_fullscreen"), true, rightAligned, {});
        mpPanel->mPanelLayout += ZWinPanel::MakeButton("quit_button", "", "", "$apppath$/res/x.svg", "Quit", ZMessage("quit", this), true, rightAligned, {});

        // Context


        mpPanel->mPanelLayout += "</row></panel>";
        
//        mpPanel->mRAD = ZGUI::RA_Descriptor(ZRect(0, 0, grFullArea.Width(), grFullArea.Height()), "full", ZGUI::L | ZGUI::T | ZGUI::R | ZGUI::VInside, 1.0, 0.025);
        
//        ZRect rPanel = ZGUI::Align(mpPanel->mRAD.area, grFullArea, mpPanel->mRAD.alignment, 0, 0, 1.0, 0.05);
//        mpPanel->SetArea(rPanel);

        string sPersistedViewPath;
        if (gRegistry.Get("ZImageViewer", "image", sPersistedViewPath))
        {
            ViewImage(sPersistedViewPath);
        }
        else
        {
            gMessageSystem.Post(ZMessage("show_help", this));
        }

        
        GetTopWindow()->ChildAdd(mpPanel);
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
    std::filesystem::path cr3Path = FindCR3();
    gMessageSystem.Post(ZMessage("set_enabled", "enabled", SH::FromInt((int)!cr3Path.empty()), "target", "psraw")); // if cr3Path is not empty, then a raw file is available to open


    string sAppPath = gRegistry["apppath"];
    if (gGraphicSystem.mbFullScreen)
        gMessageSystem.Post(ZMessage("set_image", "target", "toggle_fullscreen", "image", sAppPath + "/res/windowed.svg"));
    else
        gMessageSystem.Post(ZMessage("set_image", "target", "toggle_fullscreen", "image", sAppPath + "/res/fullscreen.svg"));
    
    if (mbShowUI && mpPanel)
    {
        if (mFilterState == kRanked)
            gMessageSystem.Post(ZMessage("radio_check", "target", "filterranked", "group", "FilterGroup", "checked", "filterranked"));
        else if (mFilterState == kToBeDeleted)
            gMessageSystem.Post(ZMessage("radio_check", "target", "filterdel", "group", "FilterGroup", "checked", "filterdel"));
        else if (mFilterState == kFavs)
            gMessageSystem.Post(ZMessage("radio_check", "target", "filterfavs", "group", "FilterGroup", "checked", "filterfavs"));
        else
            gMessageSystem.Post(ZMessage("radio_check", "target", "filterall", "group", "FilterGroup", "checked", "filterall"));


        gMessageSystem.Post(ZMessage("set_enabled", "target", "copylink", "enabled", SH::FromInt((int)ValidIndex(mViewingIndex))));

        gMessageSystem.Post(ZMessage("set_enabled", "target", "filterfavs", "enabled", SH::FromInt((int)!mFavImageArray.empty())));

        gMessageSystem.Post(ZMessage("set_enabled", "enabled", SH::FromInt((int)!mRankedArray.empty()), "target", "filterranked"));

        gMessageSystem.Post(ZMessage("set_enabled", "enabled", SH::FromInt((int)!mToBeDeletedImageArray.empty()), "target", "filterdel"));

        gMessageSystem.Post(ZMessage("set_enabled", "enabled", SH::FromInt((int)!mToBeDeletedImageArray.empty()), "target", "deletemarked"));

        gMessageSystem.Post(ZMessage("set_visible", "visible", SH::FromInt((int)(mFilterState == kFavs || mFilterState == kRanked)), "target", "rank_favorites"));
    }

    if (mpPanel)
        mpPanel->SetVisible(mbShowUI);
}

bool ImageViewer::OnParentAreaChange()
{
    if (!mpSurface)
        return false;

    SetArea(mpParentWin->GetArea());

    ZWin::OnParentAreaChange();
    UpdateUI();

/*    ZGUI::Style favorites = ZGUI::Style(ZFontParams("Arial", 8000, 400, 0, 0, false, true), ZGUI::ZTextLook{}, ZGUI::C, 0);
    mpFavoritesFont = gpFontSystem->CreateFont(favorites.fp);
    ((ZDynamicFont*)mpFavoritesFont.get())->GenerateSymbolicGlyph('C', 0x2655);  // crown
    */

    return true;
}


bool ImageViewer::ViewImage(const std::filesystem::path& filename)
{
    if (!filename.empty() && filesystem::exists(filename.parent_path()))
    {
        if (mCurrentFolder != filename.parent_path())
            ScanForImagesInFolder(filename.parent_path());
    }

    mToggleUIHotkey = VK_TAB;
//    UpdateFilteredView(kAll);   // set filter to all and fill out del and fav arrays

    mViewingIndex = IndexFromPath(filename);
    UpdateFilteredView(mFilterState);

/*    if (mViewingIndex.favIndex >= 0) // if current image is fav, set that as the filter
        UpdateFilteredView(kFavs);
    else if (mViewingIndex.delIndex >= 0)   // same with del
        UpdateFilteredView(kToBeDeleted);*/

    UpdateControlPanel();
    mCachingState = kReadingAhead;
    KickCaching();

    return true;
}


// 1) look for cr3 version in cur folder
// 2) look for cr3 version in 'raw' subfolder
// 3) look for cr3 version in '../raw' subfolder
std::filesystem::path ImageViewer::FindCR3()
{
    if (ValidIndex(mViewingIndex))
    {

        std::filesystem::path cr3Filename = mImageArray[mViewingIndex.absoluteIndex]->filename.filename();
        cr3Filename.replace_extension(".cr3");

        std::filesystem::path curFolderPath = mCurrentFolder;
        curFolderPath.append(cr3Filename.string());

        if (std::filesystem::exists(curFolderPath))
            return curFolderPath;

        std::filesystem::path curFolderRawPath = mCurrentFolder;
        curFolderRawPath += "/raw/";
        curFolderRawPath.append(cr3Filename.string());

        if (std::filesystem::exists(curFolderRawPath))
            return curFolderRawPath;

        std::filesystem::path parentFolderRawPath = mCurrentFolder.parent_path();
        parentFolderRawPath += "/raw/";
        parentFolderRawPath.append(cr3Filename.string());

        if (std::filesystem::exists(parentFolderRawPath))
            return parentFolderRawPath;
    }

    return {};
}

bool ImageViewer::LaunchCR3(std::filesystem::path cr3Path)
{
    // launch from cur folder
    if (cr3Path.empty() || !std::filesystem::exists(cr3Path))
        return false;

    ShellExecuteA(0, "open", cr3Path.string().c_str(), 0, 0, SW_SHOWNORMAL);
    return true;
}


void ImageViewer::LoadMetadataProc(std::filesystem::path& imagePath, shared_ptr<ImageEntry> pEntry, std::atomic<int64_t>* pnOutstanding)
{
    if (!pEntry || !pnOutstanding)
        return;

//    ZDEBUG_OUT("Loading EXIF:", imagePath, "\n");

    pEntry->mMeta = gImageMeta.Entry(imagePath.string());

    string sExt = imagePath.extension().string();
    SH::makelower(sExt);
    bool bJPG = (sExt == ".jpg" || sExt == ".jpeg");

    if (bJPG && ZBuffer::ReadEXIFFromFile(imagePath.string(), pEntry->mEXIF))
    {
        pEntry->mState = ImageEntry::kMetadataReady;
    }
    else
        pEntry->mState = ImageEntry::kNoExifAvailable;

    (*pnOutstanding)--;
    int64_t n = *pnOutstanding;
    //ZDEBUG_OUT("outstanding:", n, "\n");
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
    mOutstandingMetadataCount = mImageArray.size();

    // kick off exif reading if necessary
    for (auto& entry : mImageArray)
    {
        if (entry->mState == ImageEntry::kInit)
        {
            entry->mState = ImageEntry::kLoadingMetadata;
            mpImageLoaderPool->enqueue(&ImageViewer::LoadMetadataProc, entry->filename, entry, &mOutstandingMetadataCount);
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
    if (mpFolderLabel)
    {
        mpFolderLabel->mCurPath = folder;
        mpFolderLabel->SizeToPath();
    }

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
        gMessageSystem.Post(ZMessage("toggleconsole"));
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

            UpdateFilteredView(mFilterState);
        }


        tZBufferPtr curImage = GetCurImage();

        if (curImage && mpWinImage->mpImage.get() != curImage.get())
        {
            if (mpWinImage->GetArea().Width() > 0 && mpWinImage->GetArea().Height() > 0)
            {
                mpWinImage->SetImage(curImage);
                ZDEBUG_OUT("Setting image:", curImage->GetEXIF().DateTime, "\n");

                mpWinImage->mCaptionMap["no_image"].Clear();

                mImageArrayMutex.lock();
                gRegistry["ZImageViewer"]["image"] = mImageArray[mViewingIndex.absoluteIndex]->filename.string();
                mImageArrayMutex.unlock();

                UpdateCaptions();
                InvalidateChildren();
            }
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
        mFilterState = kAll;
    }

    // filter state may have changed to kAll above.... check again if anything matches
    if (CountImagesMatchingFilter(mFilterState) != 0)
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
    else
    {
        mViewingIndex = {};
        mpWinImage->Clear();
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

    mpWinImage->mCaptionMap["filename"].Clear();
//    mpWinImage->mCaptionMap["folder"].Clear();
    //mpWinImage->mCaptionMap["favorite"].Clear();
    mpWinImage->mCaptionMap["for_delete"].Clear();
    mpWinImage->mCaptionMap["no_image"].Clear();
    mpWinImage->mCaptionMap["image_count"].Clear();
    mpWinImage->mCaptionMap["rank"].Clear();

    mpWinImage->mpTable->mCellStyle = gStyleCaption;
    mpWinImage->mpTable->mCellStyle.pos = ZGUI::LC;
    mpWinImage->mpTable->mCellStyle.pad.h = (int32_t)(gSpacer / 2);
    mpWinImage->mpTable->mCellStyle.pad.v = (int32_t)(gSpacer / 2);

    mpWinImage->mpTable->mTableStyle.pos = ZGUI::RB;
    mpWinImage->mpTable->mTableStyle.bgCol = 0x88000000;
    mpWinImage->mpTable->mTableStyle.pad.h = (int32_t)gSpacer;
    mpWinImage->mpTable->mTableStyle.pad.v = (int32_t)gSpacer;

    string sCaption;
    if (mViewingIndex.absoluteIndex >= 0)
        sCaption = std::format("All\n({}/{})", mViewingIndex.absoluteIndex+1, mImageArray.size());
    else
        sCaption = std::format("All\n({})", mImageArray.size());
    gMessageSystem.Post(ZMessage("set_caption", "target", "filterall", "text", sCaption));


    if (mViewingIndex.favIndex >= 0)
        sCaption = std::format("Favorites\n({}/{})", mViewingIndex.favIndex+1, mFavImageArray.size());
    else
        sCaption = std::format("Favorites\n({})", mFavImageArray.size());
    gMessageSystem.Post(ZMessage("set_caption", "target", "filterfavs", "text", sCaption));

    if (mViewingIndex.rankedIndex >= 0)
        sCaption = std::format("Ranked\n({}/{})", mViewingIndex.rankedIndex+1, mRankedArray.size());
    else
        sCaption = std::format("Ranked\n({})", mRankedArray.size());
    gMessageSystem.Post(ZMessage("set_caption", "target", "filterranked", "text", sCaption));

    if (mViewingIndex.delIndex >= 0)
        sCaption = std::format("To Be Deleted\n({}/{})", mViewingIndex.delIndex+1, mToBeDeletedImageArray.size());
    else
        sCaption = std::format("To Be Deleted\n({})", mToBeDeletedImageArray.size());
    gMessageSystem.Post(ZMessage("set_caption", "text", sCaption, "target", "filterdel"));


    if (mpWinImage)
    {
        int64_t topPadding = 0; // in case panel is visible
        if (mpPanel)
            topPadding = mpPanel->GetArea().Height();


        ZGUI::Style folderStyle(gStyleButton);
        folderStyle.pos = ZGUI::LT;
        folderStyle.look = ZGUI::ZTextLook::kShadowed;
        folderStyle.pad.h = (int32_t) (gSpacer / 2);
        folderStyle.pad.v = (int32_t)(gSpacer / 2);


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
                if (ValidIndex(mViewingIndex))
                {
                    if (mFilterState == kRanked)
                    {
                        ImageMetaEntry& meta = mImageArray[mViewingIndex.absoluteIndex]->mMeta;
                        //                    if (meta.elo > 1000)
                        {
                            int64_t nRank = GetRank(meta.filename);
                            if (nRank > 0)
                            {
                                ZGUI::Style eloStyle(gStyleCaption);
                                eloStyle.fp.nScalePoints = 2000;
                                eloStyle.look.colTop = 0xffffd800;
                                eloStyle.look.colBottom = 0xff9d8500;
                                eloStyle.look.decoration = ZGUI::ZTextLook::kShadowed;
                                eloStyle.pos = ZGUI::RT;
                                eloStyle.pad.h += (int32_t)eloStyle.fp.Height();
                                Sprintf(mpWinImage->mCaptionMap["rank"].sText, "#%d\n%d", nRank, meta.elo);
                                mpWinImage->mCaptionMap["rank"].visible = true;
                                mpWinImage->mCaptionMap["rank"].style = eloStyle;
                            }
                        }
                    }
                }
            }

            if (ValidIndex(mViewingIndex))
            {
                mpWinImage->mIconMap["favorite"].visible = mImageArray[mViewingIndex.absoluteIndex]->IsFavorite();

                string sCountCaption("Image ");
                if (mImageArray[mViewingIndex.absoluteIndex]->IsFavorite())
                    sCountCaption = "Favorite ";
                else if (mImageArray[mViewingIndex.absoluteIndex]->ToBeDeleted())
                    sCountCaption = "To Be Deleted ";

                string sImageCount = "[" + sCountCaption + SH::FromInt(IndexInCurMode() + 1) + "/" + SH::FromInt(CountInCurMode()) + "]";
                mpWinImage->mCaptionMap["image_count"].sText = sImageCount;
                mpWinImage->mCaptionMap["image_count"].style = gStyleCaption;
                mpWinImage->mCaptionMap["image_count"].style.pos = ZGUI::LB;
                mpWinImage->mCaptionMap["image_count"].visible = true;

                const std::lock_guard<std::recursive_mutex> lock(mImageArrayMutex);
                if (mImageArray[mViewingIndex.absoluteIndex]->IsFavorite()/* && mpFavoritesFont*/)
                {
                    mpWinImage->mIconMap["favorite"].area = ZGUI::Arrange(ZRect(0, 0, gM * 4, gM * 4), mAreaLocal, ZGUI::RT, gSpacer, gSpacer);
                    mpWinImage->mCaptionMap["image_count"].style.look.colTop = 0xffe1b131;
                    mpWinImage->mCaptionMap["image_count"].style.look.colBottom = 0xffe1b131;
                    bShow = mbShowUI;
                }

                if (mImageArray[mViewingIndex.absoluteIndex]->ToBeDeleted())
                {
                    mpWinImage->mCaptionMap["for_delete"].sText = /*mImageArray[mnViewingIndex].filename.filename().string() +*/ "\nMARKED FOR DELETE";
                    mpWinImage->mCaptionMap["for_delete"].style = ZGUI::Style(ZFontParams("Ariel Bold", 5000, 400), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xffff0000, 0xffff0000), ZGUI::CB, ZGUI::Padding((int32_t)gM*8, (int32_t)gM*4), 0x88000000, true);
                    mpWinImage->mCaptionMap["for_delete"].visible = true;
                    //mpWinImage->mCaptionMap["for_delete"].blurBackground = 8.0;
                    mpWinImage->mCaptionMap["for_delete"].renderedText.clear(); // force re-render

                    mpWinImage->mCaptionMap["image_count"].style.look.colTop = 0xffff0000;
                    mpWinImage->mCaptionMap["image_count"].style.look.colBottom = 0xffff0000;

                    bShow = true;
                }
            }
        }

        if (!mMoveToFolder.empty())
        {
            Sprintf(mpWinImage->mCaptionMap["move_to_folder"].sText, "'M' -> move to:\n%s", mMoveToFolder.string().c_str());
            mpWinImage->mCaptionMap["move_to_folder"].style = ZGUI::Style(ZFontParams("Ariel Bold", folderStyle.fp.nScalePoints, 400), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xff0088ff, 0xff0088ff), ZGUI::LT, ZGUI::Padding((int32_t)(gSpacer / 2), (int32_t)(gSpacer / 2 + folderStyle.fp.nScalePoints *8)), 0x88000000, true);
            mpWinImage->mCaptionMap["move_to_folder"].visible = bShow;
        }
        else
        {
            mpWinImage->mCaptionMap["move_to_folder"].Clear();
        }

        if (!mCopyToFolder.empty())
        {
            Sprintf(mpWinImage->mCaptionMap["copy_to_folder"].sText, "'C' -> copy to:\n%s", mCopyToFolder.string().c_str());
            mpWinImage->mCaptionMap["copy_to_folder"].style = ZGUI::Style(ZFontParams("Ariel Bold", folderStyle.fp.nScalePoints, 400), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xff0088ff, 0xff0088ff), ZGUI::LT, ZGUI::Padding((int32_t)(gSpacer / 2), (int32_t)(gSpacer / 2 + folderStyle.fp.nScalePoints * 12)), 0x88000000, true);
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

            mpWinImage->mpTable->AddRow("Filename", EntryFromIndex(mViewingIndex)->filename.filename().string());



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
            style.fp.nScalePoints = (int64_t) ((float)style.fp.nScalePoints * 1.2f);

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

    //mbShowFavOrDelState = true;

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

    //mbShowFavOrDelState = true;

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
    if (!PrePaintCheck())
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpSurface.get()->GetMutex());

    if (mImageArray.empty())
    {
        gStyleCaption.Font()->DrawTextParagraph(mpSurface.get(), "No images", mAreaLocal, &gStyleCaption);
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
   
