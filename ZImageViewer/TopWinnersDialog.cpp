#pragma once

#include "TopWinnersDialog.h"
#include "Resources.h"
#include "ZMainWin.h"
#include "ZWinText.H"
#include "ZThumbCache.h"

using namespace std;

const string kTopWinnersDialogName("TopWinnersDialog");

TopWinnersDialog::TopWinnersDialog()
{
    msWinName = kTopWinnersDialogName;
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mpFilesList = nullptr;
}

bool TopWinnersDialog::Init()
{
    if (!mbInitted)
    {
        if (sortedList.empty())
            return false;

        Paint();    // Paint dialog for labels to copy from

        ZWinLabel* pLabel = new ZWinLabel();

        ZGUI::Style caption(gDefaultDialogStyle);
        caption.fp.nHeight = gM * 2.5;
        caption.fp.nWeight = 800;
        caption.fp.nTracking = gM / 8;
        caption.bgCol = 0;
        caption.look.colTop = 0xffff8800;
        caption.look.colBottom = 0xffffdd00;
        caption.look.decoration = ZGUI::ZTextLook::kEmbossed;

        pLabel->msText = "Top Winners";
        ZRect rLabel = caption.Font()->Arrange(mAreaToDrawTo, (uint8_t*)pLabel->msText.c_str(), pLabel->msText.length(), ZGUI::CT, gSpacer);

        pLabel->SetArea(rLabel);
        pLabel->mStyle = caption;
        pLabel->mStyle.bgCol = 0;
        ChildAdd(pLabel);


        ZRect rButton(0, 0, gM*3, gM*2);
        rButton = ZGUI::Arrange(rButton, mAreaToDrawTo, ZGUI::RB, gSpacer, gSpacer);

        ZRect rFileList(gSpacer, gSpacer + rLabel.bottom, mAreaToDrawTo.Width() - gSpacer, rButton.top - gSpacer);

        mpFilesList = new ZWinFormattedDoc();
        mpFilesList->SetArea(rFileList);
        mpFilesList->mDialogStyle.bgCol = gDefaultTextAreaFill;
        mpFilesList->mbDrawBorder = true;

        mpFilesList->Clear();

        ZFontParams font;
        font.sFacename = "Verdana";
        font.nHeight = gM;

        size_t nCount = 1;
        for (auto& entry : sortedList)
        {
            string sWin(" wins");
            if (entry.wins == 1)
                sWin = " win";

            string sLink("select;filename=" + SH::URL_Encode(entry.filename) + ";target=ZWinImageContest");
            string sListBoxEntry = "<line wrap=0><img link=" + sLink + ">" + gThumbCache.ThumbPath(entry.filename).string() + "</img><text color=0xffffdd00 color2=0xffffdd00 fontparams=" + SH::URL_Encode(font) + " position=lb link=" + sLink + "> [ELO:" + SH::FromInt(entry.elo) + " " + SH::FromInt(entry.wins) + sWin + "] " + std::filesystem::path(entry.filename).filename().string() + "</text></line>";
            nCount++;
            mpFilesList->mbEvenColumns = true;
            mpFilesList->AddLineNode(sListBoxEntry);


        }

        mpFilesList->mbScrollable = true;
        mpFilesList->mDialogStyle.paddingH = gM;
        mpFilesList->mDialogStyle.paddingV = gSpacer;
        mpFilesList->mbUnderlineLinks = false;

        ChildAdd(mpFilesList);
    }

    mBehavior |= eBehavior::OKButton;

    SetFocus();

    return ZWinDialog::Init();
}

bool  TopWinnersDialog::OnKeyDown(uint32_t c)
{
    switch (c)
    {
    case VK_ESCAPE:
        OnOK();
        break;
    };

    return ZWin::OnKeyDown(c);
}


bool TopWinnersDialog::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();

    if (sType == "dlg_ok")
    {
        OnOK();
        return true;
    }

    return ZWin::HandleMessage(message);
}


void TopWinnersDialog::OnOK()
{
    if (mpParentWin)
        mpParentWin->SetFocus();
    gMessageSystem.Post("kill_child", "name", msWinName);
}

bool TopWinnersDialog::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return false;

    ZWinDialog::Paint();

    return ZWin::Paint();
}


TopWinnersDialog* TopWinnersDialog::ShowDialog(tImageMetaList sortedList, ZRect rDialogArea)
{
    ZWin* pContestWin = (ZWin*)gpMainWin->GetChildWindowByWinName("ZWinImageContest");
    if (!pContestWin)
    {
        ZERROR("Something terribly wrong. Couldn't retrieve contest dialog");
        return nullptr;
    }

    // only one dialog
    TopWinnersDialog* pDialog = (TopWinnersDialog*)pContestWin->GetChildWindowByWinName(kTopWinnersDialogName);
    if (pDialog)
    {
        pDialog->SetVisible();
        return pDialog;
    }

    pDialog = new TopWinnersDialog();

    pDialog->mStyle = gDefaultDialogStyle;
    pDialog->SetArea(rDialogArea);
    pDialog->sortedList = sortedList;

    pContestWin->ChildAdd(pDialog);
    return pDialog;
}