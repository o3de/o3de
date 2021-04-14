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
#include "CryCustomTypes.h"
#include "CryUtils.h"
#include "../DeviceManager/DeviceWrapper12.h"
#include "DriverD3D.h"
#include <AzCore/std/smart_ptr/make_shared.h>

#if defined(CRY_USE_DX12)
#define CRY_USE_DX12_NATIVE
#endif

#if defined(CRY_USE_DX12_NATIVE)

using namespace DX12;
D3D12_SHADER_BYTECODE g_EmptyShader;

static DX12::Device* GetDevice()
{
    return reinterpret_cast<CCryDX12Device&>(gcpRendD3D->GetDevice()).GetDX12Device();
}

D3D12_SHADER_RESOURCE_VIEW_DESC GetNullSrvDesc(const CTexture* pTexture)
{
    D3D12_SRV_DIMENSION viewDimension[eTT_MaxTexType] =
    {
        D3D12_SRV_DIMENSION_TEXTURE1D,                      // eTT_1D
        D3D12_SRV_DIMENSION_TEXTURE2D,                      // eTT_2D
        D3D12_SRV_DIMENSION_TEXTURE3D,                      // eTT_3D
        D3D12_SRV_DIMENSION_TEXTURECUBE,                    // eTT_Cube
        D3D12_SRV_DIMENSION_TEXTURECUBEARRAY,               // eTT_CubeArray
        D3D12_SRV_DIMENSION_TEXTURE2D,                      // eTT_Dyn2D
        D3D12_SRV_DIMENSION_TEXTURE2D,                      // eTT_User
        D3D12_SRV_DIMENSION_TEXTURECUBE,                    // eTT_NearestCube
        D3D12_SRV_DIMENSION_TEXTURE2DARRAY,                 // eTT_2DArray
        D3D12_SRV_DIMENSION_TEXTURE2DMS,                    // eTT_2DMS
        D3D12_SRV_DIMENSION_TEXTURE2D,                      // eTT_Auto2D
    };

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroStruct(srvDesc);

    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = pTexture ? viewDimension[pTexture->GetTexType()] : D3D12_SRV_DIMENSION_TEXTURE2D;

    return srvDesc;
}


D3D12_SHADER_RESOURCE_VIEW_DESC GetNullSrvDesc([[maybe_unused]] const WrappedDX11Buffer& buffer)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroStruct(srvDesc);
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R32_UINT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

    return srvDesc;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceResourceSet_DX12
    : public CDeviceResourceSet
{
public:
    friend CDeviceGraphicsCommandList;
    friend CDeviceComputeCommandList;

    CDeviceResourceSet_DX12(Device* pDevice, CDeviceResourceSet::EFlags flags)
        : CDeviceResourceSet(flags)
        , m_pDevice(pDevice)
        , m_DescriptorBlockHandle(NULL)
    {}

    virtual void Prepare() final;
    virtual void Build() final;

    const DescriptorBlock& GetDescriptorBlock() const { return m_DescriptorBlock; }

protected:
    DX12::SmartPtr<Device> m_pDevice;
    SDescriptorBlock* m_DescriptorBlockHandle;
    DescriptorBlock  m_DescriptorBlock;
};

// requires command-list
void CDeviceResourceSet_DX12::Prepare()
{
    // Trigger initial buffer uploads asynchronously, before the resource is used
    // TODO: should go through a CopyCommandList at the beginning of a frame

    auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(&gcpRendD3D->GetDeviceContext());
    auto pCommandList = pContext->GetCoreGraphicsCommandList();

    for (const auto& it : m_ConstantBuffers)
    {
        if (AzRHI::ConstantBuffer* pBuffer = it.second.resource)
        {
            if (auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pBuffer->GetPlatformBuffer()))
            {
                if (pResource->GetDX12Resource().InitHasBeenDeferred())
                {
                    pResource->GetDX12Resource().TryStagingUpload(pCommandList);
                }
            }
        }
    }

    for (const auto& it : m_Textures)
    {
        if (CTexture* pTexture = std::get<1>(it.second.resource))
        {
            if (auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pTexture->GetDevTexture() ? pTexture->GetDevTexture()->GetBaseTexture() : nullptr))
            {
                if (pResource->GetDX12Resource().InitHasBeenDeferred())
                {
                    pResource->GetDX12Resource().TryStagingUpload(pCommandList);
                }
            }
        }
    }

    for (const auto& it : m_Buffers)
    {
        if (auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(it.second.resource.m_pBuffer))
        {
            if (pResource->GetDX12Resource().InitHasBeenDeferred())
            {
                pResource->GetDX12Resource().TryStagingUpload(pCommandList);
            }
        }
    }
}

