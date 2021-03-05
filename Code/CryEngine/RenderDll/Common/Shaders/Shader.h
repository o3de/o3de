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

// Description : Shaders declarations.


#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_SHADERS_SHADER_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_SHADERS_SHADER_H
#pragma once


#include "../Defs.h"
#include "Vertex.h"

#include <CryName.h>
#include <IShader.h>
#include "ShaderResources.h"

// bump this value up if you want to invalidate shader cache (e.g. changed some code or .ext file)
// #### VIP NOTE ####: DON'T USE MORE THAN ONE DECIMAL PLACE!!!! else it doesn't work...
#define FX_CACHE_VER       10.4f
#define FX_SER_CACHE_VER   1.3f  // Shader serialization version (FX_CACHE_VER + FX_SER_CACHE_VER)

// Maximum 1 digit here
// The version determines the parse logic in the shader cache gen, these values cannot overlap
#define SHADER_LIST_VER 4
#define SHADER_SERIALISE_VER (SHADER_LIST_VER + 1)

//#define SHADER_NO_SOURCES 1 // If this defined all binary shaders (.fxb) should be located in Game folder (not user)
#if !defined(NULL_RENDERER)
#define SHADERS_SERIALIZING 1 // Enables shaders serializing (Export/Import) to/from .fxb files
#endif

struct SShaderPass;
class CShader;
class CRendElementBase;
class CResFile;
struct SEnvTexture;
struct SParserFrame;
struct SPreprocessTree;
struct SEmptyCombination;
struct SShaderCombination;
struct SShaderCache;
struct SShaderDevCache;
struct SCGParam;
struct SSFXParam;
struct SSFXSampler;
struct SSFXTexture;


enum eCompareFunc
{
    eCF_Disable,
    eCF_Never,
    eCF_Less,
    eCF_Equal,
    eCF_LEqual,
    eCF_Greater,
    eCF_NotEqual,
    eCF_GEqual,
    eCF_Always
};


struct SPair
{
    string m_szMacroName;
    string m_szMacro;
    uint32 m_nMask;
};

#if defined(MOBILE)
    #define GEOMETRYSHADER_SUPPORT  false
#else
    #define GEOMETRYSHADER_SUPPORT  true
#endif

//------------------------------------------------------------------------------
// SFX structures are the structures gathered from the shader during shader parsing 
// and associated later on to a binding slot / buffer.
// They represent constants, textures and samplers.
//------------------------------------------------------------------------------
struct SFXBaseParam
{
    CCryNameR               m_Name;                 // Parameter name
    std::vector <uint32>    m_dwName;
    uint32                  m_nFlags;
    short                   m_nArray;               // Number of parameters
    CCryNameR               m_Annotations;          // Additional parameters (between <>)
    CCryNameR               m_Semantic;             // Parameter semantic type (after ':')
    CCryNameR               m_Values;               // Parameter values (after '=')
    byte                    m_eType;                // Type per usage

    // [Shader System] register offset per shader stage (class) - VS, PS, GS...
    // This needs to be unified for all stages (and renamed as m_RegisterOffset)
    short                   m_Register[eHWSC_Num]; 

    SFXBaseParam()
    {
        m_nArray = 0;
        m_nFlags = 0;
        for (int i = 0; i < eHWSC_Num; i++)
        {
            m_Register[i] = 10000;
        }
    }

    uint32   GetFlags() { return m_nFlags; }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_dwName);
    }
};

//------------------------------------------------------------------------------
// An SFXParam is the constant data gathered from the shader parsing.
// Example of usage - In Matrix 3x4: m_nParams = 3, m_nComps = 4
// Needs some more refactor to fully use SFXBaseParam.
//------------------------------------------------------------------------------
struct SFXParam : SFXBaseParam
{
    short       m_RegisterCount;
    short       m_ComponentCount;
    int8        m_BindingSlot;              // the CB slot

    // [Shaders System] - the following needs to be removed as part of the unified offset.
    // The next two parameters are only valid after the gather stage for final parameters
    AZ::u8      m_OffsetStageSetter;        // which stage set the offset
    AZ::u8      m_StagesUsage;              // Adding visibility to who's using the param

    SFXParam()
    {
        m_RegisterCount = 0;
        m_ComponentCount = 0;
        m_BindingSlot = -1;
        m_OffsetStageSetter = eHWSC_Vertex;
        m_StagesUsage = (1 << eHWSC_Vertex);
    }

    ~SFXParam() {}

    void GetParamComp(uint32 nOffset, CryFixedStringT<128>& param);
    void GetCompName(uint32 nId, CryFixedStringT<128>& name);
    string GetValueForName(const char* szName, EParamType& eType);
    void PostLoad(class CParserBin& Parser, SParserFrame& Name, SParserFrame& Annotations, SParserFrame& Values, SParserFrame& Assign);
    void PostLoad();
    bool Export(SShaderSerializeContext& SC);
    bool Import(SShaderSerializeContext& SC, SSFXParam* pPR);

    uint32 Size()
    {
        uint32 nSize = sizeof(SFXParam);
        nSize += sizeofVector(m_dwName);
        return nSize;
    }

    inline bool operator == (const SFXParam& m) const
    {
        if (m_Name == m.m_Name && m_Annotations == m.m_Annotations && m_Semantic == m.m_Semantic && m_Values == m.m_Values &&
            m_RegisterCount == m.m_RegisterCount && m_ComponentCount == m.m_ComponentCount && m_nFlags == m.m_nFlags && m_Register[0] == m.m_Register[0] && m_Register[1] == m.m_Register[1] &&
            m_eType == m.m_eType)
        {
            return true;
        }
        return false;
    }
};

//------------------------------------------------------------------------------
// An SFX structure is the structure gathered from the shader during
// parsing and associated later on.   
//------------------------------------------------------------------------------
struct SFXSampler : SFXBaseParam
{
    int         m_nTexState;

    SFXSampler()
    {
        m_nTexState = -1;
    }
    ~SFXSampler()    {}

    void PostLoad(class CParserBin& Parser, SParserFrame& Name, SParserFrame& Annotations, SParserFrame& Values, SParserFrame& Assign);
    void PostLoad();
    bool Export(SShaderSerializeContext& SC);
    bool Import(SShaderSerializeContext& SC, SSFXSampler* pPR);

    uint32 Size()
    {
        uint32 nSize = sizeof(SFXSampler);
        nSize += sizeofVector(m_dwName);
        return nSize;
    }

