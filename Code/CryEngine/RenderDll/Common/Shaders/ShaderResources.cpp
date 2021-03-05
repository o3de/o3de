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

#include "RenderDll_precompiled.h"
#include "ShaderResources.h"
#include "GraphicsPipeline/Common/GraphicsPipelineStateSet.h"

#include <AzCore/std/sort.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#ifndef  NULL_RENDERER
    #include "DriverD3D.h"
#endif // ! NULL_RENDERER

enum MaterialRegister
{
    MaterialRegister_DiffuseColor       = 0, // float4
    MaterialRegister_SpecularColor      = 1, // float4
    MaterialRegister_EmissiveColor      = 2, // float4
    MaterialRegister_DeformWave         = 3, // float2x4
    MaterialRegister_DetailTiling       = 5, // float4
    MaterialRegister_TexelDensity       = 6, // float4
    MaterialRegister_UVMatrixDiffuse    = 7, // float4x4
    MaterialRegister_UVMatrixCustom     = 11,// float4x4
    MaterialRegister_UVMatrixEmissiveMultiplier = 15,// float4x4
    MaterialRegister_UVMatrixEmittance  = 19,// float4x4
    MaterialRegister_UVMatrixDetail     = 23,// float4x4

    // Reflected constants are appended after the fixed ones.
    MaterialRegister_MaxFixed = 27
};

namespace UVTransform
{
    struct TextureSlot
    {
        EEfResTextures m_Slot;
        MaterialRegister m_RegisterOffset;
    };

    TextureSlot g_SupportedSlots[] =
    {
        { EFTT_DIFFUSE, MaterialRegister_UVMatrixDiffuse },
        { EFTT_CUSTOM, MaterialRegister_UVMatrixCustom },
        { EFTT_DECAL_OVERLAY, MaterialRegister_UVMatrixEmissiveMultiplier },
        { EFTT_EMITTANCE, MaterialRegister_UVMatrixEmittance },
        { EFTT_DETAIL_OVERLAY, MaterialRegister_UVMatrixDetail }
    };

    inline TextureSlot GetSupportedSlot(AZ::u32 index)
    {
        return g_SupportedSlots[index];
    }

    inline AZ::u32 GetSupportedSlotCount()
    {
        return AZ_ARRAY_SIZE(g_SupportedSlots);
    }
}

void CShaderResources::Reset()
{
    m_TexturesResourcesMap.clear();
    m_Id = 0;
    m_IdGroup = 0;
    m_pDeformInfo = NULL;
    m_pSky = NULL;
    m_ConstantBuffer = NULL;
    m_nMtlLayerNoDrawFlags = 0;

    m_Constants.resize(MaterialRegister_MaxFixed);
}

void CShaderResources::ConvertToInputResource(SInputShaderResources* pDst)
{
    pDst->m_ResFlags = m_ResFlags;
    pDst->m_AlphaRef = m_AlphaRef;
    pDst->m_VoxelCoverage = m_VoxelCoverage;

    pDst->m_SortPrio = m_SortPrio;
    if (m_pDeformInfo)
    {
        pDst->m_DeformInfo = *m_pDeformInfo;
    }
    else
    {
        pDst->m_DeformInfo.m_eType = eDT_Unknown;
    }

    pDst->m_TexturePath = m_TexturePath;
    if (m_pDeformInfo)
    {
        pDst->m_DeformInfo = *m_pDeformInfo;
    }

    // Copy all used textures data
    pDst->m_TexturesResourcesMap = m_TexturesResourcesMap;

    ToInputLM(pDst->m_LMaterial);
}