// requires no command-list, just a device
void CDeviceResourceSet_DX12::Build()
{
    // NOTE: will deadlock multi-threaded command-lists when uploads occur on the core command-list (which has a fence-value larger than the active command-list)
    // TODO: call from somewhere safe
    Prepare();

    if (m_DescriptorBlockHandle != NULL)
    {
        gcpRendD3D->m_DevBufMan.ReleaseDescriptorBlock(m_DescriptorBlockHandle);
    }

    // CBV_SRV_UAV heap, SMP heap not yet supported
    uint32 numberResources = m_ConstantBuffers.size() + m_Textures.size() + m_Buffers.size();
    uint32 blockSize = std::max(numberResources, 1u);

    m_DescriptorBlockHandle = gcpRendD3D->m_DevBufMan.CreateDescriptorBlock(blockSize);
    m_DescriptorBlock = DescriptorBlock(*m_DescriptorBlockHandle);

    for (const auto& it : m_ConstantBuffers)
    {
        auto cbData = it.second;
        if (!cbData.resource || !cbData.resource->GetPlatformBuffer())
        {
            m_pDevice->GetD3D12Device()->CreateConstantBufferView(nullptr, m_DescriptorBlock.GetHandleOffsetCPU(0));
            m_DescriptorBlock.IncrementCursor();
            continue;
        }

        AzRHI::ConstantBuffer& constantBuffer = *cbData.resource.get();
        AZ::u32 start = constantBuffer.GetByteOffset();
        AZ::u32 length = constantBuffer.GetByteCount();
        DX12::ResourceView& bufferView = reinterpret_cast<CCryDX12Buffer*>(constantBuffer.GetPlatformBuffer())->GetDX12View();
        assert(bufferView.GetType() == EVT_ConstantBufferView);

        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;

            cbvDesc = bufferView.GetCBVDesc();
            cbvDesc.BufferLocation += start;
            cbvDesc.SizeInBytes = length > 0 ? length : cbvDesc.SizeInBytes - start;
            cbvDesc.SizeInBytes = min(cbvDesc.SizeInBytes, D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * DX12::CONSTANT_BUFFER_ELEMENT_SIZE);

            m_pDevice->GetD3D12Device()->CreateConstantBufferView(&cbvDesc, m_DescriptorBlock.GetHandleOffsetCPU(0));
        }

        m_DescriptorBlock.IncrementCursor();
    }

    for (const auto& it : m_Textures)
    {
        SResourceView::KeyType srvKey = std::get<0>(it.second.resource);
        CTexture* pTexture            = std::get<1>(it.second.resource);

        if (!pTexture || !pTexture->GetDevTexture())
        {
            auto srvDesc = GetNullSrvDesc(pTexture);
            m_pDevice->GetD3D12Device()->CreateShaderResourceView(nullptr, &srvDesc, m_DescriptorBlock.GetHandleOffsetCPU(0));
        }
        else
        {
            DX12::ResourceView& srv = reinterpret_cast<CCryDX12ShaderResourceView*>(pTexture->GetShaderResourceView(srvKey))->GetDX12View();
            m_pDevice->GetD3D12Device()->CopyDescriptorsSimple(1, m_DescriptorBlock.GetHandleOffsetCPU(0), srv.GetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        m_DescriptorBlock.IncrementCursor();
    }

    for (const auto& it : m_Buffers)
    {
        if (!it.second.resource.GetShaderResourceView())
        {
            auto srvDesc = GetNullSrvDesc(it.second.resource);
            m_pDevice->GetD3D12Device()->CreateShaderResourceView(nullptr, &srvDesc, m_DescriptorBlock.GetHandleOffsetCPU(0));
        }
        else
        {
            DX12::ResourceView& srv = reinterpret_cast<CCryDX12ShaderResourceView*>(it.second.resource.GetShaderResourceView())->GetDX12View();
            m_pDevice->GetD3D12Device()->CopyDescriptorsSimple(1, m_DescriptorBlock.GetHandleOffsetCPU(0), srv.GetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        m_DescriptorBlock.IncrementCursor();
    }

    assert(m_DescriptorBlock.GetCursor() == m_DescriptorBlock.GetCapacity());

    // set descriptor block cursor to block start again
    m_DescriptorBlock.Reset();

    m_bDirty = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceResourceLayout_DX12
    : public CDeviceResourceLayout
{
public:
    CDeviceResourceLayout_DX12(Device* pDevice)
        : m_pDevice(pDevice)
    {}

    virtual bool Build() final;

    RootSignature* GetRootSignature() const { return m_pRootSignature.get(); }

protected:
    DX12::SmartPtr<Device> m_pDevice;
    DX12::SmartPtr<RootSignature> m_pRootSignature;
};

bool CDeviceResourceLayout_DX12::Build()
{
    if (!IsValid())
    {
        return false;
    }

    auto getShaderVisibility = [&](::EShaderStage shaderStages)
        {
            D3D12_SHADER_VISIBILITY shaderVisibility[eHWSC_Num + 1] =
            {
                D3D12_SHADER_VISIBILITY_VERTEX, // eHWSC_Vertex
                D3D12_SHADER_VISIBILITY_PIXEL,  // eHWSC_Pixel
                D3D12_SHADER_VISIBILITY_GEOMETRY, // eHWSC_Geometry
                D3D12_SHADER_VISIBILITY_ALL,    // eHWSC_Compute
                D3D12_SHADER_VISIBILITY_DOMAIN, // eHWSC_Domain
                D3D12_SHADER_VISIBILITY_HULL,   // eHWSC_Hull
                D3D12_SHADER_VISIBILITY_ALL,    // eHWSC_Num
            };

            EHWShaderClass shaderClass = eHWSC_Num;

            if ((int(shaderStages) & (int(shaderStages) - 1)) == 0) // only bound to a single shader stage?
            {
                for (shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
                {
                    if (shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
                    {
                        break;
                    }
                }
            }

            return shaderVisibility[shaderClass];
        };

    PipelineLayout pipelineLayout;

    // inline constants
    if (m_InlineConstantCount > 0)
    {
        AZ_Assert(0, "Inline constant is not supported yet");
        pipelineLayout.m_RootParameters[0].InitAsConstants(m_InlineConstantCount, InlineConstantsShaderSlot);
        pipelineLayout.m_NumRootParameters++;
    }

    // inline constant buffers
    for (auto it : m_ConstantBuffers)
    {
        const int bindSlot = it.first;
        const SConstantBufferShaderBinding& cb = it.second;

        pipelineLayout.m_RootParameters[bindSlot].InitAsConstantBufferView(cb.shaderSlot, 0, getShaderVisibility(cb.shaderStages));
        pipelineLayout.m_NumRootParameters++;
    }

    // descriptor table resource sets
    for (auto it : m_ResourceSets)
    {
        const int bindSlot = it.first;
        CDeviceResourceSet* pResourceSet = it.second.get();
        auto shaderVisibility = getShaderVisibility(pResourceSet->GetShaderStages());

        uint32 startDesc = pipelineLayout.m_DescRangeCursor;
        uint32 tableOffset = 0;

        for (auto it2 : pResourceSet->m_ConstantBuffers)
        {
            const int shaderSlot = it2.first;

            CD3DX12_DESCRIPTOR_RANGE cbDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, shaderSlot, 0, tableOffset);
            pipelineLayout.m_DescRanges[pipelineLayout.m_DescRangeCursor] = cbDescRange;

            ++pipelineLayout.m_DescRangeCursor;
            ++tableOffset;
        }

        for (auto it2 : pResourceSet->m_Textures)
        {
            const int shaderSlot = it2.first;

            CD3DX12_DESCRIPTOR_RANGE texDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, shaderSlot, 0, tableOffset);
            pipelineLayout.m_DescRanges[pipelineLayout.m_DescRangeCursor] = texDescRange;

            ++pipelineLayout.m_DescRangeCursor;
            ++tableOffset;
        }

        for (auto it2 : pResourceSet->m_Buffers)
        {
            const int shaderSlot = it2.first;

            CD3DX12_DESCRIPTOR_RANGE bufferDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, shaderSlot, 0, tableOffset);
            pipelineLayout.m_DescRanges[pipelineLayout.m_DescRangeCursor] = bufferDescRange;

            ++pipelineLayout.m_DescRangeCursor;
            ++tableOffset;
        }

        if ((pipelineLayout.m_DescRangeCursor - startDesc) > 0)
        {
            pipelineLayout.m_RootParameters[bindSlot].InitAsDescriptorTable(
                pipelineLayout.m_DescRangeCursor - startDesc, &pipelineLayout.m_DescRanges[startDesc], shaderVisibility);

            pipelineLayout.m_NumRootParameters++;
        }

        for (auto itSampler : pResourceSet->m_Samplers)
        {
            int shaderSlot = itSampler.first;
            const CDeviceResourceSet::SSamplerData& samplerData = itSampler.second;

            auto pDeviceState = reinterpret_cast<CCryDX12SamplerState*>(CTexture::s_TexStates[samplerData.resource].m_pDeviceState);
            auto samplerDesc = pDeviceState->GetDX12SamplerState().GetSamplerDesc();

            // copy parameters from sampler desc first
            memcpy(&pipelineLayout.m_StaticSamplers[pipelineLayout.m_NumStaticSamplers], &samplerDesc, sizeof(samplerDesc));

            // fill in remaining parameters
            auto& staticSamplerDesc = pipelineLayout.m_StaticSamplers[pipelineLayout.m_NumStaticSamplers];
            staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
            staticSamplerDesc.MinLOD = samplerDesc.MinLOD;
            staticSamplerDesc.MaxLOD = samplerDesc.MaxLOD;
            staticSamplerDesc.ShaderRegister = shaderSlot;
            staticSamplerDesc.RegisterSpace = 0;
            staticSamplerDesc.ShaderVisibility = shaderVisibility;

            ++pipelineLayout.m_NumStaticSamplers;
        }
    }

    m_pRootSignature = new RootSignature(m_pDevice);
    return m_pRootSignature->Init(pipelineLayout, DX12::CommandModeGraphics);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceGraphicsPSO_DX12
    : public CDeviceGraphicsPSO
{
public:

    CDeviceGraphicsPSO_DX12(Device* pDevice)
        : graphicsPSO(pDevice)
    {}

    bool Init(const CDeviceGraphicsPSODesc& desc);

    const DX12::GraphicsPipelineState* GetGraphicsPSO() const { return &graphicsPSO; }
    D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }

protected:
    DX12::GraphicsPipelineState graphicsPSO;

    SOnDemandD3DVertexDeclaration m_InputLayout;
    D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
};

bool CDeviceGraphicsPSO_DX12::Init(const CDeviceGraphicsPSODesc& psoDesc)
{
    if (psoDesc.m_pResourceLayout == NULL)
    {
        return false;
    }

    std::array<SDeviceObjectHelpers::SShaderInstanceInfo, eHWSC_Num> hwShaders;
    bool shadersAvailable = SDeviceObjectHelpers::GetShaderInstanceInfo(hwShaders, psoDesc.m_pShader, psoDesc.m_technique, psoDesc.m_ShaderFlags_RT, psoDesc.m_ShaderFlags_MD, psoDesc.m_ShaderFlags_MDV, nullptr, psoDesc.m_bAllowTesselation);
    if (!shadersAvailable)
        return false;

    // validate shaders first
    for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
    {
        if (hwShaders[shaderClass].pHwShader && hwShaders[shaderClass].pHwShaderInstance == NULL)
        {
            return false;
        }

        // TODO: remove
        m_pHwShaders[shaderClass]         = hwShaders[shaderClass].pHwShader;
        m_pHwShaderInstances[shaderClass] = hwShaders[shaderClass].pHwShaderInstance;
    }

    D3D11_RASTERIZER_DESC rasterizerDesc;
    D3D11_BLEND_DESC blendDesc;
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    psoDesc.FillDescs(rasterizerDesc, blendDesc, depthStencilDesc);

    // prepare pso init params
    GraphicsPipelineState::InitParams psoInitParams;
    ZeroStruct(psoInitParams);

    // root signature
    psoInitParams.rootSignature = reinterpret_cast<CDeviceResourceLayout_DX12*>(psoDesc.m_pResourceLayout)->GetRootSignature();

    // blend state
    {
        psoInitParams.desc.BlendState.AlphaToCoverageEnable = blendDesc.AlphaToCoverageEnable;
        psoInitParams.desc.BlendState.IndependentBlendEnable = blendDesc.IndependentBlendEnable;

        for (int i = 0; i < CRY_ARRAY_COUNT(blendDesc.RenderTarget); ++i)
        {
            psoInitParams.desc.BlendState.RenderTarget[i].BlendEnable = blendDesc.RenderTarget[i].BlendEnable;
            psoInitParams.desc.BlendState.RenderTarget[i].LogicOpEnable = false;
            psoInitParams.desc.BlendState.RenderTarget[i].SrcBlend = (D3D12_BLEND)blendDesc.RenderTarget[i].SrcBlend;
            psoInitParams.desc.BlendState.RenderTarget[i].DestBlend = (D3D12_BLEND)blendDesc.RenderTarget[i].DestBlend;
            psoInitParams.desc.BlendState.RenderTarget[i].BlendOp = (D3D12_BLEND_OP)blendDesc.RenderTarget[i].BlendOp;
            psoInitParams.desc.BlendState.RenderTarget[i].SrcBlendAlpha = (D3D12_BLEND)blendDesc.RenderTarget[i].SrcBlendAlpha;
            psoInitParams.desc.BlendState.RenderTarget[i].DestBlendAlpha = (D3D12_BLEND)blendDesc.RenderTarget[i].DestBlendAlpha;
            psoInitParams.desc.BlendState.RenderTarget[i].BlendOpAlpha = (D3D12_BLEND_OP)blendDesc.RenderTarget[i].BlendOpAlpha;
            psoInitParams.desc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_CLEAR;
            psoInitParams.desc.BlendState.RenderTarget[i].RenderTargetWriteMask = blendDesc.RenderTarget[i].RenderTargetWriteMask;
        }
    }

    // depth stencil and rasterizer state
    memcpy(&psoInitParams.desc.DepthStencilState, &depthStencilDesc, sizeof(depthStencilDesc));
    memcpy(&psoInitParams.desc.RasterizerState, &rasterizerDesc, sizeof(rasterizerDesc));

    auto extractShaderBytecode = [&](EHWShaderClass shaderClass)
        {
            return hwShaders[shaderClass].pHwShader
                   ? reinterpret_cast<CCryDX12Shader*>(hwShaders[shaderClass].pDeviceShader)->GetD3D12ShaderBytecode()
                   : g_EmptyShader;
        };

    psoInitParams.desc.VS = extractShaderBytecode(eHWSC_Vertex);
    psoInitParams.desc.DS = extractShaderBytecode(eHWSC_Domain);
    psoInitParams.desc.HS = extractShaderBytecode(eHWSC_Hull);
    psoInitParams.desc.GS = extractShaderBytecode(eHWSC_Geometry);
    psoInitParams.desc.PS = extractShaderBytecode(eHWSC_Pixel);

    psoInitParams.desc.SampleMask = UINT_MAX;
    psoInitParams.desc.SampleDesc.Count = 1;

    // primitive topology
    {
        m_PrimitiveTopology = gcpRendD3D->FX_ConvertPrimitiveType(psoDesc.m_PrimitiveType);

        struct
        {
            eRenderPrimitiveType primitiveType;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology;
        }
        topologyTypes[] =
        {
            { eptUnknown,                D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED },
            { eptTriangleList,           D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE  },
            { eptTriangleStrip,          D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE  },
            { eptLineList,               D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE      },
            { eptLineStrip,              D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE      },
            { eptPointList,              D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT     },
            { ept1ControlPointPatchList, D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     },
            { ept2ControlPointPatchList, D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     },
            { ept3ControlPointPatchList, D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     },
            { ept4ControlPointPatchList, D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     },
        };

        psoInitParams.desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        for (int i = 0; i < CRY_ARRAY_COUNT(topologyTypes); ++i)
        {
            if (topologyTypes[i].primitiveType == psoDesc.m_PrimitiveType)
            {
                psoInitParams.desc.PrimitiveTopologyType = topologyTypes[i].primitiveTopology;
                break;
            }
        }
    }

    // input layout
    m_InputLayout.m_Declaration.resize(0);
    if (CHWShader_D3D::SHWSInstance* pVsInstance = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(hwShaders[eHWSC_Vertex].pHwShaderInstance))
    {
        uint8 streamMask = psoDesc.CombineVertexStreamMasks(uint8(pVsInstance->m_VStreamMask_Decl), psoDesc.m_ObjectStreamMask);

        const bool bMorph = false;
        const bool bInstanced = (streamMask & VSM_INSTANCED) != 0;

        gcpRendD3D->EF_OnDemandVertexDeclaration(m_InputLayout, streamMask >> 1, psoDesc.m_VertexFormat, bMorph, bInstanced);

        psoInitParams.desc.InputLayout.pInputElementDescs = reinterpret_cast<D3D12_INPUT_ELEMENT_DESC*>(&m_InputLayout.m_Declaration[0]);
        psoInitParams.desc.InputLayout.NumElements = m_InputLayout.m_Declaration.size();
    }

    // render targets
    for (int i = 0; i < psoDesc.m_RenderTargetFormats.size(); ++i)
    {
        psoInitParams.desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;

        if (psoDesc.m_RenderTargetFormats[i] != eTF_Unknown)
        {
            psoInitParams.desc.RTVFormats[i] = CTexture::DeviceFormatFromTexFormat(psoDesc.m_RenderTargetFormats[i]);
            psoInitParams.desc.NumRenderTargets = i + 1;
        }
    }

    psoInitParams.desc.DSVFormat = CTexture::DeviceFormatFromTexFormat(psoDesc.m_DepthStencilFormat);

#if defined(ENABLE_PROFILING_CODE)
    m_PrimitiveTypeForProfiling = psoDesc.m_PrimitiveType;
#endif

    bool bSuccess = graphicsPSO.Init(psoInitParams);
    return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceComputePSO_DX12
    : public CDeviceComputePSO
{
public:
    friend CDeviceGraphicsCommandList;

    CDeviceComputePSO_DX12(Device* pDevice, CDeviceResourceLayoutPtr pDeviceResourceLayout)
        : computePSO(pDevice)
        , pResourceLayout(pDeviceResourceLayout)
    {}

    virtual bool Build() final { return false; }

    CDeviceResourceLayoutPtr pResourceLayout;
    DX12::ComputePipelineState computePSO;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDeviceGraphicsCommandList_DX12
    : public CDeviceGraphicsCommandList
{
public:
    CDeviceGraphicsCommandList_DX12(DX12::CommandList* pCommandList)
        : m_pCommandList(pCommandList)
    {
        Reset();
    }

    DX12::SmartPtr<DX12::CommandList> m_pCommandList;
};

#define GET_DX12_GRAPHICS_COMMANDLIST(abstractCommandList) reinterpret_cast<CDeviceGraphicsCommandList_DX12*>(abstractCommandList)

void CDeviceGraphicsCommandList::SetRenderTargets(uint32 targetCount, CTexture** pTargets, SDepthTexture* pDepthTarget)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);

    const DX12::ResourceView* pDSV = nullptr;
    const DX12::ResourceView* pRTV[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nullptr };

    // Get current depth stencil views
    if (pDepthTarget)
    {
        if (auto pView = reinterpret_cast<CCryDX12DepthStencilView*>(pDepthTarget->pSurf))
        {
            pDSV = &pView->GetDX12View();
            pView->BeginResourceStateTransition(pCommandListDX12->m_pCommandList.get());
        }
    }

    // Get current render target views
    for (int i = 0; i < targetCount; ++i)
    {
        if (pTargets[i])
        {
            if (auto pView = reinterpret_cast<CCryDX12RenderTargetView*>(pTargets[i]->GetSurface(0, 0)))
            {
                pRTV[i] = &pView->GetDX12View();
                pView->BeginResourceStateTransition(pCommandListDX12->m_pCommandList.get());
            }
        }
    }

    // TODO: if we know early that the resource(s) will be RENDER_TARGET/DEPTH_READ|WRITE we can begin the barrier early and end it here
    pCommandListDX12->m_pCommandList->BindAndSetOutputViews(targetCount, pRTV, pDSV);
}

void CDeviceGraphicsCommandList::SetViewports(uint32 vpCount, const D3DViewPort* pViewports)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);

    // D3D11_VIEWPORT := D3D12_VIEWPORT
    pCommandListDX12->m_pCommandList->SetViewports(vpCount, (D3D12_VIEWPORT*)pViewports);
}

void CDeviceGraphicsCommandList::SetScissorRects(uint32 rcCount, const D3DRectangle* pRects)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);

    // D3D11_RECT := D3D12_RECT
    pCommandListDX12->m_pCommandList->SetScissorRects(rcCount, (D3D12_RECT*)pRects);
}

