#include "ZTypes.h"
#include "ZWin.h"
#include "ImageViewer.h"
#include "ZWinImage.h"
#include "Resources.h"

using namespace std;

ImageViewer::ImageViewer()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mIdleSleepMS = 13;
}
   
bool ImageViewer::OnKeyDown(uint32_t key)
{
#ifdef _WIN64
    if (key == VK_ESCAPE)
    {
        gMessageSystem.Post("quit_app_confirmed");
    }
#endif

    return true;
}

bool ImageViewer::Init()
{
    if (!mbInitted)
    {
        SetFocus();
        mpWinImage = new ZWinImage();
        mpWinImage->SetArea(mAreaToDrawTo);
        ChildAdd(mpWinImage);
    }
    return ZWin::Init();
}

bool ImageViewer::ViewImage(const std::string& sFilename)
{
    msFilenameToLoad = sFilename;
    if (mbInitted)
    {
        if (msLoadedFilename != msFilenameToLoad)
        {
            msLoadedFilename = msFilenameToLoad;
            if (mpWinImage->LoadImage(msLoadedFilename))
            {
                mpWinImage->mBehavior |= (ZWinImage::kZoom | ZWinImage::kShowControlPanel);

                mpWinImage->SetFill(0x00000000);
                Invalidate();
            }
        }
    }

    return true;
}


bool ImageViewer::Process()
{
    if (mpWinImage && msFilenameToLoad != msLoadedFilename)
        ViewImage(msFilenameToLoad);
    return ZWin::Process();
}

bool ImageViewer::Paint()
{
/*    if (!mbInvalid)
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());

    mpTransformTexture.get()->Fill(mAreaToDrawTo, 0xff003300);*/

    return ZWin::Paint();
}
   
