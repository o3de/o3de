/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Shaders common interface.

#pragma once


#if defined(LINUX) || defined(APPLE)
  #include <platform.h>
#endif

#include "smartptr.h"

#include "Cry_Vector3.h"

#include "Cry_Color.h"
#include "smartptr.h"

#include "VertexFormats.h"
#include <Vertex.h>



#include <CrySizer.h>
#include <IMaterial.h>

struct IMaterial;
class CRendElementBase;
class CRenderObject;
class CREMesh;
struct IRenderMesh;
struct IShader;
struct IVisArea;

class CRendElement;
class CRendElementBase;
class CTexture;
class CTexAnim;
class CShader;
class ITexAnim;
struct SShaderPass;
struct SShaderItem;
class ITexture;
struct IMaterial;
struct SParam;
struct SShaderSerializeContext;
struct IAnimNode;
struct SSkinningData;
struct SSTexSamplerFX;
struct SShaderTextureSlot;
struct IRenderElement;

namespace AZ
{
    class LegacyJobExecutor;

    namespace Vertex
    {
        class Format;
    }
}

//================================================================
// Summary:
//   Geometry Culling type.
enum ECull
{
    eCULL_Back = 0, // Back culling flag.
    eCULL_Front,    // Front culling flag.
    eCULL_None      // No culling flag.
};

enum ERenderResource
{
    eRR_Unknown,
    eRR_Mesh,
    eRR_Texture,
    eRR_Shader,
    eRR_ShaderResource,
};

enum EEfResTextures : int // This needs a fixed size so the enum can be forward declared (needed by IMaterial.h)
{
    EFTT_DIFFUSE = 0,
    EFTT_NORMALS,
    EFTT_SPECULAR,
    EFTT_ENV,
    EFTT_DETAIL_OVERLAY,
    EFTT_SECOND_SMOOTHNESS,
    EFTT_HEIGHT,
    EFTT_DECAL_OVERLAY,
    EFTT_SUBSURFACE,
    EFTT_CUSTOM,
    EFTT_CUSTOM_SECONDARY,
    EFTT_OPACITY,
    EFTT_SMOOTHNESS,
    EFTT_EMITTANCE,
    EFTT_OCCLUSION,
    EFTT_SPECULAR_2,

    EFTT_MAX,
    EFTT_UNKNOWN = EFTT_MAX
};

enum EEfResSamplers
{
    EFSS_ANISO_HIGH = 0,
    EFSS_ANISO_LOW,
    EFSS_TRILINEAR,
    EFSS_BILINEAR,
    EFSS_TRILINEAR_CLAMP,
    EFSS_BILINEAR_CLAMP,
    EFSS_ANISO_HIGH_BORDER,
    EFSS_TRILINEAR_BORDER,

    EFSS_MAX,
};

//=========================================================================

// Summary:
//   Array Pointers for Shaders.

enum ESrcPointer
{
    eSrcPointer_Unknown,
    eSrcPointer_Vert,
    eSrcPointer_Color,
    eSrcPointer_Tex,
    eSrcPointer_TexLM,
    eSrcPointer_Normal,
    eSrcPointer_Tangent,
    eSrcPointer_Max,
};

struct SWaveForm;
struct SWaveForm2;

#define FRF_REFRACTIVE 1
// FREE                2
#define FRF_HEAT       4
#define MAX_HEATSCALE 4

#if !defined(MAX_JOINT_AMOUNT)
#error MAX_JOINT_AMOUNT is not defined
#endif

#if (MAX_JOINT_AMOUNT <= 256)
typedef uint8 JointIdType;
#else
typedef uint16 JointIdType;
#endif

// The soft maximum cap for the sliders for emissive intensity.  Also used to clamp legacy glow calculations in MaterialHelpers::MigrateXmlLegacyData.
// Note this is a "soft max" because the Emissive Intensity slider is capped at 200, but values higher than 200 may be entered in the text field.
#define EMISSIVE_INTENSITY_SOFT_MAX 200.0f

//=========================================================================

enum EParamType
{
    eType_UNKNOWN,
    eType_BYTE,
    eType_BOOL,
    eType_SHORT,
    eType_INT,
    eType_HALF,
    eType_FLOAT,
    eType_STRING,
    eType_FCOLOR,
    eType_VECTOR,
    eType_TEXTURE_HANDLE,
    eType_CAMERA,
    eType_FCOLORA, //  with alpha channel
};

enum ESamplerType
{
    eSType_UNKNOWN,
    eSType_Sampler,
    eSType_SamplerComp,
};

union UParamVal
{
    int8 m_Byte;
    bool m_Bool;
    short m_Short;
    int m_Int;
    float m_Float;
    char* m_String;
    float m_Color[4];
    float m_Vector[3];
    CCamera* m_pCamera;
};

// Note:
//   In order to facilitate the memory allocation tracking, we're using here this class;
//   if you don't like it, please write a substitute for all string within the project and use them everywhere.
struct SShaderParam
{
    AZStd::string m_Name;
    AZStd::string m_Script;
    UParamVal m_Value;
    EParamType m_Type;
    uint8 m_eSemantic;
    uint8 m_Pad[3] = { 0 };

    void Construct()
    {
        memset(&m_Value, 0, sizeof(m_Value));
        m_Type = eType_UNKNOWN;
        m_eSemantic = 0;
        m_Name.clear();
        m_Script.clear();
    }

