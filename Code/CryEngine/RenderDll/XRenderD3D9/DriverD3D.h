/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once


#include "Cry_XOptimise.h"
#include "IWindowMessageHandler.h"

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define DRIVERD3D_H_SECTION_2 2
#define DRIVERD3D_H_SECTION_3 3
#define DRIVERD3D_H_SECTION_4 4
#define DRIVERD3D_H_SECTION_5 5
#define DRIVERD3D_H_SECTION_6 6
#define DRIVERD3D_H_SECTION_7 7
#define DRIVERD3D_H_SECTION_8 8
#define DRIVERD3D_H_SECTION_9 9
#define DRIVERD3D_H_SECTION_10 10
#define DRIVERD3D_H_SECTION_11 11
#define DRIVERD3D_H_SECTION_12 12
#endif

# if !defined(_RELEASE)
# define ENABLE_CONTEXT_THREAD_CHECKING 0
# endif

# if !defined(ENABLE_CONTEXT_THREAD_CHECKING)
# define ENABLE_CONTEXT_THREAD_CHECKING 0
# endif

/*
===========================================
The DXRenderer interface Class
===========================================
*/

_inline int _VertBufferSize(D3DBuffer* pVB)
{
    if (!pVB)
    {
        return 0;
    }
    D3D11_BUFFER_DESC Desc;
    pVB->GetDesc(&Desc);
    return Desc.ByteWidth;
}
_inline int _IndexBufferSize(D3DBuffer* pIB)
{
    if (!pIB)
    {
        return 0;
    }
    D3D11_BUFFER_DESC Desc;
    pIB->GetDesc(&Desc);
    return Desc.ByteWidth;
}


#define VERSION_D3D 2.0

struct SPixFormat;
struct SGraphicsPiplinePassContext;

#include "D3DRenderAuxGeom.h"
#include "D3DColorGradingController.h"
#include "D3DStereo.h"
#include "GraphicsPipeline/StandardGraphicsPipeline.h"
#include <RenderDll/Common/PerInstanceConstantBufferPool.h>
#include "D3DDeferredShading.h"
#include "D3DTiledShading.h"
#include "D3DVolumetricFog.h"
#include "GPUTimer.h"
#include "PipelineProfiler.h"
#include "D3DDebug.h"
#include "DeviceInfo.h"

//=======================================================================

#include <memory> // std::unique_ptr
#include <utility> // std::forward

template<typename T, typename ... TArgs>
inline std::unique_ptr<T> CryMakeUnique(TArgs&& ... args)
{
    return std::make_unique<T>(std::forward<TArgs>(args) ...);
}


struct SD3DContext
{
    HWND m_hWnd;
    int m_X;
    int m_Y;
    // Real offscreen target width for rendering
    int m_Width;
    // Real offscreen target height for rendering
    int m_Height;
    IDXGISwapChain* m_pSwapChain;
    std::vector<D3DSurface*> m_pBackBuffers;
    D3DSurface* m_pBackBuffer;
    unsigned int m_pCurrentBackBufferIndex;
    // Width of viewport on screen to display rendered content in
    int m_nViewportWidth;
    // Height of viewport on screen to display rendered content in
    int m_nViewportHeight;
    // Pixel resolution scale in X, includes scale from r_SuperSampling and any operating system screen or viewport scale
    float m_fPixelScaleX;
    // Pixel resolution scale in Y, includes scale from r_SuperSampling and any operating system screen or viewport scale
    float m_fPixelScaleY;
    // Denotes if context refers to main viewport
    bool m_bMainViewport;
};

// Texture coordinate rectangle
struct CoordRect
{
    float fLeftU, fTopV;
    float fRightU, fBottomV;
};

bool DrawFullScreenQuad(float fLeftU, float fTopV, float fRightU, float fBottomV, bool bClampToScreenRes = true);
bool DrawFullScreenQuad(CoordRect c, bool bClampToScreenRes = true);


struct SStateBlend
{
    UnINT64 nHashVal;
    D3D11_BLEND_DESC Desc;
    ID3D11BlendState* pState;

    SStateBlend() { memset(this, 0, sizeof(*this)); }

    static uint64 GetHash(const D3D11_BLEND_DESC& InDesc)
    {
        UnINT64 nHash;
        nHash.i.Low = InDesc.AlphaToCoverageEnable |
            (InDesc.RenderTarget[0].BlendEnable << 1) | (InDesc.RenderTarget[1].BlendEnable << 2) | (InDesc.RenderTarget[2].BlendEnable << 3) | (InDesc.RenderTarget[3].BlendEnable << 4) |
            (InDesc.RenderTarget[0].SrcBlend << 5) | (InDesc.RenderTarget[0].DestBlend << 10) |
            (InDesc.RenderTarget[0].SrcBlendAlpha << 15) | (InDesc.RenderTarget[0].DestBlendAlpha << 20) |
            (InDesc.RenderTarget[0].BlendOp << 25) | (InDesc.RenderTarget[0].BlendOpAlpha << 28);
        nHash.i.High = InDesc.RenderTarget[0].RenderTargetWriteMask |
            (InDesc.RenderTarget[1].RenderTargetWriteMask << 4) |
            (InDesc.RenderTarget[2].RenderTargetWriteMask << 8) |
            (InDesc.RenderTarget[3].RenderTargetWriteMask << 12) |
            InDesc.IndependentBlendEnable << 16;

        return nHash.SortVal;
    }
} _ALIGN(16);


struct SStateRaster
{
    uint64 nValuesHash;
    uint32 nHashVal;
    ID3D11RasterizerState* pState;
    D3D11_RASTERIZER_DESC Desc;

    SStateRaster()
    {
        memset(this, 0, sizeof(*this));
        Desc.DepthClipEnable = true;
        Desc.FillMode = D3D11_FILL_SOLID;
        Desc.FrontCounterClockwise = TRUE;
    }

    static uint32 GetHash(const D3D11_RASTERIZER_DESC& InDesc)
    {
        uint32 nHash;
        nHash =      InDesc.FillMode | (InDesc.CullMode << 2) |
            (InDesc.DepthClipEnable << 4) | (InDesc.FrontCounterClockwise << 5) |
            (InDesc.ScissorEnable << 6) | (InDesc.MultisampleEnable << 7) | (InDesc.AntialiasedLineEnable << 8) |
            (InDesc.DepthBias << 9);
        return nHash;
    }

    static uint64 GetValuesHash(const D3D11_RASTERIZER_DESC& InDesc)
    {
        uint64 nHash;
        //avoid breaking strict alising rules
        union f32_u
        {
            float floatVal;
            unsigned int uintVal;
        };
        f32_u uDepthBiasClamp;
        uDepthBiasClamp.floatVal = InDesc.DepthBiasClamp;
        f32_u uSlopeScaledDepthBias;
        uSlopeScaledDepthBias.floatVal = InDesc.SlopeScaledDepthBias;
        nHash = (((uint64)uDepthBiasClamp.uintVal) |
                 ((uint64)uSlopeScaledDepthBias.uintVal) << 32);
        return nHash;
    }
} _ALIGN(16);


_inline uint32 sStencilState(const D3D11_DEPTH_STENCILOP_DESC& Desc)
{
    uint32 nST = (Desc.StencilFailOp << 0) |
        (Desc.StencilDepthFailOp << 4) |
        (Desc.StencilPassOp << 8) |
        (Desc.StencilFunc << 12);
    return nST;
}
struct SStateDepth
{
    uint64 nHashVal;
    D3D11_DEPTH_STENCIL_DESC Desc;
    ID3D11DepthStencilState* pState;
    SStateDepth()
        : nHashVal()
        , pState()
    {
        Desc.DepthEnable      = TRUE;
        Desc.DepthWriteMask   = D3D11_DEPTH_WRITE_MASK_ALL;
        Desc.DepthFunc        = D3D11_COMPARISON_LESS;
        Desc.StencilEnable    = FALSE;
        Desc.StencilReadMask  = D3D11_DEFAULT_STENCIL_READ_MASK;
        Desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

        Desc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
        Desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        Desc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        Desc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;

        Desc.BackFace = Desc.FrontFace;
    }

    static uint64 GetHash(const D3D11_DEPTH_STENCIL_DESC& InDesc)
    {
        uint64 nHash;
        nHash = (InDesc.DepthEnable << 0) |
            (InDesc.DepthWriteMask << 1) |
            (InDesc.DepthFunc << 2) |
            (InDesc.StencilEnable << 6) |
            (InDesc.StencilReadMask << 7) |
            (InDesc.StencilWriteMask << 15) |
            (((uint64)sStencilState(InDesc.FrontFace)) << 23) |
            (((uint64)sStencilState(InDesc.BackFace)) << 39);
        return nHash;
    }
} _ALIGN(16); \

#if defined(AZ_PLATFORM_ANDROID)
#define MAX_OCCL_QUERIES    256
#else
#define MAX_OCCL_QUERIES    4096
#endif

#define MAXFRAMECAPTURECALLBACK 1

//======================================================================

// Options for clearing
#define CLEAR_ZBUFFER           0x00000001  /* Clear target z buffer, equals D3D11_CLEAR_DEPTH */
#define CLEAR_STENCIL           0x00000002  /* Clear stencil planes, equals D3D11_CLEAR_STENCIL */
#define CLEAR_RTARGET           0x00000004  /* Clear target surface */

//======================================================================
//======================================================================
/// Forward declared classes

class CHWShader_D3D;

//======================================================================
/// Direct3D Render driver class

#ifdef SHADER_ASYNC_COMPILATION
class CAsyncShaderTask;
#endif

AZStd::vector<D3D11_INPUT_ELEMENT_DESC> GetD3D11Declaration(const AZ::Vertex::Format& vertexFormat);