    inline bool operator == (const SFXSampler& m) const
    {
        if (m_Name == m.m_Name && m_Annotations == m.m_Annotations && m_Semantic == m.m_Semantic && m_Values == m.m_Values &&
            m_nArray == m.m_nArray && m_nFlags == m.m_nFlags && m_Register[0] == m.m_Register[0] && m_Register[1] == m.m_Register[1] &&
            m_eType == m.m_eType && m_nTexState == m.m_nTexState)
        {
            return true;
        }
        return false;
    }
};

//------------------------------------------------------------------------------
// An SFX structure is the structure gathered from the shader during
// parsing and associated later on.   
// SFXTexture - This structure contains a meta data gathered during shader parsing.
// It doesn't contain the actual texture data and doesn't apply to the binding directly 
// but used as the data associated with the SCGTexture binding structure.
//------------------------------------------------------------------------------
struct SFXTexture : SFXBaseParam
{
    uint32      m_nTexFlags;
    string      m_szTexture;    // Texture source name
    string      m_szUIName;     // UI name
    string      m_szUIDesc;     // UI description
    bool        m_bSRGBLookup;  // Lookup
    byte        m_Type;         // Data type (float, float4, etc)

    SFXTexture()
    {
        m_bSRGBLookup = false;
        m_Type = 0;
        m_nTexFlags = 0;
    }

    ~SFXTexture()    {}

    uint32 GetTexFlags() { return m_nTexFlags; }
    void PostLoad(class CParserBin& Parser, SParserFrame& Name, SParserFrame& Annotations, SParserFrame& Values, SParserFrame& Assign);
    void PostLoad();
    bool Export(SShaderSerializeContext& SC);
    bool Import(SShaderSerializeContext& SC, SSFXTexture* pPR);

    uint32 Size()
    {
        uint32 nSize = sizeof(SFXTexture);
        //nSize += m_Name.capacity();
        nSize += sizeofVector(m_dwName);
        //nSize += m_Values.capacity();
        return nSize;
    }

    inline bool operator == (const SFXTexture& m) const
    {
        if (m_Name == m.m_Name && m_Annotations == m.m_Annotations && m_Semantic == m.m_Semantic && m_Values == m.m_Values &&
            m_nArray == m.m_nArray && m_nFlags == m.m_nFlags && m_Register[0] == m.m_Register[0] && m_Register[1] == m.m_Register[1] &&
            m_eType == m.m_eType && m_bSRGBLookup == m.m_bSRGBLookup && m_szTexture == m.m_szTexture)
        {
            return true;
        }
        return false;
    }
};

//------------------------------------------------------------------------------
struct STokenD
{
    //std::vector<int> Offsets;
    uint32 Token;
    string SToken;
    unsigned Size() { return sizeof(STokenD) /*+ sizeofVector(Offsets)*/ + SToken.capacity(); }
    void GetMemoryUsage(ICrySizer* pSizer) const { pSizer->AddObject(SToken); }
};
typedef AZStd::vector<STokenD, AZ::StdLegacyAllocator> FXShaderToken;
typedef FXShaderToken::iterator FXShaderTokenItor;

struct SFXStruct
{
    string m_Name;
    string m_Struct;
    SFXStruct()
    {
    }
};

enum ETexFilter
{
    eTEXF_None,
    eTEXF_Point,
    eTEXF_Linear,
    eTEXF_Anisotropic,
};

//=============================================================================
// Vertex programms / Vertex shaders (VP/VS)

static _inline float* sfparam(Vec3 param)
{
    static float sparam[4];
    sparam[0] = param.x;
    sparam[1] = param.y;
    sparam[2] = param.z;
    sparam[3] = 1.0f;

    return &sparam[0];
}

static _inline float* sfparam(float param)
{
    static float sparam[4];
    sparam[0] = param;
    sparam[1] = 0;
    sparam[2] = 0;
    sparam[3] = 1.0f;
    return &sparam[0];
}

static _inline float* sfparam(float param0, float param1, float param2, float param3)
{
    static float sparam[4];
    sparam[0] = param0;
    sparam[1] = param1;
    sparam[2] = param2;
    sparam[3] = param3;
    return &sparam[0];
}

_inline char* sGetFuncName(const char* pFunc)
{
    static char func[128];
    const char* b = pFunc;
    if (*b == '[')
    {
        const char* s = strchr(b, ']');
        if (s)
        {
            b = s + 1;
        }
        while (*b <= 0x20)
        {
            b++;
        }
    }
    while (*b > 0x20)
    {
        b++;
    }
    while (*b <= 0x20)
    {
        b++;
    }
    int n = 0;
    while (*b > 0x20 && *b != '(')
    {
        func[n++] = *b++;
    }
    func[n] = 0;

    return func;
}

enum ERenderOrder
{
    eRO_PreProcess,
    eRO_PostProcess,
    eRO_PreDraw
};

enum ERTUpdate
{
    eRTUpdate_Unknown,
    eRTUpdate_Always,
    eRTUpdate_WaterReflect
};

struct SHRenderTarget
    : public IRenderTarget
{
    int m_nRefCount;
    ERenderOrder m_eOrder;
    int m_nProcessFlags;   // FSPR_ flags
    string m_TargetName;
    int m_nWidth;
    int m_nHeight;
    ETEX_Format m_eTF;
    int m_nIDInPool;
    ERTUpdate m_eUpdateType;
    CTexture* m_pTarget[2];
    bool m_bTempDepth;
    ColorF m_ClearColor;
    float m_fClearDepth;
    uint32 m_nFlags;
    uint32 m_nFilterFlags;
    int m_refSamplerID;

    SHRenderTarget()
    {
        m_nRefCount = 1;
        m_eOrder = eRO_PreProcess;
        m_pTarget[0] = NULL;
        m_pTarget[1] = NULL;
        m_bTempDepth = true;
        m_ClearColor = Col_Black;
        m_fClearDepth = 1.f;
        m_nFlags = 0;
        m_nFilterFlags = 0xffffffff;
        m_nProcessFlags = 0;
        m_nIDInPool = -1;
        m_nWidth = 256;
        m_nHeight = 256;
        m_eTF = eTF_R8G8B8A8;
        m_eUpdateType = eRTUpdate_Unknown;
        m_refSamplerID = -1;
    }
    virtual void Release()
    {
        m_nRefCount--;
        if (m_nRefCount)
        {
            return;
        }
        delete this;
    }
    virtual void AddRef()
    {
        m_nRefCount++;
    }
    SEnvTexture* GetEnv2D();
    SEnvTexture* GetEnvCM();

    void GetMemoryUsage(ICrySizer* pSizer) const;
};

