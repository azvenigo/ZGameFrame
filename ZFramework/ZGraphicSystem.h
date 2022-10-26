#pragma once

//#define USE_D3D

#include "ZStdTypes.h"
#include "ZStdDebug.h"
#include "ZBuffer.h"

//#ifdef _WIN64
//#include <windows.h>
//struct IDirectDraw7;
//struct IDirect3DDevice7;
//#endif
#include "ZFont.h"


class ZScreenBuffer;

const int64_t kBitDepth = 32;		


///////////////////////////////////////////////////////////////////////////////
// class ZGraphicSystem

class ZGraphicSystem
{
public:
	ZGraphicSystem();
	~ZGraphicSystem();

	bool 					Init();
	void 					Shutdown();

	// Init settings
	void 					SetFullScreen(bool setting) 	{ mbInitSettingFullScreen = setting; }
	void 					SetArea(ZRect& r)           	{ mrSurfaceArea = r; }
    double                  GetAspectRatio()                { return (double)mrSurfaceArea.Width() / (double)mrSurfaceArea.Height(); }
	ZScreenBuffer*			GetScreenBuffer() 				{ return mpScreenBuffer; }

#ifdef _WIN64
    void 					SetHWND(HWND hwnd) { mhWnd = hwnd; }
    HWND 					GetMainHWND() { return mhWnd; }

    // GDI+
    ULONG_PTR				mpGDIPlusToken;
    HWND                    mhWnd;
#endif

#ifdef USE_D3D 
	IDirect3D9*				GetD3D()          				{ return mpD3D; }
	IDirect3DDevice9*		GetD3DDevice()					{ return mpD3DDevice; }
#endif

	bool					HandleModeChanges();

protected:
	bool                    mbInitted;

	ZScreenBuffer*			mpScreenBuffer;
#ifdef USE_D3D 
	IDirect3D9*				mpD3D;
	IDirect3DDevice9*		mpD3DDevice;
	D3DPRESENT_PARAMETERS	mPresentParams;
#endif

	ZRect                   mrSurfaceArea;

//	tRectList				mDirtyRects;

	// Init settings
	bool                    mbInitSettingFullScreen;
};

extern ZGraphicSystem gGraphicSystem;
extern ZGraphicSystem* gpGraphicSystem;
