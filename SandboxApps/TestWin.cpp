#include "ZTypes.h"
#include "ZWin.h"
#include "TestWin.h"
#include "ZWinSlider.h"
#include "Resources.h"
#include "ZWinBtn.H"
#include "ZWinPanel.h"

using namespace std;

TestWin::TestWin()
{
    mnSliderVal = 0;
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
}
   
bool TestWin::OnKeyDown(uint32_t key)
{
#ifdef _WIN64
    if (key == VK_ESCAPE)
    {
        gMessageSystem.Post("quit_app_confirmed");
    }
#endif
    else if (key == ' ')
    {
        InvalidateChildren();
    }
    return true;
}

bool TestWin::Init()
{
    SetFocus();
    ZWinSlider* pWin;
/*    pWin = new ZWinSlider(&mnSliderVal);
    pWin->SetArea(ZRect(500, 1800, 1900, 1850));
    pWin->SetSliderRange(0, 10000, 2, 0.5);
    pWin->mBehavior = ZWinSlider::kDrawSliderValueOnMouseOver;

    ChildAdd(pWin);*/

    pWin = new ZWinSlider(&mnSliderVal);
    pWin->SetArea(ZRect(2000, 100, 2060, 1150));
    pWin->SetSliderRange(10, 20000, 1, 0.2);
    pWin->mBehavior = ZWinSlider::kDrawSliderValueAlways;
    ChildAdd(pWin);


    ZWinPopupPanelBtn* pBtn = new ZWinPopupPanelBtn();
    pBtn->mPanelLayout = "<panel close_on_button=1>";
    pBtn->mPanelLayout += "<row><svgbutton id=test1 group=group1 svgpath=%apppath%/res/test.svg></svgbutton><button id=test2 group=group1 caption=hello></button></row>";
    pBtn->mPanelLayout += "<row><label id=label1 caption=\"A label\"></label></row>";
    pBtn->mPanelLayout += "<row><button id=test3 group=group1 caption=hello></button><svgbutton id=test4 group=group1 svgpath=%apppath%/res/test.svg></svgbutton></row>";
    pBtn->mPanelLayout += "</panel>";


    //panel->SetArea(ZRect(10, 200, 510, 300));
    //panel->SetRelativeArea(grFullArea, ZGUI::ePosition::IRIB, ZGUI::Arrange(ZRect(600, 800), grFullArea, ZGUI::ePosition::C, 200, 10));
    pBtn->SetArea(ZRect(100, 200, 132, 232));
    pBtn->mPanelArea = ZRect(100, 200, 500, 500);
    ChildAdd(pBtn);



    return ZWin::Init();
}
bool TestWin::Process()
{
    return ZWin::Process();
}


bool TestWin::OnParentAreaChange()
{
    if (!mpSurface)
        return false;

    SetArea(mpParentWin->GetArea());

    ZWin::OnParentAreaChange();

    return true;
}



bool TestWin::Paint()
{
    if (!PrePaintCheck())
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

    mpSurface.get()->Fill(0xff003300);

    return ZWin::Paint();
}
   
