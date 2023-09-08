#include "ZTypes.h"
#include "ZWin.h"
#include "ImageContest.h"
#include "ZWinImage.h"
#include "Resources.h"
#include "helpers/StringHelpers.h"
#include "ZStringHelpers.h"
#include "ZWinFileDialog.h"
#include "TopWinnersDialog.h"
#include "ZWinControlPanel.h"
#include "ZWinText.H"
#include "ZGUIStyle.h"
#include <algorithm>
#include "ZWinBtn.H"
#include "helpers/Registry.h"
#include "ZTimer.h"
#include "ZRandom.h"
#include "ZThumbCache.h"

using namespace std;


ImageContest::ImageContest()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mIdleSleepMS = 250;
    msWinName = "ZWinImageContest";
    mpPanel = nullptr;
    mpWinImage[kLeft] = nullptr;
    mpWinImage[kRight] = nullptr;
    mImageMeta[kLeft] = nullptr;
    mImageMeta[kRight] = nullptr;
    mToggleUIHotkey = 0;
    mbShowUI = true;
}
 
ImageContest::~ImageContest()
{
}


bool ImageContest::ShowOpenFolderDialog()
{
    string sFolder;
    string sDefaultFolder;
    if (!mCurrentFolder.empty())
        sDefaultFolder = mCurrentFolder.string();
    if (ZWinFileDialog::ShowSelectFolderDialog(sFolder, sDefaultFolder))
    {
        ScanFolder(sFolder);
    }
    return true;
}


void ImageContest::HandleQuitCommand()
{
    gMessageSystem.Post("quit_app_confirmed");
}

void ImageContest::UpdateUI()
{

    ZDEBUG_OUT("UpdateUI() mbShowUI:", mbShowUI, "\n");
    UpdateControlPanel();
    UpdateCaptions();
    InvalidateChildren();
}

bool ImageContest::OnMouseDownL(int64_t x, int64_t y)
{
    if (mpWinImage[kLeft]->GetArea().PtInRect(x, y))
    {
        SelectWinner(kLeft);
    }
    else if (mpWinImage[kRight]->GetArea().PtInRect(x, y))
    {
        SelectWinner(kRight);
    }

    return ZWin::OnMouseDownL(x, y);
}

bool ImageContest::OnKeyUp(uint32_t key)
{
    if (key == VK_MENU && !mbShowUI)
    {
        UpdateUI();
        return true;
    }


    return ZWin::OnKeyUp(key);
}

bool ImageContest::OnKeyDown(uint32_t key)
{
#ifdef _WIN64
    if (key == VK_ESCAPE)
    {
        HandleQuitCommand();
        return true;
    }
#endif

    bool bShow = true;
    gRegistry.Get("ZImageContest", "showui", bShow);
    if (key == mToggleUIHotkey)
    {
        mbShowUI = !mbShowUI;
        UpdateUI();
        gRegistry["ZImageContest"]["showui"] = mbShowUI;
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
    case VK_LEFT:
        SelectWinner(kLeft);
        return true;
        break;
    case VK_RIGHT:
        SelectWinner(kRight);
        return true;
        break;
    case 'o':
    case 'O':
        return ShowOpenFolderDialog();
    }

    return true;
}

bool ImageContest::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();
    if (sType == "quit")
    {
        HandleQuitCommand();
        return true;
    }
    else if (sType == "selectfolder")
    {
        ShowOpenFolderDialog();
        return true;
    }
    else if (sType == "show_help")
    {
        ShowHelpDialog();
        return true;
    }
    else if (sType == "show_winners")
    {
        mbShowUI = false;
        UpdateUI();
        ShowWinnersDialog();
        return true;
    }
    else if (sType == "reset_contest")
    {
        ResetContest();
        return true;
    }
    else if (sType == "select")
    {
        tZBufferPtr p1(new ZBuffer());
        string sFile = message.GetParam("filename");
        if (!p1->LoadBuffer(sFile))
        {
            ZERROR("Failed to load buffer:", message.GetParam("filename"));
            return true;
        }

        mpWinImage[kLeft]->SetImage(p1);        
        mImageMeta[kLeft] = &mMeta.Entry(sFile, std::filesystem::file_size(sFile));
        return true;
    }

    return ZWin::HandleMessage(message);
}




