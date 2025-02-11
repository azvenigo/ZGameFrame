#pragma once

//#define USE_D3D

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



class ZGraphicSystem
{
public:
    ZGraphicSystem();
    ~ZGraphicSystem();
    
    bool                    Init();
    void                    Shutdown();
    
    // Init settings
    void                    SetArea(ZRect& r)           	{ mrSurfaceArea = r; }
    double                  GetAspectRatio()                { return (double)mrSurfaceArea.Width() / (double)mrSurfaceArea.Height(); }
    ZScreenBuffer*          GetScreenBuffer() 				{ return mpScreenBuffer; }
    
#ifdef _WIN64
    void                    SetHWND(HWND hwnd) { mhWnd = hwnd; }
    HWND                    GetMainHWND() { return mhWnd; }

    // GDI+
    ULONG_PTR               mpGDIPlusToken;
    HWND                    mhWnd;

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

#endif

	bool                    HandleModeChanges(ZRect& r);

    bool                    mbFullScreen;

protected:
    bool                    CompileShaders();
    tVertexShaderMap        mVertexShaderMap;
    tPixelShaderMap         mPixelShaderMap;
    tInputLayoutMap         mInputLayoutMap;

    bool                    LoadPixelShader(const std::string& sName, const std::string& sPath);
    bool                    LoadVertexShader(const std::string& sName, const std::string& sPath);





    bool                    InitD3D();
    bool                    ShutdownD3D();

    bool                    mbInitted;
    ZScreenBuffer*          mpScreenBuffer;
    ZRect                   mrSurfaceArea;
    
};

extern ZGraphicSystem gGraphicSystem;
extern ZGraphicSystem* gpGraphicSystem;