class CD3D9Renderer
    : public CRenderer
    , public IWindowMessageHandler
    , AZ::RenderNotificationsBus::Handler
    , AZ::RenderScreenshotRequestBus::Handler
{
    friend struct SPixFormat;
    friend class CD3DStereoRenderer;
    friend class CTexture;
    friend class CSceneGBufferPass;

public:
    CD3D9Renderer();
    ~CD3D9Renderer();

    virtual SRenderPipeline* GetRenderPipeline() override { return &m_RP; }

    static void StaticCleanup();

    // Remove pointer indirection.
    ILINE CD3D9Renderer* operator->()
    {
        return this;
    }

    ILINE const bool operator !() const
    {
        return false;
    }

    ILINE operator bool() const
    {
        return true;
    }

    ILINE operator CD3D9Renderer*()
    {
        return this;
    }

    virtual void InitRenderer();
    virtual void Release();

    virtual const SRenderTileInfo* GetRenderTileInfo() const override { return &m_RenderTileInfo; }

    // CRY DX12
    static unsigned int GetCurrentBackBufferIndex(IDXGISwapChain* pSwapChain);

    struct SCharInstCB
    {
        AzRHI::ConstantBuffer* m_buffer;
        SSkinningData* m_pSD;
        util::list<SCharInstCB> list;
        bool updated;

        SCharInstCB()
            : m_buffer()
            , m_pSD()
            , list()
            , updated()
        {
        }

        ~SCharInstCB() { SAFE_RELEASE(m_buffer); list.erase(); }
    };
protected:

    // Invalidates temporal data generated by the renderer that is used by the coverage buffer system.
    // This should be called whenever a level load happens, a camera teleport occurs, etc.
    void InvalidateCoverageBufferData();

    // Windows context
    char      m_WinTitle[80];
    HINSTANCE m_hInst;
    HWND      m_hWnd;            // The main app window
    HWND      m_hWndDesktop;     // The desktop window
#ifdef WIN32
    HICON     m_hIconBig;        // Icon currently being used on the taskbar
    HICON     m_hIconSmall;      // Icon currently being used on the window
    string    m_iconPath;        // Path to the icon currently loaded
#endif

    //stereo mode
    HWND      m_hWnd2;

    int m_bDraw2dImageStretchMode;
    uint32 m_uLastBlendFlagsPassGroup;

    int m_numOcclusionDownsampleStages;

    // Cached size of the depth target previously used for generating the depth downsample chain
    uint16 m_occlusionSourceSizeX;
    uint16 m_occlusionSourceSizeY; 
        
    // Data to be populated by the render thread and read by the coverage buffer occlusion thread
    struct OcclusionReadbackData
    {
        ~OcclusionReadbackData();

        void Destroy();
        void Reset(bool reverseDepth);

        // Matrix used to generate the data in the occlusion readback buffer
        Matrix44A m_occlusionReadbackViewProj;

        // Contains modified depth data from m_zTargetReadback.  Buffer to only be used by the Coverage Buffer system.
        float* m_occlusionReadbackBuffer = nullptr;
    };

    // Packet of data used by the renderer in order to generate occlusion data for the Coverage Buffer system.
    struct CPUOcclusionData
    {
        enum class OcclusionDataState : AZ::u8
        {
            OcclusionDataInvalid = 0,

            // Occlusion data is ready for use on the GPU
            OcclusionDataOnGPU,

            // Occlusion data has been read back on the CPU and is ready for use
            OcclusionDataOnCPU
        };

        void SetupOcclusionData(const char* textureName);
        void Destroy();

        // Matrix used to render the Z-buffer that is downsampled into m_zTargetReadback
        Matrix44A m_occlusionViewProj;

        // Buffer containing downsampled depth buffer information to be read back by the CPU
        CTexture* m_zTargetReadback = nullptr;

        // Data to be read by the occlusion thread
        OcclusionReadbackData m_occlusionReadbackData;

        // Contains whether or not the occlusion data is valid.  Occlusion data should be invalidated on level loads, camera teleports, etc.
        OcclusionDataState m_occlusionDataState = OcclusionDataState::OcclusionDataInvalid;
    };

    // Triple buffer our downsampled texture that is used for CPU readbacks to prevent CPU/GPU resource contention
    static const unsigned int s_numOcclusionReadbackTextures = 3;

    // Set of data generated by FX_ZTargetReadBackOnCPU to be used by the Coverage Buffer system.
    // Data is triple buffered by default to prevent CPU/GPU synchronization
    CPUOcclusionData m_occlusionData[s_numOcclusionReadbackTextures];

    // Index into m_occlusionData for which CPU data set PrepareOcclusion should read from.
    // Will be set from the render thread and read from the occlusion thread
    AZStd::atomic_uchar m_cpuOcclusionReadIndex;
    
    // Current GPU write index into the occlusion data array
    AZ::u8 m_occlusionBufferIndex = 0;
    
    // The Coverage Buffer system in CCullRenderer is templated based on resolution, so this will not change at runtime
    static const uint16 s_occlusionBufferWidth = CULL_SIZEX;
    static const uint16 s_occlusionBufferHeight = CULL_SIZEY;
    static const uint32 s_occlusionBufferNumElements = CULL_SIZEX * CULL_SIZEY;
    
    CStandardGraphicsPipeline* m_GraphicsPipeline;
    CTiledShading* m_pTiledShading;
    CD3DStereoRenderer* m_pStereoRenderer;
    CVolumetricFog     m_volumetricFog;
    std::vector<D3DSurface*> m_pBackBuffers;
    D3DSurface* m_pBackBuffer;
    unsigned int m_pCurrentBackBufferIndex;
    ID3D11RenderTargetView* m_pSecondBackBuffer;
    D3DDepthSurface* m_pZBuffer;
    D3DDepthSurface* m_pNativeZBuffer;
    D3DTexture* m_pZTexture;
    D3DTexture* m_pNativeZTexture;

    PerInstanceConstantBufferPool m_PerInstanceConstantBufferPool;

    volatile int m_lockCharCB;

    // CharCB can have different sizes depending on bone type.
    // Therefore we need to keep a list per bone type for re-usability
    util::list<SCharInstCB> m_CharCBFreeList[eBoneType_Count];
    util::list<SCharInstCB> m_CharCBActiveList[eBoneType_Count][3];

    volatile int m_CharCBFrameRequired[3];
    volatile int m_CharCBAllocated;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_H_SECTION_2
    #include AZ_RESTRICTED_FILE(DriverD3D_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    IDXGISwapChain*  m_pSwapChain;
#endif
    enum PresentStatus
    {
        epsOccluded         = 1 << 0,
        epsNonExclusive = 1 << 1,
    };
    DWORD             m_dwPresentStatus;     // Indicate present status

    DWORD             m_dwWindowStyle;   // Saved window style for mode switches

    int               m_SceneRecurseCount;

    SRenderTileInfo m_RenderTileInfo;

    struct C2dImage
    {
        CTexture* pTex;
        CTexture* pTarget;
        float xpos, ypos, w, h, s0, t0, s1, t1, angle, z, stereoDepth;
        DWORD col;

        C2dImage(float _xpos, float _ypos, float _w, float _h, CTexture* _pTex, float _s0, float _t0, float _s1, float _t1, float _angle, DWORD _col, float _z, float _stereoDepth, CTexture* _pTarget = NULL)
            : pTex(_pTex)
            , xpos(_xpos)
            , ypos(_ypos)
            , w(_w)
            , h(_h)
            , s0(_s0)
            , t0(_t0)
            , s1(_s1)
            , t1(_t1)
            , angle(_angle)
            , z(_z)
            , col(_col)
            , stereoDepth(_stereoDepth)
            , pTarget(_pTarget) {}
    };
    TArray<C2dImage>   m_2dImages;
    // CRY DX12
    TArray<C2dImage>   m_uiImages;
    //==================================================================

public:
    enum
    {
        MAX_FRAME_QUERIES = 2
    };
    D3DQuery* m_pQuery[MAX_FRAME_QUERIES];


#if !defined(_RELEASE)
    // sprite cells referenced this frame (particular sprite in particular atlas)
    std::set<uint32> m_SpriteCellsUsed;
    // sprite atlases referenced this frame
    std::set<CTexture*> m_SpriteAtlasesUsed;
#endif

    TArray<COcclusionQuery> m_OcclQueries;
    uint32 m_OcclQueriesUsed;

    enum EDefShadows_Passes
    {
        DS_STENCIL_PASS = 0,
        DS_HISTENCIL_REFRESH = 1,
        DS_SHADOW_PASS = 2,
        DS_SHADOW_CULL_PASS = 3,
        DS_SHADOW_FRUSTUM_CULL_PASS = 4,
        DS_STENCIL_VOLUME_CLIP = 5,
        DS_CLOUDS_SEPARATE = 6,
        DS_VOLUME_SHADOW_PASS = 7,

        // DS_GMEM_STENCIL_CULL_NON_CONVEX used by CD3D9Renderer::FX_StencilCullNonConvex(...)
        // in D3DDeferredShading.cpp when using GMEM render path.
        DS_GMEM_STENCIL_CULL_NON_CONVEX = 8,

        // DS_STENCIL_CULL_NON_CONVEX_RESOLVE used by CD3D9Renderer::FX_StencilCullNonConvex(...)
        // in D3DDeferredShading.cpp when stencil texture is not supported.
        DS_STENCIL_CULL_NON_CONVEX_RESOLVE = 9,

        DS_SHADOW_CULL_PASS_FRONTFACING = 10,
        DS_SHADOW_FRUSTUM_CULL_PASS_FRONTFACING = 11,
        DS_STENCIL_VOLUME_CLIP_FRONTFACING = 12,

        DS_PASS_MAX
    };

#if defined(SUPPORT_D3D_DEBUG_RUNTIME)
    CD3DDebug m_d3dDebug;
    bool m_bUpdateD3DDebug;
#endif

    DWORD m_DeviceOwningthreadID;                   // thread if of thread who is allows to access the D3D Context

    ID3D11InputLayout* m_pLastVDeclaration;

    DXGI_SURFACE_DESC  m_d3dsdBackBuffer; // Surface desc of the BackBuffer
    DXGI_FORMAT        m_ZFormat;


    D3D11_PRIMITIVE_TOPOLOGY m_CurTopology;

    TArray<SStateBlend> m_StatesBL;
    TArray<SStateRaster> m_StatesRS;
    TArray<SStateDepth> m_StatesDP;
    uint32 m_nCurStateBL;
    uint32 m_nCurStateRS;
    uint32 m_nCurStateDP;
    uint8 m_nCurStencRef;

    inline uint32 GetOrCreateBlendState(const D3D11_BLEND_DESC& desc)
    {
        uint32 i;
        HRESULT hr = S_OK;
        uint64 nHash = SStateBlend::GetHash(desc);
        for (i = 0; i < m_StatesBL.Num(); i++)
        {
            if (m_StatesBL[i].nHashVal.SortVal == nHash)
            {
                break;
            }
        }
        if (i == m_StatesBL.Num())
        {
            SStateBlend* pState = m_StatesBL.AddIndex(1);
            pState->Desc = desc;
            pState->nHashVal.SortVal = nHash;
            hr = GetDevice().CreateBlendState(&pState->Desc, &pState->pState);
            assert(SUCCEEDED(hr));
        }
        return SUCCEEDED(hr) ? i : uint32(-1);
    }

    bool SetBlendState(const SStateBlend* pNewState)
    {
        uint32 bsIndex = GetOrCreateBlendState(pNewState->Desc);
        if (bsIndex == uint32(-1))
        {
            return false;
        }

        if (bsIndex != m_nCurStateBL)
        {
            m_nCurStateBL = bsIndex;
            m_DevMan.SetBlendState(m_StatesBL[bsIndex].pState, 0, 0xffffffff);
        }
        return true;
    }

    inline uint32 GetOrCreateRasterState(const D3D11_RASTERIZER_DESC& rasterizerDec, const bool bAllowMSAA = true)
    {
        uint32 i;
        HRESULT hr = S_OK;

        D3D11_RASTERIZER_DESC desc = rasterizerDec;

        BOOL bMSAA = bAllowMSAA && m_RP.m_MSAAData.Type > 1;
        if (bMSAA != desc.MultisampleEnable)
        {
            desc.MultisampleEnable = bMSAA;
        }

        uint32 nHash = SStateRaster::GetHash(desc);
        uint64 nValuesHash = SStateRaster::GetValuesHash(desc);
        for (i = 0; i < m_StatesRS.Num(); i++)
        {
            if (m_StatesRS[i].nHashVal == nHash && m_StatesRS[i].nValuesHash == nValuesHash)
            {
                break;
            }
        }
        if (i == m_StatesRS.Num())
        {
            SStateRaster* pState = m_StatesRS.AddIndex(1);
            pState->Desc = desc;
            pState->nHashVal = nHash;
            pState->nValuesHash = nValuesHash;
            hr = GetDevice().CreateRasterizerState(&pState->Desc, &pState->pState);
            assert(SUCCEEDED(hr));
        }
        return SUCCEEDED(hr) ? i : uint32(-1);
    }

    bool SetRasterState(const SStateRaster* pNewState, const bool bAllowMSAA = true)
    {
        uint32 rsIndex = GetOrCreateRasterState(pNewState->Desc, bAllowMSAA);
        if (rsIndex == uint32(-1))
        {
            return false;
        }

        if (rsIndex != m_nCurStateRS)
        {
            m_nCurStateRS = rsIndex;

            m_DevMan.SetRasterState(m_StatesRS[rsIndex].pState);
        }
        return true;
    }

    inline uint32 GetOrCreateDepthState(const D3D11_DEPTH_STENCIL_DESC& desc)
    {
        uint32 i;
        HRESULT hr = S_OK;
        uint64 nHash = SStateDepth::GetHash(desc);
        const int kNumStates = m_StatesDP.Num();
        for (i = 0; i < kNumStates; i++)
        {
            if (m_StatesDP[i].nHashVal == nHash)
            {
                break;
            }
        }
        if (i == kNumStates)
        {
            SStateDepth* pState = m_StatesDP.AddIndex(1);
            pState->Desc = desc;
            pState->nHashVal = nHash;
            hr = GetDevice().CreateDepthStencilState(&pState->Desc, &pState->pState);
            assert(SUCCEEDED(hr));
        }
        return SUCCEEDED(hr) ? i : uint32(-1);
    }

    bool SetDepthState(const SStateDepth* pNewState, uint8 newStencRef)
    {
        uint32 dsIndex = GetOrCreateDepthState(pNewState->Desc);
        if (dsIndex == uint32(-1))
        {
            return false;
        }

        if (dsIndex != m_nCurStateDP || m_nCurStencRef != newStencRef)
        {
            m_nCurStateDP = dsIndex;
            m_nCurStencRef = newStencRef;
            m_DevMan.SetDepthStencilState(m_StatesDP[dsIndex].pState, newStencRef);
        }

        return true;
    }

    void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topType)
    {
        if (m_CurTopology != topType)
        {
            m_CurTopology = topType;
            m_DevMan.BindTopology(m_CurTopology);
        }
    }

    virtual void LockParticleVideoMemory(uint32 nId);
    virtual void UnLockParticleVideoMemory(uint32 nId);

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_H_SECTION_3
    #include AZ_RESTRICTED_FILE(DriverD3D_h)
