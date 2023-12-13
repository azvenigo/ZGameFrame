#include "ZWinControlPanel.h"
#include "ZWinBtn.H"
#include "ZWinText.H"
#include "ZWinSlider.h"
#include "Resources.h"
#include "ZInput.h"

using namespace std;

ZWinControlPanel::ZWinControlPanel() : mbHideOnMouseExit(false), mStyle(gDefaultDialogStyle), mGroupingStyle(gDefaultGroupingStyle)
{
    if (msWinName.empty())
        msWinName = "ZWinControlPanel_" + gMessageSystem.GenerateUniqueTargetName();
}


bool ZWinControlPanel::Init()
{
    if (mbInitted)
        return true;

    if (mAreaInParent.Width() > 0 && mAreaInParent.Height() > 0)
        mrNextControl.SetRect(gnControlPanelEdge, gnControlPanelEdge, mAreaInParent.Width()- gnControlPanelEdge, gnControlPanelEdge + gnControlPanelButtonHeight);  


    mbAcceptsCursorMessages = true;

    mIdleSleepMS = 250;
    return ZWin::Init();
}

void ZWinControlPanel::FitToControls()
{
    ZRect rFit;
    for (auto child : mChildList)
    {
        rFit.UnionRect(child->GetArea());
    }

    rFit.InflateRect(gnControlPanelEdge, gnControlPanelEdge);
    rFit.MoveRect(mAreaAbsolute.left, mAreaAbsolute.top);

    SetArea(rFit);
}





ZWinSizablePushBtn* ZWinControlPanel::Button(const std::string& sID, const string& sCaption, const std::string& sMessage)
{
//    ZASSERT(mbInitted);
    ZWinSizablePushBtn* pBtn = mButtons[sID];
    if (!pBtn)
    {
        pBtn = new ZWinSizablePushBtn();
        mButtons[sID] = pBtn;
        ChildAdd(pBtn);

        if (mrNextControl.Area() > 0)
        {
            pBtn->SetArea(mrNextControl);
            mrNextControl.OffsetRect(0, mrNextControl.Height());
        }
    }

    if (!sCaption.empty())
        pBtn->mCaption.sText = sCaption;
    if (!sMessage.empty())
        pBtn->SetMessage(sMessage);


    return pBtn;
}

ZWinSizablePushBtn* ZWinControlPanel::SVGButton(const std::string& sID, const string& sSVGFilepath, const std::string& sMessage)
{
    ZWinSizablePushBtn* pBtn = mButtons[sID];
    if (!pBtn)
    {
        pBtn = new ZWinSizablePushBtn();
        mButtons[sID] = pBtn;
        ChildAdd(pBtn);

        if (mrNextControl.Area() > 0)
        {
            pBtn->SetArea(mrNextControl);
            mrNextControl.OffsetRect(0, mrNextControl.Height());
        }
    }

    if (!sSVGFilepath.empty())
        pBtn->mSVGImage.Load(sSVGFilepath);
    if (!sMessage.empty())
        pBtn->SetMessage(sMessage);


    return pBtn;
}



ZWinLabel* ZWinControlPanel::Caption(const std::string& sID, const std::string& sCaption)
{
    ZWinLabel* pLabel = mLabels[sID];
    if (!pLabel)
    {
        pLabel = new ZWinLabel();
        mLabels[sID] = pLabel;
        ChildAdd(pLabel);
        if (mrNextControl.Area() > 0)
        {
            pLabel->SetArea(mrNextControl);
            mrNextControl.OffsetRect(0, mrNextControl.Height());
        }
    }

    if (!sCaption.empty())
        pLabel->msText = sCaption;

    return pLabel;
}


ZWinCheck* ZWinControlPanel::Toggle(const std::string& sID, bool* pbValue, const string& sCaption, const string& sCheckMessage, const string& sUncheckMessage)
{
    ZWinCheck* pCheck = mChecks[sID];
    if (!pCheck)
    {
        pCheck = new ZWinCheck(pbValue);
        mChecks[sID] = pCheck;
        if (!sCheckMessage.empty() || !sUncheckMessage.empty())
            pCheck->SetMessages(sCheckMessage, sUncheckMessage);
        if (!sCaption.empty())
            pCheck->mCaption.sText = sCaption;

        if (mrNextControl.Area() > 0)
        {
            pCheck->SetArea(mrNextControl);
            mrNextControl.OffsetRect(0, mrNextControl.Height());
        }
        ChildAdd(pCheck);

    }
    return pCheck;
}


ZWinSlider* ZWinControlPanel::Slider(const std::string& sID, int64_t* pnSliderValue, int64_t nMin, int64_t nMax, int64_t nMultiplier, double fThumbSizeRatio, const string& sMessage, bool bDrawValue, bool bMouseOnlyDrawValue)
{
    ZWinSlider* pSlider = mSliders[sID];
    if (!pSlider)
    {
        pSlider = new ZWinSlider(pnSliderValue);
        mSliders[sID] = pSlider;

        pSlider->SetSliderRange(nMin, nMax, nMultiplier, fThumbSizeRatio);

        if (bDrawValue && bMouseOnlyDrawValue)
            pSlider->mBehavior |= ZWinSlider::kDrawSliderValueOnMouseOver;
        else if (bDrawValue)
            pSlider->mBehavior |= ZWinSlider::kDrawSliderValueAlways;
        ChildAdd(pSlider);
        if (mrNextControl.Area() > 0)
        {
            pSlider->SetArea(mrNextControl);
            mrNextControl.OffsetRect(0, mrNextControl.Height());
        }
    }


    if (!sMessage.empty())
        pSlider->SetSliderSetMessage(sMessage);

    return pSlider;
}

bool ZWinControlPanel::Paint()
{
    if (!PrePaintCheck())
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

    mpSurface->Fill(mStyle.bgCol);

    tGroupNameToWinList groups = GetChildGroups();
    for (auto& group : groups)
    {
        // draw embossed rect
        ZRect rBounds(GetBounds(group.second));
        rBounds.InflateRect(gSpacer, gSpacer);
        rBounds.right--;
        rBounds.bottom--;

        mpSurface->DrawRectAlpha(0x88000000, rBounds);
        rBounds.OffsetRect(1, 1);
        mpSurface->DrawRectAlpha(0x88ffffff, rBounds);
        
        mGroupingStyle.Font()->DrawTextParagraph(mpSurface.get(), group.first, rBounds, &mGroupingStyle);



        // if caption set, draw that

    }

    return ZWin::Paint();
}

bool ZWinControlPanel::OnMouseOut()
{
    mWorkToDoCV.notify_one();
    return ZWin::OnMouseOut();
}


bool ZWinControlPanel::Process()
{
    if (gInput.captureWin == nullptr)   // only process this if no window has capture
    {
        if (!mbVisible)
        {
            if (mrTrigger.PtInRect(gInput.lastMouseMove))
            {
                SetVisible();
                return true;
            }
        }
        else
        {
            ZRect rOverArea(mAreaAbsolute);
            if (mrTrigger.Width() > 0 && mrTrigger.Height() > 0)
                rOverArea.UnionRect(mrTrigger);

            if (!rOverArea.PtInRect(gInput.lastMouseMove) && mbHideOnMouseExit)        // if the mouse is outside of our area we hide. (Can't use OnMouseOut because we get called when the mouse goes over one of our controls)
            {
                SetVisible(false);
                if (mpParentWin)
                    mpParentWin->InvalidateChildren();
                return true;
            }
        }
    }

    return ZWin::Process();
}