//=============================================================================
// Hardware shaders

#define SHADER_BIND_TEXTURE 0x2000
#define SHADER_BIND_SAMPLER 0x4000

//=============================================================================

struct SShaderCacheHeaderItem
{
    AZ::u32 m_nVertexFormat;
    byte m_Class;
    byte m_nInstBinds;
    byte m_StreamMask_Stream;
    uint32 m_CRC32;
    uint16 m_StreamMask_Decl;
    int16  m_nInstructions;
    SShaderCacheHeaderItem()
    {
        memset(this, 0, sizeof(SShaderCacheHeaderItem));
    }
    AUTO_STRUCT_INFO
};

#define MAX_VAR_NAME 512
struct SShaderCacheHeaderItemVar
{
    int m_Reg;
    short m_nCount;
    char m_Name[MAX_VAR_NAME];
    SShaderCacheHeaderItemVar()
    {
        memset(this, 0, sizeof(SShaderCacheHeaderItemVar));
    }
};

struct SCompressedData
{
    byte* m_pCompressedShader;
    uint32 m_nSizeCompressedShader;
    uint32 m_nSizeDecompressedShader;

    SCompressedData()
    {
        m_pCompressedShader = NULL;
        m_nSizeCompressedShader = 0;
        m_nSizeDecompressedShader = 0;
    }
    int Size()
    {
        int nSize = sizeof(SCompressedData);
        if (m_pCompressedShader)
        {
            nSize += m_nSizeCompressedShader;
        }
        return nSize;
    }
    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        //pSizer->AddObject(this, sizeof(SCompressedData));
        if (m_pCompressedShader)
        {
            pSizer->AddObject(m_pCompressedShader, m_nSizeCompressedShader);
        }
    }
};

typedef AZStd::unordered_map<int, struct SD3DShader*, AZStd::hash<int>, AZStd::equal_to<int>, AZ::StdLegacyAllocator> FXDeviceShader;
typedef FXDeviceShader::iterator FXDeviceShaderItor;

typedef AZStd::unordered_map<int, SCompressedData, AZStd::hash<int>, AZStd::equal_to<int>, AZ::StdLegacyAllocator> FXCompressedShader;
typedef FXCompressedShader::iterator FXCompressedShaderItor;
typedef AZStd::unordered_map<CCryNameTSCRC, int, AZStd::hash<CCryNameTSCRC>, AZStd::equal_to<CCryNameTSCRC>, AZ::StdLegacyAllocator> FXCompressedShaderRemap;
typedef FXCompressedShaderRemap::iterator FXCompressedShaderRemapItor;
struct SHWActivatedShader
{
    bool m_bPersistent;
    FXCompressedShader m_CompressedShaders;
    FXCompressedShaderRemap m_Remap;
    ~SHWActivatedShader();

    int Size();
    void GetMemoryUsage(ICrySizer* pSizer) const;
};
typedef AZStd::unordered_map<CCryNameTSCRC, SHWActivatedShader*, AZStd::hash<CCryNameTSCRC>, AZStd::equal_to<CCryNameTSCRC>, AZ::StdLegacyAllocator> FXCompressedShaders;
typedef FXCompressedShaders::iterator FXCompressedShadersItor;

#define CACHE_READONLY 0
#define CACHE_USER     1

struct SOptimiseStats
{
    int nEntries;
    int nUniqueEntries;
    int nSizeUncompressed;
    int nSizeCompressed;
    int nTokenDataSize;
    int nDirDataSize;
    SOptimiseStats()
    {
        nEntries = 0;
        nUniqueEntries = 0;
        nSizeUncompressed = 0;
        nSizeCompressed = 0;
        nTokenDataSize = 0;
        nDirDataSize = 0;
    }
};

typedef AZStd::unordered_map<CCryNameR, SShaderCache*, AZStd::hash<CCryNameR>, AZStd::equal_to<CCryNameR>, AZ::StdLegacyAllocator> FXShaderCache;
typedef FXShaderCache::iterator FXShaderCacheItor;

typedef AZStd::unordered_map<CCryNameR, SShaderDevCache*, AZStd::hash<CCryNameR>, AZStd::equal_to<CCryNameR>, AZ::StdLegacyAllocator> FXShaderDevCache;
typedef FXShaderDevCache::iterator FXShaderDevCacheItor;

typedef AZStd::unordered_map<string, uint32, AZStd::hash<string>, AZStd::equal_to<string>, AZ::StdLegacyAllocator> FXShaderCacheNames;
typedef FXShaderCacheNames::iterator FXShaderCacheNamesItor;


//====================================================================
// HWShader run-time flags

enum EHWSRMaskBit
{
    HWSR_FOG = 0,

    HWSR_AMBIENT,

    HWSR_ALPHATEST,
    HWSR_ALPHABLEND,

    HWSR_HDR_MODE,      // deprecated: this flag is redundant and can be dropped, since rendering always HDR since CE3
    HWSR_HDR_ENCODE,

    HWSR_INSTANCING_ATTR,

    HWSR_VERTEX_VELOCITY,
    HWSR_SKINNING_DUAL_QUAT,
    HWSR_SKINNING_DQ_LINEAR,
    HWSR_SKINNING_MATRIX,

    HWSR_OBJ_IDENTITY,
    HWSR_DETAIL_OVERLAY,
    HWSR_NEAREST,
    HWSR_NOZPASS,
    HWSR_DISSOLVE,
    HWSR_APPLY_TOON_SHADING,
    HWSR_NO_TESSELLATION,
    HWSR_PER_INSTANCE_CB_TEMP,

    HWSR_QUALITY,
    HWSR_QUALITY1,

    HWSR_SAMPLE0,
    HWSR_SAMPLE1,
    HWSR_SAMPLE2,
    HWSR_SAMPLE3,
    HWSR_SAMPLE4,
    HWSR_SAMPLE5,

    HWSR_DEBUG0,
    HWSR_DEBUG1,
    HWSR_DEBUG2,
    HWSR_DEBUG3,

    HWSR_CUBEMAP0,

    HWSR_DECAL_TEXGEN_2D,

