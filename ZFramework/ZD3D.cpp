#include "ZD3D.h"
#include <GdiPlus.h>
#include <wrl.h> 
#include <array>
#include <filesystem>
#include "helpers/StringHelpers.h"

using Microsoft::WRL::ComPtr;
#pragma comment(lib, "d3dcompiler.lib") // Link the D3DCompiler library


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;
namespace fs = std::filesystem;

namespace ZD3D
{
    IDXGISwapChain1*        mSwapChain;
    ID3D11Device*           mD3DDevice;
    ID3D11DeviceContext*    mD3DContext;
    IDXGIFactory2*          mFactory;
    ID3D11RenderTargetView* mRenderTargetView;
    ID3D11Texture2D*        mBackBuffer;
    ID3D11SamplerState*     mSamplerState;
    D3D11_VIEWPORT          mViewport;
    tVertexShaderMap        mVertexShaderMap;
    tPixelShaderMap         mPixelShaderMap;
    tInputLayoutMap         mInputLayoutMap;






    bool InitD3D(HWND hWnd, ZPoint size)
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
        swapChainDesc.Width = (UINT)size.x;
        swapChainDesc.Height = (UINT)size.y;
        //swapChainDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // HDR format
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SampleDesc.Count = 1;                     // Single-sampling (no MSAA)
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferCount = 2; // Double buffering
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = 0;

        hr = mFactory->CreateSwapChainForHwnd(
            mD3DDevice,
            hWnd,
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

        mViewport = {};
        mViewport.TopLeftX = 0;
        mViewport.TopLeftY = 0;
        mViewport.Width = static_cast<FLOAT>(size.x);
        mViewport.Height = static_cast<FLOAT>(size.y);
        mViewport.MinDepth = 0.0f;
        mViewport.MaxDepth = 1.0f;


        return true;
    }

    bool ShutdownD3D()
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


    bool HandleModeChanges(ZRect& r)
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

