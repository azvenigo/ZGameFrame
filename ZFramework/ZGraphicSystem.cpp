#include "ZGraphicSystem.h"
#include "ZBuffer.h"
#include "ZScreenBuffer.h"
#include "ZStdDebug.h"

#ifdef _WIN64
#include <GdiPlus.h>
#endif

//extern bool gbFullScreen;

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _WIN64
void InterpretError(HRESULT hr)
{
	char buf[256];
	sprintf_s(buf, "UNKNOWN ERROR:%x", (long) hr);

	MessageBoxA(NULL, buf, "error", MB_OK);
	ZDEBUG_OUT(buf);
}
#endif

ZGraphicSystem::ZGraphicSystem()
{
	mbInitted				= false;
#ifdef USE_D3D 
	mpD3D					= NULL;
	mpD3DDevice				= NULL;
	ZeroMemory(&mPresentParams, sizeof(mPresentParams));
#endif
	//mbInitSettingFullScreen = gbFullScreen;
	mpScreenBuffer			= NULL;
  
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


#ifdef USE_D3D 
	HRESULT hr;
	mpD3D = Direct3DCreate9( D3D_SDK_VERSION );
	if (!mpD3D)
		return false;

	// Set up the structure used to create the D3DDevice

	mPresentParams.Windowed						= !mbInitSettingFullScreen;
	mPresentParams.BackBufferCount				= 1;
	mPresentParams.BackBufferFormat				= D3DFMT_X8R8G8B8;
	mPresentParams.BackBufferWidth				= mrSurfaceArea.Width();
	mPresentParams.BackBufferHeight				= mrSurfaceArea.Height();
	mPresentParams.hDeviceWindow				= mhWnd;
	mPresentParams.SwapEffect					= D3DSWAPEFFECT_COPY;
	mPresentParams.FullScreen_RefreshRateInHz	= D3DPRESENT_RATE_DEFAULT;
	mPresentParams.PresentationInterval			= D3DPRESENT_INTERVAL_IMMEDIATE;
	mPresentParams.Flags						= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	hr = mpD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, mhWnd,	D3DCREATE_HARDWARE_VERTEXPROCESSING, &mPresentParams, &mpD3DDevice);

	if (!mpD3DDevice)
	{
		hr = mpD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, mhWnd,	D3DCREATE_SOFTWARE_VERTEXPROCESSING, &mPresentParams, &mpD3DDevice);
	}

	if (!mpD3DDevice)
	{
		InterpretError(hr);
		CEASSERT(false);
		return false;
	}
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

#ifdef USE_D3D 
		if (mpD3DDevice)
		{
			mpD3DDevice->Release();
			mpD3DDevice = NULL;
		}

		if (mpD3D)
		{
			mpD3D->Release();
			mpD3D = NULL;
		}		
#endif

#ifdef _WIN64
		// GDI+ Shutdown
		Gdiplus::GdiplusShutdown(mpGDIPlusToken);
#endif
	
		mbInitted = false;
	}
}

bool ZGraphicSystem::HandleModeChanges()
{
	if (mpScreenBuffer)
	{
		mpScreenBuffer->Shutdown();
		delete mpScreenBuffer;
		mpScreenBuffer = NULL;
	}

#ifdef USE_D3D 
	if (mpD3DDevice)
	{
		mpD3DDevice->Release();
		mpD3DDevice = NULL;
	}

	// Set up the structure used to create the D3DDevice
	mPresentParams.Windowed						= !mbInitSettingFullScreen;
	mPresentParams.BackBufferCount				= 1;
	mPresentParams.BackBufferFormat				= D3DFMT_X8R8G8B8;
	mPresentParams.BackBufferWidth				= mrSurfaceArea.Width();
	mPresentParams.BackBufferHeight				= mrSurfaceArea.Height();
	mPresentParams.hDeviceWindow				= mhWnd;
	mPresentParams.SwapEffect					= D3DSWAPEFFECT_COPY;
	mPresentParams.FullScreen_RefreshRateInHz	= D3DPRESENT_RATE_DEFAULT;
	mPresentParams.PresentationInterval			= D3DPRESENT_INTERVAL_IMMEDIATE;
	mPresentParams.Flags						= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	HRESULT hr = mpD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, mhWnd,	D3DCREATE_HARDWARE_VERTEXPROCESSING, &mPresentParams, &mpD3DDevice);

	if (!mpD3DDevice)
	{
		hr = mpD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, mhWnd,	D3DCREATE_SOFTWARE_VERTEXPROCESSING, &mPresentParams, &mpD3DDevice);
	}

	if (!mpD3DDevice)
	{
		InterpretError(hr);
		CEASSERT(false);
		return false;
	}
#endif

	mpScreenBuffer = new ZScreenBuffer();
	mpScreenBuffer->Init(mrSurfaceArea.Width(), mrSurfaceArea.Height(), this);

    return true;
}