    SShaderParam()
    {
        Construct();
    }
    size_t Size()
    {
        size_t nSize = sizeof(*this);
        if (m_Type == eType_STRING)
        {
            nSize += strlen (m_Value.m_String) + 1;
        }

        return nSize;
    }
    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_Script);
        if (m_Type == eType_STRING)
        {
            pSizer->AddObject(m_Value.m_String, strlen (m_Value.m_String) + 1);
        }
    }

    void Destroy()
    {
        if (m_Type == eType_STRING)
        {
            delete [] m_Value.m_String;
        }
    }

    ~SShaderParam()
    {
        Destroy();
    }

    SShaderParam (const SShaderParam& src)
    {
        m_Name = src.m_Name;
        m_Script = src.m_Script;
        m_Type = src.m_Type;
        m_eSemantic = src.m_eSemantic;
        if (m_Type == eType_STRING)
        {
            const size_t len = strlen(src.m_Value.m_String) + 1;
            m_Value.m_String = new char[len];
            azstrcpy(m_Value.m_String, len, src.m_Value.m_String);
        }
        else
        {
            m_Value = src.m_Value;
        }
    }

    SShaderParam& operator = (const SShaderParam& src)
    {
        this->~SShaderParam();
        new(this)SShaderParam(src);
        return *this;
    }

    static bool SetParam(const char* name, AZStd::vector<SShaderParam>* Params, const UParamVal& pr)
    {
        uint32 i;
        for (i = 0; i < (uint32)Params->size(); i++)
        {
            SShaderParam* sp = &(*Params)[i];
            if (!sp || &sp->m_Value == &pr)
            {
                continue;
            }
            if (azstricmp(sp->m_Name.c_str(), name) == 0)
            {
                if (sp->m_Type == eType_STRING)
                {
                    delete[] sp->m_Value.m_String;
                }


                switch (sp->m_Type)
                {
                case eType_FLOAT:
                    sp->m_Value.m_Float = pr.m_Float;
                    break;
                case eType_SHORT:
                    sp->m_Value.m_Short = pr.m_Short;
                    break;
                case eType_INT:
                case eType_TEXTURE_HANDLE:
                    sp->m_Value.m_Int = pr.m_Int;
                    break;

                case eType_VECTOR:
                    sp->m_Value.m_Vector[0] = pr.m_Vector[0];
                    sp->m_Value.m_Vector[1] = pr.m_Vector[1];
                    sp->m_Value.m_Vector[2] = pr.m_Vector[2];
                    break;

                case eType_FCOLOR:
                case eType_FCOLORA:
                    sp->m_Value.m_Color[0] = pr.m_Color[0];
                    sp->m_Value.m_Color[1] = pr.m_Color[1];
                    sp->m_Value.m_Color[2] = pr.m_Color[2];
                    sp->m_Value.m_Color[3] = pr.m_Color[3];
                    break;

                case eType_STRING:
                {
                    char* str = pr.m_String;
                    const size_t len = strlen(str) + 1;
                    sp->m_Value.m_String = new char [len];
                    azstrcpy(sp->m_Value.m_String, len, str);
                }
                break;
                }
                break;
            }
        }
        if (i == Params->size())
        {
            return false;
        }
        return true;
    }
    static bool GetValue(const char* szName, AZStd::vector<SShaderParam>* Params, float* v, int nID);

    static bool GetValue(uint8 eSemantic, AZStd::vector<SShaderParam>* Params, float* v, int nID);

    void CopyValue(const SShaderParam& src)
    {
        if (m_Type == eType_STRING && this != &src)
        {
            delete[] m_Value.m_String;

            if (src.m_Type == eType_STRING)
            {
                size_t size = strlen(src.m_Value.m_String) + 1;
                m_Value.m_String = new char[size];
                azstrcpy(m_Value.m_String, size, src.m_Value.m_String);
                return;
            }
        }

        m_Value = src.m_Value;
    }

    void CopyValueNoString(const SShaderParam& src)
    {
        assert(m_Type != eType_STRING && src.m_Type != eType_STRING);

        m_Value = src.m_Value;
    }

    void CopyType(const SShaderParam& src)
    {
        m_Type = src.m_Type;
    }
};


// Summary:
//   Vertex modificators definitions (must be 16 bit flag).

#define MDV_BENDING            0x100
#define MDV_DET_BENDING        0x200
#define MDV_DET_BENDING_GRASS  0x400
#define MDV_WIND               0x800
#define MDV_DEPTH_OFFSET       0x2000

// Does the vertex shader require position-invariant compilation?
// This would be true of shaders rendering multiple times with different vertex shaders - for example during zprepass and the gbuffer pass
// Note this is different than the technique flag FHF_POSITION_INVARIANT as that does custom behavior for terrain
#define MDV_POSITION_INVARIANT 0x4000


//==============================================================================
// CRenderObject

//////////////////////////////////////////////////////////////////////
// CRenderObject::m_ObjFlags: Flags used by shader pipeline