void CDeviceGraphicsCommandList::SetPipelineStateImpl(CDeviceGraphicsPSOPtr devicePSO)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);
    CDeviceGraphicsPSO_DX12* pDevicePSO = (CDeviceGraphicsPSO_DX12*)devicePSO;

    pCommandListDX12->m_pCommandList->SetPipelineState(pDevicePSO->GetGraphicsPSO());
    pCommandListDX12->m_pCommandList->SetPrimitiveTopology(pDevicePSO->GetPrimitiveTopology()); // TODO: de-duplicate this call?
}

void CDeviceGraphicsCommandList::SetResourceLayout(CDeviceResourceLayout* pResourceLayout)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);
    CDeviceResourceLayout_DX12* pResourceLayoutDX12 = reinterpret_cast<CDeviceResourceLayout_DX12*>(pResourceLayout);

    pCommandListDX12->m_pCommandList->SetRootSignature(DX12::CommandModeGraphics, pResourceLayoutDX12->GetRootSignature());
}

void CDeviceGraphicsCommandList::SetVertexBuffers(uint32 bufferCount, D3DBuffer** pBuffers, size_t* offsets, uint32* strides)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);

    pCommandListDX12->m_pCommandList->ClearVertexBufferHeap(bufferCount);

    for (uint32 i = 0; i < bufferCount; ++i)
    {
        if (pBuffers[i])
        {
            auto pBuffer = reinterpret_cast<CCryDX12Buffer*>(pBuffers[i]);
            pBuffer->BeginResourceStateTransition(pCommandListDX12->m_pCommandList.get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

            pCommandListDX12->m_pCommandList->BindVertexBufferView(pBuffer->GetDX12View(), i, TRange<uint32>(offsets[i], offsets[i]), strides[i]);
        }
    }

    // TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
    pCommandListDX12->m_pCommandList->SetVertexBufferHeap(bufferCount);
}

