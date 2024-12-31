#include "ZGraphicSystem.h"
#include "ZBuffer.h"
#include "ZScreenBuffer.h"
#include "ZDebug.h"

#ifdef _WIN64
#include <GdiPlus.h>
#include <wrl.h> 
using Microsoft::WRL::ComPtr;
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
	sprintf_s(buf, "UNKNOWN ERROR:%x", (long) hr);

	MessageBoxA(NULL, buf, "error", MB_OK);
	ZDEBUG_OUT(buf);
}
#endif

ZGraphicSystem::ZGraphicSystem()
{
	mbInitted				= false;
	mpScreenBuffer			= NULL;
    mbFullScreen             = true;  
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





    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr;


    // Specify feature levels (e.g., Direct3D 11, 10.1, 10.0)
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    D3D_FEATURE_LEVEL featureLevel; // To store the level we ended up using

    // Create the device and context
    hr = D3D11CreateDevice(
        nullptr,                    // Use the default adapter
        D3D_DRIVER_TYPE_HARDWARE,   // Use the GPU
        nullptr,                    // No software rasterizer
        createDeviceFlags,          // Device creation flags
        featureLevels,              // Feature levels to check for
        ARRAYSIZE(featureLevels),   // Number of feature levels in the array
        D3D11_SDK_VERSION,          // Always D3D11_SDK_VERSION
        &mD3DDevice,                 // Returns the created device
        &featureLevel,              // Returns the selected feature level
        &mD3DContext                 // Returns the device context
    );


    // Create the DXGI Factory
    hr = CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void**)&mFactory);

    ComPtr<IDXGIFactory2> dxgiFactory2;
    hr = mFactory->QueryInterface(__uuidof(IDXGIFactory2), &dxgiFactory2);
    if (FAILED(hr)) {
        throw std::runtime_error("mFactory is not IDXGIFactory2 or later.");
    }

    InitD3D();

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

        ShutdownD3D();
        if (mD3DContext)
            mD3DContext->Release();
        mD3DContext = nullptr;

        if (mFactory)
            mFactory->Release();
        mFactory = nullptr;

        if (mD3DDevice)
            mD3DDevice->Release();
        mD3DDevice = nullptr;


		// GDI+ Shutdown
		Gdiplus::GdiplusShutdown(mpGDIPlusToken);
#endif
	
		mbInitted = false;
	}
}

bool ZGraphicSystem::InitD3D()
{

    ComPtr<IDXGIAdapter1> adapter;
    if (SUCCEEDED(mFactory->EnumAdapters1(0, &adapter))) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        wprintf(L"Adapter: %s\n", desc.Description);
    }



    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = (UINT)mrSurfaceArea.Width();
    swapChainDesc.Height = (UINT)mrSurfaceArea.Height();
    //swapChainDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // HDR format
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SampleDesc.Count = 1;                     // Single-sampling (no MSAA)
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferCount = 2; // Double buffering
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Flags = 0;

    HRESULT hr = mFactory->CreateSwapChainForHwnd(
        mD3DDevice,
        mhWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &mSwapChain
    );

    IDXGISwapChain4* swapChain4 = nullptr;
    hr = mSwapChain->QueryInterface(__uuidof(IDXGISwapChain4), (void**)&swapChain4);
    if (SUCCEEDED(hr))
    {

        DXGI_HDR_METADATA_HDR10 hdr10 = {};
        hdr10.MaxMasteringLuminance = 1000;    // Display's max brightness (cd/m²)
        hdr10.MinMasteringLuminance = 0;     // Display's min brightness (cd/m²)
        hdr10.MaxContentLightLevel = 1000;     // Peak brightness of content
        hdr10.MaxFrameAverageLightLevel = 500; // Average brightness of content


        swapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(hdr10), &hdr10);
    }


    hr = mSwapChain->GetBuffer(0, IID_PPV_ARGS(&mBackBuffer));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to get swap chain back buffer");
    }

    // Create the render target view
    hr = mD3DDevice->CreateRenderTargetView(mBackBuffer, nullptr, &mRenderTargetView);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create render target view");
    }

    mD3DContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black color
    mD3DContext->ClearRenderTargetView(mRenderTargetView, clearColor);

    return true;
}

bool ZGraphicSystem::ShutdownD3D()
{
    // Cleanup
    if (mSwapChain)
        mSwapChain->Release();
    mSwapChain = nullptr;

    if (mRenderTargetView)
        mRenderTargetView->Release();
    mRenderTargetView = nullptr;

    if (mBackBuffer)
        mBackBuffer->Release();
    mBackBuffer = nullptr;

    if (mD3DContext)
    {
        mD3DContext->ClearState();
        mD3DContext->Flush();
    }

    return true;
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
        delete mpScreenBuffer;
        mpScreenBuffer = NULL;
    }

    //Shutdown();
//    ShutdownD3D();
    SetArea(r);



    if (mBackBuffer)
        mBackBuffer->Release();
    mBackBuffer = nullptr;
    if (mRenderTargetView)
        mRenderTargetView->Release();
    mRenderTargetView = nullptr;

    mSwapChain->ResizeBuffers(2, (UINT)r.Width(), (UINT)r.Height(), DXGI_FORMAT_B8G8R8A8_UNORM, 0);

    HRESULT hr = mSwapChain->GetBuffer(0, IID_PPV_ARGS(&mBackBuffer));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to get swap chain back buffer");
    }

    // Create the render target view
    hr = mD3DDevice->CreateRenderTargetView(mBackBuffer, nullptr, &mRenderTargetView);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create render target view");
    }

    mD3DContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black color
    mD3DContext->ClearRenderTargetView(mRenderTargetView, clearColor);









//    return InitD3D();

	mpScreenBuffer = new ZScreenBuffer();
	mpScreenBuffer->Init(mrSurfaceArea.Width(), mrSurfaceArea.Height(), this);
    
    return true;
}
