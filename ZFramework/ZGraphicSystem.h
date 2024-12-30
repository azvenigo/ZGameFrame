#pragma once

//#define USE_D3D

#include "ZTypes.h"
#include "ZDebug.h"
#include "ZBuffer.h"
#ifdef _WIN64
#include <d3d11.h>
#include <dxgi1_6.h>
#endif

class ZScreenBuffer;

class ZGraphicSystem
{
public:
	ZGraphicSystem();
	~ZGraphicSystem();

	bool 					Init();
	void 					Shutdown();

	// Init settings
	void 					SetArea(ZRect& r)           	{ mrSurfaceArea = r; }
    double                  GetAspectRatio()                { return (double)mrSurfaceArea.Width() / (double)mrSurfaceArea.Height(); }
	ZScreenBuffer*			GetScreenBuffer() 				{ return mpScreenBuffer; }

#ifdef _WIN64
    void 					SetHWND(HWND hwnd) { mhWnd = hwnd; }
    HWND 					GetMainHWND() { return mhWnd; }

    // GDI+
    ULONG_PTR				mpGDIPlusToken;
    HWND                    mhWnd;

    IDXGISwapChain1*        mSwapChain;
    ID3D11Device*           mD3DDevice;
    ID3D11DeviceContext*    mD3DContext;
    IDXGIFactory2*          mFactory;
    ID3D11RenderTargetView* mRenderTargetView;
    ID3D11Texture2D*        mBackBuffer;
#endif

	bool                    HandleModeChanges(ZRect& r);

    bool                    mbFullScreen;

protected:

    bool                    InitD3D();
    bool                    ShutdownD3D();

	bool                    mbInitted;
	ZScreenBuffer*			mpScreenBuffer;
	ZRect                   mrSurfaceArea;

};

extern ZGraphicSystem gGraphicSystem;
extern ZGraphicSystem* gpGraphicSystem;