    HWSR_SHADOW_MIXED_MAP_G16R16,
    HWSR_HW_PCF_COMPARE,
    HWSR_SHADOW_JITTERING,
    HWSR_POINT_LIGHT,
    HWSR_LIGHT_TEX_PROJ,

    HWSR_PARTICLE_SHADOW,
    HWSR_SOFT_PARTICLE,
    HWSR_OCEAN_PARTICLE,
    HWSR_GLOBAL_ILLUMINATION,
    HWSR_ANIM_BLEND,
    HWSR_ENVIRONMENT_CUBEMAP,
    HWSR_MOTION_BLUR,

    HWSR_SPRITE,

    HWSR_LIGHTVOLUME0,
    HWSR_LIGHTVOLUME1,

    HWSR_TILED_SHADING,

    HWSR_VOLUMETRIC_FOG,

    HWSR_REVERSE_DEPTH,
    HWSR_GPU_PARTICLE_SHADOW_PASS,
    HWSR_GPU_PARTICLE_DEPTH_COLLISION,
    HWSR_GPU_PARTICLE_TURBULENCE,
    HWSR_GPU_PARTICLE_UV_ANIMATION,
    HWSR_GPU_PARTICLE_NORMAL_MAP,
    HWSR_GPU_PARTICLE_GLOW_MAP,
    HWSR_GPU_PARTICLE_CUBEMAP_DEPTH_COLLISION,
    HWSR_GPU_PARTICLE_WRITEBACK_DEATH_LOCATIONS,
    HWSR_GPU_PARTICLE_TARGET_ATTRACTION,
    HWSR_GPU_PARTICLE_SHAPE_ANGLE,
    HWSR_GPU_PARTICLE_SHAPE_BOX,
    HWSR_GPU_PARTICLE_SHAPE_POINT,
    HWSR_GPU_PARTICLE_SHAPE_CIRCLE,
    HWSR_GPU_PARTICLE_SHAPE_SPHERE,
    HWSR_GPU_PARTICLE_WIND,

    HWSR_MULTI_LAYER_ALPHA_BLEND,
    HWSR_ADDITIVE_BLENDING,
    HWSR_APPLY_SSDO,
    HWSR_FOG_VOLUME_HIGH_QUALITY_SHADER,


    HWSR_SRGB0,
    HWSR_SRGB1,
    HWSR_SRGB2,

    HWSR_DEPTHFIXUP,
    HWSR_DEFERRED_RENDER_TARGET_OPTIMIZATION,
    HWSR_SLIM_GBUFFER,

    HWSR_MAX
};

extern uint64 g_HWSR_MaskBit[HWSR_MAX];

// HWShader global flags (m_Flags)
#define HWSG_SUPPORTS_LIGHTING    0x20
#define HWSG_SUPPORTS_MULTILIGHTS 0x40
#define HWSG_SUPPORTS_MODIF  0x80
#define HWSG_SUPPORTS_VMODIF 0x100
#define HWSG_WASGENERATED    0x200
#define HWSG_NOSPECULAR      0x400
#define HWSG_SYNC            0x800
#define HWSG_CACHE_USER      0x1000
//#define HWSG_AUTOENUMTC      0x1000
#define HWSG_UNIFIEDPOS      0x2000
#define HWSG_DEFAULTPOS      0x4000
#define HWSG_PROJECTED       0x8000
#define HWSG_NOISE           0x10000
#define HWSG_PRECACHEPHASE   0x20000
#define HWSG_FP_EMULATION    0x40000

// HWShader per-instance Modificator flags (SHWSInstance::m_MDMask)
// Vertex shader specific

// Texture projected flags
#define HWMD_TEXCOORD_PROJ                          0x1
// Texture transform flag
#define HWMD_TEXCOORD_MATRIX                        0x100
// Object linear texgen flag
#define HWMD_TEXCOORD_GEN_OBJECT_LINEAR_DIFFUSE         0x1000
#define HWMD_TEXCOORD_GEN_OBJECT_LINEAR_EMITTANCE       0x2000
#define HWMD_TEXCOORD_GEN_OBJECT_LINEAR_EMITTANCE_MULT  0x4000
#define HWMD_TEXCOORD_GEN_OBJECT_LINEAR_DETAIL          0x8000
#define HWMD_TEXCOORD_GEN_OBJECT_LINEAR_CUSTOM          0x10000


#define HWMD_TEXCOORD_FLAG_MASK    (0xfffff000 | 0xf00)

// HWShader per-instance vertex modificator flags (SHWSInstance::m_MDVMask)
// Texture projected flags (4 bits)
#define HWMDV_TYPE      0


// HWShader input flags (passed via mfSet function)
#define HWSF_SETPOINTERSFORSHADER 1
#define HWSF_SETPOINTERSFORPASS   2
#define HWSF_PRECACHE             4
#define HWSF_SETTEXTURES          8
#define HWSF_FAKE                 0x10

#define HWSF_INSTANCED            0x20
#define HWSF_NEXT                 0x100
#define HWSF_PRECACHE_INST        0x200
#define HWSF_STORECOMBINATION     0x400
#define HWSF_STOREDATA            0x800

// Static flags
enum EHWSSTFlag
{
    HWSST_Invalid = -1,
#undef FX_STATIC_FLAG
#define FX_STATIC_FLAG(flag) HWSST_##flag,
#include "ShaderStaticFlags.inl"
    HWSST_MAX
};