        return true;
    }

    tZBufferPtr gTestBuf;
    //ID3D11Texture2D* gTestTexture = nullptr;
    DynamicTexture gTestTexture;

    bool Present()
    {
        mD3DContext->RSSetViewports(1, &mViewport);


        if (!gTestBuf)
        {
            gTestBuf.reset(new ZBuffer());
            gTestBuf->LoadBuffer("res/brick-texture.jpg");
            //gTestTexture = CreateDynamicTexture(gTestBuf->GetArea().BR());
            gTestTexture.Init(gTestBuf->GetArea().BR());
        }
        gTestTexture.UpdateTexture(gTestBuf.get());

        std::array<Vertex, 3> verts;

        verts[0].position.x = 0;
        verts[0].position.y = 1000;
        verts[0].position.z = 0;
        verts[0].uv.x = 0;
        verts[0].uv.y = 0;



        verts[1].position.x = 1800;
        verts[1].position.y = 1000;
        verts[1].position.z = 0;
        verts[1].uv.x = 1.0;
        verts[1].uv.y = 0;


        verts[2].position.x = 900;
        verts[2].position.y = 100;
        verts[2].position.z = 0;
        verts[2].uv.x = 0.5;
        verts[2].uv.y = 1.0;


        mD3DContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);

        //    float clearColor[4] = { 0.0f, 0.5f, 0.0f, 1.0f };
        //    mpGraphicSystem->mD3DContext->ClearRenderTargetView(mpGraphicSystem->mRenderTargetView, clearColor);





        RenderScreenSpaceTriangle(gTestTexture.mpSRV, verts);


        mSwapChain->Present(1, 0); // VSync: 1
        return true;
    }

    //void ZScreenBuffer::RenderScreenSpaceTriangle(ID3D11RenderTargetView* backBufferRTV, ID3D11ShaderResourceView* textureSRV, ID3D11SamplerState* sampler, float screenWidth, float screenHeight, const std::array<Vertex, 3>& triangleVertices)
    void RenderScreenSpaceTriangle(ID3D11ShaderResourceView* textureSRV, const std::array<Vertex, 3>& triangleVertices)
    {
        // Bind the back buffer as the render target
    //    ID3D11RenderTargetView* backBufferRTV = (ID3D11RenderTargetView*)mpGraphicSystem->mBackBuffer;
    //    mpGraphicSystem->mD3DContext->OMSetRenderTargets(1, &backBufferRTV, nullptr);

        // Convert screen-space coordinates to Normalized Device Coordinates (NDC)
        std::array<Vertex, 3> ndcVertices;
        for (int i = 0; i < 3; ++i)
        {
            //ndcVertices[i].position.x = (triangleVertices[i].position.x / (float)mSurfaceArea.Width()) * 2.0f - 1.0f;
            //ndcVertices[i].position.y = 1.0f - (triangleVertices[i].position.y / (float)mSurfaceArea.Height()) * 2.0f;
            ndcVertices[i].position.x = (triangleVertices[i].position.x / (float)mViewport.Width) * 2.0f - 1.0f;
            ndcVertices[i].position.y = 1.0f - (triangleVertices[i].position.y / (float)mViewport.Height) * 2.0f;


            ndcVertices[i].position.z = triangleVertices[i].position.z; // Keep Z value
            ndcVertices[i].uv = triangleVertices[i].uv;
        }

        /*    Vertex ndcVertices[] =
            {
                { XMFLOAT3(-0.5f, -0.5f, 0), XMFLOAT2(0.0f, 1.0f) }, // Bottom-left
                { XMFLOAT3(0.5f, -0.5f, 0), XMFLOAT2(1.0f, 1.0f) }, // Bottom-right
                { XMFLOAT3(0.0f,  0.5f, 0), XMFLOAT2(0.5f, 0.0f) }  // Top
            };*/

            // Create vertex buffer
        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        vertexBufferDesc.ByteWidth = sizeof(ndcVertices);
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA vertexData = {};
        vertexData.pSysMem = ndcVertices.data();
        //vertexData.pSysMem = &ndcVertices[0];

        ID3D11Buffer* vertexBuffer = nullptr;
        mD3DDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

        // Bind vertex buffer
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        mD3DContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        mD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


        ID3D11VertexShader* vertexShader = GetVertexShader("ScreenSpaceShader");
        ID3D11PixelShader* pixelShader = GetPixelShader("ScreenSpaceShader");
        ID3D11InputLayout* layout = GetInputLayout("ScreenSpaceShader");


        assert(vertexShader);
        assert(pixelShader);

        // Set shaders
        mD3DContext->VSSetShader(vertexShader, nullptr, 0);
        mD3DContext->PSSetShader(pixelShader, nullptr, 0);

        mD3DContext->IASetInputLayout(layout);


        // Set texture and sampler
        mD3DContext->PSSetShaderResources(0, 1, &textureSRV);


        ID3D11Debug* debugInterface;
        mD3DDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&debugInterface);
        debugInterface->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        debugInterface->Release();









        // Draw triangle
        mD3DContext->Draw(3, 0);

        // Cleanup
        vertexBuffer->Release();
    }




    bool CompileShaders()
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
                    string sName(filename.string().substr(2, filename.string().length() - 7)); // extract shader name

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


    DynamicTexture::~DynamicTexture()
    {
        Shutdown();
    }

    bool DynamicTexture::Init(ZPoint size)
    {
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = size.x;
        textureDesc.Height = size.y;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DYNAMIC; // Allow CPU updates
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // Required for Map()
        textureDesc.MiscFlags = 0;

        //ID3D11Texture2D* texture = nullptr;
        HRESULT hr = mD3DDevice->CreateTexture2D(&textureDesc, nullptr, &mpTexture2D);
        if (FAILED(hr))
        {
            return false;
        }

        assert(mD3DDevice);
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // Make sure this matches the texture format
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = -1; // Use all MIP levels

        hr = mD3DDevice->CreateShaderResourceView(mpTexture2D, &srvDesc, &mpSRV);
        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }

    bool DynamicTexture::Shutdown()
    {
        if (mpSRV)
            mpSRV->Release();
        mpSRV = nullptr;

        if (mpTexture2D)
            mpTexture2D->Release();
        mpTexture2D = nullptr;

        // tbd
        return false;
    }


    bool DynamicTexture::UpdateTexture(ZBuffer* pSource)
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = mD3DContext->Map(mpTexture2D, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(hr))
        {
            assert(false);
            return false;
        }

        // Copy pixel data
        int64_t width = pSource->GetArea().Width();
        for (UINT row = 0; row < pSource->GetArea().bottom; ++row)
        {
            memcpy((BYTE*)mappedResource.pData + row * mappedResource.RowPitch,
                (BYTE*)pSource->mpPixels + row * width * 4,
                width * 4);
        }

        mD3DContext->Unmap(mpTexture2D, 0);

        return true;
    }


    bool LoadPixelShader(const string& sName, const string& sPath)
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

    bool LoadVertexShader(const string& sName, const string& sPath)
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

    ID3D11VertexShader* GetVertexShader(const std::string& sName)
    {
        tVertexShaderMap::iterator it = mVertexShaderMap.find(sName);
        if (it == mVertexShaderMap.end())
            return nullptr;

        return (*it).second;
    }

    ID3D11PixelShader* GetPixelShader(const std::string& sName)
    {
        tPixelShaderMap::iterator it = mPixelShaderMap.find(sName);
        if (it == mPixelShaderMap.end())
            return nullptr;

        return (*it).second;
    }

    ID3D11InputLayout* GetInputLayout(const std::string& sName)
    {
        tInputLayoutMap::iterator it = mInputLayoutMap.find(sName);
        if (it == mInputLayoutMap.end())
            return nullptr;

        return (*it).second;
    }
};