#endif


    SDepthTexture m_DepthBufferOrig;
    SDepthTexture m_DepthBufferOrigMSAA;
    SDepthTexture m_DepthBufferNative;

    // Bindable depthstencil buffer view and shader resource view. Ideally would be unified into regular texture creation - requires big refactoring
    D3DDepthSurface* m_pZBufferReadOnlyDSV;
    D3DShaderResourceView* m_pZBufferDepthReadOnlySRV;
    D3DShaderResourceView* m_pZBufferStencilReadOnlySRV;

    int m_MaxAnisotropyLevel;
    int m_nMaterialAnisoHighSampler;
    int m_nMaterialAnisoLowSampler;
    int m_nMaterialAnisoSamplerBorder;
    //===============================================================================
    //////////////////////////////////////////////////////////////////////////
    enum
    {
        kUnitObjectIndexSizeof = 2
    };

    D3DBuffer* m_pUnitFrustumVB[SHAPE_MAX];
    D3DBuffer* m_pUnitFrustumIB[SHAPE_MAX];

    int m_UnitFrustVBSize[SHAPE_MAX];
    int m_UnitFrustIBSize[SHAPE_MAX];

    D3DBuffer* m_pQuadVB;
    int16 m_nQuadVBSize;

    //////////////////////////////////////////////////////////////////////////
#ifdef CRY_USE_METAL
    SPixFormat        m_FormatPVRTC2;     //ETC2 compressed RGB for mobile
    SPixFormat        m_FormatPVRTC4;    //ETC2a compressed RGBA for mobile
#endif
#if defined(ANDROID) || defined(CRY_USE_METAL)
    SPixFormat        m_FormatASTC_4x4;
    SPixFormat        m_FormatASTC_5x4;
    SPixFormat        m_FormatASTC_5x5;
    SPixFormat        m_FormatASTC_6x5;
    SPixFormat        m_FormatASTC_6x6;
    SPixFormat        m_FormatASTC_8x5;
    SPixFormat        m_FormatASTC_8x6;
    SPixFormat        m_FormatASTC_8x8;
    SPixFormat        m_FormatASTC_10x5;
    SPixFormat        m_FormatASTC_10x6;
    SPixFormat        m_FormatASTC_10x8;
    SPixFormat        m_FormatASTC_10x10;
    SPixFormat        m_FormatASTC_12x10;
    SPixFormat        m_FormatASTC_12x12;
#endif
    SPixFormatSupport m_hwTexFormatSupport;

    int m_fontBlendMode;

    CCryNameTSCRC m_LevelShaderCacheMissIcon;

    CColorGradingControllerD3D* m_pColorGradingControllerD3D;

    CRenderPipelineProfiler* m_pPipelineProfiler;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_H_SECTION_4
    #include AZ_RESTRICTED_FILE(DriverD3D_h)
#endif

private:
    D3DDevice* m_Device;
    D3DDeviceContext* m_DeviceContext;

    // Used for shadow casting for transparents.
    CCryNameTSCRC m_techShadowGen;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_H_SECTION_5
    #include AZ_RESTRICTED_FILE(DriverD3D_h)
#endif

    t_arrDeferredMeshIndBuff m_arrDeferredInds;
    t_arrDeferredMeshVertBuff m_arrDeferredVerts;

public:
#ifdef SHADER_ASYNC_COMPILATION
    DynArray<CAsyncShaderTask*> m_AsyncShaderTasks;
public:
#endif


    void SetDefaultTexParams(bool bUseMips, bool bRepeat, bool bLoad);

public:
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_H_SECTION_6
    #include AZ_RESTRICTED_FILE(DriverD3D_h)
#endif

    /////////////////////////////////////////////////////////////////////////////
    // Functions to access the device and associated objects
    const D3DDevice& GetDevice() const;
    D3DDevice& GetDevice();
    D3DDeviceContext& GetDeviceContext();

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_H_SECTION_7
    #include AZ_RESTRICTED_FILE(DriverD3D_h)
#endif
    bool IsDeviceContextValid() { return m_DeviceContext != nullptr; }

    /////////////////////////////////////////////////////////////////////////////
    // Debug Functions to check that the D3D DeviceContext is only accessed
    // by its owning thread
    void BindContextToThread(DWORD threadID);
    DWORD GetBoundThreadID() const;
    void CheckContextThreadAccess() const;

    bool FX_PrepareDepthMapsForLight(const SRenderLight& rLight, int nLightID, bool bClearPool = false);
    void EF_PrepareShadowGenRenderList();
    bool EF_PrepareShadowGenForLight(SRenderLight* pLight, int nLightID);
    bool PrepareShadowGenForFrustum(ShadowMapFrustum* pCurFrustum, SRenderLight* pLight, int nLightFrustumID = 0, int nLOD = 0);
    void PrepareShadowGenForFrustumNonJobs(const int nFlags);
    virtual void EF_InvokeShadowMapRenderJobs(int nFlags);
    void InvokeShadowMapRenderJobs(ShadowMapFrustum* pCurFrustum, const SRenderingPassInfo& passInfo);
    void StartInvokeShadowMapRenderJobs(ShadowMapFrustum* pCurFrustum, const SRenderingPassInfo& passInfo);

    void UpdatePreviousFrameMatrices();

    void GetReprojectionMatrix(Matrix44A& matReproj, const Matrix44A& matView, const Matrix44A& matProj, const Matrix44A& matPrevView, const Matrix44A& matPrevProj, float fFarPlane) const;

    bool m_bDepthBoundsEnabled;
    float m_fDepthBoundsMin, m_fDepthBoundsMax;
    void SetDepthBoundTest(float fMin, float fMax, bool bEnable);

    void CreateDeferredUnitBox(t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff);
    const t_arrDeferredMeshIndBuff& GetDeferredUnitBoxIndexBuffer() const;
    const t_arrDeferredMeshVertBuff& GetDeferredUnitBoxVertexBuffer() const;
    bool PrepareDepthMap(ShadowMapFrustum* SMSource, int nLightFrustumID = 0, bool bClearPool = false);
    void ConfigShadowTexgen(int Num, ShadowMapFrustum* pFr, int nFrustNum = -1, bool bScreenToLocalBasis = false, bool bUseComparisonSampling = true);
    void DrawAllShadowsOnTheScreen();
    void DrawAllDynTextures(const char* szFilter, const bool bLogNames, const bool bOnlyIfUsedThisFrame);
    void OnEntityDeleted(IRenderNode* pRenderNode);

    void RT_ResetGlass();

    void RT_PostLevelLoading() override;

    //=============================================================
    void SetCull(ECull eCull, bool bSkipMirrorCull = false) override { D3DSetCull(eCull, bSkipMirrorCull); }

    void D3DSetCull(ECull eCull, bool bSkipMirrorCull = false);

    struct gammaramp_t;

    void GetDeviceGamma();
    void SetDeviceGamma(gammaramp_t*);

    HRESULT AdjustWindowForChange();

    bool IsFullscreen() { return m_bFullScreen; }
    bool IsSuperSamplingEnabled() { return m_numSSAASamples > 1; }
    bool IsNativeScalingEnabled() { return m_width != m_nativeWidth || m_height != m_nativeHeight; }
    
    int GetNativeWidth() const { return m_nativeWidth; }
    int GetNativeHeight() const { return m_nativeHeight; }
    D3DSurface* GetBackBuffer() { return m_pBackBuffer; }

    static HWND CreateWindowCallback();
#if defined(SUPPORT_DEVICE_INFO)
    DeviceInfo& DevInfo() { return m_devInfo; }
private:
    DeviceInfo m_devInfo;
#endif

private:
    void DebugShowRenderTarget();

    bool SetWindow(int width, int height, bool fullscreen, WIN_HWND hWnd);
    bool SetRes();
    void UnSetRes();
    void DisplaySplash(); //!< Load a bitmap from a file, blit it to the windowdc and free it

    void DestroyWindow(void);
    virtual void RestoreGamma(void);
    void SetGamma(float fGamma, float fBrigtness, float fContrast, bool bForce);

    int FindTextureInRegistry(const char* filename, int* tex_type);
    int RegisterTextureInRegistry(const char* filename, int tex_type, int tid, int low_tid);
    unsigned int MakeTextureREAL(const char* filename, int* tex_type, unsigned int load_low_res);
    unsigned int CheckTexturePlus(const char* filename, const char* postfix);

    static HRESULT CALLBACK OnD3D11CreateDevice(D3DDevice* pd3dDevice);

    void PostDeviceReset();

    void ForceUpdateGlobalShaderParameters() override;

public:
    static HRESULT CALLBACK OnD3D11PostCreateDevice(D3DDevice* pd3dDevice);

    // CRY DX12
    Matrix44 m_matPsmWarp;
    Matrix44 m_matViewInv;
    int m_MatDepth;


    static int CV_d3d11_CBUpdateStats;
    static ICVar* CV_d3d11_forcedFeatureLevel;
    static int CV_r_AlphaBlendLayerCount;

#if defined(SUPPORT_D3D_DEBUG_RUNTIME)
    static int CV_d3d11_debugruntime;
    static ICVar* CV_d3d11_debugMuteSeverity;
    static ICVar* CV_d3d11_debugMuteMsgID;
    static ICVar* CV_d3d11_debugBreakOnMsgID;
    static int CV_d3d11_debugBreakOnce;
#endif

    //============================================================
    // Renderer interface
    bool m_bFullScreen;

    TArray<SD3DContext*> m_RContexts;
    SD3DContext* m_CurrContext;

    TArray<CTexture*> m_RTargets;