void CDeviceGraphicsCommandList::SetVertexBuffers(uint32 streamCount, const SStreamInfo* streams)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);

    pCommandListDX12->m_pCommandList->ClearVertexBufferHeap(streamCount);

    for (uint32 i = 0; i < streamCount; ++i)
    {
        if (auto pBuffer = const_cast<CCryDX12Buffer*>(reinterpret_cast<const CCryDX12Buffer*>(streams[i].pStream)))
        {
            pCommandListDX12->m_pCommandList->BindVertexBufferView(pBuffer->GetDX12View(), i, TRange<uint32>(streams[i].nOffset, streams[i].nOffset), streams[i].nStride);
        }
    }

    // TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
    pCommandListDX12->m_pCommandList->SetVertexBufferHeap(streamCount);
}

void CDeviceGraphicsCommandList::SetIndexBuffer(const SStreamInfo& indexStream)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);

    auto pBuffer = const_cast<CCryDX12Buffer*>(reinterpret_cast<const CCryDX12Buffer*>(indexStream.pStream));
    pBuffer->BeginResourceStateTransition(pCommandListDX12->m_pCommandList.get(), D3D12_RESOURCE_STATE_INDEX_BUFFER);

    // TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
#if !defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
    pCommandListDX12->m_pCommandList->BindAndSetIndexBufferView(pBuffer->GetDX12View(), DXGI_FORMAT_R16_UINT, indexStream.nOffset);
