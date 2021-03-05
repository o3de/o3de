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

#include <CryThreadSafeRendererContainer.h>
#include <CryThreadSafeWorkerContainer.h>
#include "Shadow_Renderer.h"
#include "../Common/PerInstanceConstantBufferPool.h"
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
//====================================================================

#define MAX_HWINST_PARAMS 32768

#define MAX_REND_OBJECTS 16384
#define TEMP_REND_OBJECTS_POOL 2048
#define MAX_REND_GEOMS_IN_BATCH 16


#define MAX_REND_SHADERS 4096
#define MAX_REND_SHADER_RES 16384
#define MAX_REND_LIGHTS  32
#define MAX_DEFERRED_LIGHTS 256
#define SG_SORT_GROUP 0
#define MAX_SHADOWMAP_LOD 20
#define MAX_SHADOWMAP_FRUSTUMS 1024
#define MAX_SORT_GROUPS  64
#define MAX_INSTANCES_THRESHOLD_HW  8
#define MAX_LIST_ORDER   2                                  // 0 = before water, 1 = after water
#define MAX_PREDICTION_ZONES MAX_STREAM_PREDICTION_ZONES

#define CULLER_MAX_CAMS 4

#define HW_INSTANCING_ENABLED

class CRenderView;

struct SViewport
{
    int nX, nY, nWidth, nHeight;
    float fMinZ, fMaxZ;
    SViewport()
        : nX(0)
        , nY(0)
        , nWidth(0)
        , nHeight(0)
        , fMinZ(0.0f)
        , fMaxZ(0.0f)
    {}

    SViewport(int nNewX, int nNewY, int nNewWidth, int nNewHeight)
        : nX(nNewX)
        , nY(nNewY)
        , nWidth(nNewWidth)
        , nHeight(nNewHeight)
        , fMinZ(0.0f)
        , fMaxZ(0.0f)
    {
    }
    _inline friend bool operator != (const SViewport& m1, const SViewport& m2)
    {
        if (m1.nX != m2.nX || m1.nY != m2.nY || m1.nWidth != m2.nWidth || m1.nHeight != m2.nHeight || m1.fMinZ != m2.fMinZ || m1.fMaxZ != m2.fMaxZ)
        {
            return true;
        }
        return false;
    }
    _inline friend bool operator == (const SViewport& m1, const SViewport& m2)
    {
        return !(m1 != m2);
    }
};

struct SRenderListDesc
{
    int m_nStartRI[MAX_LIST_ORDER][EFSLIST_NUM];
    int m_nEndRI[MAX_LIST_ORDER][EFSLIST_NUM];
    int m_nBatchFlags[MAX_LIST_ORDER][EFSLIST_NUM];
};

typedef union UnINT64
{
    uint64 SortVal;
    struct
    {
        uint32 Low;
        uint32 High;
    }i;
} UnINT64;

#define FB_IGNORE_SG_MASK   0x100000

// FIXME: probably better to sort by shaders (Currently sorted by resources)
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(RenderPipeline_h)
#endif
struct SRendItem
{
    uint32 SortVal;
    IRenderElement* pElem;
    union
    {
        uint32 ObjSort;
        float fDist;
    };
    CRenderObject* pObj;
    uint32 nBatchFlags;
    uint32 nOcclQuery : 16;
    enum
    {
        kOcclQueryInvalid = 0xFFFFU
    };
    uint32 nStencRef  : 8;
    uint8  nTextureID;
    SRendItemSorter rendItemSorter;

    union QuickCopy
    {
        uint64  data[4];
        char        bytes[32];
    };

    //==================================================
    static void* mfGetPointerCommon(ESrcPointer ePT, int* Stride, EParamType Type, ESrcPointer Dst, int Flags);

    static _inline void mfGet(uint32 nVal, int& nTechnique, CShader*& Shader, CShaderResources*& Res)
    {
        Shader = (CShader*)CShaderMan::s_pContainer->m_RList[CBaseResource::RListIndexFromId((nVal >> 6) & (MAX_REND_SHADERS - 1))];
        //Shader = (CShader *)CShaderMan::m_pContainer->m_RList[(nVal>>20) & (MAX_REND_SHADERS-1)];
        nTechnique = (nVal & 0x3f);
        if (nTechnique == 0x3f)
        {
            nTechnique = -1;
        }
        int nRes = (nVal >> 18) & (MAX_REND_SHADER_RES - 1);
        //int nRes = (nVal>>6) & (MAX_REND_SHADER_RES-1);
        Res = (nRes) ? CShader::s_ShaderResources_known[nRes] : NULL;
    }
    static _inline CShader* mfGetShader(uint32 flag)
    {
        return (CShader*)CShaderMan::s_pContainer->m_RList[CBaseResource::RListIndexFromId((flag >> 6) & (MAX_REND_SHADERS - 1))];
        //return (CShader *)CShaderMan::m_pContainer->m_RList[(flag>>20) & (MAX_REND_SHADERS-1)];
    }
    static _inline CShaderResources* mfGetRes(uint32 nVal)
    {
        int nRes = (nVal >> 18) & (MAX_REND_SHADER_RES - 1);
        return ((nRes) ? CShader::s_ShaderResources_known[nRes] : NULL);
    }
    static bool IsListEmpty(int nList, [[maybe_unused]] int nProcessID, SRenderListDesc* pRLD)
    {
        int nREs = pRLD->m_nEndRI[0][nList] - pRLD->m_nStartRI[0][nList];
        nREs += pRLD->m_nEndRI[1][nList] - pRLD->m_nStartRI[1][nList];

        if (!nREs)
        {
            return true;
        }
        return false;
    }