size_t CShaderResources::GetResourceMemoryUsage(ICrySizer*  pSizer)
{
    size_t  nCurrentElement(0);
    size_t  nCurrentElementSize(0);
    size_t  nTotalSize(0);

    SIZER_COMPONENT_NAME(pSizer, "CShaderResources");
    for (auto &iter : m_TexturesResourcesMap )
    {
        SEfResTexture* pTexture = &(iter.second);
        if (pTexture->m_Sampler.m_pITex)
        {
            nCurrentElementSize = pTexture->m_Sampler.m_pITex->GetDataSize();
            pSizer->AddObject(pTexture->m_Sampler.m_pITex, nCurrentElementSize);
            nTotalSize += nCurrentElementSize;
            IResourceCollector* pColl = pSizer->GetResourceCollector();
            if (pColl)
            {
                pColl->AddResource(pTexture->m_Sampler.m_pITex->GetName(), nCurrentElementSize);
            }
        }
    }

    return nTotalSize;
}

void CShaderResources::Release()
{
#ifndef NULL_RENDERER
    AZ_Assert(gRenDev && gRenDev->m_pRT, "renderer not initialized");
    gRenDev->m_pRT->EnqueueRenderCommand([this]()
    {
        if (!CryInterlockedDecrement(&m_nRefCounter))
        {
            delete this;
        }
    });
#endif
}

void CShaderResources::Cleanup()
{
    m_TexturesResourcesMap.clear();

    SAFE_DELETE(m_pDeformInfo);

    if (m_pSky)
    {
        for (size_t i = 0; i < sizeof(m_pSky->m_SkyBox) / sizeof(m_pSky->m_SkyBox[0]); ++i)
        {
            SAFE_RELEASE(m_pSky->m_SkyBox[i]);
        }
        SAFE_DELETE(m_pSky);
    }
    ReleaseConstants();

    // Not thread safe main thread can potentially access this destroyed entry in mfCreateShaderResources()
    // (if flushing of unloaded textures (UnloadLevel) is not complete before pre-loading of new materials)
    // [Shader System TO DO] - test and fix
    if (CShader::s_ShaderResources_known.Num() > m_Id && CShader::s_ShaderResources_known[m_Id] == this)
    {
        CShader::s_ShaderResources_known[m_Id] = NULL;
    }
}

CShaderResources::~CShaderResources()
{
    Cleanup();

    if (gRenDev->m_RP.m_pShaderResources == this)
    {
        gRenDev->m_RP.m_pShaderResources = NULL;
    }
}

CShaderResources::CShaderResources()
{
    // Only do expensive DX12 resource set building for PC DX12
#if defined(CRY_USE_DX12)
    m_pipelineStateCache = AZStd::make_shared<CGraphicsPipelineStateLocalCache>();
#endif
    Reset();
}

CShaderResources::CShaderResources(SInputShaderResources* pSrc)
{
    assert(pSrc);
    PREFAST_ASSUME(pSrc);

    // Only do expensive DX12 resource set building for PC DX12
#if defined(CRY_USE_DX12)
    m_pipelineStateCache = AZStd::make_shared<CGraphicsPipelineStateLocalCache>();
#endif
    Reset();

    if (!pSrc)
        return;

    m_szMaterialName = pSrc->m_szMaterialName;
    m_TexturePath = pSrc->m_TexturePath;
    m_ResFlags = pSrc->m_ResFlags;
    m_AlphaRef = pSrc->m_AlphaRef;
    m_VoxelCoverage = pSrc->m_VoxelCoverage;

    m_SortPrio = pSrc->m_SortPrio;
    m_ShaderParams = pSrc->m_ShaderParams;
    if (pSrc->m_DeformInfo.m_eType)
    {
        m_pDeformInfo = new SDeformInfo;
        *m_pDeformInfo = pSrc->m_DeformInfo;
    }

    for (auto it = pSrc->m_TexturesResourcesMap.begin(), end = pSrc->m_TexturesResourcesMap.end(); it != end; ++it)
    {
        const SEfResTexture& texture = it->second;
        // Omit any resources with no texture present
        if (!texture.m_Name.empty() || texture.m_Sampler.m_pTex)
        {
            m_TexturesResourcesMap[it->first] = texture;
        }
    }

    SetInputLM(pSrc->m_LMaterial);
}

