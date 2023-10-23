#include "ZTypes.h"
#include "ZWin.h"
#include "ImageContest.h"
#include "ZWinImage.h"
#include "Resources.h"
#include "helpers/StringHelpers.h"
#include "ZStringHelpers.h"
#include "ZWinFileDialog.h"
#include "WinTopWinners.h"
#include "ZWinControlPanel.h"
#include "ZWinText.H"
#include "ZGUIStyle.h"
#include <algorithm>
#include "ZWinBtn.H"
#include "helpers/Registry.h"
#include "ZTimer.h"
#include "ZRandom.h"
#include "ZThumbCache.h"
#include "ImageMeta.h"
#include "ZAnimator.h"
#include "ZGraphicSystem.h"
#include "ZScreenBuffer.h"

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
    mpChooseBtn[kLeft] = nullptr;
    mpChooseBtn[kRight] = nullptr;



    mpRatedImagesStrip = nullptr;
    mToggleUIHotkey = 0;
    mbShowUI = true;
    //mpSymbolicFont = nullptr;
    mState = kNone;
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
    gMessageSystem.Post("set_visible", GetTopWindow()->GetChildWindowByWinName("ZWinImageViewer"), "visible", "1");
    gMessageSystem.Post("kill_child", GetTopWindow(), "name", GetTargetName());
    gMessageSystem.Post("set_focus", "target", "ZWinImageViewer");

//    gMessageSystem.Post("quit_app_confirmed");
}

void ImageContest::UpdateUI()
{
//    ZDEBUG_OUT("UpdateUI() mbShowUI:", mbShowUI, "\n");
    UpdateControlPanel();
    UpdateCaptions();

    ZRect rImageArea(mAreaLocal);
    if (mpPanel && mpPanel->mbVisible)
        rImageArea.top = mpPanel->GetArea().bottom;


    ZRect rRatedStrip(rImageArea);
    rRatedStrip.left = rRatedStrip.right - gM * 4;

    rImageArea.right = rRatedStrip.left;

    if (mState == kShowingSingle)
    {
        mpWinImage[kLeft]->SetArea(rImageArea);
        mpWinImage[kRight]->SetVisible(false);

        mpChooseBtn[kLeft]->SetVisible(false);
        mpChooseBtn[kRight]->SetVisible(false);
    }
    else
    {
        rImageArea.right = rImageArea.left + rImageArea.Width() / 2;
        mpWinImage[kLeft]->SetArea(rImageArea);
        rImageArea.OffsetRect(rImageArea.Width(), 0);
        mpWinImage[kRight]->SetArea(rImageArea);
        mpWinImage[kRight]->SetVisible();

        ZRect rChooseBtn(gM * 4, gM * 4);
        rChooseBtn = ZGUI::Arrange(rChooseBtn, mpWinImage[kLeft]->GetArea(), ZGUI::RB);

        mpChooseBtn[kLeft]->SetArea(rChooseBtn);
        mpChooseBtn[kLeft]->SetVisible();

        rChooseBtn = ZGUI::Arrange(rChooseBtn, mpWinImage[kRight]->GetArea(), ZGUI::LB);
        mpChooseBtn[kRight]->SetArea(rChooseBtn);
        mpChooseBtn[kRight]->SetVisible();
    }

    mCurrentFolderImageMeta.sort([](const ImageMetaEntry& a, const ImageMetaEntry& b) -> bool { return a.elo > b.elo; });
    if (mpRatedImagesStrip)
    {
        mpRatedImagesStrip->pMetaList = &mCurrentFolderImageMeta;
        mpRatedImagesStrip->Invalidate();
    }


    if (mpRatedImagesStrip)
        mpRatedImagesStrip->SetArea(rRatedStrip);

    InvalidateChildren();
}

