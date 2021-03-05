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
#include "DX12RootSignature.hpp"

#define DX12_DESCRIPTORTABLE_MERGERANGES_CBV    1
#define DX12_DESCRIPTORTABLE_MERGERANGES_SRV    2
#define DX12_DESCRIPTORTABLE_MERGERANGES_UAV    4
#define DX12_DESCRIPTORTABLE_MERGERANGES_SMP    8
#define DX12_DESCRIPTORTABLE_MERGERANGES_ALL    (DX12_DESCRIPTORTABLE_MERGERANGES_SRV | DX12_DESCRIPTORTABLE_MERGERANGES_UAV | DX12_DESCRIPTORTABLE_MERGERANGES_SMP)
#define DX12_DESCRIPTORTABLE_MERGERANGES_MODE   DX12_DESCRIPTORTABLE_MERGERANGES_ALL

namespace DX12
{
    PipelineLayout::PipelineLayout()
    {
        Clear();
    }

    void PipelineLayout::Build(Shader* vs, Shader* hs, Shader* ds, Shader* gs, Shader* ps)
    {
        Clear();

        Shader* shaders[] = { ps, vs, hs, ds, gs };
        EShaderStage shaderStages[] = { ESS_Pixel, ESS_Vertex, ESS_Hull, ESS_Domain, ESS_Geometry };

        for (size_t stageIdx = 0; stageIdx < AZ_ARRAY_SIZE(shaders); ++stageIdx)
        {
            if (shaders[stageIdx])
            {
                AZ::u32 parameterCount = m_NumRootParameters;
                AppendRootConstantBuffers(shaders[stageIdx], shaderStages[stageIdx]);
                if (parameterCount != m_NumRootParameters)
                {
                    m_ShaderStageAccess |= BIT(shaderStages[stageIdx]);
                }
            }
        }

        for (size_t stageIdx = 0; stageIdx < AZ_ARRAY_SIZE(shaders); ++stageIdx)
        {
            if (shaders[stageIdx])
            {
                AZ::u32 parameterCount = m_NumRootParameters;
                AppendDescriptorTables(shaders[stageIdx], shaderStages[stageIdx]);
                if (parameterCount != m_NumRootParameters)
                {
                    m_ShaderStageAccess |= BIT(shaderStages[stageIdx]);
                }
            }
        }
    }

    void PipelineLayout::Build(Shader* cs)
    {
        Clear();

        if (cs)
        {
            AppendRootConstantBuffers(cs, ESS_Compute);
            AppendDescriptorTables(cs, ESS_Compute);
            m_ShaderStageAccess |= ESS_Compute;
        }
    }

    void PipelineLayout::Clear()
    {
        m_ConstantViews.clear();
        m_TableResources.clear();
        m_Samplers.clear();

        m_DescRangeCursor = 0;
        m_NumDescriptors = 0;
        m_NumDynamicSamplers = 0;
        m_NumDescriptorTables = 0;
        m_NumRootParameters = 0;
        m_NumStaticSamplers = 0;

        m_ShaderStageAccess = ESS_None;
    }

    void PipelineLayout::MakeDescriptorTable(
        AZ::u32 rangeCursorEnd,
        AZ::u32& rangeCursorBegin,
        AZ::u32 offsetEnd,
        AZ::u32& offsetBegin,
        D3D12_DESCRIPTOR_HEAP_TYPE eHeap,
        D3D12_SHADER_VISIBILITY visibility)
    {
        const AZ::u32 numRanges = rangeCursorEnd - rangeCursorBegin;
        if (numRanges > 0)
        {
            PipelineLayout::DescriptorTableInfo tableInfo;
            tableInfo.m_Type = eHeap;
            tableInfo.m_Offset = offsetBegin;
            m_DescriptorTables[m_NumDescriptorTables++] = tableInfo;

            // Setup the configuration of the root signature entry (a descriptor table range in this case)
            m_RootParameters[m_NumRootParameters++].InitAsDescriptorTable(numRanges, m_DescRanges + rangeCursorBegin, visibility);

            rangeCursorBegin = rangeCursorEnd;
            offsetBegin = offsetEnd;
        }
    }