void ImageContest::ShowWinnersDialog()
{
    tImageMetaList fullList;
    for (auto& metalist : mMeta.mSizeToMetaLists)
    {
        for (auto& entry : metalist.second)
        {
            if (entry.wins > 0 && entry.elo > 1000)
            {
                filesystem::path filepath(entry.filename);
                if (filepath.parent_path() == mCurrentFolder)
                    fullList.push_back(entry);
            }
        }
    }

    if (fullList.empty())
        return;

    fullList.sort([](const ImageMetaEntry& a, const ImageMetaEntry& b) -> bool { return a.elo > b.elo; });
    TopWinnersDialog* pDialog = TopWinnersDialog::ShowDialog(fullList, mpWinImage[kRight]->GetArea());
}

void ImageContest::ResetContest()
{
    tImageMetaList fullList;
    for (auto& metalist : mMeta.mSizeToMetaLists)
    {
        for (auto& entry : metalist.second)
        {
            entry.wins = 0;
            entry.contests = 0;
            entry.elo = 1000;
        }
    }
    mMeta.Save();
    UpdateUI();
}

void ImageContest::ShowHelpDialog()
{
    ZWinDialog* pHelp = new ZWinDialog();
    pHelp->msWinName = "ZImageContestHelp";
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
    pForm->mDialogStyle.bgCol = gDefaultDialogFill;
    pForm->mbAcceptsCursorMessages = false;
    pHelp->ChildAdd(pForm);
    ChildAdd(pHelp);
    pHelp->Arrange(ZGUI::C, mAreaToDrawTo);

    pForm->AddMultiLine("\nImage Contest", sectionText);
    pForm->AddMultiLine("Use this feature to randomly choose two images from a folder and snap-judge which is the best.", text);
    pForm->AddMultiLine("Click on either image or use LEFT/RIGHT to select.", text);
    pForm->AddMultiLine("View the top winners with the button in the control panel.", text);

    pForm->Invalidate();


}

bool ImageContest::Init()
{
    if (!mbInitted)
    {
        mbShowUI = true;

        string sUserPath(getenv("APPDATA"));
        std::filesystem::path userDataPath(sUserPath);

        mMeta.basePath = sUserPath + "/ZImageViewer/contests/";

        if (std::filesystem::exists(mMeta.basePath))
            mMeta.LoadAll();
        else
            std::filesystem::create_directories(mMeta.basePath);


        SetFocus();
        mpWinImage[kLeft] = new ZWinImage();
        ZRect rImageArea(mAreaToDrawTo);
        rImageArea.right = rImageArea.left + rImageArea.Width() / 2;
        mpWinImage[kLeft]->SetArea(rImageArea);
        mpWinImage[kLeft]->mFillColor = 0xff000000;
        mpWinImage[kLeft]->mZoomHotkey = VK_MENU;
        mpWinImage[kLeft]->mBehavior |= ZWinImage::kHotkeyZoom|ZWinImage::kNotifyOnClick;
        mpWinImage[kLeft]->mpTable = new ZGUI::ZTable();
        ChildAdd(mpWinImage[kLeft]);

        mpWinImage[kRight] = new ZWinImage();
        rImageArea.OffsetRect(rImageArea.Width(), 0);
        mpWinImage[kRight]->SetArea(rImageArea);
        mpWinImage[kRight]->mFillColor = 0xff000000;
        mpWinImage[kRight]->mZoomHotkey = VK_MENU;
        mpWinImage[kRight]->mBehavior |= ZWinImage::kHotkeyZoom | ZWinImage::kNotifyOnClick;
        mpWinImage[kRight]->mpTable = new ZGUI::ZTable();
        ChildAdd(mpWinImage[kRight]);


        ZGUI::Style style = gStyleCaption;
        style.fp.nHeight = gM * 3;
        style.look.decoration = ZGUI::ZTextLook::kShadowed;
        style.pos = ZGUI::CB;

        mpWinImage[kLeft]->mCaptionMap["contests"].style = style;
        mpWinImage[kRight]->mCaptionMap["contests"].style = style;






        mToggleUIHotkey = VK_TAB;

        string sPersistedFolder;
        if (gRegistry.Get("ImageContest", "folder", sPersistedFolder))
        {
            ScanFolder(sPersistedFolder);
        }

        UpdateControlPanel();
        UpdateUI();


    }

    return ZWin::Init();
}