class CHWShader
    : public CBaseResource
{
    static CCryNameTSCRC s_sClassNameVS;
    static CCryNameTSCRC s_sClassNamePS;

public:
    EHWShaderClass m_eSHClass;
    //EHWSProfile m_eHWProfile;
    SShaderCache* m_pGlobalCache;

    static struct SD3DShader* s_pCurPS;
    static struct SD3DShader* s_pCurVS;
    static struct SD3DShader* s_pCurGS;
    static struct SD3DShader* s_pCurDS;
    static struct SD3DShader* s_pCurHS;
    static struct SD3DShader* s_pCurCS;

    string m_Name;
    string m_NameSourceFX;
    string m_EntryFunc;
    uint64 m_nMaskAnd_RT;
    uint64 m_nMaskOr_RT;
    uint64 m_nMaskGenShader;        // Masked/Optimised m_nMaskGenFX for this specific HW shader
    uint64 m_nMaskGenFX;            // FX Shader should be parsed with this flags
    uint64 m_nMaskSetFX;            // AffectMask GL for parser tree
    uint64 m_maskGenStatic;         // Mask for global static flags used for generating the shader.

    uint32 m_nPreprocessFlags;
    int m_nFrame;
    int m_nFrameLoad;
    int m_Flags;
    uint32 m_CRC32;
    uint32 m_dwShaderType;

public:
    CHWShader()
    {
        m_nFrame = 0;
        m_nFrameLoad = 0;
        m_Flags = 0;
        m_nMaskGenShader = 0;
        m_nMaskAnd_RT = -1;
        m_nMaskOr_RT = 0;
        m_CRC32 = 0;
        m_nMaskGenFX = 0;
        m_nMaskSetFX = 0;
        m_eSHClass = eHWSC_Vertex;
        m_pGlobalCache = NULL;
    }
    virtual ~CHWShader() {}

    //EHWSProfile mfGetHWProfile(uint32 nFlags);

#if !defined (NULL_RENDERER)

    static CHWShader* mfForName(const char* name, const char* nameSource, uint32 CRC32, const char* szEntryFunc, EHWShaderClass eClass, TArray<uint32>& SHData, FXShaderToken* pTable, uint32 dwType, CShader* pFX, uint64 nMaskGen = 0, uint64 nMaskGenFX = 0);
#endif
    static void mfReloadScript(const char* szPath, const char* szName, int nFlags, uint64 nMaskGen);
    static void mfFlushPendedShadersWait(int nMaxAllowed);
    inline const char* GetName()
    {
        return m_Name.c_str();
    }
    virtual int  Size() = 0;
    virtual void GetMemoryUsage(ICrySizer* Sizer) const = 0;
    virtual void mfReset([[maybe_unused]] uint32 CRC32) {}
    virtual bool mfSetV(int nFlags = 0) = 0;
    virtual bool mfAddEmptyCombination(CShader* pSH, uint64 nRT, uint64 nGL, uint32 nLT) = 0;
    virtual bool mfStoreEmptyCombination(SEmptyCombination& Comb) = 0;
    virtual const char* mfGetCurScript() {return NULL; }
    virtual const char* mfGetEntryName() = 0;
    virtual void mfUpdatePreprocessFlags(SShaderTechnique* pTech) = 0;
    virtual bool mfFlushCacheFile() = 0;
    virtual bool mfPrecache(SShaderCombination& cmb, bool bForce, bool bFallback, bool bCompressedOnly, CShader* pSH, CShaderResources* pRes) = 0;

    virtual bool Export(SShaderSerializeContext& SC) = 0;
    static CHWShader* Import(SShaderSerializeContext& SC, int nOffs, uint32 CRC32, CShader* pSH);

    // Vertex shader specific functions
    virtual AZ::Vertex::Format mfVertexFormat(bool& bUseTangents, bool& bUseLM, bool& bUseHWSkin, bool& bUseVertexVelocity) = 0;

    virtual const char* mfGetActivatedCombinations(bool bForLevel) = 0;

    static const char* mfProfileString(EHWShaderClass eClass);
    static const char* mfClassString(EHWShaderClass eClass);
    static EHWShaderClass mfStringProfile(const char* profile);
    static EHWShaderClass mfStringClass(const char* szClass);
    static void mfGenName(uint64 GLMask, uint64 RTMask, uint32 LightMask, uint32 MDMask, uint32 MDVMask, uint64 PSS, uint64 STMask, EHWShaderClass eClass, char* dstname, int nSize, byte bType);

    static void mfCleanupCache();

    static CCryNameTSCRC mfGetClassName(EHWShaderClass eClass)
    {
        if (eClass == eHWSC_Vertex)
        {
            return s_sClassNameVS;
        }
        else
        {
            return s_sClassNamePS;
        }
    }

    static const char* GetCurrentShaderCombinations(bool bForLevel);
    static bool PreactivateShaders();
    static void RT_PreactivateShaders();

    static byte* mfIgnoreRemapsFromCache(int nRemaps, byte* pP);
    static byte* mfIgnoreBindsFromCache(int nParams, byte* pP);
#if !defined(CONSOLE)
    static bool mfOptimiseCacheFile(SShaderCache* pCache, bool bForce, SOptimiseStats* Stats);
#endif
    static SShaderDevCache* mfInitDevCache(const char* name, CHWShader* pSH);
    static SShaderCache* mfInitCache(const char* name, CHWShader* pSH, bool bCheckValid, uint32 CRC32, bool bReadOnly, bool bAsync = false);
    static bool _OpenCacheFile(float fVersion, SShaderCache* pCache, CHWShader* pSH, bool bCheckValid, uint32 CRC32, int nCache, CResFile* pRF, bool bReadOnly);
    static bool mfOpenCacheFile(const char* szName, float fVersion, SShaderCache* pCache, CHWShader* pSH, bool bCheckValid, uint32 CRC32, bool bReadOnly);
    static void mfValidateTokenData(CResFile* pRF);
    static FXShaderCacheNames m_ShaderCacheList;
    static FXShaderCache m_ShaderCache;

    // Import/Export
    static bool ImportSamplers(SShaderSerializeContext& SC, struct SCHWShader* pSHW, byte*& pData, std::vector<STexSamplerRT>& Samplers);
    static bool ImportParams(SShaderSerializeContext& SC, SCHWShader* pSHW, byte*& pData, std::vector<SFXParam>& Params);

    static FXCompressedShaders m_CompressedShaders;
};

_inline void SortLightTypes(int Types[4], int nCount)
{
    switch (nCount)
    {
    case 2:
        if (Types[0] > Types[1])
        {
            Exchange(Types[0], Types[1]);
        }
        break;
    case 3:
        if (Types[0] > Types[1])
        {
            Exchange(Types[0], Types[1]);
        }
        if (Types[0] > Types[2])
        {
            Exchange(Types[0], Types[2]);
        }
        if (Types[1] > Types[2])
        {
            Exchange(Types[1], Types[2]);
        }
        break;
    case 4:
    {
        for (int i = 0; i < 4; i++)
        {
            for (int j = i; j < 4; j++)
            {
                if (Types[i] > Types[j])
                {
                    Exchange(Types[i], Types[j]);
                }
            }
        }
    }
    break;
    }
}

//=========================================================================
// Dynamic lights evaluating via shaders

enum ELightStyle
{
    eLS_Intensity,
    eLS_RGB,
};

enum ELightMoveType
{
    eLMT_Wave,
    eLMT_Patch,
};