CShaderResources& CShaderResources::operator=(const CShaderResources& src)
{
    Cleanup();
    SBaseShaderResources::operator = (src);

    m_TexturesResourcesMap = src.m_TexturesResourcesMap;
    m_Constants = src.m_Constants;
    m_IdGroup = src.m_IdGroup;
    return *this;
}

CShaderResources* CShaderResources::Clone() const
{
    CShaderResources* pSR = new CShaderResources();
    *pSR = *this;
    pSR->m_nRefCounter = 1;
    for (uint32 i = 0; i < CShader::s_ShaderResources_known.Num(); i++)
    {
        if (!CShader::s_ShaderResources_known[i])
        {
            pSR->m_Id = i;
            CShader::s_ShaderResources_known[i] = pSR;
            return pSR;
        }
    }
    if (CShader::s_ShaderResources_known.Num() >= MAX_REND_SHADER_RES)
    {
        Warning("ERROR: CShaderMan::mfCreateShaderResources: MAX_REND_SHADER_RESOURCES hit");
        pSR->Release();
        return CShader::s_ShaderResources_known[1];
    }
    pSR->m_Id = CShader::s_ShaderResources_known.Num();
    CShader::s_ShaderResources_known.AddElem(pSR);

    return pSR;
}

void CShaderResources::SetInputLM(const CInputLightMaterial& lm)
{
    ColorF* pDst = (ColorF*)&m_Constants[0];

    memcpy(pDst, lm.m_Channels, min(int(EFTT_MAX), MaterialRegister_DiffuseColor / 2) * sizeof(lm.m_Channels[0]));

    const float fMinStepSignedFmt = (1.0f / 127.0f) * 255.0f;
    const float fSmoothness = max(fMinStepSignedFmt, lm.m_Smoothness) / 255.0f;
    const float fAlpha = lm.m_Opacity;

    pDst[MaterialRegister_DiffuseColor] = lm.m_Diffuse;
    pDst[MaterialRegister_SpecularColor] = lm.m_Specular;
    pDst[MaterialRegister_EmissiveColor] = lm.m_Emittance;

    pDst[MaterialRegister_DiffuseColor][3] = fAlpha;
    pDst[MaterialRegister_SpecularColor][3] = fSmoothness;
}

void CShaderResources::ToInputLM(CInputLightMaterial& lm)
{
    if (!m_Constants.size())
    {
        return;
    }

    ColorF* pDst = (ColorF*)&m_Constants[0];

    lm.m_Diffuse = pDst[MaterialRegister_DiffuseColor];
    lm.m_Specular = pDst[MaterialRegister_SpecularColor];
    lm.m_Emittance = pDst[MaterialRegister_EmissiveColor];

    lm.m_Opacity = pDst[MaterialRegister_DiffuseColor][3];
    lm.m_Smoothness = pDst[MaterialRegister_SpecularColor][3] * 255.0f;
}

ColorF CShaderResources::GetColorValue(EEfResTextures slot) const
{
    if (!m_Constants.size())
    {
        return Col_Black;
    }

    int majoroffs;
    switch (slot)
    {
    case EFTT_DIFFUSE:
        majoroffs = MaterialRegister_DiffuseColor;
        break;
    case EFTT_SPECULAR:
        majoroffs = MaterialRegister_SpecularColor;
        break;
    case EFTT_OPACITY:
        return Col_White;
    case EFTT_SMOOTHNESS:
        return Col_White;
    case EFTT_EMITTANCE:
        majoroffs = MaterialRegister_EmissiveColor;
        break;
    default:
        return Col_White;
    }

    return reinterpret_cast<const ColorF&>(m_Constants[majoroffs]);
}