void ImageContest::UpdateControlPanel()
{
    int64_t nControlPanelSide = gM * 2;
//    limit<int64_t>(nControlPanelSide, 64, 128);


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
        
    int64_t nGroupSide = (gM * 2) - gSpacer * 4;



    ZGUI::Style unicodeStyle = ZGUI::Style(ZFontParams("Arial", nGroupSide, 200, 0, 0, false, true), ZGUI::ZTextLook{}, ZGUI::C, 0);
    ZGUI::Style wingdingsStyle = ZGUI::Style(ZFontParams("Wingdings", nGroupSide /2, 200, 0, 0, false, true), ZGUI::ZTextLook{}, ZGUI::C);


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

    pBtn = new ZWinSizablePushBtn();
    pBtn->mSVGImage.Load(sAppPath+"/res/exit.svg");
    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("quit", this));
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width() + gSpacer*2, 0);
    

    int64_t nButtonPadding = rButton.Width() / 8;

    pBtn = new ZWinSizablePushBtn();
    pBtn->mSVGImage.Load(sAppPath + "/res/openfolder.svg");
    pBtn->SetTooltip("Go to Folder");
    pBtn->mSVGImage.style.paddingH = nButtonPadding;
    pBtn->mSVGImage.style.paddingV = nButtonPadding;

    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("selectfolder", this));
    pBtn->msWinGroup = "File";
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width(), 0);


    string sMessage;


    // Contests
    rButton.OffsetRect(rButton.Width() + gSpacer * 2, 0);
    rButton.right = rButton.left + (int64_t) (rButton.Width() * 3);     // wider buttons for management

    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = "Winners";  // undo glyph
    pBtn->mCaption.style = gDefaultGroupingStyle;
    pBtn->mCaption.style.fp.nHeight = nGroupSide / 2;
    pBtn->mCaption.style.pos = ZGUI::C;
    pBtn->SetArea(rButton);
    pBtn->msWinGroup = "Contests";
    Sprintf(sMessage, "show_winners;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width() + gSpacer * 2, 0);
    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = "Reset";  // undo glyph
    pBtn->mCaption.style = gDefaultGroupingStyle;
    pBtn->mCaption.style.fp.nHeight = nGroupSide / 2;
    pBtn->mCaption.style.pos = ZGUI::C;
    pBtn->mCaption.style.look.colTop = 0xffff8800;
    pBtn->mCaption.style.look.colBottom = 0xffffff00;
    pBtn->SetArea(rButton);
    pBtn->msWinGroup = "Contests";
    Sprintf(sMessage, "reset_contest;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);






    rButton = ZGUI::Arrange(rButton, rPanelArea, ZGUI::RC, gSpacer/2);

    rButton.OffsetRect(-gSpacer/2, 0);

    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = "F";
    pBtn->mCaption.style = unicodeStyle;
    pBtn->mCaption.style.pos = ZGUI::C;
    pBtn->mCaption.style.paddingV = (int32_t) (-pBtn->mCaption.style.fp.nHeight/10);
    pBtn->SetArea(rButton);
    Sprintf(sMessage, "toggle_fullscreen");
    pBtn->SetMessage(sMessage);
    pBtn->msWinGroup = "View";
    mpPanel->ChildAdd(pBtn);


    ZGUI::Style filterButtonStyle = gDefaultGroupingStyle;
    filterButtonStyle.look.decoration = ZGUI::ZTextLook::kEmbossed;
    filterButtonStyle.fp.nHeight = nGroupSide / 3;
    filterButtonStyle.pos = ZGUI::C;
    filterButtonStyle.look.colTop = 0xff888888;
    filterButtonStyle.look.colBottom = 0xff888888;



    rButton.OffsetRect(-rButton.Width(), 0);
    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = "?"; // unicode flip H
    pBtn->mCaption.style = filterButtonStyle;
    pBtn->mCaption.style.fp.nHeight = nGroupSide / 2;
    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("show_help", this));
    pBtn->msWinGroup = "View";
    mpPanel->ChildAdd(pBtn);
}

