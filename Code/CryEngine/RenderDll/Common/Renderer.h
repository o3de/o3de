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

#include <CryPool/PoolAlloc.h>
#include "TextMessages.h"                                                           // CTextMessages
#include "RenderAuxGeom.h"
#include "Shaders/Vertex.h"
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzCore/Jobs/LegacyJobExecutor.h>
#include <AzFramework/Viewport/ViewportBus.h>
#include <MathConversion.h>

#include <LoadScreenBus.h>

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED && !defined(NULL_RENDERER)
namespace AzRTT
{
    class RenderContextManager;
}
#endif // AZ_RENDER_TO_TEXTURE_GEM_ENABLED && !defined(NULL_RENDERER)

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define RENDERER_H_SECTION_1 1
#define RENDERER_H_SECTION_2 2
#define RENDERER_H_SECTION_3 3
#define RENDERER_H_SECTION_4 4
#define RENDERER_H_SECTION_5 5
#define RENDERER_H_SECTION_6 6
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_H_SECTION_1
    #include AZ_RESTRICTED_FILE(Renderer_h)
#endif

typedef void (PROCRENDEF)(SShaderPass* l, int nPrimType);

#define USE_NATIVE_DEPTH 1

enum eAntialiasingType
{
    eAT_NOAA = 0,
    eAT_FXAA,
    eAT_SMAA1TX,
    eAT_TAA,
    eAT_AAMODES_COUNT,

    eAT_DEFAULT_AA = eAT_TAA,

    eAT_NOAA_MASK = (1 << eAT_NOAA),
    eAT_FXAA_MASK = (1 << eAT_FXAA),
    eAT_SMAA_MASK = (1 << eAT_SMAA1TX),
    eAT_TAA_MASK  = (1 << eAT_TAA),

    eAT_TEMPORAL_MASK = (eAT_TAA_MASK | eAT_SMAA_MASK),
    eAT_JITTER_MASK = (eAT_TAA_MASK)
};

static const char* s_pszAAModes[eAT_AAMODES_COUNT] =
{
    "NO AA",
    "FXAA",
    "SMAA 1tx",
    "TAA"
};

struct ICVar;
struct ShadowMapFrustum;
struct IStatObj;
struct SShaderPass;
class CD3DStereoRenderer;
class IOpticsManager;
struct SDynTexture2;
struct SDepthTexture;
namespace Render
{
    namespace Debug
    {
        class VRAMDriller;
    }
}

typedef int (* pDrawModelFunc)(void);

#define RENDER_LOCK_CS(csLock) CryAutoCriticalSection __AL__##__FILE__##_LINE(csLock)

//=============================================================

#define D3DRGBA(r, g, b, a)                                  \
    ((((int)((a) * 255)) << 24) | (((int)((r) * 255)) << 16) \
     |   (((int)((g) * 255)) << 8) | (int)((b) * 255)        \
    )

#define CONSOLES_BACKBUFFER_WIDTH CRenderer::CV_r_ConsoleBackbufferWidth
#define CONSOLES_BACKBUFFER_HEIGHT CRenderer::CV_r_ConsoleBackbufferHeight

// Assuming 24 bits of depth precision
#define DBT_SKY_CULL_DEPTH  0.99999994f

#define DEF_SHAD_DBT_DEFAULT_VAL 1

#if defined(AZ_PLATFORM_IOS) || defined (AZ_PLATFORM_ANDROID)
    #define TEXSTREAMING_DEFAULT_VAL 0
#else
    #define TEXSTREAMING_DEFAULT_VAL 1
#endif


#define GEOM_INSTANCING_DEFAULT_VAL 1
#define COLOR_GRADING_DEFAULT_VAL 1
#define SUNSHAFTS_DEFAULT_VAL 2
#define HDR_RANGE_ADAPT_DEFAULT_VAL 0
#define HDR_RENDERING_DEFAULT_VAL 1
#define SHADOWS_POOL_DEFAULT_VAL 1
#define SHADOWS_CLIP_VOL_DEFAULT_VAL 1
#define SHADOWS_BLUR_DEFAULT_VAL 3
#define TEXPREALLOCATLAS_DEFAULT_VAL 0
#define TEXMAXANISOTROPY_DEFAULT_VAL 8
#if defined(CONSOLE)
    #define TEXNOANISOALPHATEST_DEFAULT_VAL 1
#else
    #define TEXNOANISOALPHATEST_DEFAULT_VAL 0
#endif
#define ENVTEXRES_DEFAULT_VAL 3
#define WATERREFLQUAL_DEFAULT_VAL 4
#define DOF_DEFAULT_VAL 2
#define SHADERS_ALLOW_COMPILATION_DEFAULT_VAL 1
#define SHADERS_PREACTIVATE_DEFAULT_VAL 3
#define CUSTOMVISIONS_DEFAULT_VAL 3
#define FLARES_DEFAULT_VAL 1
#define WATERVOLCAUSTICS_DEFAULT_VAL 1
#define FLARES_HQSHAFTS_DEFAULT_VAL 1
#define DEF_SHAD_DBT_STENCIL_DEFAULT_VAL 1
#define DEF_SHAD_SSS_DEFAULT_VAL 1

#define MULTITHREADED_DEFAULT_VAL 1
#define ZPASS_DEPTH_SORT_DEFAULT_VAL 1
#define TEXSTREAMING_UPDATETYPE_DEFAULT_VAL 1

#if defined(WIN32) || defined(APPLE) || defined(LINUX) || defined(SET_CBUFFER_NATIVE_DEPTH_DEAFULT_VAL_TO_1)
    #define CBUFFER_NATIVE_DEPTH_DEAFULT_VAL 1
#else
    #define CBUFFER_NATIVE_DEPTH_DEAFULT_VAL 0
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_H_SECTION_2
    #include AZ_RESTRICTED_FILE(Renderer_h)
#endif

//////////////////////////////////////////////////////////////////////
// Class to manage memory for Skinning Renderer Data
class CSkinningDataPool
{
public:
    CSkinningDataPool()
        : m_pPool(NULL)
        , m_pPages(NULL)
        , m_nPoolSize(0)
        , m_nPoolUsed(0)
        , m_nPageAllocated(0)
    {}

    ~CSkinningDataPool()
    {
        // free temp pages
        SPage* pPage = m_pPages;
        while (pPage)
        {
            SPage* pPageToDelete = pPage;
            pPage = pPage->pNext;

            CryModuleMemalignFree(pPageToDelete);
        }

        // free pool
        CryModuleMemalignFree(m_pPool);
    }

    byte* Allocate(size_t nBytes)
    {
        // If available use allocated page space
        uint32 nPoolUsed = ~0;
        do
        {
            nPoolUsed = *const_cast<volatile size_t*>(&m_nPoolUsed);
            size_t nPoolFree = m_nPoolSize - nPoolUsed;
            if (nPoolFree < nBytes)
            {
                break; // not enough memory, use fallback
            }
            if (CryInterlockedCompareExchange(alias_cast<volatile LONG*>(&m_nPoolUsed), nPoolUsed + nBytes, nPoolUsed) == nPoolUsed)
            {
                return &m_pPool[nPoolUsed];
            }
        } while (true);

        // Create memory
        byte* pMemory = alias_cast<byte*>(CryModuleMemalign(Align(nBytes, 16) + 16, 16));
        SPage* pNewPage = alias_cast<SPage*>(pMemory);

        // Assign page
        volatile SPage* pPages = 0;
        do
        {
            pPages = *(const_cast<volatile SPage**>(&m_pPages));
            pNewPage->pNext = alias_cast<SPage*>(pPages);
        } while (CryInterlockedCompareExchangePointer(alias_cast<void* volatile*>(&m_pPages), pNewPage, (void*)pPages) != pPages);

        CryInterlockedAdd((volatile int*)&m_nPageAllocated, nBytes);

        return pMemory + 16;
    }

    void ClearPool()
    {
        m_nPoolUsed = 0;
        if (m_nPageAllocated)
        {
            // free temp pages
            SPage* pPage = m_pPages;
            while (pPage)
            {
                SPage* pPageToDelete = pPage;
                pPage = pPage->pNext;

                CryModuleMemalignFree(pPageToDelete);
            }

            // adjust pool size
            CryModuleMemalignFree(m_pPool);
            m_nPoolSize += m_nPageAllocated;
            m_pPool = alias_cast<byte*>(CryModuleMemalign(m_nPoolSize, 16));

            // reset state
            m_pPages = NULL;
            m_nPageAllocated = 0;
        }
    }

    void FreePoolMemory()
    {
        // free temp pages
        SPage* pPage = m_pPages;
        while (pPage)
        {
            SPage* pPageToDelete = pPage;
            pPage = pPage->pNext;

            CryModuleMemalignFree(pPageToDelete);
        }

        // free pool
        CryModuleMemalignFree(m_pPool);

        m_pPool = NULL;
        m_pPages = NULL;
        m_nPoolSize = 0;
        m_nPoolUsed = 0;
        m_nPageAllocated = 0;
    }

    size_t AllocatedMemory()
    {
        return m_nPoolSize + m_nPageAllocated;
    }
private:
    struct SPage
    {
        SPage* pNext;
    };

    byte* m_pPool;
    SPage* m_pPages;
    size_t m_nPoolSize;
    size_t m_nPoolUsed;
    size_t m_nPageAllocated;
};

namespace LegacyInternal
{
    class JobExecutorPool final
    {
    public:
        static const AZ::u32 NumPools = 3; // Skinning data is triple buffered, see usage of m_SkinningDataPool

        void AdvanceCurrent()
        {
            m_current = (m_current + 1) % JobExecutorPool::NumPools;
            auto& currentAllocatedList = m_allocated[m_current];

            // Move all current instances to the free list
            m_freeList.reserve(m_freeList.size() + currentAllocatedList.size());
            for (auto& allocatedEntry : currentAllocatedList)
            {
                m_freeList.emplace_back(std::move(allocatedEntry));
            }
            currentAllocatedList.clear();
        }

        AZ::LegacyJobExecutor* Allocate()
        {
            auto& currentAllocatedList = m_allocated[m_current];

            if (!m_freeList.empty())
            {
                // move from the freelist
                currentAllocatedList.emplace_back(std::move(m_freeList.back()));
                m_freeList.pop_back();
            }
            else
            {
                currentAllocatedList.emplace_back(AZStd::make_unique<AZ::LegacyJobExecutor>());
            }

            return currentAllocatedList.back().get();
        }

    private:
        // We point to instances because LegacyJobExecutor instances are not movable
        using JobExecutorList = AZStd::vector<AZStd::unique_ptr<AZ::LegacyJobExecutor>>;

        JobExecutorList m_allocated[JobExecutorPool::NumPools];
        JobExecutorList m_freeList;
        AZ::u32 m_current = 0;
    };
}

//////////////////////////////////////////////////////////////////////
class CFillRateManager
    : private stl::PSyncDebug
{
public:
    CFillRateManager()
        : m_fTotalPixels(0.f)
        , m_fMaxPixels(1e9f)
    {}
    void Reset()
    {
        m_fTotalPixels = 0.f;
        m_afPixels.resize(0);
    }
    float GetMaxPixels() const
    {
        return m_fMaxPixels;
    }
    void AddPixelCount(float fPixels);
    void ComputeMaxPixels();

private:
    float m_fTotalPixels;
    float m_fMaxPixels;
    FastDynArray<float> m_afPixels;
};

//////////////////////////////////////////////////////////////////////
// 3D engine duplicated data for non-thread safe data
namespace N3DEngineCommon
{
    struct SOceanInfo
    {
        SOceanInfo()
            : m_nOceanRenderFlags(0)
            , m_fWaterLevel(0.0f)
            , m_vMeshParams(0.0f, 0.0f, 0.0f, 0.0f) { };
        Vec4 m_vMeshParams;
        float m_fWaterLevel;
        uint8 m_nOceanRenderFlags;
    };

    struct SVisAreaInfo
    {
        SVisAreaInfo()
            : nFlags(0)
        {
        };
        uint32 nFlags;
    };

    struct SRainOccluder
    {
        SRainOccluder()
            : m_RndMesh(0)
            , m_WorldMat(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0) {}
        _smart_ptr<IRenderMesh> m_RndMesh;
        Matrix34 m_WorldMat;
    };

    typedef std::vector<SRainOccluder> ArrOccluders;

    struct SRainOccluders
    {
        ArrOccluders m_arrOccluders;
        ArrOccluders m_arrCurrOccluders[RT_COMMAND_BUF_COUNT];
        size_t m_nNumOccluders;
        bool m_bProcessed[MAX_GPU_NUM];

        SRainOccluders()
            : m_nNumOccluders(0)
        {
            for (int i = 0; i < MAX_GPU_NUM; ++i)
            {
                m_bProcessed[i] = true;
            }
        }
        ~SRainOccluders()                                                  { Release(); }
        void Release(bool bAll = false)
        {
            stl::free_container(m_arrOccluders);
            m_nNumOccluders = 0;
            if (bAll)
            {
                for (int i = 0; i < RT_COMMAND_BUF_COUNT; i++)
                {
                    stl::free_container(m_arrCurrOccluders[i]);
                }
            }
            for (int i = 0; i < MAX_GPU_NUM; ++i)
            {
                m_bProcessed[i] = true;
            }
        }
    };

    struct SCausticInfo
    {
        SCausticInfo()
            : m_pCausticQuadMesh(0)
            , m_nCausticMeshWidth(0)
            , m_nCausticMeshHeight(0)
            , m_nCausticQuadTaps(0)
            , m_nVertexCount(0)
            , m_nIndexCount(0)
        {
        }

        ~SCausticInfo() { Release(); }
        void Release()
        {
            m_pCausticQuadMesh = NULL;
        }

        _smart_ptr<IRenderMesh> m_pCausticQuadMesh;
        uint32 m_nCausticMeshWidth;
        uint32 m_nCausticMeshHeight;
        uint32 m_nCausticQuadTaps;
        uint32 m_nVertexCount;
        uint32 m_nIndexCount;

        Matrix44A m_mCausticMatr;
        Matrix34 m_mCausticViewMatr;
    };
}

struct S3DEngineCommon
{
    enum EVisAreaFlags
    {
        VAF_EXISTS_FOR_POSITION = (1 << 0),
        VAF_CONNECTED_TO_OUTDOOR = (1 << 1),
        VAF_AFFECTED_BY_OUT_LIGHTS = (1 << 2),
        VAF_MASK = VAF_EXISTS_FOR_POSITION | VAF_CONNECTED_TO_OUTDOOR | VAF_AFFECTED_BY_OUT_LIGHTS
    };

    N3DEngineCommon::SVisAreaInfo m_pCamVisAreaInfo;
    N3DEngineCommon::SOceanInfo m_OceanInfo;
    N3DEngineCommon::SRainOccluders m_RainOccluders;
    N3DEngineCommon::SCausticInfo m_CausticInfo;
    SRainParams                                         m_RainInfo;
    SSnowParams                                         m_SnowInfo;

    void Update(threadID nThreadID);
    void UpdateRainInfo(threadID nThreadID);
    void UpdateRainOccInfo(int nThreadID);
    void UpdateSnowInfo(int nThreadID);
};

struct SShowRenderTargetInfo
{
    SShowRenderTargetInfo() { Reset(); }
    void Reset()
    {
        bShowList = false;
        bDisplayTransparent = false;
        col = 2;
        rtList.clear();
    }