    static bool IsListEmpty(int nList, [[maybe_unused]] int nProcessID, SRenderListDesc* pRLD, int nAW)
    {
        int nREs = pRLD->m_nEndRI[nAW][nList] - pRLD->m_nStartRI[nAW][nList];

        if (!nREs)
        {
            return true;
        }
        return false;
    }

    static uint32 BatchFlags(int nList, SRenderListDesc* pRLD)
    {
        uint32 nBatchFlags = 0;
        int nREs = pRLD->m_nEndRI[0][nList] - pRLD->m_nStartRI[0][nList];
        if (nREs)
        {
            nBatchFlags |= pRLD->m_nBatchFlags[0][nList];
        }
        nREs = pRLD->m_nEndRI[1][nList] - pRLD->m_nStartRI[1][nList];
        if (nREs)
        {
            nBatchFlags |= pRLD->m_nBatchFlags[1][nList];
        }
        return nBatchFlags;
    }

    // Sort by SortVal member of RI
    static void mfSortPreprocess(SRendItem* First, int Num);
    // Sort by distance
    static void mfSortByDist(SRendItem* First, int Num, bool bDecals, bool InvertedOrder = false);
    // Sort by light
    static void mfSortByLight(SRendItem* First, int Num, bool bSort, const bool bIgnoreRePtr, bool bSortDecals);
    // Special sorting for ZPass (compromise between depth and batching)
    static void mfSortForZPass(SRendItem* First, int Num);

    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const {}

    // Free all renditem lists. Renderer must be flushed.
    static CThreadSafeWorkerContainer<SRendItem>& RendItems(int threadIndex, int listOrder, int listNum);

    static int m_RecurseLevel[RT_COMMAND_BUF_COUNT];
    static int m_StartFrust[RT_COMMAND_BUF_COUNT][MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS];
    static int m_EndFrust[RT_COMMAND_BUF_COUNT][MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS];
    static int m_ShadowsStartRI[RT_COMMAND_BUF_COUNT][MAX_SHADOWMAP_FRUSTUMS];
    static int m_ShadowsEndRI[RT_COMMAND_BUF_COUNT][MAX_SHADOWMAP_FRUSTUMS];
};

//==================================================================

struct SShaderPass;

union UVertStreamPtr
{
    void* Ptr;
    byte* PtrB;
    SVF_P3F_C4B_T4B_N3F2* PtrVF_P3F_C4B_T4B_N3F2;
};

//==================================================================

#define MAX_DYNVBS 4

//==================================================================

#define GS_HIZENABLE             0x00010000
//==================================================================

#if defined(NULL_RENDERER)
struct SOnDemandD3DStreamProperties
{
};

struct SOnDemandD3DVertexDeclaration
{
};

struct SOnDemandD3DVertexDeclarationCache
{
};
#else
struct SOnDemandD3DStreamProperties
{
    D3D11_INPUT_ELEMENT_DESC* m_pElements;
    uint32 m_nNumElements;
};

struct SOnDemandD3DVertexDeclaration
{
    AZStd::vector<D3D11_INPUT_ELEMENT_DESC> m_Declaration;
};

struct SOnDemandD3DVertexDeclarationCache
{
    ID3D11InputLayout* m_pDeclaration;
};

template <class IndexType>
class FencedIB;
template <class VertexType>
class FencedVB;

struct SVertexDeclaration
{
    int StreamMask;
    AZ::Vertex::Format VertexFormat;
    int InstAttrMask;
    AZStd::vector<D3D11_INPUT_ELEMENT_DESC> m_Declaration;
    ID3D11InputLayout* m_pDeclaration = nullptr;

    // This caching structure is only used for auto-generated vertex formats for instanced renders.
    // The caching format was previously invalid because it cached ID3D11InputLayout based only on the
    // vertex format declaration, rather than based on the vertex format declaration with the vertex shader input
    // table since a different IA layout will be generated whether a vertex shader uses different inputs or not
    void* m_vertexShader = nullptr;

    ~SVertexDeclaration()
    {
        SAFE_RELEASE(m_pDeclaration);
    }
};

#endif

struct SMSAA
{
    SMSAA()
        : Type(0)
        , Quality(0)
        , m_pDepthTex(0)
        , m_pZBuffer(0)
    {
    }

    UINT Type;
    DWORD Quality;
#if defined(NULL_RENDERER)
    void* m_pDepthTex;
    void* m_pZBuffer;
#else
    D3DTexture* m_pDepthTex;
    ID3D11DepthStencilView* m_pZBuffer;
#endif
};

struct SProfInfo
{
    int NumPolys;
    int NumDips;
    CShader* pShader;
    SShaderTechnique* pTechnique;
    double Time;
    int m_nItems;
    SProfInfo()
    {
        NumPolys = 0;
        NumDips = 0;
        pShader = NULL;
        pTechnique = NULL;
        m_nItems = 0;
    }
    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(pShader);
        pSizer->AddObject(pTechnique);
    }
};

struct SRTargetStat
{
    string m_Name;
    uint32 m_nSize;
    uint32 m_nWidth;
    uint32 m_nHeight;
    ETEX_Format m_eTF;

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_Name);
    }
};