bool ImageContest::OnParentAreaChange()
{
    if (!mpTransformTexture)
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpTransformTexture.get()->GetMutex());
    SetArea(mpParentWin->GetArea());


    UpdateControlPanel();
    ZWin::OnParentAreaChange();
    UpdateCaptions();

    SetFocus();
    ZRect rImageArea(mAreaToDrawTo);
    rImageArea.right = rImageArea.left + rImageArea.Width() / 2;
    mpWinImage[kLeft]->SetArea(rImageArea);
    rImageArea.OffsetRect(rImageArea.Width(), 0);
    mpWinImage[kRight]->SetArea(rImageArea);

    TopWinnersDialog* pDialog = (TopWinnersDialog*)GetChildWindowByWinName("TopWinnersDialog");

    if (pDialog)    // hack to deal with resizing while winners dialog is visible
    {
     //   pDialog->SetArea(rImageArea);
        ChildDelete(pDialog);
        mbShowUI = false;
        UpdateUI();
        ShowWinnersDialog();

    }


    //    static int count = 0;
//    ZOUT("ImageViewer::OnParentAreaChange\n", count++);

    return true;
}


bool ImageContest::AcceptedExtension(std::string sExt)
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


bool ImageContest::ScanFolder(std::filesystem::path folder)
{
    // if passing in a filename, scan the containing folder
    if (filesystem::is_regular_file(folder))
        folder = folder.parent_path();

    if (!std::filesystem::exists(folder))
        return false;

    string sTail = folder.filename().string();

    mCurrentFolder = folder;
    mImagesInCurrentFolder.clear();

    bool bErrors = false;

    for (auto filePath : std::filesystem::directory_iterator(mCurrentFolder))
    {
        if (filePath.is_regular_file() && AcceptedExtension(filePath.path().extension().string()))
        {
            mImagesInCurrentFolder.emplace_back(filePath);
//            mImageArray.emplace_back(new ImageEntry(filePath));
            //ZDEBUG_OUT("Found image:", filePath, "\n");
        }
    }

    if (bErrors)
    {
        gMessageSystem.Post("toggleconsole");
        return false;
    }

    PickRandomPair();

    //std::sort(mImageArray.begin(), mImageArray.end(), [](const shared_ptr<ImageEntry>& a, const shared_ptr<ImageEntry>& b) -> bool { return a->filename.filename().string() < b->filename.filename().string(); });
    

    UpdateCaptions();

    return true;
}