    bool bShowList;
    bool bDisplayTransparent;
    int col;
    struct RT
    {
        int textureID;
        Vec4 channelWeight;
        bool bFiltered;
        bool bRGBKEncoded;
        bool bAliased;
    };
    std::vector<RT> rtList;
};

struct SRenderTileInfo
{
    SRenderTileInfo() { nPosX = nPosY = nGridSizeX = nGridSizeY = 0.f; }
    f32 nPosX, nPosY, nGridSizeX, nGridSizeY;
};

class RendererAssetListener
    : public AzFramework::LegacyAssetEventBus::MultiHandler
{
public:
    RendererAssetListener(IRenderer* renderer);
    void Connect();
    void Disconnect();

    void OnFileChanged(AZStd::string assetName) override;
private:
    IRenderer* m_renderer;
};


//////////////////////////////////////////////////////////////////////
struct SRenderPipeline;
class CRenderer
    : public IRenderer
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_H_SECTION_3
    #include AZ_RESTRICTED_FILE(Renderer_h)
#endif
{
public:

    DEFINE_ALIGNED_DATA(Matrix44A, m_IdentityMatrix, 16);
    DEFINE_ALIGNED_DATA(Matrix44A, m_ViewMatrix, 16);
    DEFINE_ALIGNED_DATA(Matrix44A, m_CameraMatrix, 16);
    DEFINE_ALIGNED_DATA(Matrix44A, m_CameraZeroMatrix[RT_COMMAND_BUF_COUNT], 16);

    DEFINE_ALIGNED_DATA(Matrix44A, m_ProjMatrix, 16);
    DEFINE_ALIGNED_DATA(Matrix44A, m_ProjNoJitterMatrix, 16);
    DEFINE_ALIGNED_DATA(Matrix44A, m_TranspOrigCameraProjMatrix, 16);
    DEFINE_ALIGNED_DATA(Matrix44A, m_ViewProjMatrix, 16);
    DEFINE_ALIGNED_DATA(Matrix44A, m_ViewProjNoJitterMatrix, 16);
    DEFINE_ALIGNED_DATA(Matrix44A, m_ViewProjNoTranslateMatrix, 16);
    DEFINE_ALIGNED_DATA(Matrix44A, m_ViewProjInverseMatrix, 16);
    DEFINE_ALIGNED_DATA(Matrix44A, m_TempMatrices[4][8], 16);

    struct PreviousFrameMatrixSet
    {
        Vec3 m_WorldViewPosition;

        Matrix44A m_ViewMatrix;
        Matrix44A m_ViewNoTranslateMatrix;

        // These are always de-jittered.
        Matrix44A m_ProjMatrix;
        Matrix44A m_ViewProjMatrix;
        Matrix44A m_ViewProjNoTranslateMatrix;
    };

    PreviousFrameMatrixSet m_PreviousFrameMatrixSets[MAX_NUM_VIEWPORTS][2];

    const PreviousFrameMatrixSet& GetPreviousFrameMatrixSet() const
    {
        return m_PreviousFrameMatrixSets[m_CurViewportID][m_CurRenderEye];
    }

    static float GetRealTime()
    {
        const auto& renderPipeline = gRenDev->m_RP;
        return renderPipeline.m_TI[renderPipeline.m_nProcessThreadID].m_RealTime;
    }

    static float GetElapsedTime()
    {
        return gEnv->pTimer->GetFrameTime();
    }

    DEFINE_ALIGNED_DATA(Matrix44A, m_CameraMatrixNearest, 16); //[RT_COMMAND_BUF_COUNT][2], 16);

    float m_TemporalJitterMipBias;
    Vec4 m_TemporalJitterClipSpace;

    byte              m_bDeviceLost;
    byte              m_bSystemResourcesInit;
    byte              m_bSystemTargetsInit;
    bool              m_bAquireDeviceThread;
    bool              m_bInitialized;
    bool              m_bDualStereoSupport;

    SRenderThread* m_pRT;

    virtual SRenderPipeline* GetRenderPipeline() override { return &m_RP; }
    virtual SRenderThread* GetRenderThread() override { return m_pRT; }
    virtual CShaderMan* GetShaderManager() override { return &m_cEF; }
    virtual uint32 GetFrameReset() override { return m_nFrameReset; }
    virtual CDeviceBufferManager* GetDeviceBufferManager() override { return &m_DevBufMan; }

    // Shaders pipeline states
    //=============================================================================================================
    CDeviceManager m_DevMan;
    CDeviceBufferManager m_DevBufMan;
    SRenderPipeline m_RP;
    //=============================================================================================================

    float m_fTimeWaitForMain[RT_COMMAND_BUF_COUNT];
    float m_fTimeWaitForRender[RT_COMMAND_BUF_COUNT];
    float m_fTimeProcessedRT[RT_COMMAND_BUF_COUNT];
    float m_fTimeProcessedGPU[RT_COMMAND_BUF_COUNT];
    float m_fTimeWaitForGPU[RT_COMMAND_BUF_COUNT];
    float m_fTimeGPUIdlePercent[RT_COMMAND_BUF_COUNT];

    float m_fRTTimeEndFrame;
    float m_fRTTimeSceneRender;
    float m_fRTTimeMiscRender;

    int              m_CurVertBufferSize;
    int              m_CurIndexBufferSize;

    int  m_VSync;
    int  m_Predicated;
    int  m_MSAA;
    int  m_MSAA_quality;
    int  m_MSAA_samples;
    int  m_deskwidth, m_deskheight;
    int  m_nHDRType;

    // Index of the current viewport used for rendering.
    int m_CurViewportID;
    // Index of the current eye used for rendering.
    int m_CurRenderEye;

#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
    float m_overrideRefreshRate;
    int m_overrideScanlineOrder;
#endif

#if defined(WIN32) || defined(WIN64)
    int m_prefMonX, m_prefMonY, m_prefMonWidth, m_prefMonHeight;
#endif

    int m_nStencilMaskRef;

    byte  m_bDeviceSupportsInstancing;

    uint32  m_bDeviceSupports_NVDBT : 1;

    uint32  m_bDeviceSupportsFP16Separate : 1;
    uint32  m_bDeviceSupportsFP16Filter : 1;
    uint32  m_bDeviceSupportsR32FRendertarget : 1;
    uint32  m_bDeviceSupportsVertexTexture : 1;
    uint32  m_bDeviceSupportsTessellation : 1;
    uint32  m_bDeviceSupportsGeometryShaders : 1;

    uint32 m_bEditor : 1; // Render instance created from editor
    uint32 m_bShaderCacheGen : 1; // Render instance create from shader cache gen mode
    uint32 m_bUseHWSkinning : 1;
    uint32 m_bShadersPresort : 1;
    uint32 m_bEndLevelLoading : 1;
    uint32 m_bLevelUnloading : 1;
    uint32 m_bStartLevelLoading : 1;
    uint32 m_bInLevel : 1;
    uint32 m_bUseWaterTessHW : 1;
    uint32 m_bUseSilhouettePOM : 1;
    uint32 m_bUseSpecularAntialiasing : 1;
    uint32 m_UseGlobalMipBias : 1;
    uint32 m_bIsWindowActive : 1;
    uint32 m_bInShutdown : 1;
    uint32 m_bDeferredDecals : 1;
    uint32 m_bShadowsEnabled : 1;
    uint32 m_bCloudShadowsEnabled : 1;
#if defined(VOLUMETRIC_FOG_SHADOWS)
    uint32 m_bVolFogShadowsEnabled : 1;
    uint32 m_bVolFogCloudShadowsEnabled : 1;
#endif

    uint8 m_nDisableTemporalEffects;
    bool m_bUseGPUFriendlyBatching[2];
    uint32 m_nGPULimited;  // How many frames we are GPU limited
    int8 m_nCurMinAniso;
    int8 m_nCurMaxAniso;

    uint32 m_nShadowPoolHeight;
    uint32 m_nShadowPoolWidth;

    ICVar* m_CVWidth;
    ICVar* m_CVHeight;
    ICVar* m_CVFullScreen;
    ICVar* m_CVColorBits;
    ICVar* m_CVDisplayInfo;

    ColorF m_CurFontColor;

    SFogState m_FSStack[8];
    int m_nCurFSStackLevel;

    AZStd::string m_apiVersion;
    AZStd::string m_adapterDescription;
    DWORD m_Features;
    int m_MaxTextureSize;
    size_t m_MaxTextureMemory;
    int m_nShadowTexSize;

    float m_fLastGamma;
    float m_fLastBrightness;
    float m_fLastContrast;
    float m_fDeltaGamma;
    uint32 m_nLastNoHWGamma;

    float m_fogCullDistance;

    static Vec2 s_overscanBorders;
    static float s_previousTexelsPerMeter;

    static int CV_r_UsePersistentRTForModelHUD;

    enum
    {
        nMeshPoolMaxTimeoutCounter = 150    /*150 ms*/
    };
    int m_nMeshPoolTimeoutCounter;

    // Cached verts/inds used for sprites
    SVF_P3F_C4B_T2F* m_pSpriteVerts;
    uint16* m_pSpriteInds;

    // Rendering Drillers
    Render::Debug::VRAMDriller* m_vramDriller = nullptr;

protected:

    typedef std::list<IRenderDebugListener*> TListRenderDebugListeners;
    TListRenderDebugListeners m_listRenderDebugListeners;

    SShowRenderTargetInfo m_showRenderTargetInfo;

    static void Cmd_ShowRenderTarget(IConsoleCmdArgs* pArgs);

public:


    // ----
    virtual ITexture* GetWhiteTexture() override;// { return CTexture::s_ptexWhite; }
    virtual ITexture* GetTextureForName(const char* name, uint32 nFlags, ETEX_Format eFormat) override;// { return CTexture::ForName(name, nFlags, eFormat); }

    virtual Matrix44A& GetViewProjectionMatrix() override { return m_ViewProjMatrix; }
    virtual void SetTranspOrigCameraProjMatrix(Matrix44A& matrix) { m_TranspOrigCameraProjMatrix = matrix; }


    //

    CRenderer();
    virtual ~CRenderer();

    virtual void InitRenderer();

    virtual void PostInit();

    virtual void PostLevelLoading();

    void PreShutDown();
    void PostShutDown();

    // Multithreading support
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_H_SECTION_4
    #include AZ_RESTRICTED_FILE(Renderer_h)
#endif

    virtual void DrawStringU(IFFont_RenderProxy* pFont, float x, float y, float z, const char* pStr, bool asciiMultiLine, const STextDrawContext& ctx) const;
    virtual void RT_DrawStringU(IFFont_RenderProxy* pFont, float x, float y, float z, const char* pStr, bool asciiMultiLine, const STextDrawContext& ctx) const = 0;

    virtual void RT_DrawLines(Vec3 v[], int nump, ColorF& col, int flags, float fGround) = 0;

    virtual void RT_PresentFast() = 0;

    virtual int  RT_CurThreadList();
    virtual void RT_BeginFrame() = 0;
    virtual void RT_EndFrame() = 0;
    virtual void RT_EndFrame([[maybe_unused]] bool isLoading) {};
    virtual void RT_ForceSwapBuffers() = 0;
    virtual void RT_SwitchToNativeResolutionBackbuffer(bool resolveBackBuffer) = 0;
    virtual void RT_Init() = 0;
    virtual void RT_ShutDown(uint32 nFlags) = 0;
    virtual bool RT_CreateDevice() = 0;
    virtual void RT_Reset() = 0;
    virtual void RT_RenderTextMessages();
    virtual void RT_SetCull(int nMode) = 0;
    virtual void RT_SetScissor(bool bEnable, int x, int y, int width, int height) = 0;
    virtual void RT_RenderScene(int nFlags, SThreadInfo& TI, RenderFunc pRenderFunc) = 0;
    virtual void RT_PrepareStereo(int mode, int output) = 0;
    virtual void RT_CopyToStereoTex(int channel) = 0;
    virtual void RT_UpdateTrackingStates() = 0;
    virtual void RT_DisplayStereo() = 0;
    virtual void RT_SetCameraInfo() = 0;
    virtual void RT_SetStereoCamera() = 0;
    virtual void RT_CreateResource(SResourceAsync* pRes) = 0;
    virtual void RT_ReleaseResource(SResourceAsync* pRes) = 0;
    virtual void RT_ReleaseRenderResources() = 0;
    virtual void RT_UnbindResources() = 0;
    virtual void RT_UnbindTMUs() = 0;
    virtual void RT_CreateRenderResources() = 0;
    virtual void RT_PrecacheDefaultShaders() = 0;
    virtual void RT_ReadFrameBuffer(unsigned char* pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX, int nScaledY) = 0;
    virtual void RT_CreateREPostProcess(CRendElementBase** re);
    virtual void RT_ReleaseVBStream(void* pVB, int nStream) = 0;
    virtual void RT_ReleaseCB(void* pCB) = 0;
    virtual void RT_DrawDynVB(SVF_P3F_C4B_T2F* pBuf, uint16* pInds, uint32 nVerts, uint32 nInds, const PublicRenderPrimitiveType nPrimType) = 0;
    virtual void RT_DrawDynVBUI(SVF_P2F_C4B_T2F_F4B* pBuf, uint16* pInds, uint32 nVerts, uint32 nInds, const PublicRenderPrimitiveType nPrimType) = 0;
    virtual void RT_Draw2dImage(float xpos, float ypos, float w, float h, CTexture* pTexture, float s0, float t0, float s1, float t1, float angle, DWORD col, float z) = 0;
    virtual void RT_Draw2dImageStretchMode(bool bStretch) = 0;
    virtual void RT_Push2dImage(float xpos, float ypos, float w, float h, CTexture* pTexture, float s0, float t0, float s1, float t1, float angle, DWORD col, float z, float stereoDepth) = 0;
    virtual void RT_Draw2dImageList() = 0;
    virtual void RT_DrawImageWithUV(float xpos, float ypos, float z, float w, float h, int texture_id, float* s, float* t, DWORD col, bool filtered = true) = 0;

    virtual void RT_PushRenderTarget(int nTarget, CTexture* pTex, SDepthTexture* pDS, int nS) = 0;
    virtual void RT_PopRenderTarget(int nTarget) = 0;
    virtual void RT_SetViewport(int x, int y, int width, int height, int id = 0) = 0;
    virtual void RT_ClearTarget(ITexture* pTex, const ColorF& color) = 0;
    virtual void RT_RenderDebug(bool bRenderStats = true) = 0;
    virtual void RT_SetRendererCVar(ICVar* pCVar, const char* pArgText, const bool bSilentMode = false) = 0;

    virtual void RT_PostLevelLoading();

    void RT_DisableTemporalEffects();

    virtual bool FlushRTCommands(bool bWait, bool bImmediatelly, bool bForce);
    virtual bool ForceFlushRTCommands();

    virtual int GetOcclusionBuffer(uint16* pOutOcclBuffer, Matrix44* pmCamBuffer) = 0;
    virtual void WaitForParticleBuffer(threadID nThreadId) = 0;

    virtual void RequestFlushAllPendingTextureStreamingJobs(int nFrames) { m_nFlushAllPendingTextureStreamingJobs = nFrames; }
    virtual void SetTexturesStreamingGlobalMipFactor(float fFactor) { m_fTexturesStreamingGlobalMipFactor = fFactor; }

    virtual void SetRendererCVar(ICVar* pCVar, const char* pArgText, bool bSilentMode = false) = 0;

    virtual void EF_ClearTargetsImmediately(uint32 nFlags) = 0;
    virtual void EF_ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors, float fDepth, uint8 nStencil) = 0;
    virtual void EF_ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors) = 0;
    virtual void EF_ClearTargetsImmediately(uint32 nFlags, float fDepth, uint8 nStencil) = 0;

    virtual void EF_ClearTargetsLater(uint32 nFlags) = 0;
    virtual void EF_ClearTargetsLater(uint32 nFlags, const ColorF& Colors, float fDepth, uint8 nStencil) = 0;
    virtual void EF_ClearTargetsLater(uint32 nFlags, const ColorF& Colors) = 0;
    virtual void EF_ClearTargetsLater(uint32 nFlags, float fDepth, uint8 nStencil) = 0;

    virtual void EF_SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa) = 0;
    virtual void EF_SetSrgbWrite(bool sRGBWrite) = 0;

    virtual Matrix44A GetIdentityMatrix() { return m_IdentityMatrix; }


    //===============================================================================

    virtual void AddRenderDebugListener(IRenderDebugListener* pRenderDebugListener);
    virtual void RemoveRenderDebugListener(IRenderDebugListener* pRenderDebugListener);

    virtual ERenderType GetRenderType() const;

    virtual WIN_HWND Init(int x, int y, int width, int height, unsigned int cbpp, int zbpp, int sbits, bool fullscreen, bool isEditor, WIN_HINSTANCE hinst, WIN_HWND Glhwnd = 0, bool bReInit = false, const SCustomRenderInitArgs* pCustomArgs = 0, bool bShaderCacheGen = false) = 0;

    virtual WIN_HWND GetCurrentContextHWND() {  return GetHWND();   };
    virtual bool IsCurrentContextMainVP() { return true; }

    virtual int GetFeatures() {return m_Features; }
    virtual const void SetApiVersion(const AZStd::string& apiVersion) { m_apiVersion = apiVersion; }
    virtual const void SetAdapterDescription(const AZStd::string& adapterDescription) { m_adapterDescription = adapterDescription; }
    virtual const AZStd::string& GetApiVersion() const { return m_apiVersion; }
    virtual const AZStd::string& GetAdapterDescription() const { return m_adapterDescription; }
    
    unsigned long GetNvidiaDriverVersion() const { return m_nvidiaDriverVersion; }
    void SetNvidiaDriverVersion(unsigned long version) { m_nvidiaDriverVersion = version; }

    virtual int GetNumGeomInstances() const
    {
#if !defined(RELEASE)
        return m_RP.m_PS[m_RP.m_nProcessThreadID].m_nInsts;
#else
        return 0;
#endif
    };

    virtual int GetNumGeomInstanceDrawCalls() const
    {
#if !defined(RELEASE)
        return m_RP.m_PS[m_RP.m_nProcessThreadID].m_nInstCalls;
#else
        return 0;
#endif
    };

    virtual int GetCurrentNumberOfDrawCalls() const
    {
        int nDIPs = 0;
#if defined(ENABLE_PROFILING_CODE)
        int nThr = m_pRT->GetThreadList();
        for (int i = 0; i < EFSLIST_NUM; i++)
        {
            nDIPs += m_RP.m_PS[nThr].m_nDIPs[i];
        }
#endif
        return nDIPs;
    }

    virtual void GetCurrentNumberOfDrawCalls([[maybe_unused]] int& nGeneral, [[maybe_unused]] int& nShadowGen) const
    {
        int nDIPs = 0;
#if defined(ENABLE_PROFILING_CODE)
        int nThr = m_pRT->GetThreadList();
        for (int i = 0; i < EFSLIST_NUM; i++)
        {
            if (i == EFSLIST_SHADOW_GEN)
            {
                continue;
            }
            nDIPs += m_RP.m_PS[nThr].m_nDIPs[i];
        }
        nGeneral = nDIPs;
        nShadowGen = m_RP.m_PS[nThr].m_nDIPs[EFSLIST_SHADOW_GEN];
#endif
    }

    virtual int GetCurrentNumberOfDrawCalls([[maybe_unused]] uint32 EFSListMask) const
    {
        int nDIPs = 0;
#if defined(ENABLE_PROFILING_CODE)
        int nThr = m_pRT->GetThreadList();
        for (uint32 i = 0; i < EFSLIST_NUM; i++)
        {
            if ((1 << i) & EFSListMask)
            {
                nDIPs += m_RP.m_PS[nThr].m_nDIPs[i];
            }
        }
#endif
        return nDIPs;
    }

    virtual float GetCurrentDrawCallRTTimes(uint32 EFSListMask) const
    {
        float fDIPTimes = 0.0f;
        int nThr = m_pRT->GetThreadList();
        for (uint32 i = 0; i < EFSLIST_NUM; i++)
        {
            if ((1 << i) & EFSListMask)
            {
                fDIPTimes += m_RP.m_PS[nThr].m_fTimeDIPs[i];
            }
        }
        return fDIPTimes;
    }

    virtual void SetDebugRenderNode(IRenderNode* pRenderNode)
    {
        m_pDebugRenderNode = pRenderNode;
    }

    virtual bool IsDebugRenderNode(IRenderNode* pRenderNode) const
    {
        return (m_pDebugRenderNode && m_pDebugRenderNode == pRenderNode);
    }

    //! Fills array of all supported video formats (except low resolution formats)
    //! Returns number of formats, also when called with NULL
    virtual int EnumDisplayFormats(SDispFormat* formats) = 0;

    //! Return all supported by video card video AA formats
    virtual int EnumAAFormats(SAAFormat* formats) = 0;

    //! Changes resolution of the window/device (doen't require to reload the level
    virtual bool    ChangeResolution(int nNewWidth, int nNewHeight, int nNewColDepth, int nNewRefreshHZ, bool bFullScreen, bool bForceReset) = 0;
    virtual bool CheckDeviceLost() { return false; };

    virtual EScreenAspectRatio GetScreenAspect(int nWidth, int nHeight);

    virtual Vec2 SetViewportDownscale(float xscale, float yscale);

    virtual void  Release();
    virtual void  FreeResources(int nFlags);
    virtual void  InitSystemResources(int nFlags);
    virtual void  InitTexturesSemantics();

    bool HasLoadedDefaultResources() override { return m_bSystemResourcesInit == 1; }

    virtual void    BeginFrame() = 0;
    virtual void    RenderDebug(bool bRenderStats = true) = 0;
    virtual void    EndFrame() = 0;
    virtual void    LimitFramerate(const int maxFPS, const bool bUseSleep) = 0;

    virtual void    ForceSwapBuffers();

    virtual void    TryFlush() = 0;

    virtual void    Reset (void) = 0;

    virtual void    SetCamera(const CCamera& cam) = 0;
    virtual void    SetViewport(int x, int y, int width, int height, int id = 0) = 0;
    virtual void    SetScissor(int x = 0, int y = 0, int width = 0, int height = 0) = 0;
    virtual void  GetViewport(int* x, int* y, int* width, int* height) const = 0;

    virtual void    SetState(int State, int AlphaRef = -1)
    {
        m_pRT->RC_SetState(State, AlphaRef);
    }

    virtual void SetStencilState(int st, uint32 nStencRef, uint32 nStencMask, uint32 nStencWriteMask, bool bForceFullReadMask)
    {
        m_pRT->RC_SetStencilState(st, nStencRef, nStencMask, nStencWriteMask, bForceFullReadMask);
    }

    virtual void PushWireframeMode(int mode)
    {
        m_pRT->RC_PushWireframeMode(mode);
    }
    virtual void PopWireframeMode()
    {
        m_pRT->RC_PopWireframeMode();
    }

    virtual void FX_PushWireframeMode(int mode) = 0;
    virtual void FX_PopWireframeMode() = 0;

    virtual void    SetCullMode (int mode = R_CULL_BACK) = 0;
    virtual void    EnableVSync(bool enable) = 0;

    virtual void  DrawPrimitivesInternal(CVertexBuffer* src, int vert_num, const eRenderPrimitiveType prim_type) = 0;

    virtual bool    ChangeDisplay(unsigned int width, unsigned int height, unsigned int cbpp) = 0;
    virtual void  ChangeViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height, bool bMainViewport = false, float scaleWidth = 1.0f, float scaleHeight = 1.0f) = 0;

    virtual bool    SaveTga(unsigned char* sourcedata, int sourceformat, int w, int h, const char* filename, bool flip) const;

    //download an image to video memory. 0 in case of failure
    virtual unsigned int DownLoadToVideoMemory(const byte* data, int w, int h, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat = true, int filter = FILTER_BILINEAR, int Id = 0, const char* szCacheName = NULL, int flags = 0, EEndian eEndian = eLittleEndian, RectI* pRegion = NULL, bool bAsynDevTexCreation = false) = 0;
    virtual void UpdateTextureInVideoMemory(uint32 tnum, const byte* newdata, int posx, int posy, int w, int h, ETEX_Format eTFSrc = eTF_R8G8B8A8, int posz = 0, int sizez = 1) = 0;
    virtual unsigned int DownLoadToVideoMemory3D(const byte* data, int w, int h, int d, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat = true, int filter = FILTER_BILINEAR, int Id = 0, const char* szCacheName = NULL, int flags = 0, EEndian eEndian = eLittleEndian, RectI* pRegion = NULL, bool bAsynDevTexCreation = false) = 0;
    virtual unsigned int DownLoadToVideoMemoryCube(const byte* data, int w, int h, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat = true, int filter = FILTER_BILINEAR, int Id = 0, const char* szCacheName = NULL, int flags = 0, EEndian eEndian = eLittleEndian, RectI* pRegion = NULL, bool bAsynDevTexCreation = false) = 0;

    virtual bool DXTCompress(const byte* raw_data, int nWidth, int nHeight, ETEX_Format eTF, bool bUseHW, bool bGenMips, int nSrcBytesPerPix, MIPDXTcallback callback);
    virtual bool DXTDecompress(const byte* srcData, size_t srcFileSize, byte* dstData, int nWidth, int nHeight, int nMips, ETEX_Format eSrcTF, bool bUseHW, int nDstBytesPerPix);
    virtual bool    SetGammaDelta(float fGamma) = 0;

    virtual void    RemoveTexture(unsigned int TextureId) = 0;

    virtual void    SetTexture(int tnum);
    virtual void    SetTexture(int tnum, int nUnit);
    virtual void    SetWhiteTexture();
    virtual int     GetWhiteTextureId() const;
    virtual int     GetBlackTextureId() const;

    // Methods exposed to external libraries
    virtual void ApplyDepthTextureState(int unit, int nFilter, bool clamp) override;
    virtual ITexture* GetZTargetTexture() override;
    virtual int GetTextureState(const STexState& TS) override;
    virtual uint32 TextureDataSize(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nSlices, const ETEX_Format eTF, ETEX_TileMode eTM = eTM_None) override;
    virtual void ApplyForID(int nID, int nTUnit, int nTState, int nTexMaterialSlot, int nSUnit, bool useWhiteDefault) override;
    virtual ITexture* Create3DTexture(const char* szName, int nWidth, int nHeight, int nDepth, int nMips, int nFlags, const byte* pData, ETEX_Format eTFSrc, ETEX_Format eTFDst) override;
    virtual bool IsTextureExist(const ITexture* pTex) override;
    virtual const char* NameForTextureFormat(ETEX_Format eTF) override;
    virtual const char* NameForTextureType(ETEX_Type eTT) override;
    virtual bool IsVideoThreadModeEnabled() override;
    virtual IDynTexture* CreateDynTexture2(uint32 nWidth, uint32 nHeight, uint32 nTexFlags, const char* szSource, ETexPool eTexPool) override;
    virtual uint32 GetCurrentTextureAtlasSize() override;

    virtual void  PrintToScreen(float x, float y, float size, const char* buf);
    virtual void TextToScreen(float x, float y, const char* format, ...);
    virtual void TextToScreenColor(int x, int y, float r, float g, float b, float a, const char* format, ...);

    int GetRecursionLevel() override;

    int GetIntegerConfigurationValue(const char* varName, int defaultValue) override;
    float GetFloatConfigurationValue(const char* varName, float defaultValue) override;
    bool GetBooleanConfigurationValue(const char* varName, bool defaultValue) override;

    void  WriteXY(int x, int y, float xscale, float yscale, float r, float g, float b, float a, const char* message, ...) PRINTF_PARAMS(10, 11);
    void    Draw2dText(float posX, float posY, const char* pStr, const SDrawTextInfo& ti);
    void    Draw2dTextWithDepth(float posX, float posY, float posZ, const char* pStr, const SDrawTextInfo& ti);

    virtual void DrawTextQueued(Vec3 pos, SDrawTextInfo& ti, const char* text);
    virtual void DrawTextQueued(Vec3 pos, SDrawTextInfo& ti, const char* format, va_list args);

    virtual void Draw2dImage(float xpos, float ypos, float w, float h, int texture_id, float s0 = 0, float t0 = 0, float s1 = 1, float t1 = 1, float angle = 0, float r = 1, float g = 1, float b = 1, float a = 1, float z = 1) = 0;
    virtual void Push2dImage(float xpos, float ypos, float w, float h, int texture_id, float s0 = 0, float t0 = 0, float s1 = 1, float t1 = 1, float angle = 0,
        float r = 1, float g = 1, float b = 1, float a = 1, float z = 1, float stereoDepth = 0) = 0;
    virtual void Draw2dImageList() = 0;

    virtual void ResetToDefault() = 0;

    virtual void PrintResourcesLeaks() = 0;

    ILINE const bool IsEditorMode() const
    {
#if defined(CONSOLE)
        return false;
#else
        return (m_bEditor != 0);
#endif
    }

    ILINE const bool IsShaderCacheGenMode() const
    {
#if defined(CONSOLE)
        return false;
#else
        return (m_bShaderCacheGen != 0);
#endif
    }

    inline float ScaleCoordXInternal(float value) const
    {
        value *= float(m_CurViewport.nWidth) / 800.0f;
        return (value);
    }
    virtual float ScaleCoordX(float value) const
    {
        return ScaleCoordXInternal(value);
    }
    inline float ScaleCoordYInternal(float value) const
    {
        value *= float(m_CurViewport.nHeight) / 600.0f;
        return (value);
    }
    virtual float ScaleCoordY(float value) const
    {
        return ScaleCoordYInternal(value);
    }
    inline void ScaleCoordInternal(float& x, float& y) const
    {
        int viewPortX, viewPortY, viewPortW, viewPortH;
        GetViewport(&viewPortX, &viewPortY, &viewPortW, &viewPortH);
        x *= viewPortW / 800.0f;
        y *= viewPortH / 600.0f;
    }
    virtual void ScaleCoord(float& x, float& y) const
    {
        ScaleCoordInternal(x, y);
    }

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    bool IsRenderToTextureActive() const override;

    int GetWidth() const override;
    void SetWidth(int width);
    int GetHeight() const override;
    void SetHeight(int height);

    int GetOverlayWidth() const override;
    int GetOverlayHeight() const override;