bool ImageContest::OnMouseDownL(int64_t x, int64_t y)
{
    if (mState == kShowingSingle)
    {
        PickRandomPair();
        UpdateUI();
    }

/*    if (mpWinImage[kLeft]->GetArea().PtInRect(x, y))
    {
        if (mState == kSelectingFromPair)
        {
            return SelectWinner(kLeft);
        }
        else
        {
            PickRandomPair();
            UpdateUI();
        }
    }
    else if (mpWinImage[kRight]->GetArea().PtInRect(x, y))
    {
        return SelectWinner(kRight);
    }*/

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
        if (mState == kSelectingFromPair)
            SelectWinner(kLeft);
        return true;
        break;
    case VK_RIGHT:
        if (mState == kSelectingFromPair)
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
    else if (sType == "reset_contest")
    {
        ResetContest();
        return true;
    }
    else if (sType == "select_winner")
    {
        if (message.GetParam("side") == "left")
            SelectWinner(kLeft);
        else
            SelectWinner(kRight);
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

        // add zoom animation

        tZBufferPtr backgroundBuf(new ZBuffer());
        backgroundBuf->Init(grFullArea.Width(), grFullArea.Height());
        //GetTopWindow()->RenderToBuffer(backgroundBuf, grFullArea, grFullArea, this);
        gpGraphicSystem->GetScreenBuffer()->RenderVisibleRectsToBuffer(backgroundBuf.get(), grFullArea);


        ZRect rImageArea(mAreaLocal);
        if (mpPanel && mpPanel->mbVisible)
            rImageArea.top = mpPanel->GetArea().bottom;
        ZRect rRatedStrip(rImageArea);
        rRatedStrip.left = rRatedStrip.right - gM * 4;
        rImageArea.right = rRatedStrip.left;


        ZRect rScaledFit(ZGUI::ScaledFit(p1->GetArea(), rImageArea));
        ZAnimObject_TransformingImage* pImage = new ZAnimObject_TransformingImage(p1, backgroundBuf, &rScaledFit);
        pImage->mbFullScreenDraw = true;




        ZRect rThumb = StringToRect(message.GetParam("area"));


        double scale = ((double)rThumb.Width() / (double)rScaledFit.Width());

        ZTransformation start(rThumb.TL(), scale, 0, 255);
        ZTransformation end(rScaledFit.TL(), 1.0, 0, 255);

        pImage->StartTransformation(start);

        ZTransformation lastTrans(end);


        Sprintf(end.msCompletionMessage, "invalidate;children=1;target=%s", GetTopWindow()->GetTargetName().c_str());
        pImage->AddTransformation(end, 1000);
        pImage->AddTransformation(lastTrans, 120);

        gAnimator.AddObject(pImage);








        mState = kShowingSingle;
        mpWinImage[kLeft]->SetImage(p1);
        mpWinImage[kLeft]->FitImageToWindow();
        mImageMeta[kLeft] = &gImageMeta.Entry(sFile, std::filesystem::file_size(sFile));





        UpdateUI();
        return true;
    }

    return ZWin::HandleMessage(message);
}





void ImageContest::ResetContest()
{
    tImageMetaList fullList;
    for (auto& metalist : gImageMeta.mSizeToMetaLists)
    {
        for (auto& entry : metalist.second)
        {
            if (std::filesystem::path(entry.filename).parent_path() == mCurrentFolder)
            {
                ZDEBUG_OUT("Resetting contest data for:", entry.filename, "\n");
                entry.wins = 0;
                entry.contests = 0;
                entry.elo = 1000;
            }
        }
    }
    gImageMeta.Save();
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
    pHelp->Arrange(ZGUI::C, mAreaLocal);

    pForm->AddMultiLine("\nImage Contest", sectionText);
    pForm->AddMultiLine("Use this feature to randomly choose two images from a folder and snap-judge which is the best.", text);
    pForm->AddMultiLine("Click on either image or use LEFT/RIGHT to select.", text);
    pForm->AddMultiLine("Thumbnails along the right show top ranked images.", text);

    pForm->Invalidate();


}