float CShaderResources::GetStrengthValue(EEfResTextures slot) const
{
    if (!m_Constants.size())
    {
        return Col_Black.a;
    }

    int majoroffs, minoroffs;
    switch (slot)
    {
    case EFTT_DIFFUSE:
        return 1.0f;
    case EFTT_SPECULAR:
        return 1.0f;
    case EFTT_OPACITY:
        majoroffs = MaterialRegister_DiffuseColor;
        minoroffs = 3;
        break;
    case EFTT_SMOOTHNESS:
        majoroffs = MaterialRegister_SpecularColor;
        minoroffs = 3;
        break;
    case EFTT_EMITTANCE:
        majoroffs = MaterialRegister_EmissiveColor;
        minoroffs = 3;
        break;
    default:
        return 1.0f;
    }

    ColorF* pDst = (ColorF*)&m_Constants[0];
    return pDst[majoroffs][minoroffs];
}

void CShaderResources::SetColorValue(EEfResTextures slot, const ColorF& color)
{
    if (!m_Constants.size())
    {
        return;
    }

    // NOTE: ideally the switch goes away and values are indexed directly
    int majoroffs;
    switch (slot)
    {
    case EFTT_DIFFUSE:
        majoroffs = MaterialRegister_DiffuseColor;
        break;
    case EFTT_SPECULAR:
        majoroffs = MaterialRegister_SpecularColor;
        break;
    case EFTT_OPACITY:
        return;
    case EFTT_SMOOTHNESS:
        return;
    case EFTT_EMITTANCE:
        majoroffs = MaterialRegister_EmissiveColor;
        break;
    default:
        return;
    }

    ColorF* pDst = (ColorF*)&m_Constants[0];
    pDst[majoroffs] = ColorF(color.toVec3(), pDst[majoroffs][3]);
}

void CShaderResources::SetStrengthValue(EEfResTextures slot, float value)
{
    if (!m_Constants.size())
    {
        return;
    }

    // NOTE: ideally the switch goes away and values are indexed directly
    int majoroffs, minoroffs;
    switch (slot)
    {
    case EFTT_DIFFUSE:
        return;
    case EFTT_SPECULAR:
        return;
    case EFTT_OPACITY:
        majoroffs = MaterialRegister_DiffuseColor;
        minoroffs = 3;
        break;
    case EFTT_SMOOTHNESS:
        majoroffs = MaterialRegister_SpecularColor;
        minoroffs = 3;
        break;
    case EFTT_EMITTANCE:
        majoroffs = MaterialRegister_EmissiveColor;
        minoroffs = 3;
        break;
    default:
        return;
    }

    ColorF* pDst = (ColorF*)&m_Constants[0];
    pDst[majoroffs][minoroffs] = value;
}
#ifndef NULL_RENDERER
void CShaderResources::UpdateConstants(IShader* pISH)
{
    if (gRenDev && gRenDev->m_pRT)
    {
        pISH->AddRef();
        this->AddRef();

        gRenDev->m_pRT->EnqueueRenderCommand([this, pISH]()
        {
            
#if defined(CRY_USE_METAL)
            //On metal the dynamic constant buffer usage assumes it will be updated every frame
            //Since that is not the case with material properties use static option.
            AzRHI::ConstantBufferUsage usage = AzRHI::ConstantBufferUsage::Static;
#else
            AzRHI::ConstantBufferUsage usage = AzRHI::ConstantBufferUsage::Dynamic;
#endif
            
            Rebuild(pISH, usage);
            pISH->Release();
            this->Release();
        });
    }
}

namespace
{
    void WriteConstants(SFXParam* requestedParameter, AZStd::vector<SShaderParam>& parameters, AZStd::vector<Vec4>& outConstants)
    {
        const AZ::u32 parameterFlags = requestedParameter->GetFlags();
        const AZ::u8  paramStageSetter = requestedParameter->m_OffsetStageSetter;
        const AZ::u32 registerOffset = requestedParameter->m_Register[paramStageSetter];
        AZ_Assert(registerOffset < outConstants.size(), "Requested parameter beyond the bounds of the constant buffer.");
        float* outputData = &outConstants[registerOffset][0];

        for (AZ::u32 componentIdx = 0; componentIdx < 4; componentIdx++)
        {
            if (parameterFlags & PF_AUTOMERGED)
            {
                CryFixedStringT<128> name;
                requestedParameter->GetCompName(componentIdx, name);
                SShaderParam::GetValue(name.c_str(), &parameters, outputData, componentIdx);
            }
            else
            {
                SShaderParam::GetValue(requestedParameter->m_Name.c_str(), &parameters, outputData, componentIdx);
            }
        }
    }

