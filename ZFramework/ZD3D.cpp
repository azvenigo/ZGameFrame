#include "ZD3D.h"


#include <GdiPlus.h>
#include <wrl.h> 
using Microsoft::WRL::ComPtr;
#pragma comment(lib, "d3dcompiler.lib") // Link the D3DCompiler library


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;
namespace fs = std::filesystem;


bool ZD3D::InitD3D()
{
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
        throw runtime_error("mFactory is not IDXGIFactory2 or later.");
    }




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
        throw runtime_error("Failed to get swap chain back buffer");
    }

    // Create the render target view
    hr = mD3DDevice->CreateRenderTargetView(mBackBuffer, nullptr, &mRenderTargetView);
    if (FAILED(hr))
    {
        throw runtime_error("Failed to create render target view");
    }

    mD3DContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black color
    mD3DContext->ClearRenderTargetView(mRenderTargetView, clearColor);

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = mD3DDevice->CreateSamplerState(&sampDesc, &mSamplerState);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create sampler state");
    }
    mD3DContext->PSSetSamplers(0, 1, &mSamplerState);


    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.FrontCounterClockwise = TRUE;
    rasterDesc.DepthClipEnable = TRUE;

    ID3D11RasterizerState* rasterizerState;
    mD3DDevice->CreateRasterizerState(&rasterDesc, &rasterizerState);
    mD3DContext->RSSetState(rasterizerState);

    CompileShaders();
    
    return true;
}

bool ZD3D::ShutdownD3D()
{
    // Cleanup

    for (auto p : mVertexShaderMap)
    {
        (p.second)->Release();
    }
    mVertexShaderMap.clear();

    for (auto p : mPixelShaderMap)
    {
        (p.second)->Release();
    }
    mPixelShaderMap.clear();

    for (auto p : mInputLayoutMap)
    {
        (p.second)->Release();
    }
    mInputLayoutMap.clear();



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
        mD3DContext->Release();
    }
    mD3DContext = nullptr;

    if (mFactory)
        mFactory->Release();
    mFactory = nullptr;

    if (mD3DDevice)
        mD3DDevice->Release();
    mD3DDevice = nullptr;

    return true;
}


bool ZD3D::HandleModeChanges(ZRect& r)
{
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
        throw runtime_error("Failed to get swap chain back buffer");
    }

    // Create the render target view
    hr = mD3DDevice->CreateRenderTargetView(mBackBuffer, nullptr, &mRenderTargetView);
    if (FAILED(hr))
    {
        throw runtime_error("Failed to create render target view");
    }

    mD3DContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black color
    mD3DContext->ClearRenderTargetView(mRenderTargetView, clearColor);









//    return InitD3D();

	mpScreenBuffer = new ZScreenBuffer();
	mpScreenBuffer->Init(mrSurfaceArea.Width(), mrSurfaceArea.Height(), this);
    
    return true;
}


bool ZD3D::CompileShaders()
{
    // enumerate shaders in subfolder and map them into our members
    tStringList shaderFiles;
    for (auto const& dir_entry : fs::directory_iterator("res/shaders/"))
    {
        if (dir_entry.is_regular_file())
        {
            fs::path p = dir_entry.path();
            p.make_preferred();
            if (p.extension() == ".hlsl")
            {
                fs::path filename = p.filename();
                string sName(filename.string().substr(2, filename.string().length()-7)); // extract shader name

                if (SH::StartsWith(p.filename().string(), "p", false))
                    LoadPixelShader(sName, p.string());
                else if (SH::StartsWith(p.filename().string(), "v", false))
                    LoadVertexShader(sName, p.string());
                else
                    cout << "Unknown shader file prefix: " << p.string() << "\n";
            }
            else
            {
                cout << "Unknown file type when shader expected: " << p.string() << "\n";
            }
        }
    }

    return true;
}


ID3D11Texture2D* ZD3D::CreateDynamicTexture(ZPoint size)
{
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = size.x;
    textureDesc.Height = size.y;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DYNAMIC; // Allow CPU updates
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // Required for Map()
    textureDesc.MiscFlags = 0;

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = mD3DDevice->CreateTexture2D(&textureDesc, nullptr, &texture);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create dynamic texture.");
        return nullptr;
    }

    return texture;
}

bool ZD3D::UpdateTexture(ID3D11Texture2D* pTexture, ZBuffer* pSource)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = mD3DContext->Map(pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to map texture.");
    }

    // Copy pixel data
    int64_t width = pSource->GetArea().Width();
    for (UINT row = 0; row < pSource->GetArea().bottom; ++row)
    {
        memcpy((BYTE*)mappedResource.pData + row * mappedResource.RowPitch,
            (BYTE*)pSource->mpPixels + row * width * 4,
            width * 4);
    }

    mD3DContext->Unmap(pTexture, 0);
}

bool ZD3D::CreateShaderResourceView( ID3D11Texture2D* texture, ID3D11ShaderResourceView** pTextureSRV)
{
    assert(mD3DDevice);
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Make sure this matches the texture format
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1; // Use all MIP levels

    HRESULT hr = mD3DDevice->CreateShaderResourceView(texture, &srvDesc, pTextureSRV);
    if (FAILED(hr))
    {
        return false;
    }
    return true;
}

bool ZD3D::LoadPixelShader(const string& sName, const string& sPath)
{
    assert(mD3DDevice);

    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // Compile Pixel Shader
    HRESULT hr = D3DCompileFromFile(SH::string2wstring(sPath).c_str(), nullptr, nullptr, "PSMain", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            cerr << (char*)errorBlob->GetBufferPointer() << "\n";
            errorBlob->Release();
        }

        return false;
    }

    // Create Pixel Shader
    ID3D11PixelShader* pixelShader = nullptr;
    hr = mD3DDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
    if (FAILED(hr))
    {
        assert(false);
        return false;
    }

    mPixelShaderMap[sName] = pixelShader;

    psBlob->Release();

    return true;
}

bool ZD3D::LoadVertexShader(const string& sName, const string& sPath)
{
    assert(mD3DDevice);

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // Compile Vertex Shader
    HRESULT hr = D3DCompileFromFile(SH::string2wstring(sPath).c_str(), nullptr, nullptr, "VSMain", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            cerr << (char*)errorBlob->GetBufferPointer() << "\n";
            errorBlob->Release();
        }

        return false;
    }

    ID3D11VertexShader* vertexShader = nullptr;

    hr = mD3DDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr))
    {
        assert(false);
        return false;
    }

    mVertexShaderMap[sName] = vertexShader;


    // Define the input layout for the vertex buffer
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, uv), D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    ID3D11InputLayout* pLayout;
    hr = mD3DDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(), &pLayout);
    if (FAILED(hr))
    {
        assert(false);
        return false;
    }

    mInputLayoutMap[sName] = pLayout;








    vsBlob->Release();

    return true;
}

ID3D11VertexShader* ZD3D::GetVertexShader(const std::string& sName)
{
    tVertexShaderMap::iterator it = mVertexShaderMap.find(sName);
    if (it == mVertexShaderMap.end())
        return nullptr;

    return (*it).second;
}

ID3D11PixelShader* ZD3D::GetPixelShader(const std::string& sName)
{
    tPixelShaderMap::iterator it = mPixelShaderMap.find(sName);
    if (it == mPixelShaderMap.end())
        return nullptr;

    return (*it).second;
}

ID3D11InputLayout* ZD3D::GetInputLayout(const std::string& sName)
{
    tInputLayoutMap::iterator it = mInputLayoutMap.find(sName);
    if (it == mInputLayoutMap.end())
        return nullptr;

    return (*it).second;
}