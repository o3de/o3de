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
#include "CryUtils.h"

#include <AzCore/std/smart_ptr/make_shared.h>

#if !defined(CRY_USE_DX12_NATIVE)
#define DEVICEWRAPPER12_D3D11_CPP_WRAP_DX11
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(DeviceWrapper12_D3D11_cpp)
#endif
#if defined(DEVICEWRAPPER12_D3D11_CPP_WRAP_DX11)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DX11_COMMANDLIST_REDUNDANT_STATE_FILTERING

class CDeviceGraphicsPSO_DX11
    : public CDeviceGraphicsPSO
{
public:
    CDeviceGraphicsPSO_DX11();

    bool Init(const CDeviceGraphicsPSODesc& psoDesc);

    _smart_ptr<ID3D11RasterizerState>                 pRasterizerState;
    _smart_ptr<ID3D11BlendState>                      pBlendState;
    _smart_ptr<ID3D11DepthStencilState>               pDepthStencilState;
    _smart_ptr<ID3D11InputLayout>                     pInputLayout;

    std::array<void*, eHWSC_Num>                      m_pDeviceShaders;

    std::array<std::array<uint8, MAX_TMU>, eHWSC_Num> m_Samplers;
    std::array<std::array<uint8, MAX_TMU>, eHWSC_Num> m_SRVs;

    std::array<uint8, eHWSC_Num>                      m_NumSamplers;
    std::array<uint8, eHWSC_Num>                      m_NumSRVs;

    // Do we still need these?
    uint64                                            m_ShaderFlags_RT;
    uint32                                            m_ShaderFlags_MD;
    uint32                                            m_ShaderFlags_MDV;

    D3DPrimitiveType                                  m_PrimitiveTopology;
};

CDeviceGraphicsPSO_DX11::CDeviceGraphicsPSO_DX11()
{
    m_PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    m_ShaderFlags_RT = 0;
    m_ShaderFlags_MD = 0;
    m_ShaderFlags_MDV = 0;

    m_NumSamplers.fill(0);
    m_NumSRVs.fill(0);
}

