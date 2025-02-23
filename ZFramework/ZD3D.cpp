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
    IDXGISwapChain1*        mSwapChain = nullptr;
    ID3D11Device*           mD3DDevice = nullptr;
    ID3D11DeviceContext*    mD3DContext = nullptr;
    ID3D11DeviceContext*    mImmediateD3DContext = nullptr;
    IDXGIFactory2*          mFactory = nullptr;
    ID3D11RenderTargetView* mRenderTargetView = nullptr;
    //ID3D11Texture2D*        mBackBuffer = nullptr;
    ID3D11SamplerState*     mSamplerState = nullptr;
    D3D11_VIEWPORT          mViewport;
    tVertexShaderMap        mVertexShaderMap;
    tPixelShaderMap         mPixelShaderMap;
    tInputLayoutMap         mInputLayoutMap;

    ID3D11Buffer*           mVertexBuffer = nullptr;
    ID3D11Texture2D*        mDepthStencilBuffer = nullptr;
    ID3D11DepthStencilView* mDepthStencilView = nullptr;
    tSSPrimArray            mSSPrimArray;
    std::mutex              mPrimitiveMutex;

    ScreenSpacePrimitive*   ReservePrimitive()
    {
        std::unique_lock<mutex> lock(mPrimitiveMutex);

        // if any free primitives return that
        for (auto p : mSSPrimArray)
        {
            if (p->state == ScreenSpacePrimitive::eState::kFree)
            {
                p->state = ScreenSpacePrimitive::eState::kHidden;
                return p.get();
            }
        }

        // reserve more space for primitives
        size_t nextIndex = mSSPrimArray.size();
        mSSPrimArray.resize(mSSPrimArray.size() + 1024);
        for (size_t i = nextIndex; i < nextIndex + 1024; i++)
            mSSPrimArray[i].reset(new ScreenSpacePrimitive);

        mSSPrimArray[nextIndex]->state = ScreenSpacePrimitive::eState::kHidden;
        return mSSPrimArray[nextIndex].get();
    }



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
        if (FAILED(hr)) 
        {
            assert(false);
            return false;
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
            assert(false);
            return false;
        }

        D3D11_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_BACK;
        //rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.FrontCounterClockwise = false;
        rasterDesc.DepthClipEnable = TRUE;
        rasterDesc.DepthBias = 0;  // Increase if you need to push geometry forward
        rasterDesc.DepthBiasClamp = 0.0f;
        rasterDesc.SlopeScaledDepthBias = 0.0f;

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