enum ERenderObjectFlags
{
    FOB_VERTEX_VELOCITY             = BIT(0),
    FOB_RENDER_TRANS_AFTER_DOF      = BIT(1),  //transparencies rendered after depth of field
    //Unused                        = BIT(2),
    FOB_RENDER_AFTER_POSTPROCESSING = BIT(3),
    FOB_OWNER_GEOMETRY              = BIT(4),
    FOB_MESH_SUBSET_INDICES         = BIT(5),
    FOB_SELECTED                    = BIT(6),
    FOB_RENDERER_IDENDITY_OBJECT    = BIT(7),
    FOB_GLOBAL_ILLUMINATION         = BIT(8),
    FOB_NO_FOG                      = BIT(9),
    FOB_DECAL                       = BIT(10),
    FOB_OCTAGONAL                   = BIT(11),
    FOB_POINT_SPRITE                = BIT(13),
    FOB_SOFT_PARTICLE               = BIT(14),
    FOB_REQUIRES_RESOLVE            = BIT(15),
    FOB_UPDATED_RTMASK              = BIT(16),
    FOB_AFTER_WATER                 = BIT(17),
    FOB_BENDED                      = BIT(18),
    FOB_ZPREPASS                    = BIT(19),
    FOB_PARTICLE_SHADOWS            = BIT(20),
    FOB_DISSOLVE                    = BIT(21),
    FOB_MOTION_BLUR                 = BIT(22),
    FOB_NEAREST                     = BIT(23), // [Rendered in Camera Space]
    FOB_SKINNED                     = BIT(24),
    FOB_DISSOLVE_OUT                = BIT(25),
    FOB_DYNAMIC_OBJECT              = BIT(26),
    FOB_ALLOW_TESSELLATION          = BIT(27),
    FOB_DECAL_TEXGEN_2D             = BIT(28),
    FOB_IN_DOORS                    = BIT(29),
    FOB_HAS_PREVMATRIX              = BIT(30),
    FOB_LIGHTVOLUME                 = BIT(31),

    FOB_DECAL_MASK =  (FOB_DECAL | FOB_DECAL_TEXGEN_2D),
    FOB_PARTICLE_MASK = (FOB_SOFT_PARTICLE | FOB_NO_FOG | FOB_GLOBAL_ILLUMINATION | FOB_PARTICLE_SHADOWS | FOB_NEAREST | FOB_MOTION_BLUR | FOB_LIGHTVOLUME | FOB_ALLOW_TESSELLATION | FOB_IN_DOORS | FOB_AFTER_WATER),

    // WARNING: FOB_MASK_AFFECTS_MERGING must start from 0x10000/bit 16 (important for instancing).
    FOB_MASK_AFFECTS_MERGING_GEOM  = (FOB_ZPREPASS | FOB_SKINNED | FOB_BENDED | FOB_DYNAMIC_OBJECT | FOB_ALLOW_TESSELLATION | FOB_NEAREST),
    FOB_MASK_AFFECTS_MERGING = (FOB_ZPREPASS | FOB_MOTION_BLUR | FOB_HAS_PREVMATRIX | FOB_SKINNED | FOB_BENDED | FOB_PARTICLE_SHADOWS | FOB_AFTER_WATER | FOB_DISSOLVE | FOB_DISSOLVE_OUT | FOB_NEAREST | FOB_DYNAMIC_OBJECT | FOB_ALLOW_TESSELLATION)
};

struct SSkyInfo
{
    ITexture* m_SkyBox[3];
    float m_fSkyLayerHeight;

    int Size()
    {
        int nSize = sizeof(SSkyInfo);
        return nSize;
    }
    SSkyInfo()
    {
        memset(this, 0, sizeof(SSkyInfo));
    }
};


// Description:
//   Interface for the skinnable objects (renderer calls its functions to get the skinning data).
// should only created by EF_CreateSkinningData
struct alignas(16) SSkinningData
{
    uint32                  nNumBones;
    uint32                  nHWSkinningFlags;
    DualQuat*               pBoneQuatsS;
    Matrix34*               pBoneMatrices;
    JointIdType*            pRemapTable;
    AZ::LegacyJobExecutor*  pAsyncJobExecutor;
    AZ::LegacyJobExecutor*  pAsyncDataJobExecutor;
    SSkinningData*          pPreviousSkinningRenderData; // used for motion blur
    uint32                  remapGUID;
    void*                   pCharInstCB; // used if per char instance cbs are available in renderdll (d3d11+);
    // members below are for Software Skinning
    void*                   pCustomData; // client specific data, used for example for sw-skinning on animation side
    SSkinningData*          pNextSkinningData;          // List to the next element which needs SW-Skinning
    [[maybe_unused]] int    m_padding[2]; // padding to avoid MSVC warning 4324
};

struct alignas(16) SRenderObjData
{
    uintptr_t m_uniqueObjectId;

    SSkinningData* m_pSkinningData;

    float   m_fTempVars[10];                                    // Different useful vars (ObjVal component in shaders)

    // using a pointer, the client code has to ensure that the data stays valid
    const AZStd::vector<SShaderParam>* m_pShaderParams;

    uint32  m_nHUDSilhouetteParams;

    uint64 m_nSubObjHideMask;


    uint16  m_FogVolumeContribIdx[2];
    
    uint16  m_nLightID;
    uint16  m_LightVolumeId;

    uint8 m_screenBounds[4];

    uint16  m_nCustomFlags;
    uint8   m_nCustomData;

    SRenderObjData()
    {
        Init();
    }