public:

    void RT_PresentFast();

    // Multithreading support
    virtual void RT_BeginFrame();
    virtual void RT_EndFrame();
    virtual void RT_EndFrame(bool isLoading);
    virtual void RT_ForceSwapBuffers();
    virtual void RT_SwitchToNativeResolutionBackbuffer(bool resolveBackBuffer);
    virtual void RT_Init();
    virtual bool RT_CreateDevice();
    virtual void RT_Reset();
    virtual void RT_RenderScene(int nFlags, SThreadInfo& TI, RenderFunc pRenderFunc);
    virtual void RT_SetCull(int nMode);
    virtual void RT_SetScissor(bool bEnable, int x, int y, int width, int height);
    virtual void RT_SetCameraInfo();
    virtual void RT_SetStereoCamera();
    virtual void RT_CreateResource(SResourceAsync* Res);
    virtual void RT_ReleaseResource(SResourceAsync* pRes);
    virtual void RT_ReleaseRenderResources();
    virtual void RT_UnbindResources();
    virtual void RT_UnbindTMUs();
    virtual void RT_CreateRenderResources();
    virtual void RT_PrecacheDefaultShaders();
    virtual void RT_ReadFrameBuffer(unsigned char* pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX, int nScaledY);
    // CRY DX12
    virtual void RT_ClearTarget(ITexture* pTex, const ColorF& color);
    virtual void RT_ReleaseVBStream(void* pVB, int nStream);
    virtual void RT_ReleaseCB(void* pCB);
    virtual void RT_DrawDynVB(SVF_P3F_C4B_T2F* pBuf, uint16* pInds, uint32 nVerts, uint32 nInds, const PublicRenderPrimitiveType nPrimType);
    virtual void RT_DrawDynVBUI(SVF_P2F_C4B_T2F_F4B* pBuf, uint16* pInds, uint32 nVerts, uint32 nInds, const PublicRenderPrimitiveType nPrimType);
    virtual void RT_DrawStringU(IFFont_RenderProxy* pFont, float x, float y, float z, const char* pStr, bool asciiMultiLine, const STextDrawContext& ctx) const;
    virtual void RT_DrawLines(Vec3 v[], int nump, ColorF& col, int flags, float fGround);
    virtual void RT_Draw2dImage(float xpos, float ypos, float w, float h, CTexture* pTexture, float s0, float t0, float s1, float t1, float angle, DWORD col, float z);
    virtual void RT_Draw2dImageStretchMode(bool bStretch);
    virtual void RT_Push2dImage(float xpos, float ypos, float w, float h, CTexture* pTexture, float s0, float t0, float s1, float t1, float angle, DWORD col, float z, float stereoDepth);

    virtual void RT_Draw2dImageList();
    virtual void RT_DrawImageWithUV(float xpos, float ypos, float z, float w, float h, int texture_id, float* s, float* t, DWORD col, bool filtered = true);
    virtual void RT_PushRenderTarget(int nTarget, CTexture* pTex, SDepthTexture* pDepth, int nS);
    virtual void RT_PopRenderTarget(int nTarget);
    virtual void RT_SetViewport(int x, int y, int width, int height, int id = -1) override;
    virtual void RT_RenderDebug(bool bRenderStats = true);
    virtual void RT_SetRendererCVar(ICVar* pCVar, const char* pArgText, const bool bSilentMode = false);

    //===============================================================================
    void RT_DrawImageWithUVInternal(float xpos, float ypos, float z, float w, float h, int texture_id, float s[4], float t[4], DWORD col, bool filtered = true);
    void RT_Draw2dImageInternal(C2dImage* images, uint32 numImages, bool stereoLeftEye = true);

    //===============================================================================

    virtual WIN_HWND Init(int x, int y, int width, int height, unsigned int cbpp, int zbpp, int sbits, bool fullscreen, bool isEditor, WIN_HINSTANCE hinst, WIN_HWND Glhwnd = 0, bool bReInit = false, const SCustomRenderInitArgs* pCustomArgs = 0, bool bShaderCacheGen = false);

    virtual void GetVideoMemoryUsageStats(size_t& vidMemUsedThisFrame, size_t& vidMemUsedRecently, bool bGetPoolsSizes = false);

    /////////////////////////////////////////////////////////////////////////////////
    // Render-context management
    /////////////////////////////////////////////////////////////////////////////////
    virtual bool SetCurrentContext(WIN_HWND hWnd);
    virtual bool CreateContext(WIN_HWND hWnd, bool bAllowMSAA = false, int SSX = 1, int SSY = 1);
    virtual bool DeleteContext(WIN_HWND hWnd);
    virtual void MakeMainContextActive();
    virtual WIN_HWND GetCurrentContextHWND() { return m_CurrContext ? m_CurrContext->m_hWnd : m_hWnd; }
    virtual bool IsCurrentContextMainVP() { return m_CurrContext ? m_CurrContext->m_bMainViewport : true; }

    virtual int GetCurrentContextViewportWidth() const { return (m_bDeviceLost ? -1 : m_CurrContext->m_nViewportWidth); }
    virtual int GetCurrentContextViewportHeight() const { return (m_bDeviceLost ? -1 : m_CurrContext->m_nViewportHeight); }
    /////////////////////////////////////////////////////////////////////////////////

    virtual int  CreateRenderTarget(const char* name, int nWidth, int nHeight, const ColorF& clearColor, ETEX_Format eTF = eTF_R8G8B8A8);
    virtual bool DestroyRenderTarget(int nHandle);
    virtual bool ResizeRenderTarget(int nHandle, int nWidth, int nHeight);
    virtual bool SetRenderTarget(int nHandle, SDepthTexture* pDepthSurf = nullptr);
    virtual SDepthTexture* CreateDepthSurface(int nWidth, int nHeight, bool shaderResourceView = false);
    virtual void DestroyDepthSurface(SDepthTexture* pDepthSurf);

    virtual bool ChangeDisplay(unsigned int width, unsigned int height, unsigned int cbpp);
    virtual void ChangeViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height, bool bMainViewport = false, float scaleWidth = 1.0f, float scaleHeight = 1.0f);
    virtual int  EnumDisplayFormats(SDispFormat* formats);
    //! Return all supported by video card video AA formats
    virtual int EnumAAFormats(SAAFormat* formats);

    //! Changes resolution of the window/device (doen't require to reload the level
    virtual bool  ChangeResolution(int nNewWidth, int nNewHeight, int nNewColDepth, int nNewRefreshHZ, bool bFullScreen, bool bForceReset);
    virtual void  Reset(void);

    virtual void  SwitchToNativeResolutionBackbuffer();
    void CalculateResolutions(int width, int height, bool bUseNativeResolution, int* pRenderWidth, int* pRenderHeight, int* pNativeWidth, int* pNativeHeight, int* pBackbufferWidth, int* pBackbufferHeight);

    virtual void BeginFrame();
    virtual void ShutDown(bool bReInit = false);
    virtual void ShutDownFast();
    virtual void RenderDebug(bool bRenderStats = true);
    virtual void RT_ShutDown(uint32 nFlags);

    void DebugPrintShader(class CHWShader_D3D* pSH, void* pInst, int nX, int nY, ColorF colSH);
    void DebugPerfBars(int nX, int nY);
    void DebugVidResourcesBars(int nX, int nY);
    void DebugDrawStats1();
    void DebugDrawStats2();
    void DebugDrawStats8();
    void DebugDrawStats();
    void DebugDrawRect(float x1, float y1, float x2, float y2, float* fColor);
    void VidMemLog();

    virtual void EndFrame();
    virtual void LimitFramerate(const int maxFPS, const bool bUseSleep);
    virtual void GetMemoryUsage(ICrySizer* Sizer);
    void GetLogVBuffers();

    virtual void TryFlush();
    virtual void Draw2dImage(float xpos, float ypos, float w, float h, int textureid, float s0, float t0, float s1, float t1, float angle, const ColorF& col, float z = 1);
    virtual void Draw2dImage(float xpos, float ypos, float w, float h, int textureid, float s0 = 0, float t0 = 0, float s1 = 1, float t1 = 1, float angle = 0, float r = 1, float g = 1, float b = 1, float a = 1, float z = 1);
    virtual void Push2dImage(float xpos, float ypos, float w, float h, int textureid, float s0 = 0, float t0 = 0, float s1 = 1, float t1 = 1, float angle = 0,
        float r = 1, float g = 1, float b = 1, float a = 1, float z = 1, float stereoDepth = 0);
    virtual void Draw2dImageStretchMode(bool bStretch);
    virtual void Draw2dImageList();
    virtual void DrawImage(float xpos, float ypos, float w, float h, int texture_id, float s0, float t0, float s1, float t1, float r, float g, float b, float a, bool filtered = true);
    virtual void DrawImageWithUV(float xpos, float ypos, float z, float w, float h, int texture_id, float* s, float* t, float r, float g, float b, float a, bool filtered = true);
    virtual void SetCullMode(int mode = R_CULL_BACK);

    virtual void PushProfileMarker(const char* label);
    virtual void PopProfileMarker(const char* label);

    virtual bool EnableFog(bool enable);
    virtual void SetFogColor(const ColorF& color);

    virtual void CreateResourceAsync(SResourceAsync* pResource);
    virtual void ReleaseResourceAsync(SResourceAsync* pResource);
    virtual void ReleaseResourceAsync(AZStd::unique_ptr<SResourceAsync> pResource) override;
    virtual unsigned int DownLoadToVideoMemory(const byte* data, int w, int h, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat = true, int filter = FILTER_BILINEAR, int Id = 0, const char* szCacheName = NULL, int flags = 0, EEndian eEndian = eLittleEndian, RectI* pRegion = NULL, bool bAsynDevTexCreation = false);
    virtual unsigned int DownLoadToVideoMemory(const byte* data, int w, int h, int d, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, ETEX_Type eTT, bool repeat = true, int filter = FILTER_BILINEAR, int Id = 0, const char* szCacheName = NULL, int flags = 0, EEndian eEndian = eLittleEndian, RectI* pRegion = NULL, bool bAsynDevTexCreation = false);
    virtual unsigned int DownLoadToVideoMemoryCube(const byte* data, int w, int h, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat = true, int filter = FILTER_BILINEAR, int Id = 0, const char* szCacheName = NULL, int flags = 0, EEndian eEndian = eLittleEndian, RectI* pRegion = NULL, bool bAsynDevTexCreation = false);
    virtual unsigned int DownLoadToVideoMemory3D(const byte* data, int w, int h, int d, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat = true, int filter = FILTER_BILINEAR, int Id = 0, const char* szCacheName = NULL, int flags = 0, EEndian eEndian = eLittleEndian, RectI* pRegion = NULL, bool bAsynDevTexCreation = false);
    virtual void UpdateTextureInVideoMemory(uint32 tnum, const byte* newdata, int posx, int posy, int w, int h, ETEX_Format eTF = eTF_R8G8B8A8, int posz = 0, int sizez = 1);
    virtual bool EF_PrecacheResource(SShaderItem* pSI, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId, int nCounter);
    virtual bool EF_PrecacheResource(ITexture* pTP, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId, int nCounter);
    virtual void RemoveTexture(unsigned int TextureId);
    virtual void DeleteFont(IFFont* font);
    virtual ITexture* EF_CreateCompositeTexture(int type, const char* szName, int nWidth, int nHeight, int nDepth, int nMips, int nFlags, ETEX_Format eTF, const STexComposition* pCompositions, size_t nCompositions, int8 nPriority = -1);

    virtual void PostLevelLoading();
    virtual void PostLevelUnload();

    virtual void ApplyViewParameters(const CameraViewParameters& viewParameters) override;
    virtual void SetCamera(const CCamera& cam);
    virtual void SetViewport(int x, int y, int width, int height, int id = 0);
    virtual void GetViewport(int* x, int* y, int* width, int* height) const;
    virtual void SetScissor(int x = 0, int y = 0, int width = 0, int height = 0);
    virtual void SetRenderTile(f32 nTilesPosX, f32 nTilesPosY, f32 nTilesGridSizeX, f32 nTilesGridSizeY);

    void DrawLines(Vec3 v[], int nump, ColorF& col, int flags, float fGround);
    virtual void Graph(byte* g, int x, int y, int wdt, int hgt, int nC, int type, const char* text, ColorF& color, float fScale);

    virtual void DrawDynVB(SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds, PublicRenderPrimitiveType nPrimType);
    virtual void DrawDynUiPrimitiveList(DynUiPrimitiveList& primitives, int totalNumVertices, int totalNumIndices);

    virtual void PrintResourcesLeaks();

    virtual void FX_PushWireframeMode(int mode);
    virtual void FX_PopWireframeMode();
    void FX_SetWireframeMode(int nMode);

    virtual void PushMatrix();
    virtual void PopMatrix();

    virtual void EnableVSync(bool enable);
    virtual void DrawPrimitivesInternal(CVertexBuffer* src, int vert_num, const eRenderPrimitiveType prim_type);
    virtual void ResetToDefault();

    // Sets default Blend, DepthStencil and Raster states.
    virtual void SetDefaultRenderStates();

    virtual void SetMaterialColor(float r, float g, float b, float a);

    virtual bool ProjectToScreen(float ptx, float pty, float ptz, float* sx, float* sy, float* sz);
    virtual int UnProject(float sx, float sy, float sz, float* px, float* py, float* pz, const float modelMatrix[16], const float projMatrix[16], const int    viewport[4]);
    virtual int UnProjectFromScreen(float  sx, float  sy, float  sz, float* px, float* py, float* pz);

    virtual void GetModelViewMatrix(float* mat);
    virtual void GetProjectionMatrix(float* mat);
    virtual void SetMatrices(float* pProjMat, float* pViewMat);

    void DrawQuad(float x0, float y0, float x1, float y1, const ColorF& color, float z = 1.0f, float s0 = 0.0f, float t0 = 0.0f, float s1 = 1.0f, float t1 = 1.0f) override;
    void DrawQuad3D(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3, const ColorF& color,
        float ftx0 = 0,  float fty0 = 0,  float ftx1 = 1,  float fty1 = 1);
    void DrawFullScreenQuad(CShader* pSH, const CCryNameTSCRC& TechName, float s0, float t0, float s1, float t1, uint32 nState = GS_NODEPTHTEST);

    // NOTE: deprecated
    virtual void ClearTargetsImmediately(uint32 nFlags);
    virtual void ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors, float fDepth);
    virtual void ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors);
    virtual void ClearTargetsImmediately(uint32 nFlags, float fDepth);

    virtual void ClearTargetsLater(uint32 nFlags);
    virtual void ClearTargetsLater(uint32 nFlags, const ColorF& Colors, float fDepth);
    virtual void ClearTargetsLater(uint32 nFlags, const ColorF& Colors);
    virtual void ClearTargetsLater(uint32 nFlags, float fDepth);
    virtual void ReadFrameBuffer(unsigned char* pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX = -1, int nScaledY = -1);
    virtual void ReadFrameBufferFast(uint32* pDstARGBA8, int dstWidth, int dstHeight, bool BGRA = true);

    virtual void SetRendererCVar(ICVar* pCVar, const char* pArgText, bool bSilentMode = false);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // This routines uses 2 destination surfaces.  It triggers a backbuffer copy to one of its surfaces,
    // and then copies the other surface to system memory.  This hopefully will remove any
    // CPU stalls due to the rect lock call since the buffer will already be in system
    // memory when it is called
    // Inputs :
    //          pDstARGBA8          :   Pointer to a buffer that will hold the captured frame (should be at least 4*dstWidth*dstHieght for RGBA surface)
    //          destinationWidth    :   Width of the frame to copy
    //          destinationHeight   :   Height of the frame to copy
    //
    //          Note :  If dstWidth or dstHeight is larger than the current surface dimensions, the dimensions
    //                  of the surface are used for the copy
    //
    bool CaptureFrameBufferFast(unsigned char* pDstRGBA8, int destinationWidth, int destinationHeight);


    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // Copy a captured surface to a buffer
    //
    // Inputs :
    //          pDstARGBA8          :   Pointer to a buffer that will hold the captured frame (should be at least 4*dstWidth*dstHieght for RGBA surface)
    //          destinationWidth    :   Width of the frame to copy
    //          destinationHeight   :   Height of the frame to copy
    //
    //          Note :  If dstWidth or dstHeight is larger than the current surface dimensions, the dimensions
    //                  of the surface are used for the copy
    //
    bool CopyFrameBufferFast(unsigned char* pDstRGBA8, int destinationWidth, int destinationHeight);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // This routine registers a callback address that is called when a new frame is available
    // Inputs :
    //          pCapture            :   Address of the ICaptureFrameListener object
    //
    // Outputs : returns true if successful, otherwise false
    //
    bool RegisterCaptureFrame(ICaptureFrameListener* pCapture);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // This routine unregisters a callback address that was previously registered
    // Inputs :
    //          pCapture            :   Address of the ICaptureFrameListener object to unregister
    //
    // Outputs : returns true if successful, otherwise false
    //
    bool UnRegisterCaptureFrame(ICaptureFrameListener* pCapture);


    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // This routine initializes 2 destination surfaces for use by the CaptureFrameBufferFast routine
    // It also, captures the current backbuffer into one of the created surfaces
    //
    // Inputs :
    //          bufferWidth : Width of capture buffer, on consoles the scaling is done on the GPU. Pass in 0 (the default) to use backbuffer dimensions
    //          bufferHeight    : Height of capture buffer.
    //
    // Outputs : returns true if surfaces were created otherwise returns false
    //
    bool InitCaptureFrameBufferFast(uint32 bufferWidth, uint32 bufferHeight);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // This routine releases the 2 surfaces used for frame capture by the CaptureFrameBufferFast routine
    //
    // Inputs : None
    //
    // Returns : None
    //
    void CloseCaptureFrameBufferFast(void);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // This routine checks for any frame buffer callbacks that are needed and calls them
    //
    // Inputs : None
    //
    //  Outputs : None
    //
    void CaptureFrameBufferCallBack(void);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // This routine checks for any frame buffer callbacks that are needed and checks their flags, calling preparation functions if required
    //
    // Inputs : None
    //
    //  Outputs : None
    //
    void CaptureFrameBufferPrepare(void);

    //////////////////////////////////////////////////////////////////////
    // RenderScreenshot request bus
    virtual void WriteScreenshotToFile(const char* filepath) override;
    virtual void WriteScreenshotToBuffer() override;
    virtual bool CopyScreenshotToBuffer(unsigned char* imageBuffer, unsigned int width, unsigned int height) override;

    //misc
    bool ScreenShotInternal(const char* filename = nullptr, int width = 0);
    virtual bool ScreenShot(const char* filename, int width = 0);
    virtual bool ScreenShot();

    virtual void UnloadOldTextures(){};

    virtual void Set2DMode(uint32 orthoX, uint32 orthoY, TransformationMatrices& backupMatrices, float znear = -1e10f, float zfar = 1e10f);
    virtual void Unset2DMode(const TransformationMatrices& restoringMatrices);
    virtual void Set2DModeNonZeroTopLeft(float orthoLeft, float orthoTop, float orthoWidth, float orthoHeight, TransformationMatrices& backupMatrices, float znear = -1e10f, float zfar = 1e10f);

    virtual int ScreenToTexture(int nTexID);

    virtual bool    SetGammaDelta(float fGamma);

    //////////////////////////////////////////////////////////////////////
    // Replacement functions for the Font engine
    virtual   bool FontUploadTexture(class CFBitmap*, ETEX_Format eTF = eTF_R8G8B8A8);
    virtual   int  FontCreateTexture(int Width, int Height, byte* pData, ETEX_Format eTF = eTF_R8G8B8A8, bool genMips = IRenderer::FontCreateTextureGenMipsDefaultValue, const char* textureName = nullptr);
    virtual   bool FontUpdateTexture(int nTexId, int X, int Y, int USize, int VSize, byte* pData);
    virtual   void FontReleaseTexture(class CFBitmap* pBmp);
    virtual void FontSetTexture(class CFBitmap*, int nFilterMode);

    virtual void FontSetTexture(int nTexId, int nFilterMode);
    virtual void FontSetRenderingState(bool overrideViewProjMatrices, TransformationMatrices& backupMatrices);
    virtual void FontSetBlending(int src, int dst, int baseState);
    virtual void FontRestoreRenderingState(bool overrideViewProjMatrices, const TransformationMatrices& backupMatrices);

    void FontSetState(bool bRestore, int baseState);
    virtual void SetProfileMarker(const char* label, ESPM mode) const;

    //////////////////////////////////////////////////////////////////////

    void FX_ApplyThreadState(SThreadInfo& TI, SThreadInfo* OldTI);
    void EF_RenderScene(int nFlags, SViewport& VP, const SRenderingPassInfo& passInfo);
    void EF_Scene3D(SViewport& VP, int nFlags, const SRenderingPassInfo& passInfo);

    bool CheckMSAAChange();
    bool CheckSSAAChange();

    // FX Shaders pipeline
    void FX_DrawInstances(CShader* ef, SShaderPass* slw, int nRE, uint32 nCurInst, uint32 nLastInst, uint32 nUsedAttr, byte* nInstanceData, int nInstAttrMask, byte Attributes[], short dwCBufSlot);
    void FX_DrawShader_InstancedHW(CShader* ef, SShaderPass* slw);
    void FX_DrawShader_General(CShader* ef, SShaderTechnique* pTech);
    void FX_SetupForwardShadows(bool bUseShaderPermutations = false);
    void FX_SetupShadowsForTransp();
    void FX_SetupShadowsForFog();
    bool FX_DrawToRenderTarget(CShader* pShader, CShaderResources* pRes, CRenderObject* pObj, SShaderTechnique* pTech, SHRenderTarget* pTarg, int nPreprType, IRenderElement* pRE);

    void FX_DrawShader_Fur(CShader* ef, SShaderTechnique* pTech);

    // hdr src texture is optional, if not specified uses default hdr destination target
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_H_SECTION_8
    #include AZ_RESTRICTED_FILE(DriverD3D_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    void CopyFramebufferDX11(CTexture* pDst, ID3D11Resource* pSrcResource, D3DFormat srcFormat);