    static D3D12_SHADER_VISIBILITY ToShaderVisibility(EShaderStage stage)
    {
        static const D3D12_SHADER_VISIBILITY visibility[] =
        {
            D3D12_SHADER_VISIBILITY_VERTEX,
            D3D12_SHADER_VISIBILITY_HULL,
            D3D12_SHADER_VISIBILITY_DOMAIN,
            D3D12_SHADER_VISIBILITY_GEOMETRY,
            D3D12_SHADER_VISIBILITY_PIXEL,
            D3D12_SHADER_VISIBILITY_ALL
        };

        return visibility[stage];
    }

    static bool IsRootConstantBuffer(const ReflectedBindingRange& range)
    {
        return !range.m_bUsed || range.m_Count == 1;
    }

    void PipelineLayout::AppendRootConstantBuffers(Shader* shader, EShaderStage shaderStage)
    {
        const D3D12_SHADER_VISIBILITY visibility = ToShaderVisibility(shaderStage);

        const ReflectedBindings& bindings = shader->GetReflectedBindings();

        if (bindings.m_ConstantBuffers.m_DescriptorCount)
        {
            auto& ranges = bindings.m_ConstantBuffers.m_Ranges;
            for (size_t rangeIdx = 0; rangeIdx < ranges.size(); ++rangeIdx)
            {
                const ReflectedBindingRange range = ranges[rangeIdx];

                AZ::u32 rangeBegin = range.m_ShaderRegister;
                AZ::u32 rangeEnd = range.m_ShaderRegister + range.m_Count;

                if (!IsRootConstantBuffer(range))
                {
                    continue;
                }

                if (range.m_bShared)
                {
                    bool bFound = false;

                    // only pick up the first occurrence
                    for (const ConstantBufferLayoutBinding& binding : m_ConstantViews)
                    {
                        if (binding.ShaderSlot == rangeBegin)
                        {
                            bFound = true;
                            break;
                        }
                    }

                    if (!bFound)
                    {
                        ConstantBufferLayoutBinding constant;
                        constant.ShaderStage = shaderStage;
                        constant.ShaderSlot = rangeBegin;
                        constant.RootParameterIndex = m_NumRootParameters;
                        m_ConstantViews.push_back(constant);

                        m_RootParameters[m_NumRootParameters++].InitAsConstantBufferView(rangeBegin, 0, D3D12_SHADER_VISIBILITY_ALL);
                    }
                }
                else if (!range.m_bUsed)
                {
                    // Setup the configuration of the root signature entry (a constant dummy CBV in this case)
                    for (AZ::u32 rootParameterIdx = rangeBegin; rootParameterIdx < rangeEnd; ++rootParameterIdx)
                    {
                        m_RootParameters[m_NumRootParameters++].InitAsConstantBufferView(rootParameterIdx, 0, visibility);
                    }
                }
                else
                {
                    ConstantBufferLayoutBinding constant;
                    constant.ShaderStage = shaderStage;
                    constant.ShaderSlot = rangeBegin;
                    constant.RootParameterIndex = m_NumRootParameters;
                    m_ConstantViews.push_back(constant);

                    m_RootParameters[m_NumRootParameters++].InitAsConstantBufferView(rangeBegin, 0, visibility);
                }
            }
        }
    }

