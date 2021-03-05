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
#include "SCryDX11PipelineState.hpp"

#include "DX12/API/DX12Shader.hpp"
#include "DX12/API/DX12RootSignature.hpp"
#include "DX12/Device/CCryDX12DeviceContext.hpp"

static D3D12_SHADER_BYTECODE g_EmptyShader = { NULL, 0 };

const D3D12_SHADER_BYTECODE& SCryDX11ShaderStageState::GetD3D12ShaderBytecode() const
{
    return Shader.m_Value ? Shader.m_Value->GetD3D12ShaderBytecode() : g_EmptyShader;
}

AZ::u32 SCryDX11ShaderStageState::GetShaderHash() const 
{
    return Shader.m_Value ? static_cast<AZ::u32>(Shader.m_Value->GetDX12Shader()->GetHash()) : 0;
}

#ifndef NDEBUG
static const char* TopologyToString(D3D11_PRIMITIVE_TOPOLOGY type)
{
    switch (type)
    {
    case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
        return "point list";
    case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
        return "line list";
    case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
        return "line strip";
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
        return "triangle list";
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
        return "triangle strip";
    case D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ:
        return "line list adj";
    case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ:
        return "line strip adj";
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ:
        return "triangle list adj";
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ:
        return "triangle strip adj";
    default:
        return "unmapped type";
    }
}

static const char* TypeToString(UINT8 dim)
{
    switch (dim)
    {
    case D3D_SIT_CBUFFER:
        return "constant buffer";
    case D3D_SIT_TBUFFER:
        return "texture buffer";
    case D3D_SIT_TEXTURE:
        return "texture";
    case D3D_SIT_SAMPLER:
        return "sampler";
    case D3D_SIT_UAV_RWTYPED:
        return "typed r/w texture";
    case D3D_SIT_STRUCTURED:
        return "structured buffer";
    case D3D_SIT_UAV_RWSTRUCTURED:
        return "structured r/w buffer";
    case D3D_SIT_BYTEADDRESS:
        return "raw buffer";
    case D3D_SIT_UAV_RWBYTEADDRESS:
        return "raw r/w buffer";
    case D3D_SIT_UAV_APPEND_STRUCTURED:
        return "append buffer";
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
        return "consume buffer";
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        return "structured r/w buffer with counter";
    default:
        return "unmapped dimension";
    }
}

static const char* DimensionToString(UINT8 dim)
{
    switch (dim)
    {
    case D3D_SRV_DIMENSION_UNKNOWN:
        return "unknown";
    case D3D_SRV_DIMENSION_BUFFER:
        return "buffer";
    case D3D_SRV_DIMENSION_TEXTURE1D:
        return "texture1d";
    case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        return "texture1d array";
    case D3D_SRV_DIMENSION_TEXTURE2D:
        return "texture2d";
    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        return "texture2d array";
    case D3D_SRV_DIMENSION_TEXTURE2DMS:
        return "texture2d ms";
    case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
        return "texture2d ms array";
    case D3D_SRV_DIMENSION_TEXTURE3D:
        return "texture3d";
    case D3D_SRV_DIMENSION_TEXTURECUBE:
        return "texturecube";
    case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
        return "texturecube array";
    case D3D_SRV_DIMENSION_BUFFEREX:
        return "buffer ex";
    default:
        return "unmapped dimension";
    }
}