    void Init()
    {
        m_nSubObjHideMask = 0;
        m_uniqueObjectId = 0;
        m_nLightID = 0;
        m_LightVolumeId = 0;
        m_pSkinningData = NULL;
        m_screenBounds[0] = m_screenBounds[1] = m_screenBounds[2] = m_screenBounds[3] = 0;
        m_nCustomData = 0;
        m_nCustomFlags = 0;
        m_nHUDSilhouetteParams = 0;

        m_pShaderParams = nullptr;
        m_FogVolumeContribIdx[0] = m_FogVolumeContribIdx[1] = static_cast<uint16>(-1);

        // The following should be changed to be something like 0xac to indicate invalid data so that by default 
        // data that was not set will break render features and will be traced (otherwise, default 0 just might pass)
        memset(m_fTempVars, 0, 10 * sizeof(float));
    }

    void SetShaderParams(const AZStd::vector<SShaderParam>* pShaderParams)
    {
        m_pShaderParams = pShaderParams;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        AZ_UNUSED(pSizer);
    }
};

//////////////////////////////////////////////////////////////////////
// Objects using in shader pipeline


// Summary:
//   Same as in the 3dEngine.
#define MAX_LIGHTS_NUM 32

struct ShadowMapFrustum;

//////////////////////////////////////////////////////////////////////
///
/// Objects using in shader pipeline
/// Single rendering item, that can be created from 3DEngine and persist across multiple frames
/// It can be compiled into the platform specific efficient rendering compiled object.
///
//////////////////////////////////////////////////////////////////////
class alignas(16) CRenderObject
{
public:
    AZ_CLASS_ALLOCATOR(CRenderObject, AZ::LegacyAllocator, 0);

    struct SInstanceInfo
    {
        Matrix34 m_Matrix;
        ColorF m_AmbColor;
    };

    struct SInstanceData
    {
        Matrix34 m_MatInst;
        Vec4 m_vBendInfo;
        Vec4 m_vDissolveInfo;
    };

    struct PerInstanceConstantBufferKey
    {
        PerInstanceConstantBufferKey()
            : m_Id{0xFFFF}
            , m_IndirectId{0xFF}
        {}

        bool IsValid() const
        {
            return m_Id != 0xFFFF;
        }

        AZ::u16 m_Id;
        AZ::u8  m_IndirectId;
    };

    //////////////////////////////////////////////////////////////////////////
    SInstanceInfo               m_II;                   //!< Per instance data

    uint64                      m_ObjFlags;             //!< Combination of FOB_ flags.
    uint32                      m_Id;

    float                       m_fAlpha;               //!< Object alpha.
    float                       m_fDistance;            //!< Distance to the object.

    union
    {
        float                   m_fSort;                //!< Custom sort value.
        uint16                  m_nSort;
    };

    uint64                      m_nRTMask;              //!< Shader runtime modification flags
    uint16                      m_nMDV;                 //!< Vertex modifier flags for Shader.
    uint16                      m_nRenderQuality;       //!< 65535 - full quality, 0 - lowest quality, used by CStatObj
    int16                       m_nTextureID;           //!< Custom texture id.

    union
    {
        uint8                   m_breakableGlassSubFragIndex;
        uint8                   m_ParticleObjFlags;
    };
    uint8                       m_nClipVolumeStencilRef;  //!< Per instance vis area stencil reference ID
    uint8                       m_DissolveRef;            //!< Dissolve value
    uint8                       m_RState;                 //!< Render state used for object

    bool                        m_NoDecalReceiver;

    uint32                      m_nMaterialLayers;        //!< Which mtl layers active and how much to blend them


    _smart_ptr<IMaterial>       m_pCurrMaterial;          //!< Parent material used for render object.


    PerInstanceConstantBufferKey m_PerInstanceConstantBufferKey;

    [[maybe_unused]] int m_padding[1]; // padding to avoid MSVC warning 4324

    //! Embedded SRenderObjData, optional data carried by CRenderObject
    SRenderObjData             m_data;

public:

    //////////////////////////////////////////////////////////////////////////
    // Methods
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    /// Constructor
    //////////////////////////////////////////////////////////////////////////
    CRenderObject()
        : m_Id(~0u)
    {
        Init();
    }
    ~CRenderObject() {}

    //=========================================================================================================

    Vec3 GetTranslation() const { return m_II.m_Matrix.GetTranslation(); }
    float GetScaleX() const { return sqrt_tpl(m_II.m_Matrix(0, 0) * m_II.m_Matrix(0, 0) + m_II.m_Matrix(0, 1) * m_II.m_Matrix(0, 1) + m_II.m_Matrix(0, 2) * m_II.m_Matrix(0, 2)); }
    float GetScaleZ() const { return sqrt_tpl(m_II.m_Matrix(2, 0) * m_II.m_Matrix(2, 0) + m_II.m_Matrix(2, 1) * m_II.m_Matrix(2, 1) + m_II.m_Matrix(2, 2) * m_II.m_Matrix(2, 2)); }

    void Init()
    {
        m_ObjFlags = 0;
        m_nRenderQuality = 65535;

        m_RState = 0;
        m_fDistance = 0.0f;

        m_nClipVolumeStencilRef = 0;
        m_nMaterialLayers = 0;
        m_DissolveRef = 0;

        m_nMDV = 0;
        m_fSort = 0;

        m_II.m_AmbColor = Col_White;
        m_fAlpha = 1.0f;
        m_nTextureID = -1;
        m_pCurrMaterial = nullptr;

        m_PerInstanceConstantBufferKey = {};

        m_nRTMask = 0;


        m_NoDecalReceiver = false;
        m_data.Init();
    }
    void AssignId(uint32 id) { m_Id = id; }

    ILINE Matrix34A& GetMatrix() { return m_II.m_Matrix; }


protected:

    // Disallow copy (potential bugs with PERMANENT objects)
    // alwasy use IRendeer::EF_DuplicateRO if you want a copy
    // of a CRenderObject
    CRenderObject& operator= (CRenderObject& other) = default;

    void CloneObject(CRenderObject* srcObj)
    {
        *this = *srcObj;
    }

    friend class CRenderer;
};

enum EResClassName
{
    eRCN_Texture,
    eRCN_Shader,
};

// className: CTexture, CHWShader_VS, CHWShader_PS, CShader
struct SResourceAsync
{
    AZ_CLASS_ALLOCATOR(SResourceAsync, AZ::SystemAllocator, 0);
    int nReady;          // 0: Not ready; 1: Ready; -1: Error
    int8* pData;
    EResClassName eClassName;     // Resource class name
    char* Name;          // Resource name
    union
    {
        // CTexture parameters
        struct
        {
            int nWidth, nHeight, nMips, nTexFlags, nFormat, nTexId;
        };
        // CShader parameters
        struct
        {
            int nShaderFlags;
        };
    };
    void* pResource; // Pointer to created resource

    SResourceAsync()
    {
        memset(this, 0, sizeof(SResourceAsync));
    }

    ~SResourceAsync()
    {
        delete Name;
    }
};

#include "IRenderer.h"

//==============================================================================

// Summary:
//   Color operations flags.
enum EColorOp
{
    eCO_NOSET = 0,
    eCO_DISABLE = 1,
    eCO_REPLACE = 2,
    eCO_DECAL = 3,
    eCO_ARG2 = 4,
    eCO_MODULATE = 5,
    eCO_MODULATE2X = 6,
    eCO_MODULATE4X = 7,
    eCO_BLENDDIFFUSEALPHA = 8,
    eCO_BLENDTEXTUREALPHA = 9,
    eCO_DETAIL = 10,
    eCO_ADD = 11,
    eCO_ADDSIGNED = 12,
    eCO_ADDSIGNED2X = 13,
    eCO_MULTIPLYADD = 14,
    eCO_BUMPENVMAP = 15,
    eCO_BLEND = 16,
    eCO_MODULATEALPHA_ADDCOLOR = 17,
    eCO_MODULATECOLOR_ADDALPHA = 18,
    eCO_MODULATEINVALPHA_ADDCOLOR = 19,
    eCO_MODULATEINVCOLOR_ADDALPHA = 20,
    eCO_DOTPRODUCT3 = 21,
    eCO_LERP = 22,
    eCO_SUBTRACT = 23,
    eCO_MODULATE_METAL_FONT_SPECIAL_MODE = 24,
};

enum EColorArg
{
    eCA_Unknown,
    eCA_Specular,
    eCA_Texture,
    eCA_Texture1,
    eCA_Normal,
    eCA_Diffuse,
    eCA_Previous,
    eCA_Constant,
};

#define DEF_TEXARG0 (eCA_Texture | (eCA_Diffuse << 3))
#define DEF_TEXARG1 (eCA_Texture | (eCA_Previous << 3))

enum ETexModRotateType
{
    ETMR_NoChange,
    ETMR_Fixed,
    ETMR_Constant,
    ETMR_Oscillated,
    ETMR_Max
};

enum ETexModMoveType
{
    ETMM_NoChange,
    ETMM_Fixed,
    ETMM_Constant,
    ETMM_Jitter,
    ETMM_Pan,
    ETMM_Stretch,
    ETMM_StretchRepeat,
    ETMM_Max
};

enum ETexGenType
{
    ETG_Stream,
    ETG_World,
    ETG_Camera,
    ETG_Max
};




//////////////////////////////////////////////////////////////////////
#define FILTER_NONE      -1
#define FILTER_POINT      0
#define FILTER_LINEAR     1
#define FILTER_BILINEAR   2
#define FILTER_TRILINEAR  3
#define FILTER_ANISO2X    4
#define FILTER_ANISO4X    5
#define FILTER_ANISO8X    6
#define FILTER_ANISO16X   7

//////////////////////////////////////////////////////////////////////
#define TADDR_WRAP        0
#define TADDR_CLAMP       1
#define TADDR_MIRROR      2
#define TADDR_BORDER      3


struct IRenderTarget
{
    virtual ~IRenderTarget(){}
    virtual void Release() = 0;
    virtual void AddRef() = 0;
};


struct IRenderShaderResources
{
    // <interfuscator:shuffle>
    virtual void AddRef() = 0;
    virtual void UpdateConstants(IShader* pSH) = 0;
    virtual void CloneConstants(const IRenderShaderResources* pSrc) = 0;
    virtual bool HasLMConstants() const = 0;

    // properties

    virtual ColorF GetColorValue(EEfResTextures slot) const = 0;
    virtual void SetColorValue(EEfResTextures slot, const ColorF& color) = 0;

    virtual float GetStrengthValue(EEfResTextures slot) const = 0;
    virtual void SetStrengthValue(EEfResTextures slot, float value) = 0;

    // configs
    virtual const float& GetAlphaRef() const = 0;
    virtual void SetAlphaRef(float v) = 0;

    virtual int GetResFlags() = 0;
    virtual void SetMtlLayerNoDrawFlags(uint8 nFlags) = 0;
    virtual uint8 GetMtlLayerNoDrawFlags() const = 0;
    virtual SSkyInfo* GetSkyInfo() = 0;
    virtual void SetMaterialName(const char* szName) = 0;