#else
    void SetWidth(int nW) { m_width = nW; }
    void SetHeight(int nH) { m_height = nH; }

    virtual int     GetWidth() const { return (m_width); }
    virtual int     GetHeight() const { return (m_height); }

    virtual int GetOverlayWidth() const { return m_nativeWidth; }
    virtual int GetOverlayHeight() const { return m_nativeHeight; }
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    void SetPixelAspectRatio(float fPAR) {m_pixelAspectRatio = fPAR; }
    virtual float GetPixelAspectRatio() const { return (m_pixelAspectRatio); }

    int GetBackbufferWidth() { return m_backbufferWidth; }
    int GetBackbufferHeight() { return m_backbufferHeight; }

    void GetClampedWindowSize(int& widthPixels, int& heightPixels);

    inline int GetMaxSquareRasterDimension() const override
    {
        // m_MaxTextureSize is actually maxTextureDimension. Since MaxResolution refers to a 2D raster, the upper limit on a square raster is (m_MaxTextureSize/2)
        return (CV_r_CustomResMaxSize == s_CustomResMaxSize_USE_MAX_RESOURCES) ? (m_MaxTextureSize / 2) : clamp_tpl(CV_r_CustomResMaxSize, 32, (m_MaxTextureSize / 2));
    }

    virtual bool IsStereoEnabled() const { return false; }

    virtual float   GetNearestRangeMax() const { return (CV_r_DrawNearZRange); }

    //  Get mipmap distance factor (depends on screen width, screen height and aspect ratio)

    _inline float GetMipDistFactor() { return TANGENT30_2 * TANGENT30_2 / (GetHeight() * GetHeight()); }

    virtual int     GetWireframeMode() { return(m_wireframe_mode); }


    _inline const CCamera& GetCamera(void) { return(m_RP.m_TI[m_pRT->GetThreadList()].m_cam);  }

    virtual const CameraViewParameters& GetViewParameters() override
    {
        return(m_RP.m_TI[m_pRT->GetThreadList()].m_cam.m_viewParameters);
    }

    virtual void SetViewParameters(const CameraViewParameters& viewParameters) override
    {
        m_RP.m_TI[m_pRT->GetThreadList()].m_cam.m_viewParameters = viewParameters;
    }

    virtual CRenderView* GetRenderViewForThread(int nThreadID) final;
    Matrix44A GetCameraMatrix();

    void GetPolyCount([[maybe_unused]] int& nPolygons, [[maybe_unused]] int& nShadowPolys) const
    {
#if defined(ENABLE_PROFILING_CODE)
        nPolygons = GetPolyCount();
        nShadowPolys = m_RP.m_PS[m_RP.m_nFillThreadID].m_nPolygons[EFSLIST_SHADOW_GEN];
        nShadowPolys += m_RP.m_PS[m_RP.m_nFillThreadID].m_nPolygons[EFSLIST_SHADOW_PASS];
        nPolygons -= nShadowPolys;
#endif
    }

    int GetPolyCount() const
    {
#if defined(ENABLE_PROFILING_CODE)
        int nPolys = 0;
        for (int i = 0; i < EFSLIST_NUM; i++)
        {
            nPolys += m_RP.m_PS[m_pRT->IsMainThread()?m_RP.m_nFillThreadID:m_RP.m_nProcessThreadID].m_nPolygons[i];
        }
        return nPolys;
#else
        return 0;
#endif
    }

    virtual void SetMaterialColor(float r, float g, float b, float a) = 0;

    virtual bool WriteDDS(const byte* dat, int wdt, int hgt, int Size, const char* name, ETEX_Format eF, int NumMips);
    virtual bool WriteTGA(const byte* dat, int wdt, int hgt, const char* name, int src_bits_per_pixel, int dest_bits_per_pixel);
    virtual bool WriteJPG(const byte* dat, int wdt, int hgt, char* name, int src_bits_per_pixel, int nQuality = 100);

    virtual void GetMemoryUsage(ICrySizer* Sizer);

    virtual void GetBandwidthStats(float* fBandwidthRequested);

    virtual void SetTextureStreamListener(ITextureStreamListener* pListener);

    virtual void GetLogVBuffers() = 0;

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    // Returns a frame ID that is sequential for the active camera.  This is 
    // useful for camera-specific temporal data like motion vectors.
    virtual int GetCameraFrameID() const
    {
        const int nThreadID = m_pRT ? m_pRT->GetThreadList() : 0;
        return m_RP.m_TI[nThreadID].m_cam.GetFrameUpdateId();
    }
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    virtual int GetFrameID(bool bIncludeRecursiveCalls = true)
    {
        int nThreadID = m_pRT ? m_pRT->GetThreadList() : 0;
        if (bIncludeRecursiveCalls)
        {
            return m_RP.m_TI[nThreadID].m_nFrameID;
        }
        return m_RP.m_TI[nThreadID].m_nFrameUpdateID;
    }

    // GPU being updated
    virtual int32 RT_GetCurrGpuID() const override
    {
        return gRenDev->m_nFrameSwapID % gRenDev->GetActiveGPUCount();
    }

    // Project/UnProject.  Returns true if successful.
    virtual bool ProjectToScreen(float ptx, float pty, float ptz,
        float* sx, float* sy, float* sz) = 0;
    virtual int UnProject(float sx, float sy, float sz,
        float* px, float* py, float* pz,
        const float modelMatrix[16],
        const float projMatrix[16],
        const int    viewport[4]) = 0;
    virtual int UnProjectFromScreen(float  sx, float  sy, float  sz,
        float* px, float* py, float* pz) = 0;

    virtual void EF_RenderTextMessages();
    void RenderTextMessages(CTextMessages& messages);

    // Shadow Mapping
    virtual bool PrepareDepthMap(ShadowMapFrustum* SMSource, int nFrustumLOD = 0, bool bClearPool = false) = 0;
    virtual void DrawAllShadowsOnTheScreen() = 0;
    virtual void OnEntityDeleted(IRenderNode* pRenderNode) = 0;

    //for editor
    virtual void  GetModelViewMatrix(float* mat) = 0;
    virtual void  GetProjectionMatrix(float* mat) = 0;
    virtual void SetMatrices(float* pProjMat, float* pViewMat) = 0;

    virtual void ReadFrameBuffer(unsigned char* pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX = -1, int nScaledY = -1) = 0;

    //misc
    virtual bool ScreenShot(const char* filename = NULL, int width = 0) = 0;

    virtual int GetColorBpp()       { return m_cbpp; }
    virtual int GetDepthBpp()       { return m_zbpp; }
    virtual int GetStencilBpp()     { return m_sbpp; }

    virtual int ScreenToTexture(int nTexID) = 0;

    virtual void LockParticleVideoMemory([[maybe_unused]] uint32 nId) {}
    virtual void UnLockParticleVideoMemory([[maybe_unused]] uint32 nId){}

    virtual void ActivateLayer([[maybe_unused]] const char* pLayerName, [[maybe_unused]] bool activate) {}

    virtual void FlushPendingTextureTasks()
    {
        if (m_pRT)
        {
            m_pRT->RC_FlushTextureStreaming(true);
            FlushRTCommands(true, true, true);
        }
    }

    void SetShadowJittering (float shadowJittering)
    {
        m_shadowJittering = shadowJittering;
    }
    float GetShadowJittering() const { return m_shadowJittering; }

    // Shaders/Shaders support
    // RE - RenderElement
    bool m_bTimeProfileUpdated;
    int m_PrevProfiler;
    int m_nCurSlotProfiler;

    AZ::IO::HandleType m_logFileHandle;
    AZ::IO::HandleType m_logFileStrHandle;
    AZ::IO::HandleType m_logFileShHandle;
    inline void Logv(int RecLevel, const char* format, ...)
    {
        va_list argptr;

        if (m_logFileHandle != AZ::IO::InvalidHandle)
        {
            for (int i = 0; i < RecLevel; i++)
            {
                AZ::IO::Print(m_logFileHandle, "  ");
            }
            va_start (argptr, format);
            AZ::IO::PrintV(m_logFileHandle, format, argptr);
            va_end (argptr);
        }
    }
    inline void LogStrv(int RecLevel, const char* format, ...)
    {
        va_list argptr;

        if (m_logFileStrHandle != AZ::IO::InvalidHandle)
        {
            for (int i = 0; i < RecLevel; i++)
            {
                AZ::IO::Print(m_logFileStrHandle, "  ");
            }
            va_start (argptr, format);
            AZ::IO::PrintV(m_logFileStrHandle, format, argptr);
            va_end (argptr);
        }
    }
    inline void LogShv(int RecLevel, const char* format, ...)
    {
        va_list argptr;

        if (m_logFileShHandle != AZ::IO::InvalidHandle)
        {
            for (int i = 0; i < RecLevel; i++)
            {
                AZ::IO::Print(m_logFileShHandle, "  ");
            }
            va_start (argptr, format);
            AZ::IO::PrintV(m_logFileShHandle, format, argptr);
            va_end (argptr);
            gEnv->pFileIO->Flush(m_logFileShHandle);
        }
    }
    _inline void Log(const char* str)
    {
        if (m_logFileHandle != AZ::IO::InvalidHandle)
        {
            AZ::IO::Print(m_logFileHandle, str);
        }
    }

    void EF_AddClientPolys(const SRenderingPassInfo& passInfo);
    void EF_RemovePolysFromScene();

    bool FX_TryToMerge(CRenderObject* pNewObject, CRenderObject* pOldObject, IRenderElement* pRE, bool bResIdentical);
    virtual void* FX_AllocateCharInstCB(SSkinningData*, uint32) { return NULL; }
    virtual void  FX_ClearCharInstCB(uint32) {}

    void EF_TransformDLights();
    void EF_IdentityDLights();

    _inline void* EF_GetPointer(ESrcPointer ePT, int* Stride, EParamType Type, ESrcPointer Dst, int Flags)
    {
        void* p;

        if (m_RP.m_pRE)
        {
            p = m_RP.m_pRE->mfGetPointer(ePT, Stride, Type, Dst, Flags);
        }
        else
        {
            p = SRendItem::mfGetPointerCommon(ePT, Stride, Type, Dst, Flags);
        }

        return p;
    }
    inline void FX_StartMerging()
    {
        if (m_RP.m_FrameMerge != m_RP.m_Frame)
        {
            m_RP.m_FrameMerge = m_RP.m_Frame;
            int Size = m_RP.m_CurVFormat.GetStride();
            m_RP.m_StreamStride = Size;
            m_RP.m_CurVFormat.TryCalculateOffset(m_RP.m_StreamOffsetColor, AZ::Vertex::AttributeUsage::Color);
            m_RP.m_CurVFormat.TryCalculateOffset(m_RP.m_StreamOffsetTC, AZ::Vertex::AttributeUsage::TexCoord);
            m_RP.m_NextStreamPtr = m_RP.m_StreamPtr;
            m_RP.m_NextStreamPtrTang = m_RP.m_StreamPtrTang;
        }
    }

    _inline void EF_PushFog()
    {
        assert(m_pRT->IsRenderThread());
        int nLevel = m_nCurFSStackLevel;
        if (nLevel >= 8)
        {
            return;
        }
        memcpy(&m_FSStack[nLevel], &m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS, sizeof(SFogState));
        m_nCurFSStackLevel++;
    }
    _inline void EF_PopFog()
    {
        assert(m_pRT->IsRenderThread());
        int nLevel = m_nCurFSStackLevel;
        if (nLevel <= 0)
        {
            return;
        }
        nLevel--;
        bool bFog = m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_bEnable;
        if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS != m_FSStack[nLevel])
        {
            memcpy(&m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS, &m_FSStack[nLevel], sizeof(SFogState));
            SetFogColor(m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_FogColor);
        }
        else
        {
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_bEnable = m_FSStack[nLevel].m_bEnable;
        }
        bool bNewFog = m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_bEnable;
        m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_bEnable = bFog;
        EnableFog(bNewFog);
        m_nCurFSStackLevel--;
    }

    //================================================================================
    SViewport m_MainRTViewport;
    SViewport m_MainViewport;

    SViewport m_CurViewport;
    SViewport m_NewViewport;
    bool m_bViewportDirty;
    bool m_bViewportDisabled;

    int m_nCurVPStackLevel;
    SViewport m_VPStack[8];
    _inline void FX_PushVP()
    {
        int nLevel = m_nCurVPStackLevel;
        if (nLevel >= 8)
        {
            return;
        }
        memcpy(&m_VPStack[nLevel], &m_NewViewport, sizeof(SViewport));
        m_nCurVPStackLevel++;
    }
    _inline void FX_PopVP()
    {
        int nLevel = m_nCurVPStackLevel;
        if (nLevel <= 0)
        {
            return;
        }
        nLevel--;
        if (m_NewViewport != m_VPStack[nLevel])
        {
            memcpy(&m_NewViewport, &m_VPStack[nLevel], sizeof(SViewport));
            m_bViewportDirty = true;
        }
        m_nCurVPStackLevel--;
    }


    void EF_AddRTStat(CTexture* pTex, int nFlags = 0, int nW = -1, int nH = -1);
    void EF_PrintRTStats(const char* szName);

    int FX_ApplyShadowQuality();

    static eAntialiasingType FX_GetAntialiasingType()
    {
        return (eAntialiasingType) ((uint32)1 << min(CV_r_AntialiasingMode, eAT_AAMODES_COUNT - 1));
    }

    inline float GetTemporalJitterMipBias() const
    {
        return m_TemporalJitterMipBias;
    }

    void FX_ApplyShaderQuality(const EShaderType eST);

    virtual EShaderQuality EF_GetShaderQuality(EShaderType eST);
    virtual ERenderQuality EF_GetRenderQuality() const;

    void RefreshSystemShaders();
    uint32 EF_BatchFlags(SShaderItem& SH, CRenderObject* pObj, IRenderElement* re, const SRenderingPassInfo& passInfo);

    virtual void FX_PipelineShutdown(bool bFastShutdown = false) = 0;

    virtual IOpticsElementBase* CreateOptics(EFlareType type) const;

    virtual bool EF_PrecacheResource(IShader* pSH, float fMipFactor, float fTimeToReady, int Flags);
    virtual bool EF_PrecacheResource(SShaderItem* pSI, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId, int nCounter) = 0;
    virtual bool EF_PrecacheResource(ITexture* pTP, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId, int nCounter) = 0;
    virtual bool EF_PrecacheResource(IRenderMesh* pPB, _smart_ptr<IMaterial> pMaterial, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId);
    virtual bool EF_PrecacheResource(CDLight* pLS, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId);

    virtual CRenderObject* EF_AddPolygonToScene(SShaderItem& si, int numPts, const SVF_P3F_C4B_T2F* verts, const SPipTangents* tangs, CRenderObject* obj, const SRenderingPassInfo& passInfo, uint16* inds, int ninds, int nAW, const SRendItemSorter& rendItemSorter);
    virtual CRenderObject* EF_AddPolygonToScene(SShaderItem& si, CRenderObject* obj, const SRenderingPassInfo& passInfo, int numPts, int ninds, SVF_P3F_C4B_T2F*& verts, SPipTangents*& tangs, uint16*& inds, int nAW, const SRendItemSorter& rendItemSorter);

    virtual void FX_CheckOverflow(int nVerts, int nInds, IRenderElement* re, int* nNewVerts = NULL, int* nNewInds = NULL) override;
    virtual void FX_Start(CShader* ef, int nTech, CShaderResources* Res, IRenderElement* re) override;

    virtual int GenerateTextureId() override { return m_TexGenID++; }

    //==========================================================
    // external interface for shaders
    //==========================================================

    AZ::LegacyJobExecutor m_ComputeVerticesJobExecutors[RT_COMMAND_BUF_COUNT];

    CFillRateManager m_FillRateManager;

    void FinalizeRendItems(int nThreadID);
    void FinalizeRendItems_ReorderShadowRendItems(int nThreadID);
    void FinalizeRendItems_FindShadowFrustums(int nThreadID);
    void FinalizeRendItems_ReorderRendItems(int nThreadID);
    void FinalizeRendItems_ReorderRendItemList(int nAW, int nList, int nThreadID);
    void FinalizeRendItems_SortRenderLists(int nThreadID);

    void FinalizeShadowRendItems(int nThreadID);
    void RegisterFinalizeShadowJobs(int nThreadID);

    // Summary:
    virtual void BeginSpawningGeneratingRendItemJobs(int nThreadID);
    virtual void BeginSpawningShadowGeneratingRendItemJobs(int nThreadID);
    virtual void EndSpawningGeneratingRendItemJobs();

    AZ::LegacyJobExecutor* GetGenerateRendItemJobExecutor() override;
    AZ::LegacyJobExecutor* GetGenerateShadowRendItemJobExecutor() override;
    AZ::LegacyJobExecutor* GetGenerateRendItemJobExecutorPreProcess() override;
    AZ::LegacyJobExecutor* GetFinalizeRendItemJobExecutor(int nThreadID) override;
    AZ::LegacyJobExecutor* GetFinalizeShadowRendItemJobExecutor(int nThreadID) override;


    // Shaders management
    virtual void                EF_SetShaderMissCallback(ShaderCacheMissCallback callback);
    virtual const char* EF_GetShaderMissLogPath();
    virtual string*        EF_GetShaderNames(int& nNumShaders);
    virtual IShader*       EF_LoadShader (const char* name, int flags = 0, uint64 nMaskGen = 0);
    virtual SShaderItem EF_LoadShaderItem (const char* name, bool bShare, int flags = 0, SInputShaderResources* Res = NULL, uint64 nMaskGen = 0);
    virtual uint64          EF_GetRemapedShaderMaskGen(const char* name, uint64 nMaskGen = 0, bool bFixup = 0);

    virtual uint64      EF_GetShaderGlobalMaskGenFromString(const char* szShaderName, const char* szShaderGen, uint64 nMaskGen = 0);
    virtual AZStd::string EF_GetStringFromShaderGlobalMaskGen(const char* szShaderName, uint64 nMaskGen = 0);

    // reload file
    virtual bool EF_ReloadFile(const char* szFileName);
    virtual bool EF_ReloadFile_Request(const char* szFileName);
    virtual void EF_ReloadShaderFiles (int nCategory);
    virtual void EF_ReloadTextures();
    virtual int EF_LoadLightmap(const char* nameTex);
    virtual bool  EF_RenderEnvironmentCubeHDR (int size, Vec3& Pos, TArray<unsigned short>& vecData);
    virtual ITexture* EF_GetTextureByID(int Id);
    virtual ITexture* EF_GetTextureByName(const char* name, uint32 flags = 0);
    virtual ITexture* EF_LoadTexture(const char* nameTex, uint32 flags = 0);
    virtual ITexture* EF_LoadCubemapTexture(const char* nameTex, uint32 flags = 0);
    virtual ITexture* EF_LoadDefaultTexture(const char* nameTex);

    virtual const SShaderProfile& GetShaderProfile(EShaderType eST) const;
    virtual void EF_SetShaderQuality(EShaderType eST, EShaderQuality eSQ);

    virtual _smart_ptr<IImageFile> EF_LoadImage(const char* szFileName, uint32 nFlags);

    // Create new RE of type (edt)
    virtual IRenderElement* EF_CreateRE (EDataType edt);

    // Begin using shaders
    virtual void EF_StartEf (const SRenderingPassInfo& passInfo);

    virtual SRenderObjData* EF_GetObjData(CRenderObject* pObj, bool bCreate, int nThreadID);
    SRenderObjData* FX_GetObjData(CRenderObject* pObj, int nThreadID);

    // Get Object for RE transformation
    virtual CRenderObject* EF_GetObject_Temp (int nThreadID);
    CRenderObject* EF_DuplicateRO(CRenderObject* pObj, const SRenderingPassInfo& passInfo);

    // Add shader to the list (virtual)
    virtual void EF_AddEf (IRenderElement* pRE, SShaderItem& pSH,  CRenderObject* pObj, const SRenderingPassInfo& passInfo, int nList, int nAW, const SRendItemSorter& rendItemSorter);

    // Add shader to the list
    void EF_AddEf_NotVirtual (IRenderElement* pRE, SShaderItem& pSH, CRenderObject* pObj, const SRenderingPassInfo& passInfo, int nList, int nAW, const SRendItemSorter& rendItemSorter);

    // Draw all shaded REs in the list
    virtual void EF_EndEf3D (int nFlags, int nPrecacheUpdateId, int nNearPrecacheUpdateId, const SRenderingPassInfo& passInfo) = 0;

    virtual void EF_InvokeShadowMapRenderJobs(int nFlags) = 0;
    // 2d interface for shaders
    virtual void EF_EndEf2D(bool bSort) = 0;

    // Dynamic lights
    virtual bool EF_IsFakeDLight (const CDLight* Source);
    virtual void EF_ADDDlight(CDLight* Source, const SRenderingPassInfo& passInfo);
    virtual bool EF_AddDeferredDecal(const SDeferredDecal& rDecal);
    virtual uint32 EF_GetDeferredLightsNum(eDeferredLightType eLightType = eDLT_DeferredLight);
    virtual int EF_AddDeferredLight(const CDLight& pLight, float fMult, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter);
    virtual TArray<SRenderLight>* EF_GetDeferredLights(const SRenderingPassInfo& passInfo, eDeferredLightType eLightType = eDLT_DeferredLight);
    SRenderLight* EF_GetDeferredLightByID(const uint16 nLightID, const eDeferredLightType eLightType = eDLT_DeferredLight);
    virtual void EF_ClearDeferredLightsList();
    virtual void EF_ReleaseDeferredData();
    virtual void EF_ReleaseInputShaderResource(SInputShaderResources* pRes);
    virtual void EF_ClearLightsList();
    virtual bool EF_UpdateDLight(SRenderLight* pDL);
    void EF_CheckLightMaterial(CDLight* pLight, uint16 nRenderLightID, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter);

    // Water sim hits
    virtual void EF_AddWaterSimHit(const Vec3& vPos, float scale, float strength);
    virtual void EF_DrawWaterSimHits();

    virtual void EF_QueryImpl(ERenderQueryTypes eQuery, void* pInOut0, uint32 nInOutSize0, void* pInOut1, uint32 nInOutSize1);
    virtual void FX_SetState(int st, int AlphaRef = -1, int RestoreState = 0) = 0;
    void FX_SetStencilState(int st, uint32 nStencRef, uint32 nStencMask, uint32 nStencWriteMask, bool bForceFullReadMask = false);

    //////////////////////////////////////////////////////////////////////////
    // Deferred ambient passes

    virtual uint8 EF_AddDeferredClipVolume(const IClipVolume* pClipVolume);
    virtual bool EF_SetDeferredClipVolumeBlendData(const IClipVolume* pVolume, const SClipVolumeBlendInfo& blendInfo);

    virtual void EF_ClearDeferredClipVolumesList();

    //////////////////////////////////////////////////////////////////////////
    // Post processing effects interfaces

    virtual void EF_SetPostEffectParam(const char* pParam, float fValue, bool bForceValue = false);
    virtual void EF_SetPostEffectParamVec4(const char* pParam, const Vec4& pValue, bool bForceValue = false);
    virtual void EF_SetPostEffectParamString(const char* pParam, const char* pszArg);

    virtual void EF_GetPostEffectParam(const char* pParam, float& fValue);
    virtual void EF_GetPostEffectParamVec4(const char* pParam, Vec4& pValue);
    virtual void EF_GetPostEffectParamString(const char* pParam, const char*& pszArg);

    virtual int32 EF_GetPostEffectID(const char* pPostEffectName);

    virtual void EF_ResetPostEffects(bool bOnSpecChange = false);

    virtual void SyncPostEffects();

    virtual void EF_DisableTemporalEffects();

    virtual void ForceGC();

    virtual void RT_ResetGlass() {}

    // create/delete RenderMesh object
    virtual _smart_ptr<IRenderMesh> CreateRenderMesh(
        const char* szType
        , const char* szSourceName
        , IRenderMesh::SInitParamerers* pInitParams = NULL
        , ERenderMeshType eBufType = eRMT_Static
        );

    virtual _smart_ptr<IRenderMesh> CreateRenderMeshInitialized(
        const void* pVertBuffer, int nVertCount, const AZ::Vertex::Format& vertexFormat,
        const vtx_idx* pIndices, int nIndices,
        const PublicRenderPrimitiveType nPrimetiveType, const char* szType, const char* szSourceName, ERenderMeshType eBufType = eRMT_Static,
        int nMatInfoCount = 1, int nClientTextureBindID = 0,
        bool (* PrepareBufferCallback)(IRenderMesh*, bool) = NULL,
        void* CustomData = NULL,
        bool bOnlyVideoBuffer = false, bool bPrecache = true, const SPipTangents* pTangents = NULL, bool bLockForThreadAcc = false, Vec3* pNormals = NULL);

    virtual int GetMaxActiveTexturesARB() { return 0; }

    //////////////////////////////////////////////////////////////////////
    // Replacement functions for the Font engine ( vlad: for font can be used old functions )
    virtual bool FontUpdateTexture(int nTexId, int X, int Y, int USize, int VSize, byte* pData) = 0;
    virtual void FontSetTexture(int nTexId, int nFilterMode) = 0;
    virtual void FontSetRenderingState(bool overrideViewProjMatrices, TransformationMatrices& backupMatrices) = 0;
    virtual void FontSetBlending(int src, int dst, int baseState) = 0;
    virtual void FontRestoreRenderingState(bool overrideViewProjMatrices, const TransformationMatrices& restoringMatrices) = 0;

    //////////////////////////////////////////////////////////////////////
    // Used for pausing timer related stuff (eg: for texture animations, and shader 'time' parameter)
    void PauseTimer(bool bPause) {  m_bPauseTimer = bPause; }
    virtual IShaderPublicParams* CreateShaderPublicParams();

    virtual void GetThreadIDs(threadID& mainThreadID, threadID& renderThreadID) const;

    enum ESPM
    {
        ESPM_PUSH = 0, ESPM_POP = 1
    };
    virtual void SetProfileMarker([[maybe_unused]] const char* label, [[maybe_unused]] ESPM mode) const {};

    virtual uint16 PushFogVolumeContribution(const SFogVolumeData& fogVolData, const SRenderingPassInfo& passInfo);
    void GetFogVolumeContribution(uint16 idx, SFogVolumeData& fogVolData) const;
    virtual void PushFogVolume([[maybe_unused]] class CREFogVolume* pFogVolume, [[maybe_unused]] const SRenderingPassInfo& passInfo) {assert(false); }

    virtual int GetMaxTextureSize() { return m_MaxTextureSize; }

    virtual void SetCloudShadowsParams(int nTexID, const Vec3& speed, float tiling, bool invert, float brightness)
    {
        m_cloudShadowTexId = nTexID;
        m_cloudShadowSpeed = speed;
        m_cloudShadowTiling = tiling;
        m_cloudShadowInvert = invert;
        m_cloudShadowBrightness = brightness;
    }
    int GetCloudShadowTextureId() const { return m_cloudShadowTexId; }

    virtual ShadowFrustumMGPUCache* GetShadowFrustumMGPUCache() { return &m_ShadowFrustumMGPUCache; }

    virtual const StaticArray<int, MAX_GSM_LODS_NUM>& GetCachedShadowsResolution() const { return m_CachedShadowsResolution; }
    virtual void SetCachedShadowsResolution(const StaticArray<int, MAX_GSM_LODS_NUM>& arrResolutions) { m_CachedShadowsResolution = arrResolutions; }
    virtual void UpdateCachedShadowsLodCount(int nGsmLods) const override;

    bool IsHDRModeEnabled() const
    {
        // If there's no support for floating point render targets, we disable HDR.
        return (gRenDev && gRenDev->UseHalfFloatRenderTargets()) && !CV_r_measureoverdraw && !m_wireframe_mode;
    }

    bool IsShadowPassEnabled() const
    {
        return (CV_r_ShadowPass && CV_r_usezpass && !m_wireframe_mode) ? true : false;
    }

    bool IsCustomRenderModeEnabled(uint32 nRenderModeMask);
    virtual bool IsPost3DRendererEnabled() const;

    virtual const char* GetTextureFormatName(ETEX_Format eTF);
    virtual int GetTextureFormatDataSize(int nWidth, int nHeight, int nDepth, int nMips, ETEX_Format eTF);
    virtual void SetDefaultMaterials(_smart_ptr<IMaterial> pDefMat, _smart_ptr<IMaterial> pTerrainDefMat) { m_pDefaultMaterial = pDefMat; m_pTerrainDefaultMaterial = pTerrainDefMat; }
    virtual byte* GetTextureSubImageData32([[maybe_unused]] byte* pData, [[maybe_unused]] int nDataSize, [[maybe_unused]] int nX, [[maybe_unused]] int nY, [[maybe_unused]] int nW, [[maybe_unused]] int nH, [[maybe_unused]] CTexture* pTex){return 0; }

    virtual void PrecacheTexture(ITexture* pTP, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId, int nCounter = 1);

    virtual SSkinningData* EF_CreateSkinningData(uint32 nNumBones, bool bNeedJobSyncVar, bool bUseMatrixSkinning = false);
    virtual SSkinningData* EF_CreateRemappedSkinningData(uint32 nNumBones, SSkinningData* pSourceSkinningData, uint32 nCustomDataSize, uint32 pairGuid);
    virtual void EF_ClearSkinningDataPool();
    virtual int EF_GetSkinningPoolID();


    bool EF_GetParticleListAndBatchFlags(uint32& nBatchFlags, int& nList, SShaderItem& shaderItem, CRenderObject* pRO, const SRenderingPassInfo& passInfo);
    virtual void ClearShaderItem(SShaderItem* pShaderItem);
    virtual void UpdateShaderItem(SShaderItem* pShaderItem, _smart_ptr<IMaterial> pMaterial);
    virtual void ForceUpdateShaderItem(SShaderItem* pShaderItem, _smart_ptr<IMaterial> pMaterial);
    virtual void RefreshShaderResourceConstants(SShaderItem* pShaderItem, IMaterial* pMaterial);

    void RT_UpdateShaderItem (SShaderItem* pShaderItem, IMaterial* material);
    void RT_RefreshShaderResourceConstants(SShaderItem* shaderItem) const;

    bool UseHalfFloatRenderTargets();

    virtual bool LoadShaderStartupCache()
    {
        return m_cEF.LoadShaderStartupCache();
    }

    virtual void UnloadShaderStartupCache()
    {
        m_cEF.UnloadShaderStartupCache();
    }

    virtual bool LoadShaderLevelCache() { return false; }
    virtual void UnloadShaderLevelCache() {}

    void SyncMainWithRender();

    virtual void RegisterSyncWithMainListener(ISyncMainWithRenderListener* pListener);
    virtual void RemoveSyncWithMainListener(const ISyncMainWithRenderListener* pListener);

    void InitializeVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer) final
    {
        m_pRT->RC_InitializeVideoRenderer(pVideoRenderer);
    }
    void RT_InitializeVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer);

    void CleanupVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer) final
    {
        m_pRT->RC_CleanupVideoRenderer(pVideoRenderer);
    }
    void RT_CleanupVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer);

    void DrawVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer, const AZ::VideoRenderer::DrawArguments& drawArguments) final
    {
        m_pRT->RC_DrawVideoRenderer(pVideoRenderer, drawArguments);
    }
    virtual void RT_DrawVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer, const AZ::VideoRenderer::DrawArguments& drawArguments) = 0;

