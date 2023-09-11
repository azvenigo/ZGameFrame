#include "ZTypes.h"
#include "ZWin.h"
#include "TestWin.h"
#include "ZWinSlider.h"
#include "Resources.h"
#include "ZWinBtn.H"

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
    ZWinSlider* pWin = new ZWinSlider(&mnSliderVal);
    pWin->SetArea(ZRect(500, 1800, 1900, 1850));
    pWin->SetSliderRange(0, 1000, 2, 0.5);
    pWin->mBehavior = ZWinSlider::kDrawSliderValueOnMouseOver;

    ChildAdd(pWin);

    pWin = new ZWinSlider(&mnSliderVal);
    pWin->SetArea(ZRect(2000, 100, 2060, 1150));
    pWin->SetSliderRange(10, 20000, 1, 0.2);
    pWin->mBehavior = ZWinSlider::kDrawSliderValueAlways;
    ChildAdd(pWin);


    ZRect rBtn(100, 100, 500, 200);

    for (int i = 0; i < 1; i++)
    {

        ZWinSizablePushBtn* pBtn = new ZWinSizablePushBtn();
        pBtn->mSVGImage.Load("C:/temp/svg/tivo.svg");
        pBtn->SetTooltip("Load Image");
        pBtn->mSVGImage.style.paddingH = 3;
        pBtn->mSVGImage.style.paddingV = 3;

        pBtn->SetArea(rBtn);
        pBtn->SetMessage(ZMessage("loadimg", this));
        ChildAdd(pBtn);

        rBtn.OffsetRect(20, 20);
    }

    return ZWin::Init();
}
bool TestWin::Process()
{
    return ZWin::Process();
}

bool TestWin::Paint()
{
    if (!mbInvalid)
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());

    mpTransformTexture.get()->Fill(0xff003300);

    return ZWin::Paint();
}
   
