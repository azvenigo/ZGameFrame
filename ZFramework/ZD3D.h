#pragma once

#include "ZTypes.h"
#include "ZDebug.h"
#include "ZBuffer.h"
#include <vector>
#include <filesystem>
#ifdef _WIN64
#include <d3d11.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>  // Required for shader compilation
#include <DirectXMath.h>
#endif

class ZScreenBuffer;

typedef std::map<std::string, ID3D11VertexShader*> tVertexShaderMap;
typedef std::map<std::string, ID3D11PixelShader*> tPixelShaderMap;
typedef std::map<std::string, ID3D11ComputeShader*> tComputeShaderMap;

typedef std::map<std::string, ID3D11InputLayout*> tInputLayoutMap;

struct Vertex {
    DirectX::XMFLOAT3 position; // Already in screen space (x, y, z)
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 uv;       // Texture coordinates
};


namespace ZD3D
{
    extern IDXGISwapChain1* mSwapChain;
    extern ID3D11Device* mD3DDevice;
    extern ID3D11DeviceContext* mD3DContext;
    //    extern ID3D11DeviceContext*     mImmediateD3DContext;
    extern IDXGIFactory2* mFactory;
    extern ID3D11RenderTargetView* mRenderTargetView;
    //    extern ID3D11Texture2D*         mBackBuffer;
    extern ID3D11SamplerState* mSamplerState;
    extern D3D11_VIEWPORT           mViewport;
    extern ID3D11Texture2D* mDepthStencilBuffer;
    extern ID3D11DepthStencilView* mDepthStencilView;


    class DynamicTexture
    {
    public:
        DynamicTexture() : mpTexture2D(nullptr), mpSRV(nullptr), mbNeedsTransferring(false) {}
        ~DynamicTexture();

        bool            Init(ZPoint size);
        bool            Shutdown();
        bool            UpdateTexture(ZBuffer* pSource);
        //tZBufferPtr     GetSourceTexture() { return mSourceTexture; }
        ID3D11ShaderResourceView* GetSRV(ID3D11DeviceContext* pContext);
        ID3D11Texture2D* GetTexture(ID3D11DeviceContext* pContext);

    private:
        bool            Transfer(ID3D11DeviceContext* pContext);
        ZBuffer         mSourceTexture;
        bool            mbNeedsTransferring;
        ID3D11ShaderResourceView*   mpSRV;
        ID3D11Texture2D* mpTexture2D;
    };

    typedef std::shared_ptr<DynamicTexture> tDynamicTexturePtr;

    class Light
    {
    public:
        Light() 
        {
            clear();
        }

        inline void clear()
        {
            direction = { 0, 0, 0 };
            color = { 0, 0, 0 };
            intensity = 0;
        }

        DirectX::XMFLOAT3 direction;    // Direction of the light
        float intensity;                // Light strength
        DirectX::XMFLOAT3 color;        // Light color (R, G, B)
        float padding;                  // 16 byte alignment;
    };

    struct TimeBufferType 
    {
        float time;
        float padding[3]; // Ensure 16-byte alignment (HLSL requires float4 alignment)
    };

    class ScreenSpacePrimitive
    {
    public:

        enum eState
        {
            kFree = 0,
            kHidden = 1,
            kVisible = 2
        };

        ScreenSpacePrimitive()
        {
            clear();
        }

        inline void clear()
        {
            verts.clear();
            texture.reset();
            texture = nullptr;
            light = nullptr;
            vs = nullptr;
            ps = nullptr;
            state = kFree;
        }