#endif
    void FX_ScreenStretchRect(CTexture* pDst, CTexture* pHDRSrc = NULL);

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_H_SECTION_9
    #include AZ_RESTRICTED_FILE(DriverD3D_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    virtual bool BakeMesh(const SMeshBakingInputParams* pInputParams, SMeshBakingOutput* pReturnValues);
#endif

    virtual int GetOcclusionBuffer(uint16* pOutOcclBuffer, Matrix44* pmCamBuffer);

    virtual void WaitForParticleBuffer(threadID nThreadId);
    void InsertParticleVideoMemoryFence(int nThreadId);

    void FX_StencilTestCurRef(bool bEnable, bool bNoStencilClear = true, bool bStFuncEqual = true);
    void EF_PrepareCustomShadowMaps();
    void EF_PrepareAllDepthMaps();

    void SetFrontFacingStencillState(int nStencilID);
    void SetBackFacingStencillState(int nStencilID);
    
    //These two overridden functions allow for control over which pass to use during the stencil pass.
    void FX_StencilCullPass(int nStencilID, int nNumVers, int nNumInds, CShader*  pShader, int frontFacePass);
    void FX_StencilCullPass(int nStencilID, int nNumVers, int nNumInds, CShader*  pShader, int frontFacePass, int backFacePass);

    void FX_StencilFrustumCull(int nStencilID, const SRenderLight* pLight, ShadowMapFrustum* pFrustum, int nAxis);
    void FX_StencilCullNonConvex(int nStencilID, IRenderMesh* pWaterTightMesh, const Matrix34& mWorldTM);
    
    // Prepare the GPU-targets for next frame's CPU depth readback
    void FX_ZTargetReadBack();
    void UpdateOcclusionDataForCPU();

    // Perform the CPU-side readback of the GPU target prepared in FX_ZTargetReadBack
    void FX_ZTargetReadBackOnCPU();
    

    void FX_UpdateCharCBs();
    void* FX_AllocateCharInstCB(SSkinningData*, uint32);
    void FX_ClearCharInstCB(uint32);

    void UpdatePerFrameParameters();

    bool CreateAuxiliaryMeshes();
    bool ReleaseAuxiliaryMeshes();

    bool CreateUnitVolumeMesh(t_arrDeferredMeshIndBuff& arrDeferredInds, t_arrDeferredMeshVertBuff& arrDeferredVerts, D3DBuffer*& pUnitFrustumIB, D3DBuffer*& pUnitFrustumVB);
    void FX_DeferredShadowPass(const SRenderLight* pLight, ShadowMapFrustum* pShadowFrustum, bool bShadowPass, bool bCloudShadowPass, bool bStencilPrepass, int nLod);
    bool FX_DeferredShadowPassSetup(const Matrix44& mShadowTexGen, ShadowMapFrustum* pShadowFrustum, float maskRTWidth, float maskRTHeight, Matrix44& mScreenToShadow, bool bNearest);
    bool FX_DeferredShadowPassSetupBlend(const Matrix44& mShadowTexGen, int nFrustumNum, float maskRTWidth, float maskRTHeight);
    bool FX_DeferredShadows(SRenderLight* pLight, int maskRTWidth, int maskRTHeight);
    void FX_DeferredShadowMaskGen(const TArray<uint32>& shadowPoolLights);
    void FX_MergeShadowMaps(ShadowMapFrustum* pDst, const ShadowMapFrustum* pSrc);
    void FX_ClearShadowMaskTexture();

    void FX_DrawDebugPasses();

    void FX_DrawMultiLayers();

    void FX_DrawTechnique(CShader* ef, SShaderTechnique* pTech);

    void FX_RefractionPartialResolve();

    bool FX_HDRScene(bool bEnable, bool bClear = true);

    void FX_RenderForwardOpaque(void (* RenderFunc)(), const bool bLighting, const bool bAllowDeferred);
    void FX_RenderWater(void (* RenderFunc)());
    void FX_RenderFog();

    bool FX_ZScene(bool bEnable, bool bClearZBuffer, bool bRenderNormalsOnly = false, bool bZPrePass = false);

    // GMEM Related /////////////////////////////////////////////////////
    enum EGmemTransitions
    {
        eGT_PRE_Z,
        eGT_POST_GBUFFER,
        eGT_POST_Z_PRE_DEFERRED,
        eGT_POST_DEFERRED_PRE_FORWARD,
        eGT_POST_AW_TRANS_PRE_POSTFX
    };

    enum EGmemDepthStencilMode
    {
        eGDSM_RenderTarget,         // Values are written/read to/from a RT during the ZPass. Values are linearized when written to the RT.
        eGDSM_DepthStencilBuffer,   // Values are written to the depth/stencil buffer and read using an extension. Values are linearized in the shader when fetching them. 
        eGDSM_Texture,              // Values are resolved (and linearized) from the depth/stencil buffer to a texture with an extra pass.
        eGDSM_Invalid
    };

    // Binds appropriate GMEM RTs depending on which section of the rendering pipeline we're in
    void FX_GmemTransition(const EGmemTransitions transition);

    EGmemDepthStencilMode FX_GmemGetDepthStencilMode() const;

    // Values of this enum must match the intended values for cvar r_EnableGMEMPath
    enum EGmemPath
    {
        eGT_REGULAR_PATH = 0, // No GMEM path is enabled. Using regular render path.
        eGT_256bpp_PATH,
        eGT_128bpp_PATH,
        eGT_PathCount // Must be last
    };

    enum EGmemPathState
    {
        eGT_OK,                     // Nothing to report
        eGT_DEV_UNSUPPORTED,        // GMEM path not supported due to device limitations
        eGT_FEATURES_UNSUPPORTED    // Some rendering features are no supported with the GMEM path defined in .cfg file (r_EnableGMEMPath)
    };

    enum EGmemRendertargetType
    {
        eGT_Diffuse,
        eGT_Specular,
        eGT_Normals,
        eGT_DepthStencil,
        eGT_DiffuseLight,
        eGT_SpecularLight,
        eGT_VelocityBuffer,
        eGT_RenderTargetCount // Must be last
    };

    static const int s_gmemRendertargetSlots[eGT_PathCount][eGT_RenderTargetCount];

    // Checks if GMEM path is enabled.
    EGmemPath FX_GetEnabledGmemPath(EGmemPathState* const gmemPathStateOut) const;

    static const int s_gmemLargeRTCount = 5;
    /////////////////////////////////////////////////////////////////////

    bool FX_FogScene();
    bool FX_DeferredCaustics();
    bool FX_DeferredWaterVolumeCaustics(const N3DEngineCommon::SCausticInfo& causticInfo);
    bool FX_DeferredRainOcclusionMap(const N3DEngineCommon::ArrOccluders& arrOccluders, const SRainParams& rainVolParams);
    bool FX_DeferredRainOcclusion();
    bool FX_DeferredRainPreprocess();
    bool FX_DeferredRainGBuffer();
    bool FX_DeferredSnowLayer();
    bool FX_DeferredSnowDisplacement();
    bool FX_MotionVectorGeneration(bool bEnable);
    bool FX_CustomRenderScene(bool bEnable);
    bool FX_PostProcessScene(bool bEnable);
    bool FX_DeferredRendering(bool bDebugPass = false, bool bUpdateRTOnly = false);
    bool FX_DeferredDecals();
    bool FX_DeferredDecalsEmissive();
    bool FX_SkinRendering(bool bEnable);
    void FX_DepthFixupPrepare();
    void FX_DepthFixupMerge();
    void FX_SRGBConversion();

    // Linearize Depth
    void SetupLinearizeDepthParams(CShader* shader);
    void FX_LinearizeDepth(CTexture* ptexZ);

    // Performance queries
    //=======================================================================

    virtual float GetGPUFrameTime();
    virtual void GetRenderTimes(SRenderTimes& outTimes);

    // Shaders pipeline
    //=======================================================================
    virtual void FX_PipelineShutdown(bool bFastShutdown = false);

    virtual void EF_ClearTargetsImmediately(uint32 nFlags);
    virtual void EF_ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors, float fDepth, uint8 nStencil);
    virtual void EF_ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors);
    virtual void EF_ClearTargetsImmediately(uint32 nFlags, float fDepth, uint8 nStencil);

    virtual void EF_ClearTargetsLater(uint32 nFlags);
    virtual void EF_ClearTargetsLater(uint32 nFlags, const ColorF& Colors, float fDepth, uint8 nStencil);
    virtual void EF_ClearTargetsLater(uint32 nFlags, const ColorF& Colors);
    virtual void EF_ClearTargetsLater(uint32 nFlags, float fDepth, uint8 nStencil);

    void FX_Invalidate();
    void EF_Restore();
    virtual void FX_SetState(int st, int AlphaRef = -1, int RestoreState = 0) override;
    void FX_StateRestore(int prevState);

    void ChangeLog();

    void SetCurDownscaleFactor(Vec2 sf);
    // sets fullscreen viewport regardless of current scaling -
    // should only be used for final upscale (in Post AA)
    void SetFullscreenViewport();

    bool CreateMSAADepthBuffer();
    void FlushHardware(bool bIssueBeforeSync);
    void PostMeasureOverdraw();
    void DrawTexelsPerMeterInfo();
    virtual bool CheckDeviceLost();

    short m_nPrepareShadowFrame;

    _inline void EF_DirtyMatrix()
    {
        int nThreadID = m_pRT->GetThreadList();
        m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_FP_MATRIXDIRTY | RBPF_FP_DIRTY;
    }

    void FX_ResetPipe() override;

    _inline void EF_SetGlobalColor(float r, float g, float b, float a)
    {
        assert(m_pRT->IsRenderThread());

        EF_SetColorOp(255, 255, eCA_Texture | (eCA_Constant << 3), eCA_Texture | (eCA_Constant << 3));
        EF_SetSrgbWrite(false);

        if (m_RP.m_CurGlobalColor[0] != r || m_RP.m_CurGlobalColor[1] != g || m_RP.m_CurGlobalColor[2] != b || m_RP.m_CurGlobalColor[3] != a)
        {
            m_RP.m_CurGlobalColor[0] = r;
            m_RP.m_CurGlobalColor[1] = g;
            m_RP.m_CurGlobalColor[2] = b;
            m_RP.m_CurGlobalColor[3] = a;
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_DIRTY;
        }
    }
    _inline void EF_SetVertColor()
    {
        // Only used from FontSetState, we do not want to call SetSrgbWrite - use whatever is set
        EF_SetColorOp(255, 255, eCA_Texture | (eCA_Diffuse << 3), eCA_Texture | (eCA_Diffuse << 3));
    }
    _inline void EF_DisableTextureAndColor()
    {
        assert(m_pRT->IsRenderThread());
        if ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg & 7) == eCA_Texture || (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg & 7) == eCA_Diffuse)
        {
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg & ~7) | eCA_Constant;
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_DIRTY;
        }
        if (((m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg >> 3) & 7) == eCA_Texture || ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg >> 3) & 7) == eCA_Diffuse)
        {
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg & ~0x38) | (eCA_Constant << 3);
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_DIRTY;
        }
        if ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg & 7) == eCA_Texture || (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg & 7) == eCA_Diffuse)
        {
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg & ~7) | eCA_Constant;
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_DIRTY;
        }
        if (((m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg >> 3) & 7) == eCA_Texture || ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg >> 3) & 7) == eCA_Diffuse)
        {
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg & ~0x38) | (eCA_Constant << 3);
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_DIRTY;
        }
    }

    //================================================================================

    virtual long FX_SetVertexDeclaration(int StreamMask, const AZ::Vertex::Format& vertexFormat) override;
    inline bool FX_SetStreamFlags([[maybe_unused]] SShaderPass* pPass)
    {
        if (CV_r_usehwskinning && m_RP.m_pRE && (m_RP.m_pRE->mfGetFlags() & FCEF_SKINNED))
        {
            m_RP.m_FlagsStreams_Decl |= VSM_HWSKIN;
            m_RP.m_FlagsStreams_Stream |= VSM_HWSKIN;
            return true;
        }

        return false;
    }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_H_SECTION_10
    #include AZ_RESTRICTED_FILE(DriverD3D_h)
