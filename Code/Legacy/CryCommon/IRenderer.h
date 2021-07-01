/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "Cry_Geo.h"
#include "Cry_Camera.h"
#include "ITexture.h"
#include "Cry_Vector2.h"
#include "Cry_Vector3.h"
#include "Cry_Matrix33.h"
#include "Cry_Color.h"
#include "smartptr.h"
#include "StringUtils.h"
#include <IXml.h> // <> required for Interfuscator
#include "smartptr.h"
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/containers/intrusive_slist.h>

// forward declarations
struct SRenderingPassInfo;
struct SRTStack;
struct SFogVolumeData;
// Callback used for DXTCompress
typedef void (* MIPDXTcallback)(const void* buffer, size_t count, void* userData);

typedef void (* GpuCallbackFunc)(DWORD context);

// Callback for shadercache miss
typedef void (* ShaderCacheMissCallback)(const char* acShaderRequest);

struct ICaptureFrameListener
{
    virtual ~ICaptureFrameListener (){}
    virtual bool OnNeedFrameData(unsigned char*& pConvertedTextureBuf) = 0;
    virtual void OnFrameCaptured(void) = 0;
    virtual int OnGetFrameWidth(void) = 0;
    virtual int OnGetFrameHeight(void) = 0;
    virtual int OnCaptureFrameBegin(int* pTexHandle) = 0;

    enum ECaptureFrameFlags
    {
        eCFF_NoCaptureThisFrame = (0 << 1),
        eCFF_CaptureThisFrame = (1 << 1),
    };
};

// Forward declarations.
//////////////////////////////////////////////////////////////////////
typedef void* WIN_HWND;
typedef void* WIN_HINSTANCE;
typedef void* WIN_HDC;
typedef void* WIN_HGLRC;

class CREMesh;
class CMesh;
//class   CImage;
struct  CStatObj;
class CVegetation;
struct  ShadowMapFrustum;
struct  IStatObj;
class CObjManager;
struct  SPrimitiveGroup;
class CRendElementBase;
class CRenderObject;
class CTexMan;
//class   ColorF;
class CShadowVolEdge;
class CCamera;
class CDLight;
struct SDeferredLightVolume;
struct  ILog;
struct  IConsole;
struct  ICVar;
struct  ITimer;
struct  ISystem;
class IGPUParticleEngine;
class ICrySizer;
struct IRenderAuxGeom;
struct SREPointSpriteCreateParams;
struct SPointSpriteVertex;
struct RenderLMData;
struct SShaderParam;
struct SSkyLightRenderParams;
struct SParticleRenderInfo;
struct SParticleAddJobCompare;
struct IColorGradingController;
class IStereoRenderer;
struct IFFont;
struct IFFont_RenderProxy;
struct STextDrawContext;
struct IRenderMesh;
struct ShadowFrustumMGPUCache;
struct IAsyncTextureCompileListener;
struct IClipVolume;
struct SClipVolumeBlendInfo;
class CRenderView;
struct SDynTexture2;
class CTexture;
enum ETexPool : int;

//////////////////////////////////////////////////////////////////////
typedef unsigned char bvec4[4];
typedef float vec4_t[4];
typedef unsigned char byte;
typedef float vec2_t[2];

//DOC-IGNORE-BEGIN
#include "Cry_Color.h"
#include "Tarray.h"

#include <IFont.h>
//DOC-IGNORE-END

#define MAX_NUM_VIEWPORTS 7

// Query types for CryInd editor (used in EF_Query() function).
enum ERenderQueryTypes
{
    EFQ_DeleteMemoryArrayPtr = 1,
    EFQ_DeleteMemoryPtr,
    EFQ_GetShaderCombinations,
    EFQ_SetShaderCombinations,
    EFQ_CloseShaderCombinations,

    EFQ_MainThreadList,
    EFQ_RenderThreadList,
    EFQ_RenderMultithreaded,

    EFQ_RecurseLevel,
    EFQ_IncrementFrameID,
    EFQ_DeviceLost,
    EFQ_LightSource,

    EFQ_Alloc_APITextures,
    EFQ_Alloc_APIMesh,

    // Memory allocated by meshes in system memory.
    EFQ_Alloc_Mesh_SysMem,
    EFQ_Mesh_Count,

    EFQ_HDRModeEnabled,
    EFQ_ParticlesTessellation,
    EFQ_WaterTessellation,
    EFQ_MeshTessellation,
    EFQ_GetShadowPoolFrustumsNum,
    EFQ_GetShadowPoolAllocThisFrameNum,
    EFQ_GetShadowMaskChannelsNum,
    EFQ_GetTiledShadingSkippedLightsNum,

    // Description:
    //      Query will return all textures in the renderer,
    //      pass pointer to an SRendererQueryGetAllTexturesParam instance
    EFQ_GetAllTextures,

    // Description:
    //      Release resources allocated by GetAllTextures query
    //      pass pointer to an SRendererQueryGetAllTexturesParam instance, populated by EFQ_GetAllTextures
    EFQ_GetAllTexturesRelease,

    // Description:
    //      Query will return all IRenderMesh objects in the renderer,
    //      Pass an array pointer to be allocated and filled with the IRendermesh pointers. The calling function is responsible for freeing this memory.
    //      This was originally a two pass process, but proved to be non-thread-safe, leading to buffer overruns and underruns.
    EFQ_GetAllMeshes,

    // Summary:
    //      Multigpu (crossfire/sli) is enabled.
    EFQ_MultiGPUEnabled,
    EFQ_SetDrawNearFov,
    EFQ_GetDrawNearFov,
    EFQ_TextureStreamingEnabled,
    EFQ_MSAAEnabled,
    EFQ_AAMode,

    EFQ_Fullscreen,
    EFQ_GetTexStreamingInfo,
    EFQ_GetMeshPoolInfo,

    // Description:
    //      True when shading is done in linear space, de-gamma on texture lookup, gamma on frame buffer writing (sRGB), false otherwise.
    EFQ_sLinearSpaceShadingEnabled,

    // The percentages of overscan borders for left/right and top/bottom to adjust the title safe area.
    EFQ_OverscanBorders,

    // Get num active post effects
    EFQ_NumActivePostEffects,

    // Get size of textures memory pool
    EFQ_TexturesPoolSize,
    EFQ_RenderTargetPoolSize,

    EFQ_GetShaderCacheInfo,

    EFQ_GetFogCullDistance,
    EFQ_GetMaxRenderObjectsNum,

    EFQ_IsRenderLoadingThreadActive,

    EFQ_GetSkinningDataPoolSize,

    EFQ_GetViewportDownscaleFactor,
    EFQ_ReverseDepthEnabled,

    EFQ_GetLastD3DDebugMessage
};

struct ID3DDebugMessage
{
public:
    virtual void Release() = 0;
    virtual const char* GetMessage() const = 0;

protected:
    ID3DDebugMessage() {}
    virtual ~ID3DDebugMessage() {}
};

enum EScreenAspectRatio
{
    eAspect_Unknown,
    eAspect_4_3,
    eAspect_16_9,
    eAspect_16_10,
};

class SBoundingVolume
{
public:
    SBoundingVolume()
        : m_vCenter(0, 0, 0)
        , m_fRadius(0) {}
    ~SBoundingVolume() {}

    void SetCenter(const Vec3& center)  { m_vCenter = center; }
    void SetRadius(float radius)        { m_fRadius = radius; }
    const Vec3& GetCenter() const       { return m_vCenter;   }
    float GetRadius() const             { return m_fRadius;   }

protected:
    Vec3    m_vCenter;
    float   m_fRadius;
};

class SMinMaxBox
    : public SBoundingVolume
{
public:
    SMinMaxBox()
    {
        Clear();
    }
    SMinMaxBox(const Vec3& min, const Vec3& max) :
        m_min(min), 
        m_max(max)
    {
        UpdateSphere();
    }

    // Summary:
    //  Destructor
    virtual ~SMinMaxBox() {}

    void  AddPoint(const Vec3& pt)
    {
        if (pt.x > m_max.x)
        {
            m_max.x = pt.x;
        }
        if (pt.x < m_min.x)
        {
            m_min.x = pt.x;
        }

        if (pt.y > m_max.y)
        {
            m_max.y = pt.y;
        }
        if (pt.y < m_min.y)
        {
            m_min.y = pt.y;
        }

        if (pt.z > m_max.z)
        {
            m_max.z = pt.z;
        }
        if (pt.z < m_min.z)
        {
            m_min.z = pt.z;
        }

        // Summary:
        //   Updates the center and radius.
        UpdateSphere();
    }
    void  AddPoint(float x, float y, float z)
    {
        AddPoint(Vec3(x, y, z));
    }

    void  Union(const SMinMaxBox& box)  { AddPoint(box.GetMin()); AddPoint(box.GetMax()); }

    const Vec3& GetMin() const     { return m_min; }
    const Vec3& GetMax() const     { return m_max; }

    void  SetMin(const Vec3& min)  { m_min = min; UpdateSphere(); }
    void  SetMax(const Vec3& max)  { m_max = max; UpdateSphere(); }

    float GetWidthInX() const       { return m_max.x - m_min.x; }
    float GetWidthInY() const       { return m_max.y - m_min.y; }
    float GetWidthInZ() const       { return m_max.z - m_min.z; }

    bool  PointInBBox(const Vec3& pt) const;

    bool  ViewFrustumCull(const CameraViewParameters& viewParameters, const Matrix44& mat);

    void  Transform(const Matrix34& mat)
    {
        Vec3 verts[8];
        CalcVerts(verts);
        Clear();
        for (int i = 0; i < 8; i++)
        {
            AddPoint(mat.TransformPoint(verts[i]));
        }
    }

    // Summary:
    //  Resets the bounding box.
    void  Clear()
    {
        m_min = Vec3(999999.0f, 999999.0f, 999999.0f);
        m_max = Vec3(-999999.0f, -999999.0f, -999999.0f);
    }

protected:
    void UpdateSphere()
    {
        m_vCenter =  m_min;
        m_vCenter += m_max;
        m_vCenter *= 0.5f;

        Vec3 rad  =  m_max;
        rad      -= m_vCenter;
        m_fRadius =  rad.len();
    }
    void CalcVerts(Vec3 pVerts[8]) const
    {
        pVerts[0].Set(m_max.x, m_max.y, m_max.z);
        pVerts[4].Set(m_max.x, m_max.y, m_min.z);
        pVerts[1].Set(m_min.x, m_max.y, m_max.z);
        pVerts[5].Set(m_min.x, m_max.y, m_min.z);
        pVerts[2].Set(m_min.x, m_min.y, m_max.z);
        pVerts[6].Set(m_min.x, m_min.y, m_min.z);
        pVerts[3].Set(m_max.x, m_min.y, m_max.z);
        pVerts[7].Set(m_max.x, m_min.y, m_min.z);
    }

private:
    Vec3 m_min; // Original object space BV.
    Vec3 m_max;
};



//////////////////////////////////////////////////////////////////////
// All possible primitive types

enum PublicRenderPrimitiveType
{
    prtTriangleList,
    prtTriangleStrip,
    prtLineList,
    prtLineStrip
};

//////////////////////////////////////////////////////////////////////
#define R_CULL_DISABLE  0
#define R_CULL_NONE     0
#define R_CULL_FRONT    1
#define R_CULL_BACK     2

//////////////////////////////////////////////////////////////////////
#define R_DEFAULT_LODBIAS 0

//////////////////////////////////////////////////////////////////////
#define R_SOLID_MODE    0
#define R_WIREFRAME_MODE 1

#define R_DX9_RENDERER  2
#define R_DX11_RENDERER 3
#define R_NULL_RENDERER 4
#define R_CUBAGL_RENDERER 5
#define R_GL_RENDERER 6
#define R_METAL_RENDERER 7
#define R_DX12_RENDERER 8

//////////////////////////////////////////////////////////////////////
// Render features

#define RFT_FREE_0x1          0x1
#define RFT_ALLOW_RECTTEX     0x2
#define RFT_OCCLUSIONQUERY    0x4
#define RFT_FREE_0x8          0x8
#define RFT_HWGAMMA           0x10
#define RFT_FREE_0x20         0x20
#define RFT_COMPRESSTEXTURE   0x40
#define RFT_FREE_0x80         0x80
#define RFT_ALLOWANISOTROPIC  0x100      // Allows anisotropic texture filtering.
#define RFT_SUPPORTZBIAS      0x200
#define RFT_FREE_0x400        0x400
#define RFT_FREE_0x800        0x800
#define RFT_FREE_0x1000       0x1000
#define RFT_FREE_0x2000       0x2000
#define RFT_OCCLUSIONTEST     0x8000     // Support hardware occlusion test.

#define RFT_HW_ARM_MALI       0x04000    // Unclassified ARM (MALI) hardware.
#define RFT_HW_INTEL          0x10000    // Unclassified intel hardware.
#define RFT_HW_QUALCOMM       0x10000    // Unclassified Qualcomm hardware
#define RFT_HW_ATI            0x20000    // Unclassified ATI hardware.
#define RFT_HW_NVIDIA         0x40000    // Unclassified NVidia hardware.
#define RFT_HW_MASK           0x74000    // Graphics chip mask.