#else
    pCommandListDX12->m_pCommandList->BindAndSetIndexBufferView(pBuffer->GetDX12View(), (DXGI_FORMAT)indexStream.nStride, indexStream.nOffset);
#endif
}

void CDeviceGraphicsCommandList::SetResourcesImpl(uint32 bindSlot, CDeviceResourceSet* pResourceSet)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);
    CDeviceResourceSet_DX12* pResources = reinterpret_cast<CDeviceResourceSet_DX12*>(pResourceSet);

    for (auto& it : pResources->m_ConstantBuffers)
    {
        if (AzRHI::ConstantBuffer* pBuffer = it.second.resource)
        {
            auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pBuffer->GetPlatformBuffer());
            pResource->BeginResourceStateTransition(pCommandListDX12->m_pCommandList.get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

            // TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
            pCommandListDX12->m_pCommandList->TrackResourceCBVUsage(pResource->GetDX12Resource());
        }
    }

    for (auto& it : pResources->m_Textures)
    {
        if (CTexture* pTexture = std::get<1>(it.second.resource))
        {
            auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pTexture->GetDevTexture()->GetBaseTexture());
            pResource->BeginResourceStateTransition(pCommandListDX12->m_pCommandList.get(), D3D12_RESOURCE_STATE_GENERIC_READ);

            // TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
            pCommandListDX12->m_pCommandList->TrackResourceSRVUsage(pResource->GetDX12Resource());
        }
    }

    for (auto& it : pResources->m_Buffers)
    {
        if (ID3D11Buffer* pBuffer = it.second.resource.m_pBuffer)
        {
            auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pBuffer);
            pResource->BeginResourceStateTransition(pCommandListDX12->m_pCommandList.get(), D3D12_RESOURCE_STATE_GENERIC_READ);

            // TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
            pCommandListDX12->m_pCommandList->TrackResourceSRVUsage(pResource->GetDX12Resource());
        }
    }

    const DescriptorBlock& descriptorBlock = pResources->GetDescriptorBlock();
    pCommandListDX12->m_pCommandList->SetDescriptorTable(DX12::CommandModeGraphics, bindSlot, descriptorBlock.GetHandleOffsetGPU(0));
}