struct SLightMove
{
    ELightMoveType m_eLMType;
    SWaveForm m_Wave;
    Vec3 m_Dir;
    float m_fSpeed;

    int Size()
    {
        int nSize = sizeof(SLightMove);
        return nSize;
    }
};

struct SLightStyleKeyFrame
{
    ColorF cColor; // xyz: color, w: spec mult
    Vec3 vPosOffset;    // position offset

    SLightStyleKeyFrame()
    {
        cColor = ColorF(Col_Black);
        vPosOffset = Vec3(ZERO);
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};

class CLightStyle
{
public:

    CLightStyle()
        :  m_Color(Col_White)
        , m_vPosOffset(ZERO)
        , m_LastTime(0.0f)
        , m_TimeIncr(60.0f)
        , m_bRandColor(0)
        , m_bRandIntensity(0)
        , m_bRandPosOffset(0)
        , m_bRandSpecMult(0)
    {
    }

    static TArray <CLightStyle*> s_LStyles;
    TArray<SLightStyleKeyFrame> m_Map;

    ColorF m_Color;             // xyz: color, w: spec mult
    Vec3 m_vPosOffset;      // position offset

    float m_TimeIncr;
    float m_LastTime;

    uint8 m_bRandColor : 1;
    uint8 m_bRandIntensity : 1;
    uint8 m_bRandPosOffset : 1;
    uint8 m_bRandSpecMult : 1;

    int Size()
    {
        int nSize = sizeof(CLightStyle);
        nSize += m_Map.GetMemoryUsage();
        return nSize;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->Add(*this);
        pSizer->AddObject(m_Map);
    }

    static _inline CLightStyle* mfGetStyle(uint32 nStyle, float fTime)
    {
        if (nStyle >= s_LStyles.Num() || !s_LStyles[nStyle])
        {
            return NULL;
        }
        s_LStyles[nStyle]->mfUpdate(fTime);
        return s_LStyles[nStyle];
    }

    void mfUpdate(float fTime);
};


//=========================================================================
// HW Shader Layer

#define SHPF_AMBIENT         0x100
#define SHPF_HASLM           0x200
#define SHPF_SHADOW          0x400
#define SHPF_RADIOSITY       0x800
#define SHPF_ALLOW_SPECANTIALIAS 0x1000
#define SHPF_BUMP            0x2000
#define SHPF_NOMATSTATE      0x4000
#define SHPF_FORCEZFUNC      0x8000


// Shader pass definition for HW shaders
struct SShaderPass
{
    uint32 m_RenderState;        // Render state flags
    signed char m_eCull;
    uint8 m_AlphaRef;

    uint16 m_PassFlags;            // Different usefull Pass flags (SHPF_)

    CHWShader* m_VShader;       // Pointer to the vertex shader for the current pass
    CHWShader* m_PShader;       // Pointer to fragment shader
    CHWShader* m_GShader;       // Pointer to the geometry shader for the current pass
    CHWShader* m_HShader;       // Pointer to the hull shader for the current pass
    CHWShader* m_DShader;       // Pointer to the domain shader for the current pass
    CHWShader* m_CShader;       // Pointer to the compute shader for the current pass
    SShaderPass();

    int Size()
    {
        int nSize = sizeof(SShaderPass);
        return nSize;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_VShader);
        pSizer->AddObject(m_PShader);
        pSizer->AddObject(m_GShader);
        pSizer->AddObject(m_HShader);
        pSizer->AddObject(m_DShader);
        pSizer->AddObject(m_CShader);
    }
    void mfFree()
    {
        SAFE_RELEASE(m_VShader);
        SAFE_RELEASE(m_PShader);
        SAFE_RELEASE(m_GShader);
        SAFE_RELEASE(m_HShader);
        SAFE_RELEASE(m_DShader);
        SAFE_RELEASE(m_CShader);
    }

    void AddRefsToShaders()
    {
        if (m_VShader)
        {
            m_VShader->AddRef();
        }
        if (m_PShader)
        {
            m_PShader->AddRef();
        }
        if (m_GShader)
        {
            m_GShader->AddRef();
        }
        if (m_DShader)
        {
            m_DShader->AddRef();
        }
        if (m_HShader)
        {
            m_HShader->AddRef();
        }
        if (m_CShader)
        {
            m_CShader->AddRef();
        }
    }

private:
    SShaderPass& operator = (const SShaderPass& sl);
};

//===================================================================================
// Hardware Stage for HW only Shaders

#define FHF_FIRSTLIGHT   8
#define FHF_FORANIM      0x10
#define FHF_TERRAIN      0x20
#define FHF_NOMERGE      0x40
#define FHF_DETAILPASS   0x80
#define FHF_LIGHTPASS    0x100
#define FHF_FOGPASS      0x200
#define FHF_PUBLIC       0x400
#define FHF_NOLIGHTS     0x800
#define FHF_POSITION_INVARIANT 0x1000
#define FHF_RE_CLOUD     0x20000
#define FHF_TRANSPARENT  0x40000
#define FHF_WASZWRITE    0x80000
#define FHF_USE_GEOMETRY_SHADER 0x100000
#define FHF_USE_HULL_SHADER     0x200000
#define FHF_USE_DOMAIN_SHADER   0x400000
#define FHF_RE_LENSOPTICS 0x1000000


struct SShaderTechnique
{
    CShader*  m_shader;
    CCryNameR m_NameStr;
    CCryNameTSCRC m_NameCRC;
    TArray <SShaderPass> m_Passes;       // General passes
    int m_Flags;                         // Different flags (FHF_)
    uint32 m_nPreprocessFlags;
    int8 m_nTechnique[TTYPE_MAX];        // Next technique in sequence
    TArray<CRendElementBase*> m_REs;         // List of all render elements registered in the shader
    TArray<SHRenderTarget*> m_RTargets;
    float m_fProfileTime;

    int Size()
    {
        uint32 i;
        int nSize = sizeof(SShaderTechnique);
        for (i = 0; i < m_Passes.Num(); i++)
        {
            nSize += m_Passes[i].Size();
        }
        nSize += m_RTargets.GetMemoryUsage();
        return nSize;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->Add(*this);
        pSizer->AddObject(m_Passes);
        pSizer->AddObject(m_REs);
        pSizer->AddObject(m_RTargets);
    }