    void PipelineLayout::AppendDescriptorTables(Shader* pShader, EShaderStage shaderStage)
    {
        const D3D12_SHADER_VISIBILITY visibility = ToShaderVisibility(shaderStage);

        const ReflectedBindings& bindings = pShader->GetReflectedBindings();

        AZ::u32 rangeCursorBegin = m_DescRangeCursor;
        AZ::u32 rangeCursorEnd = m_DescRangeCursor;
        AZ::u32 descriptorBegin = m_NumDescriptors;
        AZ::u32 descriptorEnd = m_NumDescriptors;

        if (bindings.m_ConstantBuffers.m_DescriptorCount)
        {
            auto& ranges = bindings.m_ConstantBuffers.m_Ranges;
            for (size_t rangeIdx = 0; rangeIdx < ranges.size(); ++rangeIdx)
            {
                const ReflectedBindingRange range = ranges[rangeIdx];

                AZ::u32 rangeBegin = range.m_ShaderRegister;
                AZ::u32 rangeEnd = range.m_ShaderRegister + range.m_Count;

                if (IsRootConstantBuffer(range))
                {
                    continue;
                }

                // Setup the configuration of the descriptor table range (a number of CBVs in this case)
                m_DescRanges[rangeCursorEnd++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, range.m_Count, range.m_ShaderRegister);

                for (size_t shaderRegister = rangeBegin; shaderRegister < rangeEnd; ++shaderRegister)
                {
                    ResourceLayoutBinding bindingLayout;
                    ZeroStruct(bindingLayout);

                    bindingLayout.ViewType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                    bindingLayout.ShaderStage = shaderStage;
                    bindingLayout.ShaderSlot = shaderRegister;

#ifdef GFX_DEBUG
                    bindingLayout.DescriptorOffset = descriptorEnd;
#endif

                    m_TableResources.push_back(bindingLayout);
                    ++descriptorEnd;
                }

                if constexpr (!(DX12_DESCRIPTORTABLE_MERGERANGES_MODE & DX12_DESCRIPTORTABLE_MERGERANGES_CBV))
                {
                    MakeDescriptorTable(rangeCursorEnd, rangeCursorBegin, descriptorEnd, descriptorBegin, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, visibility);
                }
            }
        }

        if constexpr (!(DX12_DESCRIPTORTABLE_MERGERANGES_MODE & DX12_DESCRIPTORTABLE_MERGERANGES_SRV))
        {
            MakeDescriptorTable(rangeCursorEnd, rangeCursorBegin, descriptorEnd, descriptorBegin, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, visibility);
        }

        if (bindings.m_InputResources.m_DescriptorCount)
        {
            auto& ranges = bindings.m_InputResources.m_Ranges;
            for (size_t rangeIdx = 0; rangeIdx < ranges.size(); ++rangeIdx)
            {
                const ReflectedBindingRange range = ranges[rangeIdx];
                AZ::u32 rangeBegin = range.m_ShaderRegister;
                AZ::u32 rangeEnd = range.m_ShaderRegister + range.m_Count;

                if (!range.m_bUsed)
                {
                    // Setup the configuration of the root signature entry (a constant dummy SRV in this case)
                    for (AZ::u32 rootParameterIdx = rangeBegin; rootParameterIdx < rangeEnd; ++rootParameterIdx)
                    {
                        m_RootParameters[m_NumRootParameters++].InitAsShaderResourceView(rootParameterIdx, 0, visibility);
                    }

                    continue;
                }

                // Setup the configuration of the descriptor table range (a number of SRVs in this case)
                m_DescRanges[rangeCursorEnd++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, range.m_Count, range.m_ShaderRegister);

                for (size_t shaderRegister = rangeBegin; shaderRegister < rangeEnd; ++shaderRegister)
                {
                    ResourceLayoutBinding bindingLayout;
                    ZeroStruct(bindingLayout);

                    bindingLayout.ViewType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                    bindingLayout.ShaderStage = shaderStage;
                    bindingLayout.ShaderSlot = shaderRegister;
#ifdef GFX_DEBUG
                    bindingLayout.DescriptorOffset = descriptorEnd;
#endif
                    m_TableResources.push_back(bindingLayout);
                    ++descriptorEnd;
                }

                if constexpr (!(DX12_DESCRIPTORTABLE_MERGERANGES_MODE & DX12_DESCRIPTORTABLE_MERGERANGES_SRV))
                {
                    MakeDescriptorTable(rangeCursorEnd, rangeCursorBegin, descriptorEnd, descriptorBegin, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, visibility);
                }
            }
        }

        if constexpr (!(DX12_DESCRIPTORTABLE_MERGERANGES_MODE & DX12_DESCRIPTORTABLE_MERGERANGES_UAV))
        {
            MakeDescriptorTable(rangeCursorEnd, rangeCursorBegin, descriptorEnd, descriptorBegin, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, visibility);
        }

        if (bindings.m_OutputResources.m_DescriptorCount)
        {
            auto& ranges = bindings.m_OutputResources.m_Ranges;
            for (size_t rangeIdx = 0; rangeIdx < ranges.size(); ++rangeIdx)
            {
                const ReflectedBindingRange range = ranges[rangeIdx];
                AZ::u32 rangeBegin = range.m_ShaderRegister;
                AZ::u32 rangeEnd = range.m_ShaderRegister + range.m_Count;

                if (!range.m_bUsed)
                {
                    // Setup the configuration of the root signature entry (a constant dummy UAV in this case)
                    for (AZ::u32 rootParameterIdx = rangeBegin; rootParameterIdx < rangeEnd; ++rootParameterIdx)
                    {
                        m_RootParameters[m_NumRootParameters++].InitAsUnorderedAccessView(rootParameterIdx, 0, visibility);
                    }

                    continue;
                }

                // Setup the configuration of the descriptor table range (a number of UAVs in this case)
                m_DescRanges[rangeCursorEnd++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, range.m_Count, range.m_ShaderRegister);

                for (size_t shaderRegister = rangeBegin; shaderRegister < rangeEnd; ++shaderRegister)
                {
                    ResourceLayoutBinding bindingLayout;
                    ZeroStruct(bindingLayout);

                    bindingLayout.ViewType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                    bindingLayout.ShaderStage = shaderStage;
                    bindingLayout.ShaderSlot = shaderRegister;
#ifdef GFX_DEBUG
                    bindingLayout.DescriptorOffset = descriptorEnd;
#endif
                    m_TableResources.push_back(bindingLayout);
                    ++descriptorEnd;
                }

                if constexpr (!(DX12_DESCRIPTORTABLE_MERGERANGES_MODE & DX12_DESCRIPTORTABLE_MERGERANGES_UAV))
                {
                    MakeDescriptorTable(rangeCursorEnd, rangeCursorBegin, descriptorEnd, descriptorBegin, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, visibility);
                }
            }
        }

        if constexpr (DX12_DESCRIPTORTABLE_MERGERANGES_MODE != 0)
        {
            MakeDescriptorTable(rangeCursorEnd, rangeCursorBegin, descriptorEnd, descriptorBegin, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, visibility);
        }

        m_NumDescriptors = descriptorEnd;

        AZ::u32 lastSamplers = m_NumDynamicSamplers;
        AZ::u32 currentSamplers = m_NumDynamicSamplers;

        if (bindings.m_Samplers.m_DescriptorCount)
        {
            auto& ranges = bindings.m_Samplers.m_Ranges;
            for (size_t rangeIdx = 0; rangeIdx < ranges.size(); ++rangeIdx)
            {
                const ReflectedBindingRange range = ranges[rangeIdx];
                AZ::u32 rangeBegin = range.m_ShaderRegister;
                AZ::u32 rangeEnd = range.m_ShaderRegister + range.m_Count;

                if (!range.m_bUsed)
                {
                    // Setup the configuration of the root signature entry (a constant dummy UAV in this case)
                    for (AZ::u32 rootParameterIdx = rangeBegin; rootParameterIdx < rangeEnd; ++rootParameterIdx)
                    {
                        __debugbreak();
                        //                      m_StaticSamplers[m_NumStaticSamplers++].Init(rj, 0, ..., static_cast<D3D12_SHADER_VISIBILITY>((stage + 1) % (D3D12_SHADER_VISIBILITY_PIXEL + 1)));
                    }

                    continue;
                }

                // Setup the configuration of the descriptor table range (a number of Samplers in this case)
                m_DescRanges[rangeCursorEnd++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, range.m_Count, range.m_ShaderRegister);

                for (size_t shaderRegister = rangeBegin; shaderRegister < rangeEnd; ++shaderRegister)
                {
                    ResourceLayoutBinding bindingLayout;
                    ZeroStruct(bindingLayout);

                    bindingLayout.ViewType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                    bindingLayout.ShaderStage = shaderStage;
                    bindingLayout.ShaderSlot = shaderRegister;
#ifdef GFX_DEBUG
                    bindingLayout.DescriptorOffset = currentSamplers;
#endif
                    m_Samplers.push_back(bindingLayout);
                    ++currentSamplers;
                }

                if constexpr (!(DX12_DESCRIPTORTABLE_MERGERANGES_MODE & DX12_DESCRIPTORTABLE_MERGERANGES_SMP))
                {
                    MakeDescriptorTable(rangeCursorEnd, rangeCursorBegin, currentSamplers, lastSamplers, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, visibility);
                }
            }
        }

        if constexpr (bool(DX12_DESCRIPTORTABLE_MERGERANGES_MODE & DX12_DESCRIPTORTABLE_MERGERANGES_SMP))
        {
            MakeDescriptorTable(rangeCursorEnd, rangeCursorBegin, currentSamplers, lastSamplers, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, visibility);
        }

        m_NumDynamicSamplers = currentSamplers;
        m_DescRangeCursor = rangeCursorEnd;
    }