    virtual AZStd::vector<SShaderParam>& GetParameters() = 0;

    virtual ColorF GetFinalEmittance() = 0;
    virtual float GetVoxelCoverage() = 0;

    virtual ~IRenderShaderResources() {}
    virtual void Release() = 0;
    virtual void ConvertToInputResource(struct SInputShaderResources* pDst) = 0;
    virtual IRenderShaderResources* Clone() const = 0;
    virtual void SetShaderParams(struct SInputShaderResources* pDst, IShader* pSH) = 0;

    virtual size_t GetResourceMemoryUsage(ICrySizer*    pSizer) = 0;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    // </interfuscator:shuffle>

    bool IsEmissive() const
    {
        // worst: *reinterpret_cast<int32*>(&) > 0x00000000
        // causes value to pass from FPU to CPU registers
        return GetStrengthValue(EFTT_EMITTANCE) > 0.0f;
    }

    bool IsTransparent() const
    {
        // worst: *reinterpret_cast<int32*>(&) < 0x3f800000
        // causes value to pass from FPU to CPU registers
        return GetStrengthValue(EFTT_OPACITY) < 1.0f;
    }

    bool IsAlphaTested() const
    {
        return GetAlphaRef() > 0.0f;
    }

    bool IsInvisible() const
    {
        const float o = GetStrengthValue(EFTT_OPACITY);
        const float a = GetAlphaRef();

        return o == 0.0f || a == 1.0f || o <= a;
    }
};


//===================================================================================
// Shader gen structure (used for automatic shader script generating).

//
#define SHGF_HIDDEN   1
#define SHGF_PRECACHE 2
#define SHGF_AUTO_PRECACHE 4
#define SHGF_LOWSPEC_AUTO_PRECACHE 8
#define SHGF_RUNTIME 0x10

#define SHGD_LM_DIFFUSE             0x1
#define SHGD_TEX_DETAIL             0x2
#define SHGD_TEX_NORMALS            0x4
#define SHGD_TEX_ENVCM              0x8
#define SHGD_TEX_SPECULAR           0x10
#define SHGD_TEX_SECOND_SMOOTHNESS  0x20
#define SHGD_TEX_HEIGHT             0x40
#define SHGD_TEX_SUBSURFACE         0x80
#define SHGD_HW_BILINEARFP16        0x100
#define SHGD_HW_SEPARATEFP16        0x200
#define SHGD_HW_ORBIS               0x800
#define SHGD_TEX_CUSTOM             0x1000
#define SHGD_TEX_CUSTOM_SECONDARY   0x2000
#define SHGD_TEX_DECAL              0x4000
#define SHGD_TEX_OCC                0x8000
#define SHGD_TEX_SPECULAR_2         0x10000
#define SHGD_HW_GLES3               0x20000
#define SHGD_USER_ENABLED           0x40000
#define SHGD_HW_SAA                 0x80000
#define SHGD_TEX_EMITTANCE          0x100000
#define SHGD_HW_DX10                0x200000
#define SHGD_HW_DX11                0x400000
#define SHGD_HW_GL4                 0x800000
#define SHGD_HW_WATER_TESSELLATION  0x1000000
#define SHGD_HW_SILHOUETTE_POM      0x2000000
// Confetti Nicholas Baldwin: adding metal shader language support
#define SHGD_HW_METAL               0x4000000
#define SHGD_TEX_MASK       (   SHGD_TEX_DETAIL | SHGD_TEX_NORMALS | SHGD_TEX_ENVCM | SHGD_TEX_SPECULAR | SHGD_TEX_SECOND_SMOOTHNESS | \
                                SHGD_TEX_HEIGHT | SHGD_TEX_SUBSURFACE | SHGD_TEX_CUSTOM | SHGD_TEX_CUSTOM_SECONDARY | SHGD_TEX_DECAL | \
                                SHGD_TEX_OCC | SHGD_TEX_SPECULAR_2 | SHGD_TEX_EMITTANCE)


//===================================================================================

enum EShaderType
{
    eST_All = -1,           // To set all with one call.

    eST_General = 0,
    eST_Metal,
    eST_Glass,
    eST_Ice,
    eST_Shadow,
    eST_Water,
    eST_FX,
    eST_PostProcess,
    eST_HDR,
    eST_Sky,
    eST_Compute,
    eST_Max                     // To define array size.
};

enum EShaderDrawType
{
    eSHDT_General,
    eSHDT_Light,
    eSHDT_Shadow,
    eSHDT_Terrain,
    eSHDT_Overlay,
    eSHDT_OceanShore,
    eSHDT_Fur,
    eSHDT_NoDraw,
    eSHDT_CustomDraw,
    eSHDT_Sky,
    eSHDT_Volume
};

enum EShaderQuality
{
    eSQ_Low = 0,
    eSQ_Medium = 1,
    eSQ_High = 2,
    eSQ_VeryHigh = 3,
    eSQ_Max = 4
};

enum ERenderQuality
{
    eRQ_Low = 0,
    eRQ_Medium = 1,
    eRQ_High = 2,
    eRQ_VeryHigh = 3,
    eRQ_Max = 4
};


//====================================================================================
// Phys. material flags

#define MATF_NOCLIP 1

//====================================================================================
// Registered shader techniques ID's