        void SetScreenRect(ZRect r, float z, Light* pLight = nullptr)
        {
            verts.resize(4);


            verts[0].position.x = (r.left / (float)mViewport.Width) * 2.0f - 1.0f;
            verts[0].position.y = 1.0f - (r.top / (float)mViewport.Height) * 2.0f;
            verts[0].position.z = z;
            verts[0].uv.x = 0.0;
            verts[0].uv.y = 0.0;

            verts[1].position.x = (r.right / (float)mViewport.Width) * 2.0f - 1.0f;
            verts[1].position.y = 1.0f - (r.top / (float)mViewport.Height) * 2.0f;
            verts[1].position.z = z;
            verts[1].uv.x = 1.0;
            verts[1].uv.y = 0.0;

            verts[2].position.x = (r.left / (float)mViewport.Width) * 2.0f - 1.0f;
            verts[2].position.y = 1.0f - (r.bottom / (float)mViewport.Height) * 2.0f;
            verts[2].position.z = z;
            verts[2].uv.x = 0.0;
            verts[2].uv.y = 1.0;

            verts[3].position.x = (r.right / (float)mViewport.Width) * 2.0f - 1.0f;
            verts[3].position.y = 1.0f - (r.bottom / (float)mViewport.Height) * 2.0f;
            verts[3].position.z = z;
            verts[3].uv.x = 1.0;
            verts[3].uv.y = 1.0;

            light = pLight;
        }

        void SetVertex(size_t index, float screenX, float screenY, float screenZ, DirectX::XMFLOAT3 normal, float u, float v)
        {
            if (index > verts.size())
                verts.resize(index);
            verts[index].position.x = (screenX / (float)mViewport.Width) * 2.0f - 1.0f;
            verts[index].position.y = 1.0f - (screenY / (float)mViewport.Height) * 2.0f;
            verts[index].position.z = screenZ;
            verts[index].normal = normal;
            verts[index].uv.x = u;
            verts[index].uv.y = v;
        }

        eState                  state;
        std::vector<Vertex>     verts;
        ID3D11VertexShader*     vs;
        ID3D11PixelShader*      ps;
        tDynamicTexturePtr      texture;
        Light*                  light;
    };

    typedef std::shared_ptr<ScreenSpacePrimitive> tScreenSpacePrimitivePtr;
    typedef std::vector<tScreenSpacePrimitivePtr> tSSPrimArray;


    bool                    InitD3D(HWND hWnd, ZPoint size);
    bool                    ShutdownD3D();
    bool                    HandleModeChanges(ZRect& r);

    bool                    Present();

//    void                    AddPrim(ID3D11VertexShader* vs, ID3D11PixelShader* ps, DynamicTexture* tex, const std::vector<Vertex>& verts);
//    void                    AddPrim(ID3D11VertexShader* vs, ID3D11PixelShader* ps, ID3D11ShaderResourceView* tex, const std::vector<Vertex>& verts);
    void                    RenderPrimitive(ScreenSpacePrimitive* pPrim);

    bool                            Blur(ZBuffer* pSrc, ZBuffer* pResult, int radius, float sigma);


    ID3D11Texture2D*                CreateDynamicTexture(ZPoint size);
    bool                            CreateShaderResourceView(ID3D11Texture2D* texture, ID3D11ShaderResourceView** pTextureSRV);
    ID3D11VertexShader*             GetVertexShader(const std::string& sName);
    ID3D11PixelShader*              GetPixelShader(const std::string& sName);
    ID3D11ComputeShader*            GetComputeShader(const std::string& sName);
    ID3D11InputLayout*              GetInputLayout(const std::string& sName);

    bool                            CompileShaders();
    extern tVertexShaderMap         mVertexShaderMap;
    extern tPixelShaderMap          mPixelShaderMap;
    extern tComputeShaderMap        mComputeShaderMap;
    extern tInputLayoutMap          mInputLayoutMap;

    extern ID3D11Buffer*            mVertexBuffer;
    extern TimeBufferType           mTime;
    extern ID3D11Buffer*            mTimeBuffer;

    extern tSSPrimArray             mSSPrimArray;
    ScreenSpacePrimitive*           ReservePrimitive();


    extern ID3D11Buffer* pD3DLightBuffer;   // temp light

    bool                    LoadPixelShader(const std::string& sName, const std::filesystem::path& path);
    bool                    LoadVertexShader(const std::string& sName, const std::filesystem::path& path);
    bool                    LoadComputeShader(const std::string& sName, const std::filesystem::path& path);
};
