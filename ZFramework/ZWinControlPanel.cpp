#include "ZWinControlPanel.h"
#include "ZWinBtn.H"
#include "ZWinText.H"
#include "ZWinSlider.h"
#include "ZWinFormattedText.h"
#include "Resources.h"
#include "ZInput.h"

using namespace std;

ZWinControlPanel::ZWinControlPanel() : mbHideOnMouseExit(false), mStyle(gDefaultDialogStyle), mGroupingStyle(gDefaultGroupingStyle)
{
}


bool ZWinControlPanel::Init()
{
    if (mbInitted)
        return true;

    assert(mArea.Width() > 0 && mArea.Height() > 0);

    mrNextControl.SetRect(gnControlPanelEdge, gnControlPanelEdge, mArea.Width()- gnControlPanelEdge, gnControlPanelEdge + gnControlPanelButtonHeight);


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


ZWinSizablePushBtn* ZWinControlPanel::AddButton(const string& sCaption, const std::string& sMessage)
{
    ZASSERT(mbInitted);

    ZWinSizablePushBtn* pBtn;

    pBtn = new ZWinSizablePushBtn();
    pBtn->mCaption.sText = sCaption;
    pBtn->SetMessage(sMessage);
    pBtn->SetArea(mrNextControl);
    ChildAdd(pBtn);

    mrNextControl.OffsetRect(0,mrNextControl.Height());

    return pBtn;
}

ZWinLabel* ZWinControlPanel::AddCaption(const std::string& sCaption)
{
    ZWinLabel* pWin = new ZWinLabel();
    pWin->msText = sCaption;
    pWin->SetArea(mrNextControl);
    ChildAdd(pWin);

    mrNextControl.OffsetRect(0, mrNextControl.Height());

    return pWin;
}


ZWinCheck* ZWinControlPanel::AddToggle(bool* pbValue, const string& sCaption, const string& sCheckMessage, const string& sUncheckMessage)
{
    ZASSERT(mbInitted);

    ZWinCheck* pCheck = new ZWinCheck(pbValue);
    pCheck->SetMessages(sCheckMessage, sUncheckMessage);
    pCheck->mCaption.sText = sCaption;
    pCheck->SetArea(mrNextControl);
    ChildAdd(pCheck);

    mrNextControl.OffsetRect(0, mrNextControl.Height());
    return pCheck;
}


ZWinSlider* ZWinControlPanel::AddSlider(int64_t* pnSliderValue, int64_t nMin, int64_t nMax, int64_t nMultiplier, double fThumbSizeRatio, const string& sMessage, bool bDrawValue, bool bMouseOnlyDrawValue)
{
    ZWinSlider* pSlider = new ZWinSlider(pnSliderValue);
    pSlider->SetArea(mrNextControl);

    if (bDrawValue && bMouseOnlyDrawValue)
        pSlider->mBehavior |= ZWinSlider::kDrawSliderValueOnMouseOver;
    else if (bDrawValue)
        pSlider->mBehavior |= ZWinSlider::kDrawSliderValueAlways;

    pSlider->Init();
    pSlider->SetSliderRange(nMin, nMax, nMultiplier, fThumbSizeRatio);
    pSlider->SetSliderSetMessage(sMessage);
    ChildAdd(pSlider);

    mrNextControl.OffsetRect(0, mrNextControl.Height());

    return pSlider;
}

bool ZWinControlPanel::Paint()
{
    if (!mbVisible)
        return false;

    if (!mbInvalid)
        return false;

    if (!mpTransformTexture.get())
        return false;


    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());


    mpTransformTexture->Fill(mAreaToDrawTo, mStyle.bgCol);

    tGroupNameToWinList groups = GetChildGroups();
    for (auto& group : groups)
    {
        // draw embossed rect
        ZRect rBounds(GetBounds(group.second));
        rBounds.InflateRect(gnDefaultGroupInlaySize, gnDefaultGroupInlaySize);
        rBounds.right--;
        rBounds.bottom--;

        mpTransformTexture->DrawRectAlpha(rBounds, 0x88000000);
        rBounds.OffsetRect(1, 1);
        mpTransformTexture->DrawRectAlpha(rBounds, 0x88ffffff);
        
        mGroupingStyle.Font()->DrawTextParagraph(mpTransformTexture.get(), group.first, rBounds, &mGroupingStyle);



        // if caption set, draw that

    }

    return ZWin::Paint();
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
            ZRect rOverArea(mrTrigger);
            rOverArea.UnionRect(mAreaAbsolute);

            if (!rOverArea.PtInRect(gInput.lastMouseMove) && mbHideOnMouseExit)        // if the mouse is outside of our area we hide. (Can't use OnMouseOut because we get called when the mouse goes over one of our controls)
            {
                SetVisible(false);
                return true;
            }
        }
    }

    return ZWin::Process();
}