bool CDeviceGraphicsPSO_DX11::Init(const CDeviceGraphicsPSODesc& psoDesc)
{
    CD3D9Renderer* rd = gcpRendD3D;

    pRasterizerState = NULL;
    pBlendState = NULL;
    pDepthStencilState = NULL;
    pInputLayout = NULL;

    m_NumSamplers.fill(0);
    m_NumSRVs.fill(0);

    std::array<SDeviceObjectHelpers::SShaderInstanceInfo, eHWSC_Num> hwShaders;
    bool shadersAvailable = SDeviceObjectHelpers::GetShaderInstanceInfo(hwShaders, psoDesc.m_pShader, psoDesc.m_technique, psoDesc.m_ShaderFlags_RT, psoDesc.m_ShaderFlags_MD, psoDesc.m_ShaderFlags_MDV, nullptr, psoDesc.m_bAllowTesselation);
    if (!shadersAvailable)
        return false;

    // validate shaders first
    for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
    {
        if (hwShaders[shaderClass].pHwShader && (hwShaders[shaderClass].pHwShaderInstance == NULL || hwShaders[shaderClass].pDeviceShader == NULL))
        {
            return false;
        }

        m_pDeviceShaders[shaderClass] = hwShaders[shaderClass].pDeviceShader;

        // TODO: remove
        m_pHwShaders[shaderClass]         = hwShaders[shaderClass].pHwShader;
        m_pHwShaderInstances[shaderClass] = hwShaders[shaderClass].pHwShaderInstance;
    }

    D3D11_RASTERIZER_DESC rasterizerDesc;
    D3D11_BLEND_DESC blendDesc;
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

    psoDesc.FillDescs(rasterizerDesc, blendDesc, depthStencilDesc);

    uint32 rasterStateIndex = rd->GetOrCreateRasterState(rasterizerDesc);
    uint32 blendStateIndex  = rd->GetOrCreateBlendState(blendDesc);
    uint32 depthStateIndex  = rd->GetOrCreateDepthState(depthStencilDesc);

    if (rasterStateIndex == uint32(-1) || blendStateIndex == uint32(-1) || depthStateIndex == uint32(-1))
    {
        return false;
    }

    pDepthStencilState = rd->m_StatesDP[depthStateIndex].pState;
    pRasterizerState =   rd->m_StatesRS[rasterStateIndex].pState;
    pBlendState =        rd->m_StatesBL[blendStateIndex].pState;

    // input layout
    {
#if defined(FEATURE_PER_SHADER_INPUT_LAYOUT_CACHE)
        CRY_ASSERT(false); // TODO: IMPLEMENT
        return false;
#endif

        if (CHWShader_D3D::SHWSInstance* pVsInstance = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(hwShaders[eHWSC_Vertex].pHwShaderInstance))
        {
            uint8 streamMask = psoDesc.CombineVertexStreamMasks(uint8(pVsInstance->m_VStreamMask_Decl), psoDesc.m_ObjectStreamMask);

            const bool bMorph = false;
            const bool bInstanced = (streamMask & VSM_INSTANCED) != 0;
            AZ::u32 declCacheKey = pVsInstance->GenerateVertexDeclarationCacheKey(psoDesc.m_VertexFormat);
            SOnDemandD3DVertexDeclarationCache& declCache = rd->m_RP.m_D3DVertexDeclarationCache[streamMask >> 1][bMorph || bInstanced][declCacheKey];

            if (!declCache.m_pDeclaration)
            {
                SOnDemandD3DVertexDeclaration Decl;
                rd->EF_OnDemandVertexDeclaration(Decl, streamMask >> 1, psoDesc.m_VertexFormat, bMorph, bInstanced);

                if (!Decl.m_Declaration.empty())
                {
                    auto hr = rd->GetDevice().CreateInputLayout(&Decl.m_Declaration[0], Decl.m_Declaration.size(), pVsInstance->m_pShaderData, pVsInstance->m_nDataSize, &declCache.m_pDeclaration);
                    if (!SUCCEEDED(hr))
                    {
                        return false;
                    }
                }
            }

            pInputLayout = declCache.m_pDeclaration;
        }

        if (!pInputLayout)
        {
            return false;
        }
    }

    for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
    {
        if (auto pShaderInstance = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(hwShaders[shaderClass].pHwShaderInstance))
        {
            for (const auto& smp : pShaderInstance->m_Samplers)
            {
                m_Samplers[shaderClass][m_NumSamplers[shaderClass]++] = uint8(smp.m_BindingSlot);
            }

            for (const auto& tex : pShaderInstance->m_Textures)
            {
                m_SRVs[shaderClass][m_NumSRVs[shaderClass]++] = uint8(tex.m_BindingSlot);
            }
        }
    }

    m_PrimitiveTopology = rd->FX_ConvertPrimitiveType(psoDesc.m_PrimitiveType);
    m_ShaderFlags_RT = psoDesc.m_ShaderFlags_RT;
    m_ShaderFlags_MD = psoDesc.m_ShaderFlags_MD;
    m_ShaderFlags_MDV = psoDesc.m_ShaderFlags_MDV;

#if defined(ENABLE_PROFILING_CODE)
    m_PrimitiveTypeForProfiling = psoDesc.m_PrimitiveType;
#endif

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceComputePSO_DX11 final
    : public CDeviceComputePSO
{
public:
    CDeviceComputePSO_DX11() {}
    virtual ~CDeviceComputePSO_DX11() {}
    virtual bool Build() final { return false; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct CDeviceResourceSet_DX11
    : CDeviceResourceSet
{
    static const void* const InvalidPointer;

    struct SCompiledConstantBuffer
    {
        AzRHI::ConstantBuffer* m_constantBuffer;
        uint32 offset;
        uint32 size;
        uint64 code;
        int    m_shaderSlot;
    };

    CDeviceResourceSet_DX11(CDeviceResourceSet::EFlags flags)
        : CDeviceResourceSet(flags)
    {}

    virtual void Prepare() final {}
    virtual void Build() final;

    // set via reflection from shader
    std::array< std::array<ID3D11ShaderResourceView*, MAX_TMU>, eHWSC_Num>                        compiledSRVs;
    std::array< std::array<ID3D11SamplerState*,       MAX_TMU>, eHWSC_Num>                        compiledSamplers;

    // set directly
    std::array< std::array<SCompiledConstantBuffer, eConstantBufferShaderSlot_Count>, eHWSC_Num>  compiledCBs;
    std::array<uint8_t,                                                               eHWSC_Num>  numCompiledCBs;
};

const void* const CDeviceResourceSet_DX11::InvalidPointer = (const void* const)0xBADA55;

void CDeviceResourceSet_DX11::Build()
{
    for (auto& stage : compiledSRVs)
    {
        stage.fill((ID3D11ShaderResourceView*)InvalidPointer);
    }

    for (auto& stage : compiledSamplers)
    {
        stage.fill((ID3D11SamplerState*)InvalidPointer);
    }

    SCompiledConstantBuffer nullBuffer;
    ZeroStruct(nullBuffer);

    for (auto& stage : compiledCBs)
    {
        stage.fill(nullBuffer);
    }

    numCompiledCBs.fill(0);

    for (const auto& it : m_Textures)
    {
        ID3D11ShaderResourceView* pSrv = nullptr;

        if (CTexture* pTex = std::get<1>(it.second.resource))
        {
            if (pTex->GetDevTexture())
            {
                SResourceView::KeyType srvKey = std::get<0>(it.second.resource);
                pSrv = reinterpret_cast<ID3D11ShaderResourceView*>(pTex->GetShaderResourceView(srvKey));
            }
        }

        for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
        {
            if (it.second.shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
            {
                compiledSRVs[shaderClass][it.first] = pSrv;
            }
        }

    }

    for (const auto& it : m_Buffers)
    {
        ID3D11ShaderResourceView* pSrv = it.second.resource.GetShaderResourceView();

        for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
        {
            if (it.second.shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
            {
                compiledSRVs[shaderClass][it.first] = pSrv;
            }
        }
        AZ_Assert(pSrv != nullptr, "null buffer");
    }

    for (const auto& it : m_Samplers)
    {
        ID3D11SamplerState* pSamplerState = nullptr;

        if (it.second.resource >= 0 && it.second.resource < CTexture::s_TexStates.size())
        {
            const auto& ts = CTexture::s_TexStates[it.second.resource];
            pSamplerState = reinterpret_cast<ID3D11SamplerState*>(ts.m_pDeviceState);
        }

        for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
        {
            if (it.second.shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
            {
                compiledSamplers[shaderClass][it.first] = pSamplerState;
            }
        }
        AZ_Assert(pSamplerState != nullptr, "null sampler");
    }

    for (const auto& it : m_ConstantBuffers)
    {
        SCompiledConstantBuffer compiledCB;
        ZeroStruct(compiledCB);

        if (it.second.resource)
        {
            AzRHI::ConstantBuffer* buffer = it.second.resource.get();
            compiledCB.m_constantBuffer = buffer;
            compiledCB.code = buffer->GetCode();
            compiledCB.offset = buffer->GetByteOffset();
            compiledCB.size = buffer->GetByteCount();
            compiledCB.m_shaderSlot = it.first;
        }

        for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
        {
            if (it.second.shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
            {
                compiledCBs[shaderClass][numCompiledCBs[shaderClass]++] = compiledCB;
            }
        }
        AZ_Assert(compiledCB.m_constantBuffer != nullptr, "null constant buffer");
    }

    m_bDirty = false;
}

class CDeviceResourceLayout_DX11
    : public CDeviceResourceLayout
{
public:
    ~CDeviceResourceLayout_DX11() override = default;
    virtual bool Build() final
    {
        return IsValid();
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDeviceGraphicsCommandList_DX11
    : public CDeviceGraphicsCommandList
{
public:
    template<typename T>
    struct SCachedValue
    {
        T cachedValue;
        SCachedValue() {}
        SCachedValue(const T& value)
            : cachedValue(value) {}

        template<typename U>
        inline bool Set(U newValue)
        {
#if defined(DX11_COMMANDLIST_REDUNDANT_STATE_FILTERING)
            if (cachedValue == newValue)
            {
                return false;
            }

            cachedValue = newValue;
            return true;
#else
            return true;
#endif
        }
    };

    CDeviceGraphicsCommandList_DX11()
    {
        Reset();
    }

    void SetResources_RequestedByShaderOnly(CDeviceResourceSet_DX11* pResources);
    void SetResources_All(CDeviceResourceSet_DX11* pResources);

    SCachedValue<ID3D11DepthStencilState*>              m_CurrentDS;
    SCachedValue<ID3D11RasterizerState*>                m_CurrentRS;
    SCachedValue<ID3D11BlendState*>                     m_CurrentBS;
    SCachedValue<ID3D11InputLayout*>                    m_CurrentInputLayout;
    SCachedValue<D3D11_PRIMITIVE_TOPOLOGY>              m_CurrentTopology;
    SCachedValue<SStreamInfo>                           m_CurrentVertexStream[MAX_STREAMS];
    SCachedValue<SStreamInfo>                           m_CurrentIndexStream;
    SCachedValue<void*>                                 m_CurrentShader[eHWSC_Num];
    SCachedValue<ID3D11ShaderResourceView*>             m_CurrentSRV[eHWSC_Num][MAX_TMU];
    SCachedValue<ID3D11SamplerState*>                   m_CurrentSamplerState[eHWSC_Num][MAX_TMU];
    SCachedValue<uint64>                                m_CurrentCB[eHWSC_Num][eConstantBufferShaderSlot_Count];

    std::array< std::array<uint8, MAX_TMU>, eHWSC_Num > m_SRVs;
    std::array< std::array<uint8, MAX_TMU>, eHWSC_Num > m_Samplers;

    std::array<uint8, eHWSC_Num>                        m_NumSRVs;
    std::array<uint8, eHWSC_Num>                        m_NumSamplers;

    EShaderStage                                        m_ValidShaderStages;
};

#define GET_DX11_COMMANDLIST(abstractCommandList) reinterpret_cast<CDeviceGraphicsCommandList_DX11*>(abstractCommandList)

void CDeviceGraphicsCommandList::SetRenderTargets(uint32 targetCount, CTexture** pTargets, SDepthTexture* pDepthTarget)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    ID3D11RenderTargetView* pRTV[RT_STACK_WIDTH] = { NULL };

    for (int i = 0; i < targetCount; ++i)
    {
        if (auto pView = pTargets[i]->GetSurface(0, 0))
        {
            pRTV[i] = pView;
        }
    }

    rd->GetDeviceContext().OMSetRenderTargets(targetCount, pRTV, pDepthTarget ? static_cast<D3DDepthSurface*>(pDepthTarget->pSurf) : NULL);
}

void CDeviceGraphicsCommandList::SetViewports(uint32 vpCount, const D3DViewPort* pViewports)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    rd->GetDeviceContext().RSSetViewports(vpCount, pViewports);
}

void CDeviceGraphicsCommandList::SetScissorRects(uint32 rcCount, const D3DRectangle* pRects)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    rd->GetDeviceContext().RSSetScissorRects(rcCount, pRects);
}

void CDeviceGraphicsCommandList::SetPipelineStateImpl(CDeviceGraphicsPSOPtr devicePSO)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    CDeviceGraphicsPSO_DX11* pDevicePSO = reinterpret_cast<CDeviceGraphicsPSO_DX11*>(devicePSO);
    CDeviceGraphicsCommandList_DX11* pCmdList = GET_DX11_COMMANDLIST(this);

    // RasterState, DepthStencilState, BlendState
    if (pCmdList->m_CurrentDS.Set(pDevicePSO->pDepthStencilState))
    {
        rd->m_DevMan.SetDepthStencilState(pDevicePSO->pDepthStencilState, 0);
    }
    if (pCmdList->m_CurrentBS.Set(pDevicePSO->pBlendState))
    {
        rd->m_DevMan.SetBlendState(pDevicePSO->pBlendState, NULL, 0xffffffff);
    }
    if (pCmdList->m_CurrentRS.Set(pDevicePSO->pRasterizerState))
    {
        rd->m_DevMan.SetRasterState(pDevicePSO->pRasterizerState);
    }

    // Shaders
    const std::array<void*, eHWSC_Num>& shaders = pDevicePSO->m_pDeviceShaders;
    if (pCmdList->m_CurrentShader[eHWSC_Vertex].Set(shaders[eHWSC_Vertex]))
    {
        rd->m_DevMan.BindShader(eHWSC_Vertex, (ID3D11Resource*)shaders[eHWSC_Vertex]);
    }
    if (pCmdList->m_CurrentShader[eHWSC_Pixel].Set(shaders[eHWSC_Pixel]))
    {
        rd->m_DevMan.BindShader(eHWSC_Pixel, (ID3D11Resource*)shaders[eHWSC_Pixel]);
    }
    if (pCmdList->m_CurrentShader[eHWSC_Geometry].Set(shaders[eHWSC_Geometry]))
    {
        rd->m_DevMan.BindShader(eHWSC_Geometry, (ID3D11Resource*)shaders[eHWSC_Geometry]);
    }
    if (pCmdList->m_CurrentShader[eHWSC_Domain].Set(shaders[eHWSC_Domain]))
    {
        rd->m_DevMan.BindShader(eHWSC_Domain, (ID3D11Resource*)shaders[eHWSC_Domain]);
    }
    if (pCmdList->m_CurrentShader[eHWSC_Hull].Set(shaders[eHWSC_Hull]))
    {
        rd->m_DevMan.BindShader(eHWSC_Hull, (ID3D11Resource*)shaders[eHWSC_Hull]);
    }

    // input layout and topology
    if (pCmdList->m_CurrentInputLayout.Set(pDevicePSO->pInputLayout))
    {
        rd->m_DevMan.BindVtxDecl(pDevicePSO->pInputLayout);
    }
    if (pCmdList->m_CurrentTopology.Set(pDevicePSO->m_PrimitiveTopology))
    {
        rd->m_DevMan.BindTopology(pDevicePSO->m_PrimitiveTopology);
    }

    // update valid shader mask
    pCmdList->m_ValidShaderStages = EShaderStage_None;
    if (pDevicePSO->m_pDeviceShaders[eHWSC_Vertex])
    {
        pCmdList->m_ValidShaderStages |= EShaderStage_Vertex;
    }
    if (pDevicePSO->m_pDeviceShaders[eHWSC_Pixel])
    {
        pCmdList->m_ValidShaderStages |= EShaderStage_Pixel;
    }
    if (pDevicePSO->m_pDeviceShaders[eHWSC_Geometry])
    {
        pCmdList->m_ValidShaderStages |= EShaderStage_Geometry;
    }
    if (pDevicePSO->m_pDeviceShaders[eHWSC_Domain])
    {
        pCmdList->m_ValidShaderStages |= EShaderStage_Domain;
    }
    if (pDevicePSO->m_pDeviceShaders[eHWSC_Hull])
    {
        pCmdList->m_ValidShaderStages |= EShaderStage_Hull;
    }

    pCmdList->m_SRVs     = pDevicePSO->m_SRVs;
    pCmdList->m_Samplers = pDevicePSO->m_Samplers;

    pCmdList->m_NumSRVs     = pDevicePSO->m_NumSRVs;
    pCmdList->m_NumSamplers = pDevicePSO->m_NumSamplers;

    // TheoM TODO: REMOVE once shaders are set up completely via pso
    rd->m_RP.m_FlagsShader_RT = pDevicePSO->m_ShaderFlags_RT;
    rd->m_RP.m_FlagsShader_MD = pDevicePSO->m_ShaderFlags_MD;
    rd->m_RP.m_FlagsShader_MDV = pDevicePSO->m_ShaderFlags_MDV;
}

void CDeviceGraphicsCommandList::SetResourceLayout([[maybe_unused]] CDeviceResourceLayout* pResourceLayout)
{
}

void CDeviceGraphicsCommandList::SetResourcesImpl([[maybe_unused]] uint32 bindSlot, CDeviceResourceSet* pResources)
{
    auto pCmdList = GET_DX11_COMMANDLIST(this);
    auto pResourcesDX11 = reinterpret_cast<CDeviceResourceSet_DX11*>(pResources);

    if (pResources->GetFlags() & CDeviceResourceSet::EFlags_ForceSetAllState)
    {
        pCmdList->SetResources_All(pResourcesDX11);
    }
    else
    {
        pCmdList->SetResources_RequestedByShaderOnly(pResourcesDX11);
    }
}

void CDeviceGraphicsCommandList_DX11::SetResources_RequestedByShaderOnly(CDeviceResourceSet_DX11* pResources)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
    {
        if (m_ValidShaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
        {
            // Bind SRVs
            // if (!pResources->compiledSRVs.empty()) // currently this is always the case
            {
                for (int i = 0; i < m_NumSRVs[shaderClass]; ++i)
                {
                    uint8 srvSlot = m_SRVs[shaderClass][i];
                    ID3D11ShaderResourceView* pSrv = pResources->compiledSRVs[shaderClass][srvSlot];

                    if (pSrv != CDeviceResourceSet_DX11::InvalidPointer)
                    {
                        if (m_CurrentSRV[shaderClass][srvSlot].Set(pSrv))
                        {
                            switch (shaderClass)
                            {
                            case eHWSC_Vertex:
                                rd->m_DevMan.BindSRV(eHWSC_Vertex, pSrv, srvSlot);
                                break;
                            case eHWSC_Pixel:
                                rd->m_DevMan.BindSRV(eHWSC_Pixel, pSrv, srvSlot);
                                break;
                            case eHWSC_Geometry:
                                rd->m_DevMan.BindSRV(eHWSC_Geometry, pSrv, srvSlot);
                                break;
                            case eHWSC_Domain:
                                rd->m_DevMan.BindSRV(eHWSC_Domain, pSrv, srvSlot);
                                break;
                            case eHWSC_Hull:
                                rd->m_DevMan.BindSRV(eHWSC_Hull, pSrv, srvSlot);
                                break;
                            default:
                                CRY_ASSERT(false);
                            }
                        }
                    }
                }
            }

            // Bind Samplers
            if (!pResources->compiledSamplers.empty())
            {
                for (int i = 0; i < m_NumSamplers[shaderClass]; ++i)
                {
                    uint8 samplerSlot = m_Samplers[shaderClass][i];
                    ID3D11SamplerState* pSamplerState = pResources->compiledSamplers[shaderClass][samplerSlot];

                    if (pSamplerState != CDeviceResourceSet_DX11::InvalidPointer)
                    {
                        if (m_CurrentSamplerState[shaderClass][samplerSlot].Set(pSamplerState))
                        {
                            switch (shaderClass)
                            {
                            case eHWSC_Vertex:
                                rd->m_DevMan.BindSampler(eHWSC_Vertex, pSamplerState, samplerSlot);
                                break;
                            case eHWSC_Pixel:
                                rd->m_DevMan.BindSampler(eHWSC_Pixel, pSamplerState, samplerSlot);
                                break;
                            case eHWSC_Geometry:
                                rd->m_DevMan.BindSampler(eHWSC_Geometry, pSamplerState, samplerSlot);
                                break;
                            case eHWSC_Domain:
                                rd->m_DevMan.BindSampler(eHWSC_Domain, pSamplerState, samplerSlot);
                                break;
                            case eHWSC_Hull:
                                rd->m_DevMan.BindSampler(eHWSC_Hull, pSamplerState, samplerSlot);
                                break;
                            default:
                                CRY_ASSERT(false);
                            }
                        }
                    }
                }
            }

            // Bind constant buffers
            for (int i = 0; i < pResources->numCompiledCBs[shaderClass]; ++i)
            {
                const CDeviceResourceSet_DX11::SCompiledConstantBuffer& cb = pResources->compiledCBs[shaderClass][i];
                if (m_CurrentCB[shaderClass][cb.m_shaderSlot].Set(cb.code))
                {
                    switch (shaderClass)
                    {
                    case eHWSC_Vertex:
                        rd->m_DevMan.BindConstantBuffer(eHWSC_Vertex, cb.m_constantBuffer, cb.m_shaderSlot);
                        break;
                    case eHWSC_Pixel:
                        rd->m_DevMan.BindConstantBuffer(eHWSC_Pixel, cb.m_constantBuffer, cb.m_shaderSlot);
                        break;
                    case eHWSC_Geometry:
                        rd->m_DevMan.BindConstantBuffer(eHWSC_Geometry, cb.m_constantBuffer, cb.m_shaderSlot);
                        break;
                    case eHWSC_Domain:
                        rd->m_DevMan.BindConstantBuffer(eHWSC_Domain, cb.m_constantBuffer, cb.m_shaderSlot);
                        break;
                    case eHWSC_Hull:
                        rd->m_DevMan.BindConstantBuffer(eHWSC_Hull, cb.m_constantBuffer, cb.m_shaderSlot);
                        break;
                    default:
                        CRY_ASSERT(false);
                    }
                }
            }
        }
    }
}

void CDeviceGraphicsCommandList_DX11::SetResources_All(CDeviceResourceSet_DX11* pResources)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
    {
        if (m_ValidShaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
        {
            // Bind SRVs
            for (int slot = 0; slot < pResources->compiledSRVs[shaderClass].size(); ++slot)
            {
                ID3D11ShaderResourceView* pSrv = pResources->compiledSRVs[shaderClass][slot];

                if (pSrv != CDeviceResourceSet_DX11::InvalidPointer)
                {
                    if (m_CurrentSRV[shaderClass][slot].Set(pSrv))
                    {
                        switch (shaderClass)
                        {
                        case eHWSC_Vertex:
                            rd->m_DevMan.BindSRV(eHWSC_Vertex, pSrv, slot);
                            break;
                        case eHWSC_Pixel:
                            rd->m_DevMan.BindSRV(eHWSC_Pixel, pSrv, slot);
                            break;
                        case eHWSC_Geometry:
                            rd->m_DevMan.BindSRV(eHWSC_Geometry, pSrv, slot);
                            break;
                        case eHWSC_Domain:
                            rd->m_DevMan.BindSRV(eHWSC_Domain, pSrv, slot);
                            break;
                        case eHWSC_Hull:
                            rd->m_DevMan.BindSRV(eHWSC_Hull, pSrv, slot);
                            break;
                        default:
                            CRY_ASSERT(false);
                        }
                    }
                }
            }

            // Bind Samplers
            for (int slot = 0; slot < pResources->compiledSamplers[shaderClass].size(); ++slot)
            {
                ID3D11SamplerState* pSamplerState = pResources->compiledSamplers[shaderClass][slot];

                if (pSamplerState != CDeviceResourceSet_DX11::InvalidPointer)
                {
                    if (m_CurrentSamplerState[shaderClass][slot].Set(pSamplerState))
                    {
                        switch (shaderClass)
                        {
                        case eHWSC_Vertex:
                            rd->m_DevMan.BindSampler(eHWSC_Vertex, pSamplerState, slot);
                            break;
                        case eHWSC_Pixel:
                            rd->m_DevMan.BindSampler(eHWSC_Pixel, pSamplerState, slot);
                            break;
                        case eHWSC_Geometry:
                            rd->m_DevMan.BindSampler(eHWSC_Geometry, pSamplerState, slot);
                            break;
                        case eHWSC_Domain:
                            rd->m_DevMan.BindSampler(eHWSC_Domain, pSamplerState, slot);
                            break;
                        case eHWSC_Hull:
                            rd->m_DevMan.BindSampler(eHWSC_Hull, pSamplerState, slot);
                            break;
                        default:
                            CRY_ASSERT(false);
                        }
                    }
                }
            }

            // Bind constant buffers
            for (int i = 0; i < pResources->numCompiledCBs[shaderClass]; ++i)
            {
                const CDeviceResourceSet_DX11::SCompiledConstantBuffer& cb = pResources->compiledCBs[shaderClass][i];
                if (m_CurrentCB[shaderClass][cb.m_shaderSlot].Set(cb.code))
                {
                    switch (shaderClass)
                    {
                    case eHWSC_Vertex:
                        rd->m_DevMan.BindConstantBuffer(eHWSC_Vertex, cb.m_constantBuffer, cb.m_shaderSlot);
                        break;
                    case eHWSC_Pixel:
                        rd->m_DevMan.BindConstantBuffer(eHWSC_Pixel, cb.m_constantBuffer, cb.m_shaderSlot);
                        break;
                    case eHWSC_Geometry:
                        rd->m_DevMan.BindConstantBuffer(eHWSC_Geometry, cb.m_constantBuffer, cb.m_shaderSlot);
                        break;
                    case eHWSC_Domain:
                        rd->m_DevMan.BindConstantBuffer(eHWSC_Domain, cb.m_constantBuffer, cb.m_shaderSlot);
                        break;
                    case eHWSC_Hull:
                        rd->m_DevMan.BindConstantBuffer(eHWSC_Hull, cb.m_constantBuffer, cb.m_shaderSlot);
                        break;
                    default:
                        CRY_ASSERT(false);
                    }
                }
            }
        }
    }
}

void CDeviceGraphicsCommandList::SetInlineConstantBuffer(uint32 bindSlot, AzRHI::ConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EShaderStage shaderStages)
{
    for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
    {
        if (shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
        {
            SetInlineConstantBuffer(bindSlot, pBuffer, shaderSlot, shaderClass);
        }
    }
}

void CDeviceGraphicsCommandList::SetInlineConstantBuffer([[maybe_unused]] uint32 bindSlot, AzRHI::ConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass)
{
    auto pCmdList = GET_DX11_COMMANDLIST(this);

    if (pCmdList->m_CurrentCB[shaderClass][shaderSlot].Set(pBuffer->GetCode()))
    {
        switch (shaderClass)
        {
        case eHWSC_Vertex:
            gcpRendD3D->m_DevMan.BindConstantBuffer(eHWSC_Vertex, pBuffer, shaderSlot);
            break;
        case eHWSC_Pixel:
            gcpRendD3D->m_DevMan.BindConstantBuffer(eHWSC_Pixel, pBuffer, shaderSlot);
            break;
        case eHWSC_Geometry:
            gcpRendD3D->m_DevMan.BindConstantBuffer(eHWSC_Geometry, pBuffer, shaderSlot);
            break;
        case eHWSC_Domain:
            gcpRendD3D->m_DevMan.BindConstantBuffer(eHWSC_Domain, pBuffer, shaderSlot);
            break;
        case eHWSC_Hull:
            gcpRendD3D->m_DevMan.BindConstantBuffer(eHWSC_Hull, pBuffer, shaderSlot);
            break;
        }
    }
}

void CDeviceGraphicsCommandList::SetVertexBuffers(uint32 bufferCount, D3DBuffer** pBuffers, size_t* offsets, uint32* strides)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    auto pCmdList = GET_DX11_COMMANDLIST(this);

    for (uint32 slot = 0; slot < bufferCount; ++slot)
    {
        if (pCmdList->m_CurrentVertexStream[slot].Set(SStreamInfo(pBuffers[slot], offsets[slot], strides[slot])))
        {
            rd->m_DevMan.BindVB(pBuffers[slot], slot, offsets[slot], strides[slot]);
        }
    }
}

void CDeviceGraphicsCommandList::SetVertexBuffers(uint32 streamCount, const SStreamInfo* streams)
{
    auto pCmdList = GET_DX11_COMMANDLIST(this);

    for (uint32 slot = 0; slot < streamCount; ++slot)
    {
        if (streams[slot].pStream && pCmdList->m_CurrentVertexStream[slot].Set(streams[slot]))
        {
            D3DBuffer* pBuffer = const_cast<D3DBuffer*>(reinterpret_cast<const D3DBuffer*>(streams[slot].pStream));
            gcpRendD3D->m_DevMan.BindVB(pBuffer, slot, streams[slot].nOffset, streams[slot].nStride);
        }
    }
}

void CDeviceGraphicsCommandList::SetIndexBuffer(const SStreamInfo& indexStream)
{
    auto pCmdList = GET_DX11_COMMANDLIST(this);

    if (pCmdList->m_CurrentIndexStream.Set(indexStream))
    {
        D3DBuffer* pIB = const_cast<D3DBuffer*>(reinterpret_cast<const D3DBuffer*>(indexStream.pStream));

#if !defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
        gcpRendD3D->m_DevMan.BindIB(pIB, indexStream.nOffset, DXGI_FORMAT_R16_UINT);
#else
        gcpRendD3D->m_DevMan.BindIB(pIB, indexStream.nOffset, (DXGI_FORMAT)indexStream.nStride);
#endif
    }
}

void CDeviceGraphicsCommandList::SetInlineConstants([[maybe_unused]] uint32 bindSlot, [[maybe_unused]] uint32 constantCount, [[maybe_unused]] float* pConstants)
{
}

void CDeviceGraphicsCommandList::SetStencilRefImpl(uint8_t stencilRefValue)
{
    auto pCmdList = GET_DX11_COMMANDLIST(this);

    ID3D11DepthStencilState* pDS = NULL;

    if (m_pCurrentPipelineState)
    {
        pDS = reinterpret_cast<CDeviceGraphicsPSO_DX11*>(m_pCurrentPipelineState)->pDepthStencilState;
    }

    gcpRendD3D->m_DevMan.SetDepthStencilState(pDS, stencilRefValue);
}

void CDeviceGraphicsCommandList::DrawImpl(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    if (InstanceCount > 1)
    {
        rd->m_DevMan.DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }
    else
    {
        rd->m_DevMan.Draw(VertexCountPerInstance, StartVertexLocation);
    }
}

void CDeviceGraphicsCommandList::DrawIndexedImpl(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, int BaseVertexLocation, uint32 StartInstanceLocation)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

#if !defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
    STATIC_CHECK(false, "NOT IMPLEMENTED");
#endif

    if (InstanceCount > 1)
    {
        rd->m_DevMan.DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }
    else
    {
        rd->m_DevMan.DrawIndexed(IndexCountPerInstance, StartIndexLocation, BaseVertexLocation);
    }
}

void CDeviceGraphicsCommandList::ClearSurface(D3DSurface* pView, const FLOAT Color[4], UINT NumRects, const D3D11_RECT* pRect)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    rd->FX_ClearTarget(pView, ColorF(Color[0], Color[1], Color[2], Color[3]), NumRects, pRect);
}

void CDeviceGraphicsCommandList::LockToThread()
{
    // ...
}

void CDeviceGraphicsCommandList::Build()
{
    // pContext->FinishCommandList(false, ID3D11CommandList)
}

void CDeviceGraphicsCommandList::ResetImpl()
{
    auto pCmdList = GET_DX11_COMMANDLIST(this);

    pCmdList->m_CurrentDS = nullptr;
    pCmdList->m_CurrentInputLayout = nullptr;
    pCmdList->m_CurrentRS = nullptr;
    pCmdList->m_CurrentTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    pCmdList->m_CurrentBS = nullptr;
    pCmdList->m_ValidShaderStages = EShaderStage_All;

    for (int i = 0; i < CRY_ARRAY_COUNT(pCmdList->m_CurrentVertexStream); ++i)
    {
        pCmdList->m_CurrentVertexStream[i] = SStreamInfo(nullptr, 0, 0);
    }

    pCmdList->m_CurrentIndexStream = SStreamInfo(nullptr, 0, 0);

    memset(pCmdList->m_CurrentShader,       0x0, sizeof(pCmdList->m_CurrentShader));
    memset(pCmdList->m_CurrentSRV,          0x0, sizeof(pCmdList->m_CurrentSRV));
    memset(pCmdList->m_CurrentSamplerState, 0x0, sizeof(pCmdList->m_CurrentSamplerState));
    memset(pCmdList->m_CurrentCB,           0x0, sizeof(pCmdList->m_CurrentCB));

    pCmdList->m_NumSRVs.fill(0);
    pCmdList->m_NumSamplers.fill(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeviceComputeCommandList::SetPipelineStateImpl(CDeviceComputePSOPtr devicePSO)
{
}

void CDeviceComputeCommandList::DispatchImpl([[maybe_unused]] uint32 X, [[maybe_unused]] uint32 Y, [[maybe_unused]] uint32 Z)
{
}

void CDeviceComputeCommandList::LockToThread()
{
    // ...
}

void CDeviceComputeCommandList::Build()
{
    // pContext->FinishCommandList(false, ID3D11CommandList)
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeviceCopyCommandList::LockToThread()
{
    // ...
}

void CDeviceCopyCommandList::Build()
{
    // pContext->FinishCommandList(false, ID3D11CommandList)
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDeviceObjectFactory::CDeviceObjectFactory()
{
    m_pCoreCommandList = AZStd::make_shared<CDeviceGraphicsCommandList_DX11>();
}

CDeviceGraphicsPSOUPtr CDeviceObjectFactory::CreateGraphicsPSOImpl(const CDeviceGraphicsPSODesc& psoDesc) const
{
    auto pResult = CryMakeUnique<CDeviceGraphicsPSO_DX11>();
    if (pResult->Init(psoDesc))
    {
        return AZStd::move(pResult);
    }

    return nullptr;
}

CDeviceComputePSOPtr CDeviceObjectFactory::CreateComputePSO(CDeviceResourceLayoutPtr pResourceLayout) const
{
    return AZStd::make_shared<CDeviceComputePSO_DX11>();
}

CDeviceResourceSetPtr CDeviceObjectFactory::CreateResourceSet(CDeviceResourceSet::EFlags flags) const
{
    return AZStd::make_shared<CDeviceResourceSet_DX11>(flags);
}

CDeviceResourceLayoutPtr CDeviceObjectFactory::CreateResourceLayout() const
{
    return AZStd::make_shared<CDeviceResourceLayout_DX11>();
}

CDeviceGraphicsCommandListPtr CDeviceObjectFactory::GetCoreGraphicsCommandList() const
{
    return m_pCoreCommandList;
}

// Acquire one or more command-lists which are independent of the core command-list
// Only one thread is allowed to call functions on this command-list (DX12 restriction).
// The thread that gets the permition is the one calling Begin() on it AFAIS
CDeviceGraphicsCommandListUPtr CDeviceObjectFactory::AcquireGraphicsCommandList()
{
    // TODO: implement deferred contexts
    __debugbreak();

    return CryMakeUnique<CDeviceGraphicsCommandList>();
}

std::vector<CDeviceGraphicsCommandListUPtr> CDeviceObjectFactory::AcquireGraphicsCommandLists(uint32 listCount)
{
    // TODO: implement deferred contexts
    __debugbreak();

    return (std::vector<CDeviceGraphicsCommandListUPtr>(size_t(listCount)));
}

void CDeviceObjectFactory::ForfeitGraphicsCommandList(CDeviceGraphicsCommandListUPtr pCommandList)
{
    // TODO: implement deferred contexts
    __debugbreak();

    // pContext->ExecuteCommandList(ID3D11CommandList)
}

void CDeviceObjectFactory::ForfeitGraphicsCommandLists(std::vector<CDeviceGraphicsCommandListUPtr> pCommandLists)
{
    // TODO: implement deferred contexts
    __debugbreak();

    // pContext->ExecuteCommandList(ID3D11CommandList)
}

#undef DEVICEWRAPPER12_D3D11_CPP_WRAP_DX11
#endif
#endif
