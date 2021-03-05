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
#include "DeviceWrapper12.h"
#include "DriverD3D.h"
#include "../../Common/ReverseDepth.h"
#include "../../Common/Textures/TextureHelpers.h"
#include "../GraphicsPipeline/Common/GraphicsPipelineStateSet.h"
#include <AzCore/std/hash.h>

extern uint8 g_StencilFuncLookup[8];
extern uint8 g_StencilOpLookup[8];

volatile int CDeviceResourceSet::s_dirtyCount = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
bool SDeviceObjectHelpers::GetShaderInstanceInfo(std::array<SDeviceObjectHelpers::SShaderInstanceInfo, eHWSC_Num>& shaderInstanceInfos, CShader* pShader, const CCryNameTSCRC& technique, uint64 rtFlags, uint32 mdFlags, uint32 mdvFlags, UPipelineState pipelineState[eHWSC_Num], bool bAllowTesselation)
{
    bool shadersAvailable = true;
    if (SShaderTechnique* pShaderTechnique = pShader->mfFindTechnique(technique))
    {
        SShaderPass& shaderPass = pShaderTechnique->m_Passes[0];

        CHWShader* pHWShaders[] =
        {
            shaderPass.m_VShader,
            shaderPass.m_PShader,
            shaderPass.m_GShader,
            shaderPass.m_CShader,
            shaderPass.m_DShader,
            shaderPass.m_HShader,
        };

        for (EHWShaderClass shaderStage = eHWSC_Vertex; shaderStage < eHWSC_Num; shaderStage = EHWShaderClass(shaderStage + 1))
        {
            if (!bAllowTesselation && (shaderStage == eHWSC_Hull || shaderStage == eHWSC_Domain))
            {
                continue;
            }

            CHWShader_D3D* pHWShaderD3D = reinterpret_cast<CHWShader_D3D*>(pHWShaders[shaderStage]);
            shaderInstanceInfos[shaderStage].pHwShader = pHWShaderD3D;
            shaderInstanceInfos[shaderStage].technique = technique;

            if (pHWShaderD3D)
            {
                SShaderCombIdent Ident;
                Ident.m_LightMask = 0;
                Ident.m_RTMask = rtFlags & pHWShaderD3D->m_nMaskAnd_RT | pHWShaderD3D->m_nMaskOr_RT;
                Ident.m_MDMask = mdFlags & (shaderStage != eHWSC_Pixel ? 0xFFFFFFFF : ~HWMD_TEXCOORD_FLAG_MASK);
                Ident.m_MDVMask = ((shaderStage != eHWSC_Pixel) ? mdvFlags : 0) | CParserBin::m_nPlatform;
                Ident.m_GLMask = pHWShaderD3D->m_nMaskGenShader;
                Ident.m_STMask = pHWShaderD3D->m_maskGenStatic;
                Ident.m_pipelineState = pipelineState ? pipelineState[shaderStage] : UPipelineState();

                if (auto pInstance = pHWShaderD3D->mfGetInstance(pShader, Ident, 0))
                {
                    if (pHWShaderD3D->mfCheckActivation(pShader, pInstance, 0))
                    {
                        shaderInstanceInfos[shaderStage].pHwShaderInstance = pInstance;
                        shaderInstanceInfos[shaderStage].pDeviceShader = pInstance->m_Handle.m_pShader->m_pHandle;
                    }
                    else
                    {
                        shadersAvailable = false;
                        break;
                    }
                }
            }
        }
    }

    return shadersAvailable;
}

bool SDeviceObjectHelpers::GetConstantBuffersFromShader(std::vector<SDeviceObjectHelpers::SConstantBufferBindInfo>& constantBufferInfos, CShader* pShader, const CCryNameTSCRC& technique, uint64 rtFlags, uint32 mdFlags, uint32 mdvFlags)
{
    constantBufferInfos.clear();

    std::array<SDeviceObjectHelpers::SShaderInstanceInfo, eHWSC_Num> hwShaders;
    bool shadersAvailable = GetShaderInstanceInfo(hwShaders, pShader, technique, rtFlags, mdFlags, mdvFlags, nullptr, true);
    if (!shadersAvailable)
        return false;

    for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
    {
        if (hwShaders[shaderClass].pHwShader && hwShaders[shaderClass].pHwShaderInstance)
        {
            VectorSet<EConstantBufferShaderSlot> usedBuffersSlots;
            auto* pInstance = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(hwShaders[shaderClass].pHwShaderInstance);
            auto* pHwShader = reinterpret_cast<CHWShader_D3D*>(hwShaders[shaderClass].pHwShader);

            const int vectorCount[] =
            {
                pInstance->m_nMaxVecs[eConstantBufferShaderSlot_PerBatch],
                pInstance->m_nMaxVecs[eConstantBufferShaderSlot_PerInstanceLegacy]
            };

            for (const auto& bind : pInstance->m_pBindVars)
            {
                if ((bind.m_RegisterOffset & (SHADER_BIND_TEXTURE | SHADER_BIND_SAMPLER)) == 0)
                {
                    usedBuffersSlots.insert(EConstantBufferShaderSlot(bind.m_BindingSlot));
                }
            }

            for (auto bufferSlot : usedBuffersSlots)
            {
                SDeviceObjectHelpers::SConstantBufferBindInfo bindInfo;
                bindInfo.shaderClass = shaderClass;
                bindInfo.shaderSlot = bufferSlot;
                bindInfo.vectorCount = vectorCount[bufferSlot];
                bindInfo.pBuffer.attach(gcpRendD3D->m_DevBufMan.CreateConstantBuffer(
                    "ReflectedConstantBuffer",
                    bindInfo.vectorCount * sizeof(Vec4),
                    AzRHI::ConstantBufferUsage::Dynamic,
                    AzRHI::ConstantBufferFlags::None));
                bindInfo.pPreviousBuffer = nullptr;

                bindInfo.shaderInfo.pHwShader = hwShaders[shaderClass].pHwShader;
                bindInfo.shaderInfo.pHwShaderInstance = hwShaders[shaderClass].pHwShaderInstance;
                bindInfo.shaderInfo.pDeviceShader = hwShaders[shaderClass].pDeviceShader;

                constantBufferInfos.push_back(bindInfo);
            }
        }
    }
    return true;
}