protected:
    int m_width, m_height, m_cbpp, m_zbpp, m_sbpp;
    int m_nativeWidth, m_nativeHeight;
    int m_backbufferWidth, m_backbufferHeight;
    int m_numSSAASamples;
    int m_wireframe_mode, m_wireframe_mode_prev;
    uint32 m_nGPUs;  // Use GetActiveGPUCount() to read
    float m_drawNearFov;
    float m_pixelAspectRatio;
    float m_shadowJittering;
    StaticArray<int, MAX_GSM_LODS_NUM> m_CachedShadowsResolution;
    CTextMessages m_TextMessages[RT_COMMAND_BUF_COUNT];     // [ThreadID], temporary stores 2d/3d text messages to render them at the end of the frame

    CSkinningDataPool m_SkinningDataPool[3]; // Tripple Buffered for motion blur
    LegacyInternal::JobExecutorPool m_jobExecutorPool;

    uint32 m_nShadowGenId[RT_COMMAND_BUF_COUNT];

    int m_cloudShadowTexId;
    Vec3 m_cloudShadowSpeed;
    float m_cloudShadowTiling;
    bool m_cloudShadowInvert;
    float m_cloudShadowBrightness;

public:
    // these ids can be used for tripple (or more) buffered structures
    // they are incremented in RenderWorld on the mainthread
    // use m_nPoolIndex from the mainthread (or jobs which are synced before Renderworld)
    // and m_nPoolIndexRT from the renderthread
    // right now the skinning data pool and particle are using this id
    uint32 m_nPoolIndex;
    uint32 m_nPoolIndexRT;


    bool m_bVendorLibInitialized;

    _inline uint32 GetActiveGPUCount() const
    {
        return CV_r_multigpu > 0 ? m_nGPUs : 1;
    }

    CCamera m_prevCamera;                               // camera from previous frame

    uint32 m_nFrameLoad;
    uint32 m_nFrameReset;
    uint32 m_nFrameSwapID;                          // without recursive calls, access through GetFrameID(false)

    ColorF m_cClearColor;
    bool m_clearBackground;
    int  m_NumResourceSlots;
    int  m_NumSamplerSlots;

    ////////////////////////////////////////////////////////
    // downscaling viewport information.

    // Set from CrySystem via IRenderer interface
    Vec2 m_ReqViewportScale;

    // Updated in RT_EndFrame. Fixed across the whole frame.
    Vec2 m_CurViewportScale;
    Vec2 m_PrevViewportScale;

    // a RECT that represents the full screen size, accounting for above scaling
    RECT m_FullResRect;
    // same, but half resolution in each direction
    RECT m_HalfResRect;

    virtual void SetCurDownscaleFactor(Vec2 sf) = 0;

    ////////////////////////////////////////////////////////

    class CPostEffectsMgr* m_pPostProcessMgr;
    class CWater* m_pWaterSimMgr;

    // Used for pausing timer related stuff (eg: for texture animations, and shader 'time' parameter)
    bool m_bPauseTimer;
    float m_fPrevTime;
    uint8 m_nUseZpass : 2;
    bool m_bCollectDrawCallsInfo;
    bool m_bCollectDrawCallsInfoPerNode;

    // HDR rendering stuff
    int m_dwHDRCropWidth;
    int m_dwHDRCropHeight;

    S3DEngineCommon m_p3DEngineCommon;

    typedef std::map< uint64, PodArray<uint16>* > ShadowFrustumListsCache;
    ShadowFrustumListsCache m_FrustumsCache;

    ShadowFrustumMGPUCache m_ShadowFrustumMGPUCache;

    //Debug Gun
    IRenderNode* m_pDebugRenderNode;

    //=====================================================================
    // Shaders interface
    CShaderMan m_cEF;
    _smart_ptr<IMaterial> m_pDefaultMaterial;
    _smart_ptr<IMaterial> m_pTerrainDefaultMaterial;

    int m_TexGenID;

    IFFont* m_pDefaultFont;

    //=================================================================
    // Light volumes data

    struct SLightVolume* m_pLightVols;
    uint32 m_nNumVols;

    void RT_UpdateLightVolumes(int32 nFlags, int32 nRecurseLevel);
    void RT_SetSkinningPoolId(uint32);

    _inline void RT_SetLightVolumeShaderFlags(uint8 nNumLights)
    {
        const uint64 lightVolume = g_HWSR_MaskBit[HWSR_LIGHTVOLUME0];
        m_RP.m_FlagsShader_RT &= ~lightVolume;
        IF (nNumLights > 0, 1)
        {
            m_RP.m_FlagsShader_RT |= lightVolume;
        }
    }

    //=================================================================
    // High res screen shot

    int m_screenShotType;
    virtual void StartScreenShot(int e_ScreenShotType) { m_screenShotType = e_ScreenShotType; }
    virtual void EndScreenShot([[maybe_unused]] int e_ScreenShotType) { m_screenShotType = 0; }

    //=================================================================

    virtual void SetClearColor(const Vec3& vColor) { m_cClearColor.r = vColor[0]; m_cClearColor.g = vColor[1]; m_cClearColor.b = vColor[2]; }
    
    virtual void SetClearBackground(bool clearBackground) { m_clearBackground = clearBackground; }

    static void ChangeGeomInstancingThreshold(ICVar* pVar = 0);

    static int m_iGeomInstancingThreshold;      // internal value, auto mapped depending on GPU hardware, 0 means not set yet

    //////////////////////////////////////////////////////////////////////
    // console variables
    //////////////////////////////////////////////////////////////////////

    //------------------int cvars-------------------------------

    static ICVar* CV_r_ShowDynTexturesFilter;
    static ICVar*   CV_r_ShaderCompilerServer;
    static int CV_r_AssetProcessorShaderCompiler; // If true, will forward requests for shader compilation to the Asset Processor instead of the server directly.
    static ICVar* CV_r_ShaderCompilerFolderSuffix;
    static ICVar* CV_r_ShaderEmailTags;
    static ICVar* CV_r_ShaderEmailCCs;
    static ICVar* CV_r_excludeshader;
    static ICVar* CV_r_excludemesh;
    static ICVar* CV_r_ShowTexture;
    static ICVar* CV_r_TexturesStreamingDebugfilter;

    //declare cvars differing on platforms
    static int CV_r_vsync;
    static int CV_r_OldBackendSkip;
