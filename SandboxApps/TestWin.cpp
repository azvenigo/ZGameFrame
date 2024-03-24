#include "ZTypes.h"
#include "ZWin.h"
#include "TestWin.h"
#include "ZWinSlider.h"
#include "Resources.h"
#include "ZWinBtn.H"
#include "ZWinPanel.h"
#include "ZWinFormattedText.h"

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
        gMessageSystem.Post("{quit_app_confirmed}");
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
    pWin->SetArea(ZRect(100, 100, 2000, 150));
    pWin->SetSliderRange(10, 20000, 1, 0.2);
    pWin->mBehavior = ZWinSlider::kDrawSliderValueAlways;
    ChildAdd(pWin);
    

    ZGUI::Style style(gDefaultPanelStyle);
    style.pos = ZGUI::CT;
    style.paddingH = 0;
    style.paddingV = 0;

    ZGUI::RA_Descriptor rad(ZRect(0, 0, grFullArea.right, gM * 2), grFullArea, ZGUI::ICIT);


//    ZGUI::RA_Descriptor subpanelRad(ZRect(0, 0, gM * 2, gM*4), grFullArea, ZGUI::ILIC);

    std::string subPanelLayout = "<panel hide_on_mouse_exit=1 show_init=1 style=" + SH::URL_Encode((string)style) + ">";
    subPanelLayout += "<row><svgbutton id=subtest1 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/flipH.svg\"></svgbutton></row>";
    subPanelLayout += "<row><svgbutton id=subtest2 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/flipV.svg\"></svgbutton></row>";
    subPanelLayout += "</panel>";



    ZWinPanel* pPanel = new ZWinPanel();
    pPanel->mPanelLayout = "<panel close_on_button=1 show_init=1 style=\"" + SH::URL_Encode((string)style) + "\" rel_area_desc=\"" + SH::URL_Encode((string)rad) + "\"><row>";

    pPanel->mPanelLayout += "<panelbutton id=test1 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/openfile.svg\" layout=" + SH::URL_Encode(subPanelLayout) + " pos=ICOB scale=" + FPointToString(ZFPoint(1.0,4.0)) + "></panelbutton>";
    pPanel->mPanelLayout += "<svgbutton id=test2 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/copy.svg\"></svgbutton>";
    pPanel->mPanelLayout += "<svgbutton id=test3 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/exit.svg\"></svgbutton>";
    pPanel->mPanelLayout += "<svgbutton id=test4 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/move.svg\"></svgbutton>";
    pPanel->mPanelLayout += "<svgbutton id=test5 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/flipH.svg\"></svgbutton>";
    pPanel->mPanelLayout += "<svgbutton id=test6 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/flipV.svg\"></svgbutton>";
    pPanel->mPanelLayout += "<svgbutton id=test7 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/return.svg\"></svgbutton>";
    pPanel->mPanelLayout += "<svgbutton id=test8 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/fullscreen.svg\"></svgbutton>";
    //    pPanel->mPanelLayout += "<button id=test8 group=group1 caption=hello></button>";
    pPanel->mPanelLayout += "</row>";
//    sPanelLayout += "<row><svgbutton id=test1 group=group1 svgpath=%apppath%/res/test.svg></svgbutton><button id=test2 group=group1 caption=hello></button></row>";
//    sPanelLayout += "<row><label id=label1 caption=\"A label\"></label></row>";
//    sPanelLayout += "<row><button id=test9 group=group1 caption=hello></button><svgbutton id=test10 group=group1 svgpath=%apppath%/res/test.svg></svgbutton></row>";
    pPanel->mPanelLayout += "</panel>";
    ChildAdd(pPanel);
    
/*    ZWinFormattedDoc* pForm = new ZWinFormattedDoc();
    pForm->SetArea(ZRect(1010, 500, 1610, 2000));
    pForm->mStyle = gDefaultFormattedDocStyle;
    pForm->SetBehavior(ZWinFormattedDoc::kBackgroundFill | ZWinFormattedDoc::kScrollable | ZWinFormattedDoc::kDrawBorder, true);
    pForm->AddMultiLine("Test text\nMore text\nStill more and more and more and more text...\nHow much is there\n");

    for (int i = 0; i < 100; i++)
        pForm->AddMultiLine(std::format("This is line number {}", i));

    ChildAdd(pForm);*/



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
   