    void PipelineLayout::DebugPrint() const
    {
        DX12_LOG("Root Signature:");
        for (AZ::u32 p = 0, r = 0; p < m_NumRootParameters; ++p)
        {
            DX12_LOG(" %s: %c [%2d to %2d] -> %s [%2d to %2d]",
                m_RootParameters[p].ShaderVisibility == D3D12_SHADER_VISIBILITY_ALL      ? "Every    shader stage" :
                m_RootParameters[p].ShaderVisibility == D3D12_SHADER_VISIBILITY_VERTEX   ? "Vertex   shader stage" :
                m_RootParameters[p].ShaderVisibility == D3D12_SHADER_VISIBILITY_HULL     ? "Hull     shader stage" :
                m_RootParameters[p].ShaderVisibility == D3D12_SHADER_VISIBILITY_DOMAIN   ? "Domain   shader stage" :
                m_RootParameters[p].ShaderVisibility == D3D12_SHADER_VISIBILITY_GEOMETRY ? "Geometry shader stage" :
                m_RootParameters[p].ShaderVisibility == D3D12_SHADER_VISIBILITY_PIXEL    ? "Pixel    shader stage" : "Unknown  shader stage",
                m_RootParameters[p].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE && m_RootParameters[p].DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV ? 'C' :
                m_RootParameters[p].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE && m_RootParameters[p].DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV ? 'T' :
                m_RootParameters[p].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE && m_RootParameters[p].DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV ? 'U' :
                m_RootParameters[p].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE && m_RootParameters[p].DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER ? 'S' :
                m_RootParameters[p].ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV ? 'c' :
                m_RootParameters[p].ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV ? 't' :
                m_RootParameters[p].ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV ? 'u' : '?',
                m_RootParameters[p].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE ? m_RootParameters[p].DescriptorTable.pDescriptorRanges->BaseShaderRegister : m_RootParameters[p].Descriptor.ShaderRegister,
                m_RootParameters[p].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE ? m_RootParameters[p].DescriptorTable.pDescriptorRanges->BaseShaderRegister + m_RootParameters[p].DescriptorTable.pDescriptorRanges->NumDescriptors : 1,
                m_RootParameters[p].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE ? "descriptor table" :
                m_RootParameters[p].ParameterType != D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS ? "descriptor" : "32bit constant",
                m_RootParameters[p].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE ? m_DescriptorTables[r].m_Offset : -1,
                m_RootParameters[p].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE ? m_DescriptorTables[r].m_Offset + m_RootParameters[p].DescriptorTable.pDescriptorRanges->NumDescriptors : -1);

            r += m_RootParameters[p].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        }
    }