//        mSSPrimArray.resize(16*1024);


        D3D11_TEXTURE2D_DESC depthStencilBufferDesc = {};
        depthStencilBufferDesc.Width = swapChainDesc.Width;      // Match your backbuffer width
        depthStencilBufferDesc.Height = swapChainDesc.Height;    // Match your backbuffer height
        depthStencilBufferDesc.MipLevels = 1;      // No mip levels
        depthStencilBufferDesc.ArraySize = 1;      // Not a texture array
        depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;  // 24-bit depth, 8-bit stencil
        depthStencilBufferDesc.SampleDesc.Count = 1;  // No MSAA (or match swap chain settings)
        depthStencilBufferDesc.SampleDesc.Quality = 0;
        depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthStencilBufferDesc.CPUAccessFlags = 0;
        depthStencilBufferDesc.MiscFlags = 0;

        hr = mD3DDevice->CreateTexture2D(&depthStencilBufferDesc, nullptr, &mDepthStencilBuffer);
        if (FAILED(hr))
        {
            assert(false);
            return false;
        }

        hr = mD3DDevice->CreateDepthStencilView(mDepthStencilBuffer, nullptr, &mDepthStencilView);
        if (FAILED(hr))
        {
            assert(false);
            return false;
        }

        return true;
    }

    bool ShutdownD3D()
    {
        // Cleanup

        if (mDepthStencilView)
            mDepthStencilView->Release();

        if (mDepthStencilBuffer)
            mDepthStencilBuffer->Release();

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

        if (mVertexBuffer)
            mVertexBuffer->Release();


        if (mSwapChain)
            mSwapChain->Release();
        mSwapChain = nullptr;

        if (mRenderTargetView)
            mRenderTargetView->Release();
        mRenderTargetView = nullptr;

//        if (mBackBuffer)
//            mBackBuffer->Release();
//        mBackBuffer = nullptr;

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


    void UpdateRenderTarget()
    {
        // Release previous render target
        if (mRenderTargetView)
        {
            mRenderTargetView->Release();
            mRenderTargetView = nullptr;
        }

        // Resize swap chain (if needed)
        HRESULT hr = mSwapChain->ResizeBuffers(0, mViewport.Width, mViewport.Height, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(hr))
        {
            return;
        }

        // Get new back buffer
        ID3D11Texture2D* pBackBuffer = nullptr;
        hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
        if (FAILED(hr))
        {
            return;
        }

        // Create a new Render Target View
        hr = mD3DDevice->CreateRenderTargetView(pBackBuffer, nullptr, &mRenderTargetView);
        pBackBuffer->Release(); // Done with back buffer

        if (FAILED(hr))
        {
            return;
        }

        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = TRUE;  // Enable depth testing
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; // Typical depth function

        ID3D11DepthStencilState* depthStencilState = nullptr;
        mD3DDevice->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
        mD3DContext->OMSetDepthStencilState(depthStencilState, 1);



        // Set new render target
//        mD3DContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);
        mD3DContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);


        mD3DContext->RSSetViewports(1, &mViewport);
    }

    bool HandleModeChanges(ZRect& r)
    {
//        if (mBackBuffer)
//            mBackBuffer->Release();
//        mBackBuffer = nullptr;
//        if (mRenderTargetView)
//            mRenderTargetView->Release();
//        mRenderTargetView = nullptr;

        mSwapChain->ResizeBuffers(2, (UINT)r.Width(), (UINT)r.Height(), DXGI_FORMAT_B8G8R8A8_UNORM, 0);

/*        HRESULT hr = mSwapChain->GetBuffer(0, IID_PPV_ARGS(&mBackBuffer));
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

        mD3DContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);*/
        //UpdateRenderTarget();

//        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black color
//        mD3DContext->ClearRenderTargetView(mRenderTargetView, clearColor);









        //    return InitD3D();

        return true;
    }

    bool Present()
    {
        UpdateRenderTarget();

        const FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        mD3DContext->ClearRenderTargetView(mRenderTargetView, clearColor);
        mD3DContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        // run through queued commands. Lock the mutex so that no additional commands can come in the mean time
        std::unique_lock<mutex> lock(mPrimitiveMutex);


        // Set shaders
        mD3DContext->VSSetShader(GetVertexShader("ScreenSpaceShader"), nullptr, 0);
        mD3DContext->PSSetShader(GetPixelShader("ScreenSpaceShader"), nullptr, 0);
        ID3D11InputLayout* layout = GetInputLayout("ScreenSpaceShader");
        mD3DContext->IASetInputLayout(layout);


        for (auto& p : mSSPrimArray)
        {
            if (p->state == ScreenSpacePrimitive::eState::kVisible)
                RenderPrimitive(p.get());
        }

        
        HRESULT hr = mSwapChain->Present(1, 0); // VSync: 1
        if (FAILED(hr))
        {
            assert(false);
        }

        return true;
    }