enum EShaderTechniqueID
{
    TTYPE_GENERAL = -1,
    TTYPE_Z = 0,
    TTYPE_SHADOWGEN,
    TTYPE_GLOWPASS,
    TTYPE_MOTIONBLURPASS,
    TTYPE_CUSTOMRENDERPASS,
    TTYPE_EFFECTLAYER,
    TTYPE_SOFTALPHATESTPASS,
    TTYPE_WATERREFLPASS,
    TTYPE_WATERCAUSTICPASS,
    TTYPE_ZPREPASS,
    TTYPE_PARTICLESTHICKNESSPASS,

    // PC specific techniques must go after this point, to support shader serializing
    // TTYPE_CONSOLE_MAX must equal TTYPE_MAX for console
    TTYPE_CONSOLE_MAX,
    TTYPE_DEBUG = TTYPE_CONSOLE_MAX,

    TTYPE_MAX
};


//================================================================
// Different preprocess flags for shaders that require preprocessing (like recursive render to texture, screen effects, visibility check, ...)
// SShader->m_nPreprocess flags in priority order

#define  SPRID_FIRST          25
#define  SPRID_SCANTEXWATER   26
#define  FSPR_SCANTEXWATER    (1 << SPRID_SCANTEXWATER)
#define  SPRID_SCANTEX        27
#define  FSPR_SCANTEX         (1 << SPRID_SCANTEX)
#define  SPRID_SCANLCM        28
#define  FSPR_SCANLCM         (1 << SPRID_SCANLCM)
#define  SPRID_GENSPRITES_DEPRECATED     29
#define  FSPR_GENSPRITES_DEPRECATED      (1 << SPRID_GENSPRITES_DEPRECATED)
#define  SPRID_CUSTOMTEXTURE  30
#define  FSPR_CUSTOMTEXTURE   (1 << SPRID_CUSTOMTEXTURE)
#define  SPRID_GENCLOUDS      31
#define  FSPR_GENCLOUDS       (1 << SPRID_GENCLOUDS)

#define  FSPR_MASK            0xfff00000
#define  FSPR_MAX             (1 << 31)

#define FEF_DONTSETTEXTURES   1                 // Set: explicit setting of samplers (e.g. tex->Apply(1,nTexStatePoint)), not set: set sampler by sematics (e.g. $ZTarget).
#define FEF_DONTSETSTATES     2

// SShader::m_Flags
// Different useful flags
#define EF_RELOAD        1                      // Shader needs tangent vectors array.
#define EF_FORCE_RELOAD  2
#define EF_RELOADED      4
#define EF_NODRAW        8
#define EF_HASCULL       0x10
#define EF_SUPPORTSDEFERREDSHADING_MIXED 0x20
#define EF_SUPPORTSDEFERREDSHADING_FULL 0x40
#define EF_SUPPORTSDEFERREDSHADING (EF_SUPPORTSDEFERREDSHADING_MIXED | EF_SUPPORTSDEFERREDSHADING_FULL)
#define EF_DECAL         0x80
#define EF_LOADED        0x100
#define EF_LOCALCONSTANTS 0x200
#define EF_BUILD_TREE     0x400
#define EF_LIGHTSTYLE    0x800
#define EF_NOCHUNKMERGING 0x1000
#define EF_SUNFLARES     0x2000
#define EF_NEEDNORMALS   0x4000                 // Need normals operations.
#define EF_OFFSETBUMP    0x8000
#define EF_NOTFOUND      0x10000
#define EF_DEFAULT       0x20000
#define EF_SKY           0x40000
#define EF_USELIGHTS     0x80000
#define EF_ALLOW3DC      0x100000
#define EF_FOGSHADER     0x200000
#define EF_FAILED_IMPORT 0x400000               // Currently just for debug, can be removed if necessary
#define EF_PRECACHESHADER 0x800000
#define EF_FORCEREFRACTIONUPDATE    0x1000000
#define EF_SUPPORTSINSTANCING_CONST 0x2000000
#define EF_SUPPORTSINSTANCING_ATTR  0x4000000
#define EF_SUPPORTSINSTANCING (EF_SUPPORTSINSTANCING_CONST | EF_SUPPORTSINSTANCING_ATTR)
#define EF_WATERPARTICLE  0x8000000
#define EF_CLIENTEFFECT  0x10000000
#define EF_SYSTEM        0x20000000
#define EF_REFRACTIVE    0x40000000
#define EF_NOPREVIEW     0x80000000

#define EF_PARSE_MASK    (EF_SUPPORTSINSTANCING | EF_SKY | EF_HASCULL | EF_USELIGHTS | EF_REFRACTIVE)


// SShader::Flags2
// Additional Different useful flags

#define EF2_PREPR_GENSPRITES_DEPRECATED 0x1
#define EF2_PREPR_GENCLOUDS 0x2
#define EF2_PREPR_SCANWATER 0x4
#define EF2_NOCASTSHADOWS  0x8
#define EF2_NODRAW         0x10
#define EF2_HASOPAQUE      0x40
#define EF2_AFTERHDRPOSTPROCESS  0x80
#define EF2_DONTSORTBYDIST 0x100
#define EF2_FORCE_WATERPASS    0x200
#define EF2_FORCE_GENERALPASS   0x400
#define EF2_AFTERPOSTPROCESS  0x800
#define EF2_IGNORERESOURCESTATES  0x1000
#define EF2_EYE_OVERLAY  0x2000
#define EF2_FORCE_TRANSPASS       0x4000
#define EF2_DEFAULTVERTEXFORMAT 0x8000
#define EF2_FORCE_ZPASS 0x10000
#define EF2_FORCE_DRAWLAST 0x20000
#define EF2_FORCE_DRAWAFTERWATER 0x40000
// free 0x80000
#define EF2_DEPTH_FIXUP 0x100000
#define EF2_SINGLELIGHTPASS 0x200000
#define EF2_FORCE_DRAWFIRST 0x400000
#define EF2_HAIR            0x800000
#define EF2_DETAILBUMPMAPPING 0x1000000
#define EF2_HASALPHATEST      0x2000000
#define EF2_HASALPHABLEND     0x4000000
#define EF2_ZPREPASS 0x8000000
#define EF2_VERTEXCOLORS 0x10000000
#define EF2_SKINPASS 0x20000000
#define EF2_HW_TESSELLATION 0x40000000
#define EF2_ALPHABLENDSHADOWS 0x80000000