#define RFT_HW_HDR            0x80000    // Hardware supports high dynamic range rendering.

#define RFT_HW_SM20           0x100000   // Shader model 2.0
#define RFT_HW_SM2X           0x200000   // Shader model 2.X
#define RFT_HW_SM30           0x400000   // Shader model 3.0
#define RFT_HW_SM40           0x800000   // Shader model 4.0
#define RFT_HW_SM50           0x1000000  // Shader model 5.0

#define RFT_FREE_0x2000000    0x2000000
#define RFT_FREE_0x4000000    0x4000000
#define RFT_FREE_0x8000000    0x8000000

#define RFT_HW_VERTEX_STRUCTUREDBUF 0x10000000 // Supports Structured Buffers in the Vertex Shader.
#define RFT_RGBA                    0x20000000 // RGBA order (otherwise BGRA).
#define RFT_COMPUTE_SHADERS         0x40000000 // Compute Shaders support
#define RFT_HW_VERTEXTEXTURES       0x80000000 // Vertex texture fetching supported.

//====================================================================
// PrecacheResources flags

#define FPR_NEEDLIGHT     1
#define FPR_2D            2
#define FPR_HIGHPRIORITY  4
#define FPR_SYNCRONOUS    8
#define FPR_STARTLOADING    16
#define FPR_SINGLE_FRAME_PRIORITY_UPDATE 32

//=====================================================================
// SetRenderTarget flags
#define SRF_SCREENTARGET  1
#define SRF_USE_ORIG_DEPTHBUF 2
#define SRF_USE_ORIG_DEPTHBUF_MSAA 4

//====================================================================
// Draw shaders flags (EF_EndEf3d)

#define SHDF_ALLOWHDR               BIT(0)
#define SHDF_CUBEMAPGEN             BIT(1)
#define SHDF_ZPASS                  BIT(2)
#define SHDF_ZPASS_ONLY             BIT(3)
#define SHDF_DO_NOT_CLEAR_Z_BUFFER  BIT(4)
#define SHDF_ALLOWPOSTPROCESS       BIT(5)
#define SHDF_ALLOW_AO               BIT(8)
#define SHDF_ALLOW_WATER            BIT(9)
#define SHDF_NOASYNC                BIT(10)
#define SHDF_NO_DRAWNEAR            BIT(11)
#define SHDF_STREAM_SYNC            BIT(13)
#define SHDF_NO_SHADOWGEN           BIT(15)

//////////////////////////////////////////////////////////////////////
// Virtual screen size
const float VIRTUAL_SCREEN_WIDTH = 800.0f;
const float VIRTUAL_SCREEN_HEIGHT = 600.0f;

//////////////////////////////////////////////////////////////////////
// Object states
#define OS_ALPHA_BLEND             0x1
#define OS_ADD_BLEND               0x2
#define OS_MULTIPLY_BLEND          0x4
#define OS_TRANSPARENT            (OS_ALPHA_BLEND | OS_ADD_BLEND | OS_MULTIPLY_BLEND)
#define OS_NODEPTH_TEST            0x8
#define OS_NODEPTH_WRITE           0x10
#define OS_ANIM_BLEND              0x20
#define OS_ENVIRONMENT_CUBEMAP     0x40

// Render State flags
#define GS_BLSRC_MASK              0xf
#define GS_BLSRC_ZERO              0x1
#define GS_BLSRC_ONE               0x2
#define GS_BLSRC_DSTCOL            0x3
#define GS_BLSRC_ONEMINUSDSTCOL    0x4
#define GS_BLSRC_SRCALPHA          0x5
#define GS_BLSRC_ONEMINUSSRCALPHA  0x6
#define GS_BLSRC_DSTALPHA          0x7
#define GS_BLSRC_ONEMINUSDSTALPHA  0x8
#define GS_BLSRC_ALPHASATURATE     0x9
#define GS_BLSRC_SRCALPHA_A_ZERO   0xa // separate alpha blend state
#define GS_BLSRC_SRC1ALPHA         0xb // dual source blending


#define GS_BLDST_MASK              0xf0
#define GS_BLDST_ZERO              0x10
#define GS_BLDST_ONE               0x20
#define GS_BLDST_SRCCOL            0x30
#define GS_BLDST_ONEMINUSSRCCOL    0x40
#define GS_BLDST_SRCALPHA          0x50
#define GS_BLDST_ONEMINUSSRCALPHA  0x60
#define GS_BLDST_DSTALPHA          0x70
#define GS_BLDST_ONEMINUSDSTALPHA  0x80
#define GS_BLDST_ONE_A_ZERO        0x90 // separate alpha blend state
#define GS_BLDST_ONEMINUSSRC1ALPHA 0xa0 // dual source blending


#define GS_DEPTHWRITE              0x00000100

#define GS_COLMASK_RT1             0x00000200
#define GS_COLMASK_RT2             0x00000400
#define GS_COLMASK_RT3             0x00000800

#define GS_NOCOLMASK_R             0x00001000
#define GS_NOCOLMASK_G             0x00002000
#define GS_NOCOLMASK_B             0x00004000
#define GS_NOCOLMASK_A             0x00008000
#define GS_COLMASK_RGB             (GS_NOCOLMASK_A)
#define GS_COLMASK_A               (GS_NOCOLMASK_R | GS_NOCOLMASK_G | GS_NOCOLMASK_B)
#define GS_COLMASK_NONE            (GS_NOCOLMASK_R | GS_NOCOLMASK_G | GS_NOCOLMASK_B | GS_NOCOLMASK_A)
#define GS_COLMASK_MASK            GS_COLMASK_NONE
#define GS_COLMASK_SHIFT           12

#define GS_WIREFRAME               0x00010000
#define GS_NODEPTHTEST             0x00040000

#define GS_BLEND_MASK              0x0f0000ff

#define GS_DEPTHFUNC_LEQUAL        0x00000000
#define GS_DEPTHFUNC_EQUAL         0x00100000
#define GS_DEPTHFUNC_GREAT         0x00200000
#define GS_DEPTHFUNC_LESS          0x00300000
#define GS_DEPTHFUNC_GEQUAL        0x00400000
#define GS_DEPTHFUNC_NOTEQUAL      0x00500000
#define GS_DEPTHFUNC_HIZEQUAL      0x00600000 // keep hi-z test, always pass fine depth. Useful for debug display
#define GS_DEPTHFUNC_ALWAYS        0x00700000
#define GS_DEPTHFUNC_MASK          0x00700000

#define GS_STENCIL                 0x00800000

#define GS_BLEND_OP_MASK           0x03000000
#define GS_BLOP_MAX                              0x01000000
#define GS_BLOP_MIN                              0x02000000

// Separate alpha blend mode
#define GS_BLALPHA_MASK            0x0c000000
#define GS_BLALPHA_MIN             0x04000000
#define GS_BLALPHA_MAX             0x08000000

#define GS_ALPHATEST_MASK          0xf0000000
#define GS_ALPHATEST_GREATER       0x10000000
#define GS_ALPHATEST_LESS          0x20000000
#define GS_ALPHATEST_GEQUAL        0x40000000
#define GS_ALPHATEST_LEQUAL        0x80000000

#define FORMAT_8_BIT   8
#define FORMAT_24_BIT 24
#define FORMAT_32_BIT 32

//==================================================================
// StencilStates

//Note: If these are altered, g_StencilFuncLookup and g_StencilOpLookup arrays
//          need to be updated in turn

#define FSS_STENCFUNC_ALWAYS   0x0
#define FSS_STENCFUNC_NEVER    0x1
#define FSS_STENCFUNC_LESS     0x2
#define FSS_STENCFUNC_LEQUAL   0x3
#define FSS_STENCFUNC_GREATER  0x4
#define FSS_STENCFUNC_GEQUAL   0x5
#define FSS_STENCFUNC_EQUAL    0x6
#define FSS_STENCFUNC_NOTEQUAL 0x7
#define FSS_STENCFUNC_MASK     0x7

#define FSS_STENCIL_TWOSIDED   0x8

#define FSS_CCW_SHIFT          16

#define FSS_STENCOP_KEEP    0x0
#define FSS_STENCOP_REPLACE 0x1
#define FSS_STENCOP_INCR    0x2
#define FSS_STENCOP_DECR    0x3
#define FSS_STENCOP_ZERO    0x4
#define FSS_STENCOP_INCR_WRAP 0x5
#define FSS_STENCOP_DECR_WRAP 0x6
#define FSS_STENCOP_INVERT 0x7

#define FSS_STENCFAIL_SHIFT   4
#define FSS_STENCFAIL_MASK    (0x7 << FSS_STENCFAIL_SHIFT)

#define FSS_STENCZFAIL_SHIFT  8
#define FSS_STENCZFAIL_MASK   (0x7 << FSS_STENCZFAIL_SHIFT)

#define FSS_STENCPASS_SHIFT   12
#define FSS_STENCPASS_MASK    (0x7 << FSS_STENCPASS_SHIFT)

#define STENC_FUNC(op) (op)
#define STENC_CCW_FUNC(op) (op << FSS_CCW_SHIFT)
#define STENCOP_FAIL(op) (op << FSS_STENCFAIL_SHIFT)
#define STENCOP_ZFAIL(op) (op << FSS_STENCZFAIL_SHIFT)
#define STENCOP_PASS(op) (op << FSS_STENCPASS_SHIFT)
#define STENCOP_CCW_FAIL(op) (op << (FSS_STENCFAIL_SHIFT + FSS_CCW_SHIFT))
#define STENCOP_CCW_ZFAIL(op) (op << (FSS_STENCZFAIL_SHIFT + FSS_CCW_SHIFT))
#define STENCOP_CCW_PASS(op) (op << (FSS_STENCPASS_SHIFT + FSS_CCW_SHIFT))

//Stencil masks
#define BIT_STENCIL_RESERVED 0x80
#define BIT_STENCIL_INSIDE_CLIPVOLUME 0x40
#define STENC_VALID_BITS_NUM 7
#define STENC_MAX_REF ((1 << STENC_VALID_BITS_NUM) - 1)

// Read FrameBuffer type
enum ERB_Type
{
    eRB_BackBuffer,
    eRB_FrontBuffer,
    eRB_ShadowBuffer
};

enum EVertexCostTypes
{
    EVCT_STATIC = 0,
    EVCT_VEGETATION,
    EVCT_SKINNED,
    EVCT_NUM
};

//////////////////////////////////////////////////////////////////////

struct SDispFormat
{
    int m_Width;
    int m_Height;
    int m_BPP;
};

struct SAAFormat
{
    char szDescr[64];
    int nSamples;
    int nQuality;
};

// Summary:
//   Info about Terrain sector texturing.
struct SSectorTextureSet
{
    SSectorTextureSet(unsigned short nT0)
    {
        nTex0 = nT0;
        fTexOffsetX = fTexOffsetY = 0;
        fTexScale = 1.f;
    }

    unsigned short nTex0;
    float fTexOffsetX, fTexOffsetY, fTexScale;
};

struct IRenderNode;
struct SShaderItem;

#ifdef SUPPORT_HW_MOUSE_CURSOR
class IHWMouseCursor
{
public:
    virtual ~IHWMouseCursor() {}
    virtual void SetPosition(int x, int y) = 0;
    virtual void Show() = 0;
    virtual void Hide() = 0;
};
#endif

//////////////////////////////////////////////////////////////////////
//DOC-IGNORE-BEGIN
#include <IShader.h> // <> required for Interfuscator
//DOC-IGNORE-END
#include <IRenderMesh.h>

// Flags passed in function FreeResources.
#define FRR_SHADERS   1
#define FRR_SHADERTEXTURES 2
#define FRR_TEXTURES  4
#define FRR_SYSTEM    8
#define FRR_RESTORE   0x10
#define FRR_REINITHW  0x20
#define FRR_DELETED_MESHES 0x40
#define FRR_FLUSH_TEXTURESTREAMING 0x80
#define FRR_OBJECTS     0x100
#define FRR_RENDERELEMENTS 0x200
#define FRR_RP_BUFFERS 0x400
#define FRR_SYSTEM_RESOURCES 0x800
#define FRR_POST_EFFECTS 0x1000
#define FRR_ALL      -1

// Refresh render resources flags.
// Flags passed in function RefreshResources.
#define FRO_SHADERS  1
#define FRO_SHADERTEXTURES  2
#define FRO_TEXTURES 4
#define FRO_GEOMETRY 8
#define FRO_FORCERELOAD 0x10

//=============================================================================
// Shaders render target stuff.

#define FRT_CLEAR_DEPTH   0x1
#define FRT_CLEAR_STENCIL 0x2
#define FRT_CLEAR_COLOR   0x4
#define FRT_CLEAR (FRT_CLEAR_COLOR | FRT_CLEAR_DEPTH | FRT_CLEAR_STENCIL)
#define FRT_CLEAR_FOGCOLOR 0x8
#define FRT_CLEAR_IMMEDIATE 0x10
#define FRT_CLEAR_COLORMASK 0x20
#define FRT_CLEAR_RESET_VIEWPORT 0x40

#define FRT_CAMERA_REFLECTED_WATERPLANE 0x40
#define FRT_CAMERA_REFLECTED_PLANE      0x80
#define FRT_CAMERA_CURRENT              0x100