#endif
    void FX_DrawBatches(CShader* pSh, SShaderPass* pPass);
    void FX_DrawBatchesSkinned(CShader* pSh, SShaderPass* pPass, SSkinningData* pSkinningData);

    //========================================================================================

    void FX_SetActiveRenderTargets(bool bAllowDIP = false) override;

    void FX_SetViewport();
    void FX_ClearTargets();


    /* NOTES:
     *  - passing D3DSurface/D3DDepthSurface are the close-to-metal calls and might not be
     *    implemented for a specific device
     *  - using rectangles and passing optional=true will use the most close-to-metal call
     *    possible, even if it clears more than the given rect - otherwise a
     *    shader-implementation will do the exact rect clear
     *
     *  - if there is a fallback, then the RT-stack is utilized to clear, which comes with
     *    moderate CPU-overhead (relative to the cost of the close-to-metal call)
     */

    void FX_ClearTarget(D3DSurface* pView, const ColorF& cClear, const uint numRects = 0, const RECT* pRects = nullptr);
    void FX_ClearTarget(ITexture* pTex, const ColorF& cClear, const uint numRects, const RECT* pRects, const bool bOptional);
    void FX_ClearTarget(ITexture* pTex, const ColorF& cClear);
    void FX_ClearTarget(ITexture* pTex);

    void FX_ClearTarget(D3DDepthSurface* pView, const int nFlags, const float cDepth, const uint8 cStencil, const uint numRects = 0, const RECT* pRects = nullptr);
    void FX_ClearTarget(SDepthTexture* pTex, const int nFlags, const float cDepth, const uint8 cStencil, const uint numRects, const RECT* pRects, const bool bOptional);
    void FX_ClearTarget(SDepthTexture* pTex, const int nFlags, const float cDepth, const uint8 cStencil);
    void FX_ClearTarget(SDepthTexture* pTex, const int nFlags);
    void FX_ClearTarget(SDepthTexture* pTex);

    // shader-implementation of clear
    void FX_ClearTargetRegion(const uint32 nAdditionalStates = 0);

#define MAX_RT_STACK 8

#if defined(WIN32) || defined(OPENGL)
    #define RT_STACK_WIDTH  D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT
#else
    #define RT_STACK_WIDTH  4
