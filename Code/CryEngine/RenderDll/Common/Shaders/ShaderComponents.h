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

#include "../Defs.h"
#include "ShadersResourcesGroups/PerFrame.h"

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(ShaderComponents_h)
#endif

#define PF_SINGLE_COMP 2
#define PF_DONTALLOW_DYNMERGE 4
#define PF_INTEGER    8
#define PF_BOOL       0x10
#define PF_POSITION   0x20
#define PF_MATRIX     0x40
#define PF_SCALAR     0x80
#define PF_TWEAKABLE_0 0x100
#define PF_TWEAKABLE_1 0x200
#define PF_TWEAKABLE_2 0x400
#define PF_TWEAKABLE_3 0x800
#define PF_TWEAKABLE_MASK 0xf00
#define PF_MERGE_MASK    0xff000
#define PF_MERGE         0x1000
#define PF_INSTANCE      0x100000
#define PF_MATERIAL      0x200000
#define PF_CUSTOM_BINDED 0x1000000
#define PF_CANMERGED     0x2000000
#define PF_AUTOMERGED    0x4000000
#define PF_GLOBAL        0x10000000

enum ECGParam
{
    ECGP_Unknown,

    ECGP_SI_AmbientOpacity,
    ECGP_SI_ObjectAmbColComp,
    ECGP_SI_BendInfo,
    ECGP_SI_PrevBendInfo,
    ECGP_SI_AlphaTest,
    ECGP_Matr_PI_Obj_T,
    ECGP_PB_GmemStencilValue,
    ECGP_PI_MotionBlurData,
    ECGP_PI_TessParams,
    ECGP_Matr_PI_ViewProj,
    ECGP_Matr_PI_Composite,
    ECGP_Matr_PI_ObjOrigComposite,
    ECGP_PI_OSCameraPos,
    ECGP_PI_Ambient,
    ECGP_PI_VisionParams,
    ECGP_PB_VisionMtlParams,
    ECGP_PI_AvgFogVolumeContrib,
    ECGP_PI_NumInstructions,
    ECGP_PI_TextureTileSize,
    ECGP_PI_MotionBlurInfo,
    ECGP_PI_ParticleParams,
    ECGP_PI_ParticleSoftParams,
    ECGP_PI_ParticleExtParams,
    ECGP_PI_ParticleAlphaTest,
    ECGP_PI_ParticleEmissiveColor,
    ECGP_PI_WrinklesMask0,
    ECGP_PI_WrinklesMask1,
    ECGP_PI_WrinklesMask2,
    ECGP_Matr_PI_OceanMat,

    ECGP_PB_Scalar,
    ECGP_Matr_PB_ProjMatrix,
    ECGP_Matr_PB_UnProjMatrix,

    ECGP_Matr_PB_Camera,
    ECGP_Matr_PB_Camera_I,
    ECGP_Matr_PB_Camera_T,
    ECGP_Matr_PB_Camera_IT,

    ECGP_Matr_PB_Temp4_0,
    ECGP_Matr_PB_Temp4_1,
    ECGP_Matr_PB_Temp4_2,
    ECGP_Matr_PB_Temp4_3,
    ECGP_Matr_PB_TerrainBase,
    ECGP_Matr_PB_TerrainLayerGen,
    ECGP_Matr_PI_TexMatrix,
    ECGP_Matr_PI_TCGMatrix,

    ECGP_PM_Tweakable,
    ECGP_PM_DiffuseColor,
    ECGP_PM_SpecularColor,
    ECGP_PM_EmissiveColor,
    ECGP_PM_DeformWave,
    ECGP_PM_DetailTiling,
    ECGP_PM_TexelDensity,
    ECGP_PM_UVMatrixDiffuse,
    ECGP_PM_UVMatrixCustom,
    ECGP_PM_UVMatrixEmissiveMultiplier,
    ECGP_PM_UVMatrixEmittance,
    ECGP_PM_UVMatrixDetail,

