#include "ZTypes.h"
#include "ZWin.h"
#include "OnePageDocWin.h"
#include "ZWinSlider.h"
#include "Resources.h"
#include "ZWinBtn.H"
#include "ZWinPanel.h"
#include "ZWinFormattedText.h"
#include "helpers/StringHelpers.h"

using namespace std;

OnePageDocWin::OnePageDocWin()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    msWinName = "OnePageDocWin";
}
   
bool OnePageDocWin::OnKeyDown(uint32_t key)
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

bool OnePageDocWin::Init()
{
    if (!mbInitted)
    {
        SetFocus();

        mpWinImage = new ZWinImage();

        string sText;
        bool bRet = SH::Load("res/docs/alice_wonderland.txt", sText);
        sText = SH::convertToASCII(sText);
        assert(bRet);

        int64_t nHeightPoints = 300;
        mDocStyle = ZGUI::Style(ZFontParams("verdana", nHeightPoints, 100));
        ZRect rFull;
        int64_t lines = mDocStyle.Font()->CalculateNumberOfLines(nHeightPoints*5, (uint8_t*)sText.c_str(), sText.length());
        rFull.SetRect(0, 0, nHeightPoints*5, lines * mDocStyle.fp.Height());
        
        tZBufferPtr docImage(new ZBuffer());
        docImage->Init(rFull.Width(), rFull.Height());
        docImage->Fill(0xfff4f0f4);

        mDocStyle.look.colTop = 0xff000000;
        mDocStyle.look.colBottom = 0xff000000;
        mDocStyle.Font()->DrawTextParagraph(docImage.get(), sText, rFull, &mDocStyle);

        mpWinImage->mBehavior |= ZWinImage::kHotkeyZoom | ZWinImage::kScrollable;
        mpWinImage->mZoomHotkey = VK_MENU;
        mpWinImage->SetArea(mAreaLocal);
        mpWinImage->SetImage(docImage);
        mpWinImage->nSubsampling = 2;

//        docImage->SaveBuffer("c:/temp/bigdoc.bmp");

        ChildAdd(mpWinImage);
    }

    return ZWin::Init();
}
bool OnePageDocWin::Process()
{
    return ZWin::Process();
}


bool OnePageDocWin::OnParentAreaChange()
{
    if (!mpSurface)
        return false;

    SetArea(mpParentWin->GetArea());

    ZWin::OnParentAreaChange();

    return true;
}



bool OnePageDocWin::Paint()
{
    if (!PrePaintCheck())
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());


    return ZWin::Paint();
}