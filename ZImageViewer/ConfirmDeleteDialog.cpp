#pragma once

#include "ConfirmDeleteDialog.h"
#include "Resources.h"
#include "ZMainWin.h"
#include "ZWinText.H"

using namespace std;

const string kConfirmDeleteDialogName("ConfirmDeleteDialog");

ConfirmDeleteDialog::ConfirmDeleteDialog()
{
    msWinName = kConfirmDeleteDialogName;
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mpFilesList = nullptr;
}

bool ConfirmDeleteDialog::Init()
{
    if (!mbInitted)
    {
        if (mFiles.empty())
            return false;

        Paint();    // Paint dialog for labels to copy from

        ZWinLabel* pLabel = new ZWinLabel();
        pLabel->msText = msCaption;

        ZRect rLabel = gStyleCaption.Font()->Arrange(mAreaToDrawTo, (uint8_t*)pLabel->msText.c_str(), pLabel->msText.length(), ZGUI::LT, gDefaultSpacer);

        pLabel->SetArea(rLabel);
        pLabel->mStyle = gStyleCaption;
        pLabel->mStyle.bgCol = 0;
        ChildAdd(pLabel);

        string sFolder = mFiles.begin()->parent_path().string();

        rLabel.OffsetRect(0, rLabel.Height());
//        rLabel = gStyleButton.Font()->GetOutputRect(mAreaToDrawTo, (uint8_t*)sFolder.c_str(), sFolder.length(), ZGUI::LT, gDefaultSpacer);

        pLabel = new ZWinLabel();
        pLabel->msText = sFolder;
        pLabel->SetArea(rLabel);
        pLabel->mStyle = gDefaultDialogStyle;
        pLabel->mStyle.bgCol = 0;
        pLabel->mStyle.look.colTop = 0xffffff00;
        pLabel->mStyle.look.colBottom = 0xffffff00;
        ChildAdd(pLabel);


        int nButtonWidth = (mAreaToDrawTo.Width() - gDefaultSpacer * 4) / 3;

        ZRect rButton(0, 0, nButtonWidth, gDefaultSpacer * 3);
        rButton = ZGUI::Arrange(rButton, mAreaToDrawTo, ZGUI::RB, gDefaultSpacer, gDefaultSpacer);


        ZWinSizablePushBtn* pBtn;


        pBtn = new ZWinSizablePushBtn();
        pBtn->SetMessage(ZMessage("deleteconfirm", this));
        pBtn->mCaption.sText = "Confirm Delete";
        pBtn->mCaption.style = gStyleButton;
        pBtn->mCaption.style.look.colTop = 0xffff0000;
        pBtn->mCaption.style.look.colBottom = 0xffff0000;


        pBtn->SetArea(rButton);
        ChildAdd(pBtn);

        rButton = ZGUI::Arrange(rButton, mAreaToDrawTo, ZGUI::LB, gDefaultSpacer, gDefaultSpacer);

//        rButton.MoveRect(-rButton.Width()*2 - gDefaultSpacer, 0);


        pBtn = new ZWinSizablePushBtn();
        pBtn->SetMessage(ZMessage("goback", this));
        pBtn->mCaption.sText = "Go Back";
        pBtn->mCaption.style = gStyleButton;

        pBtn->SetArea(rButton);
        ChildAdd(pBtn);



        ZRect rFileList(gDefaultSpacer, gDefaultSpacer + rLabel.bottom, mAreaToDrawTo.Width() - gDefaultSpacer, rButton.top - gDefaultSpacer);

        mpFilesList = new ZWinFormattedText();
        mpFilesList->SetArea(rFileList);
        mpFilesList->SetFill(gDefaultTextAreaFill);
        mpFilesList->SetDrawBorder();

        mpFilesList->Clear();

        ZFontParams font;
        font.sFacename = "Verdana";
        font.nHeight = 30;

        size_t nCount = 1;
        for (auto& entry : mFiles)
        {
            string sListBoxEntry = "<line wrap=0><text color=0xff000000 color2=0xff000000 fontparams=" + SH::URL_Encode(font) + " position=lb link=select;filename=" + SH::URL_Encode(entry.string()) + ";target=ZWinImageViewer>[" + SH::FromInt(nCount) + "] " + entry.filename().string() + "</text></line>";
            nCount++;
            mpFilesList->AddLineNode(sListBoxEntry);
        }

        mpFilesList->SetScrollable();
        mpFilesList->SetUnderlineLinks(false);

        ChildAdd(mpFilesList);




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
    gMessageSystem.Post("kill_child", "name", msWinName);
}


void ConfirmDeleteDialog::OnGoBack()
{
    gMessageSystem.Post(msOnGoBack);
    gMessageSystem.Post("kill_child", "name", msWinName);
}


void ConfirmDeleteDialog::OnCancel()
{
    gMessageSystem.Post(msOnCancel);
    gMessageSystem.Post("kill_child", "name", msWinName);
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
    // only one dialog
    ConfirmDeleteDialog* pDialog = (ConfirmDeleteDialog*)gpMainWin->GetChildWindowByWinName(kConfirmDeleteDialogName);
    if (pDialog)
    {
        pDialog->SetVisible();
        return pDialog;
    }

    pDialog = new ConfirmDeleteDialog();

    ZRect rMain(gpMainWin->GetArea());

    int64_t nWidth = rMain.Width() / 4;
    limit<int64_t>(nWidth, 800, 2048);

    ZRect r(0, 0, nWidth, nWidth*3/4);
    r = ZGUI::Arrange(r, rMain, ZGUI::RB, rMain.Height()/20, rMain.Height() / 20);

    pDialog->mBehavior |= ZWinDialog::eBehavior::Draggable;
    pDialog->mStyle = gDefaultDialogStyle;
    pDialog->SetArea(r);
    pDialog->msCaption = sCaption;
    pDialog->mFiles = fileList;

    gpMainWin->ChildAdd(pDialog);
    return pDialog;
}