#define FRT_USE_FRONTCLIPPLANE          0x200
#define FRT_USE_BACKCLIPPLANE           0x400

#define FRT_GENERATE_MIPS               0x800

#define FRT_RENDTYPE_CUROBJECT          0x1000
#define FRT_RENDTYPE_CURSCENE           0x2000
#define FRT_RENDTYPE_RECURSIVECURSCENE  0x4000
#define FRT_RENDTYPE_COPYSCENE          0x8000

// Summary:
//   Flags used in DrawText function.
// See also:
//   SDrawTextInfo
// Remarks:
//   Text must be fixed pixel size.
enum EDrawTextFlags
{
    eDrawText_Left          = 0,        // default left alignment if neither Center or Right are specified
    eDrawText_Center        = BIT(0),   // centered alignment, otherwise right or left
    eDrawText_Right         = BIT(1),   // right alignment, otherwise center or left
    eDrawText_CenterV       = BIT(2),   // center vertically, otherwise top
    eDrawText_Bottom        = BIT(3),   // bottom alignment

    eDrawText_2D            = BIT(4),   // 3 component vector is used for xy screen position, otherwise it's 3d world space position

    eDrawText_FixedSize     = BIT(5),   // font size is defined in the actual pixel resolution, otherwise it's in the virtual 800x600
    eDrawText_800x600       = BIT(6),   // position are specified in the virtual 800x600 resolution, otherwise coordinates are in pixels

    eDrawText_Monospace     = BIT(7),   // non proportional font rendering (Font width is same for all characters)

    eDrawText_Framed        = BIT(8),   // draw a transparent, rectangular frame behind the text to ease readability independent from the background

    eDrawText_DepthTest     = BIT(9),   // text should be occluded by world geometry using the depth buffer
    eDrawText_IgnoreOverscan = BIT(10),  // ignore the overscan borders, text should be drawn at the location specified
    eDrawText_UseTransform  = BIT(11),  // use a transform for the text
};

// Debug stats/views for Partial resolves
// if REFRACTION_PARTIAL_RESOLVE_DEBUG_VIEWS is enabled, make sure REFRACTION_PARTIAL_RESOLVE_STATS is too
#if defined(PERFORMANCE_BUILD)
    #define REFRACTION_PARTIAL_RESOLVE_STATS 1
    #define REFRACTION_PARTIAL_RESOLVE_DEBUG_VIEWS 0
#elif defined(_RELEASE) // note: _RELEASE is defined in PERFORMANCE_BUILD, so this check must come second
    #define REFRACTION_PARTIAL_RESOLVE_STATS 0
    #define REFRACTION_PARTIAL_RESOLVE_DEBUG_VIEWS 0
#else
    #define REFRACTION_PARTIAL_RESOLVE_STATS 1
    #define REFRACTION_PARTIAL_RESOLVE_DEBUG_VIEWS 1
#endif

#if REFRACTION_PARTIAL_RESOLVE_DEBUG_VIEWS
enum ERefractionPartialResolvesDebugViews
{
    eRPR_DEBUG_VIEW_2D_AREA = 1,
    eRPR_DEBUG_VIEW_3D_BOUNDS,
    eRPR_DEBUG_VIEW_2D_AREA_OVERLAY
};
#endif

//////////////////////////////////////////////////////////////////////////
// Description:
//   This structure used in DrawText method of renderer.
//   It provide all necessary information of how to render text on screen.
// See also:
//   IRenderer::Draw2dText
struct SDrawTextInfo
{
    // Summary:
    //  One of EDrawTextFlags flags.
    // See also:
    //  EDrawTextFlags
    int     flags;
    // Summary:
    //  Text color, (r,g,b,a) all members must be specified.
    float   color[4];
    float xscale;
    float yscale;

    SDrawTextInfo()
    {
        flags = 0;
        color[0] = color[1] = color[2] = color[3] = 1;
        xscale = 1.0f;
        yscale = 1.0f;
    }
};

#define UIDRAW_TEXTSIZEFACTOR (12.0f)
#define MIN_RESOLUTION_SCALE (0.25f)
#define MAX_RESOLUTION_SCALE (4.0f)

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(IRenderer_h)
#else
//SLI/CROSSFIRE GPU maximum count
    #define MAX_GPU_NUM 4
#endif

#define MAX_FRAME_ID_STEP_PER_FRAME 20
const int MAX_GSM_LODS_NUM = 16;

const f32 DRAW_NEAREST_MIN = 0.03f;
const f32 DRAW_NEAREST_MAX = 40.0f;

//===================================================================

//////////////////////////////////////////////////////////////////////
struct IRenderDebugListener
{
    virtual ~IRenderDebugListener() {}

    virtual void OnDebugDraw() = 0;
};

//////////////////////////////////////////////////////////////////////
struct ILoadtimeCallback
{
    virtual void LoadtimeUpdate(float fDeltaTime) = 0;
    virtual void LoadtimeRender() = 0;
    virtual ~ILoadtimeCallback(){}
};

//////////////////////////////////////////////////////////////////////
struct ISyncMainWithRenderListener
{
    virtual void SyncMainWithRender() = 0;
    virtual ~ISyncMainWithRenderListener(){}
};

//////////////////////////////////////////////////////////////////////
enum ERenderType
{
    eRT_Undefined,
    eRT_Null,
    eRT_DX11,
    eRT_DX12,
    eRT_Provo,
    eRT_OpenGL,
    eRT_Metal,
    eRT_Jasper,
};

//////////////////////////////////////////////////////////////////////
// Enum for types of deferred lights
enum eDeferredLightType
{
    eDLT_DeferredLight = 0,

    eDLT_NumShadowCastingLights = eDLT_DeferredLight + 1,
    // these lights cannot cast shadows
    eDLT_DeferredCubemap = eDLT_NumShadowCastingLights,
    eDLT_DeferredAmbientLight,
    eDLT_NumLightTypes,
};

const float RENDERER_LIGHT_UNIT_SCALE = 10000.0f;  // Scale factor between photometric and internal light units

//////////////////////////////////////////////////////////////////////
struct SCustomRenderInitArgs
{
    bool appStartedFromMediaCenter;
};

#if defined(ANDROID)
enum
{
    CULL_SIZEX = 128
};
enum
{
    CULL_SIZEY = 64
};
#else
enum
{
    CULL_SIZEX = 256
};
enum
{
    CULL_SIZEY = 128
};
#endif

//////////////////////////////////////////////////////////////////////
// Description:
//   Z-buffer as occlusion buffer definitions: used, shared and initialized in engine and renderer.
struct SHWOccZBuffer
{
    uint32* pHardwareZBuffer;
    uint32* pZBufferVMem;
    uint32 ZBufferSizeX;
    uint32 ZBufferSizeY;
    uint32 HardwareZBufferRSXOff;
    uint32 ZBufferVMemRSXOff;
    uint32 pad[2];  // Keep 32 byte aligned
    SHWOccZBuffer()
        : pHardwareZBuffer(NULL)
        , pZBufferVMem(NULL)
        , ZBufferSizeX(CULL_SIZEX)
        , ZBufferSizeY(CULL_SIZEY)
        , ZBufferVMemRSXOff(0)
        , HardwareZBufferRSXOff(0){}
};

class ITextureStreamListener
{
public:
    virtual void OnCreatedStreamedTexture(void* pHandle, const char* name, int nMips, int nMinMipAvailable) = 0;
    virtual void OnDestroyedStreamedTexture(void* pHandle) = 0;
    virtual void OnTextureWantsMip(void* pHandle, int nMinMip) = 0;
    virtual void OnTextureHasMip(void* pHandle, int nMinMip) = 0;
    virtual void OnBegunUsingTextures(void** pHandles, size_t numHandles) = 0;
    virtual void OnEndedUsingTextures(void** pHandle, size_t numHandles) = 0;

protected:
    virtual ~ITextureStreamListener() {}
};

enum eDolbyVisionMode
{
    eDVM_Disabled,
    eDVM_RGBPQ,
    eDVM_Vision,
};

enum ERenderPipelineProfilerStats
{
    eRPPSTATS_OverallFrame = 0,
    eRPPSTATS_Recursion,

    // Scene
    eRPPSTATS_SceneOverall,
    eRPPSTATS_SceneDecals,
    eRPPSTATS_SceneForward,
    eRPPSTATS_SceneWater,

    // Shadows
    eRPPSTATS_ShadowsOverall,
    eRPPSTATS_ShadowsSun,
    eRPPSTATS_ShadowsSunCustom,
    eRPPSTATS_ShadowsLocal,

    // Lighting
    eRPPSTATS_LightingOverall,
    eRPPSTATS_LightingGI,

    // VFX
    eRPPSTATS_VfxOverall,
    eRPPSTATS_VfxTransparent,
    eRPPSTATS_VfxFog,
    eRPPSTATS_VfxFlares,

    // Individual Total Illumination stats
    eRPPSTATS_TI_INJECT_CLEAR,
    eRPPSTATS_TI_VOXELIZE,
    eRPPSTATS_TI_INJECT_AIR,
    eRPPSTATS_TI_INJECT_LIGHT,
    eRPPSTATS_TI_INJECT_REFL0,
    eRPPSTATS_TI_INJECT_REFL1,
    eRPPSTATS_TI_INJECT_DYNL,
    eRPPSTATS_TI_NID_DIFF,
    eRPPSTATS_TI_GEN_DIFF,
    eRPPSTATS_TI_GEN_SPEC,
    eRPPSTATS_TI_GEN_AIR,
    eRPPSTATS_TI_DEMOSAIC_DIFF,
    eRPPSTATS_TI_DEMOSAIC_SPEC,
    eRPPSTATS_TI_UPSCALE_DIFF,
    eRPPSTATS_TI_UPSCALE_SPEC,

    RPPSTATS_NUM
};

struct RPProfilerStats
{
    float   gpuTime;
    float   gpuTimeSmoothed;
    float   gpuTimeMax;
    float   cpuTime;
    uint32  numDIPs;
    uint32  numPolys;

    // Internal
    float   _gpuTimeMaxNew;
};

struct TransformationMatrices
{
    Matrix44A m_viewMatrix;
    Matrix44A m_projectMatrix;
};

struct ISvoRenderer
{
    virtual bool IsShaderItemUsedForVoxelization([[maybe_unused]] SShaderItem& rShaderItem, [[maybe_unused]] IRenderNode* pRN){ return false; }
    virtual void Release(){}
};

//////////////////////////////////////////////////////////////////////
struct SRenderPipeline;
struct SRenderThread;
struct SShaderTechnique;
struct SShaderPass;
struct SDepthTexture;
struct SRenderTileInfo;

class CShaderMan;
class CDeviceBufferManager;
class CShaderResources;
class PerInstanceConstantBufferPool;

namespace AZ {
    class Plane;
    namespace Vertex {
        class Format;
    }
}
enum eRenderPrimitiveType : int8;
enum RenderIndexType : int;

struct IRenderAPI
{
};