struct SPipeStat
{
#if !defined(_RELEASE)
    int m_NumRendHWInstances;
    int m_RendHWInstancesPolysAll;
    int m_RendHWInstancesPolysOne;
    int m_RendHWInstancesDIPs;
    int m_NumTextChanges;
    int m_NumRTChanges;
    int m_NumStateChanges;
    int m_NumRendSkinnedObjects;
    int m_NumVShadChanges;
    int m_NumPShadChanges;
    int m_NumGShadChanges;
    int m_NumDShadChanges;
    int m_NumHShadChanges;
    int m_NumCShadChanges;
    int m_NumVShaders;
    int m_NumPShaders;
    int m_NumGShaders;
    int m_NumDShaders;
    int m_NumHShaders;
    int m_NumRTs;
    int m_NumSprites;
    int m_NumSpriteDIPS;
    int m_NumSpritePolys;
    int m_NumSpriteUpdates;
    int m_NumSpriteAltasesUsed;
    int m_NumSpriteCellsUsed;
    int m_NumQIssued;
    int m_NumQOccluded;
    int m_NumQNotReady;
    int m_NumQStallTime;
    int m_NumImpostersUpdates;
    int m_NumCloudImpostersUpdates;
    int m_NumImpostersDraw;
    int m_NumCloudImpostersDraw;
    int m_NumTextures;
    uint32 m_NumShadowPoolFrustums;
    uint32 m_NumShadowPoolAllocsThisFrame;
    uint32 m_NumShadowMaskChannels;
    uint32 m_NumTiledShadingSkippedLights;
#endif
    int m_NumPSInstructions;
    int m_NumVSInstructions;
    int m_RTCleared;
    int m_RTClearedSize;
    int m_RTCopied;
    int m_RTCopiedSize;
    int m_RTSize;

    CHWShader* m_pMaxPShader;
    CHWShader* m_pMaxVShader;
    void* m_pMaxPSInstance;
    void* m_pMaxVSInstance;

    size_t m_ManagedTexturesStreamSysSize;
    size_t m_ManagedTexturesStreamVidSize;
    size_t m_ManagedTexturesSysMemSize;
    size_t m_ManagedTexturesVidMemSize;
    size_t m_DynTexturesSize;
    size_t m_MeshUpdateBytes;
    size_t m_DynMeshUpdateBytes;
    float m_fOverdraw;
    float m_fSkinningTime;
    float m_fPreprocessTime;
    float m_fSceneTimeMT;
    float m_fTexUploadTime;
    float m_fTexRestoreTime;
    float m_fOcclusionTime;
    float m_fRenderTime;
    float m_fEnvCMapUpdateTime;
    float m_fEnvTextUpdateTime;

    int m_ImpostersSizeUpdate;
    int m_CloudImpostersSizeUpdate;

#if REFRACTION_PARTIAL_RESOLVE_STATS
    float m_fRefractionPartialResolveEstimatedCost;
    int m_refractionPartialResolveCount;
    int m_refractionPartialResolvePixelCount;
#endif

#if defined(ENABLE_PROFILING_CODE)
    int m_NumRendMaterialBatches;
    int m_NumRendGeomBatches;
    int m_NumRendInstances;

    int m_nDIPs[EFSLIST_NUM];
    int m_nInsts;
    int m_nInstCalls;
    int m_nPolygons[EFSLIST_NUM];
    int m_nPolygonsByTypes[EFSLIST_NUM][EVCT_NUM][2];
#endif

#if defined(ENABLE_ART_RT_TIME_ESTIMATE)
    float m_actualRenderTimeMinusPost;
#endif

    float m_fTimeDIPs[EFSLIST_NUM];
    float m_fTimeDIPsZ;
    float m_fTimeDIPsAO;
    float m_fTimeDIPsRAIN;
    float m_fTimeDIPsDeferredLayers;
    float m_fTimeDIPsSprites;
} _ALIGN(128);


//Batch flags.
// - When adding/removing batch flags, please, update sBatchList static list in D3DRendPipeline.cpp
enum EBatchFlags
{
    FB_GENERAL        = 0x1,
    FB_TRANSPARENT    = 0x2,
    FB_SKIN           = 0x4,
    FB_Z              = 0x8,
    FB_FUR            = 0x10,
    FB_ZPREPASS       = 0x20,
    FB_PREPROCESS     = 0x40,
    FB_MOTIONBLUR     = 0x80,
    FB_POST_3D_RENDER = 0x100,
    FB_MULTILAYERS    = 0x200,
    FB_COMPILED_OBJECT = 0x400,
    FB_CUSTOM_RENDER  = 0x800,
    FB_SOFTALPHATEST  = 0x1000,
    FB_LAYER_EFFECT   = 0x2000,
    FB_WATER_REFL     = 0x4000,
    FB_WATER_CAUSTIC  = 0x8000,
    FB_DEBUG          = 0x10000,
    FB_PARTICLES_THICKNESS = 0x20000,
    FB_TRANSPARENT_AFTER_DOF = 0x40000, // for transparent render element skip Depth of field effect
    FB_EYE_OVERLAY    = 0x80000,

    FB_MASK           = 0xfffff //! FB flags cannot exceed 0xfffff
};


// Commit flags
#define FC_TARGETS        1
#define FC_GLOBAL_PARAMS  2
#define FC_PER_INSTANCE_PARAMS 4
#define FC_MATERIAL_PARAMS     0x10
#define FC_ALL            0x1f


// m_RP.m_Flags
#define RBF_NEAREST          0x10000

// m_RP.m_TI.m_PersFlags
#define RBPF_DRAWTOTEXTURE   (1 << 16)     // 0x10000
#define RBPF_MIRRORCAMERA    (1 << 17)     // 0x20000
#define RBPF_MIRRORCULL      (1 << 18)     // 0x40000

#define RBPF_ZPASS             (1 << 19)   // 0x80000
#define RBPF_SHADOWGEN         (1 << 20)   // 0x100000