#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
    static float CV_r_overrideRefreshRate;
    static int CV_r_overrideScanlineOrder;
    static int CV_r_overrideDXGIOutput;
    static int CV_r_overrideDXGIOutputFS;
#endif
#if defined(WIN32) || defined(WIN64)
    static int CV_r_FullscreenPreemption;
#endif

    static int CV_r_DebugLightLayers;

    static int CV_r_ApplyToonShading;
    static int CV_r_GraphicsPipeline;

    static int CV_r_DeferredShadingTiled;
    static int CV_r_DeferredShadingTiledHairQuality;
    static int CV_r_DeferredShadingTiledDebugDirect;
    static int CV_r_DeferredShadingTiledDebugIndirect;
    static int CV_r_DeferredShadingTiledDebugAccumulation;
    static int CV_r_DeferredShadingTiledDebugAlbedo;
    static int CV_r_DeferredShadingSSS;
    static int CV_r_DeferredShadingFilterGBuffer;

    DeclareStaticConstIntCVar(CV_r_MotionVectors, 1);
    DeclareStaticConstIntCVar(CV_r_MotionVectorsTransparency, 1);
    DeclareStaticConstIntCVar(CV_r_MotionVectorsDebug, 0);
    static float CV_r_MotionVectorsTransparencyAlphaThreshold;
    static int CV_r_MotionBlur;
    static int CV_r_RenderMotionBlurAfterHDR;
    static int CV_r_MotionBlurScreenShot;
    static int CV_r_MotionBlurQuality;
    static int CV_r_MotionBlurGBufferVelocity;
    static float CV_r_MotionBlurThreshold;
    static int CV_r_flush;
    static int CV_r_minimizeLatency;
    static int CV_r_texatlassize;
    static int CV_r_DeferredShadingSortLights;
    static int CV_r_DeferredShadingAmbientSClear;
    static int CV_r_batchtype;