void CDeviceGraphicsCommandList::SetInlineConstantBuffer(uint32 bindSlot, AzRHI::ConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, [[maybe_unused]] ::EShaderStage shaderStages)
{
    SetInlineConstantBuffer(bindSlot, pBuffer, shaderSlot, eHWSC_Num);
}

void CDeviceGraphicsCommandList::SetInlineConstantBuffer(uint32 bindSlot, AzRHI::ConstantBuffer* pConstantBuffer, [[maybe_unused]] EConstantBufferShaderSlot shaderSlot, [[maybe_unused]] EHWShaderClass shaderClass)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);

    auto pBuffer = reinterpret_cast<CCryDX12Buffer*>(pConstantBuffer->GetPlatformBuffer());
    pBuffer->BeginResourceStateTransition(pCommandListDX12->m_pCommandList.get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    // TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
    pCommandListDX12->m_pCommandList->TrackResourceCBVUsage(pBuffer->GetDX12Resource());

    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = pBuffer->GetDX12View().GetCBVDesc().BufferLocation + pConstantBuffer->GetByteOffset();
    pCommandListDX12->m_pCommandList->SetConstantBufferView(DX12::CommandModeGraphics, bindSlot, gpuAddress);
}

void CDeviceGraphicsCommandList::SetInlineConstants(uint32 bindSlot, uint32 constantCount, float* pConstants)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);
    pCommandListDX12->m_pCommandList->Set32BitConstants(DX12::CommandModeGraphics, bindSlot, constantCount, pConstants, 0);
}

void CDeviceGraphicsCommandList::SetStencilRefImpl(uint8_t stencilRefValue)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);
    pCommandListDX12->m_pCommandList->SetStencilRef(stencilRefValue);
}

void CDeviceGraphicsCommandList::DrawImpl(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);
    pCommandListDX12->m_pCommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void CDeviceGraphicsCommandList::DrawIndexedImpl(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, int BaseVertexLocation, uint32 StartInstanceLocation)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);
    pCommandListDX12->m_pCommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void CDeviceGraphicsCommandList::ClearSurface(D3DSurface* pView, const FLOAT Color[4], UINT NumRects, const D3D11_RECT* pRect)
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);
    auto pViewDX12 = reinterpret_cast<CCryDX12RenderTargetView*>(pView);
    pCommandListDX12->m_pCommandList->ClearRenderTargetView(pViewDX12->GetDX12View(), Color, NumRects, pRect);
}

void CDeviceGraphicsCommandList::LockToThread()
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);
    pCommandListDX12->m_pCommandList->Begin();
    pCommandListDX12->m_pCommandList->SetResourceAndSamplerStateHeaps();
}

void CDeviceGraphicsCommandList::Build()
{
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);
    pCommandListDX12->m_pCommandList->End();
}