bool ImageContest::PickRandomPair()
{
    if (mImagesInCurrentFolder.size() < 2)
        return false;

    int64_t i = RANDI64(0, mImagesInCurrentFolder.size());
    int64_t j;
    do
    {
        j = RANDI64(0, mImagesInCurrentFolder.size());
    } while (j == i);

    tImageFilenames::iterator it1 = mImagesInCurrentFolder.begin();
    while (i-- > 0)
        it1++;

    tImageFilenames::iterator it2 = mImagesInCurrentFolder.begin();
    while (j-- > 0)
        it2++;


    tZBufferPtr p1(new ZBuffer());
    string file1 = (*it1).string();
    if (!p1->LoadBuffer(file1))
    {
        ZERROR("Failed to load buffer:", *it1);
        return false;
    }

    mpWinImage[kLeft]->SetImage(p1);

    tZBufferPtr p2(new ZBuffer());
    string file2 = (*it2).string();
    if (!p2->LoadBuffer(file2))
    {
        ZERROR("Failed to load buffer:", *it1);
        return false;
    }

    mpWinImage[kRight]->SetImage(p2);

    mImageMeta[kLeft] = &mMeta.Entry(file1, std::filesystem::file_size(*it1));
    mImageMeta[kRight] = &mMeta.Entry(file2, std::filesystem::file_size(*it2));

    UpdateCaptions();

    return true;
}



bool ImageContest::SelectWinner(int leftOrRight)
{
    if (leftOrRight == kLeft || leftOrRight == kRight)
    {
        mImageMeta[kLeft]->contests++;
        mImageMeta[kRight]->contests++;

        mImageMeta[leftOrRight]->wins++;

        double fExpectedLeft = 1.0 / (1.0 + std::pow(10, (mImageMeta[kRight]->elo - mImageMeta[kLeft]->elo) / 400.0));
        double fExpectedRight = 1.0 / (1.0 + std::pow(10, (mImageMeta[kLeft]->elo - mImageMeta[kRight]->elo) / 400.0));

        double fWeight = 100.0;

        // Assume kLeft
        double fActualLeft = 1;
        double fActualRight = 0;

        if (leftOrRight == kRight)
        {
            fActualLeft = 0;
            fActualRight = 1;
        }

        mImageMeta[kLeft]->elo += fWeight * (fActualLeft - fExpectedLeft);
        mImageMeta[kRight]->elo += fWeight * (fActualRight - fExpectedRight);

        ZOUT("Image:", mImageMeta[leftOrRight]->filename, " wins/contests:", mImageMeta[leftOrRight]->wins, "/", mImageMeta[leftOrRight]->contests, " ELO:", mImageMeta[leftOrRight]->elo);

        mMeta.SaveBucket(mImageMeta[kLeft]->filesize);
        mMeta.SaveBucket(mImageMeta[kRight]->filesize);



        // Store thumbnails for winning images
        if (mImageMeta[leftOrRight]->elo > 1000)
        {
            tZBufferPtr thumb = gThumbCache.GetThumb(mImageMeta[leftOrRight]->filename);
            if (!thumb)
            {
                gThumbCache.Add(mImageMeta[leftOrRight]->filename, mpWinImage[leftOrRight]->mpImage);
            }
        }



        PickRandomPair();
        return true;
    }

    return false;
}


void ImageContest::UpdateCaptions()
{
    if (!mpWinImage[kLeft] || !mpWinImage[kRight] || !mImageMeta[kLeft] || !mImageMeta[kRight])
        return;

    bool bShow = mbShowUI;
    if (gInput.IsKeyDown(VK_MENU))
        bShow = true;

    string sCap;
    Sprintf(sCap, "Wins %d/%d ELO:%d", mImageMeta[kLeft]->wins, mImageMeta[kLeft]->contests, mImageMeta[kLeft]->elo);
    mpWinImage[kLeft]->mCaptionMap["contests"].sText = sCap;

    Sprintf(sCap, "Wins %d/%d ELO:%d", mImageMeta[kRight]->wins, mImageMeta[kRight]->contests, mImageMeta[kRight]->elo);
    mpWinImage[kRight]->mCaptionMap["contests"].sText = sCap;

    if (bShow)
    {
        mpWinImage[kLeft]->mBehavior |= ZWinImage::kShowCaption;
        mpWinImage[kRight]->mBehavior |= ZWinImage::kShowCaption;
    }
    else
    {
        mpWinImage[kLeft]->mBehavior &= ~ZWinImage::kShowCaption;
        mpWinImage[kRight]->mBehavior &= ~ZWinImage::kShowCaption;
    }


    mpWinImage[kLeft]->Invalidate();
    mpWinImage[kRight]->Invalidate();
}