    ECGP_PB_BlendTerrainColInfo,

    ECGP_PB_DLightsInfo,
    ECGP_PB_IrregKernel,
    ECGP_PB_TFactor,
    ECGP_PB_TempData,
    ECGP_PB_RTRect,
    ECGP_PB_FromRE,
    ECGP_PB_ObjVal,
    ECGP_PB_ScreenSize,

    ECGP_PB_ClipVolumeParams,

    ECGP_PB_ResInfoDiffuse,
    ECGP_PB_FromObjSB,
    ECGP_PB_TexelDensityParam,
    ECGP_PB_TexelDensityColor,
    ECGP_PB_TexelsPerMeterInfo,

    ECGP_PB_WaterRipplesLookupParams,
    ECGP_PB_SkinningExtraWeights,

    ECGP_PI_FurLODInfo,
    ECGP_PI_FurParams,
    ECGP_PI_PrevObjWorldMatrix,

    ECGP_COUNT,
};

enum EOperation
{
    eOp_Unknown,
    eOp_Add,
    eOp_Sub,
    eOp_Div,
    eOp_Mul,
    eOp_Log,
};

//-----------------------------------------------------------------------------
// This is the binding structure that represents any parameter parsed by the shader parser 
// and is to be bound in the shader.
//-----------------------------------------------------------------------------
struct SCGBind
{
    CCryNameR   m_Name;
    uint32      m_Flags;

    // m_BindingSlot - For constants it represents the buffer binding slot, for example B0, B1..
    // For textures and samplers it is the actual binding slot / offset.
    short       m_BindingSlot;

    // m_RegisterOffset - For constants it is the register offset within the binding slot group
    // For textures and samplers the offset simply uses the MSB to indicate the usage.   
    // A texture will be SHADER_BIND_TEXTURE while a sampler will be SHADER_BIND_SAMPLER
    // [Shader System] - possibly change / remove MSB usage.
    short       m_RegisterOffset;   
    
    // m_RegisterCount - number of vectors used by the parameters.  
    // Example: matrix 4x4 will require 4 vectors.
    int         m_RegisterCount;    

    SCGBind()
    {
        m_RegisterCount = 1;
        m_RegisterOffset = -2;
        m_BindingSlot = 0;
        m_Flags = 0;
    }
    SCGBind (const SCGBind& sb)
    {
        m_Name = sb.m_Name;
        m_RegisterOffset = sb.m_RegisterOffset;
        m_BindingSlot = sb.m_BindingSlot;
        m_RegisterCount = sb.m_RegisterCount;
        m_Flags = sb.m_Flags;
    }
    SCGBind& operator = (const SCGBind& sb)
    {
        this->~SCGBind();
        new(this)SCGBind(sb);
        return *this;
    }
    int Size()
    {
        return sizeof(SCGBind);
    }
    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const{}
};

struct SParamData
{
    CCryNameR m_CompNames[4];
    union UData
    {
        uint64 nData64[4];
        uint32 nData32[4];
        float fData[4];
    } d;
    SParamData()
    {
        memset(&d, 0, sizeof(UData));
    }
    ~SParamData();
    SParamData(const SParamData& sp);
    SParamData& operator = (const SParamData& sp)
    {
        this->~SParamData();
        new(this)SParamData(sp);
        return *this;
    }
    unsigned Size() { return sizeof(SParamData); }
    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};

struct SCGLiteral
{
    int m_nIndex;
    //Vec4 m_vVec;
    unsigned Size() { return sizeof(SCGLiteral); }
    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const{}
};

