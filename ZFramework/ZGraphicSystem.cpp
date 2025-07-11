#include "ZGraphicSystem.h"
#include "ZBuffer.h"
#include "ZScreenBuffer.h"
#include "ZDebug.h"

#ifdef _WIN64
#include <GdiPlus.h>
#endif

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _WIN64
void InterpretError(HRESULT hr)
{
    char buf[256];
    sprintf_s(buf, "UNKNOWN ERROR:%x", (long)hr);

    MessageBoxA(NULL, buf, "error", MB_OK);
    ZDEBUG_OUT(buf);
}
#endif

ZGraphicSystem::ZGraphicSystem()
{
    mbInitted = false;
    mpScreenBuffer = NULL;
    mbFullScreen = true;
}

ZGraphicSystem::~ZGraphicSystem()
{
    Shutdown();
}

bool ZGraphicSystem::Init()
{
    mbInitted = true;

#ifdef _WIN64
    // GDI+ Init
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&mpGDIPlusToken, &gdiplusStartupInput, NULL);
#endif

    mpScreenBuffer = new ZScreenBuffer();
    mpScreenBuffer->Init(mrSurfaceArea.Width(), mrSurfaceArea.Height(), this);

    return true;
}

void ZGraphicSystem::Shutdown()
{
    if (mbInitted)
    {

        if (mpScreenBuffer)
        {
            mpScreenBuffer->Shutdown();
            delete mpScreenBuffer;
            mpScreenBuffer = NULL;
        }

#ifdef _WIN64
        // GDI+ Shutdown
        Gdiplus::GdiplusShutdown(mpGDIPlusToken);
#endif

        mbInitted = false;
    }
}

bool ZGraphicSystem::HandleModeChanges(ZRect& r)
{
    if (mpScreenBuffer && mpScreenBuffer->GetArea() == r)
        return true;

    if (mpScreenBuffer)
    {
        if (mpScreenBuffer->GetArea() == r)
            return true;

        mpScreenBuffer->Shutdown();
    }

    SetArea(r);

    mpScreenBuffer = new ZScreenBuffer();
    mpScreenBuffer->Init(mrSurfaceArea.Width(), mrSurfaceArea.Height(), this);

    return true;
}
