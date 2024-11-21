#include "ZTypes.h"
#include "ZWin.h"
#include "OverlayWin.h"
#include "ZWinSlider.h"
#include "Resources.h"
#include "ZWinBtn.H"
#include "ZWinPanel.h"
#include "ZWinFormattedText.h"

using namespace std;

OverlayWin::OverlayWin()
{
    mbAcceptsCursorMessages = false;
    mbAcceptsFocus = false;
    msWinName = "OverlayWin";
}
   
bool OverlayWin::Init()
{
    return ZWin::Init();
}


bool OverlayWin::OnParentAreaChange()
{
    if (!mpSurface)
        return false;

    SetArea(mpParentWin->GetArea());

    ZWin::OnParentAreaChange();

    return true;
}



bool OverlayWin::Paint()
{
    if (!PrePaintCheck())
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

    mpSurface.get()->Fill(0xff000000);
    ZRect r(100, 100, 500, 500);
    mpSurface.get()->Fill(0, &r);

    return ZWin::Paint();
}
   