void SDeviceObjectHelpers::BeginUpdateConstantBuffers(std::vector<SDeviceObjectHelpers::SConstantBufferBindInfo>& constantBuffers)
{
    AzRHI::ConstantBufferCache& cache = AzRHI::ConstantBufferCache::GetInstance();
    for (auto& cb : constantBuffers)
    {
        cache.BeginExternalConstantBuffer(
            (EHWShaderClass)cb.shaderClass,
            (EConstantBufferShaderSlot)cb.shaderSlot,
            cb.pBuffer,
            cb.vectorCount);
    }
}

void SDeviceObjectHelpers::EndUpdateConstantBuffers(std::vector<SDeviceObjectHelpers::SConstantBufferBindInfo>& constantBuffers)
{
    // set per batch and per instance parameters
    auto setParams = [&](EHWShaderClass shaderClass, bool bPerBatch, bool bPerInstance)
        {
            for (auto& cb : constantBuffers)
            {
                if (cb.shaderClass == shaderClass)
                {
                    auto pHwShader = cb.shaderInfo.pHwShader;
                    auto pHwShaderInstance = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(cb.shaderInfo.pHwShaderInstance);

                    if (pHwShader && pHwShaderInstance)
                    {
                        pHwShader->m_pCurInst = pHwShaderInstance;

                        if (bPerBatch)
                        {
                            pHwShader->UpdatePerBatchConstantBuffer();
                        }
                        if (bPerInstance)
                        {
                            pHwShader->UpdatePerInstanceConstantBuffer();
                        }
                    }

                    break;
                }
            }
        };

    setParams(eHWSC_Pixel,    true, true);
    setParams(eHWSC_Vertex,   true, true);
    setParams(eHWSC_Geometry, true, false);
    setParams(eHWSC_Compute,  true, false);

    AzRHI::ConstantBufferCache& cache = AzRHI::ConstantBufferCache::GetInstance();
    for (auto& cb : constantBuffers)
    {
        cache.EndExternalConstantBuffer(EHWShaderClass(cb.shaderClass), EConstantBufferShaderSlot(cb.shaderSlot));
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDeviceGraphicsPSODesc::CDeviceGraphicsPSODesc(CDeviceResourceLayout* pResourceLayout, const SGraphicsPipelineStateDescription& pipelineDesc)
{
    InitWithDefaults();

    m_pResourceLayout = pResourceLayout;
    m_pShader = reinterpret_cast<CShader*>(pipelineDesc.shaderItem.m_pShader);
    if (auto pTech = m_pShader->GetTechnique(pipelineDesc.shaderItem.m_nTechnique, pipelineDesc.technique))
    {
        m_technique   = pTech->m_NameCRC;
    }

    m_ShaderFlags_RT   = pipelineDesc.objectRuntimeMask;
    m_ShaderFlags_MDV  = pipelineDesc.objectFlags_MDV;
    m_VertexFormat     = pipelineDesc.vertexFormat;
    m_ObjectStreamMask = pipelineDesc.streamMask;
    m_PrimitiveType    = (eRenderPrimitiveType)pipelineDesc.primitiveType;
}


CDeviceGraphicsPSODesc::CDeviceGraphicsPSODesc(CDeviceResourceLayout* pResourceLayout, CShader* pShader, const CCryNameTSCRC& technique, uint64 rtFlags, uint32 mdFlags, uint32 mdvFlags, bool bAllowTessellation)
{
    InitWithDefaults();

    m_pResourceLayout   = pResourceLayout;
    m_pShader           = pShader;
    m_technique         = technique;
    m_bAllowTesselation = bAllowTessellation;
    m_ShaderFlags_RT    = rtFlags;
    m_ShaderFlags_MD    = mdFlags;
    m_ShaderFlags_MDV   = mdvFlags;
}

void CDeviceGraphicsPSODesc::InitWithDefaults()
{
    ZeroStruct(*this);

    m_StencilState       = STENC_FUNC(FSS_STENCFUNC_ALWAYS) | STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP) | STENCOP_PASS(FSS_STENCOP_KEEP);
    m_StencilReadMask    = D3D11_DEFAULT_STENCIL_READ_MASK;
    m_StencilWriteMask   = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    m_VertexFormat       = eVF_P3F_C4B_T2S;
    m_CullMode           = eCULL_Back;
    m_PrimitiveType      = eptTriangleList;
    m_DepthStencilFormat = eTF_Unknown;
    m_RenderTargetFormats.fill(eTF_Unknown);
}

CDeviceGraphicsPSODesc::CDeviceGraphicsPSODesc(const CDeviceGraphicsPSODesc& other)
{
    *this = other;
}

AZ::u32 CDeviceGraphicsPSODesc::GetHash() const
{
    return m_Hash;
}

void CDeviceGraphicsPSODesc::FillDescs(D3D11_RASTERIZER_DESC& rasterizerDesc, D3D11_BLEND_DESC& blendDesc, D3D11_DEPTH_STENCIL_DESC& depthStencilDesc) const
{
    const uint32 renderState = m_RenderState;

    ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
    ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));
    ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

    // Fill mode
    rasterizerDesc.DepthClipEnable = 1;
    rasterizerDesc.FrontCounterClockwise = 1;
    rasterizerDesc.FillMode = (renderState & GS_WIREFRAME) ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    rasterizerDesc.CullMode =
        (m_CullMode == eCULL_Back)
        ? D3D11_CULL_BACK
        : ((m_CullMode == eCULL_None) ? D3D11_CULL_NONE : D3D11_CULL_FRONT);

    // Blend state
    {
        const bool bBlendEnable = (renderState & GS_BLEND_MASK) != 0;

        for (size_t i = 0; i < RT_STACK_WIDTH; ++i)
        {
            blendDesc.RenderTarget[i].BlendEnable = bBlendEnable;
        }

        if (bBlendEnable)
        {
            const uint32 SrcFactorShift = 0;
            const uint32 DstFactorShift = 4;
            const uint32 BlendOpShift = 24;
            const uint32 BlendAlphaOpShift = 26;

            struct BlendFactors
            {
                D3D11_BLEND BlendColor;
                D3D11_BLEND BlendAlpha;
            };

            BlendFactors SrcBlendFactors[GS_BLSRC_MASK >> SrcFactorShift] =
            {
                { (D3D11_BLEND)0,                 (D3D11_BLEND)0 },                 // UNINITIALIZED BLEND FACTOR
                { D3D11_BLEND_ZERO,               D3D11_BLEND_ZERO },              // GS_BLSRC_ZERO
                { D3D11_BLEND_ONE,                D3D11_BLEND_ONE },               // GS_BLSRC_ONE
                { D3D11_BLEND_DEST_COLOR,         D3D11_BLEND_DEST_ALPHA },        // GS_BLSRC_DSTCOL
                { D3D11_BLEND_INV_DEST_COLOR,     D3D11_BLEND_INV_DEST_ALPHA },    // GS_BLSRC_ONEMINUSDSTCOL
                { D3D11_BLEND_SRC_ALPHA,          D3D11_BLEND_SRC_ALPHA },         // GS_BLSRC_SRCALPHA
                { D3D11_BLEND_INV_SRC_ALPHA,      D3D11_BLEND_INV_SRC_ALPHA },     // GS_BLSRC_ONEMINUSSRCALPHA
                { D3D11_BLEND_DEST_ALPHA,         D3D11_BLEND_DEST_ALPHA },        // GS_BLSRC_DSTALPHA
                { D3D11_BLEND_INV_DEST_ALPHA,     D3D11_BLEND_INV_DEST_ALPHA },    // GS_BLSRC_ONEMINUSDSTALPHA
                { D3D11_BLEND_SRC_ALPHA_SAT,      D3D11_BLEND_SRC_ALPHA_SAT },     // GS_BLSRC_ALPHASATURATE
                { D3D11_BLEND_SRC_ALPHA,          D3D11_BLEND_ZERO },              // GS_BLSRC_SRCALPHA_A_ZERO
                { D3D11_BLEND_SRC1_ALPHA,         D3D11_BLEND_SRC1_ALPHA },        // GS_BLSRC_SRC1ALPHA
            };

            BlendFactors DstBlendFactors[GS_BLDST_MASK >> DstFactorShift] =
            {
                { (D3D11_BLEND)0,                 (D3D11_BLEND)0 },                 // UNINITIALIZED BLEND FACTOR
                { D3D11_BLEND_ZERO,               D3D11_BLEND_ZERO },              // GS_BLDST_ZERO
                { D3D11_BLEND_ONE,                D3D11_BLEND_ONE },               // GS_BLDST_ONE
                { D3D11_BLEND_SRC_COLOR,          D3D11_BLEND_SRC_ALPHA },         // GS_BLDST_SRCCOL
                { D3D11_BLEND_INV_SRC_COLOR,      D3D11_BLEND_INV_SRC_ALPHA },     // GS_BLDST_ONEMINUSSRCCOL
                { D3D11_BLEND_SRC_ALPHA,          D3D11_BLEND_SRC_ALPHA },         // GS_BLDST_SRCALPHA
                { D3D11_BLEND_INV_SRC_ALPHA,      D3D11_BLEND_INV_SRC_ALPHA },     // GS_BLDST_ONEMINUSSRCALPHA
                { D3D11_BLEND_DEST_ALPHA,         D3D11_BLEND_DEST_ALPHA },        // GS_BLDST_DSTALPHA
                { D3D11_BLEND_INV_DEST_ALPHA,     D3D11_BLEND_INV_DEST_ALPHA },    // GS_BLDST_ONEMINUSDSTALPHA
                { D3D11_BLEND_ONE,                D3D11_BLEND_ZERO },              // GS_BLDST_ONE_A_ZERO
                { D3D11_BLEND_INV_SRC1_ALPHA,     D3D11_BLEND_INV_SRC1_ALPHA },    // GS_BLDST_ONEMINUSSRC1ALPHA
            };

            D3D11_BLEND_OP BlendOp[GS_BLEND_OP_MASK >> BlendOpShift] =
            {
                D3D11_BLEND_OP_ADD,     // 0 (unspecified): Default
                D3D11_BLEND_OP_MAX,     // GS_BLOP_MAX / GS_BLALPHA_MAX
                D3D11_BLEND_OP_MIN,     // GS_BLOP_MIN / GS_BLALPHA_MIN
            };

            blendDesc.RenderTarget[0].SrcBlend = SrcBlendFactors[(renderState & GS_BLSRC_MASK) >> SrcFactorShift].BlendColor;
            blendDesc.RenderTarget[0].SrcBlendAlpha = SrcBlendFactors[(renderState & GS_BLSRC_MASK) >> SrcFactorShift].BlendAlpha;
            blendDesc.RenderTarget[0].DestBlend = DstBlendFactors[(renderState & GS_BLDST_MASK) >> DstFactorShift].BlendColor;
            blendDesc.RenderTarget[0].DestBlendAlpha = DstBlendFactors[(renderState & GS_BLDST_MASK) >> DstFactorShift].BlendAlpha;

            for (size_t i = 0; i < RT_STACK_WIDTH; ++i)
            {
                blendDesc.RenderTarget[i].BlendOp = BlendOp[(renderState & GS_BLEND_OP_MASK) >> BlendOpShift];
                blendDesc.RenderTarget[i].BlendOpAlpha = BlendOp[(renderState & GS_BLALPHA_MASK) >> BlendAlphaOpShift];
            }

            if (renderState & GS_BLALPHA_MASK)
            {
                blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
                blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            }
        }
    }

    // Color mask
    {
        uint32 mask = 0xfffffff0 | ((renderState & GS_COLMASK_MASK) >> GS_COLMASK_SHIFT);
        mask = (~mask) & 0xf;
        for (uint32 i = 0; i < RT_STACK_WIDTH; ++i)
        {
            blendDesc.RenderTarget[i].RenderTargetWriteMask = mask;
        }
    }

    // Depth-Stencil
    {
        depthStencilDesc.DepthWriteMask = (renderState & GS_DEPTHWRITE) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        depthStencilDesc.DepthEnable = !(renderState & GS_NODEPTHTEST);

        uint32 depthState = renderState;
        if (gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
        {
            depthState = ReverseDepthHelper::ConvertDepthFunc(renderState);
        }

        const uint32 DepthFuncShift = 20;
        D3D11_COMPARISON_FUNC DepthFunc[GS_DEPTHFUNC_MASK >> DepthFuncShift] =
        {
            D3D11_COMPARISON_LESS_EQUAL,     // GS_DEPTHFUNC_LEQUAL
            D3D11_COMPARISON_EQUAL,          // GS_DEPTHFUNC_EQUAL
            D3D11_COMPARISON_GREATER,        // GS_DEPTHFUNC_GREAT
            D3D11_COMPARISON_LESS,           // GS_DEPTHFUNC_LESS
            D3D11_COMPARISON_GREATER_EQUAL,  // GS_DEPTHFUNC_GEQUAL
            D3D11_COMPARISON_NOT_EQUAL,      // GS_DEPTHFUNC_NOTEQUAL
            D3D11_COMPARISON_EQUAL,          // GS_DEPTHFUNC_HIZEQUAL
        };

        depthStencilDesc.DepthFunc = DepthFunc[(depthState & GS_DEPTHFUNC_MASK) >> DepthFuncShift];

        depthStencilDesc.StencilEnable = (renderState & GS_STENCIL);
        depthStencilDesc.StencilReadMask = m_StencilReadMask;
        depthStencilDesc.StencilWriteMask = m_StencilWriteMask;

        depthStencilDesc.FrontFace.StencilFunc        = (D3D11_COMPARISON_FUNC)g_StencilFuncLookup[m_StencilState & FSS_STENCFUNC_MASK];
        depthStencilDesc.FrontFace.StencilFailOp      = (D3D11_STENCIL_OP)g_StencilOpLookup[(m_StencilState & FSS_STENCFAIL_MASK)  >> FSS_STENCFAIL_SHIFT];
        depthStencilDesc.FrontFace.StencilDepthFailOp = (D3D11_STENCIL_OP)g_StencilOpLookup[(m_StencilState & FSS_STENCZFAIL_MASK) >> FSS_STENCZFAIL_SHIFT];
        depthStencilDesc.FrontFace.StencilPassOp      = (D3D11_STENCIL_OP)g_StencilOpLookup[(m_StencilState & FSS_STENCPASS_MASK)  >> FSS_STENCPASS_SHIFT];
        depthStencilDesc.BackFace = depthStencilDesc.FrontFace;

        if (m_StencilState & FSS_STENCIL_TWOSIDED)
        {
            depthStencilDesc.BackFace.StencilFunc        = (D3D11_COMPARISON_FUNC)g_StencilFuncLookup[(m_StencilState & (FSS_STENCFUNC_MASK << FSS_CCW_SHIFT)) >>  FSS_CCW_SHIFT];
            depthStencilDesc.BackFace.StencilFailOp      = (D3D11_STENCIL_OP)g_StencilOpLookup[(m_StencilState  & (FSS_STENCFAIL_MASK  << FSS_CCW_SHIFT)) >> (FSS_STENCFAIL_SHIFT + FSS_CCW_SHIFT)];
            depthStencilDesc.BackFace.StencilDepthFailOp = (D3D11_STENCIL_OP)g_StencilOpLookup[(m_StencilState  & (FSS_STENCZFAIL_MASK << FSS_CCW_SHIFT)) >> (FSS_STENCZFAIL_SHIFT + FSS_CCW_SHIFT)];
            depthStencilDesc.BackFace.StencilPassOp      = (D3D11_STENCIL_OP)g_StencilOpLookup[(m_StencilState  & (FSS_STENCPASS_MASK  << FSS_CCW_SHIFT)) >> (FSS_STENCPASS_SHIFT + FSS_CCW_SHIFT)];
        }
    }
}