    SShaderTechnique(CShader* shader)
    {
        m_shader = shader;
        uint32 i;
        for (i = 0; i < TTYPE_MAX; i++)
        {
            m_nTechnique[i] = -1;
        }
        for (i = 0; i < m_REs.Num(); i++)
        {
            SAFE_DELETE(m_REs[i]);
        }
        m_REs.Free();

        m_Flags = 0;
        m_nPreprocessFlags = 0;
        m_fProfileTime = 0;
    }
    SShaderTechnique& operator = (const SShaderTechnique& sl)
    {
        memcpy(this, &sl, sizeof(SShaderTechnique));
        if (sl.m_Passes.Num())
        {
            m_Passes.Copy(sl.m_Passes);
            for (uint32 i = 0; i < sl.m_Passes.Num(); i++)
            {
                SShaderPass* d = &m_Passes[i];
                d->AddRefsToShaders();
            }
        }
        if (sl.m_REs.Num())
        {
            m_REs.Create(sl.m_REs.Num());
            for (uint32 i = 0; i < sl.m_REs.Num(); i++)
            {
                if (sl.m_REs[i])
                {
                    m_REs[i] = sl.m_REs[i]->mfCopyConstruct();
                }
            }
        }

        return *this;
    }

    ~SShaderTechnique()
    {
        for (uint32 i = 0; i < m_Passes.Num(); i++)
        {
            SShaderPass* sl = &m_Passes[i];

            sl->mfFree();
        }
        for (uint32 i = 0; i < m_REs.Num(); i++)
        {
            CRendElementBase* pRE = m_REs[i];
            pRE->Release(false);
        }
        m_REs.Free();
        m_Passes.Free();
    }
    void UpdatePreprocessFlags(CShader* pSH);

    void* operator new(size_t Size) { void* ptr = CryModuleMalloc(Size); memset(ptr, 0, Size); return ptr; }
    void* operator new(size_t Size, [[maybe_unused]] const std::nothrow_t& nothrow)
    {
        void* ptr = CryModuleMalloc(Size);
        if (ptr)
        {
            memset(ptr, 0, Size);
        }
        return ptr;
    }
    void operator delete(void* Ptr) { CryModuleFree(Ptr); }
};

//===============================================================================