bool ImageContest::Init()
{
    if (!mbInitted)
    {
        mbShowUI = true;

        mpRatedImagesStrip = new WinTopWinners();
        ChildAdd(mpRatedImagesStrip);

        SetFocus();
        mpWinImage[kLeft] = new ZWinImage();
        ZRect rImageArea(mAreaLocal);
        rImageArea.right = rImageArea.left + rImageArea.Width() / 2;
        mpWinImage[kLeft]->SetArea(rImageArea);
        mpWinImage[kLeft]->mFillColor = 0xff000000;
        mpWinImage[kLeft]->mZoomHotkey = VK_MENU;
        mpWinImage[kLeft]->mBehavior |= ZWinImage::kHotkeyZoom | ZWinImage::kScrollable | ZWinImage::kNotifyOnClick;
        mpWinImage[kLeft]->mpTable = new ZGUI::ZTable();
        ChildAdd(mpWinImage[kLeft]);

        mpWinImage[kRight] = new ZWinImage();
        rImageArea.OffsetRect(rImageArea.Width(), 0);
        mpWinImage[kRight]->SetArea(rImageArea);
        mpWinImage[kRight]->mFillColor = 0xff000000;
        mpWinImage[kRight]->mZoomHotkey = VK_MENU;
        mpWinImage[kRight]->mBehavior |= ZWinImage::kHotkeyZoom | ZWinImage::kScrollable;
        mpWinImage[kRight]->mpTable = new ZGUI::ZTable();
        ChildAdd(mpWinImage[kRight]);


        string sAppPath = gRegistry["apppath"];

        mpChooseBtn[kLeft] = new ZWinSizablePushBtn();
        mpChooseBtn[kLeft]->mSVGImage.Load(sAppPath + "/res/left.svg");
        mpChooseBtn[kLeft]->msTooltip = "Choose Left";
        mpChooseBtn[kLeft]->SetMessage(ZMessage("select_winner;side=left", this));
        ChildAdd(mpChooseBtn[kLeft]);

        mpChooseBtn[kRight] = new ZWinSizablePushBtn();
        mpChooseBtn[kRight]->mSVGImage.Load(sAppPath + "/res/right.svg");
        mpChooseBtn[kRight]->msTooltip = "Choose Right";
        mpChooseBtn[kRight]->SetMessage(ZMessage("select_winner;side=right", this));
        ChildAdd(mpChooseBtn[kRight]);

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
    int64_t nControlPanelSide = (int64_t)(gM * 2.5);
    limit<int64_t>(nControlPanelSide, 40, 88);


    const std::lock_guard<std::recursive_mutex> panelLock(mPanelMutex);
    
    if (mpPanel && mpPanel->GetArea().Width() == mAreaLocal.Width() && mpPanel->GetArea().Height() == nControlPanelSide)
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
        //ZDEBUG_OUT("Deleting Control Panel\n");
        ChildDelete(mpPanel);
        mpPanel = nullptr;
    }


    mpPanel = new ZWinControlPanel();
        
    int64_t nGroupSide = (gM * 2) - gSpacer * 4;



    mSymbolicStyle = ZGUI::Style(ZFontParams("Arial", nGroupSide, 200, 0, 0, false, true), ZGUI::ZTextLook{}, ZGUI::C, 0);
    ZDynamicFont* pFont = (ZDynamicFont*)mSymbolicStyle.Font().get();

    pFont->GenerateSymbolicGlyph('F', 0x2750);
    pFont->GenerateSymbolicGlyph('Q', 0x0F1C);  // quality rendering


    ZRect rPanelArea(mAreaLocal.left, mAreaLocal.top, mAreaLocal.right, mAreaLocal.top + nControlPanelSide);
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
    pBtn->msTooltip = "Back to viewer";
    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("quit", this));
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width() + gSpacer*2, 0);
    

    int64_t nButtonPadding = rButton.Width() / 8;

    pBtn = new ZWinSizablePushBtn();
    pBtn->mSVGImage.Load(sAppPath + "/res/openfolder.svg");
    pBtn->msTooltip = "Go to Folder";
    pBtn->mSVGImage.style.paddingH = (int32_t)nButtonPadding;
    pBtn->mSVGImage.style.paddingV = (int32_t)nButtonPadding;

    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("selectfolder", this));
    pBtn->msWinGroup = "File";
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width(), 0);


    string sMessage;


    // Contests
    rButton.OffsetRect(rButton.Width() + gSpacer * 2, 0);
    rButton.right = rButton.left + (int64_t) (rButton.Width() * 3);     // wider buttons for management