uint8 CDeviceGraphicsPSODesc::CombineVertexStreamMasks(uint8 fromShader, uint8 fromObject) const
{
    uint8 result = fromShader;

    if (fromObject & VSM_NORMALS)
    {
        result |= VSM_NORMALS;
    }

    if (fromObject & BIT(VSF_QTANGENTS))
    {
        result &= ~VSM_TANGENTS;
        result |= BIT(VSF_QTANGENTS);
    }

    if (fromObject & VSM_INSTANCED)
    {
        result |= VSM_INSTANCED;
    }

    return result;
}

template <typename T>
void HashSimpleType(AZ::Crc32& crc, const T& type)
{
    crc.Add(&type, sizeof(T), false);
}

void CDeviceGraphicsPSODesc::Build()
{
    AZ::Crc32 crc;
    HashSimpleType(crc, m_pShader);
    HashSimpleType(crc, m_technique);
    HashSimpleType(crc, m_bAllowTesselation);
    HashSimpleType(crc, m_ShaderFlags_RT);
    HashSimpleType(crc, m_ShaderFlags_MD);
    HashSimpleType(crc, m_ShaderFlags_MDV);
    HashSimpleType(crc, m_RenderState);
    HashSimpleType(crc, m_StencilState);
    HashSimpleType(crc, m_StencilReadMask);
    HashSimpleType(crc, m_StencilWriteMask);
    HashSimpleType(crc, m_ObjectStreamMask);
    HashSimpleType(crc, m_RenderTargetFormats);
    HashSimpleType(crc, m_DepthStencilFormat);
    HashSimpleType(crc, m_CullMode);
    HashSimpleType(crc, m_DepthBias);
    HashSimpleType(crc, m_DepthBiasClamp);
    HashSimpleType(crc, m_SlopeScaledDepthBias);
    HashSimpleType(crc, m_pResourceLayout);
    HashSimpleType(crc, m_VertexFormat.GetEnum());
    m_Hash = crc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CDeviceComputePSO::CDeviceComputePSO()
{
    m_pHwShaders.fill(NULL);
    m_pHwShaderInstances.fill(NULL);
    m_pDeviceShaders.fill(NULL);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CDeviceResourceSet::CDeviceResourceSet(EFlags flags)
{
    Clear();
    m_Flags = flags;
}

CDeviceResourceSet::~CDeviceResourceSet()
{
    Clear();
}


void CDeviceResourceSet::SetDirty([[maybe_unused]] bool bDirty)
{
    m_bDirty = true;
    CryInterlockedIncrement(&s_dirtyCount);
}

void CDeviceResourceSet::Clear()
{
    for (size_t i = 0; i < m_Textures.size(); i++)
    {
        auto& rsTexBind = m_Textures[i];

        CTexture* pTex = std::get<1>(rsTexBind.resource);
        if (pTex != nullptr)
        {
            pTex->RemoveInvalidateCallbacks(this);
        }
    }

    m_Textures.clear();
    m_Samplers.clear();
    m_ConstantBuffers.clear();
    m_Buffers.clear();
    m_bDirty = true;
}

void CDeviceResourceSet::SetTexture(ShaderSlot shaderSlot, _smart_ptr<CTexture> pTexture, SResourceView::KeyType resourceViewID, EShaderStage shaderStages)
{
    STextureData texData(std::tie(resourceViewID, pTexture), shaderStages);
    if (m_Textures.find(shaderSlot) != m_Textures.end() && m_Textures[shaderSlot] == texData)
    {
        return;
    }

    if (CTexture* pPreviousTexture = std::get<1>(m_Textures[shaderSlot].resource))
    {
        pPreviousTexture->RemoveInvalidateCallbacks(this);
    }

    m_Textures[shaderSlot] = texData;
    
    if (pTexture)
    {
        pTexture->AddInvalidateCallback(this, AZStd::bind(&CDeviceResourceSet::OnTextureChanged, this, AZStd::placeholders::_1));
    }

    m_bDirty = true;
}

void CDeviceResourceSet::SetSampler(ShaderSlot shaderSlot, int sampler, EShaderStage shaderStages)
{
    SSamplerData samplerData(sampler, shaderStages);
    if (m_Samplers.find(shaderSlot) != m_Samplers.end() && m_Samplers[shaderSlot] == samplerData)
    {
        return;
    }

    m_Samplers[shaderSlot] = samplerData;
    m_bDirty = true;
}

void CDeviceResourceSet::SetConstantBuffer(ShaderSlot shaderSlot, _smart_ptr<AzRHI::ConstantBuffer> pBuffer, EShaderStage shaderStages)
{
    SConstantBufferData cbData(pBuffer, shaderStages);
    if (m_ConstantBuffers.find(shaderSlot) != m_ConstantBuffers.end() && m_ConstantBuffers[shaderSlot] == cbData)
    {
        return;
    }

    m_ConstantBuffers[shaderSlot] = cbData;
    m_bDirty = true;
}

void CDeviceResourceSet::SetBuffer(ShaderSlot shaderSlot, WrappedDX11Buffer buffer, EShaderStage shaderStages)
{
    SBufferData bufferData(buffer, shaderStages);
    if (m_Buffers.find(shaderSlot) != m_Buffers.end() && m_Buffers[shaderSlot] == bufferData)
    {
        return;
    }

    m_Buffers[shaderSlot] = bufferData;
    m_bDirty = true;
}

EShaderStage CDeviceResourceSet::GetShaderStages() const
{
    EShaderStage result = EShaderStage_None;

    for (const auto& it : m_ConstantBuffers)
    {
        result |= it.second.shaderStages;
    }

    for (const auto& it : m_Textures)
    {
        result |= it.second.shaderStages;
    }

    for (const auto& it : m_Samplers)
    {
        result |=  it.second.shaderStages;
    }

    return result;
}

bool CDeviceResourceSet::Fill([[maybe_unused]] CShader* pShader, CShaderResources* pResources, EShaderStage shaderStages)
{
    Clear();

    for (EEfResTextures texType = EFTT_DIFFUSE; texType < EFTT_MAX; texType = EEfResTextures(texType + 1))
    {
        CTexture*       pTex = TextureHelpers::LookupTexDefault(texType);
        SEfResTexture*  pRTextureRes = pResources->GetTextureResource(texType);
        if (pRTextureRes && pRTextureRes->m_Sampler.m_pTex)
        {
            pTex = pRTextureRes->m_Sampler.m_pTex;
        }
        else if (pRTextureRes)
        {
            AZ_Warning("Graphics", pRTextureRes->m_Sampler.m_pTex, "Texture at slot %d is Null", (uint16)texType );
        }

        auto bindSlot = IShader::GetTextureSlot(texType);
        SetTexture(bindSlot, pTex, SResourceView::DefaultView, shaderStages);
    }
/*
    [Shader System TO DO] - replace with the following optimized slots set code 
    for (auto iter = pResources->m_TexturesResourcesMap.begin(); iter != pResources->m_TexturesResourcesMap.end(); ++iter)
    {
        SEfResTexture*          pTexture = &iter->second;
        CTexture*               pTex = pTexture->m_Sampler.m_pTex;
        uint16                  texSlot = iter->first;
        const STexSamplerRT&    smp = pTexture->m_Sampler;

        AZ_Warning("Graphics", pTex, "Texture is Null in CDeviceResourceSet.");

        SetTexture(texSlot, pTex, SResourceView::DefaultView, shaderStages);
    }
*/
    // Eventually we should only have one constant buffer for all shader stages. for now just pick the one from the pixel shader
    m_ConstantBuffers[eConstantBufferShaderSlot_PerMaterial]  = SConstantBufferData(pResources->GetConstantBuffer(), shaderStages);
    return true;
}

void CDeviceResourceSet::OnTextureChanged([[maybe_unused]] uint32 dirtyFlags)
{
    SetDirty(true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeviceResourceLayout::SetInlineConstants(uint32 numConstants)
{
    m_InlineConstantCount += numConstants;
}

void CDeviceResourceLayout::SetResourceSet(uint32 bindSlot, CDeviceResourceSetPtr pResourceSet)
{
    m_ResourceSets[bindSlot] = pResourceSet;
}

void CDeviceResourceLayout::SetConstantBuffer(uint32 bindSlot, EConstantBufferShaderSlot shaderSlot, EShaderStage shaderStages)
{
    m_ConstantBuffers[bindSlot].shaderSlot = shaderSlot;
    m_ConstantBuffers[bindSlot].shaderStages = shaderStages;
}

CDeviceResourceLayout::CDeviceResourceLayout()
{
    Clear();
}

void CDeviceResourceLayout::Clear()
{
    m_InlineConstantCount = 0;
    m_ConstantBuffers.clear();
    m_ResourceSets.clear();
}

bool CDeviceResourceLayout::IsValid()
{
    // TODO: validate buffers

    // need to have at least one resource set or constant buffer/inline constants
    if (m_ResourceSets.empty() && m_ConstantBuffers.empty() && m_InlineConstantCount == 0)
    {
        CRY_ASSERT_MESSAGE(false, "Invalid Resource Layout: no data");
        return false;
    }

    // check for overlapping resource set and constant buffer bind slots
    std::set<uint32> usedBindSlots;
    std::map<ShaderSlot, CDeviceResourceSet::STextureData>        shaderTexBinds;
    std::map<ShaderSlot, CDeviceResourceSet::SConstantBufferData> shaderCBufferBinds;

    if (m_InlineConstantCount > 0)
    {
        usedBindSlots.insert(0);
    }

    auto validateTex = [&](int shaderSlot, const CDeviceResourceSet::STextureData& newTex)
        {
            auto it = shaderTexBinds.find(shaderSlot);
            if (it != shaderTexBinds.end()) // existing texture found on this shader slot
            {
                const auto& existingTex = it->second;
                if (!(newTex == existingTex))
                {
                    CRY_ASSERT_MESSAGE(false, "Invalid Resource Layout: Cannot bind multiple textures to same shader slot (even when shader stages differ) - DX12 limitation");
                    return false; // DX12 does not allow binding different textures to same slot (even when shader stages differ) in one descriptor table
                }
            }

            return true;
        };

    auto validateCB = [&](int shaderSlot, const CDeviceResourceSet::SConstantBufferData& newCB)
        {
            auto it = shaderCBufferBinds.find(shaderSlot);
            bool existed = (it != shaderCBufferBinds.end() && it->second.shaderStages == newCB.shaderStages);
            CRY_ASSERT_MESSAGE(!existed, "Invalid Resource Layout: Cannot bind multiple constant buffers to same shader slot (even when shader stages differ) - DX12 limitation");
            return !existed;
        };

    auto validateResourceSetShaderStages = [](EShaderStage resourceSetShaderStages, EShaderStage currentItemShaderStages)
        {
            // DX12 limitation: all textures and constant buffers in a resource set must be bound the the same shader stages
            return resourceSetShaderStages == EShaderStage_None || currentItemShaderStages == resourceSetShaderStages;
        };

    for (const auto& rs : m_ResourceSets)
    {
        if (usedBindSlots.insert(rs.first).second == false)
        {
            CRY_ASSERT_MESSAGE(false, "Invalid Resource Layout: Multiple resources on same bind slot");
            return false;
        }

        EShaderStage currentShaderStages = EShaderStage_None;

        for (const auto& rsTexBind : rs.second->m_Textures)
        {
            if (!validateTex(rsTexBind.first, rsTexBind.second))
            {
                return false;
            }

            if (!validateResourceSetShaderStages(currentShaderStages, rsTexBind.second.shaderStages))
            {
                return false;
            }

            shaderTexBinds[rsTexBind.first] = rsTexBind.second;
            currentShaderStages = rsTexBind.second.shaderStages;
        }

        for (const auto& rsCbBind : rs.second->m_ConstantBuffers)
        {
            if (!validateCB(rsCbBind.first, rsCbBind.second))
            {
                return false;
            }

            // DX12 limitation: all textures and constant buffers in a resource set must be bound the the same shader stages
            if (!validateResourceSetShaderStages(currentShaderStages, rsCbBind.second.shaderStages))
            {
                return false;
            }

            shaderCBufferBinds[rsCbBind.first] = rsCbBind.second;
            currentShaderStages = rsCbBind.second.shaderStages;
        }
    }

    for (const auto& cb : m_ConstantBuffers)
    {
        uint32 resourceLayoutBindSlot = cb.first;
        if (usedBindSlots.insert(resourceLayoutBindSlot).second == false)
        {
            CRY_ASSERT_MESSAGE(false, "Invalid Resource Layout: Multiple resources on same bind slot");
            return false;
        }

        CDeviceResourceSet::SConstantBufferData cbData(NULL, cb.second.shaderStages);
        EConstantBufferShaderSlot shaderSlot = cb.second.shaderSlot;
        if (!validateCB(shaderSlot, cbData))
        {
            return false;
        }

        shaderCBufferBinds[shaderSlot] = cbData;
    }

    // make sure there are no 'holes' in the used binding slots
    {
        int previousSlot = -1;
        for (auto slot : usedBindSlots)
        {
            if (slot != previousSlot + 1)
            {
                CRY_ASSERT_MESSAGE(false, "Invalid Resource Layout: gap in bind slots");
                return false;
            }

            previousSlot = slot;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeviceGraphicsCommandList::Reset()
{
    m_pCurrentPipelineState = nullptr;
    m_pCurrentResources.fill(nullptr);
    m_CurrentStencilRef = -1;
    ResetImpl();
}

void CDeviceGraphicsCommandList::SetPipelineState(CDeviceGraphicsPSOPtr devicePSO)
{
    if (m_pCurrentPipelineState != devicePSO)
    {
        m_pCurrentPipelineState = devicePSO;
        SetPipelineStateImpl(devicePSO);
    }

#ifndef _RELEASE
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    CryInterlockedIncrement(&rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_NumStateChanges);
#endif
}

void CDeviceGraphicsCommandList::SetResources(uint32 bindSlot, CDeviceResourceSet* pResources)
{
    assert(bindSlot < m_pCurrentResources.size());
    if (m_pCurrentResources[bindSlot] != pResources)
    {
        m_pCurrentResources[bindSlot] = pResources;
        SetResourcesImpl(bindSlot, pResources);
    }
}

void CDeviceGraphicsCommandList::SetStencilRef(uint8 stencilRefValue)
{
    if (m_CurrentStencilRef != int(stencilRefValue))
    {
        m_CurrentStencilRef = stencilRefValue;
        SetStencilRefImpl(stencilRefValue);
    }
}

void CDeviceGraphicsCommandList::Draw(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    DrawImpl(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);

#if defined(ENABLE_PROFILING_CODE)
    int nPrimitives;

    switch (m_pCurrentPipelineState->m_PrimitiveTypeForProfiling)
    {
    case eptTriangleList:
        nPrimitives = VertexCountPerInstance / 3;
        assert(VertexCountPerInstance % 3 == 0);
        break;
    case eptTriangleStrip:
        nPrimitives = VertexCountPerInstance - 2;
        assert(VertexCountPerInstance > 2);
        break;
    case eptLineList:
        nPrimitives = VertexCountPerInstance / 2;
        assert(VertexCountPerInstance % 2 == 0);
        break;
    case eptLineStrip:
        nPrimitives = VertexCountPerInstance - 1;
        assert(VertexCountPerInstance > 1);
        break;
    case eptPointList:
        nPrimitives = VertexCountPerInstance;
        assert(VertexCountPerInstance > 0);
        break;
    case ept1ControlPointPatchList:
        nPrimitives = VertexCountPerInstance;
        assert(VertexCountPerInstance > 0);
        break;

#ifndef _RELEASE
    default:
        nPrimitives = 0;
        assert(0);
#endif
    }

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    CryInterlockedAdd(&rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP], nPrimitives * InstanceCount);
    CryInterlockedIncrement(&rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]);
#endif
}

void CDeviceGraphicsCommandList::DrawIndexed(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, int BaseVertexLocation, uint32 StartInstanceLocation)
{
    DrawIndexedImpl(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

#if defined(ENABLE_PROFILING_CODE)
    int nPrimitives;

    switch (m_pCurrentPipelineState->m_PrimitiveTypeForProfiling)
    {
    case eptTriangleList:
        nPrimitives = IndexCountPerInstance / 3;
        assert(IndexCountPerInstance % 3 == 0);
        break;
    case ept4ControlPointPatchList:
        nPrimitives = IndexCountPerInstance >> 2;
        assert(IndexCountPerInstance % 4 == 0);
        break;
    case ept3ControlPointPatchList:
        nPrimitives = IndexCountPerInstance / 3;
        assert(IndexCountPerInstance % 3 == 0);
        break;
    case eptTriangleStrip:
        nPrimitives = IndexCountPerInstance - 2;
        assert(IndexCountPerInstance > 2);
        break;
    case eptLineList:
        nPrimitives = IndexCountPerInstance >> 1;
        assert(IndexCountPerInstance % 2 == 0);
        break;
#ifdef _DEBUG
    default:
        nPrimitives = 0;
        assert(0);
#endif
    }

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    CryInterlockedAdd(&rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP], nPrimitives);
    CryInterlockedIncrement(&rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeviceComputeCommandList::SetPipelineState(CDeviceComputePSOPtr devicePSO)
{
    m_pCurrentPipelineState = devicePSO;
    SetPipelineStateImpl(devicePSO);

#ifndef _RELEASE
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    CryInterlockedIncrement(&rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_NumStateChanges);
#endif
}

void CDeviceComputeCommandList::Dispatch(uint32 X, uint32 Y, uint32 Z)
{
    DispatchImpl(X, Y, Z);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CDeviceGraphicsPSOPtr CDeviceObjectFactory::CreateGraphicsPSO(const CDeviceGraphicsPSODesc& psoDesc)
{
    auto it = m_PsoCache.find(psoDesc);
    if (it != m_PsoCache.end())
    {
        return it->second.get();
    }

    if (auto pPSO = CreateGraphicsPSOImpl(psoDesc))
    {
        auto insertResult = m_PsoCache.emplace(psoDesc, std::move(pPSO));
        return insertResult.first->second.get();
    }

    return nullptr;
}

#if !defined(NULL_RENDERER)
CDeviceObjectFactory& CDeviceObjectFactory::GetInstance()
{
    static CDeviceObjectFactory ext12;
    return ext12;
}
#endif
