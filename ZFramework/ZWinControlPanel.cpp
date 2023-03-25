#include "ZWinControlPanel.h"
#include "ZWinBtn.H"
#include "ZWinText.H"
#include "ZWinSlider.h"
#include "ZFormattedTextWin.h"
#include "Resources.h"

extern ZPoint gLastMouseMove;
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


ZWinSlider* ZWinControlPanel::AddSlider(int64_t* pnSliderValue, int64_t nMin, int64_t nMax, int64_t nMultiplier, const string& sMessage, bool bDrawValue, bool bMouseOnlyDrawValue, tZFontPtr pFont)
{
    ZWinSlider* pSlider = new ZWinSlider(pnSliderValue);
    pSlider->SetArea(mrNextControl);
    pSlider->SetDrawSliderValueFlag(bDrawValue, bMouseOnlyDrawValue, pFont);
    pSlider->Init(gSliderThumbHorizontal, grSliderThumbEdge, gSliderBackground, grSliderBgEdge, ZWinSlider::kHorizontal);
    pSlider->SetSliderRange(nMin, nMax, nMultiplier);
    pSlider->SetSliderSetMessage(sMessage);
    ChildAdd(pSlider);

    mrNextControl.OffsetRect(0, mrNextControl.Height());

    return pSlider;
}

bool ZWinControlPanel::Paint()
{
    if (!mbVisible)
        return false;

    mpTransformTexture->Fill(mAreaToDrawTo, gDefaultDialogFill);

    return ZWin::Paint();
}

bool ZWinControlPanel::Process()
{
    if (mbHideOnMouseExit && gpCaptureWin == nullptr)   // only process this if no window has capture
    {
        if (!mbVisible)
        {
            if (mrTrigger.PtInRect(gLastMouseMove))
            {
                SetVisible();
                return true;
            }
        }
        else
        {
            ZRect rOverArea(mrTrigger);
            rOverArea.UnionRect(mAreaAbsolute);

            if (!rOverArea.PtInRect(gLastMouseMove))        // if the mouse is outside of our area we hide. (Can't use OnMouseOut because we get called when the mouse goes over one of our controls)
            {
                SetVisible(false);
                return true;
            }
        }
    }

    return ZWin::Process();
}