struct IRenderer
    : public IRenderAPI
{
    virtual ~IRenderer(){}

    virtual void AddRenderDebugListener(IRenderDebugListener* pRenderDebugListener) = 0;
    virtual void RemoveRenderDebugListener(IRenderDebugListener* pRenderDebugListener) = 0;

    virtual ERenderType GetRenderType() const = 0;

    virtual const char* GetRenderDescription() const
    {
        return "CryRenderer";
    }

    // Summary:
    //  Initializes the renderer, params are self-explanatory.
    virtual WIN_HWND Init(int x, int y, int width, int height, unsigned int cbpp, int zbpp, int sbits, bool fullscreen, bool isEditor, WIN_HINSTANCE hinst, WIN_HWND Glhwnd = 0, bool bReInit = false, const SCustomRenderInitArgs* pCustomArgs = 0, bool bShaderCacheGen = false) = 0;
    virtual void PostInit() = 0;

    virtual bool IsPost3DRendererEnabled() const { return false; }

    virtual int GetFeatures() = 0;
    virtual const void SetApiVersion(const AZStd::string& apiVersion) = 0;
    virtual const void SetAdapterDescription(const AZStd::string& adapterDescription) = 0;
    virtual const AZStd::string& GetApiVersion() const = 0;
    virtual const AZStd::string& GetAdapterDescription() const = 0;
    virtual void GetVideoMemoryUsageStats(size_t& vidMemUsedThisFrame, size_t& vidMemUsedRecently, bool bGetPoolsSizes = false) = 0;
    virtual int GetNumGeomInstances() const = 0;
    virtual int GetNumGeomInstanceDrawCalls() const = 0;
    virtual int GetCurrentNumberOfDrawCalls() const = 0;
    virtual void GetCurrentNumberOfDrawCalls(int& nGeneral, int& nShadowGen) const = 0;
    //Sums DIP counts for the EFSLIST_* passes that match the submitted mask.
    //Compose the mask with bitwise arithmetic, use (1 << EFSLIST_*) per list.
    //e.g. to sum general and transparent, pass in ( (1 << EFSLIST_GENERAL) | (1 << EFSLIST_TRANSP) )
    virtual int GetCurrentNumberOfDrawCalls(uint32 EFSListMask) const = 0;
    virtual float GetCurrentDrawCallRTTimes(uint32 EFSListMask) const = 0;

    virtual void SetDebugRenderNode(IRenderNode* pRenderNode) = 0;
    virtual bool IsDebugRenderNode(IRenderNode* pRenderNode) const = 0;

    /////////////////////////////////////////////////////////////////////////////////
    // Render-context management
    /////////////////////////////////////////////////////////////////////////////////
    virtual bool DeleteContext(WIN_HWND hWnd) = 0;
    virtual bool CreateContext(WIN_HWND hWnd, bool bAllowMSAA = false, int SSX = 1, int SSY = 1) = 0;
    virtual bool SetCurrentContext(WIN_HWND hWnd) = 0;
    virtual void MakeMainContextActive() = 0;
    virtual WIN_HWND GetCurrentContextHWND() = 0;
    virtual bool IsCurrentContextMainVP() = 0;

    // Summary:
    //  Gets height of the current viewport.
    virtual int   GetCurrentContextViewportHeight() const = 0;

    // Summary:
    //  Gets width of the current viewport.
    virtual int   GetCurrentContextViewportWidth() const = 0;
    /////////////////////////////////////////////////////////////////////////////////

    // Summary:
    //  Shuts down the renderer.
    virtual void  ShutDown(bool bReInit = false) = 0;
    virtual void  ShutDownFast() = 0;

    // Description:
    //  Creates array of all supported video formats (except low resolution formats).
    // Return value:
    //  Number of formats in memory.
    virtual int EnumDisplayFormats(SDispFormat* Formats) = 0;

    // Summary:
    //  Returns all supported by video card video AA formats.
    virtual int EnumAAFormats(SAAFormat* Formats) = 0;

    // Summary:
    //  Changes resolution of the window/device (doesn't require to reload the level.
    virtual bool  ChangeResolution(int nNewWidth, int nNewHeight, int nNewColDepth, int nNewRefreshHZ, bool bFullScreen, bool bForceReset) = 0;

    // Note:
    //  Should be called at the beginning of every frame.
    virtual void  BeginFrame() = 0;

    // Summary:
    //  Creates default system shaders and textures.
    virtual void  InitSystemResources(int nFlags) = 0;
    virtual void  InitTexturesSemantics() = 0;

    // Summary:
    //  Frees the allocated resources.
    virtual void  FreeResources(int nFlags) = 0;

    // Summary:
    //  Shuts down the renderer.
    virtual void  Release() = 0;

    // See also:
    //   r_ShowDynTextures
    virtual void RenderDebug(bool bRenderStats = true) = 0;

    // Note:
    //   Should be called at the end of every frame.
    virtual void  EndFrame() = 0;

    // Force a swap on the backbuffer
    virtual void    ForceSwapBuffers() = 0;

    // Summary:
    //      Try to flush the render thread commands to keep the render thread active during
    //      level loading, but simpy return if the render thread is still busy
    virtual void TryFlush() = 0;

    virtual void GetViewport(int* x, int* y, int* width, int* height) const = 0;
    virtual void SetViewport(int x, int y, int width, int height, int id = 0) = 0;
    virtual void SetRenderTile(f32 nTilesPosX = 0.f, f32 nTilesPosY = 0.f, f32 nTilesGridSizeX = 1.f, f32 nTilesGridSizeY = 1.f) = 0;
    virtual void SetScissor(int x = 0, int y = 0, int width = 0, int height = 0) = 0;
    virtual Matrix44A& GetViewProjectionMatrix() = 0;
    virtual void SetTranspOrigCameraProjMatrix(Matrix44A& matrix) = 0;

    virtual EScreenAspectRatio GetScreenAspect(int nWidth, int nHeight) = 0;

    virtual Vec2 SetViewportDownscale(float xscale, float yscale) = 0;
    virtual void SetViewParameters(const CameraViewParameters& viewParameters) = 0; // Direct setter
    virtual void ApplyViewParameters(const CameraViewParameters& viewParameters) = 0; // Uses CameraViewParameters to create matrices.

    // Summary:
    //  Draws user primitives.
    virtual void DrawDynVB(SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds, PublicRenderPrimitiveType nPrimType) = 0;

    struct DynUiPrimitive : public AZStd::intrusive_slist_node<DynUiPrimitive>
    {
        SVF_P2F_C4B_T2F_F4B* m_vertices = nullptr;
        uint16* m_indices = nullptr;
        int m_numVertices = 0;
        int m_numIndices = 0;
    };

    using DynUiPrimitiveList = AZStd::intrusive_slist<DynUiPrimitive, AZStd::slist_base_hook<DynUiPrimitive>>;

    // Summary:
    //  Draws a list of UI primitives as one draw call (if using separate render thread)
    virtual void DrawDynUiPrimitiveList(DynUiPrimitiveList& primitives, int totalNumVertices, int totalNumIndices) = 0;

    // Summary:
    //  Sets the renderer camera.
    virtual void  SetCamera(const CCamera& cam) = 0;

    // Summary:
    //  Gets the renderer camera.
    virtual const CCamera& GetCamera() = 0;

    virtual CRenderView* GetRenderViewForThread(int nThreadID) = 0;
    // Summary:
    //  Gets the renderer previous camera.
    //virtual const CCamera& GetCameraPrev() = 0;

    // Summary:
    //  Sets delta gamma.
    virtual bool  SetGammaDelta(float fGamma) = 0;

    // Summary:
    //  Restores gamma
    // Note:
    //  Reset gamma setting if not in fullscreen mode.
    virtual void  RestoreGamma(void) = 0;

    // Summary:
    //  Changes display size.
    virtual bool  ChangeDisplay(unsigned int width, unsigned int height, unsigned int cbpp) = 0;

    // Summary:
    //  Changes viewport size.
    virtual void  ChangeViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height, bool bMainViewport = false, float scaleWidth = 1.0f, float scaleHeight = 1.0f) = 0;

    // Summary:
    //  Saves source data to a Tga file.
    // Note:
    //  Should not be here.
    virtual bool  SaveTga(unsigned char* sourcedata, int sourceformat, int w, int h, const char* filename, bool flip) const = 0;

    // Summary:
    //  Sets the current binded texture.
    virtual void  SetTexture(int tnum) = 0;

    // Summary:
    //  Sets the current bound texture for the given texture unit
    virtual void  SetTexture(int tnum, int nUnit) = 0;

    // Summary:
    //  Sets the white texture.
    virtual void  SetWhiteTexture() = 0;

    // Summary:
    //  Gets the white texture Id.
    virtual int  GetWhiteTextureId() const = 0;

    // Summary:
    //  Gets the white texture Id.
    virtual int  GetBlackTextureId() const = 0;

    // Summary:
    //  Draws a 2d image on the screen.
    // Example:
    //  Hud etc.
    virtual void  Draw2dImage(float xpos, float ypos, float w, float h, int texture_id, float s0 = 0, float t0 = 0, float s1 = 1, float t1 = 1, float angle = 0, float r = 1, float g = 1, float b = 1, float a = 1, float z = 1) = 0;

    virtual void  Draw2dImageStretchMode(bool stretch) = 0;

    // Summary:
    //  Adds a 2d image that should be drawn on the screen to an internal render list. The list can be drawn with Draw2dImageList.
    //  If several images will be drawn, using this function is more efficient than calling Draw2dImage as it allows better batching.
    //  The function supports placing images in stereo 3d space.
    // Arguments:
    //      stereoDepth - Places image in stereo 3d space. The depth is specified in camera space, the stereo params are the same that
    //                                  are used for the scene. A value of 0 is handled as a special case and always places the image on the screen plane.
    virtual void  Push2dImage(float xpos, float ypos, float w, float h, int texture_id, float s0 = 0, float t0 = 0, float s1 = 1, float t1 = 1, float angle = 0, float r = 1, float g = 1, float b = 1, float a = 1, float z = 1, float stereoDepth = 0) = 0;

    // Summary:
    //  Draws all images to the screen that were collected with Push2dImage.
    virtual void  Draw2dImageList() = 0;

    // Summary:
    //  Draws a image using the current matrix.
    virtual void DrawImage(float xpos, float ypos, float w, float h, int texture_id, float s0, float t0, float s1, float t1, float r, float g, float b, float a, bool filtered = true) = 0;

    // Description:
    //  Draws a image using the current matrix, more flexible than DrawImage
    //  order for s and t: 0=left_top, 1=right_top, 2=right_bottom, 3=left_bottom.
    virtual void DrawImageWithUV(float xpos, float ypos, float z, float width, float height, int texture_id, float* s, float* t, float r = 1, float g = 1, float b = 1, float a = 1, bool filtered = true) = 0;

    // Summary:
    //  Sets the polygon mode with Push, Pop restores the last used one
    // Example:
    //  Wireframe, solid.
    virtual void PushWireframeMode(int mode) = 0;
    virtual void PopWireframeMode() = 0;

    // Summary:
    //  Gets height of the main rendering resolution.
    virtual int   GetHeight() const = 0;

    // Summary:
    //  Gets width of the main rendering resolution.
    virtual int   GetWidth() const = 0;

    // Summary:
    //  Gets Pixel Aspect Ratio.
    virtual float GetPixelAspectRatio() const = 0;

    // Summary:
    //  Gets the height of the overlay viewport where UI and debug output are rendered.
    virtual int   GetOverlayHeight() const = 0;

    // Summary:
    //  Gets the width of the overlay viewport where UI and debug output are rendered.
    virtual int   GetOverlayWidth() const = 0;

    // Summary:
    //  Gets the maximum dimension for a square custom render resolution.
    virtual int   GetMaxSquareRasterDimension() const = 0;

    // Summary:
    //  Switches subsequent rendering from the internal backbuffer to the native resolution backbuffer if available.
    virtual void  SwitchToNativeResolutionBackbuffer() = 0;

    // Summary:
    //  Gets memory status information
    virtual void GetMemoryUsage(ICrySizer* Sizer) = 0;

    // Summary:
    //  Gets textures streaming bandwidth information
    virtual void GetBandwidthStats(float* fBandwidthRequested) = 0;

    // Summary:
    //  Sets an event listener for texture streaming updates
    virtual void SetTextureStreamListener(ITextureStreamListener* pListener) = 0;

    // Summary:
    //  Populates a CPU-side occlusion buffer with the contents from the previous frame's downsampled depth buffer.
    //  This will be called from a job thread within the occlusion system.
    virtual int GetOcclusionBuffer(uint16* pOutOcclBuffer, Matrix44* pmCamBuffer) = 0;

    // Summary:
    //   Gets a screenshot and save to a file
    // Returns:
    //   true=success
    virtual bool ScreenShot(const char* filename = NULL, int width = 0) = 0;

    // Summary:
    //  Gets current bpp.
    virtual int GetColorBpp() = 0;

    // Summary:
    //  Gets current z-buffer depth.
    virtual int GetDepthBpp() = 0;

    // Summary:
    //  Gets current stencil bits.
    virtual int GetStencilBpp() = 0;

    // Summary:
    //  Returns true if stereo rendering is enabled.
    virtual bool IsStereoEnabled() const = 0;

    // Summary:
    //  Returns values of nearest rendering z-range max
    virtual float GetNearestRangeMax() const = 0;

    // Summary:
    //  Returns the PerInstanceConstantBufferPool
    virtual PerInstanceConstantBufferPool* GetPerInstanceConstantBufferPoolPointer() = 0;

    // Summary:
    //  Projects to screen.
    //  Returns true if successful.
    virtual bool ProjectToScreen(
        float ptx, float pty, float ptz,
        float* sx, float* sy, float* sz) = 0;

    // Summary:
    //  Unprojects to screen.
    virtual int UnProject(
        float sx, float sy, float sz,
        float* px, float* py, float* pz,
        const float modelMatrix[16],
        const float projMatrix[16],
        const int    viewport[4]) = 0;

    // Summary:
    //  Unprojects from screen.
    virtual int UnProjectFromScreen(
        float  sx, float  sy, float  sz,
        float* px, float* py, float* pz) = 0;

    // Remarks:
    //  For editor.
    virtual void  GetModelViewMatrix(float* mat) = 0;

    // Remarks:
    //  For editor.
    virtual void  GetProjectionMatrix(float* mat) = 0;

    virtual bool WriteDDS(const byte* dat, int wdt, int hgt, int Size, const char* name, ETEX_Format eF, int NumMips) = 0;
    virtual bool WriteTGA(const byte* dat, int wdt, int hgt, const char* name, int src_bits_per_pixel, int dest_bits_per_pixel) = 0;
    virtual bool WriteJPG(const byte* dat, int wdt, int hgt, char* name, int src_bits_per_pixel, int nQuality = 100) = 0;

    /////////////////////////////////////////////////////////////////////////////////
    //Replacement functions for Font

    static const bool FontCreateTextureGenMipsDefaultValue = false;
    virtual int  FontCreateTexture(int Width, int Height, byte* pData, ETEX_Format eTF = eTF_R8G8B8A8, bool genMips = FontCreateTextureGenMipsDefaultValue, const char* textureName = nullptr) = 0;
    virtual bool FontUpdateTexture(int nTexId, int X, int Y, int USize, int VSize, byte* pData) = 0;
    virtual void FontSetTexture(int nTexId, int nFilterMode) = 0;
    virtual void FontSetRenderingState(bool overrideViewProjMatrices, TransformationMatrices& backupMatrices) = 0;
    virtual void FontSetBlending(int src, int dst, int baseState) = 0;
    virtual void FontRestoreRenderingState(bool overrideViewProjMatrices, const TransformationMatrices& restoringMatrices) = 0;

    virtual bool FlushRTCommands(bool bWait, bool bImmediatelly, bool bForce) = 0;
    virtual void DrawStringU(IFFont_RenderProxy* pFont, float x, float y, float z, const char* pStr, bool asciiMultiLine, const STextDrawContext& ctx) const = 0;

    virtual int  RT_CurThreadList() = 0;

    /////////////////////////////////////////////////////////////////////////////////
    // External interface for shaders
    /////////////////////////////////////////////////////////////////////////////////
    virtual bool EF_PrecacheResource(SShaderItem* pSI, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId, int nCounter = 1) = 0;
    virtual bool EF_PrecacheResource(IShader* pSH, float fMipFactor, float fTimeToReady, int Flags) = 0;
    virtual bool EF_PrecacheResource(ITexture* pTP, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId, int nCounter = 1) = 0;
    virtual bool EF_PrecacheResource(IRenderMesh* pPB, _smart_ptr<IMaterial> pMaterial, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId) = 0;
    virtual bool EF_PrecacheResource(CDLight* pLS, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId) = 0;

    virtual ITexture* EF_CreateCompositeTexture(int type, const char* szName, int nWidth, int nHeight, int nDepth, int nMips, int nFlags, ETEX_Format eTF, const STexComposition* pCompositions, size_t nCompositions, int8 nPriority = -1) = 0;

    virtual void PostLevelLoading() = 0;
    virtual void PostLevelUnload() = 0;

    virtual CRenderObject* EF_AddPolygonToScene(SShaderItem& si, int numPts, const SVF_P3F_C4B_T2F* verts, const SPipTangents* tangs, CRenderObject* obj, const SRenderingPassInfo& passInfo, uint16* inds, int ninds, int nAW, const SRendItemSorter& rendItemSorter) = 0;
    virtual CRenderObject* EF_AddPolygonToScene(SShaderItem& si, CRenderObject* obj, const SRenderingPassInfo& passInfo, int numPts, int ninds, SVF_P3F_C4B_T2F*& verts, SPipTangents*& tangs, uint16*& inds, int nAW, const SRendItemSorter& rendItemSorter) = 0;

    // This is a workaround for when an editor viewport needs to do immediate rendering
    // in the editor. Specifically, global constants are updated in a deferred fashion, so
    // if a viewport (like the lens flare view) starts doing main-thread rendering, those
    // parameters are not bound.
    virtual void ForceUpdateGlobalShaderParameters() {}

    /////////////////////////////////////////////////////////////////////////////////
    // Shaders/Shaders management /////////////////////////////////////////////////////////////////////////////////

    virtual const char* EF_GetShaderMissLogPath() = 0;

    /////////////////////////////////////////////////////////////////////////////////
    virtual string* EF_GetShaderNames(int& nNumShaders) = 0;
    // Summary:
    //  Reloads file
    virtual bool          EF_ReloadFile (const char* szFileName) = 0;
    // Summary:
    //  Reloads file at any time the renderer feels to do so (no guarantees, but likely on next frame update)
    //  Is threadsafe
    virtual bool          EF_ReloadFile_Request (const char* szFileName) = 0;

    // Summary:
    //      Remaps shader gen mask to common global mask.
    virtual uint64      EF_GetRemapedShaderMaskGen(const char* name, uint64 nMaskGen = 0, bool bFixup = 0) = 0;

    virtual uint64      EF_GetShaderGlobalMaskGenFromString(const char* szShaderName, const char* szShaderGen, uint64 nMaskGen = 0) = 0;
    virtual AZStd::string EF_GetStringFromShaderGlobalMaskGen(const char* szShaderName, uint64 nMaskGen = 0) = 0;

    virtual const SShaderProfile& GetShaderProfile(EShaderType eST) const = 0;
    virtual void          EF_SetShaderQuality(EShaderType eST, EShaderQuality eSQ) = 0;

    // Summary:
    //  Gets renderer quality.
    virtual ERenderQuality EF_GetRenderQuality() const = 0;
    // Summary:
    //  Gets shader type quality.
    virtual EShaderQuality EF_GetShaderQuality(EShaderType eST) = 0;
    // Summary:
    //  Loads shader item for name (name).
    virtual SShaderItem   EF_LoadShaderItem (const char* szName, bool bShare, int flags = 0, SInputShaderResources* Res = NULL, uint64 nMaskGen = 0) = 0;
    // Summary:
    //  Loads shader for name (name).
    virtual IShader* EF_LoadShader (const char* name, int flags = 0, uint64 nMaskGen = 0) = 0;
    // Summary:
    //  Reinitializes all shader files (build hash tables).
    virtual void          EF_ReloadShaderFiles (int nCategory) = 0;
    // Summary:
    //  Reloads all texture files.
    virtual void          EF_ReloadTextures () = 0;
    // Summary:
    //  Gets texture object by ID.
    virtual ITexture* EF_GetTextureByID(int Id) = 0;
    // Summary:
    //  Gets texture object by Name.
    virtual ITexture* EF_GetTextureByName(const char* name, uint32 flags = 0) = 0;
    // Summary:
    //  Loads the texture for name(nameTex).
    virtual ITexture* EF_LoadTexture(const char* nameTex, uint32 flags = 0) = 0;
    virtual ITexture* EF_LoadCubemapTexture(const char* nameTex, uint32 flags = 0) = 0;
    // Summary:
    //  Loads default texture whose life cycle is managed by Texture Manager, do not try to release them by yourself!
    virtual ITexture* EF_LoadDefaultTexture(const char* nameTex) = 0;

    // Summary:
    //  Loads lightmap for name.
    virtual int           EF_LoadLightmap (const char* name) = 0;

    // Summary:
    //  Starts using of the shaders (return first index for allow recursions).
    virtual void EF_StartEf (const SRenderingPassInfo& passInfo) = 0;

    virtual SRenderObjData* EF_GetObjData(CRenderObject* pObj, bool bCreate, int nThreadID) = 0;

    // Summary:
    //  Gets CRenderObject for RE transformation.
    //Get temporary RenderObject
    virtual CRenderObject* EF_GetObject_Temp (int nThreadID) = 0;

    //Get permanent RenderObject
    virtual CRenderObject* EF_DuplicateRO(CRenderObject* pObj, const SRenderingPassInfo& passInfo) = 0;

    // Summary:
    //  Adds shader to the list.
    virtual void EF_AddEf (IRenderElement* pRE, SShaderItem& pSH, CRenderObject* pObj, const SRenderingPassInfo& passInfo, int nList, int nAW, const SRendItemSorter& rendItemSorter) = 0;

    //! Draw all shaded REs in the list
    virtual void EF_EndEf3D (int nFlags, int nPrecacheUpdateId, int nNearPrecacheUpdateId, const SRenderingPassInfo& passInfo) = 0;

    virtual void EF_InvokeShadowMapRenderJobs(int nFlags) = 0;

    // Dynamic lights
    void EF_ClearLightsList() {}; // For FC Compatibility.
    virtual bool EF_IsFakeDLight (const CDLight* Source) = 0;
    virtual void EF_ADDDlight(CDLight* Source, const SRenderingPassInfo& passInfo) = 0;
    virtual bool EF_UpdateDLight(SRenderLight* pDL) = 0;
    virtual bool EF_AddDeferredDecal([[maybe_unused]] const SDeferredDecal& rDecal){return true; }

    // Deferred lights/vis areas

    virtual int EF_AddDeferredLight(const CDLight& pLight, float fMult, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter) = 0;
    virtual uint32 EF_GetDeferredLightsNum(eDeferredLightType eLightType = eDLT_DeferredLight) = 0;
    virtual void EF_ClearDeferredLightsList() = 0;

    virtual uint8 EF_AddDeferredClipVolume(const IClipVolume* pClipVolume) = 0;
    virtual bool EF_SetDeferredClipVolumeBlendData(const IClipVolume* pClipVolume, const SClipVolumeBlendInfo& blendInfo) = 0;
    virtual void EF_ClearDeferredClipVolumesList() = 0;

    // called in between levels to free up memory
    virtual void EF_ReleaseDeferredData() = 0;

    // called in between levels to free up memory
    virtual void EF_ReleaseInputShaderResource(SInputShaderResources* pRes) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Post processing effects interfaces

    virtual void EF_SetPostEffectParam(const char* pParam, float fValue, bool bForceValue = false) = 0;
    virtual void EF_SetPostEffectParamVec4(const char* pParam, const Vec4& pValue, bool bForceValue = false) = 0;
    virtual void EF_SetPostEffectParamString(const char* pParam, const char* pszArg) = 0;

    virtual void EF_GetPostEffectParam(const char* pParam, float& fValue) = 0;
    virtual void EF_GetPostEffectParamVec4(const char* pParam, Vec4& pValue) = 0;
    virtual void EF_GetPostEffectParamString(const char* pParam, const char*& pszArg) = 0;

    virtual int32 EF_GetPostEffectID(const char* pPostEffectName) = 0;

    virtual void EF_ResetPostEffects(bool bOnSpecChange = false) = 0;

    virtual void SyncPostEffects() = 0;

    virtual void EF_DisableTemporalEffects() = 0;

    //////////////////////////////////////////////////////////////////////////

    virtual void EF_AddWaterSimHit(const Vec3& vPos, float scale, float strength) = 0;
    virtual void EF_DrawWaterSimHits() = 0;

    /////////////////////////////////////////////////////////////////////////////////
    // 2d interface for the shaders
    /////////////////////////////////////////////////////////////////////////////////
    virtual void EF_EndEf2D(bool bSort) = 0;

    // Summary:
    //   Returns various Renderer Settings, see ERenderQueryTypes.
    // Arguments:
    //   Query - e.g. EFQ_GetShaderCombinations.
    //   rInOut - Input/Output Parameter, depends on the query if written to/read from, or both
    void EF_Query(ERenderQueryTypes eQuery)
    {
        EF_QueryImpl(eQuery, NULL, 0, NULL, 0);
    }
    template<typename T>
    void EF_Query(ERenderQueryTypes eQuery, T& rInOut)
    {
        EF_QueryImpl(eQuery, static_cast<void*>(&rInOut), sizeof(T), NULL, 0);
    }
    template<typename T0, typename T1>
    void EF_Query(ERenderQueryTypes eQuery, T0& rInOut0, T1& rInOut1)
    {
        EF_QueryImpl(eQuery, static_cast<void*>(&rInOut0), sizeof(T0), static_cast<void*>(&rInOut1), sizeof(T1));
    }

    // Summary:
    //   Toggles render mesh garbage collection
    // Arguments:
    //   Param -
    virtual void ForceGC() = 0;

    // Remarks:
    //  For stats.
    virtual int  GetPolyCount() const = 0;
    virtual void GetPolyCount(int& nPolygons, int& nShadowVolPolys) const = 0;

    // Note:
    //  3d engine set this color to fog color.
    virtual void SetClearColor(const Vec3& vColor) = 0;
  
    virtual void SetClearBackground(bool bClearBackground) = 0;

    // Summary:
    //  Creates/deletes RenderMesh object.
    virtual _smart_ptr<IRenderMesh> CreateRenderMesh(
        const char* szType
        , const char* szSourceName
        , IRenderMesh::SInitParamerers* pInitParams = NULL
        , ERenderMeshType eBufType = eRMT_Static
        ) = 0;

    virtual _smart_ptr<IRenderMesh> CreateRenderMeshInitialized(
        const void* pVertBuffer, int nVertCount, const AZ::Vertex::Format& vertexFormat,
        const vtx_idx* pIndices, int nIndices,
        const PublicRenderPrimitiveType nPrimetiveType, const char* szType, const char* szSourceName, ERenderMeshType eBufType = eRMT_Static,
        int nMatInfoCount = 1, int nClientTextureBindID = 0,
        bool (* PrepareBufferCallback)(IRenderMesh*, bool) = NULL,
        void* CustomData = NULL,
        bool bOnlyVideoBuffer = false,
        bool bPrecache = true,
        const SPipTangents* pTangents = NULL, bool bLockForThreadAcc = false, Vec3* pNormals = NULL) = 0;

    //Pass false to get a frameID that increments by one each frame. For this case the increment happens in the game thread at the beginning of the frame.
    virtual int GetFrameID(bool bIncludeRecursiveCalls = true) = 0;

    virtual void MakeMatrix(const Vec3& pos, const Vec3& angles, const Vec3& scale, Matrix34* mat) = 0;

    // Description:
    //   Draws text queued.
    // Note:
    //   Position can be in 3d or in 2d depending on the flags.
    virtual void DrawTextQueued(Vec3 pos, SDrawTextInfo& ti, const char* format, va_list args) = 0;

    // Description:
    //   Draws text queued.
    // Note:
    //   Position can be in 3d or in 2d depending on the flags.
    virtual void DrawTextQueued(Vec3 pos, SDrawTextInfo& ti, const char* text) = 0;

    //////////////////////////////////////////////////////////////////////

    virtual float ScaleCoordX(float value) const = 0;
    virtual float ScaleCoordY(float value) const = 0;
    virtual void ScaleCoord(float& x, float& y) const = 0;

    virtual void SetState(int State, int AlphaRef = -1) = 0;
    virtual void SetCullMode  (int mode = R_CULL_BACK) = 0;
    virtual void SetStencilState(int st, uint32 nStencRef, uint32 nStencMask, uint32 nStencWriteMask, bool bForceFullReadMask = false) = 0;

    virtual void PushProfileMarker(const char* label) = 0;
    virtual void PopProfileMarker(const char* label) = 0;

    virtual bool EnableFog(bool enable) = 0;
    virtual void SetFogColor(const ColorF& color) = 0;

    virtual void SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa) = 0;
    virtual void SetSrgbWrite(bool srgbWrite) = 0;

    // for one frame allows to disable limit of texture streaming requests
    virtual void RequestFlushAllPendingTextureStreamingJobs([[maybe_unused]] int nFrames) { }

    // allows to dynamically adjust texture streaming load depending on game conditions
    virtual void SetTexturesStreamingGlobalMipFactor([[maybe_unused]] float fFactor) { }

    //////////////////////////////////////////////////////////////////////
    // Summary:
    //  Interface for auxiliary geometry (for debugging, editor purposes, etc.)
    virtual IRenderAuxGeom* GetIRenderAuxGeom(void* jobID = 0) = 0;
    //////////////////////////////////////////////////////////////////////

    //  Interface for renderer side SVO
    virtual ISvoRenderer* GetISvoRenderer() { return 0; }

    virtual IColorGradingController* GetIColorGradingController() = 0;
    virtual IStereoRenderer* GetIStereoRenderer() = 0;

    virtual ITexture* Create2DTexture(const char* name, int width, int height, int numMips, int flags, unsigned char* data, ETEX_Format format) = 0;
    virtual void TextToScreen(float x, float y, const char* format, ...) PRINTF_PARAMS(4, 5) = 0;
    virtual void TextToScreenColor(int x, int y, float r, float g, float b, float a, const char* format, ...) PRINTF_PARAMS(8, 9) = 0;
    virtual void ResetToDefault() = 0;
    virtual void SetMaterialColor(float r, float g, float b, float a) = 0;

    // Sets default Blend, DepthStencil and Raster states.
    virtual void SetDefaultRenderStates() = 0;

    virtual void Graph(byte* g, int x, int y, int wdt, int hgt, int nC, int type, const char* text, ColorF& color, float fScale) = 0;
    virtual void EF_RenderTextMessages() = 0;

    virtual void ClearTargetsImmediately(uint32 nFlags) = 0;
    virtual void ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors, float fDepth) = 0;
    virtual void ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors) = 0;
    virtual void ClearTargetsImmediately(uint32 nFlags, float fDepth) = 0;

    virtual void ClearTargetsLater(uint32 nFlags) = 0;
    virtual void ClearTargetsLater(uint32 nFlags, const ColorF& Colors, float fDepth) = 0;
    virtual void ClearTargetsLater(uint32 nFlags, const ColorF& Colors) = 0;
    virtual void ClearTargetsLater(uint32 nFlags, float fDepth) = 0;

    virtual void ReadFrameBuffer(unsigned char* pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX = -1, int nScaledY = -1) = 0;
    virtual void ReadFrameBufferFast(uint32* pDstARGBA8, int dstWidth, int dstHeight, bool BGRA = true) = 0;

    // Note:
    //  The following functions will be removed.
    virtual void EnableVSync(bool enable) = 0;

    virtual void CreateResourceAsync(SResourceAsync* Resource) = 0;
    virtual void ReleaseResourceAsync(SResourceAsync* Resource) = 0;
    virtual void ReleaseResourceAsync(AZStd::unique_ptr<SResourceAsync> pResource) = 0;
    virtual unsigned int DownLoadToVideoMemory(const byte* data, int w, int h, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat = true, int filter = FILTER_BILINEAR, int Id = 0, const char* szCacheName = NULL, int flags = 0, EEndian eEndian = eLittleEndian, RectI* pRegion = NULL, bool bAsynDevTexCreation = false) = 0;
    virtual unsigned int DownLoadToVideoMemory3D(const byte* data, int w, int h, int d, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat = true, int filter = FILTER_BILINEAR, int Id = 0, const char* szCacheName = NULL, int flags = 0, EEndian eEndian = eLittleEndian, RectI* pRegion = NULL, bool bAsynDevTexCreation = false) = 0;
    virtual unsigned int DownLoadToVideoMemoryCube(const byte* data, int w, int h, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat = true, int filter = FILTER_BILINEAR, int Id = 0, const char* szCacheName = NULL, int flags = 0, EEndian eEndian = eLittleEndian, RectI* pRegion = NULL, bool bAsynDevTexCreation = false) = 0;
    virtual void UpdateTextureInVideoMemory(uint32 tnum, const byte* newdata, int posx, int posy, int w, int h, ETEX_Format eTFSrc = eTF_B8G8R8, int posz = 0, int sizez = 1) = 0;

    virtual bool DXTCompress(const byte* raw_data, int nWidth, int nHeight, ETEX_Format eTF, bool bUseHW, bool bGenMips, int nSrcBytesPerPix, MIPDXTcallback callback) = 0;
    virtual bool DXTDecompress(const byte* srcData, size_t srcFileSize, byte* dstData, int nWidth, int nHeight, int nMips, ETEX_Format eSrcTF, bool bUseHW, int nDstBytesPerPix) = 0;
    virtual void RemoveTexture(unsigned int TextureId) = 0;
    virtual void DeleteFont(IFFont* font) = 0;

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
    virtual bool CaptureFrameBufferFast(unsigned char* pDstRGBA8, int destinationWidth, int destinationHeight) = 0;


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
    virtual bool CopyFrameBufferFast(unsigned char* pDstRGBA8, int destinationWidth, int destinationHeight) = 0;



    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // This routine registers a callback address that is called when a new frame is available
    // Inputs :
    //          pCapture            :   Address of the ICaptureFrameListener object
    //
    // Outputs : returns true if successful, otherwise false
    //
    virtual bool RegisterCaptureFrame(ICaptureFrameListener* pCapture) = 0;

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // This routine unregisters a callback address that was previously registered
    // Inputs :
    //          pCapture            :   Address of the ICaptureFrameListener object to unregister
    //
    // Outputs : returns true if successful, otherwise false
    //
    virtual bool UnRegisterCaptureFrame(ICaptureFrameListener* pCapture) = 0;


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
    virtual bool InitCaptureFrameBufferFast(uint32 bufferWidth = 0, uint32 bufferHeight = 0) = 0;

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // This routine releases the 2 surfaces used for frame capture by the CaptureFrameBufferFast routine
    //
    // Inputs : None
    //
    // Returns : None
    //
    virtual void CloseCaptureFrameBufferFast(void) = 0;


    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // This routine checks for any frame buffer callbacks that are needed and calls them
    //
    // Inputs : None
    //
    //  Outputs : None
    //
    virtual void CaptureFrameBufferCallBack(void) = 0;

    virtual void RegisterSyncWithMainListener(ISyncMainWithRenderListener* pListener) = 0;
    virtual void RemoveSyncWithMainListener(const ISyncMainWithRenderListener* pListener) = 0;

    virtual void Set2DMode(uint32 orthoX, uint32 orthoY, TransformationMatrices& backupMatrices, float znear = -1e10f, float zfar = 1e10f) = 0;
    virtual void Unset2DMode(const TransformationMatrices& restoringMatrices) = 0;
    virtual void Set2DModeNonZeroTopLeft(float orthoLeft, float orthoTop, float orthoWidth, float orthoHeight, TransformationMatrices& backupMatrices, float znear = -1e10f, float zfar = 1e10f) = 0;

    virtual int ScreenToTexture(int nTexID) = 0;
    virtual void EnableSwapBuffers(bool bEnable) = 0;
    virtual WIN_HWND GetHWND() = 0;

    // Set the window icon to be displayed on the output window.
    // The parameter is the path to a DDS texture file to be used as the icon.
    // For best results, pass a square power-of-two sized texture, with a mip-chain.
    virtual bool SetWindowIcon(const char* path) = 0;

    virtual void OnEntityDeleted(struct IRenderNode* pRenderNode) = 0;

    virtual int CreateRenderTarget(const char* name, int nWidth, int nHeight, const ColorF& clearColor, ETEX_Format eTF) = 0;
    virtual bool DestroyRenderTarget (int nHandle) = 0;
    virtual bool ResizeRenderTarget(int nHandle, int nWidth, int nHeight) = 0;
    virtual bool SetRenderTarget(int nHandle, SDepthTexture* pDepthSurf = nullptr) = 0;
    virtual SDepthTexture* CreateDepthSurface(int nWidth, int nHeight, bool shaderResourceView = false) = 0;
    virtual void DestroyDepthSurface(SDepthTexture* pDepthSurf) = 0;

    // Note:
    //  Used for pausing timer related stuff.
    // Example:
    //  For texture animations, and shader 'time' parameter.
    virtual void PauseTimer(bool bPause) = 0;

    // Description:
    //    Creates an Interface to the public params container.
    // Return:
    //    Created IShaderPublicParams interface.
    virtual IShaderPublicParams* CreateShaderPublicParams() = 0;

    virtual void GetThreadIDs(threadID& mainThreadID, threadID& renderThreadID) const = 0;

    struct SArtProfileData
    {
        enum EArtProfileUnit
        {
            eArtProfileUnit_GPU = 0,
            eArtProfileUnit_CPU,
            eArtProfile_NumUnits
        };

        enum EArtProfileSections
        {
            eArtProfile_Shadows = 0,
            eArtProfile_ZPass,
            eArtProfile_Decals,
            eArtProfile_Lighting,
            eArtProfile_Opaque,
            eArtProfile_Transparent,
            eArtProfile_Max,
        };

        float times[eArtProfile_Max];
        float budgets[eArtProfile_Max];
        float total, budgetTotal;

        // detailed values for anything that is grouped together and can be timed
        enum EBreakdownDetailValues
        {
            // Lighting
            eArtProfileDetail_LightsAmbient,
            eArtProfileDetail_LightsCubemaps,
            eArtProfileDetail_LightsDeferred,
            eArtProfileDetail_LightsShadowMaps, // just the cost of the shadow maps

            // Transparent
            eArtProfileDetail_Reflections,
            eArtProfileDetail_Caustics,
            eArtProfileDetail_RefractionOverhead, // partial resolves
            eArtProfileDetail_Rain,
            eArtProfileDetail_LensOptics,

            eArtProfileDetail_Max,
        };

        float breakdowns[eArtProfileDetail_Max];

        int batches, drawcalls, processedLights;

#if defined(ENABLE_ART_RT_TIME_ESTIMATE)
        int numStandardBatches;
        int numStandardDrawCalls;
        int numLightDrawCalls;
        float actualRenderTimeMinusPost;
        float actualRenderTimePost;
        float actualMiscRTTime;
        float actualTotalRTTime;
#endif
    };

    virtual void EnableGPUTimers2(bool bEnabled) = 0;
    virtual void AllowGPUTimers2(bool bAllow) = 0;
    virtual const RPProfilerStats* GetRPPStats(ERenderPipelineProfilerStats eStat, bool bCalledFromMainThread = true) const = 0;
    virtual const RPProfilerStats* GetRPPStatsArray(bool bCalledFromMainThread = true) const = 0;

    virtual int GetPolygonCountByType(uint32 EFSList, EVertexCostTypes vct, uint32 z, bool bCalledFromMainThread = true) = 0;

    virtual void SetCloudShadowsParams(int nTexID, const Vec3& speed, float tiling, bool invert, float brightness) = 0;
    virtual uint16 PushFogVolumeContribution(const SFogVolumeData& fogVolData, const SRenderingPassInfo& passInfo) = 0;
    virtual void PushFogVolume(class CREFogVolume* pFogVolume, const SRenderingPassInfo& passInfo) = 0;

    virtual int GetMaxTextureSize() = 0;

    virtual const char* GetTextureFormatName(ETEX_Format eTF) = 0;
    virtual int GetTextureFormatDataSize(int nWidth, int nHeight, int nDepth, int nMips, ETEX_Format eTF) = 0;

    virtual void SetDefaultMaterials(_smart_ptr<IMaterial> pDefMat, _smart_ptr<IMaterial> pTerrainDefMat) = 0;

    virtual IGPUParticleEngine* GetGPUParticleEngine() const { return 0; }

    virtual uint32 GetActiveGPUCount() const = 0;
    virtual ShadowFrustumMGPUCache* GetShadowFrustumMGPUCache() = 0;
    virtual const StaticArray<int, MAX_GSM_LODS_NUM>& GetCachedShadowsResolution() const = 0;
    virtual void SetCachedShadowsResolution(const StaticArray<int, MAX_GSM_LODS_NUM>& arrResolutions) = 0;
    virtual void UpdateCachedShadowsLodCount(int nGsmLods) const = 0;

    virtual void SetTexturePrecaching(bool stat) = 0;

    //platform specific
    virtual void    RT_InsertGpuCallback(uint32 context, GpuCallbackFunc callback) = 0;
    virtual void EnablePipelineProfiler(bool bEnable) = 0;

    struct SRenderTimes
    {
        float fWaitForMain;
        float fWaitForRender;
        float fWaitForGPU;
        float fTimeProcessedRT;
        float fTimeProcessedRTScene;    //The part of the render thread between the "SCENE" profiler labels
        float fTimeProcessedGPU;
        float fTimeGPUIdlePercent;
    };
    virtual void GetRenderTimes(SRenderTimes& outTimes) = 0;
    virtual float GetGPUFrameTime() = 0;

    // Enable the batch mode if the meshpools are used to enable quick and dirty flushes.
    virtual void EnableBatchMode(bool enable) = 0;
    // Flag level unloading in progress to disable f.i. rendermesh creation requests
    virtual void EnableLevelUnloading(bool enable) = 0;
    // Function to handle cleanup required if a level load fails
    virtual void OnLevelLoadFailed() = 0;

    struct SDrawCallCountInfo
    {
        static const uint32 MESH_NAME_LENGTH = 32;
        static const uint32 TYPE_NAME_LENGTH = 16;

        SDrawCallCountInfo()
            : pPos(0, 0, 0)
            , nZpass(0)
            , nShadows(0)
            , nGeneral(0)
            , nTransparent(0)
            , nMisc(0)
        {
            meshName[0] = '\0';
            typeName[0] = '\0';
        }

        void Update(CRenderObject* pObj, IRenderMesh* pRM);

        Vec3 pPos;
        uint8 nZpass, nShadows, nGeneral, nTransparent, nMisc;
        char meshName[MESH_NAME_LENGTH];
        char typeName[TYPE_NAME_LENGTH];
    };

    //Debug draw call info (per node)
    typedef AZStd::unordered_map< IRenderNode*, IRenderer::SDrawCallCountInfo, AZStd::hash<IRenderNode*>, AZStd::equal_to<IRenderNode*>, AZ::StdLegacyAllocator > RNDrawcallsMapNode;
    typedef RNDrawcallsMapNode::iterator RNDrawcallsMapNodeItor;

    //Debug draw call info (per mesh)
    typedef AZStd::unordered_map< IRenderMesh*, IRenderer::SDrawCallCountInfo, AZStd::hash<IRenderMesh*>, AZStd::equal_to<IRenderMesh*>, AZ::StdLegacyAllocator > RNDrawcallsMapMesh;
    typedef RNDrawcallsMapMesh::iterator RNDrawcallsMapMeshItor;