void CDeviceGraphicsCommandList::ResetImpl()
{
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDeviceComputeCommandList_DX12
    : public CDeviceComputeCommandList
{
public:
    DX12::SmartPtr<DX12::CommandList> m_pCommandList;
};

#define GET_DX12_COMPUTE_COMMANDLIST(abstractCommandList) reinterpret_cast<CDeviceComputeCommandList_DX12*>(abstractCommandList)

void CDeviceComputeCommandList::SetResourceLayout(CDeviceResourceLayout* pResourceLayout)
{
    CDeviceComputeCommandList_DX12* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this);
    CDeviceResourceLayout_DX12* pResourceLayoutDX12 = reinterpret_cast<CDeviceResourceLayout_DX12*>(pResourceLayout);

    pCommandListDX12->m_pCommandList->SetRootSignature(DX12::CommandModeCompute, pResourceLayoutDX12->GetRootSignature());
}

void CDeviceComputeCommandList::SetResources(uint32 bindSlot, CDeviceResourceSet* pResources)
{
    CDeviceComputeCommandList_DX12* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this);
    CDeviceResourceSet_DX12* pResourcesDX12 = reinterpret_cast<CDeviceResourceSet_DX12*>(pResources);

    for (auto& it : pResourcesDX12->m_ConstantBuffers)
    {
        auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(it.second.resource->GetPlatformBuffer());
        pResource->BeginResourceStateTransition(pCommandListDX12->m_pCommandList.get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        // TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
        pCommandListDX12->m_pCommandList->TrackResourceCBVUsage(pResource->GetDX12Resource());
    }

    for (auto& it : pResourcesDX12->m_Textures)
    {
        auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(std::get<1>(it.second.resource)->GetDevTexture()->GetBaseTexture());
        pResource->BeginResourceStateTransition(pCommandListDX12->m_pCommandList.get(), D3D12_RESOURCE_STATE_GENERIC_READ);

        // TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
        pCommandListDX12->m_pCommandList->TrackResourceSRVUsage(pResource->GetDX12Resource());
    }

    const DescriptorBlock& descriptorBlock = pResourcesDX12->GetDescriptorBlock();
    pCommandListDX12->m_pCommandList->SetDescriptorTable(DX12::CommandModeCompute, bindSlot, descriptorBlock.GetHandleOffsetGPU(0));
}

void CDeviceComputeCommandList::SetConstantBuffer(uint32 bindSlot, AzRHI::ConstantBuffer* pConstantBuffer, [[maybe_unused]] EConstantBufferShaderSlot shaderSlot, [[maybe_unused]] EHWShaderClass shaderClass)
{
    CDeviceComputeCommandList_DX12* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this);

    auto pBuffer = reinterpret_cast<CCryDX12Buffer*>(pConstantBuffer->GetPlatformBuffer());
    pBuffer->BeginResourceStateTransition(pCommandListDX12->m_pCommandList.get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    // TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
    pCommandListDX12->m_pCommandList->TrackResourceCBVUsage(pBuffer->GetDX12Resource());

    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = pBuffer->GetDX12View().GetCBVDesc().BufferLocation + pConstantBuffer->GetByteOffset();
    pCommandListDX12->m_pCommandList->SetConstantBufferView(DX12::CommandModeCompute, bindSlot, gpuAddress);
}

void CDeviceComputeCommandList::SetInlineConstants(uint32 bindSlot, uint32 constantCount, float* pConstants)
{
    CDeviceComputeCommandList_DX12* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this);
    pCommandListDX12->m_pCommandList->Set32BitConstants(DX12::CommandModeCompute, bindSlot, constantCount, pConstants, 0);
}

void CDeviceComputeCommandList::SetPipelineStateImpl(CDeviceComputePSOPtr devicePSO)
{
    CDeviceComputeCommandList_DX12* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this);
    CDeviceComputePSO_DX12* pDevicePSO = (CDeviceComputePSO_DX12*)devicePSO.get();

    pCommandListDX12->m_pCommandList->SetPipelineState(&pDevicePSO->computePSO);
}

void CDeviceComputeCommandList::DispatchImpl(uint32 X, uint32 Y, uint32 Z)
{
    CDeviceComputeCommandList_DX12* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this);
    pCommandListDX12->m_pCommandList->Dispatch(X, Y, Z);
}

void CDeviceComputeCommandList::LockToThread()
{
    CDeviceComputeCommandList_DX12* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this);
    pCommandListDX12->m_pCommandList->Begin();
    pCommandListDX12->m_pCommandList->SetResourceAndSamplerStateHeaps();
}

void CDeviceComputeCommandList::Build()
{
    CDeviceComputeCommandList_DX12* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this);
    pCommandListDX12->m_pCommandList->End();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDeviceCopyCommandList_DX12
    : public CDeviceCopyCommandList
{
public:
    DX12::SmartPtr<DX12::CommandList> m_pCommandList;
};

#define GET_DX12_COPY_COMMANDLIST(abstractCommandList) reinterpret_cast<CDeviceCopyCommandList_DX12*>(abstractCommandList)

CDeviceCopyCommandList::ECopyType CDeviceCopyCommandList::DetermineCopyType(CDeviceCopyCommandList::ECopyType eCurrent, D3DResource* pResource)
{
    auto pRes = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pResource);
    DX12::Resource& rResource = pRes->GetDX12Resource();

    if (rResource.IsOffCard())
    {
        return eCT_OffCardResources;
    }

    if (rResource.IsTarget())
    {
        return eCT_GraphicsResources;
    }
    if (rResource.IsGeneric()) // could be non-compute shader too
    {
        return eCT_GenericResources;
    }

    if (eCurrent == eCT_OffCardResources)
    {
        return eCT_GraphicsResources;
    }

    return eCurrent;
}

void CDeviceCopyCommandList::LockToThread()
{
    CDeviceCopyCommandList_DX12* pCommandListDX12 = GET_DX12_COPY_COMMANDLIST(this);
    pCommandListDX12->m_pCommandList->Begin();
    pCommandListDX12->m_pCommandList->SetResourceAndSamplerStateHeaps();
}