bool ImageContest::Paint()
{
    if (!mpTransformTexture)
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpTransformTexture.get()->GetMutex());

    if (!mbVisible)
        return false;

    if (!mbInvalid)
        return false;


    return ZWin::Paint();
}

std::filesystem::path ImageMeta::BucketFilename(int64_t size)
{
    uint16_t nSizeBucket = size % (16 * 1024);
    std::filesystem::path filename(basePath);
    filename.append(SH::FromInt(nSizeBucket));
    filename += kImageListExt;

    return filename;
}


ImageMetaEntry& ImageMeta::Entry(const std::string& filename, int64_t size)
{
    uint16_t nSizeBucket = size % (16 * 1024);
    tImageSizeToMeta::iterator findIt = mSizeToMetaLists.find(nSizeBucket);

    // See if it needs loading
    if (findIt == mSizeToMetaLists.end())
    {
        std::filesystem::path bucketFilename = BucketFilename(size);
        if (std::filesystem::exists(bucketFilename))
        {
            // try loading
            if (!Load(bucketFilename))
            {
                ZERROR("Failed to load bucket:", bucketFilename);
            }
        }
    }

    if (mSizeToMetaLists[nSizeBucket].empty())
    {
        ZOUT("Creating new bucket:", nSizeBucket, " for size:", size);
        return mSizeToMetaLists[nSizeBucket].emplace_back(ImageMetaEntry(filename, size));
    }

    for (tImageMetaList::iterator imageIt = mSizeToMetaLists[nSizeBucket].begin(); imageIt != mSizeToMetaLists[nSizeBucket].end(); imageIt++)
    {
        if ((*imageIt).filename == filename)
        {
            ZOUT("Found:", filename, " bucket:", nSizeBucket);
            return *imageIt;
        }
    }

    ZOUT("Creating entry for :", filename, " bucket:", nSizeBucket);
    return mSizeToMetaLists[nSizeBucket].emplace_back(ImageMetaEntry(filename, size));
}

bool ImageMeta::LoadAll()
{
    if (basePath.empty())
        return false;
    if (!std::filesystem::exists(basePath))
        return false;

    for (auto filePath : std::filesystem::directory_iterator(basePath))
    {
        if (filePath.is_regular_file() && filePath.path().extension() == kImageListExt)
        {
            Load(filePath.path());
        }
    }


}

bool ImageMeta::Load(const std::filesystem::path& imagelist)
{
    ifstream infile(imagelist, ios::binary);
    if (!infile)
    {
        cerr << "Failed to load " << imagelist << "\n";
        return false;
    }

    int64_t nSize = (int64_t)std::filesystem::file_size(imagelist);
    assert(nSize < 4 * 1024 * 1024);    // seems unusually large to be this large

    int64_t nImageSizes = SH::ToInt(imagelist.filename().string());

    assert(nImageSizes > 0 && nImageSizes < 32 * 1024);     // 16bit value
    mSizeToMetaLists[nImageSizes].clear();

    uint8_t* pBuf = new uint8_t[nSize];

    infile.read((char*)pBuf, nSize);

    uint8_t* pWalker = pBuf;
    int64_t nBytesLeft = nSize;

    if (*(uint32_t*)pWalker != kImageListTAG)
    {
        ZERROR("Image file:", imagelist, " doesn't start with tag:", kImageListTAG);
        return false;
    }
    pWalker += sizeof(uint32_t);

    uint32_t entries = *(uint32_t*)pWalker;
    pWalker += sizeof(uint32_t);

    nBytesLeft -= sizeof(uint32_t) * 2;



    while (nBytesLeft > 0)
    {
        ImageMetaEntry entry;
        int32_t nRead = entry.ReadEntry(pWalker);
        assert(nRead > 0 && nRead <= nBytesLeft);
        nBytesLeft -= nRead;
        pWalker += nRead;

        cout << "Read ImageMetaEntry: filename:" << entry.filename << " size:" << entry.filesize << " wins/contests:" << entry.wins << "/" << entry.contests << " elo:" << entry.elo << "\n";

        mSizeToMetaLists[nImageSizes].emplace_back(std::move(entry));
    }
    assert(mSizeToMetaLists[nImageSizes].size() == entries);
    if (mSizeToMetaLists[nImageSizes].size() != entries)
    {
        ZERROR("Image file", imagelist, " was supposed to contain:", entries, " entries but only:", mSizeToMetaLists[nImageSizes].size(), " parsed.");
    }

    delete[] pBuf;
    return true;
}