#if defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(APPLE) || defined(USE_SILHOUETTEPOM_CVAR)
    //HACK: make sure we can only use it for dx11
    static int CV_r_SilhouettePOM;
#else
    enum
    {
        CV_r_SilhouettePOM = 0
    };
#endif
#ifdef WATER_TESSELLATION_RENDERER
    static int CV_r_WaterTessellationHW;
#else
    enum
    {
        CV_r_WaterTessellationHW = 0
    };
#endif
    static int CV_r_tessellationdebug;
    static float CV_r_tessellationtrianglesize;
    static float CV_r_displacementfactor;
    static int CV_r_geominstancingthreshold;
    static int CV_r_ShadowsDepthBoundNV;
    static int CV_r_ShadowsPCFiltering;
    static int CV_r_rc_autoinvoke;
    static int CV_r_Refraction;
    static int CV_r_PostProcessReset;
    static int CV_r_colorRangeCompression;
    static int CV_r_colorgrading_selectivecolor;
    static int CV_r_colorgrading_charts;
    static int CV_r_ColorgradingChartsCache;
    static int CV_r_ShaderCompilerPort;
    static int CV_r_ShowDynTexturesMaxCount;
    static int CV_r_ShaderCompilerDontCache;
    static int CV_r_dyntexmaxsize;
    static int CV_r_dyntexatlascloudsmaxsize;
    static int CV_r_texminanisotropy;
    static int CV_r_texmaxanisotropy;
    static int CV_r_texturesskiplowermips;
    static int CV_r_rendertargetpoolsize;
    static int CV_r_texturesstreamingsync;
    static int CV_r_ConditionalRendering;
    static int CV_r_watercaustics; //@NOTE: CV_r_watercaustics will be removed when the infinite ocean component feature toggle is removed.
    static int CV_r_watervolumecaustics;
    static int CV_r_watervolumecausticsdensity;
    static int CV_r_watervolumecausticsresolution;
#if !defined(CONSOLE)
    static int CV_r_shadersorbis;
    static int CV_r_shadersdx10;
    static int CV_r_shadersdx11;
    static int CV_r_shadersGL4;
    static int CV_r_shadersGLES3;
    static int CV_r_shadersMETAL;
    
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_H_SECTION_5
    #include AZ_RESTRICTED_FILE(Renderer_h)
#endif

    static int CV_r_shadersPlatform;
