#include "ZTypes.h"
#include "ZWin.h"
#include "TestWin.h"
#include "ZWinSlider.h"
#include "Resources.h"

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

    return true;
}

bool TestWin::Init()
{
    SetFocus();
    ZWinSlider* pWin = new ZWinSlider(&mnSliderVal);
    pWin->SetArea(ZRect(500, 1800, 1900, 1850));
    pWin->SetSliderRange(0, 1000, 2, 0.5);
    pWin->SetBehavior(ZWinSlider::kDrawSliderValueOnMouseOver | ZWinSlider::kHorizontal, gStyleButton.Font());

    ChildAdd(pWin);

    pWin = new ZWinSlider(&mnSliderVal);
    pWin->SetArea(ZRect(2000, 100, 2060, 1150));
    pWin->SetSliderRange(10, 20000, 1, 0.2);
    pWin->SetBehavior(ZWinSlider::kDrawSliderValueAlways| ZWinSlider::kVertical, gStyleButton.Font());
    ChildAdd(pWin);


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

    mpTransformTexture.get()->Fill(mAreaToDrawTo, 0xff003300);

    return ZWin::Paint();
}
   
