#include "ZWinControlPanel.h"
#include "ZWinBtn.H"
#include "ZWinText.H"
#include "ZWinSlider.h"
#include "ZWinFormattedText.h"
#include "Resources.h"
#include "ZInput.h"

using namespace std;

bool ZWinControlPanel::Init()
{
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


ZWinSizablePushBtn* ZWinControlPanel::AddButton(const string& sCaption, const std::string& sMessage, const ZGUI::Style& style)
{
    ZASSERT(mbInitted);

    ZWinSizablePushBtn* pBtn;

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption(sCaption);
    pBtn->SetMessage(sMessage);
    pBtn->mStyle = style;
    pBtn->SetArea(mrNextControl);
    ChildAdd(pBtn);

    mrNextControl.OffsetRect(0,mrNextControl.Height());

    return pBtn;
}

ZWinLabel* ZWinControlPanel::AddCaption(const std::string& sCaption, const ZGUI::Style& style)
{
    ZASSERT(mbInitted);

    ZWinLabel* pWin = new ZWinLabel();
    pWin->msText = sCaption;
    pWin->mStyle = style;
    pWin->SetArea(mrNextControl);
    ChildAdd(pWin);

    mrNextControl.OffsetRect(0, mrNextControl.Height());

    return pWin;
}


ZWinCheck* ZWinControlPanel::AddToggle(bool* pbValue, const string& sCaption, const string& sCheckMessage, const string& sUncheckMessage, const std::string& sRadioGroup, const ZGUI::Style& checkedStyle, const ZGUI::Style& uncheckedStyle)
{
    ZASSERT(mbInitted);

    ZWinCheck* pCheck = new ZWinCheck(pbValue);
    pCheck->SetMessages(sCheckMessage, sUncheckMessage);
    pCheck->SetRadioGroup(sRadioGroup);
    pCheck->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pCheck->SetCaption(sCaption);
    pCheck->SetArea(mrNextControl);
    pCheck->mCheckedStyle = checkedStyle;
    pCheck->mUncheckedStyle = uncheckedStyle;
    ChildAdd(pCheck);

    mrNextControl.OffsetRect(0, mrNextControl.Height());
    return pCheck;
}


ZWinSlider* ZWinControlPanel::AddSlider(int64_t* pnSliderValue, int64_t nMin, int64_t nMax, int64_t nMultiplier, double fThumbSizeRatio, const string& sMessage, bool bDrawValue, bool bMouseOnlyDrawValue, tZFontPtr pFont)
{
    ZWinSlider* pSlider = new ZWinSlider(pnSliderValue);
    pSlider->SetArea(mrNextControl);

    uint32_t nBehavior = ZWinSlider::kHorizontal;
    if (bDrawValue && bMouseOnlyDrawValue)
        nBehavior |= ZWinSlider::kDrawSliderValueOnMouseOver;
    else if (bDrawValue)
        nBehavior |= ZWinSlider::kDrawSliderValueAlways;
    pSlider->SetBehavior(nBehavior, pFont);

    pSlider->Init();
    pSlider->SetSliderRange(nMin, nMax, nMultiplier, fThumbSizeRatio);
    pSlider->SetSliderSetMessage(sMessage);
    ChildAdd(pSlider);

    mrNextControl.OffsetRect(0, mrNextControl.Height());

    return pSlider;
}

void ZWinControlPanel::AddGrouping(const std::string& sCaption, const ZRect& rArea, const ZGUI::Style& groupingStyle)
{
    mGroupingBorders.push_back(GroupingBorder(sCaption, rArea, groupingStyle));
    Invalidate();
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

    for (auto group : mGroupingBorders)
    {
        // draw embossed rect
        ZRect r(group.mArea);

        mpTransformTexture->DrawRectAlpha(r, 0x88000000);
        r.OffsetRect(1, 1);
        mpTransformTexture->DrawRectAlpha(r, 0x88ffffff);

        ZRect rCaption = group.mArea;
        rCaption.InflateRect(-group.mStyle.padding, group.mStyle.padding);
        

        group.mStyle.Font()->DrawTextParagraph(mpTransformTexture.get(), group.msCaption, rCaption, group.mStyle.look, group.mStyle.pos);



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