#endif
    //  static int CV_r_envcmwrite;
    static int CV_r_shaderspreactivate;
    DeclareStaticConstIntCVar(CV_r_shadersremotecompiler, 0);
    static int CV_r_shadersasynccompiling;
    static int CV_r_shadersasyncactivation;
    static int CV_r_shadersasyncmaxthreads;
    static int CV_r_shaderscachedeterministic;
    static int CV_r_shaderssubmitrequestline;
    static int CV_r_shadersuseinstancelookuptable;
    static int CV_r_shaderslogcachemisses;
    static int CV_r_shadersImport;
    static int CV_r_shadersExport;
    static int CV_r_shadersCacheUnavailableShaders;
    DeclareStaticConstIntCVar(CV_r_ShadersUseLLVMDirectXCompiler, 0);
    static int CV_r_meshpoolsize;
    static int CV_r_meshinstancepoolsize;
    static int CV_r_multigpu;
    static int CV_r_msaa;
    static int CV_r_msaa_samples;
    static int CV_r_msaa_quality;
    static int CV_r_msaa_debug;
    static int CV_r_impostersupdateperframe;
    static int CV_r_beams;
    static int CV_r_nodrawnear;
    static int CV_r_DrawNearShadows;
    static int CV_r_scissor;
    static int CV_r_usezpass;
    static int CV_r_ShowVideoMemoryStats;
    static int CV_r_TexturesStreamingDebugMinSize;
    static int CV_r_TexturesStreamingDebugMinMip;
    static int CV_r_enableAltTab;
    static int CV_r_StereoDevice;
    static int CV_r_StereoMode;
    static int CV_r_StereoOutput;
    static int CV_r_StereoFlipEyes;
    static int CV_r_GetScreenShot;
    enum class ScreenshotType : int // define as int to match the CVar
    {
        None               = 0,
        HdrAndNormal       = 1,
        Normal             = 2,
        // Now for internal ScreenshotRequestBus use only.
        NormalWithFilepath = 3,
        NormalToBuffer     = 4
    };

    static int CV_r_BreakOnError;

    static int CV_r_TexturesStreamPoolSize; //plz do not access directly, always by GetTexturesStreamPoolSize()
    static int CV_r_TexturesStreamPoolSecondarySize;
    static inline int GetTexturesStreamPoolSize()
    {
        int poolSize = CV_r_TexturesStreamPoolSize + CV_r_TexturesStreamPoolSecondarySize;
        return gEnv->IsEditor()
               ? max(poolSize, 512)
               : poolSize;
    }

    static int CV_r_ReprojectOnlyStaticObjects;
    static int CV_r_D3D12SubmissionThread;
    static int CV_r_ReverseDepth;

    // DX12 related cvars
    static int CV_r_EnableDebugLayer;
    static int CV_r_NoDraw;

    //declare in release mode constant cvars
    DeclareStaticConstIntCVar(CV_r_stats, 0);
    DeclareStaticConstIntCVar(CV_r_statsMinDrawcalls, 0);
    DeclareStaticConstIntCVar(CV_r_profiler, 0);
    static float CV_r_profilerTargetFPS;
    DeclareStaticConstIntCVar(CV_r_ShadowPoolMaxFrames, 30);
    static int CV_r_log;
    static int CV_r_VRAMDebug;
    DeclareStaticConstIntCVar(CV_r_logTexStreaming, 0);
    DeclareStaticConstIntCVar(CV_r_logShaders, 0);
    static int CV_r_logVBuffers;
    DeclareStaticConstIntCVar(CV_r_logVidMem, 0);
    DeclareStaticConstIntCVar(CV_r_predicatedtiling, 0);
    DeclareStaticConstIntCVar(CV_r_useESRAM, 1);
    DeclareStaticConstIntCVar(CV_r_multithreaded, MULTITHREADED_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_multithreadedDrawing, 0);
    DeclareStaticConstIntCVar(CV_r_multithreadedDrawingActiveThreshold, 0);
    DeclareStaticConstIntCVar(CV_r_deferredshadingLightVolumes, 1);
    DeclareStaticConstIntCVar(CV_r_deferredDecals, 1);
    DeclareStaticConstIntCVar(CV_r_deferredDecalsDebug, 0);
    DeclareStaticConstIntCVar(CV_r_deferredDecalsOnDynamicObjects, 0);
    DeclareStaticConstIntCVar(CV_r_DeferredShadingLBuffersFmt, 1);
    DeclareStaticConstIntCVar(CV_r_DeferredShadingScissor, 1);
    DeclareStaticConstIntCVar(CV_r_DeferredShadingDebug, 0);
    DeclareStaticConstIntCVar(CV_r_DeferredShadingDebugGBuffer, 0);
    DeclareStaticConstIntCVar(CV_r_DeferredShadingEnvProbes, 1);
    DeclareStaticConstIntCVar(CV_r_DeferredShadingAmbient, 1);
    DeclareStaticConstIntCVar(CV_r_DeferredShadingAmbientLights, 1);
    DeclareStaticConstIntCVar(CV_r_DeferredShadingLights, 1);
    DeclareStaticConstIntCVar(CV_r_DeferredShadingAreaLights, 1);
    DeclareStaticConstIntCVar(CV_r_DeferredShadingStencilPrepass, 1);
    static int CV_r_HDRDebug;
    static int CV_r_HDRBloom;
    static int CV_r_HDRBloomQuality;
    static int CV_r_ToneMapTechnique;
    static int CV_r_ColorSpace;
    static int CV_r_ToneMapExposureType;
    static float CV_r_ToneMapManualExposureValue;    
    DeclareStaticConstIntCVar(CV_r_HDRVignetting, 1);
    DeclareStaticConstIntCVar(CV_r_HDRTexFormat, 0);
    static int CV_r_HDREyeAdaptationMode;
    DeclareStaticConstIntCVar(CV_r_geominstancing, GEOM_INSTANCING_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_geominstancingdebug, 0);
    DeclareStaticConstIntCVar(CV_r_materialsbatching, 1);
    DeclareStaticConstIntCVar(CV_r_DebugLightVolumes, 0);
    DeclareStaticConstIntCVar(CV_r_UseShadowsPool, SHADOWS_POOL_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_shadowtexformat, 0);
    DeclareStaticConstIntCVar(CV_r_ShadowsMaskResolution, 0);
    DeclareStaticConstIntCVar(CV_r_ShadowsMaskDownScale, 0);
    static int CV_r_CBufferUseNativeDepth;
    DeclareStaticConstIntCVar(CV_r_ShadowsStencilPrePass, 1);
    DeclareStaticConstIntCVar(CV_r_ShadowsGridAligned, 1);
    DeclareStaticConstIntCVar(CV_r_ShadowPass, 1);
    DeclareStaticConstIntCVar(CV_r_ShadowGen, 1);
    DeclareStaticConstIntCVar(CV_r_ShadowsUseClipVolume, SHADOWS_CLIP_VOL_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_ShadowGenMode, 1);
    static int CV_r_ShadowsCache;
    static int CV_r_ShadowsCacheFormat;
    static int CV_r_ShadowsNearestMapResolution;
    static int CV_r_ShadowsScreenSpace;
    DeclareStaticConstIntCVar(CV_r_TerrainAO, 7);
    DeclareStaticConstIntCVar(CV_r_TerrainAO_FadeDist, 8);
    DeclareStaticConstIntCVar(CV_r_debuglights, 0);
    DeclareStaticConstIntCVar(CV_r_DeferredShadingDepthBoundsTest, DEF_SHAD_DBT_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_deferredshadingDBTstencil, DEF_SHAD_DBT_STENCIL_DEFAULT_VAL);
    static int CV_r_sunshafts;
    DeclareStaticConstIntCVar(CV_r_MergeShadowDrawcalls, 1);
    static int CV_r_PostProcess_CB;
    static int CV_r_PostProcess;
    DeclareStaticConstIntCVar(CV_r_PostProcessFilters, 1);
    DeclareStaticConstIntCVar(CV_r_PostProcessGameFx, 1);
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    static int CV_r_FinalOutputsRGB;
    static int CV_r_FinalOutputAlpha;
    static int CV_r_RTT;
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    static int CV_r_colorgrading;
    DeclareStaticConstIntCVar(CV_r_colorgrading_levels, 1);
    DeclareStaticConstIntCVar(CV_r_colorgrading_filters, 1);
    DeclareStaticConstIntCVar(CV_r_cloudsupdatealways, 0);
    DeclareStaticConstIntCVar(CV_r_cloudsdebug, 0);
    DeclareStaticConstIntCVar(CV_r_showdyntextures, 0);
    DeclareStaticConstIntCVar(CV_r_shownormals, 0);
    DeclareStaticConstIntCVar(CV_r_showlines, 0);
    DeclareStaticConstIntCVar(CV_r_showtangents, 0);
    DeclareStaticConstIntCVar(CV_r_showtimegraph, 0);
    DeclareStaticConstIntCVar(CV_r_DebugFontRendering, 0);
    DeclareStaticConstIntCVar(CV_profileStreaming, 0);
    DeclareStaticConstIntCVar(CV_r_graphstyle, 0);
    DeclareStaticConstIntCVar(CV_r_showbufferusage, 0);
    DeclareStaticConstIntCVar(CV_r_profileshaders, 0);
    DeclareStaticConstIntCVar(CV_r_ProfileShadersSmooth, 4);
    DeclareStaticConstIntCVar(CV_r_ProfileShadersGroupByName, 1);
    DeclareStaticConstIntCVar(CV_r_texpostponeloading, 1);
    DeclareStaticConstIntCVar(CV_r_texpreallocateatlases, TEXPREALLOCATLAS_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_texlog, 0);
    DeclareStaticConstIntCVar(CV_r_texnoload, 0);
    DeclareStaticConstIntCVar(CV_r_texturecompiling, 1);
    DeclareStaticConstIntCVar(CV_r_texBlockOnLoad, 0);
    DeclareStaticConstIntCVar(CV_r_texturecompilingIndicator, 0);
    DeclareStaticConstIntCVar(CV_r_TexturesDebugBandwidth, 0);
    DeclareStaticConstIntCVar(CV_r_texturesstreaming, TEXSTREAMING_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_TexturesStreamingDebug, 0);
    DeclareStaticConstIntCVar(CV_r_texturesstreamingnoupload, 0);
    DeclareStaticConstIntCVar(CV_r_texturesstreamingonlyvideo, 0);
    DeclareStaticConstIntCVar(CV_r_texturesstreamingResidencyEnabled, 1);
    DeclareStaticConstIntCVar(CV_r_texturesstreamingmipfading, 1);
    DeclareStaticConstIntCVar(CV_r_texturesstreamingUpdateType, TEXSTREAMING_UPDATETYPE_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_texturesstreamingPrecacheRounds, 1);
    DeclareStaticConstIntCVar(CV_r_texturesstreamingSuppress, 0);
    static int CV_r_texturesstreamingSkipMips;
    static int CV_r_texturesstreamingMinUsableMips;
    static int CV_r_texturesstreamingJobUpdate;
#if defined(TEXSTRM_DEFERRED_UPLOAD)
    static int CV_r_texturesstreamingDeferred;
#endif
    DeclareStaticConstIntCVar(CV_r_texturesstreamingPostponeMips, 0);
    DeclareStaticConstIntCVar(CV_r_texturesstreamingPostponeThresholdKB, 1024);
    DeclareStaticConstIntCVar(CV_r_texturesstreamingPostponeThresholdMip, 1);
    DeclareStaticConstIntCVar(CV_r_texturesstreamingMinReadSizeKB, 64);
#if defined(SUPPORTS_INPLACE_TEXTURE_STREAMING)
    static int CV_r_texturesstreamingInPlace;
#endif
    DeclareStaticConstIntCVar(CV_r_lightssinglepass, 1);
    static int CV_r_envcmresolution;
    static int CV_r_envtexresolution;
    DeclareStaticConstIntCVar(CV_r_waterreflections, 1);
    DeclareStaticConstIntCVar(CV_r_waterreflections_quality, WATERREFLQUAL_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_water_godrays, 1);
    DeclareStaticConstIntCVar(CV_r_reflections, 1);
    DeclareStaticConstIntCVar(CV_r_reflections_quality, 3);
    DeclareStaticConstIntCVar(CV_r_dof, DOF_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_texNoAnisoAlphaTest, TEXNOANISOALPHATEST_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_reloadshaders, 0);
    DeclareStaticConstIntCVar(CV_r_detailtextures, 1);
    DeclareStaticConstIntCVar(CV_r_texbindmode, 0);
    DeclareStaticConstIntCVar(CV_r_nodrawshaders, 0);
    DeclareStaticConstIntCVar(CV_r_shadersdebug, 0);
    DeclareStaticConstIntCVar(CV_r_shadersignoreincludeschanging, 0);
    DeclareStaticConstIntCVar(CV_r_shaderslazyunload, 0);
    static int CV_r_shadersAllowCompilation;
    DeclareStaticConstIntCVar(CV_r_shaderscompileautoactivate, 0);
    DeclareStaticConstIntCVar(CV_r_shadersediting, 0);
    DeclareStaticConstIntCVar(CV_r_shadersprecachealllights, 1);
    DeclareStaticConstIntCVar(CV_r_ReflectTextureSlots, 1);
    DeclareStaticConstIntCVar(CV_r_debugrendermode, 0);
    DeclareStaticConstIntCVar(CV_r_debugrefraction, 0);
    DeclareStaticConstIntCVar(CV_r_meshprecache, 1);
    DeclareStaticConstIntCVar(CV_r_impostersdraw, 1);
    static int CV_r_flares;
    DeclareStaticConstIntCVar(CV_r_flareHqShafts, FLARES_HQSHAFTS_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_ZPassDepthSorting, ZPASS_DEPTH_SORT_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_TransparentPasses, 1);
    DeclareStaticConstIntCVar(CV_r_TranspDepthFixup, 1);
    DeclareStaticConstIntCVar(CV_r_SoftAlphaTest, 1);
    DeclareStaticConstIntCVar(CV_r_usehwskinning, 1);
    DeclareStaticConstIntCVar(CV_r_usemateriallayers, 2);
    DeclareStaticConstIntCVar(CV_r_ParticlesSoftIsec, 1);
    DeclareStaticConstIntCVar(CV_r_ParticlesRefraction, 1);
    static int CV_r_ParticlesTessellation;
    static int CV_r_ParticlesTessellationTriSize;
    static float CV_r_ParticlesAmountGI;
    static int CV_r_ParticlesGpuMaxEmitCount;
    static int CV_r_ParticlesHalfRes;
    DeclareStaticConstIntCVar(CV_r_ParticlesHalfResAmount, 0);
    DeclareStaticConstIntCVar(CV_r_ParticlesHalfResBlendMode, 0);
    DeclareStaticConstIntCVar(CV_r_ParticlesInstanceVertices, 1);
    DeclareStaticConstIntCVar(CV_r_AntialiasingModeEditor, 1);
    DeclareStaticConstIntCVar(CV_r_AntialiasingModeDebug, 0);
    DeclareStaticConstIntCVar(CV_r_rain, 2);
    DeclareStaticConstIntCVar(CV_r_rain_ignore_nearest, 1);
    DeclareStaticConstIntCVar(CV_r_snow, 2);
    DeclareStaticConstIntCVar(CV_r_snow_halfres, 0);
    DeclareStaticConstIntCVar(CV_r_snow_displacement, 0);
    DeclareStaticConstIntCVar(CV_r_snowFlakeClusters, 100);
    DeclareStaticConstIntCVar(CV_r_customvisions, CUSTOMVISIONS_DEFAULT_VAL);
    DeclareStaticConstIntCVar(CV_r_nohwgamma, 2);
    DeclareStaticConstIntCVar(CV_r_wireframe, 0);
    DeclareStaticConstIntCVar(CV_r_printmemoryleaks, 0);
    DeclareStaticConstIntCVar(CV_r_releaseallresourcesonexit, 1);
    DeclareStaticConstIntCVar(CV_r_character_nodeform, 0);
    DeclareStaticConstIntCVar(CV_r_ZPassOnly, 0);
    DeclareStaticConstIntCVar(CV_r_measureoverdraw, 0);
    DeclareStaticConstIntCVar(CV_r_ShowLightBounds, 0);
    DeclareStaticConstIntCVar(CV_r_MergeRenderChunks, 1);
    DeclareStaticConstIntCVar(CV_r_TextureCompressor, 1);
    DeclareStaticConstIntCVar(CV_r_TexturesStreamingDebugDumpIntoLog, 0);
    DeclareStaticConstIntCVar(CV_e_DebugTexelDensity, 0);
    DeclareStaticConstIntCVar(CV_r_RainDropsEffect, 1);
    DeclareStaticConstIntCVar(CV_r_RefractionPartialResolves, 2);
    DeclareStaticConstIntCVar(CV_r_RefractionPartialResolvesDebug, 0);
    DeclareStaticConstIntCVar(CV_r_Batching, 1);
    DeclareStaticConstIntCVar(CV_r_Unlit, 0);
    DeclareStaticConstIntCVar(CV_r_HideSunInCubemaps, 1);
    DeclareStaticConstIntCVar(CV_r_ParticlesDebug, 0);

    // Confetti David Srour: Upscaling Quality (Metal only at the moment)
    DeclareStaticConstIntCVar(CV_r_UpscalingQuality, 0);
    //Clears GMEM G-Buffer
    DeclareStaticConstIntCVar(CV_r_ClearGMEMGBuffer, 0);

    // 0 = disable, 1 = enables fast math for metal shaders
    DeclareStaticConstIntCVar(CV_r_MetalShadersFastMath, 1);
    // Confetti Vera
    static int CV_r_CubeDepthMapResolution;

    // Specular Antialiasing
    static int CV_r_SpecularAntialiasing;

    //--------------float cvars----------------------


    static float CV_r_ZPrepassMaxDist;
    static float CV_r_FlaresChromaShift;
    static int CV_r_FlaresIrisShaftMaxPolyNum;
    static float CV_r_FlaresTessellationRatio;

    static float CV_r_msaa_threshold_normal;
    static float CV_r_msaa_threshold_depth;

    static float CV_r_drawnearfov;
    static float CV_r_measureoverdrawscale;
    static float CV_r_DeferredShadingLightLodRatio;
    static float CV_r_DeferredShadingLightStencilRatio;
    static float CV_r_rainDistMultiplier;
    static float CV_r_rainOccluderSizeTreshold;

    static float CV_r_HDREyeAdaptationSpeed;
    static float CV_r_HDRGrainAmount;

    static int CV_r_HDRDolbyDynamicMetadata;
    static int CV_r_HDRDolbyScurve;
    static float CV_r_HDRDolbyScurveSourceMin;
    static float CV_r_HDRDolbyScurveSourceMid;
    static float CV_r_HDRDolbyScurveSourceMax;
    static float CV_r_HDRDolbyScurveSlope;
    static float CV_r_HDRDolbyScurveScale;
    static float CV_r_HDRDolbyScurveRGBPQTargetMin;
    static float CV_r_HDRDolbyScurveRGBPQTargetMax;
    static float CV_r_HDRDolbyScurveRGBPQTargetMid;
    static float CV_r_HDRDolbyScurveVisionTargetMin;
    static float CV_r_HDRDolbyScurveVisionTargetMax;
    static float CV_r_HDRDolbyScurveVisionTargetMid;

    static float CV_r_Sharpening;
    static float CV_r_ChromaticAberration;
    static float CV_r_dofMinZ;
    static float CV_r_dofMinZScale;
    static float CV_r_dofMinZBlendMult;
    static float CV_r_ShadowsBias;
    static float CV_r_ShadowsAdaptionRangeClamp;
    static float CV_r_ShadowsAdaptionSize;
    static float CV_r_ShadowsAdaptionMin;
    static float CV_r_ShadowsParticleKernelSize;
    static float CV_r_ShadowsParticleJitterAmount;
    static float CV_r_ShadowsParticleAnimJitterAmount;
    static float CV_r_ShadowsParticleNormalEffect;
private:
    static float CV_r_shadow_jittering; // dont use this directly for rendering. use m_shadowJittering or GetShadowJittering() instead;