#if !defined(_RELEASE)
    //Get draw call info for frame
    virtual RNDrawcallsMapMesh& GetDrawCallsInfoPerMesh(bool mainThread = true) = 0;
    virtual RNDrawcallsMapMesh& GetDrawCallsInfoPerMeshPreviousFrame(bool mainThread = true) = 0;
    virtual RNDrawcallsMapNode& GetDrawCallsInfoPerNodePreviousFrame(bool mainThread = true) = 0;
    virtual int GetDrawCallsPerNode(IRenderNode* pRenderNode) = 0;
    virtual void ForceRemoveNodeFromDrawCallsMap(IRenderNode* pNode) = 0;
#endif

    virtual void CollectDrawCallsInfo(bool status) = 0;
    virtual void CollectDrawCallsInfoPerNode(bool status) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Summary:
    //   Helper functions to draw text.
    //////////////////////////////////////////////////////////////////////////
    void DrawLabel(Vec3 pos, float font_size, const char* label_text, ...) PRINTF_PARAMS(4, 5)
    {
        va_list args;
        va_start(args, label_text);
        SDrawTextInfo ti;
        ti.xscale = ti.yscale = font_size;
        ti.flags = eDrawText_FixedSize | eDrawText_800x600;
        DrawTextQueued(pos, ti, label_text, args);
        va_end(args);
    }

    void DrawLabelEx(Vec3 pos, float font_size, const float* pfColor, bool bFixedSize, bool bCenter, const char* label_text, ...) PRINTF_PARAMS(7, 8)
    {
        va_list args;
        va_start(args, label_text);
        SDrawTextInfo ti;
        ti.xscale = ti.yscale = font_size;
        ti.flags = ((bFixedSize) ? eDrawText_FixedSize : 0) | ((bCenter) ? eDrawText_Center : 0) | eDrawText_800x600;
        if (pfColor)
        {
            ti.color[0] = pfColor[0];
            ti.color[1] = pfColor[1];
            ti.color[2] = pfColor[2];
            ti.color[3] = pfColor[3];
        }
        DrawTextQueued(pos, ti, label_text, args);
        va_end(args);
    }

    void Draw2dLabelEx(float x, float y, float font_size, const ColorF& fColor, EDrawTextFlags flags, const char* label_text, ...) PRINTF_PARAMS(7, 8)
    {
        va_list args;
        va_start(args, label_text);
        SDrawTextInfo ti;
        ti.xscale = ti.yscale = font_size;
        ti.flags = flags;
        {
            ti.color[0] = fColor[0];
            ti.color[1] = fColor[1];
            ti.color[2] = fColor[2];
            ti.color[3] = fColor[3];
        }
        DrawTextQueued(Vec3(x, y, 0.5f), ti, label_text, args);
        va_end(args);
    }

    void Draw2dLabel(float x, float y, float font_size, const float* pfColor, bool bCenter, const char* label_text, ...) PRINTF_PARAMS(7, 8)
    {
        va_list args;
        va_start(args, label_text);
        SDrawTextInfo ti;
        ti.xscale = ti.yscale = font_size;
        ti.flags = eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize | ((bCenter) ? eDrawText_Center : 0);
        if (pfColor)
        {
            ti.color[0] = pfColor[0];
            ti.color[1] = pfColor[1];
            ti.color[2] = pfColor[2];
            ti.color[3] = pfColor[3];
        }
        DrawTextQueued(Vec3(x, y, 0.5f), ti, label_text, args);
        va_end(args);
    }

    void Draw2dLabel(float x, float y, float font_size, const ColorF& fColor, bool bCenter, const char* label_text, ...) PRINTF_PARAMS(7, 8)
    {
        va_list args;
        va_start(args, label_text);
        SDrawTextInfo ti;
        ti.xscale = ti.yscale = font_size;
        ti.flags = eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize | ((bCenter) ? eDrawText_Center : 0);
        {
            ti.color[0] = fColor[0];
            ti.color[1] = fColor[1];
            ti.color[2] = fColor[2];
            ti.color[3] = fColor[3];
        }
        DrawTextQueued(Vec3(x, y, 0.5f), ti, label_text, args);
        va_end(args);
    }

    // BLM - Added override that takes flags manually, so we can draw monospaced, etc.
    void Draw2dLabelWithFlags(float x, float y, float font_size, const ColorF& fColor, uint32 flags, const char* label_text, ...) PRINTF_PARAMS(7, 8)
    {
        va_list args;
        va_start(args, label_text);
        SDrawTextInfo ti;
        ti.xscale = ti.yscale = font_size;
        ti.flags = flags;
        {
            ti.color[0] = fColor[0];
            ti.color[1] = fColor[1];
            ti.color[2] = fColor[2];
            ti.color[3] = fColor[3];
        }
        DrawTextQueued(Vec3(x, y, 0.5f), ti, label_text, args);
        va_end(args);
    }

    /**
    * Used to determine if the renderer has loaded default system
    * textures yet.
    *
    * Some textures like s_ptexWhite aren't available until this is true.
    *
    * @return True if the renderer has loaded default resources
    */
    virtual bool HasLoadedDefaultResources() { return false; }

    // Summary:
    virtual SSkinningData* EF_CreateSkinningData(uint32 nNumBones, bool bNeedJobSyncVar, bool bUseMatrixSkinning = false) = 0;
    virtual SSkinningData* EF_CreateRemappedSkinningData(uint32 nNumBones, SSkinningData* pSourceSkinningData, uint32 nCustomDataSize, uint32 pairGuid) = 0;
    virtual void EF_ClearSkinningDataPool() = 0;
    virtual int EF_GetSkinningPoolID() = 0;

    virtual void ClearShaderItem(SShaderItem* pShaderItem) = 0;
    virtual void UpdateShaderItem(SShaderItem* pShaderItem, _smart_ptr<IMaterial> pMaterial) = 0;
    virtual void ForceUpdateShaderItem(SShaderItem* pShaderItem, _smart_ptr<IMaterial> pMaterial) = 0;
    virtual void RefreshShaderResourceConstants(SShaderItem* pShaderItem, IMaterial* pMaterial) = 0;

    // Summary:
    //  Determine if a switch to stereo mode will occur at the start of the next frame
    virtual bool IsStereoModeChangePending() = 0;

    // Summary:
    // Lock/Unlock the video memory buffer used by particles when using the jobsystem
    virtual void LockParticleVideoMemory(uint32 nId) = 0;
    virtual void UnLockParticleVideoMemory(uint32 nId) = 0;

    // Summary:
    // tell the renderer that we will begin/stop spawning jobs which generate SRendItems
    virtual void BeginSpawningGeneratingRendItemJobs(int nThreadID) = 0;
    virtual void BeginSpawningShadowGeneratingRendItemJobs(int nThreadID) = 0;
    virtual void EndSpawningGeneratingRendItemJobs() = 0;

    virtual void StartLoadtimePlayback(ILoadtimeCallback* pCallback) = 0;
    virtual void StopLoadtimePlayback() = 0;

    // Summary:
    // get the shared job state for SRendItem Generating jobs
    virtual AZ::LegacyJobExecutor* GetGenerateRendItemJobExecutor() = 0;
    virtual AZ::LegacyJobExecutor* GetGenerateShadowRendItemJobExecutor() = 0;
    virtual AZ::LegacyJobExecutor* GetGenerateRendItemJobExecutorPreProcess() = 0;
    virtual AZ::LegacyJobExecutor* GetFinalizeRendItemJobExecutor(int nThreadID) = 0;
    virtual AZ::LegacyJobExecutor* GetFinalizeShadowRendItemJobExecutor(int nThreadID) = 0;

    virtual void FlushPendingTextureTasks() = 0;

    virtual void SetShadowJittering(float fShadowJittering) = 0;
    virtual float GetShadowJittering() const = 0;

    virtual bool LoadShaderStartupCache() = 0;
    virtual void UnloadShaderStartupCache() = 0;

    virtual bool LoadShaderLevelCache() = 0;
    virtual void UnloadShaderLevelCache() = 0;

    virtual void StartScreenShot([[maybe_unused]] int e_ScreenShot) {};
    virtual void EndScreenShot([[maybe_unused]] int e_ScreenShot) {};

    // Sets a renderer tracked cvar
    virtual void SetRendererCVar(ICVar* pCVar, const char* pArgText, bool bSilentMode = false) = 0;

    // Get the render piepline
    virtual SRenderPipeline* GetRenderPipeline() = 0;

    // Get the sahder manager
    virtual CShaderMan* GetShaderManager() = 0;

    // Get render thread
    virtual SRenderThread* GetRenderThread() = 0;

    // Get premade white texture
    virtual ITexture* GetWhiteTexture() = 0;

    // Get the texture for the name and format given
    virtual ITexture* GetTextureForName(const char* name, uint32 nFlags, ETEX_Format eFormat) = 0;

    // Get the camera view parameters
    virtual const CameraViewParameters& GetViewParameters() = 0;

    // Get frame reset number
    virtual uint32 GetFrameReset() = 0;

    // Get original depth buffer
    virtual SDepthTexture* GetDepthBufferOrig() = 0;

    // Get width of backbuffer
    virtual uint32 GetBackBufferWidth() = 0;

    // Get height of backbuffer
    virtual uint32 GetBackBufferHeight() = 0;

    // Get the device buffer manager
    virtual CDeviceBufferManager* GetDeviceBufferManager() = 0;

    // Get render tile info
    virtual const SRenderTileInfo* GetRenderTileInfo() const = 0;

    // Returns precomputed identity matrix
    virtual Matrix44A GetIdentityMatrix() = 0;

    // Get current GPU group Id. Used for tracking which GPU is being used
    virtual int32 RT_GetCurrGpuID() const = 0;

    // Generate the next texture id
    virtual int GenerateTextureId() = 0;

    // Set culling mode
    virtual void SetCull(ECull eCull, bool bSkipMirrorCull = false) = 0;

    // Draw a 2D quad
    virtual void DrawQuad(float x0, float y0, float x1, float y1, const ColorF& color, float z = 1.0f, float s0 = 0.0f, float t0 = 0.0f, float s1 = 1.0f, float t1 = 1.0f) = 0;

    // Draw a quad
    virtual void DrawQuad3D(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3, const ColorF& color, float ftx0, float fty0, float ftx1, float fty1) = 0;

    // Resets render pipeline state
    virtual void FX_ResetPipe() = 0;

    // Gets an (existing) depth surface of the dimensions given
    virtual SDepthTexture* FX_GetDepthSurface(int nWidth, int nHeight, bool bAA, bool shaderResourceView = false) = 0;

    // Check to see if buffers are full and if so flush
    virtual void FX_CheckOverflow(int nVerts, int nInds, IRenderElement* re, int* nNewVerts = nullptr, int* nNewInds = nullptr) = 0;

    // Perform pre render work
    virtual void FX_PreRender(int Stage) = 0;

    // Perform post render work
    virtual void FX_PostRender() = 0;

    // Set render states
    virtual void FX_SetState(int st, int AlphaRef = -1, int RestoreState = 0) = 0;

    // Commit render states
    virtual void FX_CommitStates(const SShaderTechnique* pTech, const SShaderPass* pPass, bool bUseMaterialState) = 0;

    // Commit changes made thus dar
    virtual void FX_Commit(bool bAllowDIP = false) = 0;

    // Sets vertex declaration
    virtual long FX_SetVertexDeclaration(int StreamMask, const AZ::Vertex::Format& vertexFormat) = 0;

    // Draw indexed prim
    virtual void FX_DrawIndexedPrimitive(eRenderPrimitiveType eType, int nVBOffset, int nMinVertexIndex, int nVerticesCount, int nStartIndex, int nNumIndices, bool bInstanced = false) = 0;

    // Set Index stream
    virtual long FX_SetIStream(const void* pB, uint32 nOffs, RenderIndexType idxType) = 0;

    // Set vertex stream
    virtual long FX_SetVStream(int nID, const void* pB, uint32 nOffs, uint32 nStride, uint32 nFreq = 1) = 0;

    // Draw primitives
    virtual void FX_DrawPrimitive(eRenderPrimitiveType eType, int nStartVertex, int nVerticesCount, int nInstanceVertices = 0) = 0;

    // Clear texture
    virtual void FX_ClearTarget(ITexture* pTex) = 0;

    // Clear depth
    virtual void FX_ClearTarget(SDepthTexture* pTex) = 0;

    // Set render target
    virtual bool FX_SetRenderTarget(int nTarget, void* pTargetSurf, SDepthTexture* pDepthTarget, uint32 nTileCount = 1) = 0;

    // Pushes render target
    virtual bool FX_PushRenderTarget(int nTarget, void* pTargetSurf, SDepthTexture* pDepthTarget, uint32 nTileCount = 1) = 0;

    // Sets up the render target
    virtual bool FX_SetRenderTarget(int nTarget, CTexture* pTarget, SDepthTexture* pDepthTarget, bool bPush = false, int nCMSide = -1, bool bScreenVP = false, uint32 nTileCount = 1) = 0;

    // Push render target
    virtual bool FX_PushRenderTarget(int nTarget, CTexture* pTarget, SDepthTexture* pDepthTarget, int nCMSide = -1, bool bScreenVP = false, uint32 nTileCount = 1) = 0;

    // Restore render target
    virtual bool FX_RestoreRenderTarget(int nTarget) = 0;

    // Pop render target
    virtual bool FX_PopRenderTarget(int nTarget) = 0;

    // Set active render targets
    virtual void FX_SetActiveRenderTargets(bool bAllowDIP = false) = 0;

    // Start an effect / shader / etc..
    virtual void FX_Start(CShader* ef, int nTech, CShaderResources* Res, IRenderElement* re) = 0;

    // Pop render target on render thread
    virtual void RT_PopRenderTarget(int nTarget) = 0;

    // Sets viewport dimensions on render thread
    virtual void RT_SetViewport(int x, int y, int width, int height, int id = -1) = 0;

    // Push render target on render thread
    virtual void RT_PushRenderTarget(int nTarget, CTexture* pTex, SDepthTexture* pDS, int nS) = 0;

    // Setup scissors rect
    virtual void EF_Scissor(bool bEnable, int sX, int sY, int sWdt, int sHgt) = 0;