    RootSignature::RootSignature(Device* device)
        : DeviceObject(device)
        , m_nodeMask(0)
    {
    }

    RootSignature::~RootSignature()
    {
    }

    bool RootSignature::Init(const ComputeInitParams& params)
    {
        m_PipelineLayout.Build(params.m_ComputeShader);
        return Init(CommandModeCompute);
    }

    bool RootSignature::Init(const GraphicsInitParams& params)
    {
        m_PipelineLayout.Build(
            params.m_VertexShader,
            params.m_HullShader ,
            params.m_DomainShader,
            params.m_GeometryShader,
            params.m_PixelShader);

        return Init(CommandModeGraphics);
    }

    bool RootSignature::Init(PipelineLayout& pipelineLayout, CommandMode commandType)
    {
        m_PipelineLayout = pipelineLayout;
        return Init(commandType);
    }

    bool RootSignature::Init(CommandMode commandType)
    {
        ID3D12RootSignature* rootSign = NULL;
        CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;

        if (commandType == CommandModeGraphics)
        {
            D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            flags |= (m_PipelineLayout.m_ShaderStageAccess & BIT(ESS_Vertex)) == 0 ? D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS : D3D12_ROOT_SIGNATURE_FLAG_NONE;
            flags |= (m_PipelineLayout.m_ShaderStageAccess & BIT(ESS_Hull)) == 0 ? D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS : D3D12_ROOT_SIGNATURE_FLAG_NONE;
            flags |= (m_PipelineLayout.m_ShaderStageAccess & BIT(ESS_Domain)) == 0 ? D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS : D3D12_ROOT_SIGNATURE_FLAG_NONE;
            flags |= (m_PipelineLayout.m_ShaderStageAccess & BIT(ESS_Geometry)) == 0 ? D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS : D3D12_ROOT_SIGNATURE_FLAG_NONE;
            flags |= (m_PipelineLayout.m_ShaderStageAccess & BIT(ESS_Pixel)) == 0 ? D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS : D3D12_ROOT_SIGNATURE_FLAG_NONE;

            descRootSignature.Init(
                m_PipelineLayout.m_NumRootParameters,
                m_PipelineLayout.m_RootParameters,
                m_PipelineLayout.m_NumStaticSamplers,
                m_PipelineLayout.m_StaticSamplers,
                flags);
        }
        else
        {
            descRootSignature.Init(
                m_PipelineLayout.m_NumRootParameters,
                m_PipelineLayout.m_RootParameters,
                0, NULL,
                D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);
        }

        {
            ID3DBlob* pOutBlob = NULL;
            ID3DBlob* pErrorBlob = NULL;

            if (S_OK != D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob))
            {
                DX12_ERROR("Could not serialize root signature!");
                return false;
            }

            if (S_OK != GetDevice()->GetD3D12Device()->CreateRootSignature(m_nodeMask, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_GRAPHICS_PPV_ARGS(&rootSign)))
            {
                DX12_ERROR("Could not create root signature!");
                return false;
            }

            if (pOutBlob)
            {
                pOutBlob->Release();
            }

            if (pErrorBlob)
            {
                pErrorBlob->Release();
            }
        }

