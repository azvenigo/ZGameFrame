#include "ZD3D.h"
#include <GdiPlus.h>
#include <wrl.h> 
#include <array>
#include <filesystem>
#include "helpers/StringHelpers.h"
#include "ZTimer.h"

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
    tComputeShaderMap       mComputeShaderMap;
    tInputLayoutMap         mInputLayoutMap;

    ID3D11Buffer* pD3DLightBuffer;   // temp light

    ID3D11Buffer*           mVertexBuffer = nullptr;
    ID3D11Texture2D*        mDepthStencilBuffer = nullptr;
    ID3D11DepthStencilView* mDepthStencilView = nullptr;
    tSSPrimArray            mSSPrimArray;
    size_t                  mReservedHighWaterMark = 0;
    std::mutex              mPrimitiveMutex;
    TimeBufferType           mTime;
    ID3D11Buffer*           mTimeBuffer;
    ScreenSpacePrimitive*   ReservePrimitive()
    {
        std::unique_lock<mutex> lock(mPrimitiveMutex);

        // if any free primitives return that

        for (size_t i = 0; i < mSSPrimArray.size(); i++)
        {
            auto& p = mSSPrimArray[i];
            if (p->state == ScreenSpacePrimitive::eState::kFree)
            {
                if (mReservedHighWaterMark < i+1)
                    mReservedHighWaterMark = i+1;
                p->state = ScreenSpacePrimitive::eState::kHidden;
                return p.get();
            }
        }

        // reserve more space for primitives
        size_t nextIndex = mSSPrimArray.size();
        mSSPrimArray.resize(mSSPrimArray.size() + 1024);
        for (size_t i = nextIndex; i < nextIndex + 1024; i++)
            mSSPrimArray[i].reset(new ScreenSpacePrimitive);

        mReservedHighWaterMark = nextIndex+1;

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



        D3D11_BUFFER_DESC timeBufferDesc = {};
        timeBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        timeBufferDesc.ByteWidth = sizeof(TimeBufferType);
        timeBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        timeBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        timeBufferDesc.MiscFlags = 0;
        timeBufferDesc.StructureByteStride = 0;

        HRESULT result = mD3DDevice->CreateBuffer(&timeBufferDesc, nullptr, &mTimeBuffer);
        if (FAILED(result)) {
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

        for (auto p : mComputeShaderMap)
        {
            (p.second)->Release();
        }
        mComputeShaderMap.clear();

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
        HRESULT hr;
        // Resize swap chain (if needed)
/*        hr = mSwapChain->ResizeBuffers(0, mViewport.Width, mViewport.Height, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(hr))
        {
            return;
        }*/

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


        float currentTime = static_cast<float>(gTimer.GetElapsedSecondsSinceEpoch()); // Replace with your timing function

        mTime.time = currentTime;

        // Map the buffer and copy data
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT result = mD3DContext->Map(mTimeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(result)) 
        {
            // Handle error
        }
        memcpy(mappedResource.pData, &mTime.time, sizeof(TimeBufferType));
        mD3DContext->Unmap(mTimeBuffer, 0);

        // Set the buffer to the shader
        mD3DContext->PSSetConstantBuffers(1, 1, &mTimeBuffer);


        // Set shaders
        mD3DContext->VSSetShader(GetVertexShader("ScreenSpaceShader"), nullptr, 0);
        mD3DContext->PSSetShader(GetPixelShader("ScreenSpaceShader"), nullptr, 0);
        ID3D11InputLayout* layout = GetInputLayout("ScreenSpaceShader");
        mD3DContext->IASetInputLayout(layout);

        for (size_t i = 0; i < mReservedHighWaterMark; i++)
        {
            auto& p = mSSPrimArray[i];
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
        
        if (pPrim->light && pD3DLightBuffer)
        {
            D3D11_MAPPED_SUBRESOURCE mappedResource;

            mD3DContext->Map(pD3DLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

            memcpy(mappedResource.pData, pPrim->light, sizeof(Light));
            mD3DContext->Unmap(pD3DLightBuffer, 0);

            mD3DContext->PSSetConstantBuffers(0, 1, &pD3DLightBuffer);
        }
        
        // Set texture and sampler
        ID3D11ShaderResourceView* pSRV = pPrim->texture->GetSRV(mD3DContext);
        mD3DContext->PSSetShaderResources(0, 1, &pSRV);
        mD3DContext->PSSetSamplers(0, 1, &mSamplerState);

        // Draw triangle
        mD3DContext->Draw(pPrim->verts.size(), 0);
        vertexBuffer->Release();
    }


    bool Blur(ZBuffer* pSrc, ZBuffer* pResult, int radius, float sigma)
    {
        HRESULT hr;

        // Get the blur shader
        ID3D11ComputeShader* pComputeShaderH = GetComputeShader("GaussianBlur_Horizontal");
        if (!pComputeShaderH)
        {
            cout << "GaussianBlur_Horizontal shader not loaded\n";
            assert(false);
            return false;
        }

        ID3D11ComputeShader* pComputeShaderV = GetComputeShader("GaussianBlur_Vertical");
        if (!pComputeShaderV)
        {
            cout << "GaussianBlur_Vertical shader not loaded\n";
            assert(false);
            return false;
        }



        ZRect rSrc(pSrc->GetArea());
        int64_t w = rSrc.Width();
        int64_t h = rSrc.Height();

        // Create staging texture for transfering CPU to GPU memory
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = w;
        texDesc.Height = h;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        texDesc.BindFlags = 0;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.SampleDesc.Count = 1;




        D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
        stagingDesc.BindFlags = 0;  // No binding flags for a staging texture



        ID3D11Texture2D* pStaging = nullptr;
        mD3DDevice->CreateTexture2D(&stagingDesc, nullptr, &pStaging);
        assert(pStaging);

        D3D11_MAPPED_SUBRESOURCE mappedResource;

        // copy source to staging texture
        std::vector<float> floatPixels(w * h * 4); // 4 floats per pixel (RGBA)
       for (size_t i = 0; i < w * h; i++) 
        {
            uint32_t argb = pSrc->mpPixels[i]; // Assuming srcPixels is uint32_t*
            floatPixels[i * 4 + 0] = ((argb >> 16) & 0xFF) / 255.0f; // R
            floatPixels[i * 4 + 1] = ((argb >> 8) & 0xFF) / 255.0f;  // G
            floatPixels[i * 4 + 2] = ((argb >> 0) & 0xFF) / 255.0f;  // B
            floatPixels[i * 4 + 3] = ((argb >> 24) & 0xFF) / 255.0f; // A
        }
        mD3DContext->Map(pStaging, 0, D3D11_MAP_WRITE, 0, &mappedResource);
        float* dest = reinterpret_cast<float*>(mappedResource.pData);
        size_t rowPitch = mappedResource.RowPitch / sizeof(float);
        for (size_t y = 0; y < h; y++) 
        {
            memcpy(reinterpret_cast<char*>(dest) + y * mappedResource.RowPitch, &floatPixels[y * w * 4], w * 4 * sizeof(float));
        }
        mD3DContext->Unmap(pStaging, 0);

        


        


        struct BlurParams
        {
            float texelSize[2]; // 1 / width, 1 / height
            float sigma;
            int radius;
            int width;
            int height;
            float padding[10]; // 16-byte alignment
        };

        BlurParams blurParams = {};
        blurParams.texelSize[0] = 1.0f / w;
        blurParams.texelSize[1] = 1.0f / h;
        blurParams.sigma = sigma;
        blurParams.radius = radius;
        blurParams.width = w;
        blurParams.height = h;

        ID3D11Buffer* pConstantBuffer = nullptr;

        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.ByteWidth = sizeof(BlurParams);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        mD3DDevice->CreateBuffer(&cbDesc, nullptr, &pConstantBuffer);
        assert(pConstantBuffer);

        // Map and write to the buffer
        mD3DContext->Map(pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, &blurParams, sizeof(BlurParams));
        mD3DContext->Unmap(pConstantBuffer, 0);

        // Bind the buffer to the compute shader
        mD3DContext->CSSetConstantBuffers(0, 1, &pConstantBuffer);










        D3D11_TEXTURE2D_DESC srcDesc = {};
        srcDesc.Width = w;
        srcDesc.Height = h;
        srcDesc.MipLevels = 1;
        srcDesc.ArraySize = 1;
        srcDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;  // Float format for compute shader compatibility
        srcDesc.SampleDesc.Count = 1;
        srcDesc.Usage = D3D11_USAGE_DEFAULT;
        srcDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;  // Only read access in shader
        srcDesc.CPUAccessFlags = 0;  // No CPU read/write
        srcDesc.MiscFlags = 0;

        ID3D11Texture2D* pBlurSrcTexture = nullptr;
        mD3DDevice->CreateTexture2D(&srcDesc, nullptr, &pBlurSrcTexture);




        // copy from staging to blur source
        mD3DContext->CopyResource(pBlurSrcTexture, pStaging);


        // create and bind a SRV for the blur input texture
        ID3D11ShaderResourceView* pSrcSRV = nullptr;
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        mD3DDevice->CreateShaderResourceView(pBlurSrcTexture, &srvDesc, &pSrcSRV);
        assert(pSrcSRV);








        D3D11_TEXTURE2D_DESC texture_desc = {};
        texture_desc.Width = w;
        texture_desc.Height = h;
        texture_desc.MipLevels = 1;
        texture_desc.ArraySize = 1;
        texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;  // Must match src format
        texture_desc.SampleDesc.Count = 1;
        texture_desc.Usage = D3D11_USAGE_DEFAULT;
        texture_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;  // Only write access in shader
        texture_desc.CPUAccessFlags = 0;  // No CPU read/write
        texture_desc.MiscFlags = 0;


        ID3D11Texture2D* pBlurDstTexture = nullptr;
        mD3DDevice->CreateTexture2D(&texture_desc, nullptr, &pBlurDstTexture);









        D3D11_TEXTURE2D_DESC temp_texture_desc = {};
        temp_texture_desc.Width = w;
        temp_texture_desc.Height = h;
        temp_texture_desc.MipLevels = 1;
        temp_texture_desc.ArraySize = 1;
        temp_texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;  // Must match src format
        temp_texture_desc.SampleDesc.Count = 1;
        temp_texture_desc.Usage = D3D11_USAGE_DEFAULT;
        temp_texture_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        temp_texture_desc.CPUAccessFlags = 0;  // No CPU read/write
        temp_texture_desc.MiscFlags = 0;


        ID3D11Texture2D* pTempTexture = nullptr;
        mD3DDevice->CreateTexture2D(&temp_texture_desc, nullptr, &pTempTexture);







        ID3D11UnorderedAccessView* pDstUAV = nullptr;
        ID3D11UnorderedAccessView* pTempUAV = nullptr;
        ID3D11ShaderResourceView* pTempSRV = nullptr;


        ID3D11UnorderedAccessView* pNullUAV = nullptr;
        ID3D11ShaderResourceView* pNullSRV = nullptr;



        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;

        mD3DDevice->CreateUnorderedAccessView(pBlurDstTexture, &uavDesc, &pDstUAV);
        mD3DDevice->CreateUnorderedAccessView(pTempTexture, &uavDesc, &pTempUAV);

        mD3DDevice->CreateShaderResourceView(pTempTexture, &srvDesc, &pTempSRV);
        assert(pTempSRV);
        






        // Perform blur


        mD3DContext->CSSetShader(pComputeShaderH, nullptr, 0);
        mD3DContext->CSSetShaderResources(0, 1, &pSrcSRV);
        mD3DContext->CSSetUnorderedAccessViews(0, 1, &pTempUAV, nullptr);

        UINT groupsX = (w + 15) / 16;  // Divide by thread group size
        UINT groupsY = (h + 15) / 16;

        mD3DContext->Dispatch(groupsX, groupsY, 1);
        

                   
        mD3DContext->CSSetUnorderedAccessViews(0, 1, &pNullUAV, nullptr);
        mD3DContext->CSSetShaderResources(0, 1, &pNullSRV);

        mD3DContext->CSSetShader(pComputeShaderV, nullptr, 0);
        mD3DContext->CSSetShaderResources(0, 1, &pTempSRV);
        mD3DContext->CSSetUnorderedAccessViews(0, 1, &pDstUAV, nullptr);

        // Update and set constant buffer (same params)
        mD3DContext->CSSetConstantBuffers(0, 1, &pConstantBuffer);

        mD3DContext->Dispatch(groupsX, groupsY, 1);
        








        mD3DContext->Flush();


        // copy back out to staging
        mD3DContext->CopyResource(pStaging, pBlurDstTexture);


        ID3D11ShaderResourceView* nullSRV = nullptr;
        mD3DContext->CSSetShaderResources(0, 1, &nullSRV);
        mD3DContext->CSSetUnorderedAccessViews(0, 1, &pNullUAV, nullptr);






        hr = mD3DContext->Map(pStaging, 0, D3D11_MAP_READ, 0, &mappedResource);

        if (SUCCEEDED(hr))
        {
            float* data = reinterpret_cast<float*>(mappedResource.pData);
            int rowPitch = mappedResource.RowPitch / sizeof(float);  // Convert bytes to float count

            pResult->Init(w, h);

            // Copy to your own buffer (assuming width * height size)
            for (int y = 0; y < h; y++)
            {
//                memcpy(&floatPixels[y * w * 4], reinterpret_cast<char*>(data) + y * mappedResource.RowPitch, w * 4 * sizeof(float));


                for (int x = 0; x < w; x++)
                {
                    int iin = y * rowPitch + x*4;
                    int iout = y * w + x;

                    uint32_t a = (uint32_t)(data[iin + 3] * 255.0);
                    uint32_t r = (uint32_t)(data[iin + 0] * 255.0);
                    uint32_t g = (uint32_t)(data[iin + 1] * 255.0);
                    uint32_t b = (uint32_t)(data[iin + 2] * 255.0);
                    pResult->mpPixels[iout] = ARGB(a, r, g, b);
                }
            }

            mD3DContext->Unmap(pStaging, 0);
        }

        pConstantBuffer->Release();
        pStaging->Release();
        pBlurSrcTexture->Release();
        pDstUAV->Release();
    }



    bool CompileShaders()
    {
        if (!fs::exists("res/shaders/"))
        {
            cout << "No shaders folder\n";
            return true;
        }

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

                    char c = p.filename().string()[0];

                    if (c == 'p')
                        LoadPixelShader(sName, p.string());
                    else if (c == 'v')
                        LoadVertexShader(sName, p.string());
                    else if (c == 'c')
                        LoadComputeShader(sName, p.string());
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
                assert(false);
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


    bool LoadComputeShader(const string& sName, const string& sPath)
    {
        assert(mD3DDevice);

        ID3DBlob* psBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;

        // Compile Compute Shader
        HRESULT hr = D3DCompileFromFile(SH::string2wstring(sPath).c_str(), nullptr, nullptr, "CSMain", "cs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &psBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                cerr << (char*)errorBlob->GetBufferPointer() << "\n";
                assert(false);
                errorBlob->Release();
            }

            return false;
        }

        // Create Compute Shader
        ID3D11ComputeShader* pShader = nullptr;
        hr = mD3DDevice->CreateComputeShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pShader);
        if (FAILED(hr))
        {
            assert(false);
            return false;
        }

        mComputeShaderMap[sName] = pShader;

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
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

    ID3D11ComputeShader* GetComputeShader(const std::string& sName)
    {
        tComputeShaderMap::iterator it = mComputeShaderMap.find(sName);
        if (it == mComputeShaderMap.end())
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