#define RBPF_FP_DIRTY          (1 << 21)   // 0x200000

#define RBPF_NO_SHADOWGEN      (1 << 22)

#define RBPF_IMPOSTERGEN       (1 << 23)
#define RBPF_MAKESPRITE        (1 << 24)   // 0x1000000
// CRY DX12
#define RBPF_FP_MATRIXDIRTY    (1 << 25)

#define RBPF_HDR               (1 << 26)
#define RBPF_REVERSE_DEPTH     (1 << 27)
#define RBPF_ENCODE_HDR        (1 << 29)
#define RBPF_OBLIQUE_FRUSTUM_CLIPPING  (1 << 30)

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
#define RBPF_RENDER_SCENE_TO_TEXTURE (1 << 31)
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED


// m_RP.m_PersFlags1
#define RBPF1_USESTREAM       (1 << 0)
#define RBPF1_USESTREAM_MASK  ((1 << VSF_NUM) - 1)

#define RBPF1_IN_CLEAR        (1 << 17)

#define RBPF1_SKIP_AFTER_POST_PROCESS   (1 << 18)

// m_RP.m_PersFlags2
#define RBPF2_NOSHADERFOG      (1 << 0)
#define RBPF2_RAINRIPPLES       (1 << 1)
#define RBPF2_NOALPHABLEND     (1 << 2)
#define RBPF2_SINGLE_FORWARD_LIGHT_PASS  (1 << 3)
#define RBPF2_MSAA_RESTORE_SAMPLE_MASK         (1 << 4)
#define RBPF2_READMASK_RESERVED_STENCIL_BIT      (1 << 5)
#define RBPF2_POST_3D_RENDERER_PASS  (1 << 6)
#define RBPF2_LENS_OPTICS_COMPOSITE  (1 << 7)
#define RBPF2_HDR_FP16         (1 << 9)
#define RBPF2_CUSTOM_SHADOW_PASS (1 << 10)
#define RBPF2_CUSTOM_RENDER_PASS (1 << 11)
// UNUSED: (1<<12)

#define RBPF2_COMMIT_CM        (1 << 13)
#define RBPF2_ZPREPASS         (1 << 14)

#define RBPF2_FORWARD_SHADING_PASS  (1 << 15)

#define RBPF2_MSAA_STENCILCULL  (1 << 16)

#define RBPF2_THERMAL_RENDERMODE_TRANSPARENT_PASS (1 << 17)
#define RBPF2_NOALPHATEST     (1 << 18)
#define RBPF2_WATERRIPPLES    (1 << 19)
#define RBPF2_ALLOW_DEFERREDSHADING  (1 << 20)

#define RBPF2_COMMIT_PF       (1 << 21)
#define RBPF2_MSAA_SAMPLEFREQ_PASS  (1 << 22)
#define RBPF2_DRAWTOCUBE      (1 << 23)

#define RBPF2_MOTIONBLURPASS     (1 << 24)
#define RBPF2_MATERIALLAYERPASS  (1 << 25)
#define RBPF2_DISABLECOLORWRITES (1 << 26)

#define RBPF2_NOPOSTAA  (1 << 27)
#define RBPF2_SKIN        (1 << 28)

#define RBPF2_LIGHTSHAFTS        (1 << 29)
#define RBPF2_WRITEMASK_RESERVED_STENCIL_BIT      (1 << 30)           // 0x010
#define RBPF2_HALFRES_PARTICLES  (1 << 31)

// m_RP.m_FlagsPerFlush
#define RBSI_LOCKCULL                               0x2
#define RBSI_EXTERN_VMEM_BUFFERS    0x800000
#define RBSI_INSTANCED                          0x10000000
#define RBSI_CUSTOM_PREVMATRIX    0x20000000

// m_RP.m_ShaderLightMask
#define SLMF_DIRECT       0
#define SLMF_POINT        1
#define SLMF_PROJECTED    2
#define SLMF_TYPE_MASK    (SLMF_POINT | SLMF_PROJECTED)

#define SLMF_LTYPE_SHIFT  8
#define SLMF_LTYPE_BITS   4

struct SLightPass
{
    SRenderLight* pLights[4];
    uint32 nStencLTMask;
    uint32 nLights;
    uint32 nLTMask;
    bool bRect;
    RectI rc;
};

#define MAX_STREAMS 16

struct SStreamInfo
{
    const void* pStream;
    int nOffset;
    int nStride;

    SStreamInfo() {}
    SStreamInfo(const void* stream, int offset, int stride)
        : pStream(stream)
        , nOffset(offset)
        , nStride(stride) {}

    SStreamInfo& operator=(const CRendElementBase::SGeometryStreamInfo& stream)
    {
        pStream = stream.pStream;
        nOffset = stream.nOffset;
        nStride = stream.nStride;
        return *this;
    }

    inline bool operator==(const SStreamInfo& other) const
    {
        return pStream == other.pStream && nOffset == other.nOffset && nStride == other.nStride;
    }
};

struct SFogState
{
    bool m_bEnable;
    ColorF m_FogColor;
    ColorF m_CurColor;

    bool operator != (const SFogState& fs) const
    {
        return m_FogColor != fs.m_FogColor;
    }
};

enum EShapeMeshType
{
    SHAPE_PROJECTOR = 0,
    SHAPE_PROJECTOR1,
    SHAPE_PROJECTOR2,
    SHAPE_CLIP_PROJECTOR,
    SHAPE_CLIP_PROJECTOR1,
    SHAPE_CLIP_PROJECTOR2,
    SHAPE_SIMPLE_PROJECTOR,
    SHAPE_SPHERE,
    SHAPE_BOX,
    SHAPE_MAX,
};