#ifdef SUPPORT_HW_MOUSE_CURSOR
    virtual IHWMouseCursor* GetIHWMouseCursor() = 0;
#endif

    virtual int GetRecursionLevel() = 0;

    virtual int GetIntegerConfigurationValue(const char* varName, int defaultValue) = 0;
    virtual float GetFloatConfigurationValue(const char* varName, float defaultValue) = 0;
    virtual bool GetBooleanConfigurationValue(const char* varName, bool defaultValue) = 0;

    // Methods exposed to external libraries
    virtual void ApplyDepthTextureState(int unit, int nFilter, bool clamp) = 0;
    virtual ITexture* GetZTargetTexture() = 0;
    virtual int GetTextureState(const STexState& TS) = 0;
    virtual uint32 TextureDataSize(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nSlices, ETEX_Format eTF, ETEX_TileMode eTM = eTM_None) = 0;
    virtual void ApplyForID(int nID, int nTUnit, int nTState, int nTexMaterialSlot, int nSUnit, bool useWhiteDefault) = 0;
    virtual ITexture* Create3DTexture(const char* szName, int nWidth, int nHeight, int nDepth, int nMips, int nFlags, const byte* pData, ETEX_Format eTFSrc, ETEX_Format eTFDst) = 0;
    virtual bool IsTextureExist(const ITexture* pTex) = 0;
    virtual const char* NameForTextureFormat(ETEX_Format eTF) = 0;
    virtual const char* NameForTextureType(ETEX_Type eTT) = 0;
    virtual bool IsVideoThreadModeEnabled() = 0;
    virtual IDynTexture* CreateDynTexture2(uint32 nWidth, uint32 nHeight, uint32 nTexFlags, const char* szSource, ETexPool eTexPool) = 0;
    virtual uint32 GetCurrentTextureAtlasSize() = 0;

    virtual void BeginProfilerSection(const char* name, uint32 eProfileLabelFlags = 0) = 0;
    virtual void EndProfilerSection(const char* name) = 0;
    virtual void AddProfilerLabel(const char* name) = 0;

