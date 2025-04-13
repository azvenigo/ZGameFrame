#pragma once


#include "ZTypes.h"
#include "ZDebug.h"
#include "ZBuffer.h"

class ZScreenBuffer;


class ZGraphicSystem
{
public:
    ZGraphicSystem();
    ~ZGraphicSystem();
    
    bool                    Init();
    void                    Shutdown();
    
    // Init settings
    void                    SetArea(ZRect& r)           	{ mrSurfaceArea = r; }
    double                  GetAspectRatio()                { return (double)mrSurfaceArea.Width() / (double)mrSurfaceArea.Height(); }
    ZScreenBuffer*          GetScreenBuffer() 				{ return mpScreenBuffer; }
    
#ifdef _WIN64
    void                    SetHWND(HWND hwnd) { mhWnd = hwnd; }
    HWND                    GetMainHWND() { return mhWnd; }

    // GDI+
    ULONG_PTR               mpGDIPlusToken;
    HWND                    mhWnd;

#endif
    bool                    HandleModeChanges(ZRect& r);
    bool                    mbFullScreen;

protected:
    bool                    mbInitted;
    ZScreenBuffer*          mpScreenBuffer;
    ZRect                   mrSurfaceArea;
    
};

extern ZGraphicSystem gGraphicSystem;
extern ZGraphicSystem* gpGraphicSystem;