//-----------------------------------------------------------------------------
// This is the binding structure for constant data parsed by the shader parser 
// and is to be bound in the shader.
//-----------------------------------------------------------------------------
struct SCGParam
    : SCGBind
{
    ECGParam m_eCGParamType;
    SParamData* m_pData;
    UINT_PTR m_nID;
    SCGParam()
    {
        m_eCGParamType = ECGP_Unknown;
        m_pData = NULL;
        m_nID = 0;
    }
    ~SCGParam()
    {
        SAFE_DELETE(m_pData);
    }
    SCGParam(const SCGParam& sp)
        : SCGBind(sp)
    {
        m_eCGParamType = sp.m_eCGParamType;
        m_nID = sp.m_nID;
        if (sp.m_pData)
        {
            m_pData = new SParamData;
            *m_pData = *sp.m_pData;
        }
        else
        {
            m_pData = NULL;
        }
    }
    SCGParam& operator = (const SCGParam& sp)
    {
        this->~SCGParam();
        new(this)SCGParam(sp);
        return *this;
    }
    bool operator != (const SCGParam& sp) const
    {
        if (sp.m_RegisterOffset == m_RegisterOffset &&
            sp.m_Name == m_Name &&
            sp.m_nID == m_nID &&
            sp.m_RegisterCount == m_RegisterCount &&
            sp.m_eCGParamType == m_eCGParamType &&
            sp.m_BindingSlot == m_BindingSlot &&
            sp.m_Flags == m_Flags &&
            !sp.m_pData && !m_pData)
        {
            return false;
        }
        return true;
    }

    const CCryNameR GetParamCompName(int nComp) const
    {
        if (!m_pData)
        {
            return CCryNameR("None");
        }
        return m_pData->m_CompNames[nComp];
    }

    int Size()
    {
        int nSize = sizeof(SCGParam);
        if (m_pData)
        {
            nSize += m_pData->Size();
        }
        return nSize;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_pData);
    }
};


enum ECGSampler
{
    ECGS_Unknown,
    ECGS_MatSlot_Diffuse,
    ECGS_MatSlot_Normalmap,
    ECGS_MatSlot_Gloss,
    ECGS_MatSlot_Env,
    ECGS_Shadow0,
    ECGS_Shadow1,
    ECGS_Shadow2,
    ECGS_Shadow3,
    ECGS_Shadow4,
    ECGS_Shadow5,
    ECGS_Shadow6,
    ECGS_Shadow7,
    ECGS_TrilinearClamp,
    ECGS_MatAnisoHighWrap,
    ECGS_MatAnisoLowWrap,
    ECGS_MatTrilinearWrap,
    ECGS_MatBilinearWrap,
    ECGS_MatTrilinearClamp,
    ECGS_MatBilinearClamp,
    ECGS_MatAnisoHighBorder,
    ECGS_MatTrilinearBorder,
    ECGS_COUNT
};

struct SCGSampler
    : SCGBind
{
    int m_nStateHandle;
    ECGSampler m_eCGSamplerType;
    SCGSampler()
    {
        m_nStateHandle = -1;
        m_eCGSamplerType = ECGS_Unknown;
    }
    ~SCGSampler()
    {
    }
    SCGSampler(const SCGSampler& sp)
        : SCGBind(sp)
    {
        m_eCGSamplerType = sp.m_eCGSamplerType;
        m_nStateHandle = sp.m_nStateHandle;
    }
    SCGSampler& operator = (const SCGSampler& sp)
    {
        this->~SCGSampler();
        new(this)SCGSampler(sp);
        return *this;
    }
    bool operator != (const SCGSampler& sp) const
    {
        if (sp.m_RegisterOffset == m_RegisterOffset &&
            sp.m_Name == m_Name &&
            sp.m_nStateHandle == m_nStateHandle &&
            sp.m_RegisterCount == m_RegisterCount &&
            sp.m_eCGSamplerType == m_eCGSamplerType &&
            sp.m_BindingSlot == m_BindingSlot &&
            sp.m_Flags == m_Flags)
        {
            return false;
        }
        return true;
    }

    int Size()
    {
        int nSize = sizeof(SCGSampler);
        return nSize;
    }

    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const
    {
    }
};