struct SThreadInfo
{
    uint32 m_PersFlags;          // Never reset
    float m_RealTime;

    Matrix44A m_matView = Matrix44A(IDENTITY);
    Matrix44A m_matProj = Matrix44A(IDENTITY);

    CCamera m_cam;  // current camera
    int m_nFrameID;                         // with recursive calls, access through GetFrameID(true)
    uint32 m_nFrameUpdateID;    // without recursive calls, access through GetFrameID(false)
    int     m_arrZonesRoundId[MAX_PREDICTION_ZONES];        // rounds ID from 3D engine, useful for texture streaming
    SFogState m_FS;
    CRenderObject* m_pIgnoreObject;

    Plane m_pObliqueClipPlane;
    bool m_bObliqueClipPlane;

    byte m_eCurColorOp;
    byte m_eCurAlphaOp;
    byte m_eCurColorArg;
    byte m_eCurAlphaArg;
    bool m_sRGBWrite {
        false
    };

    PerFrameParameters m_perFrameParameters;

    ~SThreadInfo() {}
    SThreadInfo& operator = (const SThreadInfo& ti)
    {
        if (&ti == this)
        {
            return *this;
        }
        m_PersFlags = ti.m_PersFlags;
        m_RealTime = ti.m_RealTime;
        m_matView = ti.m_matView;
        m_matProj = ti.m_matProj;
        m_cam = ti.m_cam;
        m_nFrameID = ti.m_nFrameID;
        m_nFrameUpdateID = ti.m_nFrameUpdateID;
        for (int z = 0; z < MAX_PREDICTION_ZONES; z++)
        {
            m_arrZonesRoundId[z] = ti.m_arrZonesRoundId[z];
        }
        m_FS = ti.m_FS;
        m_perFrameParameters = ti.m_perFrameParameters;
        m_pIgnoreObject = ti.m_pIgnoreObject;
        memcpy(&m_pObliqueClipPlane, &ti.m_pObliqueClipPlane, sizeof(m_pObliqueClipPlane));
        m_bObliqueClipPlane = ti.m_bObliqueClipPlane;
        m_eCurColorOp = ti.m_eCurColorOp;
        m_eCurAlphaOp = ti.m_eCurAlphaOp;
        m_eCurColorArg = ti.m_eCurColorArg;
        m_eCurAlphaArg = ti.m_eCurAlphaArg;
        m_sRGBWrite = ti.m_sRGBWrite;
        return *this;
    }
};


#ifdef STRIP_RENDER_THREAD
struct SSingleThreadInfo
    : public SThreadInfo
{
    SThreadInfo& operator[] (const int) {return *this; }
    const SThreadInfo& operator[] (const int) const {return *this; }
};
#endif


// Render pipeline structure
struct SRenderPipeline
{
    CShader* m_pShader;
    CShader* m_pReplacementShader;
    CRenderObject* m_pCurObject;
    CRenderObject* m_pIdendityRenderObject;
    IRenderElement* m_pRE;
    CRendElementBase* m_pEventRE;
    int m_RendNumVerts;
    uint32 m_nBatchFilter;           // Batch flags ( FB_ )
    SShaderTechnique* m_pRootTechnique;
    SShaderTechnique* m_pCurTechnique;
    SShaderPass* m_pCurPass;
    uint32 m_CurPassBitMask;
    int m_nShaderTechnique;
    int m_nShaderTechniqueType;
    CShaderResources* m_pShaderResources;
    CRenderObject* m_pPrevObject;
    int m_nLastRE;
    TArray<SRendItem*> m_RIs[MAX_REND_GEOMS_IN_BATCH];

    ColorF m_CurGlobalColor;

    float m_fMinDistance;               // min distance to texture
    uint64 m_ObjFlags;             // Instances flag for batch (merged)
    int m_Flags;                // Reset on start pipeline

    EShapeMeshType m_nDeferredPrimitiveID;
    int m_nZOcclusionBufferID;

    threadID m_nFillThreadID;
    threadID m_nProcessThreadID;
    SThreadInfo m_TI[RT_COMMAND_BUF_COUNT];
    SThreadInfo m_OldTI[MAX_RECURSION_LEVELS];
    // SFogVolumeData container will be used to accumulate the fog volume influences.
    CThreadSafeRendererContainer<SFogVolumeData> m_fogVolumeContibutionsData[RT_COMMAND_BUF_COUNT];
   
    uint32 m_PersFlags1;        // Persistent flags - never reset
    uint32 m_PersFlags2;          // Persistent flags - never reset
    int m_FlagsPerFlush;          // Flags which resets for each shader flush
    uint32 m_nCommitFlags;
    uint32 m_FlagsStreams_Decl;
    uint32 m_FlagsStreams_Stream;
    AZ::Vertex::Format m_CurVFormat;
    uint32 m_FlagsShader_LT;      // Shader light mask
    uint64 m_FlagsShader_RT;    // Shader runtime mask
    uint32 m_FlagsShader_MD;      // Shader texture modificator mask
    uint32 m_FlagsShader_MDV;     // Shader vertex modificator mask
    uint32 m_nShaderQuality;

    void (* m_pRenderFunc)();

    uint32 m_CurGPRAllocStateCommit;
    uint32 m_CurGPRAllocState;
    int m_CurHiZState;

    uint32 m_CurState;
    uint32 m_StateOr;
    uint32 m_StateAnd;
    int m_CurAlphaRef;
    uint32 m_MaterialStateOr;
    uint32 m_MaterialStateAnd;
    int m_MaterialAlphaRef;
    uint32 m_ForceStateOr;
    uint32 m_ForceStateAnd;
    bool m_bIgnoreObjectAlpha;
    ECull m_eCull;
    uint32 m_previousPersFlags = 0;

