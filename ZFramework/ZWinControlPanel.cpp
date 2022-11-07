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


bool ZWinControlPanel::AddButton(const string& sCaption, const string& sMessage, uint32_t nFont, uint32_t nFontCol1, uint32_t nFontCol2, ZFont::eStyle style)
{
    ZWinSizablePushBtn* pBtn;

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage.get(), gStandardButtonDownEdgeImage.get(), grStandardButtonEdge);
    pBtn->SetCaption(sCaption);
    pBtn->SetFontID(nFont);
    pBtn->SetColor(nFontCol1);
    pBtn->SetColor2(nFontCol2);
    pBtn->SetStyle(style);

    pBtn->SetArea(mrNextControl);
    pBtn->SetMessage(sMessage);
    ChildAdd(pBtn);

    mrNextControl.OffsetRect(0,mrNextControl.Height());

    return true;
}

bool ZWinControlPanel::AddCaption(const string& sCaption, uint32_t nFont, uint32_t nFontCol, uint32_t nFillCol, ZFont::eStyle style, ZFont::ePosition pos)
{
    ZFormattedTextWin* pWin = new ZFormattedTextWin();
    pWin->SetArea(mrNextControl.left, mrNextControl.top, mrNextControl.right, mrNextControl.bottom);
    pWin->AddTextLine(sCaption, nFont, nFontCol, nFontCol, style, pos);
    pWin->SetFill(nFillCol);
    ChildAdd(pWin);

    mrNextControl.OffsetRect(0, mrNextControl.Height());

    return true;
}


bool ZWinControlPanel::AddToggle(bool* pbValue, const string& sCaption, const string& sCheckMessage, const string& sUncheckMessage, uint32_t nFont, uint32_t nFontCol, ZFont::eStyle style)
{
    ZWinCheck* pCheck = new ZWinCheck(pbValue);
    pCheck->SetMessages(sCheckMessage, sUncheckMessage);
    pCheck->SetImages(gStandardButtonUpEdgeImage.get(), gStandardButtonDownEdgeImage.get(), grStandardButtonEdge);
    pCheck->SetCaption(sCaption);
    pCheck->SetArea(mrNextControl);
    pCheck->SetFontID(nFont);
    pCheck->SetColor(nFontCol);
    pCheck->SetColor2(nFontCol);
    pCheck->SetStyle(style);
    ChildAdd(pCheck);

    mrNextControl.OffsetRect(0, mrNextControl.Height());
    return true;
}


bool ZWinControlPanel::AddSlider(int64_t* pnSliderValue, int64_t nMin, int64_t nMax, int64_t nMultiplier, const string& sMessage, bool bDrawValue, bool bMouseOnlyDrawValue, int32_t nFontID)
{
    ZSliderWin* pSlider = new ZSliderWin(pnSliderValue);
    pSlider->SetArea(mrNextControl);
    pSlider->Init(gSliderThumb.get(), grSliderThumbEdge, gSliderBackground.get(), grSliderBgEdge, ZSliderWin::kHorizontal);
    pSlider->SetSliderRange(nMin, nMax, nMultiplier);
    pSlider->SetSliderSetMessage(sMessage);
    pSlider->SetDrawSliderValueFlag(bDrawValue, bMouseOnlyDrawValue, nFontID);
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
            if (!mAreaAbsolute.PtInRect(gLastMouseMove))        // if the mouse is outside of our area we hide. (Can't use OnMouseOut because we get called when the mouse goes over one of our controls)
            {
                Hide();
                return true;
            }
        }
    }

    return ZWin::Process();
}