class CCryNameR;
class CCryNameTSCRC;
struct IShader
{
public:
    // <interfuscator:shuffle>
    virtual ~IShader(){}
    virtual int GetID() = 0;
    virtual int AddRef() = 0;
    virtual int Release() = 0;
    virtual int ReleaseForce() = 0;

    virtual const char* GetName() = 0;
    virtual const char* GetName() const = 0;
    virtual int GetFlags() const = 0;
    virtual int GetFlags2() const = 0;
    virtual void SetFlags2(int Flags) = 0;
    virtual void ClearFlags2(int Flags) = 0;
    virtual bool Reload(int nFlags, const char* szShaderName) = 0;

    virtual int GetTexId () = 0;

    virtual ECull GetCull(void) = 0;
    virtual int Size(int Flags) = 0;

    virtual int GetTechniqueID(int nTechnique, int nRegisteredTechnique) = 0;
    virtual AZ::Vertex::Format GetVertexFormat(void) = 0;


    virtual EShaderType GetShaderType() = 0;
    virtual EShaderDrawType GetShaderDrawType() const = 0;
    virtual uint32      GetVertexModificator() = 0;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    // </interfuscator:shuffle>

    static uint32 GetTextureSlot(EEfResTextures textureType) { return (uint32)textureType; }
};

struct SShaderItem
{
    IShader* m_pShader;
    IRenderShaderResources* m_pShaderResources;
    int32 m_nTechnique;
    uint32 m_nPreprocessFlags;

    SShaderItem()
    {
        m_pShader = NULL;
        m_pShaderResources = NULL;
        m_nTechnique = -1;
        m_nPreprocessFlags = 1;
    }
    SShaderItem(IShader* pSH)
    {
        m_pShader = pSH;
        m_pShaderResources = NULL;
        m_nTechnique = -1;
        m_nPreprocessFlags = 1;
    }
    SShaderItem(IShader* pSH, IRenderShaderResources* pRS)
    {
        m_pShader = pSH;
        m_pShaderResources = pRS;
        m_nTechnique = -1;
        m_nPreprocessFlags = 1;
    }
    SShaderItem(IShader* pSH, IRenderShaderResources* pRS, int nTechnique)
    {
        m_pShader = pSH;
        m_pShaderResources = pRS;
        m_nTechnique = nTechnique;
        m_nPreprocessFlags = 1;
    }

    uint32 PostLoad();
    bool Update();
    bool RefreshResourceConstants();
    // Note:
    //   If you change this function please check bTransparent variable in CRenderMesh::Render().
    // See also:
    //   CRenderMesh::Render()
    bool IsZWrite() const
    {
        IShader* pSH = m_pShader;
        if (pSH->GetFlags() & (EF_NODRAW | EF_DECAL))
        {
            return false;
        }
        if (pSH->GetFlags2() & EF2_FORCE_ZPASS)
        {
            return true;
        }
        if (m_pShaderResources && m_pShaderResources->IsTransparent())
        {
            return false;
        }
        return true;
    }
    inline struct SShaderTechnique* GetTechnique() const;
    bool IsMergable(SShaderItem& PrevSI);

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_pShader);
        pSizer->AddObject(m_pShaderResources);
    }
};


#include <IRenderMesh.h>  // <> required for Interfuscator

struct IAnimNode;

struct ILightAnimWrapper
    : public _i_reference_target_t
{
public:
    virtual bool Resolve() = 0;
    IAnimNode* GetNode() const { return m_pNode; }

protected:
    ILightAnimWrapper(const char* name)
        : m_name(name)
        , m_pNode(0) {}
    virtual ~ILightAnimWrapper() {}

protected:
    AZStd::string m_name;
    IAnimNode* m_pNode;
};


#define MAX_RECURSION_LEVELS 2

#define DECAL_HAS_NORMAL_MAP    (1 << 0)
#define DECAL_STATIC                    (1 << 1)
#define DECAL_HAS_SPECULAR_MAP (1 << 2)


// Summary:
//   Runtime shader flags for HW skinning.
enum EHWSkinningRuntimeFlags
{
    eHWS_MotionBlured = 0x04,
    eHWS_Skinning_DQ_Linear = 0x08,     // Convert dual-quaternions to matrices on the GPU
    eHWS_Skinning_Matrix = 0x10,        // Pass float3x4 skinning matrices directly to the GPU
};

// Enum of data types that can be used as bones on GPU for skinning
enum EBoneTypes
{
    eBoneType_DualQuat = 0,
    eBoneType_Matrix,
    eBoneType_Count,
};


#include <CryCommon/StaticInstance.h>