enum ECGTexture
{
    ECGT_Unknown,
    ECGT_MatSlot_Diffuse,   // @adiblev - move this to the end in order to support dynamically growing number
    ECGT_MatSlot_Normals,
    ECGT_MatSlot_Height,
    ECGT_MatSlot_Specular,
    ECGT_MatSlot_Env,
    ECGT_MatSlot_SubSurface,
    ECGT_MatSlot_Smoothness,
    ECGT_MatSlot_DecalOverlay,
    ECGT_MatSlot_Custom,
    ECGT_MatSlot_CustomSecondary,
    ECGT_MatSlot_Opacity,
    ECGT_MatSlot_Detail,
    ECGT_MatSlot_Emittance,
    ECGT_MatSlot_Occlusion,
    ECGT_MatSlot_Specular2,
    ECGT_SF_Slot0,
    ECGT_SF_Slot1,
    ECGT_SF_SlotY,
    ECGT_SF_SlotU,
    ECGT_SF_SlotV,
    ECGT_SF_SlotA,
    ECGT_Shadow0,
    ECGT_Shadow1,
    ECGT_Shadow2,
    ECGT_Shadow3,
    ECGT_Shadow4,
    ECGT_Shadow5,
    ECGT_Shadow6,
    ECGT_Shadow7,
    ECGT_ShadowMask,
    ECGT_ZTarget,
    ECGT_ZTargetScaled,
    ECGT_ZTargetMS,
    ECGT_ShadowMaskZTarget,
    ECGT_SceneNormalsBent,
    ECGT_SceneNormals,
    ECGT_SceneDiffuse,
    ECGT_SceneSpecular,
    ECGT_SceneDiffuseAcc,
    ECGT_SceneSpecularAcc,
    ECGT_SceneNormalsMapMS,
    ECGT_SceneDiffuseAccMS,
    ECGT_SceneSpecularAccMS,
    ECGT_VolumetricClipVolumeStencil,
    ECGT_VolumetricFog,
    ECGT_VolumetricFogGlobalEnvProbe0,
    ECGT_VolumetricFogGlobalEnvProbe1,
    ECGT_COUNT
};

class CTexAnim;

//-----------------------------------------------------------------------------
// This is the binding structure for texture data parsed by the shader parser 
// as well as its binding to the shader (bind slot).
//-----------------------------------------------------------------------------
struct SCGTexture : SCGBind
{
    CTexture* m_pTexture;
    CTexAnim* m_pAnimInfo;
    ECGTexture m_eCGTextureType;
    bool m_bSRGBLookup;
    bool m_bGlobal;

    SCGTexture()
    {
        m_pTexture = nullptr;
        m_pAnimInfo = nullptr;
        m_eCGTextureType = ECGT_Unknown;
        m_bSRGBLookup = false;
        m_bGlobal = false;
    }
    ~SCGTexture();
    SCGTexture(const SCGTexture& sp);
    SCGTexture& operator = (const SCGTexture& sp)
    {
        this->~SCGTexture();
        new(this)SCGTexture(sp);
        return *this;
    }
    bool operator != (const SCGTexture& sp) const
    {
        if (sp.m_RegisterOffset == m_RegisterOffset &&
            sp.m_Name == m_Name &&
            sp.m_pTexture == m_pTexture &&
            sp.m_RegisterCount == m_RegisterCount &&
            sp.m_eCGTextureType == m_eCGTextureType &&
            sp.m_BindingSlot == m_BindingSlot &&
            sp.m_Flags == m_Flags &&
            sp.m_pAnimInfo == m_pAnimInfo &&
            sp.m_bSRGBLookup == m_bSRGBLookup &&
            sp.m_bGlobal == m_bGlobal)
        {
            return false;
        }
        return true;
    }

    CTexture* GetTexture() const;

    int Size()
    {
        int nSize = sizeof(SCGTexture);
        return nSize;
    }

    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const
    {
    }
};