void CDeviceCopyCommandList::Build()
{
    CDeviceCopyCommandList_DX12* pCommandListDX12 = GET_DX12_COPY_COMMANDLIST(this);
    pCommandListDX12->m_pCommandList->End();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDeviceObjectFactory::CDeviceObjectFactory()
{
    m_pCoreCommandList = AZStd::make_shared<CDeviceGraphicsCommandList_DX12>(nullptr);
}

CDeviceGraphicsPSOUPtr CDeviceObjectFactory::CreateGraphicsPSOImpl(const CDeviceGraphicsPSODesc& psoDesc) const
{
    auto pResult = CryMakeUnique<CDeviceGraphicsPSO_DX12>(GetDevice());
    if (pResult->Init(psoDesc))
    {
        return std::move(pResult);
    }

    return nullptr;
}

CDeviceComputePSOPtr CDeviceObjectFactory::CreateComputePSO(CDeviceResourceLayoutPtr pResourceLayout) const
{
    return AZStd::make_shared<CDeviceComputePSO_DX12>(GetDevice(), pResourceLayout);
}

CDeviceResourceSetPtr CDeviceObjectFactory::CreateResourceSet(CDeviceResourceSet::EFlags flags) const
{
    return AZStd::make_shared<CDeviceResourceSet_DX12>(GetDevice(), flags);
}

CDeviceResourceLayoutPtr CDeviceObjectFactory::CreateResourceLayout() const
{
    return AZStd::make_shared<CDeviceResourceLayout_DX12>(GetDevice());
}

CDeviceGraphicsCommandListPtr CDeviceObjectFactory::GetCoreGraphicsCommandList() const
{
    auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(&gcpRendD3D->GetDeviceContext());
    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(m_pCoreCommandList.get());
    pCommandListDX12->m_pCommandList = pContext->GetCoreGraphicsCommandList();

    return m_pCoreCommandList;
}

// Acquire one or more command-lists which are independent of the core command-list
// Only one thread is allowed to call functions on this command-list (DX12 restriction).
// The thread that gets the permition is the one calling Begin() on it AFAIS
CDeviceGraphicsCommandListUPtr CDeviceObjectFactory::AcquireGraphicsCommandList()
{
    // In theory this whole function need to be atomic, instead it voids the core command-list(s),
    // Synchronization between different threads acquiring command-lists is deferred to the higher level.
    auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(&gcpRendD3D->GetDeviceContext());
    DX12::CommandListPool& pQueue = pContext->GetCoreCommandListPool(CMDQUEUE_GRAPHICS);
    pContext->CeaseAllCommandQueues(false);

    DX12::SmartPtr<CommandList> pCL;
    pQueue.AcquireCommandList(pCL);

    pContext->ResumeAllCommandQueues();
    return CryMakeUnique<CDeviceGraphicsCommandList_DX12>(pCL);
}

std::vector<CDeviceGraphicsCommandListUPtr> CDeviceObjectFactory::AcquireGraphicsCommandLists(uint32 listCount)
{
    // In theory this whole function need to be atomic, instead it voids the core command-list(s),
    // Synchronization between different threads acquiring command-lists is deferred to the higher level.
    auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(&gcpRendD3D->GetDeviceContext());
    DX12::CommandListPool& pQueue = pContext->GetCoreCommandListPool(CMDQUEUE_GRAPHICS);
    pContext->CeaseAllCommandQueues(false);

    std::vector<CDeviceGraphicsCommandListUPtr> pCommandLists;
    DX12::SmartPtr<CommandList> pCLs[256];

    // Allocate in chunks of 256
    for (uint32 n = 0; n < listCount; n += 256U)
    {
        const uint32 chunkCount = std::min(listCount - n, 256U);
        pQueue.AcquireCommandLists(chunkCount, pCLs);

        for (uint32 b = 0; b < chunkCount; ++b)
        {
            pCommandLists.emplace_back(CryMakeUnique<CDeviceGraphicsCommandList_DX12>(pCLs[b]));
        }
    }

    pContext->ResumeAllCommandQueues();
    return pCommandLists;
}

// Command-list sinks, will automatically submit command-lists in allocation-order
void CDeviceObjectFactory::ForfeitGraphicsCommandList(CDeviceGraphicsCommandListUPtr pCommandList)
{
    auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(&gcpRendD3D->GetDeviceContext());
    DX12::CommandListPool& pQueue = pContext->GetCoreCommandListPool(CMDQUEUE_GRAPHICS);

    CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(pCommandList.get());
    DX12::SmartPtr<CommandList> pCL = pCommandListDX12->m_pCommandList;
    pQueue.ForfeitCommandList(pCL);
}

void CDeviceObjectFactory::ForfeitGraphicsCommandLists(std::vector<CDeviceGraphicsCommandListUPtr> pCommandLists)
{
    auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(&gcpRendD3D->GetDeviceContext());
    DX12::CommandListPool& pQueue = pContext->GetCoreCommandListPool(CMDQUEUE_GRAPHICS);

    const uint32 listCount = pCommandLists.size();
    DX12::SmartPtr<CommandList> pCLs[256];

    // Deallocate in chunks of 256
    for (uint32 n = 0; n < listCount; n += 256U)
    {
        const uint32 chunkCount = std::min(listCount - n, 256U);
        for (uint32 b = 0; b < chunkCount; ++b)
        {
            pCLs[b] = GET_DX12_GRAPHICS_COMMANDLIST(pCommandLists[b].get())->m_pCommandList;
        }
        pQueue.ForfeitCommandLists(chunkCount, pCLs);
    }
}

#endif