bool ImageMeta::Save()
{
    if (basePath.empty())
        return false;
    if (!std::filesystem::exists(basePath))
        return false;

    for (auto& bucket : mSizeToMetaLists)
    {
        SaveBucket(bucket.first);
    }
}

bool ImageMeta::SaveBucket(int64_t size)
{
    uint16_t nSizeBucket = size % (16 * 1024);
    tImageSizeToMeta::iterator findIt = mSizeToMetaLists.find(nSizeBucket);

    // See if it needs loading
    if (findIt == mSizeToMetaLists.end())
    {
        ZERROR("No bucket for filesize:", size, " bucket not found:", nSizeBucket);
        return false;
    }

    tImageMetaList& metaList = (*findIt).second;
    tImageMetaList filteredList;
    for (auto& entry : metaList)    // remove entries for missing files
    {
        if (filesystem::exists(entry.filename))
        {
            filteredList.push_back(entry);
        }
        else
        {
            ZWARNING("Removing entry for image not found on disk:", entry.filename);
        }
    }


    std::ofstream outBucket(BucketFilename(size), ios::binary | ios::trunc);

    outBucket.write((char*)&kImageListTAG, sizeof(uint32_t));
    uint32_t nEntries = filteredList.size();
    outBucket.write((char*)&nEntries, sizeof(uint32_t));

    char buf[1024];

    for (auto& entry : filteredList)
    {
        int32_t bytes = entry.WriteEntry((uint8_t*)buf);
        assert(bytes < 1024);
        outBucket.write(buf, bytes);
    }
}



int32_t ImageMetaEntry::ReadEntry(const uint8_t* pData)
{
    const uint8_t* pWalker = pData;
    uint8_t filenameSize = *pWalker;
    pWalker++;

    char tempbuf[256];
    filename.assign((const char*)pWalker, filenameSize);
    pWalker += filenameSize;

    filesize = *((int64_t*)pWalker);
    pWalker += sizeof(int64_t);

    contests = *((int32_t*)pWalker);
    pWalker += sizeof(int32_t);

    wins = *((int32_t*)pWalker);
    pWalker += sizeof(int32_t);

    elo = *((int32_t*)pWalker);
    pWalker += sizeof(int32_t);

    return (int32_t)(pWalker - pData);
}

int32_t ImageMetaEntry::WriteEntry(uint8_t* pDest)
{
    uint8_t* pWriter = pDest;
    assert(filename.size() < 256);

    uint8_t size = (uint8_t) filename.size();
    *pWriter = size;
    pWriter++;

    memcpy(pWriter, filename.data(), size);
    pWriter += size;

    *(int64_t*)pWriter = filesize;
    pWriter += sizeof(int64_t);

    *(int32_t*)pWriter = contests;
    pWriter += sizeof(int32_t);

    *(int32_t*)pWriter = wins;
    pWriter += sizeof(int32_t);

    *(int32_t*)pWriter = elo;
    pWriter += sizeof(int32_t);

    return (int32_t)(pWriter - pDest);
}