private:
    // use private for EF_Query to prevent client code to submit arbitrary combinations of output data/size
    virtual void EF_QueryImpl(ERenderQueryTypes eQuery, void* pInOut0, uint32 nInOutSize0, void* pInOut1, uint32 nInOutSize1) = 0;
};

struct SShaderCacheStatistics
{
    size_t m_nTotalLevelShaderCacheMisses;
    size_t m_nGlobalShaderCacheMisses;
    size_t m_nNumShaderAsyncCompiles;
    bool m_bShaderCompileActive;

    SShaderCacheStatistics()
        : m_nTotalLevelShaderCacheMisses(0)
        , m_nGlobalShaderCacheMisses(0)
        , m_nNumShaderAsyncCompiles(0)
        , m_bShaderCompileActive(false)
    {}
};

// The statistics about the pool for render mesh data
// Note:
struct SMeshPoolStatistics
{
    // The size of the mesh data size in bytes
    size_t nPoolSize;

    // The amount of memory currently in use in the pool
    size_t nPoolInUse;

    // The highest amount of memory allocated within the mesh data pool
    size_t nPoolInUsePeak;

    // The size of the mesh data size in bytes
    size_t nInstancePoolSize;

    // The amount of memory currently in use in the pool
    size_t nInstancePoolInUse;

