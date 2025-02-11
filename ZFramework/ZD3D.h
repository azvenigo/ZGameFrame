#pragma once

#include "ZTypes.h"
#include "ZDebug.h"
#include "ZBuffer.h"
#ifdef _WIN64
#include <d3d11.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>  // Required for shader compilation
#include <DirectXMath.h>
#endif

class ZScreenBuffer;

typedef std::map<std::string, ID3D11VertexShader*> tVertexShaderMap;
typedef std::map<std::string, ID3D11PixelShader*> tPixelShaderMap;
typedef std::map<std::string, ID3D11InputLayout*> tInputLayoutMap;

struct Vertex {
    DirectX::XMFLOAT3 position; // Already in screen space (x, y, z)
    DirectX::XMFLOAT2 uv;       // Texture coordinates
};



namespace ZD3D
{
    class DynamicTexture
    {
    public:
        DynamicTexture() : mpTexture2D(nullptr), mpSRV(nullptr) {}
        ~DynamicTexture();

        bool            Init(ZPoint size);
        bool            Shutdown();
        bool            UpdateTexture(ZBuffer* pSource);


        ID3D11Texture2D*            mpTexture2D;
        ID3D11ShaderResourceView*   mpSRV;
    };





    bool                    InitD3D(HWND hWnd, ZPoint size);
    bool                    ShutdownD3D();
    bool                    HandleModeChanges(ZRect& r);

    bool                    Present();

    void                    RenderScreenSpaceTriangle(ID3D11ShaderResourceView* textureSRV, const std::array<Vertex, 3>& triangleVertices);

    extern IDXGISwapChain1*        mSwapChain;
    extern ID3D11Device*           mD3DDevice;
    extern ID3D11DeviceContext*    mD3DContext;
    extern IDXGIFactory2*          mFactory;
    extern ID3D11RenderTargetView* mRenderTargetView;
    extern ID3D11Texture2D*        mBackBuffer;
    extern ID3D11SamplerState*     mSamplerState;
    extern D3D11_VIEWPORT          mViewport;


    ID3D11Texture2D*        CreateDynamicTexture(ZPoint size);
    bool                    CreateShaderResourceView(ID3D11Texture2D* texture, ID3D11ShaderResourceView** pTextureSRV);
    ID3D11VertexShader*     GetVertexShader(const std::string& sName);
    ID3D11PixelShader*      GetPixelShader(const std::string& sName);
    ID3D11InputLayout*      GetInputLayout(const std::string& sName);

    bool                    CompileShaders();
    extern tVertexShaderMap        mVertexShaderMap;
    extern tPixelShaderMap         mPixelShaderMap;
    extern tInputLayoutMap         mInputLayoutMap;

    bool                    LoadPixelShader(const std::string& sName, const std::string& sPath);
    bool                    LoadVertexShader(const std::string& sName, const std::string& sPath);  
};