public:
    static int   CV_r_ShadowPoolMaxTimeslicedUpdatesPerFrame;
    static int   CV_r_ShadowCastingLightsMaxCount;
    static int   CV_r_HeightMapAO;
    static float CV_r_HeightMapAOAmount;
    static float CV_r_HeightMapAOResolution;
    static float CV_r_HeightMapAORange;
    static float CV_r_RenderMeshHashGridUnitSize;
    static float CV_r_normalslength;
    static float CV_r_TexelsPerMeter;
    static float CV_r_TexturesStreamingMaxRequestedMB;
    static int CV_r_TexturesStreamingMaxRequestedJobs;
    static float CV_r_TexturesStreamingMipBias;
    static int CV_r_TexturesStreamingMipClampDVD;
    static int CV_r_TexturesStreamingDisableNoStreamDuringLoad;
    static float CV_r_texturesstreamingResidencyTimeTestLimit;
    static float CV_r_texturesstreamingResidencyTime;
    static float CV_r_texturesstreamingResidencyThrottle;
    static float CV_r_envcmupdateinterval;
    static float CV_r_envtexupdateinterval;
    static int   CV_r_SlimGBuffer;
    static float CV_r_TextureLodDistanceRatio;
    static float CV_r_water_godrays_distortion;
    static float CV_r_waterupdateFactor;
    static float CV_r_waterupdateDistance;
    static float CV_r_waterreflections_min_visible_pixels_update;
    static float CV_r_waterreflections_minvis_updatefactormul;
    static float CV_r_waterreflections_minvis_updatedistancemul;
    static float CV_r_waterreflections_offset;
    static float CV_r_watercausticsdistance;
    static float CV_r_watervolumecausticssnapfactor;
    static float CV_r_watervolumecausticsmaxdistance;
    static float CV_r_detaildistance;
    static float CV_r_DrawNearZRange;
    static float CV_r_DrawNearFarPlane;
    static float CV_r_imposterratio;
    static float CV_r_rainamount;
    static float CV_r_MotionBlurShutterSpeed;
    static float CV_r_MotionBlurCameraMotionScale;
    static float CV_r_MotionBlurMaxViewDist;
    static float CV_r_gamma;
    static float CV_r_contrast;
    static float CV_r_brightness;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_H_SECTION_6
    #include AZ_RESTRICTED_FILE(Renderer_h)
#endif
    static float CV_r_ZFightingDepthScale;
    static float CV_r_ZFightingExtrude;
    static float CV_r_StereoStrength;
    static float CV_r_StereoEyeDist;
    static float CV_r_StereoScreenDist;
    static float CV_r_StereoNearGeoScale;
    static float CV_r_StereoHudScreenDist;
    static float CV_r_StereoGammaAdjustment;
    static int CV_r_ConsoleBackbufferWidth;
    static int CV_r_ConsoleBackbufferHeight;

    static int CV_r_AntialiasingMode_CB;
    static int CV_r_AntialiasingMode;
    static float CV_r_AntialiasingNonTAASharpening;
    static int CV_r_AntialiasingTAAJitterPattern;
    DeclareStaticConstIntCVar(CV_r_AntialiasingTAAUseAntiFlickerFilter, 1);
    DeclareStaticConstIntCVar(CV_r_AntialiasingTAAUseJitterMipBias, 1);
    DeclareStaticConstIntCVar(CV_r_AntialiasingTAAUseVarianceClamping, 0);
    static float CV_r_AntialiasingTAAClampingFactor;
    static float CV_r_AntialiasingTAANewFrameWeight;
    static float CV_r_AntialiasingTAASharpening;
    static float CV_r_FogDepthTest;
#if defined(VOLUMETRIC_FOG_SHADOWS)
    static int CV_r_FogShadows;
    static int CV_r_FogShadowsMode;
#endif
    static int CV_r_FogShadowsWater;

    static float CV_r_rain_maxviewdist;
    static float CV_r_rain_maxviewdist_deferred;

    static int CV_r_SSReflections;
    static int CV_r_SSReflHalfRes;
    static int CV_r_ssdo;
    static int CV_r_ssdoHalfRes;
    static int CV_r_ssdoColorBleeding;
    static float CV_r_ssdoRadius;
    static float CV_r_ssdoRadiusMin;
    static float CV_r_ssdoRadiusMax;
    static float CV_r_ssdoAmountDirect;
    static float CV_r_ssdoAmountAmbient;
    static float CV_r_ssdoAmountReflection;

    // constant used to indicate that CustomResMax should be set to the maximum allowable by device resources
    static const int s_CustomResMaxSize_USE_MAX_RESOURCES;
    static int CV_r_CustomResMaxSize;
    static int CV_r_CustomResWidth;
    static int CV_r_CustomResHeight;
    static int CV_r_CustomResPreview;
    static int CV_r_Supersampling;
    static int CV_r_SupersamplingFilter;

#if defined(ENABLE_RENDER_AUX_GEOM)
    static int CV_r_enableauxgeom;
#endif

    static int CV_r_buffer_banksize;
    static int CV_r_constantbuffer_banksize;
    static int CV_r_constantbuffer_watermark;
    static int CV_r_transient_pool_size;
    static int CV_r_buffer_sli_workaround;
    DeclareStaticConstIntCVar(CV_r_buffer_enable_lockless_updates, 1);
    DeclareStaticConstIntCVar(CV_r_enable_full_gpu_sync, 0);
    static int CV_r_buffer_pool_max_allocs;
    static int CV_r_buffer_pool_defrag_static;
    static int CV_r_buffer_pool_defrag_dynamic;
    static int CV_r_buffer_pool_defrag_max_moves;

    static int CV_r_ParticleVerticePoolSize;

    static int CV_r_GeomCacheInstanceThreshold;

    static int CV_r_VisAreaClipLightsPerPixel;
    static int CV_r_OutputShaderSourceFiles;

    static int CV_r_VolumetricFog;
    static int CV_r_VolumetricFogTexScale;
    static int CV_r_VolumetricFogTexDepth;
    static float CV_r_VolumetricFogReprojectionBlendFactor;
    static int CV_r_VolumetricFogSample;
    static int CV_r_VolumetricFogShadow;
    static int CV_r_VolumetricFogDownscaledSunShadow;
    static int CV_r_VolumetricFogDownscaledSunShadowRatio;
    static int CV_r_VolumetricFogReprojectionMode;
    static float CV_r_VolumetricFogMinimumLightBulbSize;

    static float CV_r_ResolutionScale;

    // Confetti David Srour: Global VisArea/Portals blend weight for GMEM path
    static float CV_r_GMEMVisAreasBlendWeight;

    // Confetti David Srour: 0 = disable, 1= 256bpp GMEM path, 2 = 128bpp GMEM path
    static int CV_r_EnableGMEMPath;

    // Confetti David Srour: 0 = GMEM postproc w/out CS, 1 = GMEM postproc w/ CS
    static int CV_r_EnableGMEMPostProcCS;

    // Confetti David Srour: Used to reduce draw calls during DOF's gathers
    static int CV_r_GMEM_DOF_Gather1_Quality;
    static int CV_r_GMEM_DOF_Gather2_Quality;

    static int CV_r_RainUseStencilMasking;

    // Confetti Thomas Zeng: 0 = disable, 1 = enable
    static int CV_r_EnableComputeDownSampling;

    static int CV_r_ForceFixedPointRenderTargets;

    // Confetti Vera
    static float CV_r_CubeDepthMapFarPlane;

    // Fur control parameters
    static int CV_r_Fur;
    static int CV_r_FurShellPassCount;
    static int CV_r_FurShowBending;
    static int CV_r_FurDebug;
    static int CV_r_FurDebugOneShell;
    static int CV_r_FurFinPass;
    static int CV_r_FurFinShadowPass;
    static float CV_r_FurMovementBendingBias;
    static float CV_r_FurMaxViewDist;
    
    static int CV_r_SkipNativeUpscale;
    static int CV_r_SkipRenderComposites;

    static float CV_r_minConsoleFontSize;
    static float CV_r_maxConsoleFontSize;

    // Linux CVARS
    static int CV_r_linuxSkipWindowCreation;

    // Graphics programmers: Use these in your code for local tests/debugging.
    // Delete all references in your code before you submit
    static int CV_r_GraphicsTest00;
    static int CV_r_GraphicsTest01;
    static int CV_r_GraphicsTest02;
    static int CV_r_GraphicsTest03;
    static int CV_r_GraphicsTest04;
    static int CV_r_GraphicsTest05;
    static int CV_r_GraphicsTest06;
    static int CV_r_GraphicsTest07;
    static int CV_r_GraphicsTest08;
    static int CV_r_GraphicsTest09;

    //--------------end cvars------------------------


    virtual void MakeMatrix([[maybe_unused]] const Vec3& pos, [[maybe_unused]] const Vec3& angles, [[maybe_unused]] const Vec3& scale, [[maybe_unused]] Matrix34* mat){assert(0); };


    virtual WIN_HWND GetHWND() = 0;

    void SetTextureAlphaChannelFromRGB(byte* pMemBuffer, int nTexSize);

    void EnableSwapBuffers(bool bEnable) { m_bSwapBuffers = bEnable; }
    bool m_bSwapBuffers;

    virtual void SetTexturePrecaching(bool stat);

    virtual void EnableGPUTimers2([[maybe_unused]] bool bEnabled) {};
    virtual void AllowGPUTimers2([[maybe_unused]] bool bAllow) {}

    virtual const RPProfilerStats* GetRPPStats([[maybe_unused]] ERenderPipelineProfilerStats eStat, [[maybe_unused]] bool bCalledFromMainThread = true) const { return nullptr; }
    virtual const RPProfilerStats* GetRPPStatsArray([[maybe_unused]] bool bCalledFromMainThread = true) const { return nullptr; }

    virtual int GetPolygonCountByType([[maybe_unused]] uint32 EFSList, [[maybe_unused]] EVertexCostTypes vct, [[maybe_unused]] uint32 z, [[maybe_unused]] bool bCalledFromMainThread = true) { return 0; }

    //platform specific
    virtual void    RT_InsertGpuCallback([[maybe_unused]] uint32 context, [[maybe_unused]] GpuCallbackFunc callback) {}
    virtual void EnablePipelineProfiler(bool bEnable) = 0;

    virtual float GetGPUFrameTime();
    virtual void GetRenderTimes(SRenderTimes& outTimes);

    virtual void LogShaderImportMiss([[maybe_unused]] const CShader* pShader) {}

#if !defined(_RELEASE)
    //Get debug draw call stats stat
    virtual RNDrawcallsMapMesh& GetDrawCallsInfoPerMesh(bool mainThread = true)
    {
        if (mainThread)
        {
            return m_RP.m_pRNDrawCallsInfoPerMesh[m_RP.m_nFillThreadID];
        }
        else
        {
            return m_RP.m_pRNDrawCallsInfoPerMesh[m_RP.m_nProcessThreadID];
        }
    }
    
    // Added functionality for retrieving previous frames stats to use this frame
    virtual RNDrawcallsMapMesh& GetDrawCallsInfoPerMeshPreviousFrame(bool mainThread = true)
    {
        if (mainThread)
        {
            return m_RP.m_pRNDrawCallsInfoPerMeshPreviousFrame[m_RP.m_nFillThreadID];
        }
        else
        {
            return m_RP.m_pRNDrawCallsInfoPerMeshPreviousFrame[m_RP.m_nProcessThreadID];
        }
    }
    virtual RNDrawcallsMapNode& GetDrawCallsInfoPerNodePreviousFrame(bool mainThread = true)
    {
        if (mainThread)
        {
            return m_RP.m_pRNDrawCallsInfoPerNodePreviousFrame[m_RP.m_nFillThreadID];
        }
        else
        {
            return m_RP.m_pRNDrawCallsInfoPerNodePreviousFrame[m_RP.m_nProcessThreadID];
        }
    }

    virtual int GetDrawCallsPerNode(IRenderNode* pRenderNode);

    //Routine to perform an emergency flush of a particular render node from the stats, as not all render node holders are delay-deleted
    virtual void ForceRemoveNodeFromDrawCallsMap(IRenderNode* pNode)
    {
        for (int i = 0; i < RT_COMMAND_BUF_COUNT; i++)
        {
            IRenderer::RNDrawcallsMapNodeItor pItor = m_RP.m_pRNDrawCallsInfoPerNode[ i ].find(pNode);
            if (pItor != m_RP.m_pRNDrawCallsInfoPerNode[ i ].end())
            {
                m_RP.m_pRNDrawCallsInfoPerNode[ i ].erase(pItor);
            }
        }
    }

    void ClearDrawCallsInfo()
    {
        for (int i = 0; i < RT_COMMAND_BUF_COUNT; i++)
        {
            m_RP.m_pRNDrawCallsInfoPerMesh[i].swap(m_RP.m_pRNDrawCallsInfoPerMeshPreviousFrame[i]);
            m_RP.m_pRNDrawCallsInfoPerMesh[i].clear();
            m_RP.m_pRNDrawCallsInfoPerNode[i].swap(m_RP.m_pRNDrawCallsInfoPerNodePreviousFrame[i]);
            m_RP.m_pRNDrawCallsInfoPerNode[i].clear();
        }
    }
#endif

    virtual void CollectDrawCallsInfo(bool status)
    {
        m_bCollectDrawCallsInfo = status;
    }

    virtual void CollectDrawCallsInfoPerNode(bool status)
    {
        m_bCollectDrawCallsInfoPerNode = status;
    }

    virtual void EnableLevelUnloading(bool enable)
    {
        gRenDev->m_bLevelUnloading = enable;
    }

    virtual void EnableBatchMode(bool enable)
    {
        RENDER_LOCK_CS(SRenderThread::s_rcLock);
        if (enable)
        {
            gRenDev->m_bEndLevelLoading = false;
            gRenDev->m_bStartLevelLoading = true;
        }
        else
        {
            gRenDev->m_bEndLevelLoading = true;
            gRenDev->m_bStartLevelLoading = false;
        }
    }

    // When a level load fails, the system event for ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_END will not be called, so we need to do some cleanup here.
    // This is to ensure the renderer no longer thinks we are rendering a loading screen after the level load failed.
    virtual void OnLevelLoadFailed()
    {
#if AZ_LOADSCREENCOMPONENT_ENABLED
        EBUS_EVENT(LoadScreenBus, Stop);
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED

        gRenDev->m_bEndLevelLoading = true;
        gRenDev->m_bStartLevelLoading = false;
    }

    virtual bool IsStereoModeChangePending()
    {
        return false;
    }

    int m_nFlushAllPendingTextureStreamingJobs;
    float m_fTexturesStreamingGlobalMipFactor;

protected:

    AZ::LegacyJobExecutor m_generateRendItemJobExecutor;
    AZ::LegacyJobExecutor m_generateRendItemPreProcessJobExecutor;
    AZ::LegacyJobExecutor m_generateShadowRendItemJobExecutor;
    AZ::LegacyJobExecutor m_finalizeRendItemsJobExecutor[RT_COMMAND_BUF_COUNT];
    AZ::LegacyJobExecutor m_finalizeShadowRendItemsJobExecutor[RT_COMMAND_BUF_COUNT];

private:
    std::vector<ISyncMainWithRenderListener*> m_syncMainWithRenderListeners;
    RendererAssetListener m_assetListener;

    unsigned long m_nvidiaDriverVersion = 0;
    
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED && !defined(NULL_RENDERER)
    AZStd::unique_ptr<AzRTT::RenderContextManager> m_contextManager;
#endif // AZ_RENDER_TO_TEXTURE_GEM_ENABLED && !defined(NULL_RENDERER)
};


#include "CommonRender.h"

#define SKY_BOX_SIZE 32.f