// General Shader structure
class CShader
    : public IShader
    , public CBaseResource
{
    static CCryNameTSCRC    s_sClassName;

public:
    string                  m_NameFile;   // } FIXME: This fields order is very important
    string                  m_NameShader;
    EShaderDrawType         m_eSHDType;   // } Check CShader::operator = in ShaderCore.cpp for more info

    uint32                  m_Flags;            // Different flags EF_  (see IShader.h)
    uint32                  m_Flags2;           // Different flags EF2_ (see IShader.h)
    uint32                  m_nMDV;          // Vertex modificator flags
    uint32                  m_NameShaderICRC;

    AZ::Vertex::Format      m_vertexFormat;   // Base vertex format for the shader (see VertexFormats.h)
    ECull                   m_eCull;          // Global culling type

    TArray <SShaderTechnique*> m_HWTechniques;              // Hardware techniques
    int                     m_nMaskCB;

    EShaderType             m_eShaderType;                  // [Shader System TO DO] - possibly change to be data driven

    uint64                  m_nMaskGenFX;
    uint64                  m_maskGenStatic;                // Static global flags used for generating the shader.
    SShaderGen*             m_ShaderGenParams;              // BitMask params used in automatic script generation
    SShaderGen*             m_ShaderGenStaticParams;
    SShaderTexSlots*        m_ShaderTexSlots[TTYPE_MAX];    // filled out with data of the used texture slots for a given technique
                                                            // (might be NULL if this data isn't gathered)
    std::vector<CShader*>*  m_DerivedShaders;
    CShader*                m_pGenShader;

    int                     m_nRefreshFrame; // Current frame for shader reloading (to avoid multiple reloading)
    uint32                  m_SourceCRC32;
    uint32                  m_CRC32;

    //============================================================================

    inline int mfGetID() { return CBaseResource::GetID(); }

    void mfFree();
    CShader()
        : m_eSHDType(eSHDT_General)
        , m_Flags(0)
        , m_Flags2(0)
        , m_nMDV(0)
        , m_NameShaderICRC(0)
        , m_vertexFormat(eVF_P3F_C4B_T2F)
        , m_eCull((ECull) - 1)
        , m_nMaskCB(0)
        , m_eShaderType(eST_General)
        , m_nMaskGenFX(0)
        , m_ShaderGenParams(nullptr)
        , m_DerivedShaders(nullptr)
        , m_pGenShader(nullptr)
        , m_nRefreshFrame(0)
        , m_SourceCRC32(0)
        , m_CRC32(0)
        , m_ShaderGenStaticParams(nullptr)
        , m_maskGenStatic(0)
    {
        memset(m_ShaderTexSlots, 0, sizeof(m_ShaderTexSlots));
    }
    virtual ~CShader();

    //===================================================================================

    // IShader interface
    virtual int AddRef() { return CBaseResource::AddRef(); }
    virtual int Release()
    {
        if (m_Flags & EF_SYSTEM)
        {
            return -1;
        }
        return CBaseResource::Release();
    }
    virtual int ReleaseForce()
    {
        m_Flags &= ~EF_SYSTEM;
        int nRef = 0;
        while (true)
        {
            nRef = Release();
            if (nRef <= 0)
            {
                break;
            }
        }
        return nRef;
    }

    virtual int GetID() { return CBaseResource::GetID(); }
    virtual int GetRefCounter() const { return CBaseResource::GetRefCounter(); }
    virtual const char* GetName() { return m_NameShader.c_str(); }
    virtual const char* GetName() const { return m_NameShader.c_str(); }

    // D3D Effects interface
    virtual bool FXSetTechnique(const CCryNameTSCRC& Name);
    virtual bool FXSetPSFloat(const CCryNameR& NameParam, const Vec4 fParams[], int nParams) override;
    virtual bool FXSetCSFloat(const CCryNameR& NameParam, const Vec4 fParams[], int nParams) override;
    virtual bool FXSetVSFloat(const CCryNameR& NameParam, const Vec4 fParams[], int nParams) override;
    virtual bool FXSetGSFloat(const CCryNameR& NameParam, const Vec4 fParams[], int nParams) override;
    
    virtual bool FXSetPSFloat(const char* NameParam, const Vec4 fParams[], int nParams) override;
    virtual bool FXSetCSFloat(const char* NameParam, const Vec4 fParams[], int nParams) override;
    virtual bool FXSetVSFloat(const char* NameParam, const Vec4 fParams[], int nParams) override;
    virtual bool FXSetGSFloat(const char* NameParam, const Vec4 fParams[], int nParams) override;

    virtual bool FXBegin(uint32* uiPassCount, uint32 nFlags) override;
    virtual bool FXBeginPass(uint32 uiPass) override;
    virtual bool FXCommit(const uint32 nFlags) override;
    virtual bool FXEndPass() override;
    virtual bool FXEnd() override;

    virtual int GetFlags() const { return m_Flags; }
    virtual int GetFlags2() const { return m_Flags2; }
    virtual void SetFlags2(int Flags) { m_Flags2 |= Flags; }
    virtual void ClearFlags2(int Flags) { m_Flags2 &= ~Flags; }

    virtual bool Reload(int nFlags, const char* szShaderName);
#if !defined(CONSOLE) && !defined(NULL_RENDERER)
    virtual void mfFlushCache();
#endif

    void mfFlushPendedShaders();

    virtual int GetTechniqueID(int nTechnique, int nRegisteredTechnique)
    {
        if (nTechnique < 0)
        {
            nTechnique = 0;
        }
        if ((int)m_HWTechniques.Num() <= nTechnique)
        {
            return -1;
        }
        SShaderTechnique* pTech = m_HWTechniques[nTechnique];
        return pTech->m_nTechnique[nRegisteredTechnique];
    }
    virtual TArray<CRendElementBase*>* GetREs (int nTech)
    {
        if (nTech < 0)
        {
            nTech = 0;
        }
        if (nTech < (int)m_HWTechniques.Num())
        {
            SShaderTechnique* pTech = m_HWTechniques[nTech];
            return &pTech->m_REs;
        }
        return NULL;
    }
    virtual int GetTexId ();
    virtual unsigned int GetUsedTextureTypes (void);
    virtual AZ::Vertex::Format GetVertexFormat(void) { return m_vertexFormat; }
    virtual uint64 GetGenerationMask() { return m_nMaskGenFX; }
    virtual ECull GetCull(void)
    {
        if (m_HWTechniques.Num())
        {
            SShaderTechnique* pTech = m_HWTechniques[0];
            if (pTech->m_Passes.Num())
            {
                return (ECull)pTech->m_Passes[0].m_eCull;
            }
        }
        return eCULL_None;
    }
    virtual SShaderGen* GetGenerationParams()
    {
        if (m_ShaderGenParams)
        {
            return m_ShaderGenParams;
        }
        if (m_pGenShader)
        {
            return m_pGenShader->m_ShaderGenParams;
        }
        return NULL;
    }

    size_t GetNumberOfUVSets() override
    {
        SShaderGen* shaderGenParams = GetGenerationParams();
        if (shaderGenParams)
        {
            for (int i = 0; i < shaderGenParams->m_BitMask.Num(); i++)
            {
                // A material with any of the following shader gen params is using 2 uv sets
                if (shaderGenParams->m_BitMask[i]->m_ParamName == "%BLENDLAYER_UV_SET_2" ||
                    shaderGenParams->m_BitMask[i]->m_ParamName == "%EMITTANCE_MAP_UV_SET_2" ||
                    shaderGenParams->m_BitMask[i]->m_ParamName == "%DETAIL_MAPPING_UV_SET_2")
                {
                    if (shaderGenParams->m_BitMask[i]->m_Mask & m_nMaskGenFX)
                    {
                        return 2;
                    }
                }
            }
        }

        return 1;
    }

    virtual SShaderTexSlots* GetUsedTextureSlots(int nTechnique);

    virtual AZStd::vector<SShaderParam>& GetPublicParams();
    virtual EShaderType GetShaderType() { return m_eShaderType; }
    virtual EShaderDrawType GetShaderDrawType() const { return m_eSHDType; }
    virtual uint32 GetVertexModificator() { return m_nMDV; }

    //=======================================================================================

    bool mfPrecache(SShaderCombination& cmb, bool bForce, bool bCompressedOnly, CShaderResources* pRes);

    SShaderTechnique* mfFindTechnique(const CCryNameTSCRC& name)
    {
        uint32 i;
        for (i = 0; i < m_HWTechniques.Num(); i++)
        {
            SShaderTechnique* pTech = m_HWTechniques[i];
            if (pTech->m_NameCRC == name)
            {
                return pTech;
            }
        }
        return NULL;
    }
    SShaderTechnique* mfGetStartTechnique(int nTechnique);
    // CRY DX12
    SShaderTechnique* GetTechnique(int nStartTechnique, int nRequestedTechnique);

    virtual ITexture* GetBaseTexture(int* nPass, int* nTU);

    CShader& operator = (const CShader& src);
    CTexture* mfFindBaseTexture(TArray<SShaderPass>& Passes, int* nPass, int* nTU);

    int mfSize();

    // All loaded shaders resources list
    static TArray<CShaderResources*> s_ShaderResources_known;

    virtual int Size([[maybe_unused]] int Flags)
    {
        return mfSize();
    }

    virtual void GetMemoryUsage(ICrySizer* Sizer) const;
    void* operator new(size_t Size) { void* ptr = CryModuleMalloc(Size); memset(ptr, 0, Size); return ptr; }
    void* operator new(size_t Size, [[maybe_unused]] const std::nothrow_t& nothrow)
    {
        void* ptr = CryModuleMalloc(Size);
        if (ptr)
        {
            memset(ptr, 0, Size);
        }
        return ptr;
    }
    void operator delete(void* Ptr) { CryModuleFree(Ptr); }

    static CCryNameTSCRC mfGetClassName()
    {
        return s_sClassName;
    }
};

inline SShaderTechnique* SShaderItem::GetTechnique() const
{
    SShaderTechnique* pTech = NULL;
    int nTech = m_nTechnique;
    if (nTech < 0)
    {
        nTech = 0;
    }
    CShader* pSH = (CShader*)m_pShader;

    if (pSH && !pSH->m_HWTechniques.empty())
    {
        CryPrefetch(&pSH->m_HWTechniques[0]);

        assert(m_nTechnique < 0 || pSH->m_HWTechniques.Num() == 0 || nTech < (int)pSH->m_HWTechniques.Num());
        if (nTech < (int)pSH->m_HWTechniques.Num())
        {
            return pSH->m_HWTechniques[nTech];
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////


#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_SHADERS_SHADER_H