    int m_CurStencilState;
    uint32 m_CurStencMask;
    uint32 m_CurStencWriteMask;
    uint32 m_CurStencRef;
    int m_CurStencilRefAndMask;
    int m_CurStencilCullFunc;

    SStreamInfo m_VertexStreams[MAX_STREAMS];
    void* m_pIndexStream;

    uint32 m_IndexStreamOffset;
    RenderIndexType m_IndexStreamType;

    bool m_bFirstPass;
    uint32 m_nNumRendPasses;
    int m_NumShaderInstructions;

    string m_sExcludeShader;

    TArray<SProfInfo> m_Profile;

    CCamera m_PrevCamera;

    SRenderListDesc* m_pRLD;

    uint32 m_nRendFlags;
    bool m_bUseHDR;
    int m_nPassGroupID;           // EFSLIST_ pass type
    int m_nPassGroupDIP;          // EFSLIST_ pass type
    int m_nSortGroupID;
    uint32 m_nFlagsShaderBegin;
    uint8 m_nCurrResolveBounds[4];

    Vec2 m_CurDownscaleFactor;

    ERenderQuality m_eQuality;

    TArray<ShadowMapFrustum> m_SMFrustums[RT_COMMAND_BUF_COUNT][MAX_REND_RECURSION_LEVELS];
    TArray<int> m_SMCustomFrustumIDs[RT_COMMAND_BUF_COUNT][MAX_REND_RECURSION_LEVELS];
    CThreadSafeWorkerContainer<CustomShadowMapFrustumData> m_arrCustomShadowMapFrustumData[RT_COMMAND_BUF_COUNT];

    struct SShadowFrustumToRender
    {
        ShadowMapFrustum* pFrustum;
        int nRecursiveLevel;
        int nLightID;
        SRenderLight* pLight;
    } _ALIGN(16);
    TArray<SShadowFrustumToRender> SShadowFrustumToRenderList[RT_COMMAND_BUF_COUNT];
    TArray<SRenderLight> m_DLights[RT_COMMAND_BUF_COUNT][MAX_REND_RECURSION_LEVELS];
    SLightPass m_LPasses[MAX_REND_LIGHTS];
    float m_fProfileTime;

    struct ShadowInfo
    {
        ShadowMapFrustum* m_pCurShadowFrustum;
        Vec3 vViewerPos;
        int m_nOmniLightSide;
    } m_ShadowInfo;

    DynArray<SDeferredDecal> m_DeferredDecals[RT_COMMAND_BUF_COUNT][MAX_REND_RECURSION_LEVELS];
    bool m_isDeferrredNormalDecals[RT_COMMAND_BUF_COUNT][MAX_REND_RECURSION_LEVELS];

    UVertStreamPtr m_StreamPtrTang;
    UVertStreamPtr m_NextStreamPtrTang;

    UVertStreamPtr m_StreamPtr;
    UVertStreamPtr m_NextStreamPtr;
    int m_StreamStride;
    uint m_StreamOffsetTC;
    uint m_StreamOffsetColor;

    float m_fLastWaterFOVUpdate;
    Vec3 m_LastWaterViewdirUpdate;
    Vec3 m_LastWaterUpdirUpdate;
    Vec3 m_LastWaterPosUpdate;
    float m_fLastWaterUpdate;
    int m_nLastWaterFrameID;

    bool m_depthWriteStateUsed;

#if !defined(NULL_RENDERER)
    SMSAA   m_MSAAData;
    bool IsMSAAEnabled() const { return m_MSAAData.Type > 0; }

    enum
    {
        nNumParticleVertexIndexBuffer = 3
    };

    // particle date for writing directly to VMEM
    FencedVB<byte>* m_pParticleVertexBuffer[nNumParticleVertexIndexBuffer];
    FencedIB <uint16>* m_pParticleIndexBuffer[nNumParticleVertexIndexBuffer];

    // During writing, we only expose the base Video Memory pointer
    // and the offsets to the next free memory in this buffer
    byte* m_pParticleVertexVideoMemoryBase[nNumParticleVertexIndexBuffer];
    byte* m_pParticleindexVideoMemoryBase[nNumParticleVertexIndexBuffer];

    uint32 m_nParticleVertexOffset[nNumParticleVertexIndexBuffer];
    uint32 m_nParticleIndexOffset[nNumParticleVertexIndexBuffer];

    // total amount of allocated memory for particle vertex/index buffers
    uint32 m_nParticleVertexBufferAvailableMemory;
    uint32 m_nParticleIndexBufferAvailableMemory;


    int m_nStreamOffset[3]; // deprecated!

    AZ::Vertex::Format m_vertexFormats[eVF_Max];
    SOnDemandD3DVertexDeclaration m_D3DVertexDeclarations[eVF_Max];
    AZStd::unordered_map<AZ::u32, SOnDemandD3DVertexDeclarationCache> m_D3DVertexDeclarationCache[1 << VSF_NUM][2]; // [StreamMask][Morph][VertexFormatCRC]
    SOnDemandD3DStreamProperties m_D3DStreamProperties[VSF_NUM];

    TArray<SVertexDeclaration*> m_CustomVD;
#else
    bool IsMSAAEnabled() const { return false; }
#endif

    uint16* m_RendIndices;
    uint16* m_SysRendIndices;
    byte* m_SysArray;
    int    m_SizeSysArray;