    // The highest amount of memory allocated within the mesh data pool
    size_t nInstancePoolInUsePeak;

    size_t nFallbacks;
    size_t nInstanceFallbacks;
    size_t nFlushes;

    SMeshPoolStatistics()
        : nPoolSize()
        , nPoolInUse()
        , nInstancePoolSize()
        , nInstancePoolInUse()
        , nInstancePoolInUsePeak()
        , nFallbacks()
        , nInstanceFallbacks()
        , nFlushes()
    {}
};

struct SRendererQueryGetAllTexturesParam
{
    SRendererQueryGetAllTexturesParam()
        : pTextures(NULL)
        , numTextures(0)
    {
    }

    _smart_ptr<ITexture>* pTextures;
    uint32 numTextures;
};


//////////////////////////////////////////////////////////////////////

#define STRIPTYPE_NONE           0
#define STRIPTYPE_ONLYLISTS      1
#define STRIPTYPE_SINGLESTRIP    2
#define STRIPTYPE_MULTIPLESTRIPS 3
#define STRIPTYPE_DEFAULT        4

/////////////////////////////////////////////////////////////////////

struct IRenderMesh;

//DOC-IGNORE-BEGIN
#include "VertexFormats.h"
//DOC-IGNORE-END

struct SRestLightingInfo
{
    SRestLightingInfo()
    {
        averDir.zero();
        averCol = Col_Black;
        refPoint.zero();
    }
    Vec3 averDir;
    ColorF averCol;
    Vec3 refPoint;
};

class CLodValue
{
public:
    CLodValue()
    {
        m_nLodA = -1;
        m_nLodB = -1;
        m_nDissolveRef = 0;
    }

    CLodValue(int nLodA)
    {
        m_nLodA = aznumeric_caster(nLodA);
        m_nLodB = -1;
        m_nDissolveRef = 0;
    }

    CLodValue(int nLodA, uint8 nDissolveRef, int nLodB)
    {
        m_nLodA = aznumeric_caster(nLodA);
        m_nLodB = aznumeric_caster(nLodB);
        m_nDissolveRef = nDissolveRef;
    }

    int LodA() const { return m_nLodA; }
    int LodB() const { return m_nLodB; }

    uint8 DissolveRefA() const { return m_nDissolveRef; }
    uint8 DissolveRefB() const { return 255 - m_nDissolveRef; }

private:
    int16 m_nLodA;
    int16 m_nLodB;
    uint8 m_nDissolveRef;
};

// Description:
//   Structure used to pass render parameters to Render() functions of IStatObj and ICharInstance.
struct SRendParams
{
    SRendParams()
    {
        memset(this, 0, sizeof(SRendParams));
        fAlpha = 1.f;
        fRenderQuality = 1.f;
        nRenderList = EFSLIST_GENERAL;
        nAfterWater = 1;
        mRenderFirstContainer = false;
        NoDecalReceiver = false;
    }

    // Summary:
    //  object transformations.
    Matrix34* pMatrix;
    struct SInstancingInfo* pInstInfo;
    // Summary:
    //  object previous transformations - motion blur specific.
    Matrix34* pPrevMatrix;
    // Summary:
    //  VisArea that contains this object, used for RAM-ambient cube query
    IVisArea*       m_pVisArea;
    // Summary:
    //  Override material.
    _smart_ptr<IMaterial> pMaterial;
    // Summary:
    //   Weights stream for deform morphs.
    IRenderMesh* pWeights;
    // Summary:
    //  Object Id for objects identification in renderer.
    struct IRenderNode* pRenderNode;
    // Summary:
    //  Unique object Id for objects identification in renderer.
    void* pInstance;
    // Summary:
    //   TerrainTexInfo for grass.
    struct SSectorTextureSet* pTerrainTexInfo;
    // Summary:
    //   storage for LOD transition states.
    struct CRNTmpData** ppRNTmpData;
    // Summary:
    //  dynamic render data object which can be set by the game
    AZStd::vector<SShaderParam>* pShaderParams;
    // Summary:
    //  Ambient color for the object.
    ColorF AmbientColor;
    // Summary:
    //  Custom sorting offset.
    float       fCustomSortOffset;
    // Summary:
    //  Object alpha.
    float     fAlpha;
    // Summary:
    //  Distance from camera.
    float     fDistance;
    // Summary:
    //   Quality of shaders rendering.
    float fRenderQuality;
    // Summary:
    //  Light mask to specify which light to use on the object.
    uint32 nDLightMask;
    // Summary:
    //  Approximate information about the lights not included into nDLightMask.
    //  SRestLightingInfo restLightInfo;
    // Summary:
    //  CRenderObject flags.
    int32       dwFObjFlags;
    // Summary:
    //   Material layers blending amount
    uint32 nMaterialLayersBlend;
    // Summary:
    //  Vision modes params
    uint32 nVisionParams;
    // Summary:
    //  Vision modes params
    uint32 nHUDSilhouettesParams;
    // Summary:
    //  Defines what pieces of pre-broken geometry has to be rendered
    uint64 nSubObjHideMask;

    //   Defines per object float custom data
    float fCustomData[4];

    //   Custom TextureID
    int16 nTextureID;

    //   Defines per object custom flags
    uint16 nCustomFlags;

    // The LOD value compute for rendering
    CLodValue lodValue;

    //   Defines per object custom data
    uint8 nCustomData;

    // Summary:
    //   Defines per object DissolveRef value if used by shader.
    uint8 nDissolveRef;
    // Summary:
    //   per-instance vis area stencil ref id
    uint8   nClipVolumeStencilRef;
    // Summary:
    //   Custom offset for sorting by distance.
    uint8  nAfterWater;

    // Summary:
    //   Material layers bitmask -> which material layers are active.
    uint8 nMaterialLayers;

    // Summary:
    //  Force a sort value for render elements.
    uint8 nRenderList;
    // Summary:
    //  Special sorter to ensure correct ordering even if parts of the 3DEngine are run in parallel
    uint32 rendItemSorter;
    // Summary:
    // Render the first particle container only, instead of all the containers
    bool mRenderFirstContainer;

    // Summary:
    // Check if the preview would Show Wireframe - Vera,Confetti
    bool bIsShowWireframe;

    //Summary:
    // Force drawing static instead of deformable meshes
    bool bForceDrawStatic;

    bool NoDecalReceiver;
};