    // Creates a parameters list for populating the constants in the Constant Buffer and returns the minimum and maximum slot offset 
    // of the newly added parameters taking their size into account for the maximum offset.
    // NOTE:  the minimum and maximum slot offsets MUST be initialized outside (min=10000, max=0) for the gathering to be valid.
    void AddShaderParamToArray( 
        SShaderFXParams& inParameters, FixedDynArray<SFXParam*>& outParameters, 
        EHWShaderClass shaderClass, AZ::s32 &minSlotOffset, AZ::s32 &maxSlotOffset )
    {
        for (int n = 0; n < inParameters.m_FXParams.size(); n++)
        {
            SFXParam* parameter = &inParameters.m_FXParams[n];
            if (parameter->m_nFlags & PF_MERGE)
            {
                continue;
            }

            if (parameter->m_BindingSlot == eConstantBufferShaderSlot_PerMaterial)
            {
                if (parameter->m_Register[shaderClass] < 0 || parameter->m_Register[shaderClass] >= 10000)
                {
                    continue;
                }

                // Run over all existing parameters and look for the name entry
                size_t findIdx = 0;
                for (; findIdx < outParameters.size(); findIdx++)
                {
                    // The name entry was found - break with its index to prevent double insertion
                    if (outParameters[findIdx]->m_Name == parameter->m_Name)
                    {   
                        // Add the current usage to the marked usage
                        outParameters[findIdx]->m_StagesUsage |= ((0x1 << shaderClass) & 0xff);
                        break;
                    }
                }

                // No existing entry for that name was found - add it. (otherwise ignore to avoid adding twice)
                // Taking the first occurrence is not the optimal solution as it might leave gaps in constants offsets.  
                // A better solution would be to eliminate duplicates first with close grouping heuristics.
                if (findIdx == outParameters.size())
                {
                    outParameters.push_back(parameter);
                    parameter->m_OffsetStageSetter = shaderClass;
                    parameter->m_StagesUsage = ((0x1 << shaderClass) & 0xff);
                    minSlotOffset = AZStd::GetMin(minSlotOffset, static_cast<AZ::s32>(parameter->m_Register[shaderClass]));
                    maxSlotOffset = AZStd::GetMax(maxSlotOffset, static_cast<AZ::s32>(parameter->m_Register[shaderClass] + parameter->m_RegisterCount));
                }
            }
        }
    }
}

