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
    msWinName = "TestWin";
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
    style.pad.h = 0;
    style.pad.v = 0;

    ZGUI::RA_Descriptor rad(ZRect(0, 0, grFullArea.right, gM * 2), msWinName, ZGUI::ICIT, 0.5f, 0.1f);


//    ZGUI::RA_Descriptor subpanelRad(ZRect(0, 0, gM * 2, gM*4), grFullArea, ZGUI::ILIC);

    std::string subPanelLayout = "<panel hide_on_mouse_exit=1 show_init=1 style=" + SH::URL_Encode((string)style) + ">";
    subPanelLayout += "<row><button id=subtest1 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/flipH.svg\"></button></row>";
    subPanelLayout += "<row><button id=subtest2 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/flipV.svg\"></button></row>";
    subPanelLayout += "</panel>";



    ZWinPanel* pPanel = new ZWinPanel();
    pPanel->mPanelLayout = "<panel close_on_button=1 show_init=1 style=\"" + SH::URL_Encode((string)style) + "\" rel_area_desc=\"" + SH::URL_Encode((string)rad) + "\"><row>";

    pPanel->mPanelLayout += "<panelbutton id=test1 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/openfile.svg\" layout=" + SH::URL_Encode(subPanelLayout) + " pos=ICOB scale=" + SH::URL_Encode(FPointToString(ZFPoint(1.0,4.0))) + "></panelbutton>";
    pPanel->mPanelLayout += "<button id=test2 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/copy.svg\"></button>";
    pPanel->mPanelLayout += "<button id=test3 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/exit.svg\"></button>";
    pPanel->mPanelLayout += "<button id=test4 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/move.svg\"></button>";
    pPanel->mPanelLayout += "<button id=test5 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/flipH.svg\"></button>";
    pPanel->mPanelLayout += "<button id=test6 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/flipV.svg\"></button>";
    pPanel->mPanelLayout += "<button id=test7 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/return.svg\"></button>";
    pPanel->mPanelLayout += "<button id=test8 group=group1 svgpath=\"D:/dev/git/ZGameFrame/build/ZImageViewer/Debug/res/fullscreen.svg\"></button>";
    //    pPanel->mPanelLayout += "<button id=test8 group=group1 caption=hello></button>";
    pPanel->mPanelLayout += "</row>";
//    sPanelLayout += "<row><button id=test1 group=group1 svgpath=%apppath%/res/test.svg></button><button id=test2 group=group1 caption=hello></button></row>";
//    sPanelLayout += "<row><label id=label1 caption=\"A label\"></label></row>";
//    sPanelLayout += "<row><button id=test9 group=group1 caption=hello></button><button id=test10 group=group1 svgpath=%apppath%/res/test.svg></button></row>";
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


    textBox.sText = "Quite the pickles?";
    textBox.style = gStyleCaption;

    textBox.style.look.colTop = 0xff12ff00;
    textBox.style.look.colBottom = 0xff500000;

    textBox.style.fp.sFacename = "Arial";
    textBox.style.fp.nWeight = 100;
    textBox.style.fp.bItalic = true;
    textBox.style.fp.nTracking = 10;
    textBox.style.look.decoration = ZGUI::ZTextLook::kNormal;
    textBox.style.fp.nScalePoints = 20000;
//    textBox.blurBackground = 2.0;
    textBox.dropShadowColor = 0xff0000ff;
    textBox.dropShadowOffset = ZPoint(15, 15);
    textBox.dropShadowBlur = 10.0;
    textBox.area.SetRect(0, 0, 3000, 2000);




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

    int64_t nSide = 64;
    for (int64_t y = 0; y < mAreaLocal.bottom; y += nSide)
    {
        for (int64_t x = 0; x < mAreaLocal.right; x += nSide)
        {
            uint32_t nCol = 0xffaaaaaa;
            if (((x/nSide) + (y/nSide) % 2) % 2)
                nCol = 0xffeeeeee;
            ZRect r(x, y, x + nSide, y + nSide);
            mpSurface.get()->Fill(nCol, &r);
        }
    }


    textBox.Paint(mpSurface.get());

//    textBox.style.Font()->DrawTextParagraph(mpSurface.get(), textBox.sText, mAreaLocal, &textBox.style);


    int iterations = 1;
    
    uint64_t start = gTimer.GetUSSinceEpoch();

    for (int i = 0; i < iterations; i++)
        textBox.Paint(mpSurface.get());
//        mpSurface->BltNoClip(mpSurface.get(), mAreaLocal, mAreaLocal, ZBuffer::kAlphaDest);

    uint64_t end = gTimer.GetUSSinceEpoch();

    uint64_t result = (end - start)/ iterations;

    cout << "paint time: " << result << "\n";



    start = gTimer.GetUSSinceEpoch();

    ZGUI::Style s(textBox.style);
    s.look.colTop = 0xff12ff00;
    s.look.colBottom = 0xff12ff00;
        s.look.decoration = ZGUI::ZTextLook::kShadowed;
//    for (int i = 0; i < iterations; i++)
//        textBox.style.Font()->DrawTextParagraph(mpSurface.get(), textBox.sText, mAreaLocal, &s);
//        mpSurface->BltAlphaNoClip(mpSurface.get(), mAreaLocal, mAreaLocal, 128, ZBuffer::kAlphaDest);

    end = gTimer.GetUSSinceEpoch();

    result = (end - start) / iterations;

    cout << "text time: " << result << "\n";




    return ZWin::Paint();
}
   