    TArray<byte> m_SysVertexPool[RT_COMMAND_BUF_COUNT];
    TArray<uint16> m_SysIndexPool[RT_COMMAND_BUF_COUNT];

    int m_RendNumGroup;
    int m_RendNumIndices;
    int m_FirstIndex;
    int m_FirstVertex;

    // members for external vertex/index buffers
#if !defined(NULL_RENDERER)
    FencedVB<byte>*            m_pExternalVertexBuffer;
    FencedIB<uint16>*  m_pExternalIndexBuffer;
    int m_nExternalVertexBufferFirstIndex;
    int m_nExternalVertexBufferFirstVertex;
#endif

    // [Shader System TO DO] - change this so that we don't deal with static slots assignments
    // The following  structure is practically used only once to set Instance texture coord matrix.
    SEfResTexture* m_ShaderTexResources[MAX_TMU];

    int m_Frame;
    int m_FrameMerge;

    float m_fCurOpacity;

    SPipeStat m_PS[RT_COMMAND_BUF_COUNT];
    DynArray<SRTargetStat> m_RTStats;

    int m_MaxVerts;
    int m_MaxTris;

    int m_RECustomTexBind[8];
    int m_ShadowCustomTexBind[8];
    bool m_ShadowCustomComparisonSampling[8];

    CRenderView* m_pCurrentFillView;
    CRenderView* m_pCurrentRenderView;
    std::shared_ptr<CRenderView> m_pRenderViews[RT_COMMAND_BUF_COUNT];
    //===================================================================
    // Input render data
    SRenderLight* m_pSunLight;
    CThreadSafeWorkerContainer <CRenderObject*> m_TempObjects[RT_COMMAND_BUF_COUNT];
    CRenderObject* m_ObjectsPool;
    uint32 m_nNumObjectsInPool;

#if !defined(_RELEASE)
    //===================================================================
    // Drawcall count debug view - per Node - r_stats 6
    IRenderer::RNDrawcallsMapNode m_pRNDrawCallsInfoPerNode[RT_COMMAND_BUF_COUNT];

    // Functionality for retrieving previous frames stats to use this frame
    IRenderer::RNDrawcallsMapNode m_pRNDrawCallsInfoPerNodePreviousFrame[RT_COMMAND_BUF_COUNT];

    //===================================================================
    // Drawcall count debug view - per mesh - perf hud renderBatchStats
    IRenderer::RNDrawcallsMapMesh m_pRNDrawCallsInfoPerMesh[RT_COMMAND_BUF_COUNT];

    // Functionality for retrieving previous frames stats to use this frame
    IRenderer::RNDrawcallsMapMesh m_pRNDrawCallsInfoPerMeshPreviousFrame[RT_COMMAND_BUF_COUNT];
#endif

    //================================================================
    // Render elements..

    class CREHDRProcess* m_pREHDR;
    class CREDeferredShading* m_pREDeferredShading;
    class CREPostProcess* m_pREPostProcess;

    //=================================================================
    // WaveForm tables
    static const uint32 sSinTableCount = 1024;
    float m_tSinTable[sSinTableCount];

    // For explicit geometry cache motion blur
    Matrix44A* m_pPrevMatrix;

public:
    SRenderPipeline()
        : m_pShader(0)
        , m_nShaderTechnique(-1)
        , m_pCurTechnique(NULL)
        , m_pREPostProcess(NULL)
        , m_CurDownscaleFactor(Vec2(1.0f, 1.0f))
        , m_IndexStreamOffset(~0)
        , m_IndexStreamType(Index16)
        , m_ObjectsPool(NULL)
    {}

    ~SRenderPipeline()
    {
#if !defined(NULL_RENDERER)
        for (unsigned int i = 0, n = m_CustomVD.Num(); i < n; i++)
        {
            delete m_CustomVD[i];
        }
#endif
    }

    _inline SShaderTechnique* GetStartTechnique() const
    {
        if (m_pShader)
        {
            return m_pShader->mfGetStartTechnique(m_nShaderTechnique);
        }
        return NULL;
    }

    static const uint32 sNumObjectsInPool = 1024;

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        // don't add the following as these are intermediate members (might be NULL) and already accounted for
        //pSizer->AddObject( m_pCurPass );
        //pSizer->AddObject( m_pCurTechnique );
        //pSizer->AddObject( m_pRootTechnique );

        pSizer->AddObject(m_pSunLight);
        pSizer->AddObject(m_sExcludeShader);
        pSizer->AddObject(m_Profile);

        pSizer->AddObject(m_SysArray, m_SizeSysArray);
        for (uint32 i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
        {
            pSizer->AddObject(m_TempObjects[i]);
            for (uint32 j = 0; j < MAX_RECURSION_LEVELS; ++j)
            {
                pSizer->AddObject(m_DLights[i][j]);
            }
            pSizer->AddObject(m_SysVertexPool[i]);
            pSizer->AddObject(m_SysIndexPool[i]);
            pSizer->AddObject(m_fogVolumeContibutionsData[i]);
        }
        pSizer->AddObject(m_RIs);
        pSizer->AddObject(m_RTStats);
    }

    void SetRenderElement(IRenderElement* renderElement)
    {
        m_pRE = renderElement;
    }
};

extern CryCriticalSection m_sREResLock;

