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

        ZRect rLabel = gStyleCaption.Font()->Arrange(mAreaLocal, (uint8_t*)pLabel->msText.c_str(), pLabel->msText.length(), ZGUI::CT, gSpacer);

        pLabel->SetArea(rLabel);
        pLabel->mStyle = gStyleCaption;
        pLabel->mStyle.bgCol = 0;
        pLabel->mStyle.pos = ZGUI::C;
        ChildAdd(pLabel);

        string sFolder = mFiles.begin()->parent_path().string();

        rLabel.OffsetRect(0, rLabel.Height());
//        rLabel = gStyleButton.Font()->GetOutputRect(mAreaToDrawTo, (uint8_t*)sFolder.c_str(), sFolder.length(), ZGUI::LT, gDefaultSpacer);

        ZGUI::Style labelstyle(gStyleCaption);
        labelstyle.fp.fScale = 0.5;
        labelstyle.bgCol = 0;
        labelstyle.look.colTop = 0xffffff00;
        labelstyle.look.colBottom = 0xffffff00;

        pLabel = new ZWinLabel();
        pLabel->msText = "Folder: " + sFolder;
        pLabel->SetArea(rLabel);
        pLabel->mStyle = labelstyle;
        ChildAdd(pLabel);






        ZGUI::Style style(gStyleButton);
        style.fp.fScale = 1.0;
        style.paddingH = (int32_t)gSpacer;
        style.paddingV = (int32_t)gSpacer;
        style.pos = ZGUI::C;

        tWinList arrangeList;



        ZWinBtn* pBtn;
        pBtn = new ZWinBtn();
        pBtn->msButtonMessage = ZMessage("{deleteconfirm}", this);
        pBtn->mCaption.sText = "Confirm Delete";
        pBtn->mCaption.style = style;
        pBtn->mCaption.style.look.colTop = 0xffff0000;
        pBtn->mCaption.style.look.colBottom = 0xffff0000;


        ChildAdd(pBtn);
        arrangeList.push_back(pBtn);

        pBtn = new ZWinBtn();
        pBtn->msButtonMessage = ZMessage("{goback}", this);
        pBtn->mCaption.sText = "Go Back";
        pBtn->mCaption.style = style;

        ChildAdd(pBtn);
        arrangeList.push_back(pBtn);

        ZRect rButtonArea(mAreaLocal);
        rButtonArea.top = rButtonArea.bottom - gM * 2;
        ZWin::ArrangeWindows(arrangeList, rButtonArea, style, -1, 1);



        ZRect rFileList(gSpacer, gSpacer + rLabel.bottom, mAreaLocal.Width() - gSpacer, rButtonArea.top - gSpacer);

        mpFilesList = new ZWinFormattedDoc();
        mpFilesList->SetArea(rFileList);
        mpFilesList->mStyle.bgCol = gDefaultTextAreaFill;
        mpFilesList->SetBehavior(ZWinFormattedDoc::kDrawBorder|ZWinFormattedDoc::kBackgroundFill, true);
        mpFilesList->Clear();

        ZFontParams font;
        font.sFacename = "Verdana";
        font.fScale = 0.66f;

        size_t nCount = 1;
        for (auto& entry : mFiles)
        {
            string sListBoxEntry = "<line wrap=0><text color=0xff000000 color2=0xff000000 fontparams=" + SH::URL_Encode(font) + " position=lb link=select;filename=" + SH::URL_Encode(entry.string()) + ";target=ZWinImageViewer>[" + SH::FromInt(nCount) + "] " + entry.filename().string() + "</text></line>";
            nCount++;
            mpFilesList->AddLineNode(sListBoxEntry);
        }

        mpFilesList->mStyle.paddingH = (int32_t)gM;
        mpFilesList->mStyle.paddingV = (int32_t)gSpacer;
        mpFilesList->SetScrollable();
        mpFilesList->SetBehavior(ZWinFormattedDoc::kUnderlineLinks, false);

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
    if (!PrePaintCheck())
        return false;
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

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
    r = ZGUI::Arrange(r, rMain, ZGUI::LT, rMain.Height()/20, rMain.Height() / 20);

    pDialog->mBehavior |= ZWinDialog::eBehavior::Draggable;
    pDialog->mStyle = gDefaultDialogStyle;
    pDialog->SetArea(r);
    pDialog->msCaption = sCaption;
    pDialog->mFiles = fileList;
    pDialog->mTransformIn = ZWin::kSlideDown;
    pDialog->mTransformOut = ZWin::kSlideUp;

    gpMainWin->ChildAdd(pDialog, false);
    return pDialog;
}
