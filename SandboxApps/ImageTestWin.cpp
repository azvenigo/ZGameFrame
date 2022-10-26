#include "ImageTestWin.h"
#include "ZRandom.h"
#include "ZFont.h"
#include "ZImageWin.h"

ImageTestWin::ImageTestWin()
{
}
   
bool ImageTestWin::Init()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mbInvalidateParentWhenInvalid = true;

    SetFocus();

    ZImageWin* pWin = new ZImageWin();

    ZRect rTestArea(100, 100, 1800, 1100);

    pWin->SetArea(rTestArea);
    pWin->LoadImage("c:/temp/ericcartoon.jpg");
//    pWin->LoadImage("c:/temp/414A2422.jpg");
    pWin->SetZoomable(true);

    ChildAdd(pWin);




    return ZWin::Init();
}

bool ImageTestWin::OnKeyDown(uint32_t key)
{
#ifdef _WIN64
    if (key == VK_ESCAPE)
    {
        gMessageSystem.Post("quit_app_confirmed");
        return true;
    }
#endif

    return ZWin::OnKeyDown(key);
}


bool ImageTestWin::Paint()
{
    const std::lock_guard<std::mutex> surfaceLock(mpTransformTexture->GetMutex());
    if (!mbInvalid)
        return true;

    string sTemp;
    Sprintf(sTemp, "id: 0x%llx", (uint64_t)this);

    gpFontSystem->GetDefaultFont(5)->DrawTextParagraph(mpTransformTexture, sTemp, mAreaToDrawTo);
    return ZWin::Paint();
}
   