/*    void AddPrim(ID3D11VertexShader* vs, ID3D11PixelShader* ps, DynamicTexture* tex, const std::vector<Vertex>& verts)
    {
        // Convert screen-space coordinates to Normalized Device Coordinates (NDC)
        std::vector<Vertex> ndcVertices(verts.size());
        for (int i = 0; i < verts.size(); ++i)
        {
            ndcVertices[i].position.x = (verts[i].position.x / (float)mViewport.Width) * 2.0f - 1.0f;
            ndcVertices[i].position.y = 1.0f - (verts[i].position.y / (float)mViewport.Height) * 2.0f;
            ndcVertices[i].position.z = verts[i].position.z; // Keep Z value
            ndcVertices[i].uv = verts[i].uv;
        }


        std::unique_lock<mutex> lock(mPrimitiveMutex);

        ScreenSpacePrimitive* pPrim = ReservePrimitive();
        if (!pPrim)
            return;

        pPrim->ps = ps;
        pPrim->vs = vs;
        pPrim->verts = std::move(ndcVertices);
        pPrim->texture = tex;
    }*/

    void RenderPrimitive(ScreenSpacePrimitive* pPrim)
    {


        /*
        std::array<Vertex, 3> ndcVertices;
        for (int i = 0; i < 3; ++i)
        {
            //ndcVertices[i].position.x = (triangleVertices[i].position.x / (float)mSurfaceArea.Width()) * 2.0f - 1.0f;
            //ndcVertices[i].position.y = 1.0f - (triangleVertices[i].position.y / (float)mSurfaceArea.Height()) * 2.0f;
            ndcVertices[i].position.x = (triangleVertices[i].position.x / (float)mViewport.Width) * 2.0f - 1.0f;
            ndcVertices[i].position.y = 1.0f - (triangleVertices[i].position.y / (float)mViewport.Height) * 2.0f;



            ndcVertices[i].position.z = triangleVertices[i].position.z; // Keep Z value
            ndcVertices[i].uv = triangleVertices[i].uv;
        }*/


        // Create vertex buffer
        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        vertexBufferDesc.ByteWidth = pPrim->verts.size() * sizeof(Vertex);
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA vertexData = {};
        vertexData.pSysMem = pPrim->verts.data();

        ID3D11Buffer* vertexBuffer = nullptr;
        HRESULT hr = mD3DDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);
        if (FAILED(hr))
        {
            HRESULT reason = mD3DDevice->GetDeviceRemovedReason();
            assert(false);
        }




        // Bind vertex buffer
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        mD3DContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        mD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        // Set shaders
        if (pPrim->vs)
            mD3DContext->VSSetShader(pPrim->vs, nullptr, 0);

        if (pPrim->ps)
            mD3DContext->PSSetShader(pPrim->ps, nullptr, 0);

//        ID3D11InputLayout* layout = GetInputLayout("ScreenSpaceShader");
//        mD3DContext->IASetInputLayout(layout);

        

        // Set texture and sampler
        ID3D11ShaderResourceView* pSRV = pPrim->texture->GetSRV(mD3DContext);
        mD3DContext->PSSetShaderResources(0, 1, &pSRV);
        mD3DContext->PSSetSamplers(0, 1, &mSamplerState);

        // Draw triangle
        mD3DContext->Draw(pPrim->verts.size(), 0);
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

        mSourceTexture.Init(size.x, size.y);

        mbNeedsTransferring = true;

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

        mSourceTexture.Shutdown();
        return false;
    }

    ID3D11ShaderResourceView* DynamicTexture::GetSRV(ID3D11DeviceContext* pContext)
    {
        if (mbNeedsTransferring)
            Transfer(pContext);

        return mpSRV;
    }

    ID3D11Texture2D* DynamicTexture::GetTexture(ID3D11DeviceContext* pContext)
    {
        if (mbNeedsTransferring)
            Transfer(pContext);

        return mpTexture2D;
    }



    bool DynamicTexture::Transfer(ID3D11DeviceContext* pContext)
    {

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = pContext->Map(mpTexture2D, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(hr))
        {
            assert(false);
            return false;
        }

        // Copy pixel data
        int64_t width = mSourceTexture.GetArea().Width();
        for (UINT row = 0; row < mSourceTexture.GetArea().bottom; ++row)
        {
            memcpy((BYTE*)mappedResource.pData + row * mappedResource.RowPitch,
                (BYTE*)mSourceTexture.mpPixels + row * width * 4,
                width * 4);
        }

        pContext->Unmap(mpTexture2D, 0);
//        pContext->CopyResource(ZD3D::mBackBuffer, mpTexture2D);

        if (mpSRV)
            mpSRV->Release();

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





        mbNeedsTransferring = false;
        return true;
    }


    bool DynamicTexture::UpdateTexture(ZBuffer* pSource)
    {
        // if new texture is different dimmensions, need to recreate d3d texture
        if (mSourceTexture.GetArea() != pSource->GetArea())
        {
            Shutdown();
            Init(pSource->GetArea().BR());
        }

        mSourceTexture.CopyPixels(pSource);
        mbNeedsTransferring = true;

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