static const char* DimensionToString(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
{
    switch (static_cast<D3D_SRV_DIMENSION>(desc.ViewDimension))
    {
    case D3D_SRV_DIMENSION_UNKNOWN:
        return "unknown";
    case D3D_SRV_DIMENSION_BUFFER:
        return desc.Buffer.Flags == D3D12_BUFFER_SRV_FLAG_RAW ? "raw buffer" : desc.Buffer.StructureByteStride > 0 ? "structured buffer" : "buffer";
    case D3D_SRV_DIMENSION_TEXTURE1D:
        return "texture1d";
    case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        return "texture1d array";
    case D3D_SRV_DIMENSION_TEXTURE2D:
        return "texture2d";
    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        return "texture2d array";
    case D3D_SRV_DIMENSION_TEXTURE2DMS:
        return "texture2d ms";
    case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
        return "texture2d ms array";
    case D3D_SRV_DIMENSION_TEXTURE3D:
        return "texture3d";
    case D3D_SRV_DIMENSION_TEXTURECUBE:
        return "texturecube";
    case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
        return "texturecube array";
    case D3D_SRV_DIMENSION_BUFFEREX:
        return desc.Buffer.Flags == D3D12_BUFFER_SRV_FLAG_RAW ? "raw buffer ex" : desc.Buffer.StructureByteStride > 0 ? "structured buffer" : "buffer ex";
    default:
        return "unmapped dimension";
    }
}

static const char* DimensionToString(const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
{
    switch (static_cast<D3D_SRV_DIMENSION>(desc.ViewDimension))
    {
    case D3D_SRV_DIMENSION_UNKNOWN:
        return "r/w unknown";
    case D3D_SRV_DIMENSION_BUFFER:
        return desc.Buffer.Flags == D3D12_BUFFER_UAV_FLAG_RAW ? "raw r/w buffer" : desc.Buffer.StructureByteStride > 0 ? "structured r/w buffer" : "r/w buffer";
    case D3D_SRV_DIMENSION_TEXTURE1D:
        return "r/w texture1d";
    case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        return "r/w texture1d array";
    case D3D_SRV_DIMENSION_TEXTURE2D:
        return "r/w texture2d";
    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        return "r/w texture2d array";
    case D3D_SRV_DIMENSION_TEXTURE2DMS:
        return "r/w texture2d ms";
    case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
        return "r/w texture2d ms array";
    case D3D_SRV_DIMENSION_TEXTURE3D:
        return "r/w texture3d";
    case D3D_SRV_DIMENSION_TEXTURECUBE:
        return "r/w texturecube";
    case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
        return "r/w texturecube array";
    case D3D_SRV_DIMENSION_BUFFEREX:
        return desc.Buffer.Flags == D3D12_BUFFER_UAV_FLAG_RAW ? "raw r/w buffer ex" : desc.Buffer.StructureByteStride > 0 ? "structured r/w buffer ex" : "r/w buffer ex";
    default:
        return "r/w unmapped dimension";
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ASSIGN_TARGET(member) \
    member.m_pStateFlags = &(pParent->m_StateFlags);

void SCryDX11ShaderStageState::Init(SCryDX11PipelineState* pParent)
{
    ASSIGN_TARGET(Shader);
    ASSIGN_TARGET(ConstantBufferViews);
    ASSIGN_TARGET(ConstBufferBindRange);
    ASSIGN_TARGET(ShaderResourceViews);
    ASSIGN_TARGET(UnorderedAccessViews);
    ASSIGN_TARGET(SamplerState);
}

void SCryDX11ShaderStageState::DebugPrint()
{
    switch (Type)
    {
    case DX12::ESS_Vertex:
        DX12_LOG("Vertex shader stage:");
        break;
    case DX12::ESS_Hull:
        DX12_LOG("Hull shader stage:");
        break;
    case DX12::ESS_Domain:
        DX12_LOG("Domain shader stage:");
        break;
    case DX12::ESS_Geometry:
        DX12_LOG("Geometry shader stage:");
        break;
    case DX12::ESS_Pixel:
        DX12_LOG("Pixel shader stage:");
        break;
    case DX12::ESS_Compute:
        DX12_LOG("Compute shader stage:");
        break;
    }

    DX12_LOG("Shader = %p %s",
        Shader.Get(),
        Shader.Get()->GetName().c_str()
        );

    const DX12::ReflectedBindings& reflectedBindings = Shader.Get()->GetDX12Shader()->GetReflectedBindings();
    if (reflectedBindings.m_ConstantBuffers.m_DescriptorCount +
        reflectedBindings.m_InputResources.m_DescriptorCount +
        reflectedBindings.m_OutputResources.m_DescriptorCount +
        reflectedBindings.m_Samplers.m_DescriptorCount)
    {
        DX12_LOG(" Resource Binding Table:");
    }

    if (reflectedBindings.m_ConstantBuffers.m_DescriptorCount)
    {
        const auto& ranges = reflectedBindings.m_ConstantBuffers.m_Ranges;
        for (size_t rangeIdx = 0; rangeIdx < ranges.size(); ++rangeIdx)
        {
            const DX12::ReflectedBindingRange range = ranges[rangeIdx];

            DX12_LOG(" C [%2d to %2d]:", range.m_ShaderRegister, range.m_ShaderRegister + range.m_Count);
            for (size_t i = range.m_ShaderRegister; i < range.m_ShaderRegister + range.m_Count; ++i)
            {
                if (ConstantBufferViews.Get(i))
                {
                    DX12_LOG("  %2d: %p %p %p+%d[%d] %s", i,
                        ConstantBufferViews.Get(i).get(),
                        &ConstantBufferViews.Get(i)->GetDX12Resource(),
                        ConstantBufferViews.Get(i)->GetDX12View().GetCBVDesc().BufferLocation,
                        ConstBufferBindRange.Get(i).start,
                        ConstBufferBindRange.Get(i).end - ConstBufferBindRange.Get(i).start,
                        ConstantBufferViews.Get(i)->GetName().c_str()
                        );
                }
                else
                {
                    DX12_LOG("  %2d: ERROR! Null resource.", i);
                }
            }
        }
    }

    if (reflectedBindings.m_InputResources.m_DescriptorCount)
    {
        const auto& ranges = reflectedBindings.m_InputResources.m_Ranges;
        for (size_t rangeIdx = 0; rangeIdx < ranges.size(); ++rangeIdx)
        {
            const DX12::ReflectedBindingRange range = ranges[rangeIdx];

            DX12_LOG(" T [%2d to %2d]:", range.m_ShaderRegister, range.m_ShaderRegister + range.m_Count);
            for (size_t i = range.m_ShaderRegister; i < (range.m_ShaderRegister + range.m_Count); ++i)
            {
                if (ShaderResourceViews.Get(i))
                {
                    DX12_LOG("  %2d: %p %p [%s, %s, %s] %s", i,
                        ShaderResourceViews.Get(i).get(),
                        ShaderResourceViews.Get(i)->GetD3D12Resource(),
                        TypeToString(range.m_Types[i - range.m_ShaderRegister]),
                        DimensionToString(range.m_Dimensions[i - range.m_ShaderRegister]),
                        DimensionToString(ShaderResourceViews.Get(i)->GetDX12View().GetSRVDesc()),
                        ShaderResourceViews.Get(i)->GetResourceName().c_str()
                        );
                }
                else
                {
                    DX12_LOG("  %2d: ERROR! Null resource.", i);
                }
            }
        }
    }

    if (reflectedBindings.m_OutputResources.m_DescriptorCount)
    {
        const auto& ranges = reflectedBindings.m_OutputResources.m_Ranges;
        for (size_t rangeIdx = 0; rangeIdx < ranges.size(); ++rangeIdx)
        {
            const DX12::ReflectedBindingRange range = ranges[rangeIdx];

            DX12_LOG(" U [%2d to %2d]:", range.m_ShaderRegister, range.m_ShaderRegister + range.m_Count);
            for (size_t i = range.m_ShaderRegister; i < range.m_ShaderRegister + range.m_Count; ++i)
            {
                if (UnorderedAccessViews.Get(i))
                {
                    DX12_LOG("  %2d: %p %p [%s, %s, %s] %s", i,
                        UnorderedAccessViews.Get(i).get(),
                        UnorderedAccessViews.Get(i)->GetD3D12Resource(),
                        TypeToString(range.m_Types[i - range.m_ShaderRegister]),
                        DimensionToString(range.m_Dimensions[i - range.m_ShaderRegister]),
                        DimensionToString(UnorderedAccessViews.Get(i)->GetDX12View().GetUAVDesc()),
                        UnorderedAccessViews.Get(i)->GetResourceName().c_str()
                        );
                }
                else
                {
                    DX12_LOG("  %2d: ERROR! Null resource.", i);
                }
            }
        }
    }

    if (reflectedBindings.m_Samplers.m_DescriptorCount)
    {
        const auto& ranges = reflectedBindings.m_Samplers.m_Ranges;
        for (size_t rangeIdx = 0; rangeIdx < ranges.size(); ++rangeIdx)
        {
            const DX12::ReflectedBindingRange range = ranges[rangeIdx];

            DX12_LOG(" S [%2d to %2d]", range.m_ShaderRegister, range.m_ShaderRegister + range.m_Count);
            for (size_t i = range.m_ShaderRegister; i < range.m_ShaderRegister + range.m_Count; ++i)
            {
                if (SamplerState.Get(i))
                {
                    DX12_LOG("  %2d: %p", i, SamplerState.Get(i).get());
                }
                else
                {
                    DX12_LOG("  %2d: ERROR! Null resource.", i);
                }
            }
        }
    }

    DX12_LOG("");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SCryDX11IAState::Init(SCryDX11PipelineState* pParent)
{
    ASSIGN_TARGET(PrimitiveTopology);
    ASSIGN_TARGET(InputLayout);
    ASSIGN_TARGET(VertexBuffers);
    ASSIGN_TARGET(Strides);
    ASSIGN_TARGET(Offsets);
    ASSIGN_TARGET(NumVertexBuffers);
    ASSIGN_TARGET(IndexBuffer);
    ASSIGN_TARGET(IndexBufferFormat);
    ASSIGN_TARGET(IndexBufferOffset);
}

void SCryDX11IAState::DebugPrint()
{
    DX12_LOG("IA:");
    DX12_LOG(" PrimitiveTopology: %d => %s", PrimitiveTopology.Get(), TopologyToString(PrimitiveTopology.Get()));
    DX12_LOG(" InputLayout: %p", InputLayout.Get());

    if (NumVertexBuffers.Get())
    {
        DX12_LOG(" VertexBuffers:");
    }
    for (size_t i = 0; i < NumVertexBuffers.Get(); ++i)
    {
        if (VertexBuffers.Get(i))
        {
            DX12_LOG("  %2d: %p %p %s", i,
                VertexBuffers.Get(i).get(),
                VertexBuffers.Get(i)->GetD3D12Resource(),
                VertexBuffers.Get(i)->GetName().c_str()
                );
        }
    }

    DX12_LOG(" IndexBuffer:");
    DX12_LOG("  --: %p %p %s",
        IndexBuffer.Get(),
        IndexBuffer.Get() ? IndexBuffer.Get()->GetD3D12Resource() : nullptr,
        IndexBuffer.Get() ? IndexBuffer.Get()->GetName().c_str() : "-"
        );

    DX12_LOG("");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SCryDX11RasterizerState::Init(SCryDX11PipelineState* pParent)
{
    ASSIGN_TARGET(DepthStencilState);
    ASSIGN_TARGET(RasterizerState);
    ASSIGN_TARGET(Viewports);
    ASSIGN_TARGET(NumViewports);
    ASSIGN_TARGET(Scissors);
    ASSIGN_TARGET(NumScissors);
    ASSIGN_TARGET(ScissorEnabled);
}

void SCryDX11RasterizerState::DebugPrint()
{
    DX12_LOG("Rasterizer state:");
    DX12_LOG(" DepthStencilState: %p", DepthStencilState.Get());
    DX12_LOG(" RasterizerState: %p", RasterizerState.Get());

    DX12_LOG("");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SCryDX11OutputMergerState::Init(SCryDX11PipelineState* pParent)
{
    ASSIGN_TARGET(BlendState);
    ASSIGN_TARGET(RenderTargetViews);
    ASSIGN_TARGET(DepthStencilView);
    ASSIGN_TARGET(SampleMask);
    ASSIGN_TARGET(NumRenderTargets);
    ASSIGN_TARGET(RTVFormats);
    ASSIGN_TARGET(DSVFormat);
    ASSIGN_TARGET(SampleDesc);
    ASSIGN_TARGET(StencilRef);
}

#undef ASSIGN_TARGET

void SCryDX11OutputMergerState::DebugPrint()
{
    DX12_LOG("Output merger:");
    DX12_LOG(" BlendState: %p", BlendState.Get());

    DX12_LOG(" DepthStencilView:");
    DX12_LOG("  --: %p %p %s",
        DepthStencilView.Get(),
        DepthStencilView.Get() ? DepthStencilView.Get()->GetD3D12Resource() : nullptr,
        DepthStencilView.Get() ? DepthStencilView.Get()->GetResourceName().c_str() : "-"
        );

    if (NumRenderTargets.Get())
    {
        DX12_LOG(" RenderTargetViews:");
    }
    for (size_t i = 0; i < NumRenderTargets.Get(); ++i)
    {
        if (RenderTargetViews.Get(i))
        {
            DX12_LOG("  %2d: %p %p %s", i,
                RenderTargetViews.Get(i).get(),
                RenderTargetViews.Get(i)->GetD3D12Resource(),
                RenderTargetViews.Get(i)->GetResourceName().c_str()
                );
        }
    }

    DX12_LOG("");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SCryDX11PipelineState::MakeInitParams(DX12::ComputePipelineState::InitParams& params)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC& desc = params.desc;
    ZeroMemory(&desc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));

    desc.CS = Stages[DX12::ESS_Compute ].GetD3D12ShaderBytecode();
    params.hash = Stages[DX12::ESS_Compute].GetShaderHash();
}

void SCryDX11PipelineState::MakeInitParams(DX12::GraphicsPipelineState::InitParams& params)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = params.desc;
    ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    size_t hash = 0;

    // Set blend state
    if (OutputMerger.BlendState)
    {
        desc.BlendState = OutputMerger.BlendState->GetD3D12BlendDesc();
    }

    // Set sample mask
    desc.SampleMask = OutputMerger.SampleMask.Get();

    // Set rasterizer state
    if (Rasterizer.RasterizerState)
    {
        desc.RasterizerState = Rasterizer.RasterizerState->GetD3D12RasterizerDesc();
    }
    else
    {
        desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    }

    // Set depth/stencil state
    if (Rasterizer.DepthStencilState)
    {
        desc.DepthStencilState = Rasterizer.DepthStencilState->GetD3D12DepthStencilDesc();
    }

    desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

    // Set primitive topology type
    D3D12_PRIMITIVE_TOPOLOGY_TYPE primTopo12 = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    switch (InputAssembler.PrimitiveTopology.Get())
    {
    case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
        primTopo12 = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        break;
    case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
        primTopo12 = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;
    case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
        primTopo12 = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
        primTopo12 = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
        primTopo12 = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;
    case D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ:
        primTopo12 = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;
    case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ:
        primTopo12 = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ:
        primTopo12 = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ:
        primTopo12 = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;
    default:
        if (InputAssembler.PrimitiveTopology.Get() >= D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST)
        {
            primTopo12 = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        }
    }

    desc.PrimitiveTopologyType = primTopo12;

    // Set render targets
    desc.NumRenderTargets = OutputMerger.NumRenderTargets.Get();

    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        desc.RTVFormats[i] = i < desc.NumRenderTargets ? OutputMerger.RTVFormats.Get(i) : DXGI_FORMAT_UNKNOWN;
    }

    desc.DSVFormat = OutputMerger.DSVFormat.Get();

    // Set sample descriptor
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    // Be careful to not hash pointers. We want this hash consistent between runs so the PSO can be saved to disk and reloaded on subsequent runs
    hash = DX12::ComputeSmallHash<sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC)>(&desc);

    desc.VS = Stages[DX12::ESS_Vertex].GetD3D12ShaderBytecode();
    desc.PS = Stages[DX12::ESS_Pixel].GetD3D12ShaderBytecode();
    desc.HS = Stages[DX12::ESS_Hull].GetD3D12ShaderBytecode();
    desc.DS = Stages[DX12::ESS_Domain].GetD3D12ShaderBytecode();
    desc.GS = Stages[DX12::ESS_Geometry].GetD3D12ShaderBytecode();

    for (int i = 0; i < DX12::ESS_LastWithoutCompute; ++i)
    {
        AZStd::hash_combine(hash, Stages[i].GetShaderHash());
    }

    // Apply input layout
    if (InputAssembler.InputLayout)
    {
        const CCryDX12InputLayout::TDescriptors& descriptors = InputAssembler.InputLayout->GetDescriptors();

        desc.InputLayout.pInputElementDescs = descriptors.empty() ? NULL : &(descriptors[0]);
        desc.InputLayout.NumElements = static_cast<UINT>(descriptors.size());

        AZStd::hash_combine(hash, InputAssembler.InputLayout->GetHash());
    }

    params.hash = fasthash::fasthash64_to_32(hash);
}

void SCryDX11PipelineState::MakeInitParams(DX12::RootSignature::ComputeInitParams& params)
{
    params.m_ComputeShader = nullptr;
    if (Stages[DX12::ESS_Compute].Shader)
    {
        params.m_ComputeShader = Stages[DX12::ESS_Compute].Shader->GetDX12Shader();
    }
}

void SCryDX11PipelineState::MakeInitParams(DX12::RootSignature::GraphicsInitParams& params)
{
    params.m_VertexShader   = Stages[DX12::ESS_Vertex].Shader   ? Stages[DX12::ESS_Vertex].Shader->GetDX12Shader()   : nullptr;
    params.m_HullShader     = Stages[DX12::ESS_Hull].Shader     ? Stages[DX12::ESS_Hull].Shader->GetDX12Shader()     : nullptr;
    params.m_DomainShader   = Stages[DX12::ESS_Domain].Shader   ? Stages[DX12::ESS_Domain].Shader->GetDX12Shader()   : nullptr;
    params.m_GeometryShader = Stages[DX12::ESS_Geometry].Shader ? Stages[DX12::ESS_Geometry].Shader->GetDX12Shader() : nullptr;
    params.m_PixelShader    = Stages[DX12::ESS_Pixel].Shader    ? Stages[DX12::ESS_Pixel].Shader->GetDX12Shader()    : nullptr;
}

extern int g_nPrintDX12;

void SCryDX11PipelineState::DebugPrint()
{
    if (!g_nPrintDX12)
    {
        return;
    }

    // General
    for (size_t i = 0; i < DX12::ESS_Num; ++i)
    {
        if (Stages[i].Shader.Get())
        {
            Stages[i].DebugPrint();
        }
    }

    // Graphics Fixed Function
    InputAssembler.DebugPrint();
    Rasterizer.DebugPrint();
    OutputMerger.DebugPrint();
}