/*    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = "Winners";  // undo glyph
    pBtn->mCaption.style = gDefaultGroupingStyle;
    pBtn->mCaption.style.fp.nHeight = nGroupSide / 2;
    pBtn->mCaption.style.pos = ZGUI::C;
    pBtn->SetArea(rButton);
    pBtn->msWinGroup = "Contests";
    Sprintf(sMessage, "show_winners;target=%s", GetTargetName().c_str());
    pBtn->SetMessage(sMessage);
    mpPanel->ChildAdd(pBtn);

    rButton.OffsetRect(rButton.Width() + gSpacer * 2, 0);*/

    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = "Reset";  // undo glyph
    pBtn->msTooltip = "Resets all of the ratings for images in this folder";
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






    rButton.SetRect(0,0,nGroupSide, nGroupSide);

    rButton = ZGUI::Arrange(rButton, rPanelArea, ZGUI::RC, gSpacer/2);

    rButton.OffsetRect(-gSpacer/2, 0);


    pBtn = new ZWinSizablePushBtn();
    pBtn->mSVGImage.Load(sAppPath + "/res/fullscreen.svg");
    pBtn->msWinGroup = "View";

    pBtn = new ZWinSizablePushBtn();
    pBtn->mSVGImage.Load(sAppPath + "/res/fullscreen.svg");
    pBtn->msWinGroup = "View";
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

    SetFocus();
}

bool ImageContest::OnParentAreaChange()
{
    if (!mpSurface)
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpSurface.get()->GetMutex());
    SetArea(mpParentWin->GetArea());


    ZWin::OnParentAreaChange();
    UpdateUI();

    SetFocus();

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
    mCurrentFolderImageMeta.clear();

    bool bErrors = false;

    for (auto filePath : std::filesystem::directory_iterator(mCurrentFolder))
    {
        if (filePath.is_regular_file() && AcceptedExtension(filePath.path().extension().string()))
        {
            mImagesInCurrentFolder.emplace_back(filePath);
//            mImageArray.emplace_back(new ImageEntry(filePath));
            //ZDEBUG_OUT("Found image:", filePath, "\n");

            ImageMetaEntry localEntry(gImageMeta.Entry(filePath.path().string(), filesystem::file_size(filePath)));
            if (localEntry.contests > 0)
                mCurrentFolderImageMeta.emplace_back(std::move(localEntry));
        }
    }

    if (bErrors)
    {
        gMessageSystem.Post("toggleconsole");
        return false;
    }


    PickRandomPair();

   

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

    mImageMeta[kLeft] = &gImageMeta.Entry(file1, std::filesystem::file_size(*it1));
    mImageMeta[kRight] = &gImageMeta.Entry(file2, std::filesystem::file_size(*it2));

    mState = kSelectingFromPair;
    UpdateUI();
//    UpdateCaptions();

    return true;
}



bool ImageContest::SelectWinner(int leftOrRight)
{
    assert(mState == kSelectingFromPair);

    if (leftOrRight == kLeft || leftOrRight == kRight)
    {
        if (mImageMeta[kLeft]->contests++ == 0)
            mImageMeta[kLeft]->elo = 1000;

        if (mImageMeta[kRight]->contests++ == 0)
            mImageMeta[kRight]->elo = 1000;


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



        mImageMeta[kLeft]->elo += (int32_t)(fWeight * (fActualLeft - fExpectedLeft));
        mImageMeta[kRight]->elo += (int32_t)(fWeight * (fActualRight - fExpectedRight));

        ZOUT("Image:", mImageMeta[leftOrRight]->filename, " wins/contests:", mImageMeta[leftOrRight]->wins, "/", mImageMeta[leftOrRight]->contests, " ELO:", mImageMeta[leftOrRight]->elo);

        gImageMeta.SaveBucket(mImageMeta[kLeft]->filesize);
        gImageMeta.SaveBucket(mImageMeta[kRight]->filesize);



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
    if (!mpSurface)
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpSurface.get()->GetMutex());

    if (!mbVisible)
        return false;

    if (!mbInvalid)
        return false;


    return ZWin::Paint();
}