        m_pRootSignature = rootSign;
        rootSign->Release();
        return true;
    }

    RootSignatureCache::RootSignatureCache()
    {
    }

    RootSignatureCache::~RootSignatureCache()
    {
    }

    bool RootSignatureCache::Init(Device* device)
    {
        m_pDevice = device;
        return true;
    }

    static const THash ComputeBit = 1;

    RootSignature* RootSignatureCache::AcquireRootSignature(const RootSignature::GraphicsInitParams& params)
    {
        struct HashKey
        {
            AZ::u32 vs;
            AZ::u32 hs;
            AZ::u32 ds;
            AZ::u32 gs;
            AZ::u32 ps;
        } hashKey;

        hashKey.vs = params.m_VertexShader   ? params.m_VertexShader->GetReflectedBindingHash()   : 0;
        hashKey.hs = params.m_HullShader     ? params.m_HullShader->GetReflectedBindingHash()     : 0;
        hashKey.ds = params.m_DomainShader   ? params.m_DomainShader->GetReflectedBindingHash()   : 0;
        hashKey.gs = params.m_GeometryShader ? params.m_GeometryShader->GetReflectedBindingHash() : 0;
        hashKey.ps = params.m_PixelShader    ? params.m_PixelShader->GetReflectedBindingHash()    : 0;

        // LSB filled marks compute pipeline states, in case graphics and compute hashes collide
        THash hash = ~ComputeBit & ComputeSmallHash<sizeof(HashKey)>(&hashKey);
        {
            auto iter = m_RootSignatureMap.find(hash);
            if (iter != m_RootSignatureMap.end())
            {
                return iter->second.get();
            }
        }

        RootSignature* result = new RootSignature(m_pDevice);
        if (!result->Init(params))
        {
            DX12_ERROR("Could not create root signature!");
            return nullptr;
        }

        m_RootSignatureMap[hash] = result;
        return result;
    }

    RootSignature* RootSignatureCache::AcquireRootSignature(const RootSignature::ComputeInitParams& params)
    {
        THash hash = ComputeBit | params.m_ComputeShader->GetReflectedBindingHash();
        {
            auto iter = m_RootSignatureMap.find(hash);
            if (iter != m_RootSignatureMap.end())
            {
                return iter->second;
            }
        }

        RootSignature* result = new RootSignature(m_pDevice);
        if (!result->Init(params))
        {
            DX12_ERROR("Could not create root signature!");
            return nullptr;
        }

        m_RootSignatureMap[hash] = result;
        return result;
    }
}
