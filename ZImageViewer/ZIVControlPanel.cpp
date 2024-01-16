#include "ZIVControlPanel.h"
#include <string>
#include "helpers/StringHelpers.h"
#include "helpers/Registry.h"
#include "ZWinBtn.H"

using namespace std;

ZIVControlPanel::ZIVControlPanel() : mbShow(true), mMode(kSift)
{
    msWinName = "ImageViewer_CP";

}

bool ZIVControlPanel::Init()
{

    return ZWinControlPanel::Init();
}

void ZIVControlPanel::UpdateUI()
{

    int64_t nControlPanelSide = (int64_t)(gM * 2.5);
    limit<int64_t>(nControlPanelSide, 48, 88);


    int64_t nGroupSide = nControlPanelSide - gSpacer * 4;


    mGroupingStyle = gDefaultGroupingStyle;


    ZRect rPanelArea(grFullArea.left, grFullArea.top, grFullArea.right, grFullArea.top + nControlPanelSide);
    SetArea(rPanelArea);

    ZWinSizablePushBtn* pBtn;

    ZRect rButton((int64_t)(nGroupSide * 1.5), nGroupSide);
    rButton.OffsetRect(gSpacer * 2, gSpacer * 2);

    string sAppPath = gRegistry["apppath"];

    int32_t nButtonPadding = (int32_t)(rButton.Width() / 8);




    // File Group
    pBtn = SVGButton("loadimg", sAppPath + "/res/openfile.svg", ZMessage("loadimg", this));
    pBtn->msTooltip = "Load Image";
    pBtn->msWinGroup = "File";
    pBtn->mSVGImage.style.paddingH = nButtonPadding;
    pBtn->mSVGImage.style.paddingV = nButtonPadding;
    pBtn->SetArea(rButton);

    rButton.OffsetRect(rButton.Width(), 0);

    pBtn = SVGButton("gotofolder", sAppPath + "/res/gotofolder.svg", ZMessage("gotofolder", this));
    pBtn->msTooltip = "Browse folder with current image";
    pBtn->mSVGImage.style.paddingH = nButtonPadding;
    pBtn->mSVGImage.style.paddingV = nButtonPadding;
    pBtn->SetArea(rButton);
    pBtn->msWinGroup = "File";

    rButton.OffsetRect(rButton.Width(), 0);


    // Mode Group
    ZGUI::Style filterButtonStyle = gDefaultGroupingStyle;
    filterButtonStyle.look.decoration = ZGUI::ZTextLook::kEmbossed;
    filterButtonStyle.fp.nHeight = nGroupSide / 3;
    filterButtonStyle.pos = ZGUI::C;
    filterButtonStyle.wrap = true;
    filterButtonStyle.look.colTop = 0xff888888;
    filterButtonStyle.look.colBottom = 0xff888888;

    string sCaption;

    // All
    ZWinCheck* pCheck = Toggle("mode_sift", nullptr, "", ZMessage("mode_sift", this), "");
    pCheck->SetState(mFilterState == kAll, false);
    pCheck->mCheckedStyle = filterButtonStyle;
    pCheck->mCheckedStyle.look.colTop = 0xffffffff;
    pCheck->mCheckedStyle.look.colBottom = 0xffffffff;
    pCheck->mUncheckedStyle = filterButtonStyle;
    pCheck->SetArea(rButton);
    Sprintf(sCaption, "Sift\n(%d)", mImageArray.size());
    pCheck->mCaption.sText = sCaption;
    pCheck->msTooltip = "Keep or Delete";
    pCheck->msWinGroup = "Modes";
    pCheck->msRadioGroup = "ModesGroup";
    mpAllFilterButton = pCheck;


    // Favorites
    rButton.OffsetRect(rButton.Width(), 0);

    pCheck = Toggle("mode_favs", nullptr, "", ZMessage("mode_favs", this), "");
    pCheck->mbEnabled = !mFavImageArray.empty();
    pCheck->SetState(mFilterState == kFavs, false);
    pCheck->mCheckedStyle = filterButtonStyle;
    pCheck->mCheckedStyle.look.colTop = 0xffe1b131;
    pCheck->mCheckedStyle.look.colBottom = 0xffe1b131;
    pCheck->mUncheckedStyle = filterButtonStyle;
    pCheck->SetArea(rButton);
    Sprintf(sCaption, "Favorites\n(%d)", mFavImageArray.size());
    pCheck->mCaption.sText = sCaption;
    pCheck->msTooltip = "Favorites (hit '1' to toggle current image)";
    pCheck->msWinGroup = "Filter";
    pCheck->msRadioGroup = "FilterGroup";
    mpFavsFilterButton = pCheck;


    // Ranked
    rButton.OffsetRect(rButton.Width(), 0);

    pCheck = Toggle("filterranked", nullptr, "ranked", ZMessage("filter_ranked", this), "");
    pCheck->mCheckedStyle = filterButtonStyle;
    pCheck->mbEnabled = !mRankedArray.empty();
    pCheck->mCheckedStyle.look.colTop = 0xffe1b131;
    pCheck->mCheckedStyle.look.colBottom = 0xffe1b131;
    pCheck->mUncheckedStyle = filterButtonStyle;
    pCheck->SetArea(rButton);
    pCheck->msTooltip = "Images that have been ranked (button to the right)";
    pCheck->msWinGroup = "Filter";
    pCheck->msRadioGroup = "FilterGroup";
    pCheck->SetState(mFilterState == kRanked, false);
    mpRankedFilterButton = pCheck;















    // context specific Group



    if (mMode == kSift)
    {
    }
    else if (mMode == kFavor)
    {
    }
    else if (mMode == kManage)
    {
        pBtn = SVGButton("saveimg", sAppPath + "/res/save.svg", ZMessage("saveimg", this));
        pBtn->msWinGroup = "File";
        pBtn->msTooltip = "Save Image";
        pBtn->mSVGImage.style.paddingH = nButtonPadding;
        pBtn->mSVGImage.style.paddingV = nButtonPadding;
        pBtn->SetArea(rButton);
        pBtn->msWinGroup = "File";

        rButton.OffsetRect(rButton.Width(), 0);


        // Management
        rButton.OffsetRect(rButton.Width() + gSpacer * 2, 0);
        rButton.right = rButton.left + (int64_t)(rButton.Width() * 2.5);     // wider buttons for management

        pBtn = Button("undo", "undo", ZMessage("undo", this));
        pBtn->mCaption.style = gDefaultGroupingStyle;
        pBtn->mCaption.style.fp.nHeight = (int64_t)(nGroupSide / 1.5);
        pBtn->mCaption.style.pos = ZGUI::C;
        pBtn->SetArea(rButton);
        pBtn->msWinGroup = "Manage";

        rButton.OffsetRect(rButton.Width(), 0);

        pBtn = Button("move", "move", ZMessage("set_move_folder", this));
        pBtn->mCaption.style = gDefaultGroupingStyle;
        pBtn->mCaption.style.fp.nHeight = (int64_t)(nGroupSide / 1.5);
        pBtn->mCaption.style.pos = ZGUI::C;
        pBtn->SetArea(rButton);
        pBtn->msWinGroup = "Manage";

        rButton.OffsetRect(rButton.Width(), 0);

        pBtn = Button("copy", "copy", ZMessage("set_copy_folder", this));
        pBtn->mCaption.style = gDefaultGroupingStyle;
        pBtn->mCaption.style.fp.nHeight = (int64_t)(nGroupSide / 1.5);
        pBtn->mCaption.style.pos = ZGUI::C;
        pBtn->SetArea(rButton);
        pBtn->msWinGroup = "Manage";






        // Transformation (rotation, flip, etc.)
        rButton.OffsetRect(rButton.Width() + gSpacer * 4, 0);
        rButton.right = rButton.left + rButton.Height();    // square button

        pBtn = SVGButton("show_rotation_menu", sAppPath + "/res/rotate.svg");
        rButton.OffsetRect(rButton.Width() + gSpacer * 2, 0);
        rButton.right = rButton.left + (int64_t)(rButton.Width() * 1.25);     // wider buttons for management
        pBtn->SetArea(rButton);
        Sprintf(sMessage, "show_rotation_menu;target=%s;r=%s", GetTargetName().c_str(), RectToString(rButton).c_str());
        pBtn->SetMessage(sMessage);
        pBtn->msWinGroup = "Rotate";


        rButton.right = rButton.left + (int64_t)(rButton.Width() * 1.5);     // wider buttons for management

        rButton.OffsetRect(rButton.Width() + gSpacer * 2, 0);

    }



    // view Group







    string sMessage;



    // ToBeDeleted
    rButton.OffsetRect(rButton.Width(), 0);
    pCheck = Toggle("filterdel", nullptr, "", ZMessage("filter_del", this), "");
    pCheck->mbEnabled = !mToBeDeletedImageArray.empty();
    pCheck->SetState(mFilterState == kToBeDeleted, false);
    pCheck->mCheckedStyle = filterButtonStyle;
    pCheck->mCheckedStyle.look.colTop = 0xffff4444;
    pCheck->mCheckedStyle.look.colBottom = 0xffff4444;
    pCheck->mUncheckedStyle = filterButtonStyle;
    pCheck->SetArea(rButton);
    Sprintf(sCaption, "To Be Del\n(%d)", mToBeDeletedImageArray.size());
    pCheck->mCaption.sText = sCaption;
    pCheck->msTooltip = "Images marked for deletion (hit del to toggle current image)";
    pCheck->msWinGroup = "Filter";
    pCheck->msRadioGroup = "FilterGroup";
    mpDelFilterButton = pCheck;


    rButton.OffsetRect(rButton.Width() + gSpacer * 4, 0);
    rButton.right = rButton.left + rButton.Width() * 3;
    mpDeleteMarkedButton = Button("show_confirm", "delete marked", ZMessage("show_confirm", this));
    mpDeleteMarkedButton->SetVisible(mFilterState == kToBeDeleted);
    mpDeleteMarkedButton->mCaption.sText = "delete marked";
    mpDeleteMarkedButton->msTooltip = "Show confirmation of images marked for deleted.";
    mpDeleteMarkedButton->mCaption.style = gDefaultGroupingStyle;
    mpDeleteMarkedButton->mCaption.style.look.colTop = 0xffff0000;
    mpDeleteMarkedButton->mCaption.style.look.colBottom = 0xffff0000;
    mpDeleteMarkedButton->mCaption.style.fp.nHeight = nGroupSide / 2;
    mpDeleteMarkedButton->mCaption.style.wrap = true;
    mpDeleteMarkedButton->mCaption.style.pos = ZGUI::C;
    mpDeleteMarkedButton->SetArea(rButton);








    ZRect rExit(nControlPanelSide, nControlPanelSide);
    rExit = ZGUI::Arrange(rExit, mAreaLocal, ZGUI::RT);
    pBtn = SVGButton("quit", sAppPath + "/res/exit.svg", ZMessage("quit", this));
    pBtn->msTooltip = "Exit";
    pBtn->SetArea(rExit);
    pBtn->SetVisible(!gGraphicSystem.mbFullScreen);


    rButton.SetRect(0, 0, nGroupSide, nGroupSide);
    rButton = ZGUI::Arrange(rButton, rPanelArea, ZGUI::RC, gSpacer / 2);
    rButton.OffsetRect(-gSpacer * 2, 0);
    rButton.OffsetRect(-nControlPanelSide, 0);


    pBtn = SVGButton("toggle_fullscreen", sAppPath + "/res/fullscreen.svg", ZMessage("toggle_fullscreen"));
    pBtn->msWinGroup = "View";
    pBtn->SetArea(rButton);
    pBtn->msTooltip = "Toggle Fullscreen";

    rButton.OffsetRect(-rButton.Width(), 0);

/*    pCheck = Toggle("qualityrender", &mbSubsample, "", ZMessage("invalidate", this), ZMessage("invalidate", this));
    pCheck->mSVGImage.Load(sAppPath + "/res/subpixel.svg");
    pCheck->SetArea(rButton);
    pCheck->msTooltip = "Quality Render";
    pCheck->msWinGroup = "View";*/


    rButton.OffsetRect(-rButton.Width(), 0);
    pBtn = Button("show_help", "?", ZMessage("show_help", this));
    pBtn->mCaption.style = filterButtonStyle;
    pBtn->mCaption.style.fp.nHeight = nGroupSide / 2;
    pBtn->SetArea(rButton);
    pBtn->msTooltip = "View Help and Quick Reference";
    pBtn->msWinGroup = "View";


    // Contest Button
    rButton.right = rButton.left + rButton.Width() * 4;
    rButton.OffsetRect(-rButton.Width() - gSpacer * 2, 0);
    mpShowContestButton = Button("show_contest", "Rank Photos", ZMessage("show_contest", this));
    mpShowContestButton->mCaption.style = gStyleButton;
    mpShowContestButton->mCaption.style.pos = ZGUI::C;
    mpShowContestButton->SetArea(rButton);
    if (mFavImageArray.size() < 5)
    {
        mpShowContestButton->msTooltip = "Choose at least five favorites to enable ranking";
        mpShowContestButton->mbEnabled = false;
    }
    else
    {
        Sprintf(mpShowContestButton->msTooltip, "Rank the %u favorites in pairs", mFavImageArray.size());
        mpShowContestButton->mbEnabled = true;
    }


    string sCurVersion;
    string sAvailVersion;
    gRegistry.Get("app", "version", sCurVersion);
    gRegistry.Get("app", "newversion", sAvailVersion);

#ifndef _DEBUG
    if (sCurVersion != sAvailVersion)
    {
        rButton.OffsetRect(-rButton.Width() - gSpacer * 2, 0);
        pBtn = Button("download", "New Version", ZMessage("download", this));
        pBtn->mCaption.style = gStyleButton;
        pBtn->mCaption.style.look.colTop = 0xffff0088;
        pBtn->mCaption.style.look.colTop = 0xff8800ff;
        pBtn->mCaption.style.pos = ZGUI::C;
        pBtn->msTooltip = "New version \"" + sAvailVersion + "\" is available for download.";
        pBtn->SetArea(rButton);
    }
#endif

    if (mbShow && !mbVisible)
    {
        mTransformIn = kSlideDown;
        TransformIn();
    }
    else if (!mbShow && mbVisible)
    {
        mTransformOut = kSlideUp;
        TransformOut();
    }
    else
        Invalidate();
}


bool ZIVControlPanel::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();
    if (sType == "show_ui")
    {
        mbShow = SH::ToBool(message.GetParam("show"));
        UpdateUI();
        return true;
    }

    return ZWinControlPanel::HandleMessage(message);
}