#endif

    struct SRTStack
    {
        D3DSurface* m_pTarget;
        D3DDepthSurface* m_pDepth;

        CTexture* m_pTex;
        SDepthTexture* m_pSurfDepth;
        int m_Width;
        int m_Height;
        uint32 m_bNeedReleaseRT : 1;
        uint32 m_bWasSetRT : 1;
        uint32 m_bWasSetD : 1;

        uint32 m_bScreenVP : 1;

        uint32 m_ClearFlags;
        ColorF m_ReqColor;
        float m_fReqDepth;
        uint8 m_nReqStencil;
    };

    int m_nRTStackLevel[RT_STACK_WIDTH];
    SRTStack m_RTStack[RT_STACK_WIDTH][MAX_RT_STACK];

    int m_nMaxRT2Commit;
    SRTStack* m_pNewTarget[RT_STACK_WIDTH];
    CTexture* m_pCurTarget[RT_STACK_WIDTH];

    TArray<SDepthTexture*> m_TempDepths;

    enum
    {
        MAX_WIREFRAME_STACK = 10
    };
    int m_arrWireFrameStack[MAX_WIREFRAME_STACK];
    int m_nWireFrameStack;

    bool FX_GetTargetSurfaces(CTexture* pTarget, D3DSurface*& pTargSurf, SRTStack* pCur, int nCMSide = -1, int nTarget = 0, uint32 nTileCount = 1);

    virtual bool FX_SetRenderTarget(int nTarget, void* pTargetSurf, SDepthTexture* pDepthTarget, uint32 nTileCount = 1) override;
    virtual bool FX_PushRenderTarget(int nTarget, void* pTargetSurf, SDepthTexture* pDepthTarget, uint32 nTileCount = 1) override;
    virtual bool FX_SetRenderTarget(int nTarget, CTexture* pTarget, SDepthTexture* pDepthTarget, bool bPush = false, int nCMSide = -1, bool bScreenVP = false, uint32 nTileCount = 1) override;
    virtual bool FX_PushRenderTarget(int nTarget, CTexture* pTarget, SDepthTexture* pDepthTarget, int nCMSide = -1, bool bScreenVP = false, uint32 nTileCount = 1) override;
    virtual bool FX_RestoreRenderTarget(int nTarget) override;
    virtual bool FX_PopRenderTarget(int nTarget) override;
    virtual SDepthTexture* FX_GetDepthSurface(int nWidth, int nHeight, bool bAA, bool shaderResourceView = false) override;
    virtual SDepthTexture* GetDepthBufferOrig() override { return &m_DepthBufferOrig; }
    virtual uint32 GetBackBufferWidth() override { return m_backbufferWidth; }
    virtual uint32 GetBackBufferHeight() override { return m_backbufferHeight; }

    CTexture* FX_GetCurrentRenderTarget(int target);
    D3DSurface* FX_GetCurrentRenderTargetSurface(int target) const;

    // CONFETTI BEGIN: David Srour
    // Following is used to assign load/store actions for the nTarget render target.
    // This function should be used AFTER using FX_PushRenderTarget.
    // Currently, these will only do something meaningful for devices running Metal & GL-ES.
    void FX_SetColorDontCareActions(int const nTarget,
        bool const loadDontCare  = false,
        bool const storeDontCare = false);
    void FX_SetDepthDontCareActions(int const nTarget,
        bool const loadDontCare  = false,
        bool const storeDontCare = false);
    void FX_SetStencilDontCareActions(int const nTarget,
        bool const loadDontCare  = false,
        bool const storeDontCare = false);

    // Following is used to toggle GL-ES "Pixel Local Storage" extension
    // Will only do something meaningful for devices running GL-ES with the PLS extension enabled.
    void FX_TogglePLS(bool const enable);
    // CONFETTI END

    //========================================================================================


    virtual void FX_Commit(bool bAllowDIP = false) override;
    void FX_ZState(uint32& nState);
    void FX_HairState(uint32& nState, const SShaderPass* pPass);
    bool FX_SetFPMode();
    bool FX_SetUIMode();

    ILINE uint32 PackBlendModeAndPassGroup()
    {
        return ((m_RP.m_nPassGroupID << 24) | (m_RP.m_CurState & GS_BLEND_MASK));
    }
    ILINE bool ShouldApplyFogCorrection()
    {
        return PackBlendModeAndPassGroup() != m_uLastBlendFlagsPassGroup;
    }

    void FX_CommitStates(const SShaderTechnique* pTech, const SShaderPass* pPass, bool bUseMaterialState);

    _inline void FX_DrawRE(CShader* sh, SShaderPass* sl)
    {
        int bFogOverrided = 0;
        // Unlock all VB (if needed) and set current streams
        FX_CommitStreams(sl);

        if (ShouldApplyFogCorrection())
        {
            FX_FogCorrection();
        }

        if (m_RP.m_pRE)
        {
            m_RP.m_pRE->mfDraw(sh, sl);
        }
        else
        {
            FX_DrawIndexedMesh(eptTriangleList);
        }
    }

    ILINE long FX_SetVStream(int nID, const void* pB, uint32 nOffs, uint32 nStride, [[maybe_unused]] uint32 nFreq = 1)
    {
        FUNCTION_PROFILER_RENDER_FLAT
        D3DBuffer* pVB = (D3DBuffer*)pB;
        HRESULT h = S_OK;
        SStreamInfo& sinfo = m_RP.m_VertexStreams[nID];
        if (sinfo.pStream != pVB || sinfo.nOffset != nOffs || sinfo.nStride != nStride)
        {
            sinfo.pStream = pVB;
            sinfo.nOffset = nOffs;
            sinfo.nStride = nStride;
            m_DevMan.BindVB(nID, 1, &pVB, &nOffs, &nStride);
        }

        //assert(h == S_OK);
        return h;
    }


    virtual long FX_SetIStream(const void* pB, uint32 nOffs, RenderIndexType idxType) override
    {
#if !defined(_RELEASE) && !defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
        IF (idxType == Index32 || idxType == Index16 && (nOffs & 1), 0)
        {
            __debugbreak();
        }
#endif

        D3DBuffer* pIB = (D3DBuffer*)pB;
        HRESULT h = S_OK;
#if !defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
        if (m_RP.m_pIndexStream != pIB)
        {
            m_RP.m_pIndexStream = pIB;
            m_DevMan.BindIB(pIB, 0, DXGI_FORMAT_R16_UINT);
        }
        m_RP.m_IndexStreamOffset = nOffs;
        m_RP.m_IndexStreamType = idxType;
#else
        if (m_RP.m_pIndexStream != pIB || m_RP.m_IndexStreamOffset != nOffs || m_RP.m_IndexStreamType != idxType)
        {
            m_RP.m_pIndexStream = pIB;
            m_RP.m_IndexStreamOffset = nOffs;
            m_RP.m_IndexStreamType = idxType;
            m_DevMan.BindIB(pIB, nOffs, (DXGI_FORMAT) idxType);
        }
#endif

        assert(h == S_OK);
        return h;
    }

    ILINE uint32 ApplyIndexBufferBindOffset(uint32 firstIndex)
    {
#if !defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
        return firstIndex + (m_RP.m_IndexStreamOffset >> 1);
#else
        return firstIndex;
#endif
    }

    ILINE long FX_ResetVertexDeclaration()
    {
        GetDeviceContext().IASetInputLayout(NULL);
        m_pLastVDeclaration = NULL;
        return S_OK;
    }

    ILINE D3DPrimitiveType FX_ConvertPrimitiveType(const eRenderPrimitiveType eType) const
    {
        assert(eType != eptHWSkinGroups);
        return (D3DPrimitiveType) eType;
    }

    virtual void FX_DrawIndexedPrimitive(const eRenderPrimitiveType eType, const int nVBOffset, [[maybe_unused]] const int nMinVertexIndex, const int nVerticesCount, const int nStartIndex, const int nNumIndices, bool bInstanced = false) override
    {
        int nPrimitives = 0;

        if (eType == eptTriangleList)
        {
            nPrimitives = nNumIndices / 3;
            assert(nNumIndices % 3 == 0);
        }
        else
        {
            switch (eType)
            {
            case ept4ControlPointPatchList:
                nPrimitives = nNumIndices >> 2;
                assert(nNumIndices % 4 == 0);
                break;
            case ept3ControlPointPatchList:
                nPrimitives = nNumIndices / 3;
                assert(nNumIndices % 3 == 0);
                break;
            case eptTriangleStrip:
                nPrimitives = nNumIndices - 2;
                assert(nNumIndices > 2);
                break;
            case eptLineList:
                nPrimitives = nNumIndices >> 1;
                assert(nNumIndices % 2 == 0);
                break;
        #ifdef _DEBUG
            default:
                assert(0);      // not supported
                return;
        #endif
            }
        }

        const D3DPrimitiveType eNativePType = FX_ConvertPrimitiveType(eType);

        SetPrimitiveTopology(eNativePType);
        if (bInstanced)
        {
            m_DevMan.DrawIndexedInstanced(nNumIndices, nVerticesCount, ApplyIndexBufferBindOffset(nStartIndex), nVBOffset, nVBOffset);
        }
        else
        {
            m_DevMan.DrawIndexed(nNumIndices, ApplyIndexBufferBindOffset(nStartIndex), nVBOffset);
        }

    #if defined(ENABLE_PROFILING_CODE)
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += nPrimitives;
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
    #endif
    }

    // This is a cross-platform low-level function for DIP call
    ILINE void FX_DrawPrimitive(eRenderPrimitiveType eType, int nStartVertex, int nVerticesCount, int nInstanceVertices = 0)
    {
        int nPrimitives;
        if (nInstanceVertices)
        {
            nPrimitives = nVerticesCount;
        }
        else
        {
            switch (eType)
            {
            case eptTriangleList:
                nPrimitives = nVerticesCount / 3;
                assert(nVerticesCount % 3 == 0);
                break;
            case eptTriangleStrip:
                nPrimitives = nVerticesCount - 2;
                assert(nVerticesCount > 2);
                break;
            case eptLineList:
                nPrimitives = nVerticesCount / 2;
                assert(nVerticesCount % 2 == 0);
                break;
            case eptLineStrip:
                nPrimitives = nVerticesCount - 1;
                assert(nVerticesCount > 1);
                break;
            case eptPointList:
            case ept1ControlPointPatchList:
                nPrimitives = nVerticesCount;
                assert(nVerticesCount > 0);
                break;
#ifndef _RELEASE
            default:
                assert(0);  // not supported
                return;
#endif
            }
        }

        const D3DPrimitiveType eNativePType = FX_ConvertPrimitiveType(eType);

        SetPrimitiveTopology(eNativePType);
        if (nInstanceVertices)
        {
            m_DevMan.DrawInstanced(nInstanceVertices, nVerticesCount, 0, nStartVertex);
        }
        else
        {
            m_DevMan.Draw(nVerticesCount, nStartVertex);
        }

#if defined(ENABLE_PROFILING_CODE)
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += nPrimitives;
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
#endif
    }

    bool FX_CommitStreams(SShaderPass* sl, bool bSetVertexDecl = true);

    // get the anti aliasing formats available for current width, height and BPP
    int GetAAFormat(TArray<SAAFormat>& Formats);

    virtual void EF_SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa);
    virtual void EF_SetSrgbWrite(bool sRGBWrite);

    void FX_HDRPostProcessing();
    void FX_FinalComposite();

    void FX_PreRender(int Stage);
    void FX_PostRender();

    void EF_Init();

    void EF_InitWaveTables();

    void EF_OnDemandVertexDeclaration(SOnDemandD3DVertexDeclaration& out, const int nStreamMask, const AZ::Vertex::Format& vertexformat, const bool bMorph, const bool bInstanced);

    void EF_InitD3DVertexDeclarations();
    void EF_SetCameraInfo();
    void FX_SetObjectTransform(CRenderObject* obj, CShader* pSH, int nTransFlags);
    bool FX_ObjectChange(CShader* Shader, CShaderResources* pRes, CRenderObject* pObject, IRenderElement* pRE);

private:
    void UpdateNearestChange(int flags);                            //Helper method for FX_ObjectChange to avoid I-cache misses
    void HandleDefaultObject();                                     //Helper method for FX_ObjectChange to avoid I-cache misses
    bool IsVelocityPassEnabled() const;                             //Helper method to detect if we should enable the velocity pass. Its needed to MB and TAA
    
    // Get pointers to current D3D11 shaders, set tessellation related RT flags and return true if tessellation is enabled for current object
    inline bool FX_SetTessellationShaders(CHWShader_D3D*& pCurHS, CHWShader_D3D*& pCurDS, const SShaderPass* pPass);
#ifdef TESSELLATION_RENDERER
    inline void FX_SetAdjacencyOffsetBuffer();
#endif

    bool InternalSaveToTIFF(ID3D11Texture2D* backBuffer, const char* filePath);

    // The cached screenshot path for screenshot request bus.
    char m_screenshotFilepathCache[AZ_MAX_PATH_LEN];
public:
    void EF_DrawDebugTools(SViewport& VP, const SRenderingPassInfo& passInfo);

    static void FX_DrawWire();
    static void FX_DrawNormals();
    static void FX_DrawTangents();
    static void FX_DrawFurBending();

    static void FX_FlushShader_ShadowGen();
    static void FX_FlushShader_General();
    static void FX_FlushShader_ZPass();

    static void FX_SelectTechnique(CShader* pShader, SShaderTechnique* pTech);

    // Unbind a buffer if it is bound to a Vertex or Index buffer slot
    void FX_UnbindStreamSource(D3DBuffer* buffer);

    int m_sPrevX, m_sPrevY, m_sPrevWdt, m_sPrevHgt;
    bool m_bsPrev;
    void EF_Scissor(bool bEnable, int sX, int sY, int sWdt, int sHgt);
    bool EF_GetScissorState(int& sX, int& sY, int& sWdt, int& sHgt);

    void EF_SetFogColor(const ColorF& Color);
    void FX_FogCorrection();

    byte FX_StartQuery(SRendItem* pRI);
    void FX_EndQuery(SRendItem* pRI, byte bStartQ);

#if defined(DO_RENDERSTATS)
    ILINE bool FX_ShouldTrackStats()
    {
        bool bShouldTrackStats = (CV_r_stats == 6 || m_pDebugRenderNode || m_bCollectDrawCallsInfo || m_bCollectDrawCallsInfoPerNode);
        return bShouldTrackStats;
    }
    void FX_TrackStats(CRenderObject* pObj, IRenderMesh* pRenderMesh);
#endif

    void FX_DrawIndexedMesh (const eRenderPrimitiveType nPrimType);
    bool FX_SetResourcesState();

    int  EF_Preprocess(SRendItem* ri, uint32 nums, uint32 nume, RenderFunc pRenderFunc, const SRenderingPassInfo& passInfo);
    void EF_SortRenderLists(SRenderListDesc* pRLD, int nThreadID, bool bUseDist = false, bool bUseJobSystem = false);
    void EF_SortRenderList(int nList, int nAW, SRenderListDesc* pRLD, int nThread, bool bUseDist = false);

    void FX_StartBatching();
    void FX_ProcessBatchesList(int nums, int nume, uint32 nBatchFilter, uint32 nBatchExcludeFilter = 0);

    // Only do expensive DX12 resource set building for PC DX12
#if defined(CRY_USE_DX12)
    void PerFrameValidateResourceSets();
#endif

    void FX_ProcessZPassRenderLists();
    void FX_ProcessZPassRender_List(ERenderListID list, uint32 filter);
    void FX_ProcessThicknessRenderLists();
    void FX_ProcessSoftAlphaTestRenderLists();
    void FX_ProcessSkinRenderLists(int nList, void (* RenderFunc)(), bool bLighting);
    void FX_ProcessEyeOverlayRenderLists(int nList, void (* RenderFunc)(), bool bLightin);
    void FX_ProcessHalfResParticlesRenderList(int nList, void (* RenderFunc)(), bool bLighting);
    void FX_WaterVolumesPreprocess();
    void FX_ProcessPostRenderLists(uint32 nBatchFilter);
    void FX_ProcessRenderList(int nList, uint32 nBatchFilter, bool bSetRenderFunc = true);
    void FX_ProcessRenderList(int nums, int nume, int nList, int nAfterWater, void (* RenderFunc)(), bool bLighting, uint32 nBatchFilter = FB_GENERAL, uint32 nBatchExcludeFilter = 0);
    _inline void FX_ProcessRenderList(int nList, int nAfterWater, void (* RenderFunc)(), bool bLighting, uint32 nBatchFilter = FB_GENERAL, uint32 nBatchExcludeFilter = 0)
    {
        FX_ProcessRenderList(m_RP.m_pRLD->m_nStartRI[nAfterWater][nList], m_RP.m_pRLD->m_nEndRI[nAfterWater][nList], nList, nAfterWater, RenderFunc, bLighting, nBatchFilter, nBatchExcludeFilter);
    }

    void FX_WaterVolumesCaustics();
    void FX_WaterVolumesCausticsPreprocess(N3DEngineCommon::SCausticInfo& causticInfo);
    bool FX_WaterVolumesCausticsUpdateGrid(N3DEngineCommon::SCausticInfo& causticInfo);

    void EF_ProcessRenderLists(void (* RenderFunc)(), int nFlags, SViewport& VP, const SRenderingPassInfo& passInfo, bool bSync3DEngineJobs);
    void FX_ProcessPostGroups(int nums, int nume);

    void EF_PrintProfileInfo();

    virtual void EF_EndEf3D (int nFlags, int nPrecacheUpdateId, int nNearPrecacheUpdateId, const SRenderingPassInfo& passInfo);
    virtual void EF_EndEf2D(bool bSort); // 2d only

    virtual WIN_HWND GetHWND() { return m_hWnd; }
    virtual bool SetWindowIcon(const char* path);

    virtual void SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa);
    virtual void SetSrgbWrite(bool srgbWrite);

    //////////////////////////////////////////////////////////////////////////