void CShaderResources::Rebuild(IShader* abstractShader, AzRHI::ConstantBufferUsage usage)
{
    AZ_TRACE_METHOD();
    CShader* shader = (CShader*)abstractShader;
    
    // Do not attempt to update constant buffers for shaders that are not compiled or parsed.
    // We can hit this case easily when r_shadersImport >= 2 under two primary scenarios:
    // 1) We want to render an object with a shader that was never compiled because it was never added to the shader list from the Remote Shader Compiler.
    // This is resolved by running the game in Debug or Profile and properly building your shader permutation list and rebuilding your shader paks.
    // 2) The shader permutation was never compiled because it was never intended to render, but it is still loaded into memory and active.
    // This occurs when a material's submaterial is unused, for example when a nodraw shader is attached as a submaterial.
    // The material system naively attempts to update shader constants for all submaterial's shader techniques/passes 
    // via CMatInfo::RefreshShaderResourceConstants(), so we do not want to attempt to upload shader constants for shader permutations that will never be used.
    if (!(shader->m_Flags & EF_LOADED))
    {
        return;
    }

    // Build list of used parameters and fill constant buffer scratchpad
    SShaderFXParams& parameterRegistry = gRenDev->m_cEF.m_Bin.mfGetFXParams(shader);
    const size_t parameterCount = parameterRegistry.m_FXParams.size();
    const size_t parameterByteCount = parameterCount * sizeof(SFXParam*);

    // Added this as a precaution 
    AZ_Assert((eHWSC_Num < 8), "More than 8 shader stages - m_StagesUsage can only represent 8, please adjust it to AZ::u16" );

    FixedDynArray<SFXParam*> usedParameters;
    PREFAST_SUPPRESS_WARNING(6255) usedParameters.set(ArrayT((SFXParam**)alloca((int)parameterByteCount), (int)parameterCount));

    AZ::s32 registerStart = 10000;
    AZ::s32 registerCountMax = 0;
    for (AZ::u32 techniqueIdx = 0; techniqueIdx < (int)shader->m_HWTechniques.Num(); techniqueIdx++)
    {
        SShaderTechnique* technique = shader->m_HWTechniques[techniqueIdx];
        for (AZ::u32 passIdx = 0; passIdx < technique->m_Passes.Num(); passIdx++)
        {
            const SShaderPass* pass = &technique->m_Passes[passIdx];
            const CHWShader* shaders[] = { pass->m_VShader, pass->m_PShader, pass->m_GShader, pass->m_HShader, pass->m_DShader, pass->m_CShader };

            for (EHWShaderClass shaderClass = EHWShaderClass(0); shaderClass < eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
            {
                if (shaders[shaderClass])
                {
                    AddShaderParamToArray(parameterRegistry, usedParameters, shaderClass, registerStart, registerCountMax);
                }
            }
        }
    }

    // Ordering the slots according to the Vertex Shader's slots offsets.  The order is valid in most cases with 
    // the exception when the different stages have different slots offsets, however the slots' offsets range 
    // is always valid since it's covered by the minimum and maximum gathering that happens during the 
    // slots go over.
    AZStd::sort(usedParameters.begin(), usedParameters.end(), [] (const SFXParam* lhs, const SFXParam* rhs)
    {
        return lhs->m_Register[0] < rhs->m_Register[0];
    });

    if (usedParameters.size())
    {
        // Validate and resize constant buffer scratchpad to match our reflection data.
        {
            AZ_Assert(registerStart < registerCountMax, "invalid constant buffer register interval");

            if(registerCountMax > m_Constants.size())
            {
                m_Constants.resize(registerCountMax);
            }
        }

        // Copies local shader tweakable values to the shaders local scratchpad. Then for each used parameter
        // copies that data into the constant buffer.
        {
            AZStd::vector<SShaderParam> publicParameters = shader->GetPublicParams();
            if (publicParameters.size())
            {
                for (AZ::u32 techniqueIdx = 0; techniqueIdx < m_ShaderParams.size(); techniqueIdx++)
                {
                    SShaderParam& tweakable = m_ShaderParams[techniqueIdx];
                    for (AZ::u32 j = 0; j < publicParameters.size(); j++)
                    {
                        SShaderParam& outParameter = publicParameters[j];
                        if (outParameter.m_Name == tweakable.m_Name)
                        {
                            tweakable.CopyType(outParameter);
                            outParameter.CopyValueNoString(tweakable); // there should not be 'string' values set to shader
                            break;
                        }
                    }
                }

                for (AZ::u32 i = 0; i < usedParameters.size(); i++)
                {
                    WriteConstants(usedParameters[i], publicParameters, m_Constants);
                }
            }
        }
    }

    // Update common parameters
    {
        // Updating the texture modifiers
        SEfResTexture*          pTextureRes = nullptr;
        for (AZ::u32 i = 0; i < UVTransform::GetSupportedSlotCount(); ++i)
        {
            const EEfResTextures    slot = UVTransform::GetSupportedSlot(i).m_Slot;
            const AZ::u32           registerOffset = UVTransform::GetSupportedSlot(i).m_RegisterOffset;
            Matrix44                matrix(IDENTITY);

            pTextureRes = GetTextureResource (slot);
            if (pTextureRes && pTextureRes->m_Ext.m_pTexModifier)
            {
                pTextureRes->Update(slot);
                matrix = pTextureRes->m_Ext.m_pTexModifier->m_TexMatrix;
            }
            *reinterpret_cast<Matrix44*>(&m_Constants[registerOffset]) = matrix;
        }

        Vec4            texelDensity(0.0f, 0.0f, 1.0f, 1.0f);
        Vec4            detailTiling(1.0f);

        // [Shader System TO DO] - move that to be data driven
        pTextureRes = GetTextureResource(EFTT_NORMALS);
        if (pTextureRes && pTextureRes->m_Sampler.m_pTex)
        {
            texelDensity.x = (float)pTextureRes->m_Sampler.m_pTex->GetWidth();
            texelDensity.y = (float)pTextureRes->m_Sampler.m_pTex->GetHeight();
            texelDensity.z = 1.0f / std::max(1.0f, texelDensity.x);
            texelDensity.w = 1.0f / std::max(1.0f, texelDensity.y);
        }

        pTextureRes = GetTextureResource(EFTT_DETAIL_OVERLAY);
        if (pTextureRes && pTextureRes->m_Ext.m_pTexModifier)
        {
            pTextureRes->Update(EFTT_DETAIL_OVERLAY);
            detailTiling.x = pTextureRes->m_Ext.m_pTexModifier->m_Tiling[0];
            detailTiling.y = pTextureRes->m_Ext.m_pTexModifier->m_Tiling[1];
            detailTiling.z = 1.0f / detailTiling.x;
            detailTiling.w = 1.0f / detailTiling.y;
        }

        Vec4 deformWave0(0);
        Vec4 deformWave1(0);
        if (IsDeforming())
        {
            deformWave0.x = m_pDeformInfo->m_WaveX.m_Freq;
            deformWave0.y = m_pDeformInfo->m_WaveX.m_Phase;
            deformWave0.z = m_pDeformInfo->m_WaveX.m_Amp;
            deformWave0.w = m_pDeformInfo->m_WaveX.m_Level;
            deformWave1.x = 1.0f / m_pDeformInfo->m_fDividerX;
        }

        // We store the alpha test value into the last channel of deform wave (see GetMaterial_AlphaTest()).
        deformWave1.w = m_AlphaRef;

        m_Constants[MaterialRegister_TexelDensity] = texelDensity;
        m_Constants[MaterialRegister_DetailTiling] = detailTiling;
        m_Constants[MaterialRegister_DeformWave + 0] = deformWave0;
        m_Constants[MaterialRegister_DeformWave + 1] = deformWave1;
    }

    SAFE_RELEASE(m_ConstantBuffer);

    if (m_Constants.size())
    {
        m_ConstantBuffer = gcpRendD3D->m_DevBufMan.CreateConstantBuffer(
            "PerMaterial",
            m_Constants.size() * sizeof(Vec4),
            usage,
            AzRHI::ConstantBufferFlags::None);

        m_ConstantBuffer->UpdateBuffer(&m_Constants[0], m_Constants.size() * sizeof(Vec4));

    // Only do expensive DX12 resource set building for PC DX12
#if defined(CRY_USE_DX12)
        if (!m_pCompiledResourceSet)
        {
            m_pCompiledResourceSet = CDeviceObjectFactory::GetInstance().CreateResourceSet();
        }

        m_pCompiledResourceSet->Clear();
        m_pCompiledResourceSet->Fill(shader, this, EShaderStage_AllWithoutCompute);
        m_pCompiledResourceSet->Build();
#endif
    }
}

void CShaderResources::CloneConstants(const IRenderShaderResources* pISrc)
{
    CShaderResources* pSrc = (CShaderResources*)pISrc;

    if (!pSrc)
    {
        m_Constants.clear();
        SAFE_RELEASE(m_ConstantBuffer);
        return;
    }
    else
    {
        m_Constants = pSrc->m_Constants;
        {
            AzRHI::ConstantBuffer*& pCB0Dst = m_ConstantBuffer;
            AzRHI::ConstantBuffer*& pCB0Src = pSrc->m_ConstantBuffer;
            if (pCB0Src)
            {
                pCB0Src->AddRef();
            }
            if (pCB0Dst)
            {
                pCB0Dst->Release();
            }
            pCB0Dst = pCB0Src;
        }
    }
}

void CShaderResources::ReleaseConstants()
{
    m_Constants.clear();

    if (m_ConstantBuffer)
    {
        AzRHI::ConstantBuffer* constantBuffer = m_ConstantBuffer;
        m_ConstantBuffer = nullptr;

        gRenDev->m_pRT->EnqueueRenderCommand([constantBuffer]()
        {
            constantBuffer->Release();
        });
    }
}

static void AdjustSamplerState(SEfResTexture* pTex, bool bUseGlobalMipBias)
{
    int nTS = pTex->m_Sampler.m_nTexState;
    if (nTS < 0 || nTS >= (int)CTexture::s_TexStates.size())
    {
        return;
    }
    int8 nAniso = min(CRenderer::CV_r_texminanisotropy, CRenderer::CV_r_texmaxanisotropy);
    if (nAniso < 1)
    {
        return;
    }
    STexState* pTS = &CTexture::s_TexStates[nTS];
    STexState ST = *pTS;

    float mipBias = 0.0f;
    if (bUseGlobalMipBias)
    {
        mipBias = gRenDev->GetTemporalJitterMipBias();
    }

    if (ST.m_nAnisotropy == nAniso && ST.m_MipBias == mipBias)
    {
        return;
    }
    ST.m_pDeviceState = NULL;   //otherwise state change is not applied
    ST.m_MipBias = mipBias;

    if (nAniso >= 16)
    {
        ST.m_nMipFilter =
            ST.m_nMinFilter =
                ST.m_nMagFilter = FILTER_ANISO16X;
    }
    else if (nAniso >= 8)
    {
        ST.m_nMipFilter =
            ST.m_nMinFilter =
                ST.m_nMagFilter = FILTER_ANISO8X;
    }
    else if (nAniso >= 4)
    {
        ST.m_nMipFilter =
            ST.m_nMinFilter =
                ST.m_nMagFilter = FILTER_ANISO4X;
    }
    else if (nAniso >= 2)
    {
        ST.m_nMipFilter =
            ST.m_nMinFilter =
                ST.m_nMagFilter = FILTER_ANISO2X;
    }
    else
    {
        ST.m_nMipFilter =
            ST.m_nMinFilter =
                ST.m_nMagFilter = FILTER_TRILINEAR;
    }

    ST.m_nAnisotropy = nAniso;
    pTex->m_Sampler.m_nTexState = CTexture::GetTexState(ST);
}

// [Shader System TO DO] - delete this hard coded method.   Move to data driven
void CShaderResources::AdjustForSpec()
{
    // Note: Anisotropic filtering for smoothness maps is deliberately disabled, otherwise
    //       mip transitions become too obvious when using maps pre-filtered with normal variance
    uint16       SamplersModulatorsIdx[] = {
        EFTT_DIFFUSE,
        EFTT_NORMALS,
        EFTT_SPECULAR,
        EFTT_CUSTOM,
        EFTT_CUSTOM_SECONDARY,
        EFTT_EMITTANCE,
        0xffff,
    };

    for (uint32 pos = 0; SamplersModulatorsIdx[pos] != 0xffff ; pos++)
    {
        SEfResTexture*  pTextureRes = GetTextureResource(SamplersModulatorsIdx[pos]);
        if (pTextureRes)
        {
            AdjustSamplerState(pTextureRes, true);
        }
    }
}
#endif
