#include "ZWinControlPanel.h"
#include "ZWinBtn.H"
#include "ZSliderWin.h"
#include "ZFormattedTextWin.h"
#include "Resources.h"

extern ZPoint gLastMouseMove;
using namespace std;

bool ZWinControlPanel::Init()
{
    assert(mArea.Width() > 0 && mArea.Height() > 0);

    mrNextControl.SetRect(gnControlPanelEdge, gnControlPanelEdge, mArea.Width()- gnControlPanelEdge, gnControlPanelEdge + gnControlPanelButtonHeight);


    mbAcceptsCursorMessages = true;

    mIdleSleepMS = 100;
    return ZWin::Init();
}


bool ZWinControlPanel::AddButton(const string& sCaption, const string& sMessage, tZFontPtr pFont, uint32_t nFontColTop, uint32_t nFontColBottom, ZFont::eStyle style)
{
    ZWinSizablePushBtn* pBtn;

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption(sCaption);
    pBtn->SetFont(pFont);
    pBtn->SetColors(nFontColTop, nFontColBottom);
    pBtn->SetStyle(style);

    pBtn->SetArea(mrNextControl);
    pBtn->SetMessage(sMessage);
    ChildAdd(pBtn);

    mrNextControl.OffsetRect(0,mrNextControl.Height());

    return true;
}

bool ZWinControlPanel::AddCaption(const string& sCaption, ZFontParams& fontParams, uint32_t nFontCol, uint32_t nFillCol, ZFont::eStyle style, ZFont::ePosition pos)
{
    ZFormattedTextWin* pWin = new ZFormattedTextWin();
    pWin->SetArea(mrNextControl);
    pWin->AddTextLine(sCaption, fontParams, nFontCol, nFontCol, style, pos);
    pWin->SetFill(nFillCol);
    ChildAdd(pWin);

    mrNextControl.OffsetRect(0, mrNextControl.Height());

    return true;
}


bool ZWinControlPanel::AddToggle(bool* pbValue, const string& sCaption, const string& sCheckMessage, const string& sUncheckMessage, const std::string& sRadioGroup, tZFontPtr pFont, uint32_t nFontColUnchecked, uint32_t nFontColChecked, ZFont::eStyle style)
{
    ZWinCheck* pCheck = new ZWinCheck(pbValue);
    pCheck->SetMessages(sCheckMessage, sUncheckMessage);
    pCheck->SetRadioGroup(sRadioGroup);
    pCheck->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pCheck->SetCaption(sCaption);
    pCheck->SetArea(mrNextControl);
    pCheck->SetFont(pFont);
    pCheck->SetCheckedColors(nFontColUnchecked, nFontColChecked);
    pCheck->SetStyle(style);
    ChildAdd(pCheck);

    mrNextControl.OffsetRect(0, mrNextControl.Height());
    return true;
}


bool ZWinControlPanel::AddSlider(int64_t* pnSliderValue, int64_t nMin, int64_t nMax, int64_t nMultiplier, const string& sMessage, bool bDrawValue, bool bMouseOnlyDrawValue, tZFontPtr pFont)
{
    ZSliderWin* pSlider = new ZSliderWin(pnSliderValue);
    pSlider->SetArea(mrNextControl);
    pSlider->SetDrawSliderValueFlag(bDrawValue, bMouseOnlyDrawValue, pFont);
    pSlider->Init(gSliderThumbHorizontal, grSliderThumbEdge, gSliderBackground, grSliderBgEdge, ZSliderWin::kHorizontal);
    pSlider->SetSliderRange(nMin, nMax, nMultiplier);
    pSlider->SetSliderSetMessage(sMessage);
    ChildAdd(pSlider);

    mrNextControl.OffsetRect(0, mrNextControl.Height());

    return true;
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
                Show();
                return true;
            }
        }
        else
        {
            ZRect rOverArea(mrTrigger);
            rOverArea.UnionRect(mAreaAbsolute);

            if (!rOverArea.PtInRect(gLastMouseMove))        // if the mouse is outside of our area we hide. (Can't use OnMouseOut because we get called when the mouse goes over one of our controls)
            {
                Hide();
                return true;
            }
        }
    }

    return ZWin::Process();
}


