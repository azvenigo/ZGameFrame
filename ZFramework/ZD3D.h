#pragma once

#include "ZTypes.h"
#include "ZDebug.h"
#include "ZBuffer.h"
#ifdef _WIN64
#include <d3d11.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>  // Required for shader compilation
#endif

class ZScreenBuffer;

typedef std::map<std::string, ID3D11VertexShader*> tVertexShaderMap;
typedef std::map<std::string, ID3D11PixelShader*> tPixelShaderMap;
typedef std::map<std::string, ID3D11InputLayout*> tInputLayoutMap;



namespace ZD3D
{
    bool                    InitD3D();
    bool                    ShutdownD3D();


    IDXGISwapChain1*        mSwapChain;
    ID3D11Device*           mD3DDevice;
    ID3D11DeviceContext*    mD3DContext;
    IDXGIFactory2*          mFactory;
    ID3D11RenderTargetView* mRenderTargetView;
    ID3D11Texture2D*        mBackBuffer;
    ID3D11SamplerState*     mSamplerState;


    ID3D11Texture2D*        CreateDynamicTexture(ZPoint size);
    bool                    UpdateTexture(ID3D11Texture2D* pTexture, ZBuffer* pSource);
    bool                    CreateShaderResourceView(ID3D11Texture2D* texture, ID3D11ShaderResourceView** pTextureSRV);
    ID3D11VertexShader*     GetVertexShader(const std::string& sName);
    ID3D11PixelShader*      GetPixelShader(const std::string& sName);
    ID3D11InputLayout*      GetInputLayout(const std::string& sName);

    bool                    CompileShaders();
    tVertexShaderMap        mVertexShaderMap;
    tPixelShaderMap         mPixelShaderMap;
    tInputLayoutMap         mInputLayoutMap;

    bool                    LoadPixelShader(const std::string& sName, const std::string& sPath);
    bool                    LoadVertexShader(const std::string& sName, const std::string& sPath);  
};