public:
    bool IsDeviceLost() { return(m_bDeviceLost != 0); }

#if defined(FEATURE_SVO_GI)
    virtual ISvoRenderer* GetISvoRenderer();
#endif

    virtual IRenderAuxGeom* GetIRenderAuxGeom([[maybe_unused]] void* jobID = 0)
    {
#if defined(ENABLE_RENDER_AUX_GEOM)
        if (m_pRenderAuxGeomD3D)
        {
            return m_pRenderAuxGeomD3D->GetRenderAuxGeom(jobID);
        }
#endif
        return &m_renderAuxGeomNull;
    }

    virtual IColorGradingController* GetIColorGradingController()
    {
        return m_pColorGradingControllerD3D;
    }

    virtual IStereoRenderer* GetIStereoRenderer()
    {
        return m_pStereoRenderer;
    }

    virtual ITexture* Create2DTexture(const char* name, int width, int height, int numMips, int flags, unsigned char* data, ETEX_Format format)
    {
        return CTexture::Create2DTexture(name, width, height, numMips, flags, data, format, format);
    }

    CTiledShading& GetTiledShading()
    {
        return *m_pTiledShading;
    }

    CStandardGraphicsPipeline& GetGraphicsPipeline()
    {
        return *m_GraphicsPipeline;
    }

    CVolumetricFog& GetVolumetricFog()
    {
        return m_volumetricFog;
    }

    PerInstanceConstantBufferPool& GetPerInstanceConstantBufferPool()
    {
        return m_PerInstanceConstantBufferPool;
    }

    PerInstanceConstantBufferPool* GetPerInstanceConstantBufferPoolPointer()
    {
        return &m_PerInstanceConstantBufferPool;
    }

    virtual void PushFogVolume(class CREFogVolume* pFogVolume, const SRenderingPassInfo& passInfo)
    {
        GetVolumetricFog().PushFogVolume(pFogVolume, passInfo);
    }

    CD3DStereoRenderer& GetS3DRend() const { return *m_pStereoRenderer; }
    bool IsStereoEnabled() const;

    void RT_PrepareStereo(int mode, int output);
    void RT_CopyToStereoTex(int channel);
    void RT_UpdateTrackingStates();
    void RT_DisplayStereo();

    void RT_DrawVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer, const AZ::VideoRenderer::DrawArguments& drawArguments) final;

    virtual void EnableGPUTimers2(bool bEnabled)
    {
        if (bEnabled)
        {
            CD3DProfilingGPUTimer::EnableTiming();
        }
        else
        {
            CD3DProfilingGPUTimer::DisableTiming();
        }
    }

    virtual void AllowGPUTimers2(bool bAllow)
    {
        if (bAllow)
        {
            CD3DProfilingGPUTimer::AllowTiming();
        }
        else
        {
            CD3DProfilingGPUTimer::DisallowTiming();
        }
    }

    virtual const RPProfilerStats* GetRPPStats(ERenderPipelineProfilerStats eStat, bool bCalledFromMainThread = true) const { return m_pPipelineProfiler ? &m_pPipelineProfiler->GetBasicStats(eStat, bCalledFromMainThread ? m_RP.m_nFillThreadID : m_RP.m_nProcessThreadID) : nullptr; }
    virtual const RPProfilerStats* GetRPPStatsArray(bool bCalledFromMainThread = true) const { return m_pPipelineProfiler ? m_pPipelineProfiler->GetBasicStatsArray(bCalledFromMainThread ? m_RP.m_nFillThreadID : m_RP.m_nProcessThreadID) : nullptr; }

    virtual int GetPolygonCountByType([[maybe_unused]] uint32 EFSList, [[maybe_unused]] EVertexCostTypes vct, [[maybe_unused]] uint32 z, [[maybe_unused]] bool bCalledFromMainThread = true)
    {
#if defined(ENABLE_PROFILING_CODE)
        return m_RP.m_PS[bCalledFromMainThread ? m_RP.m_nFillThreadID : m_RP.m_nProcessThreadID].m_nPolygonsByTypes[EFSList][vct][z];
#else
        return 0;
#endif
    }

#ifdef SUPPORT_HW_MOUSE_CURSOR
    virtual IHWMouseCursor* GetIHWMouseCursor();
#endif

    virtual void StartLoadtimePlayback(ILoadtimeCallback* pCallback);
    virtual void StopLoadtimePlayback();

    // macros to implement the platform differences for pushing GPU Markers

#if defined(ENABLE_FRAME_PROFILER_LABELS)

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_H_SECTION_11
    #include AZ_RESTRICTED_FILE(DriverD3D_h)
#endif

#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(OPENGL)
    #define PROFILE_LABEL_GPU(_NAME) DXGLProfileLabel(_NAME);
    #define PROFILE_LABEL_PUSH_GPU(_NAME) DXGLProfileLabelPush(_NAME);
    #define PROFILE_LABEL_POP_GPU(_NAME) DXGLProfileLabelPop(_NAME);
#elif defined(CRY_USE_DX12)
    #define PROFILE_LABEL_GPU(_NAME) do { } while (0)
    #define PROFILE_LABEL_PUSH_GPU(_NAME) do { GetDeviceContext().PushMarker(_NAME); } while (0)
    #define PROFILE_LABEL_POP_GPU(_NAME) do { GetDeviceContext().PopMarker(); } while (0)
#else

    #define PROFILE_LABEL_GPU(X) do { wchar_t buf[256]; Unicode::Convert(buf, X); D3DPERF_SetMarker(0xffffffff, buf); } while (0)
    #define PROFILE_LABEL_PUSH_GPU(X) do { wchar_t buf[128]; Unicode::Convert(buf, X); D3DPERF_BeginEvent(0xff00ff00, buf); } while (0)
    #define PROFILE_LABEL_POP_GPU(X) do { D3DPERF_EndEvent(); } while (0)

#endif
#else
    #define PROFILE_LABEL_GPU(_NAME)
    #define PROFILE_LABEL_PUSH_GPU(_NAME)
    #define PROFILE_LABEL_POP_GPU(_NAME)
#endif

    void AddProfilerLabel([[maybe_unused]] const char* name) override { PROFILE_LABEL_GPU(name); }
    void BeginProfilerSection(const char* name, [[maybe_unused]] uint32 eProfileLabelFlags = 0) override { PROFILE_LABEL_PUSH_GPU(name); if (m_pPipelineProfiler) { m_pPipelineProfiler->BeginSection(name); } }
    void EndProfilerSection(const char* name) override { PROFILE_LABEL_POP_GPU(name); if (m_pPipelineProfiler) { m_pPipelineProfiler->EndSection(name); } }

private:
    void OnRendererFreeResources(int flags) override;
    void HandleDisplayPropertyChanges();

    void FX_SetAlphaTestState(float alphaRef);

#if defined(ENABLE_PROFILING_CODE)
#if DRIVERD3D_H_TRAIT_DEFSAVETEXTURE || defined(OPENGL)
    ID3D11Texture2D* m_pSaveTexture[2];
#endif

    // Surfaces used to capture the current screen
    unsigned int m_captureFlipFlop;
    // Variables used for frame buffer capture and callback
    ICaptureFrameListener* m_pCaptureCallBack[MAXFRAMECAPTURECALLBACK];
    unsigned int m_frameCaptureRegisterNum;
    int m_nScreenCaptureRequestFrame[RT_COMMAND_BUF_COUNT];
    int m_screenCapTexHandle[RT_COMMAND_BUF_COUNT];
#endif

    class FrameBufferDescription
    {
    public:

        ~FrameBufferDescription();

        byte* pDest = nullptr;
        ID3D11Texture2D* pBackBufferTex = nullptr;
        ID3D11Texture2D* pTmpTexture = nullptr;
        ID3D11Texture2D* tempZtex = nullptr;
        float* depthData = nullptr;

        D3D11_TEXTURE2D_DESC backBufferDesc;
        D3D11_MAPPED_SUBRESOURCE resource;

        bool includeAlpha = false;

        //size information
        int outputBytesPerPixel;
        int texSize;

        const static int inputBytesPerPixel = 4;
    };

    FrameBufferDescription* m_frameBufDesc = nullptr;

    bool PrepFrameCapture(FrameBufferDescription& frameBufDesc, CTexture* pRenderTarget = 0);
    void FillFrameBuffer(FrameBufferDescription& frameBufDesc, bool redBlueSwap);

    bool CaptureFrameBufferToFile(const char* pFilePath, CTexture* pRenderTarget = 0);
    // Store local pointers to CVars used for capturing
    void CacheCaptureCVars();
    void CaptureFrameBuffer();
    // Resolve supersampled back buffer
    void ResolveSupersampledBackbuffer();
    // Scale back buffer contents to match viewport
    void ScaleBackbufferToViewport();
    ICVar* CV_capture_frames;
    ICVar* CV_capture_folder;
    ICVar* CV_capture_buffer;
    ICVar* CV_capture_frame_once;
    ICVar* CV_capture_file_name;
    ICVar* CV_capture_file_prefix;

#if defined(WIN32) || defined(WIN64)
    ICVar* CV_r_FullscreenWindow;
    ICVar* CV_r_FullscreenNativeRes;
    bool m_fullscreenWindow;
#endif

#if DRIVERD3D_H_TRAIT_DEFREGISTEREDWINDOWHANDLER
    bool m_registeredWindoWHandler = false;
#endif

    virtual void EnablePipelineProfiler(bool bEnable);

#if defined(ENABLE_RENDER_AUX_GEOM)
    CRenderAuxGeomD3D* m_pRenderAuxGeomD3D;
#endif
    CAuxGeomCB_Null m_renderAuxGeomNull;

    static TArray<CRenderObject*> s_tempObjects[2];
    static TArray<SRendItem*> s_tempRIs;

    // For matching calls between EF_Init and FX_PipelineShutdown
    bool m_shaderPipelineInitialized = false;

    bool m_clearShadowMaskTexture = false;

#if defined(WIN32)
public:
    // Called to inspect window messages sent to this renderer's windows
    virtual bool HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

private:
    uint m_nConnectedMonitors; // The number of monitors currently connected to the system
    bool m_bDisplayChanged; // Dirty-flag set when the number of monitors in the system changes
#endif

    // Gmem variables
private:
    mutable EGmemDepthStencilMode m_gmemDepthStencilMode;
};

enum
{
    SPCBI_NUMBER_OF_BUFFERS = 64
};

struct SPersistentConstBufferInfo
{
    uint64                      m_crc[SPCBI_NUMBER_OF_BUFFERS];
    AzRHI::ConstantBuffer*    m_pStaticInstCB[SPCBI_NUMBER_OF_BUFFERS];
    int                             m_frameID;
    int                             m_buffer;
};

///////////////////////////////////////////////////////////////////////////////
inline void CD3D9Renderer::BindContextToThread([[maybe_unused]] DWORD threadID)
{
#if ENABLE_CONTEXT_THREAD_CHECKING
    m_DeviceOwningthreadID = threadID;
#endif
}

///////////////////////////////////////////////////////////////////////////////
inline void CD3D9Renderer::CheckContextThreadAccess() const
{
#if ENABLE_CONTEXT_THREAD_CHECKING
    if (m_DeviceOwningthreadID != CryGetCurrentThreadId())
    {
        CryFatalError("accessing d3d11 immediate context from unbound thread!");
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
inline DWORD CD3D9Renderer::GetBoundThreadID() const
{
    return m_DeviceOwningthreadID;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
inline D3DDevice& CD3D9Renderer::GetDevice()
{
    AZ_Assert(m_Device, "Device is null");
    return *m_Device;
}

inline const D3DDevice& CD3D9Renderer::GetDevice() const
{
    AZ_Assert(m_Device, "Device is null");
    return *m_Device;
}

///////////////////////////////////////////////////////////////////////////////
inline D3DDeviceContext& CD3D9Renderer::GetDeviceContext()
{
    CheckContextThreadAccess();
    AZ_Assert(m_DeviceContext, "Device context is null");
    return *m_DeviceContext;
}

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_H_SECTION_12
    #include AZ_RESTRICTED_FILE(DriverD3D_h)
#endif

#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
void UserOverrideDisplayProperties(DXGI_MODE_DESC& desc);
#endif

extern StaticInstance<CD3D9Renderer> gcpRendD3D;

//=========================================================================================

#include "D3DHWShader.h"

#include <RenderDll/Common/FencedIB.h>
#include <RenderDll/Common/FencedVB.h>
#include "DeviceManager/TempDynBuffer.h"

//=========================================================================================

#define STREAMED_TEXTURE_USAGE (CDeviceManager::USAGE_STREAMING)

void EnableCloseButton(void* hWnd, bool enabled);
