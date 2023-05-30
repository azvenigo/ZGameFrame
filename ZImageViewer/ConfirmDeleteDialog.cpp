#pragma once

#include "ConfirmDeleteDialog.h"
#include "Resources.h"
#include "ZMainWin.h"
#include "ZWinText.H"

using namespace std;

ConfirmDeleteDialog::ConfirmDeleteDialog()
{
    msWinName = "ConfirmDeleteDialog";
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mpFilesList = nullptr;
}

bool ConfirmDeleteDialog::Init()
{
    if (!mbInitted)
    {
        ZWinLabel* pLabel = new ZWinLabel();
        pLabel->msText = msCaption;

        ZRect rLabel = gStyleButton.Font()->GetOutputRect(mAreaToDrawTo, (uint8_t*)pLabel->msText.c_str(), pLabel->msText.length(), ZGUI::LT, gDefaultSpacer);

        pLabel->SetArea(rLabel);
        pLabel->mStyle = gStyleButton;
        ChildAdd(pLabel);

        ZRect rFileList(gDefaultSpacer, gDefaultSpacer + rLabel.bottom, mAreaToDrawTo.Width() - gDefaultSpacer, mAreaToDrawTo.bottom - gDefaultSpacer*10);

        mpFilesList = new ZWinFormattedText();
        mpFilesList->SetArea(rFileList);
        mpFilesList->SetFill(gDefaultTextAreaFill);
        mpFilesList->SetDrawBorder();

        mpFilesList->Clear();

        ZFontParams font;
        font.sFacename = "Verdana";
        font.nHeight = 30;

        size_t nCount = 0;
        for (auto& entry : mFiles)
        {
            string sListBoxEntry = "<line wrap=0><text color=0xff000000 color2=0xff000000 fontparams=" + SH::URL_Encode(font) + " position=lb>" + entry.string() + "</text></line>";
            nCount++;
            mpFilesList->AddLineNode(sListBoxEntry);
        }

        ChildAdd(mpFilesList);


        int nButtonWidth = (rFileList.Width() - gDefaultSpacer * 2) / 3;

        ZRect rButton(0, 0, nButtonWidth, gDefaultSpacer * 3);
        rButton = ZGUI::Arrange(rButton, mAreaToDrawTo, ZGUI::RB, gDefaultSpacer);


        ZWinSizablePushBtn* pBtn;

        pBtn = new ZWinSizablePushBtn();
        pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
        pBtn->SetCaption("Quit");
        pBtn->SetMessage(ZMessage("cancel", this));
        pBtn->mStyle = gStyleGeneralText;
        pBtn->SetArea(rButton);
        ChildAdd(pBtn);

        rButton.OffsetRect(-rButton.Width() - gDefaultSpacer, 0);


        pBtn = new ZWinSizablePushBtn();
        pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
        pBtn->SetCaption("Delete");
        pBtn->SetMessage(ZMessage("deleteconfirm", this));
        pBtn->mStyle = gStyleButton;
        pBtn->mStyle.look.colTop = 0xffff0000;
        pBtn->mStyle.look.colBottom = 0xffff0000;


        pBtn->SetArea(rButton);
        ChildAdd(pBtn);

        rButton.OffsetRect(-rButton.Width() - gDefaultSpacer, 0);


        pBtn = new ZWinSizablePushBtn();
        pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
        pBtn->SetCaption("Go Back");
        pBtn->SetMessage(ZMessage("goback", this));
        pBtn->mStyle = gStyleButton;

        pBtn->SetArea(rButton);
        ChildAdd(pBtn);
    }

    SetFocus();

    return ZWinDialog::Init();
}

bool  ConfirmDeleteDialog::OnKeyDown(uint32_t c)
{
    switch (c)
    {
    case VK_ESCAPE:
        OnGoBack();
        break;
    };

    return ZWin::OnKeyDown(c);
}


bool ConfirmDeleteDialog::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();

    if (sType == "cancel")
    {
        OnCancel();
        return true;
    }
    else if (sType == "deleteconfirm")
    {
        OnConfirmDelete();
        return true;
    }
    else if (sType == "goback")
    {
        OnGoBack();
        return true;
    }

    return ZWin::HandleMessage(message);
}

void ConfirmDeleteDialog::OnConfirmDelete()
{
    gMessageSystem.Post(msOnConfirmDelete);
    gMessageSystem.Post("kill_window", "name", msWinName);
}


void ConfirmDeleteDialog::OnGoBack()
{
    gMessageSystem.Post(msOnGoBack);
    gMessageSystem.Post("kill_window", "name", msWinName);
}


void ConfirmDeleteDialog::OnCancel()
{
    gMessageSystem.Post(msOnCancel);
    gMessageSystem.Post("kill_window", "name", msWinName);
}

bool ConfirmDeleteDialog::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return false;

    ZWinDialog::Paint();

    return ZWin::Paint();
}


ConfirmDeleteDialog* ConfirmDeleteDialog::ShowDialog(const std::string& sCaption, std::list<std::filesystem::path> fileList)
{
    ConfirmDeleteDialog* pDialog = new ConfirmDeleteDialog();

    ZRect rMain(gpMainWin->GetArea());

    ZRect r(0, 0, rMain.Width() / 4, rMain.Height() / 2.5);
    r = ZGUI::Arrange(r, rMain, ZGUI::C);

    pDialog->mBehavior |= ZWinDialog::eBehavior::Draggable;
    pDialog->mStyle = gDefaultDialogStyle;
    pDialog->SetArea(r);
    pDialog->msCaption = sCaption;
    pDialog->mFiles = fileList;

    gpMainWin->ChildAdd(pDialog);
    return pDialog;
}