///////////////////////////////////////////////////////////////////////////////
// sort opeartors for render items
struct SCompareItemPreprocess
{
    bool operator()(const SRendItem& a, const SRendItem& b) const
    {
        if (a.nBatchFlags != b.nBatchFlags)
        {
            return a.nBatchFlags < b.nBatchFlags;
        }

        return a.SortVal < b.SortVal;
    }
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareRendItem
{
    bool operator()(const SRendItem& a, const SRendItem& b) const
    {
        /// Nearest objects should be rendered first
        int nNearA = (a.ObjSort & FOB_HAS_PREVMATRIX);
        int nNearB = (b.ObjSort & FOB_HAS_PREVMATRIX);
        if (nNearA != nNearB)               // Sort by nearest flag
        {
            return nNearA > nNearB;
        }

        if (a.SortVal != b.SortVal)         // Sort by shaders
        {
            return a.SortVal < b.SortVal;
        }

        if (a.nTextureID != b.nTextureID)
        {
            return a.nTextureID < b.nTextureID; // Sort by object custom texture (usually terrain sector texture)
        }

        if (a.pElem != b.pElem)               // Sort by geometry
        {
            return a.pElem < b.pElem;
        }

        return (a.ObjSort & 0xFFFF) < (b.ObjSort & 0xFFFF);   // Sort by distance
    }
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareRendItemZPass
{
    bool operator()(const SRendItem& a, const SRendItem& b) const
    {
        const int layerSize = 50;  // Note: ObjSort contains round(entityDist * 2) for meshes

        // Sort by nearest flag
        int nNearA = (a.ObjSort & FOB_HAS_PREVMATRIX);
        int nNearB = (b.ObjSort & FOB_HAS_PREVMATRIX);
        if (nNearA != nNearB)
        {
            return nNearA > nNearB;
        }

        if (a.SortVal != b.SortVal)    // Sort by shaders
        {
            return a.SortVal < b.SortVal;
        }

        // Sort by depth/distance layers
        int depthLayerA = (a.ObjSort & 0xFFFF) / layerSize;
        int depthLayerB = (b.ObjSort & 0xFFFF) / layerSize;
        if (depthLayerA != depthLayerB)
        {
            return depthLayerA < depthLayerB;
        }

        if (a.nStencRef != b.nStencRef)
        {
            return a.nStencRef < b.nStencRef;
        }
        if (a.nTextureID != b.nTextureID)
        {
            return a.nTextureID < b.nTextureID; // Sort by object custom texture (usually terrain sector texture)
        }

        // Sorting by geometry less important than sorting by shaders
        //if (a.Item != b.Item)    // Sort by geometry
        //  return a.Item < b.Item;

        return (a.ObjSort & 0xFFFF) < (b.ObjSort & 0xFFFF);    // Sort by distance
    }
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareItem_Decal
{
    bool operator()(const SRendItem& a, const SRendItem& b) const
    {
        uint32 objSortA_Low(a.ObjSort & 0xFFFF);
        uint32 objSortA_High(a.ObjSort & ~0xFFFF);
        uint32 objSortB_Low(b.ObjSort & 0xFFFF);
        uint32 objSortB_High(b.ObjSort & ~0xFFFF);

        if (objSortA_Low != objSortB_Low)
        {
            return objSortA_Low < objSortB_Low;
        }

        if (a.SortVal != b.SortVal)
        {
            return a.SortVal < b.SortVal;
        }

        return objSortA_High < objSortB_High;
    }
};

struct SCompareItem_Terrain
{
    bool operator()(const SRendItem& a, const SRendItem& b) const
    {
        IRenderElement* pREa = a.pElem;
        IRenderElement* pREb = b.pElem;

        if (pREa->GetCustomTexBind(0) != pREb->GetCustomTexBind(0))
        {
            return pREa->GetCustomTexBind(0) < pREb->GetCustomTexBind(0);
        }

        return a.ObjSort < b.ObjSort;
    }
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareItem_TerrainLayers
{
    bool operator()(const SRendItem& a, const SRendItem& b) const
    {
        //if (a.ObjSort != b.ObjSort)
        //  return a.ObjSort < b.ObjSort;

        float pSurfTypeA = ((float*)a.pElem->GetCustomData())[8];
        float pSurfTypeB = ((float*)b.pElem->GetCustomData())[8];
        if (pSurfTypeA != pSurfTypeB)
        {
            return (pSurfTypeA < pSurfTypeB);
        }

        pSurfTypeA = ((float*)a.pElem->GetCustomData())[9];
        pSurfTypeB = ((float*)b.pElem->GetCustomData())[9];
        if (pSurfTypeA != pSurfTypeB)
        {
            return (pSurfTypeA < pSurfTypeB);
        }

        pSurfTypeA = ((float*)a.pElem->GetCustomData())[11];
        pSurfTypeB = ((float*)b.pElem->GetCustomData())[11];
        return (pSurfTypeA < pSurfTypeB);
    }
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareDist
{
    bool operator()(const SRendItem& a, const SRendItem& b) const
    {
        if (fcmp(a.fDist, b.fDist))
        {
            return a.rendItemSorter.ParticleCounter() < b.rendItemSorter.ParticleCounter();
        }

        return (a.fDist > b.fDist);
    }
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareDistInverted
{
    bool operator()(const SRendItem& a, const SRendItem& b) const
    {
        if (fcmp(a.fDist, b.fDist))
        {
            return a.rendItemSorter.ParticleCounter() > b.rendItemSorter.ParticleCounter();
        }

        return (a.fDist < b.fDist);
    }
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareByRenderingPass
{
    bool operator()(const SRendItem& rA, const SRendItem& rB) const
    {
        return rA.rendItemSorter.IsRecursivePass() < rB.rendItemSorter.IsRecursivePass();
    }
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareByOnlyStableFlagsOctreeID
{
    bool operator()(const SRendItem& rA, const SRendItem& rB) const
    {
        return rA.rendItemSorter < rB.rendItemSorter;
    }
};
