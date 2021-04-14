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
#include "CCryDX12DeviceContext.hpp"
#include "CCryDX12Device.hpp"

#include "DX12/Resource/CCryDX12Resource.hpp"

#include "DX12/Resource/Misc/CCryDX12Buffer.hpp"
#include "DX12/Resource/Misc/CCryDX12Shader.hpp"
#include "DX12/Resource/Misc/CCryDX12Query.hpp"

#include "DX12/Resource/State/CCryDX12SamplerState.hpp"

#include "DX12/Resource/Texture/CCryDX12Texture1D.hpp"
#include "DX12/Resource/Texture/CCryDX12Texture2D.hpp"
#include "DX12/Resource/Texture/CCryDX12Texture3D.hpp"

#include "DX12/Resource/View/CCryDX12DepthStencilView.hpp"
#include "DX12/Resource/View/CCryDX12RenderTargetView.hpp"
#include "DX12/Resource/View/CCryDX12ShaderResourceView.hpp"
#include "DX12/Resource/View/CCryDX12UnorderedAccessView.hpp"

#include "DX12/API/DX12Device.hpp"

#define DX12_SUBMISSION_UNBOUND 3 // never commit, only when the heaps overflow, or on present
#define DX12_SUBMISSION_PERPSO  2 // commit whenever the PSO changes
#define DX12_SUBMISSION_PERDRAW 1 // commit whenever a new draw is requested
#define DX12_SUBMISSION_SYNC    0 // commit always and wait as well

#define DX12_SUBMISSION_MODE   DX12_SUBMISSION_UNBOUND

CCryDX12DeviceContext* CCryDX12DeviceContext::Create(CCryDX12Device* pDeviced)
{
    return DX12::PassAddRef(new CCryDX12DeviceContext(pDeviced));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12DeviceContext::CCryDX12DeviceContext(CCryDX12Device* pDevice)
    : Super()
    , m_pDevice(pDevice)
    , m_pDX12Device(pDevice->GetDX12Device())
    , m_CmdFenceSet(pDevice->GetDX12Device())
    , m_DirectListPool(pDevice->GetDX12Device(), m_CmdFenceSet, CMDQUEUE_GRAPHICS)
    , m_CopyListPool(pDevice->GetDX12Device(), m_CmdFenceSet, CMDQUEUE_COPY)
    , m_TimestampHeap(pDevice->GetDX12Device())
    , m_PipelineHeap(pDevice->GetDX12Device())
    , m_OcclusionHeap(pDevice->GetDX12Device())
    , m_CurrentRootSignature{}
    , m_CurrentPSO{}
    , m_OutstandingQueries{}
    , m_TimestampIndex{}
    , m_OcclusionIndex{}
    , m_bInCopyRegion{}
{
    DX12_FUNC_LOG

    // Timer query heap
    {
        D3D12_QUERY_HEAP_DESC desc = {};
        desc.Count = 1024;
        desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;

        m_TimestampHeap.Init(m_pDX12Device, desc);
    }

    // Occlusion query heap
    {
        D3D12_QUERY_HEAP_DESC desc = {};
        desc.Count = 64;
        desc.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;

        m_OcclusionHeap.Init(m_pDX12Device, desc);
    }

    // Pipeline query heap
    {
        D3D12_QUERY_HEAP_DESC desc = {};
        desc.Count = 16;
        desc.Type = D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;

        m_PipelineHeap.Init(m_pDX12Device, desc);
    }

    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_READBACK);
    const CD3DX12_RESOURCE_DESC timestampHeapBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT64) * m_TimestampHeap.GetCapacity());
    if(S_OK != m_pDX12Device->GetD3D12Device()->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &timestampHeapBuffer,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(&m_TimestampDownloadBuffer)))
    {
        DX12_ERROR("Could not create intermediate timestamp download buffer!");
    }

    const CD3DX12_RESOURCE_DESC occlusionHeapBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT64) * m_OcclusionHeap.GetCapacity());
    if (S_OK != m_pDX12Device->GetD3D12Device()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &occlusionHeapBuffer,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(&m_OcclusionDownloadBuffer)))
    {
        DX12_ERROR("Could not create intermediate occlusion download buffer!");
    }

    m_pSamplerHeap = NULL;
    m_pResourceHeap = NULL;
    m_bCurrentNative = false;
    m_nResDynOffset = 0;
    m_nFrame = 0;
    m_nStencilRef = -1;

    m_TimestampMemory = nullptr;
    m_OcclusionMemory = nullptr;
    m_TimestampMapValid = false;
    m_OcclusionMapValid = false;

    m_CmdFenceSet.Init();

    m_DirectListPool.Init();
    m_CopyListPool.Init(D3D12_COMMAND_LIST_TYPE_COPY);

    m_DirectListPool.AcquireCommandList(m_DirectCommandList);
    m_CopyListPool.AcquireCommandList(m_CopyCommandList);

    m_DirectCommandList->Begin();
    m_DirectCommandList->SetResourceAndSamplerStateHeaps();

    m_CopyCommandList->Begin();

    m_pDX12Device->CalibrateClocks(m_DirectCommandList->GetD3D12CommandQueue());
}

CCryDX12DeviceContext::~CCryDX12DeviceContext()
{
    DX12_FUNC_LOG

    D3D12_RANGE sNoWrite = { 0, 0 };
    if (m_TimestampMemory)
    {
        m_TimestampDownloadBuffer->Unmap(0, &sNoWrite);
    }
    if (m_OcclusionMemory)
    {
        m_OcclusionDownloadBuffer->Unmap(0, &sNoWrite);
    }

    m_TimestampDownloadBuffer->Release();
    m_OcclusionDownloadBuffer->Release();
}

bool CCryDX12DeviceContext::PreparePSO(DX12::CommandMode commandMode)
{
    DX12::Device* device = m_pDevice->GetDX12Device();

    auto& state = m_PipelineState[commandMode];
    const DX12::RootSignature* newRootSignature = m_CurrentRootSignature[commandMode];
    const DX12::PipelineState* newPipelineState = m_CurrentPSO;

    bool bForceFlush = false;

    if (state.m_StateFlags & EPSPB_PipelineState)
    {
        if (commandMode == DX12::CommandModeGraphics)
        {
            DX12::RootSignature::GraphicsInitParams rootSignatureParams;
            state.MakeInitParams(rootSignatureParams);
            newRootSignature = device->GetRootSignatureCache().AcquireRootSignature(rootSignatureParams);

            DX12::GraphicsPipelineState::InitParams psoParams;
            psoParams.rootSignature = newRootSignature;
            state.MakeInitParams(psoParams);
            newPipelineState = device->GetPSOCache().AcquirePipelineState(psoParams);
        }
        else
        {
            DX12::RootSignature::ComputeInitParams rootSignatureParams;
            state.MakeInitParams(rootSignatureParams);
            newRootSignature = device->GetRootSignatureCache().AcquireRootSignature(rootSignatureParams);

            DX12::ComputePipelineState::InitParams psoParams;
            psoParams.rootSignature = newRootSignature;
            state.MakeInitParams(psoParams);
            newPipelineState = device->GetPSOCache().AcquirePipelineState(psoParams);
        }

        if (!newPipelineState)
        {
            return false;
        }

        if constexpr (DX12_SUBMISSION_MODE == DX12_SUBMISSION_PERPSO)
        {
            if (m_OutstandingQueries == 0)
            {
                bForceFlush = true;
            }
        }
    }

    // check for overflow and submit early
    if (bForceFlush || m_DirectCommandList->IsFull(
        /* 256, */ newRootSignature->GetPipelineLayout().m_NumDescriptors,
        /*  16, */ newRootSignature->GetPipelineLayout().m_NumDynamicSamplers,
        /*   8, */ state.OutputMerger.NumRenderTargets.Get(),
        /*   1, */ state.OutputMerger.DepthStencilView ? 1 : 0))
    {
        const bool bWait = false;
        SubmitDirectCommands(bWait);

#ifdef DX12_STATS
        m_NumCommandListOverflows++;
#endif // DX12_STATS

        ResetCachedState();
    }

    if (state.m_StateFlags & EPSPB_PipelineState)
    {
        if (newPipelineState != m_CurrentPSO)
        {
#ifdef DX12_STATS
            m_NumPSOs += 1;
#endif
            m_DirectCommandList->SetPipelineState(newPipelineState);
            m_CurrentPSO = newPipelineState;
        }

        if (newRootSignature != m_CurrentRootSignature[commandMode])
        {
#ifdef DX12_STATS
            m_NumRootSignatures += 1;
#endif
            m_DirectCommandList->SetRootSignature(commandMode, newRootSignature);
            m_CurrentRootSignature[commandMode] = newRootSignature;
            state.m_StateFlags |= EPSPB_InputResources;
        }
    }

    return true;
}

void CCryDX12DeviceContext::PrepareGraphicsFF()
{
    auto& state = m_PipelineState[DX12::CommandModeGraphics];
    const UINT& stateFlags = state.m_StateFlags;

    if (stateFlags & EPSPB_IndexBuffer & (state.InputAssembler.IndexBuffer ? ~0 : 0))
    {
        AZ_Assert(state.InputAssembler.IndexBuffer, "IndexBuffer is required by the Draw but has not been set!");
        m_DirectCommandList->BindAndSetIndexBufferView(state.InputAssembler.IndexBuffer->GetDX12View(), state.InputAssembler.IndexBufferFormat.Get(), state.InputAssembler.IndexBufferOffset.Get());
    }

    if (stateFlags & EPSPB_PrimitiveTopology)
    {
        m_DirectCommandList->SetPrimitiveTopology(static_cast<D3D12_PRIMITIVE_TOPOLOGY> (state.InputAssembler.PrimitiveTopology.Get()));
    }

    if (stateFlags & EPSPB_VertexBuffers)
    {
        m_DirectCommandList->ClearVertexBufferHeap(state.InputAssembler.NumVertexBuffers.Get());

        for (UINT i = 0, S = state.InputAssembler.NumVertexBuffers.Get(); i < S; ++i)
        {
            CCryDX12Buffer* buffer = state.InputAssembler.VertexBuffers.Get(i);

            if (buffer)
            {
                TRange<UINT> bindRange = TRange<UINT> (state.InputAssembler.Offsets.Get(i), state.InputAssembler.Offsets.Get(i));
                UINT bindStride = state.InputAssembler.Strides.Get(i);
                AZ_Assert(buffer, "VertexBuffer is required by the PSO but has not been set!");
                m_DirectCommandList->BindVertexBufferView(buffer->GetDX12View(), i, bindRange, bindStride);
            }
        }

        m_DirectCommandList->SetVertexBufferHeap(state.InputAssembler.NumVertexBuffers.Get());
    }

    if (stateFlags & EPSPB_Viewports)
    {
        UINT numScissors = state.Rasterizer.ScissorEnabled.Get() ? state.Rasterizer.NumScissors.Get() : 0;
        if (state.Rasterizer.NumViewports.Get() >= numScissors)
        {
            D3D12_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
            D3D12_RECT scissors[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];

            for (UINT i = 0, S = state.Rasterizer.NumViewports.Get(); i < S; ++i)
            {
                const D3D11_VIEWPORT& v = state.Rasterizer.Viewports.Get(i);
                viewports[i].TopLeftX = v.TopLeftX;
                viewports[i].TopLeftY = v.TopLeftY;
                viewports[i].Width = v.Width;
                viewports[i].Height = v.Height;
                viewports[i].MinDepth = v.MinDepth;
                viewports[i].MaxDepth = v.MaxDepth;

                if (i < numScissors)
                {
                    const D3D11_RECT& s = state.Rasterizer.Scissors.Get(i);

                    scissors[i].bottom = s.bottom;
                    scissors[i].left = s.left;
                    scissors[i].right = s.right;
                    scissors[i].top = s.top;
                }
                else
                {
                    scissors[i].top = static_cast<LONG>(v.TopLeftY);
                    scissors[i].left = static_cast<LONG>(v.TopLeftX);
                    scissors[i].right = static_cast<LONG>(v.TopLeftX + v.Width);
                    scissors[i].bottom = static_cast<LONG>(v.TopLeftY + v.Height);
                }
            }

            m_DirectCommandList->SetViewports(state.Rasterizer.NumViewports.Get(), viewports);
            m_DirectCommandList->SetScissorRects(state.Rasterizer.NumViewports.Get(), scissors);
        }
        else
        {
            // This should not happen, ever!
            DX12_NOT_IMPLEMENTED;
        }
    }

    if (stateFlags & EPSPB_StencilRef)
    {
        m_DirectCommandList->SetStencilRef(state.OutputMerger.StencilRef.Get());
    }

    if (stateFlags & EPSPB_OutputResources)
    {
        BindOutputViews();
    }
}

bool CCryDX12DeviceContext::PrepareGraphicsState()
{
    if (!m_PipelineState[DX12::CommandModeGraphics].AreShadersBound() || m_PipelineState[DX12::CommandModeGraphics].InputAssembler.InputLayout.m_Value == NULL)
    {
        return false;
    }

    if (!PreparePSO(DX12::CommandModeGraphics))
    {
        return false;
    }

    PrepareGraphicsFF();
    BindResources(DX12::CommandModeGraphics);

    m_PipelineState[DX12::CommandModeGraphics].m_StateFlags = 0;
    return true;
}

bool CCryDX12DeviceContext::PrepareComputeState()
{
    if (!m_PipelineState[DX12::CommandModeCompute].AreShadersBound())
    {
        return false;
    }

    if (!PreparePSO(DX12::CommandModeCompute))
    {
        return false;
    }

    BindResources(DX12::CommandModeCompute);

    m_PipelineState[DX12::CommandModeCompute].m_StateFlags = 0;
    return true;
}

void CCryDX12DeviceContext::BindResources(DX12::CommandMode commandMode)
{
    const auto& state = m_PipelineState[commandMode];
    const auto stateFlags = state.m_StateFlags;

    if ((stateFlags & EPSPB_InputResources) == 0)
    {
        return;
    }
    DX12_ASSERT(stateFlags & (BIT(EPSP_ConstantBuffers) | BIT(EPSP_Resources) | BIT(EPSP_Samplers)), "Redundant BindResources() called without state-changes");

    const DX12::PipelineLayout& layout = m_CurrentRootSignature[commandMode]->GetPipelineLayout();
    AZ::u32 resourceDescriptorOffset = 0;
    AZ::u32 samplerDescriptorOffset = 0;

    if (stateFlags & (EPSPB_ConstantBuffers | EPSPB_Resources))
    {
        for (const DX12::ResourceLayoutBinding& resourceBinding : layout.m_TableResources)
        {
            auto& stage = state.Stages[resourceBinding.ShaderStage];

            switch (resourceBinding.ViewType)
            {
            case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
            {
                CCryDX12Buffer* buffer = stage.ConstantBufferViews.Get(resourceBinding.ShaderSlot);
                TRange<UINT> bindRange = stage.ConstBufferBindRange.Get(resourceBinding.ShaderSlot);
                DX12_ASSERT(resourceDescriptorOffset == resourceBinding.DescriptorOffset, "ConstantBuffer offset has shifted: resource mapping invalid!");
                m_DirectCommandList->WriteConstantBufferDescriptor(buffer ? &buffer->GetDX12View() : nullptr, resourceDescriptorOffset++, bindRange.start, bindRange.Length());
                break;
            }
            case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
            {
                CCryDX12ShaderResourceView* resource = stage.ShaderResourceViews.Get(resourceBinding.ShaderSlot);
                DX12_ASSERT(resourceDescriptorOffset == resourceBinding.DescriptorOffset, "ShaderResourceView offset has shifted: resource mapping invalid!");
                m_DirectCommandList->WriteShaderResourceDescriptor(resource ? &resource->GetDX12View() : nullptr, resourceDescriptorOffset++);
                break;
            }
            case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
            {
                CCryDX12UnorderedAccessView* resource = stage.UnorderedAccessViews.Get(resourceBinding.ShaderSlot);
                DX12_ASSERT(resourceDescriptorOffset == resourceBinding.DescriptorOffset, "UnorderedAccessView offset has shifted: resource mapping invalid!");
                m_DirectCommandList->WriteUnorderedAccessDescriptor(resource ? &resource->GetDX12View() : nullptr, resourceDescriptorOffset++);
                break;
            }
            }
        }

        if (stateFlags & EPSPB_ConstantBuffers)
        {
            for (const DX12::ConstantBufferLayoutBinding& constantBufferBinding : layout.m_ConstantViews)
            {
                auto& stage = state.Stages[constantBufferBinding.ShaderStage];

                CCryDX12Buffer* buffer = stage.ConstantBufferViews.Get(constantBufferBinding.ShaderSlot);
                TRange<UINT> bindRange = stage.ConstBufferBindRange.Get(constantBufferBinding.ShaderSlot);

                D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = { 0ull };
                if (buffer)
                {
                    gpuAddress = buffer->GetDX12View().GetCBVDesc().BufferLocation + bindRange.start;
                }
                m_DirectCommandList->SetConstantBufferView(commandMode, constantBufferBinding.RootParameterIndex, gpuAddress);
            }
        }
    }

    // Bind samplers
    if (stateFlags & EPSPB_Samplers)
    {
        for (const DX12::ResourceLayoutBinding& samplerBinding : layout.m_Samplers)
        {
            const DX12::EShaderStage i = samplerBinding.ShaderStage;
            DX12_ASSERT(samplerBinding.ViewType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, "");
            {
                CCryDX12SamplerState* sampler = state.Stages[i].SamplerState.Get(samplerBinding.ShaderSlot);
                DX12_ASSERT(samplerDescriptorOffset == samplerBinding.DescriptorOffset, "Sampler offset has shifted: resource mapping invalid!");
                m_DirectCommandList->WriteSamplerStateDescriptor(sampler ? &sampler->GetDX12SamplerState() : nullptr, samplerDescriptorOffset++);
            }
        }
    }

    if (stateFlags & (BIT(EPSP_ConstantBuffers) | BIT(EPSP_Resources)))
    {
        m_DirectCommandList->SetDescriptorTables(commandMode, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_DirectCommandList->IncrementInputCursors(layout.m_NumDescriptors, 0);
    }

    if (stateFlags & (BIT(EPSP_Samplers)))
    {
        m_DirectCommandList->SetDescriptorTables(commandMode, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        m_DirectCommandList->IncrementInputCursors(0, layout.m_NumDynamicSamplers);
    }
}

void CCryDX12DeviceContext::BindOutputViews()
{
    const DX12::ResourceView* dsv = NULL;
    const DX12::ResourceView* rtv[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
    size_t numRTVs = 0;

    auto& outputMerger = m_PipelineState[DX12::CommandModeGraphics].OutputMerger;

    // Get current depth stencil views
    {
        CCryDX12DepthStencilView* view = outputMerger.DepthStencilView;
        if (view)
        {
            dsv = &view->GetDX12View();
        }
    }

    // Get current render target views
    for (UINT i = 0, S = outputMerger.NumRenderTargets.Get(); i < S; ++i)
    {
        CCryDX12RenderTargetView* view = outputMerger.RenderTargetViews.Get(i);
        if (view)
        {
            rtv[numRTVs++] = &view->GetDX12View();
        }
    }

    m_DirectCommandList->BindAndSetOutputViews(numRTVs, rtv, dsv);
#ifdef DX12_STATS
    m_NumberSettingOutputViews++;
#endif
}

namespace DX12
{
    const char* StateToString(D3D12_RESOURCE_STATES state);
}

void CCryDX12DeviceContext::DebugPrintResources(DX12::CommandMode commandMode)
{
    const DX12::PipelineLayout& layout = m_CurrentRootSignature[commandMode]->GetPipelineLayout();

    DX12_LOG("Resource Heap Descriptor Tables:");

    auto& state = m_PipelineState[commandMode];
#ifdef GFX_DEBUG
    UINT resourceDescriptorOffset = 0;
    UINT samplerDescriptorOffset = 0;
#endif

    // Bind constant buffers
    for (const DX12::ResourceLayoutBinding& resourceBinding : layout.m_TableResources)
    {
        const DX12::EShaderStage i = resourceBinding.ShaderStage;
        if (resourceBinding.ViewType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
        {
            auto bindRange = state.Stages[i].ConstBufferBindRange.Get(resourceBinding.ShaderSlot);
#ifdef GFX_DEBUG
            CCryDX12Buffer* buffer = state.Stages[i].ConstantBufferViews.Get(resourceBinding.ShaderSlot);
            AZ_Assert(buffer, "ConstantBuffer is required by the PSO but has not been set!");
            AZ_Assert(resourceDescriptorOffset == resourceBinding.DescriptorOffset, "ConstantBuffer offset has shifted: resource mapping invalid!");
#endif
            DX12_LOG(" %s: C %2d -> %2d %s [%s]",
                i == DX12::ESS_Compute  ? "Every    shader stage" :
                i == DX12::ESS_Vertex   ? "Vertex   shader stage" :
                i == DX12::ESS_Hull     ? "Hull     shader stage" :
                i == DX12::ESS_Domain   ? "Domain   shader stage" :
                i == DX12::ESS_Geometry ? "Geometry shader stage" :
                i == DX12::ESS_Pixel    ? "Pixel    shader stage" : "Unknown  shader stage",
                resourceBinding.ShaderSlot,
                resourceDescriptorOffset++,
                buffer ? buffer->GetName().c_str() : "nullptr",
                buffer ? DX12::StateToString(buffer->GetDX12Resource().GetCurrentState()) : "-");
        }
        else if (resourceBinding.ViewType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV)
        {
#ifdef GFX_DEBUG
            CCryDX12ShaderResourceView* resource = state.Stages[i].ShaderResourceViews.Get(resourceBinding.ShaderSlot);
            AZ_Assert(resource, "ShaderResourceView is required by the PSO but has not been set!");
            AZ_Assert(resourceDescriptorOffset == resourceBinding.DescriptorOffset, "ShaderResourceView offset has shifted: resource mapping invalid!");
#endif
            DX12_LOG(" %s: T %2d -> %2d %s [%s]",
                i == DX12::ESS_Compute  ? "Every    shader stage" :
                i == DX12::ESS_Vertex   ? "Vertex   shader stage" :
                i == DX12::ESS_Hull     ? "Hull     shader stage" :
                i == DX12::ESS_Domain   ? "Domain   shader stage" :
                i == DX12::ESS_Geometry ? "Geometry shader stage" :
                i == DX12::ESS_Pixel    ? "Pixel    shader stage" : "Unknown  shader stage",
                resourceBinding.ShaderSlot,
                resourceDescriptorOffset++,
                resource ? resource->GetResourceName().c_str() : "nullptr",
                resource ? DX12::StateToString(resource->GetDX12Resource().GetCurrentState()) : "-");
        }
        else if (resourceBinding.ViewType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
        {
#ifdef GFX_DEBUG
            CCryDX12UnorderedAccessView* resource = state.Stages[i].UnorderedAccessViews.Get(resourceBinding.ShaderSlot);
            AZ_Assert(resource, "UnorderedAccessView is required by the PSO but has not been set!");
            AZ_Assert(resourceDescriptorOffset == resourceBinding.DescriptorOffset, "UnorderedAccessView offset has shifted: resource mapping invalid!");
#endif
            DX12_LOG(" %s: U %2d -> %2d %s [%s]",
                i == DX12::ESS_Compute  ? "Every    shader stage" :
                i == DX12::ESS_Vertex   ? "Vertex   shader stage" :
                i == DX12::ESS_Hull     ? "Hull     shader stage" :
                i == DX12::ESS_Domain   ? "Domain   shader stage" :
                i == DX12::ESS_Geometry ? "Geometry shader stage" :
                i == DX12::ESS_Pixel    ? "Pixel    shader stage" : "Unknown  shader stage",
                resourceBinding.ShaderSlot,
                resourceDescriptorOffset++,
                resource ? resource->GetResourceName().c_str() : "nullptr",
                resource ? DX12::StateToString(resource->GetDX12Resource().GetCurrentState()) : "-");
        }
    }

    // Bind samplers
    for (const DX12::ResourceLayoutBinding& samplerBinding : layout.m_Samplers)
    {
        const DX12::EShaderStage i = samplerBinding.ShaderStage;
        AZ_Assert(samplerBinding.ViewType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, "");
        {
#ifdef GFX_DEBUG
            CCryDX12SamplerState* sampler = state.Stages[i].SamplerState.Get(samplerBinding.ShaderSlot);
            AZ_Assert(sampler, "Sampler is required by the PSO but has not been set!");
            AZ_Assert(samplerDescriptorOffset == samplerBinding.DescriptorOffset, "Sampler offset has shifted: resource mapping invalid!");
#endif
            DX12_LOG(" %s: S %2d -> %2d",
                i == DX12::ESS_Compute  ? "Every    shader stage" :
                i == DX12::ESS_Vertex   ? "Vertex   shader stage" :
                i == DX12::ESS_Hull     ? "Hull     shader stage" :
                i == DX12::ESS_Domain   ? "Domain   shader stage" :
                i == DX12::ESS_Geometry ? "Geometry shader stage" :
                i == DX12::ESS_Pixel    ? "Pixel    shader stage" : "Unknown  shader stage",
                samplerBinding.ShaderSlot,
                samplerDescriptorOffset++);
        }
    }
}

void CCryDX12DeviceContext::CeaseDirectCommandQueue(bool wait)
{
    AZ_TRACE_METHOD();
    AZ_Assert(m_DirectCommandList != nullptr, "CommandList hasn't been allocated!");
    AZ_Assert(m_OutstandingQueries == 0, "Flushing command list with outstanding queries!");

    m_DirectCommandList->End();
    m_DirectListPool.ForfeitCommandList(m_DirectCommandList, wait);
}

void CCryDX12DeviceContext::ResumeDirectCommandQueue()
{
    AZ_TRACE_METHOD();
    AZ_Assert(m_DirectCommandList == nullptr, "CommandList hasn't been submitted!");
    m_DirectListPool.AcquireCommandList(m_DirectCommandList);

    m_DirectCommandList->Begin();
    m_DirectCommandList->SetResourceAndSamplerStateHeaps();

    ResetCachedState();
}

void CCryDX12DeviceContext::CeaseCopyCommandQueue(bool wait)
{
    AZ_TRACE_METHOD();
    AZ_Assert(m_CopyCommandList != nullptr, "CommandList hasn't been allocated!");

    m_CopyCommandList->End();
    m_CopyListPool.ForfeitCommandList(m_CopyCommandList, wait);
}

void CCryDX12DeviceContext::ResumeCopyCommandQueue()
{
    AZ_TRACE_METHOD();
    AZ_Assert(m_CopyCommandList == nullptr, "CommandList hasn't been submitted!");
    m_CopyListPool.AcquireCommandList(m_CopyCommandList);

    m_CopyCommandList->Begin();
}

void CCryDX12DeviceContext::CeaseAllCommandQueues(bool wait)
{
    AZ_TRACE_METHOD();
    CeaseDirectCommandQueue(wait);
    CeaseCopyCommandQueue(wait);
}

void CCryDX12DeviceContext::ResumeAllCommandQueues()
{
    AZ_TRACE_METHOD();
    ResumeDirectCommandQueue();
    ResumeCopyCommandQueue();
}

void CCryDX12DeviceContext::SubmitDirectCommands(bool wait)
{
    if (m_DirectCommandList->IsUtilized())
    {
        AZ_TRACE_METHOD();
        CeaseDirectCommandQueue(wait);
        ResumeDirectCommandQueue();
    }
}

void CCryDX12DeviceContext::SubmitCopyCommands(bool wait)
{
    if (m_CopyCommandList->IsUtilized())
    {
        AZ_TRACE_METHOD();
        CeaseCopyCommandQueue(wait);
        ResumeCopyCommandQueue();
    }
}

void CCryDX12DeviceContext::SubmitDirectCommands(bool wait, const UINT64 fenceValue)
{
    if (m_DirectCommandList->IsUtilized() && (m_CmdFenceSet.GetCurrentValue(CMDQUEUE_GRAPHICS) == fenceValue))
    {
#ifdef DX12_STATS
        m_NumCommandListSplits++;
#endif // DX12_STATS

        SubmitDirectCommands(wait);
    }
}

void CCryDX12DeviceContext::SubmitCopyCommands(bool wait, const UINT64 fenceValue)
{
    if (m_CopyCommandList->IsUtilized() && (m_CmdFenceSet.GetCurrentValue(CMDQUEUE_COPY) == fenceValue))
    {
#ifdef DX12_STATS
        m_NumCommandListSplits++;
#endif // DX12_STATS

        SubmitCopyCommands(wait);
    }
}

void CCryDX12DeviceContext::SubmitAllCommands(bool bWaitForGpu)
{
    SubmitAllCommands(bWaitForGpu, m_CmdFenceSet.GetCurrentValues());
}

void CCryDX12DeviceContext::SubmitAllCommands(bool wait, const UINT64 (&fenceValues)[CMDQUEUE_NUM])
{
    SubmitDirectCommands(wait, fenceValues[CMDQUEUE_GRAPHICS]);
    SubmitCopyCommands(wait, fenceValues[CMDQUEUE_COPY]);
}


void CCryDX12DeviceContext::SubmitAllCommands(bool wait, const UINT64(&fenceValues)[CMDQUEUE_NUM], int fenceId)
{
    AZ::u64 fenceValuesMasked[CMDQUEUE_NUM];
    fenceValuesMasked[CMDQUEUE_GRAPHICS] = fenceValues[CMDQUEUE_GRAPHICS];
    fenceValuesMasked[CMDQUEUE_COPY] = fenceValues[CMDQUEUE_COPY];
    fenceValuesMasked[fenceId] = 0;

    SubmitDirectCommands(wait, fenceValuesMasked[CMDQUEUE_GRAPHICS]);
    SubmitCopyCommands(wait, fenceValuesMasked[CMDQUEUE_COPY]);
}

void CCryDX12DeviceContext::SubmitAllCommands(bool wait, const std::atomic<AZ::u64>(&fenceValues)[CMDQUEUE_NUM])
{
    SubmitDirectCommands(wait, fenceValues[CMDQUEUE_GRAPHICS]);
    SubmitCopyCommands(wait, fenceValues[CMDQUEUE_COPY]);
}

void CCryDX12DeviceContext::SubmitAllCommands(bool wait, const std::atomic<AZ::u64>(&fenceValues)[CMDQUEUE_NUM], int fenceId)
{
    AZ::u64 fenceValuesMasked[CMDQUEUE_NUM];
    fenceValuesMasked[CMDQUEUE_GRAPHICS] = fenceValues[CMDQUEUE_GRAPHICS];
    fenceValuesMasked[CMDQUEUE_COPY] = fenceValues[CMDQUEUE_COPY];
    fenceValuesMasked[fenceId] = 0;

    SubmitDirectCommands(wait, fenceValuesMasked[CMDQUEUE_GRAPHICS]);
    SubmitCopyCommands(wait, fenceValuesMasked[CMDQUEUE_COPY]);
}

void CCryDX12DeviceContext::Finish(DX12::SwapChain* pDX12SwapChain)
{
    AZ_TRACE_METHOD();
    m_DirectCommandList->PresentRenderTargetView(pDX12SwapChain);

    SubmitAllCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC, m_CmdFenceSet.GetCurrentValues());

    // Release resource after pass fence and FRAME_FENCE_LATENCY later
    m_pDX12Device->FlushReleaseHeap(DX12::Device::ResourceReleasePolicy::Deferred);

    m_pDX12Device->FinishFrame();
    m_pDX12Device->CalibrateClocks(m_DirectListPool.GetD3D12CommandQueue());

#ifdef DX12_STATS
    m_NumMapDiscardSkips = 0;
    m_NumMapDiscards = 0;

    m_NumCommandListOverflows = 0;
    m_NumCommandListSplits = 0;
    m_NumPSOs = 0;
    m_NumRootSignatures = 0;
    m_NumberSettingOutputViews = 0;
#endif // DX12_STATS
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UINT CCryDX12DeviceContext::TimestampIndex([[maybe_unused]] DX12::CommandList* pCmdList)
{
    UINT index = m_TimestampIndex;
    m_TimestampIndex = (m_TimestampIndex + 1) % m_TimestampHeap.GetCapacity();
    return index;
}

ID3D12Resource* CCryDX12DeviceContext::QueryTimestamp(DX12::CommandList* pCmdList, UINT index)
{
    pCmdList->EndQuery(m_TimestampHeap, D3D12_QUERY_TYPE_TIMESTAMP, index);

    m_TimestampMapValid = false;
    if (m_TimestampMemory)
    {
        const D3D12_RANGE sNoWrite = { 0, 0 };
        m_TimestampDownloadBuffer->Unmap(0, &sNoWrite);
        m_TimestampMemory = nullptr;
    }

    return m_TimestampDownloadBuffer;
}

void CCryDX12DeviceContext::ResolveTimestamp(DX12::CommandList* pCmdList, UINT index, void* mem)
{
    if (mem)
    {
        if (!m_TimestampMapValid)
        {
            if (m_TimestampMemory)
            {
                const D3D12_RANGE sNoWrite = { 0, 0 };
                m_TimestampDownloadBuffer->Unmap(0, &sNoWrite);
                m_TimestampMemory = nullptr;
            }

            pCmdList->ResolveQueryData(m_TimestampHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, m_TimestampHeap.GetCapacity(), m_TimestampDownloadBuffer, 0);

            // Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
            const D3D12_RANGE sFullRead = { 0, sizeof(UINT64) * m_TimestampHeap.GetCapacity() };
            m_TimestampDownloadBuffer->Map(0, &sFullRead, &m_TimestampMemory);
            m_TimestampMapValid = true;
        }

        memcpy(mem, (char*)m_TimestampMemory + index * 8, 8);
    }
}

UINT64 CCryDX12DeviceContext::MakeCpuTimestamp(UINT64 gpuTimestamp) const
{
    return m_pDX12Device->MakeCpuTimestamp(gpuTimestamp);
}

UINT64 CCryDX12DeviceContext::MakeCpuTimestampMicroseconds(UINT64 gpuTimestamp) const
{
    return m_pDX12Device->MakeCpuTimestampMicroseconds(gpuTimestamp);
}

UINT CCryDX12DeviceContext::OcclusionIndex(DX12::CommandList* pCmdList, bool counter)
{
    UINT index = m_OcclusionIndex;
    m_OcclusionIndex = (m_OcclusionIndex + 1) % m_OcclusionHeap.GetCapacity();
    pCmdList->BeginQuery(m_OcclusionHeap, counter ? D3D12_QUERY_TYPE_OCCLUSION : D3D12_QUERY_TYPE_BINARY_OCCLUSION, index);
    return index;
}

ID3D12Resource* CCryDX12DeviceContext::QueryOcclusion(DX12::CommandList* pCmdList, UINT index, bool counter)
{
    pCmdList->EndQuery(m_OcclusionHeap, counter ? D3D12_QUERY_TYPE_OCCLUSION : D3D12_QUERY_TYPE_BINARY_OCCLUSION, index);

    m_OcclusionMapValid = false;
    if (m_OcclusionMemory)
    {
        const D3D12_RANGE sNoWrite = { 0, 0 };
        m_OcclusionDownloadBuffer->Unmap(0, &sNoWrite);
        m_OcclusionMemory = nullptr;
    }

    return m_OcclusionDownloadBuffer;
}

void CCryDX12DeviceContext::ResolveOcclusion(DX12::CommandList* pCmdList, UINT index, void* mem)
{
    if (mem)
    {
        if (!m_OcclusionMapValid)
        {
            if (m_OcclusionMemory)
            {
                const D3D12_RANGE sNoWrite = { 0, 0 };
                m_OcclusionDownloadBuffer->Unmap(0, &sNoWrite);
                m_OcclusionMemory = nullptr;
            }

            pCmdList->ResolveQueryData(m_OcclusionHeap, D3D12_QUERY_TYPE_OCCLUSION, 0, m_OcclusionHeap.GetCapacity(), m_OcclusionDownloadBuffer, 0);

            // Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
            const D3D12_RANGE sFullRead = { 0, sizeof(UINT64) * m_OcclusionHeap.GetCapacity() };
            m_OcclusionDownloadBuffer->Map(0, &sFullRead, &m_OcclusionMemory);
            m_OcclusionMapValid = true;
        }

        memcpy(mem, (char*)m_OcclusionMemory + index * 8, 8);
    }
}

void CCryDX12DeviceContext::WaitForIdle()
{
    DX12::CommandListFence fence(m_pDX12Device);
    fence.Init();

    m_DirectListPool.GetD3D12CommandQueue()->Signal(fence.GetFence(), 1);

    fence.WaitForFence(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region /* ID3D11DeviceChild implementation */

void STDMETHODCALLTYPE CCryDX12DeviceContext::GetDevice(
    [[maybe_unused]] _Out_  ID3D11Device** ppDevice)
{
    DX12_FUNC_LOG
}

HRESULT STDMETHODCALLTYPE CCryDX12DeviceContext::GetPrivateData(
    [[maybe_unused]] _In_  REFGUID guid,
    [[maybe_unused]] _Inout_  UINT* pDataSize,
    [[maybe_unused]] _Out_writes_bytes_opt_(*pDataSize)  void* pData)
{
    DX12_FUNC_LOG
    return -1;
}

HRESULT STDMETHODCALLTYPE CCryDX12DeviceContext::SetPrivateData(
    [[maybe_unused]] _In_  REFGUID guid,
    [[maybe_unused]] _In_  UINT DataSize,
    [[maybe_unused]] _In_reads_bytes_opt_(DataSize)  const void* pData)
{
    DX12_FUNC_LOG
    return -1;
}

HRESULT STDMETHODCALLTYPE CCryDX12DeviceContext::SetPrivateDataInterface(
    [[maybe_unused]] _In_  REFGUID guid,
    [[maybe_unused]] _In_opt_  const IUnknown* pData)
{
    DX12_FUNC_LOG
    return -1;
}

#pragma endregion

#pragma region /* ID3D11DeviceContext implementation */

void STDMETHODCALLTYPE CCryDX12DeviceContext::VSSetConstantBuffers(
    _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_(NumBuffers)  ID3D11Buffer* const* ppConstantBuffers)
{
    VSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, NULL, NULL);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::PSSetShaderResources(
    _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    _In_reads_opt_(NumViews)  ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
    DX12_FUNC_LOG
    for (UINT i = StartSlot, S = StartSlot + NumViews; i < S; ++i, ++ppShaderResourceViews)
    {
        CCryDX12ShaderResourceView* srv = reinterpret_cast<CCryDX12ShaderResourceView*>(*ppShaderResourceViews);
        if (m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Pixel].ShaderResourceViews.Set(i, srv) && srv)
        {
            srv->BeginResourceStateTransition(m_DirectCommandList.get());
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::PSSetShader(
    _In_opt_  ID3D11PixelShader* pPixelShader,
    [[maybe_unused]] _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance* const* ppClassInstances,
    [[maybe_unused]] UINT NumClassInstances)
{
    DX12_FUNC_LOG
    m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Pixel].Shader.Set(reinterpret_cast<CCryDX12Shader*>(pPixelShader));
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::PSSetSamplers(
    _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    _In_reads_opt_(NumSamplers)  ID3D11SamplerState* const* ppSamplers)
{
    DX12_FUNC_LOG
    for (UINT i = StartSlot, S = StartSlot + NumSamplers; i < S; ++i, ++ppSamplers)
    {
        m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Pixel].SamplerState.Set(i, reinterpret_cast<CCryDX12SamplerState*>(*ppSamplers));
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::VSSetShader(
    _In_opt_  ID3D11VertexShader* pVertexShader,
    [[maybe_unused]] _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance* const* ppClassInstances,
    [[maybe_unused]] UINT NumClassInstances)
{
    DX12_FUNC_LOG
    m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Vertex].Shader.Set(reinterpret_cast<CCryDX12Shader*>(pVertexShader));
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DrawIndexed(
    _In_  UINT IndexCount,
    _In_  UINT StartIndexLocation,
    _In_  INT BaseVertexLocation)
{
    DX12_FUNC_LOG

    SubmitCopyCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);

    if (PrepareGraphicsState())
    {
        m_DirectCommandList->DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
#ifdef AZ_DEBUG_BUILD
        m_PipelineState[DX12::CommandModeGraphics].DebugPrint();
#endif
        if constexpr (DX12_SUBMISSION_MODE <= DX12_SUBMISSION_PERDRAW)
        {
            if (m_OutstandingQueries == 0)
            {
                SubmitDirectCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);
            }
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::Draw(
    _In_  UINT VertexCount,
    _In_  UINT StartVertexLocation)
{
    DX12_FUNC_LOG

    SubmitCopyCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);

    if (PrepareGraphicsState())
    {
        m_DirectCommandList->DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
#ifdef AZ_DEBUG_BUILD
        m_PipelineState[DX12::CommandModeGraphics].DebugPrint();
#endif
        if constexpr (DX12_SUBMISSION_MODE <= DX12_SUBMISSION_PERDRAW)
        {
            if (m_OutstandingQueries == 0)
            {
                SubmitDirectCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);
            }
        }
    }
}

HRESULT STDMETHODCALLTYPE CCryDX12DeviceContext::Map(
    _In_  ID3D11Resource* pResource,
    _In_  UINT Subresource,
    _In_  D3D11_MAP MapType,
    [[maybe_unused]] _In_  UINT MapFlags,
    _Out_  D3D11_MAPPED_SUBRESOURCE* pMappedResource)
{
    DX12_FUNC_LOG

    ZeroMemory(pMappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

    DX12_LOG("  Mapping resource: %p (%d)", pResource, Subresource);
    ICryDX12Resource* dx12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pResource);
    DX12::Resource& resource = dx12Resource->GetDX12Resource();

    if (!resource.IsOffCard())
    {
        return S_FALSE;
    }

    switch (MapType)
    {
    case D3D11_MAP_READ:
        // Ensure the command-list using the resource is executed
        SubmitAllCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC, resource.GetFenceValues(CMDTYPE_WRITE));

        // Block the CPU-thread until the resource is safe to map
        resource.WaitForUnused(m_DirectListPool, CMDTYPE_WRITE);
        break;
    case D3D11_MAP_WRITE:
    case D3D11_MAP_READ_WRITE:
        // Ensure the command-list using the resource is executed
        SubmitAllCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC, resource.GetFenceValues(CMDTYPE_ANY));

        // Block the CPU-thread until the resource is safe to map
        resource.WaitForUnused(m_DirectListPool, CMDTYPE_ANY);
        break;
    case D3D11_MAP_WRITE_DISCARD:
        DX12_LOG("Using D3D11_MAP_WRITE_DISCARD on old ID3D12Resource: %p", DX12_EXTRACT_D3D12RESOURCE(pResource));

#ifdef DX12_STATS
        m_NumMapDiscardSkips += !resource.IsUsed(m_DirectListPool);
        m_NumMapDiscards++;
#endif // DX12_STATS

        // If the resource is not currently used, we do not need to discard the memory
        if (resource.IsUsed(m_DirectListPool))
        {
            dx12Resource->MapDiscard(m_DirectCommandList);
        }

        DX12_LOG("New ID3D12Resource: %p", DX12_EXTRACT_D3D12RESOURCE(pResource));
        break;
    case D3D11_MAP_WRITE_NO_OVERWRITE:
        break;
    default:
        break;
    }

    static D3D12_RANGE sRg[] =
    {
        { 0, 0 }, // It is valid to specify the CPU won't read any data by passing a range where End is less than or equal to Begin
        { 0, 0 }  // It is valid to specify the CPU didn't write any data by passing a range where End is less than or equal to Begin.
    };

    static D3D12_RANGE* pRanges[] =
    {
        nullptr, // D3D11_MAP_READ = 1,
        nullptr, // D3D11_MAP_WRITE = 2,
        nullptr, // D3D11_MAP_READ_WRITE = 3,
        &sRg[0], // D3D11_MAP_WRITE_DISCARD = 4,
        &sRg[0], // D3D11_MAP_WRITE_NO_OVERWRITE = 5
    };
    AZ_Assert((MapType - 1) < AZ_ARRAY_SIZE(pRanges), "Invalid map type");

    AZ_Assert(!D3D12IsLayoutOpaque(resource.GetDesc().Layout), "Opaque layouts are unmappable until 12.2!");
    HRESULT hr = dx12Resource->GetD3D12Resource()->Map(Subresource, pRanges[MapType - 1], &(pMappedResource->pData));

    if (S_OK != hr)
    {
        AZ_Assert(0, "Could not map resource!");
        return hr;
    }

    return S_OK;
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::Unmap(
    _In_  ID3D11Resource* pResource,
    _In_  UINT Subresource)
{
    DX12_FUNC_LOG

    DX12_LOG("Unmapping resource: %p (%d)", pResource, Subresource);
    ICryDX12Resource* dx12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pResource);

    // NOTE: Don't know MapType, can't optimize writeRange!

    AZ_Assert(!D3D12IsLayoutOpaque(dx12Resource->GetDX12Resource().GetDesc().Layout), "Opaque layouts are unmappable until 12.2!");
    dx12Resource->GetD3D12Resource()->Unmap(Subresource, NULL);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::PSSetConstantBuffers(
    _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_(NumBuffers)  ID3D11Buffer* const* ppConstantBuffers)
{
    PSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, NULL, NULL);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::IASetInputLayout(
    _In_opt_  ID3D11InputLayout* pInputLayout)
{
    DX12_FUNC_LOG

    m_PipelineState[DX12::CommandModeGraphics].InputAssembler.InputLayout.Set(reinterpret_cast<CCryDX12InputLayout*>(pInputLayout));
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::IASetVertexBuffers(
    _In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_(NumBuffers)  ID3D11Buffer* const* ppVertexBuffers,
    _In_reads_opt_(NumBuffers)  const UINT* pStrides,
    _In_reads_opt_(NumBuffers)  const UINT* pOffsets)
{
    DX12_FUNC_LOG

    auto& inputAssembler = m_PipelineState[DX12::CommandModeGraphics].InputAssembler;
    for (UINT i = StartSlot, S = StartSlot + NumBuffers; i < S; ++i, ++ppVertexBuffers, ++pStrides, ++pOffsets)
    {
        CCryDX12Buffer* vb = reinterpret_cast<CCryDX12Buffer*>(*ppVertexBuffers);

        inputAssembler.Strides.Set(i, *pStrides);
        inputAssembler.Offsets.Set(i, *pOffsets);
        if (inputAssembler.VertexBuffers.Set(i, vb) && vb)
        {
            vb->BeginResourceStateTransition(m_DirectCommandList.get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        }
    }

    UINT numVertexBuffers = 0;
    for (UINT i = 0; i < D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        if (inputAssembler.VertexBuffers.Get(i))
        {
            numVertexBuffers = i + 1;
        }
    }

    inputAssembler.NumVertexBuffers.Set(numVertexBuffers);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::IASetIndexBuffer(
    _In_opt_  ID3D11Buffer* pIndexBuffer,
    _In_  DXGI_FORMAT Format,
    _In_  UINT Offset)
{
    DX12_FUNC_LOG

    auto& inputAssembler = m_PipelineState[DX12::CommandModeGraphics].InputAssembler;
    CCryDX12Buffer* ib = reinterpret_cast<CCryDX12Buffer*>(pIndexBuffer);

    inputAssembler.IndexBufferFormat.Set(Format);
    inputAssembler.IndexBufferOffset.Set(Offset);
    if (inputAssembler.IndexBuffer.Set(ib) && ib)
    {
        ib->BeginResourceStateTransition(m_DirectCommandList.get(), D3D12_RESOURCE_STATE_INDEX_BUFFER);
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DrawIndexedInstanced(
    _In_  UINT IndexCountPerInstance,
    _In_  UINT InstanceCount,
    _In_  UINT StartIndexLocation,
    _In_  INT BaseVertexLocation,
    _In_  UINT StartInstanceLocation)
{
    DX12_FUNC_LOG

    SubmitCopyCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);

    if (PrepareGraphicsState())
    {
        m_DirectCommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
#ifdef AZ_DEBUG_BUILD
        m_PipelineState[DX12::CommandModeGraphics].DebugPrint();
#endif
        if constexpr (DX12_SUBMISSION_MODE <= DX12_SUBMISSION_PERDRAW)
        {
            if (m_OutstandingQueries == 0)
            {
                SubmitDirectCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);
            }
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DrawInstanced(
    _In_  UINT VertexCountPerInstance,
    _In_  UINT InstanceCount,
    _In_  UINT StartVertexLocation,
    _In_  UINT StartInstanceLocation)
{
    DX12_FUNC_LOG

    SubmitCopyCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);

    if (PrepareGraphicsState())
    {
        m_DirectCommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
#ifdef AZ_DEBUG_BUILD
        m_PipelineState[DX12::CommandModeGraphics].DebugPrint();
#endif
        if constexpr (DX12_SUBMISSION_MODE <= DX12_SUBMISSION_PERDRAW)
        {
            if (m_OutstandingQueries == 0)
            {
                SubmitDirectCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);
            }
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::GSSetConstantBuffers(
    _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_(NumBuffers)  ID3D11Buffer* const* ppConstantBuffers)
{
    GSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, NULL, NULL);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::GSSetShader(
    _In_opt_  ID3D11GeometryShader* pShader,
    [[maybe_unused]] _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance* const* ppClassInstances,
    [[maybe_unused]] UINT NumClassInstances)
{
    DX12_FUNC_LOG
    m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Geometry].Shader.Set(reinterpret_cast<CCryDX12Shader*>(pShader));
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::IASetPrimitiveTopology(
    _In_  D3D11_PRIMITIVE_TOPOLOGY Topology)
{
    DX12_FUNC_LOG
    m_PipelineState[DX12::CommandModeGraphics].InputAssembler.PrimitiveTopology.Set(Topology);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::VSSetShaderResources(
    _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    _In_reads_opt_(NumViews)  ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
    DX12_FUNC_LOG
    for (UINT i = StartSlot, S = StartSlot + NumViews; i < S; ++i, ++ppShaderResourceViews)
    {
        CCryDX12ShaderResourceView* srv = reinterpret_cast<CCryDX12ShaderResourceView*>(*ppShaderResourceViews);
        if (m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Vertex].ShaderResourceViews.Set(i, srv) && srv)
        {
            srv->BeginResourceStateTransition(m_DirectCommandList.get());
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::VSSetSamplers(
    _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    _In_reads_opt_(NumSamplers)  ID3D11SamplerState* const* ppSamplers)
{
    DX12_FUNC_LOG
    for (UINT i = StartSlot, S = StartSlot + NumSamplers; i < S; ++i, ++ppSamplers)
    {
        m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Vertex].SamplerState.Set(i, reinterpret_cast<CCryDX12SamplerState*>(*ppSamplers));
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::Begin(
    _In_  ID3D11Asynchronous* pAsync)
{
    DX12_FUNC_LOG
    auto pQuery = reinterpret_cast<CCryDX12Query*>(pAsync);

    D3D11_QUERY_DESC desc;
    pQuery->GetDesc(&desc);

    if (desc.Query == D3D11_QUERY_EVENT)
    {
        return;
    }
    else if (desc.Query == D3D11_QUERY_TIMESTAMP_DISJOINT)
    {
        return;
    }
    else if (desc.Query == D3D11_QUERY_TIMESTAMP)
    {
        return;
    }
    else if (desc.Query == D3D11_QUERY_OCCLUSION || desc.Query == D3D11_QUERY_OCCLUSION_PREDICATE)
    {
        auto pOcclusionQuery = reinterpret_cast<CCryDX12ResourceQuery*>(pAsync);
        pOcclusionQuery->SetFenceValue(m_CmdFenceSet.GetCurrentValue(CMDQUEUE_GRAPHICS));
        pOcclusionQuery->SetQueryIndex(OcclusionIndex(m_DirectCommandList, desc.Query == D3D11_QUERY_OCCLUSION));
        ++m_OutstandingQueries;
    }
    else
    {
        __debugbreak();
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::End(
    _In_  ID3D11Asynchronous* pAsync)
{
    DX12_FUNC_LOG
    auto pQuery = reinterpret_cast<CCryDX12Query*>(pAsync);

    D3D11_QUERY_DESC desc;
    pQuery->GetDesc(&desc);

    if (desc.Query == D3D11_QUERY_EVENT)
    {
        // Record fence of commands prior to this point
        auto pEventQuery = reinterpret_cast<CCryDX12EventQuery*>(pAsync);
        pEventQuery->SetFenceValue(InsertFence());
    }
    else if (desc.Query == D3D11_QUERY_TIMESTAMP_DISJOINT)
    {
        return;
    }
    else if (desc.Query == D3D11_QUERY_TIMESTAMP)
    {
        // Record fence of commands following at this point
        auto pTimestampQuery = reinterpret_cast<CCryDX12ResourceQuery*>(pAsync);
        pTimestampQuery->SetFenceValue(InsertFence());
        pTimestampQuery->SetQueryIndex(TimestampIndex(m_DirectCommandList));
        pTimestampQuery->SetQueryResource(QueryTimestamp(m_DirectCommandList, pTimestampQuery->GetQueryIndex()));
    }
    else if (desc.Query == D3D11_QUERY_OCCLUSION)
    {
        auto pOcclusionQuery = reinterpret_cast<CCryDX12ResourceQuery*>(pAsync);
        pOcclusionQuery->SetFenceValue(InsertFence());
        pOcclusionQuery->SetQueryResource(QueryOcclusion(m_DirectCommandList, pOcclusionQuery->GetQueryIndex(), desc.Query == D3D11_QUERY_OCCLUSION));

        AZ_Assert(m_OutstandingQueries != 0, "End without a Start");
        --m_OutstandingQueries;
    }
    else // || desc.Query == D3D11_QUERY_OCCLUSION_PREDICATE)
    {
        __debugbreak();
    }
}

HRESULT STDMETHODCALLTYPE CCryDX12DeviceContext::GetData(
    _In_  ID3D11Asynchronous* pAsync,
    _Out_writes_bytes_opt_(DataSize)  void* pData,
    [[maybe_unused]] _In_  UINT DataSize,
    _In_  UINT GetDataFlags)
{
    DX12_FUNC_LOG
    auto pQuery = reinterpret_cast<CCryDX12Query*>(pAsync);

    D3D11_QUERY_DESC desc;
    pQuery->GetDesc(&desc);

    if (desc.Query == D3D11_QUERY_EVENT)
    {
        AZ_Assert(DataSize >= sizeof(BOOL), "Invalid data size");

        auto pEventQuery = reinterpret_cast<CCryDX12EventQuery*>(pAsync);
        bool bComplete = TestForFence(pEventQuery->GetFenceValue()) == S_OK;

        if (!bComplete && !(GetDataFlags & D3D11_ASYNC_GETDATA_DONOTFLUSH))
        {
            // Ensure the command-list issuing the query is executed
            SubmitDirectCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC, pEventQuery->GetFenceValue());
            FlushToFence(pEventQuery->GetFenceValue());
        }

        *reinterpret_cast<BOOL*>(pData) = bComplete;
        return bComplete ? S_OK : S_FALSE;
    }
    else if (desc.Query == D3D11_QUERY_TIMESTAMP_DISJOINT)
    {
        AZ_Assert(DataSize >= sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), "Invalid data size");
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT sData;

        // D3D12 WARNING: ID3D12CommandQueue::ID3D12CommandQueue::GetTimestampFrequency: Use
        // ID3D12Device::SetStablePowerstate for reliable timestamp queries [ EXECUTION WARNING #736: UNSTABLE_POWER_STATE]
        sData.Disjoint = FALSE;
        sData.Frequency = m_pDX12Device->GetGpuTimestampFrequency();

        *reinterpret_cast<D3D11_QUERY_DATA_TIMESTAMP_DISJOINT*>(pData) = sData;

        return S_OK;
    }
    else if (desc.Query == D3D11_QUERY_TIMESTAMP)
    {
        AZ_Assert(DataSize >= sizeof(UINT64), "Invalid data size");

        auto pTimestampQuery = reinterpret_cast<CCryDX12ResourceQuery*>(pAsync);
        bool bComplete = TestForFence(pTimestampQuery->GetFenceValue()) == S_OK;

        if (!bComplete && !(GetDataFlags & D3D11_ASYNC_GETDATA_DONOTFLUSH))
        {
            // Ensure the command-list issuing the timestamp is executed
            SubmitDirectCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC, pTimestampQuery->GetFenceValue());
            FlushToFence(pTimestampQuery->GetFenceValue());
        }

        // Since we do FlushToFence if not completed, it will be completed now.
        ResolveTimestamp(m_DirectCommandList, pTimestampQuery->GetQueryIndex(), pData);

        return S_OK;
    }
    else if (desc.Query == D3D11_QUERY_OCCLUSION)
    {
        AZ_Assert(DataSize >= sizeof(UINT64), "Invalid data size");

        auto pOcclusionQuery = reinterpret_cast<CCryDX12ResourceQuery*>(pAsync);
        bool bComplete = TestForFence(pOcclusionQuery->GetFenceValue()) == S_OK;

        if (!bComplete && !(GetDataFlags & D3D11_ASYNC_GETDATA_DONOTFLUSH))
        {
            // Ensure the command-list issuing the query is executed
            SubmitDirectCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC, pOcclusionQuery->GetFenceValue());
            FlushToFence(pOcclusionQuery->GetFenceValue());
        }

        if (bComplete)
        {
            ResolveOcclusion(m_DirectCommandList, pOcclusionQuery->GetQueryIndex(), pData);
        }

        return (bComplete) ? S_OK : S_FALSE;
    }
    else // || desc.Query == D3D11_QUERY_OCCLUSION_PREDICATE
    {
        __debugbreak();
    }

    return S_OK;
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::SetPredication(
    [[maybe_unused]] _In_opt_  ID3D11Predicate* pPredicate,
    [[maybe_unused]] _In_  BOOL PredicateValue)
{
    DX12_FUNC_LOG
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::GSSetShaderResources(
    _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    _In_reads_opt_(NumViews)  ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
    DX12_FUNC_LOG
    auto& geometry = m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Geometry];
    for (UINT i = StartSlot, S = StartSlot + NumViews; i < S; ++i, ++ppShaderResourceViews)
    {
        CCryDX12ShaderResourceView* srv = reinterpret_cast<CCryDX12ShaderResourceView*>(*ppShaderResourceViews);
        if (geometry.ShaderResourceViews.Set(i, srv) && srv)
        {
            srv->BeginResourceStateTransition(m_DirectCommandList.get());
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::GSSetSamplers(
    _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    _In_reads_opt_(NumSamplers)  ID3D11SamplerState* const* ppSamplers)
{
    DX12_FUNC_LOG
    auto& geometry = m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Geometry];
    for (UINT i = StartSlot, S = StartSlot + NumSamplers; i < S; ++i, ++ppSamplers)
    {
        geometry.SamplerState.Set(i, reinterpret_cast<CCryDX12SamplerState*>(*ppSamplers));
    }
}

static void ValidateSwapChain([[maybe_unused]] CCryDX12RenderTargetView& rtv)
{
#if defined(DEBUG)
    if (DX12::SwapChain* swapChain = rtv.GetDX12Resource().GetDX12SwapChain())
    {
        AZ_Assert(swapChain->GetCurrentBackBuffer().GetD3D12Resource() == rtv.GetDX12Resource().GetD3D12Resource(), "invalid swap chain buffer");
    }
#endif
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::OMSetRenderTargets(
    _In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumViews,
    _In_reads_opt_(NumViews)  ID3D11RenderTargetView* const* ppRenderTargetViews,
    _In_opt_  ID3D11DepthStencilView* pDepthStencilView)
{
    DX12_FUNC_LOG
    auto& outputMerger = m_PipelineState[DX12::CommandModeGraphics].OutputMerger;

    outputMerger.NumRenderTargets.Set(NumViews);
    for (UINT i = 0; i < NumViews; ++i)
    {
        CCryDX12RenderTargetView* rtv = reinterpret_cast<CCryDX12RenderTargetView*>(ppRenderTargetViews[i]);
        if (outputMerger.RenderTargetViews.Set(i, rtv))
        {
            if (!rtv)
            {
                outputMerger.RTVFormats.Set(i, DXGI_FORMAT_UNKNOWN);
                continue;
            }

            ValidateSwapChain(*rtv);

            DX12_LOG("Setting render target: %p (%d)", rtv, i);

            // TODO: this might not be the earliest moment when it is known that the resource is used as render-target
            D3D11_RENDER_TARGET_VIEW_DESC desc;
            rtv->GetDesc(&desc);
            rtv->BeginResourceStateTransition(m_DirectCommandList.get());

            outputMerger.RTVFormats.Set(i, desc.Format);
        }
    }

    for (UINT i = NumViews; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        outputMerger.RenderTargetViews.Set(i, nullptr);
        outputMerger.RTVFormats.Set(i, DXGI_FORMAT_UNKNOWN);
    }

    {
        CCryDX12DepthStencilView* dsv = reinterpret_cast<CCryDX12DepthStencilView*>(pDepthStencilView);
        if (outputMerger.DepthStencilView.Set(dsv))
        {
            if (!dsv)
            {
                outputMerger.DSVFormat.Set(DXGI_FORMAT_UNKNOWN);
                return;
            }

            DX12_LOG("Setting depth stencil view: %p", dsv);

            // TODO: this might not be the earliest moment when it is known that the resource is used as depth-stencil
            D3D11_DEPTH_STENCIL_VIEW_DESC desc;
            dsv->GetDesc(&desc);
            dsv->BeginResourceStateTransition(m_DirectCommandList.get());

            outputMerger.DSVFormat.Set(desc.Format);
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::OMSetRenderTargetsAndUnorderedAccessViews(
    _In_  UINT NumRTVs,
    [[maybe_unused]] _In_reads_opt_(NumRTVs)  ID3D11RenderTargetView* const* ppRenderTargetViews,
    [[maybe_unused]] _In_opt_  ID3D11DepthStencilView* pDepthStencilView,
    _In_range_(0, D3D11_1_UAV_SLOT_COUNT - 1)  UINT UAVStartSlot,
    _In_  UINT NumUAVs,
    _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView* const* ppUnorderedAccessViews,
    [[maybe_unused]] _In_reads_opt_(NumUAVs)  const UINT* pUAVInitialCounts)
{
    DX12_FUNC_LOG
    if (NumRTVs > 0 && NumRTVs != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL)
    {
        DX12_NOT_IMPLEMENTED //@TODO: Only the Unordered Access View portion of this function is implemented.
    }

    auto& graphicsStage = m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Pixel];
    for (UINT i = UAVStartSlot, S = UAVStartSlot + NumUAVs; i < S; ++i, ++ppUnorderedAccessViews)
    {
        CCryDX12UnorderedAccessView* uav = reinterpret_cast<CCryDX12UnorderedAccessView*>(*ppUnorderedAccessViews);
        if (graphicsStage.UnorderedAccessViews.Set(i, uav) && uav)
        {
            uav->BeginResourceStateTransition(m_DirectCommandList.get());
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::OMSetBlendState(
    _In_opt_  ID3D11BlendState* pBlendState,
    [[maybe_unused]] _In_opt_  const FLOAT BlendFactor[4],
    _In_  UINT SampleMask)
{
    DX12_FUNC_LOG

    auto& outputMerger = m_PipelineState[DX12::CommandModeGraphics].OutputMerger;
    outputMerger.BlendState.Set(reinterpret_cast<CCryDX12BlendState*>(pBlendState));
    outputMerger.SampleMask.Set(SampleMask);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::OMSetDepthStencilState(
    _In_opt_  ID3D11DepthStencilState* pDepthStencilState,
    _In_  UINT StencilRef)
{
    DX12_FUNC_LOG

    auto& state = m_PipelineState[DX12::CommandModeGraphics];
    state.Rasterizer.DepthStencilState.Set(reinterpret_cast<CCryDX12DepthStencilState*>(pDepthStencilState));
    state.OutputMerger.StencilRef.Set(StencilRef);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::SOSetTargets(
    [[maybe_unused]] _In_range_(0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
    [[maybe_unused]] _In_reads_opt_(NumBuffers)  ID3D11Buffer* const* ppSOTargets,
    [[maybe_unused]] _In_reads_opt_(NumBuffers)  const UINT* pOffsets)
{
    DX12_FUNC_LOG
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DrawAuto()
{
    DX12_FUNC_LOG
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DrawIndexedInstancedIndirect(
    [[maybe_unused]] _In_  ID3D11Buffer* pBufferForArgs,
    [[maybe_unused]] _In_  UINT AlignedByteOffsetForArgs)
{
    DX12_FUNC_LOG
    DX12_ASSERT(false, "unimplemented");
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DrawInstancedIndirect(
    [[maybe_unused]] _In_  ID3D11Buffer* pBufferForArgs,
    [[maybe_unused]] _In_  UINT AlignedByteOffsetForArgs)
{
    DX12_FUNC_LOG
    DX12_ASSERT(false, "unimplemented");
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::Dispatch(
    _In_  UINT ThreadGroupCountX,
    _In_  UINT ThreadGroupCountY,
    _In_  UINT ThreadGroupCountZ)
{
    DX12_FUNC_LOG

    SubmitCopyCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);

    if (PrepareComputeState())
    {
        m_DirectCommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
#ifdef AZ_DEBUG_BUILD
        m_PipelineState[DX12::CommandModeCompute].DebugPrint();
#endif
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DispatchIndirect(
    [[maybe_unused]] _In_  ID3D11Buffer* pBufferForArgs,
    [[maybe_unused]] _In_  UINT AlignedByteOffsetForArgs)
{
    DX12_FUNC_LOG
    DX12_ASSERT(false, "unimplemented");
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::RSSetState(
    _In_opt_  ID3D11RasterizerState* pRasterizerState)
{
    DX12_FUNC_LOG

    auto& rasterizer = m_PipelineState[DX12::CommandModeGraphics].Rasterizer;
    rasterizer.RasterizerState.Set(reinterpret_cast<CCryDX12RasterizerState*>(pRasterizerState));
    if (pRasterizerState)
    {
        const D3D11_RASTERIZER_DESC& d3d11Desc = rasterizer.RasterizerState->GetD3D11RasterizerDesc();
        rasterizer.ScissorEnabled.Set(d3d11Desc.ScissorEnable);
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::RSSetViewports(
    _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
    _In_reads_opt_(NumViewports)  const D3D11_VIEWPORT* pViewports)
{
    DX12_FUNC_LOG

    auto& rasterizer = m_PipelineState[DX12::CommandModeGraphics].Rasterizer;
    rasterizer.NumViewports.Set(NumViewports);
    if (NumViewports)
    {
        for (UINT i = 0; i < NumViewports; ++i, ++pViewports)
        {
            rasterizer.Viewports.Set(i, *pViewports);
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::RSSetScissorRects(
    _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
    _In_reads_opt_(NumRects)  const D3D11_RECT* pRects)
{
    DX12_FUNC_LOG

    auto& rasterizer = m_PipelineState[DX12::CommandModeGraphics].Rasterizer;
    rasterizer.NumScissors.Set(NumRects);
    if (NumRects)
    {
        for (UINT i = 0; i < NumRects; ++i, ++pRects)
        {
            rasterizer.Scissors.Set(i, *pRects);
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CopySubresourceRegion(
    _In_  ID3D11Resource* pDstResource,
    _In_  UINT DstSubresource,
    _In_  UINT DstX,
    _In_  UINT DstY,
    _In_  UINT DstZ,
    _In_  ID3D11Resource* pSrcResource,
    _In_  UINT SrcSubresource,
    _In_opt_  const D3D11_BOX* pSrcBox)
{
    DX12_FUNC_LOG

    CopySubresourceRegion1(
        pDstResource,
        DstSubresource,
        DstX,
        DstY,
        DstZ,
        pSrcResource,
        SrcSubresource,
        pSrcBox,
        0);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CopyResource(
    _In_  ID3D11Resource* pDstResource,
    _In_  ID3D11Resource* pSrcResource)
{
    DX12_FUNC_LOG

    CopyResource1(
        pDstResource,
        pSrcResource,
        0);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::UploadResource(
    _In_  ICryDX12Resource* pDstResource,
    _In_  const D3D11_SUBRESOURCE_DATA* pInitialData,
    _In_  size_t numInitialData)
{
    DX12_FUNC_LOG
    AZ_TRACE_METHOD();

    DX12::Resource& dstResource = pDstResource->GetDX12Resource();
    DX12::Resource::InitialData* id = dstResource.GetOrCreateInitialData();


    D3D12_RESOURCE_DESC desc = dstResource.GetDesc();
    const UINT maxSubResource = 64;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[maxSubResource];
    UINT nRows[maxSubResource];
    UINT64 rowSizes[maxSubResource];
    CRY_ASSERT(numInitialData <= maxSubResource);

    dstResource.GetDevice()->GetD3D12Device()->GetCopyableFootprints(&desc, 0, numInitialData, 0, layouts, nRows, rowSizes, &id->m_UploadSize);

    id->m_SubResourceData.resize(numInitialData);

    //Manually compute id->m_Size. This number is used to figure out the size of the intermediate CPU/GPU buffer into which the texture data is copied.
    //The reason for doing this manually is because pTotalBytes (set by GetCopyableFootprints) is not  completely accurate. For some reason it
    //seems like it doesnt take into account the 256 byte alignment padding of the last mip of a texture. Hence manually calculating the size
    //using the pNumRows (in this case nRows) and pRowSizeInBytes (in this case rowSizes).
    UINT64 totalBufferSize = 0;

    for (UINT i = 0; i < numInitialData; ++i)
    {
        D3D12_SUBRESOURCE_DATA initialDestData;

        initialDestData.RowPitch = pInitialData[i].SysMemPitch;
        initialDestData.SlicePitch = pInitialData[i].SysMemSlicePitch;

        UINT64 slicePitchSize = 0;
        slicePitchSize = pInitialData[i].SysMemSlicePitch * desc.DepthOrArraySize; // Exact size of the texture data. No padding.
        //TODO: pInitialData[i].SysMemSlicePitch can be 0 if the texture loading code doesnt set it. Fix this. For now we just query the GPU for the size (which is padded and wastes memory).
        if (slicePitchSize == 0)
        {
            slicePitchSize = dstResource.GetRequiredUploadSize(i, 1);
        }

        BYTE* destMemory = new uint8_t[slicePitchSize];
        UINT gpuSlicePitchSize = layouts[i].Footprint.RowPitch * nRows[i];
        D3D12_MEMCPY_DEST destData = { destMemory, layouts[i].Footprint.RowPitch, gpuSlicePitchSize };

        const BYTE* pSrc = static_cast<const BYTE*> (pInitialData[i].pSysMem);

        for (UINT z = 0; z < layouts[i].Footprint.Depth; ++z)
        {
            BYTE* pDestSlice = static_cast<BYTE*> (destData.pData) + pInitialData[i].SysMemSlicePitch * z;
            const BYTE* pSrcSlice = pSrc + pInitialData[i].SysMemSlicePitch * z;
            for (UINT y = 0; y < nRows[i]; ++y)
            {
                memcpy(pDestSlice + pInitialData[i].SysMemPitch * y,
                    pSrcSlice + pInitialData[i].SysMemPitch * y,
                    pInitialData[i].SysMemPitch);
            }
        }
        initialDestData.pData = destMemory;
        id->m_SubResourceData[i] = initialDestData;
        totalBufferSize += gpuSlicePitchSize;
    }
    id->m_Size = totalBufferSize;
    UploadResource(&dstResource, id);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::UploadResource(
    [[maybe_unused]] _In_  DX12::Resource* pDstResource,
    [[maybe_unused]] _In_  const DX12::Resource::InitialData* pSrcData)
{
    DX12_FUNC_LOG

    // TODO: This needs to be thread-safe
    // pDstResource->InitDeferred(m_pGraphicsCmdList);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::UpdateSubresource(
    _In_  ID3D11Resource* pDstResource,
    _In_  UINT DstSubresource,
    _In_opt_  const D3D11_BOX* pDstBox,
    _In_  const void* pSrcData,
    _In_  UINT SrcRowPitch,
    _In_  UINT SrcDepthPitch)
{
    DX12_FUNC_LOG

    UpdateSubresource1(
        pDstResource,
        DstSubresource,
        pDstBox,
        pSrcData,
        SrcRowPitch,
        SrcDepthPitch,
        0);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CopyStructureCount(
    [[maybe_unused]] _In_  ID3D11Buffer* pDstBuffer,
    [[maybe_unused]] _In_  UINT DstAlignedByteOffset,
    [[maybe_unused]] _In_  ID3D11UnorderedAccessView* pSrcView)
{
    DX12_FUNC_LOG
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::ClearRenderTargetView(
    _In_  ID3D11RenderTargetView* pRenderTargetView,
    _In_  const FLOAT ColorRGBA[4])
{
    DX12_FUNC_LOG

    DX12::ResourceView* view = DX12_EXTRACT_DX12VIEW(pRenderTargetView);
    CCryDX12RenderTargetView* rtv = reinterpret_cast<CCryDX12RenderTargetView*>(pRenderTargetView);
    DX12_LOG("Clearing render target view: %p %s", pRenderTargetView, rtv->GetResourceName().c_str());

    ValidateSwapChain(*rtv);
    m_DirectCommandList->ClearRenderTargetView(*view, ColorRGBA);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::ClearUnorderedAccessViewUint(
    _In_  ID3D11UnorderedAccessView* pUnorderedAccessView,
    _In_  const UINT Values[4])
{
    DX12_FUNC_LOG

    DX12::ResourceView* view = DX12_EXTRACT_DX12VIEW(pUnorderedAccessView);
#ifndef NDEBUG
    CCryDX12UnorderedAccessView* uav = reinterpret_cast<CCryDX12UnorderedAccessView*>(pUnorderedAccessView);
    DX12_LOG("Clearing unordered access view [int]: %p %s", pUnorderedAccessView, uav->GetResourceName().c_str());
#endif

    m_DirectCommandList->ClearUnorderedAccessView(*view, Values);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::ClearUnorderedAccessViewFloat(
    _In_  ID3D11UnorderedAccessView* pUnorderedAccessView,
    _In_  const FLOAT Values[4])
{
    DX12_FUNC_LOG

    DX12::ResourceView* view = DX12_EXTRACT_DX12VIEW(pUnorderedAccessView);
#ifndef NDEBUG
    CCryDX12UnorderedAccessView* uav = reinterpret_cast<CCryDX12UnorderedAccessView*>(pUnorderedAccessView);
    DX12_LOG("Clearing unordered access view [float]: %p %s", pUnorderedAccessView, uav->GetResourceName().c_str());
#endif

    m_DirectCommandList->ClearUnorderedAccessView(*view, Values);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::ClearDepthStencilView(
    _In_  ID3D11DepthStencilView* pDepthStencilView,
    _In_  UINT ClearFlags,     // DX11 and DX12 clear flags are identical
    _In_  FLOAT Depth,
    _In_  UINT8 Stencil)
{
    DX12_FUNC_LOG

    DX12::ResourceView* view = DX12_EXTRACT_DX12VIEW(pDepthStencilView);
#ifndef NDEBUG
    CCryDX12DepthStencilView* dsv = reinterpret_cast<CCryDX12DepthStencilView*>(pDepthStencilView);
    DX12_LOG("Clearing depth stencil view: %p %s", pDepthStencilView, dsv->GetResourceName().c_str());
#endif

    m_DirectCommandList->ClearDepthStencilView(*view, D3D12_CLEAR_FLAGS(ClearFlags), Depth, Stencil);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::GenerateMips(
    [[maybe_unused]] _In_  ID3D11ShaderResourceView* pShaderResourceView)
{
    DX12_FUNC_LOG
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::SetResourceMinLOD(
    [[maybe_unused]] _In_  ID3D11Resource* pResource,
    [[maybe_unused]] FLOAT MinLOD)
{
    DX12_FUNC_LOG
}

FLOAT STDMETHODCALLTYPE CCryDX12DeviceContext::GetResourceMinLOD(
    [[maybe_unused]] _In_  ID3D11Resource* pResource)
{
    DX12_FUNC_LOG
    return 0.0f;
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::ResolveSubresource(
    [[maybe_unused]] _In_  ID3D11Resource* pDstResource,
    [[maybe_unused]] _In_  UINT DstSubresource,
    [[maybe_unused]] _In_  ID3D11Resource* pSrcResource,
    [[maybe_unused]] _In_  UINT SrcSubresource,
    [[maybe_unused]] _In_  DXGI_FORMAT Format)
{
    DX12_FUNC_LOG
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::HSSetShaderResources(
    _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    _In_reads_opt_(NumViews)  ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
    DX12_FUNC_LOG
    for (UINT i = StartSlot, S = StartSlot + NumViews; i < S; ++i, ++ppShaderResourceViews)
    {
        CCryDX12ShaderResourceView* srv = reinterpret_cast<CCryDX12ShaderResourceView*>(*ppShaderResourceViews);
        if (m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Hull].ShaderResourceViews.Set(i, srv) && srv)
        {
            srv->BeginResourceStateTransition(m_DirectCommandList.get());
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::HSSetShader(
    _In_opt_  ID3D11HullShader* pHullShader,
    [[maybe_unused]] _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance* const* ppClassInstances,
    [[maybe_unused]] UINT NumClassInstances)
{
    DX12_FUNC_LOG
    m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Hull].Shader.Set(reinterpret_cast<CCryDX12Shader*>(pHullShader));
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::HSSetSamplers(
    _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    _In_reads_opt_(NumSamplers)  ID3D11SamplerState* const* ppSamplers)
{
    DX12_FUNC_LOG
    for (UINT i = StartSlot, S = StartSlot + NumSamplers; i < S; ++i, ++ppSamplers)
    {
        m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Hull].SamplerState.Set(i, reinterpret_cast<CCryDX12SamplerState*>(*ppSamplers));
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::HSSetConstantBuffers(
    _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_(NumBuffers)  ID3D11Buffer* const* ppConstantBuffers)
{
    HSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, NULL, NULL);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DSSetShaderResources(
    _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    _In_reads_opt_(NumViews)  ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
    DX12_FUNC_LOG
    for (UINT i = StartSlot, S = StartSlot + NumViews; i < S; ++i, ++ppShaderResourceViews)
    {
        CCryDX12ShaderResourceView* srv = reinterpret_cast<CCryDX12ShaderResourceView*>(*ppShaderResourceViews);
        if (m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Domain].ShaderResourceViews.Set(i, srv) && srv)
        {
            srv->BeginResourceStateTransition(m_DirectCommandList.get());
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DSSetShader(
    _In_opt_  ID3D11DomainShader* pDomainShader,
    [[maybe_unused]] _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance* const* ppClassInstances,
    [[maybe_unused]] UINT NumClassInstances)
{
    DX12_FUNC_LOG
    m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Domain].Shader.Set(reinterpret_cast<CCryDX12Shader*>(pDomainShader));
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DSSetSamplers(
    _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    _In_reads_opt_(NumSamplers)  ID3D11SamplerState* const* ppSamplers)
{
    DX12_FUNC_LOG
    for (UINT i = StartSlot, S = StartSlot + NumSamplers; i < S; ++i, ++ppSamplers)
    {
        m_PipelineState[DX12::CommandModeGraphics].Stages[DX12::ESS_Domain].SamplerState.Set(i, reinterpret_cast<CCryDX12SamplerState*>(*ppSamplers));
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DSSetConstantBuffers(
    _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_(NumBuffers)  ID3D11Buffer* const* ppConstantBuffers)
{
    DSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, NULL, NULL);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CSSetShaderResources(
    _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    _In_reads_opt_(NumViews)  ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
    DX12_FUNC_LOG
    auto& compute = m_PipelineState[DX12::CommandModeCompute].Stages[DX12::ESS_Compute];
    for (UINT i = StartSlot, S = StartSlot + NumViews; i < S; ++i, ++ppShaderResourceViews)
    {
        CCryDX12ShaderResourceView* srv = reinterpret_cast<CCryDX12ShaderResourceView*>(*ppShaderResourceViews);
        if (compute.ShaderResourceViews.Set(i, srv) && srv)
        {
            srv->BeginResourceStateTransition(m_DirectCommandList.get());
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CSSetUnorderedAccessViews(
    _In_range_(0, D3D11_1_UAV_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_1_UAV_SLOT_COUNT - StartSlot)  UINT NumUAVs,
    _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView* const* ppUnorderedAccessViews,
    [[maybe_unused]] _In_reads_opt_(NumUAVs)  const UINT* pUAVInitialCounts)
{
    DX12_FUNC_LOG
    auto& compute = m_PipelineState[DX12::CommandModeCompute].Stages[DX12::ESS_Compute];
    for (UINT i = StartSlot, S = StartSlot + NumUAVs; i < S; ++i, ++ppUnorderedAccessViews)
    {
        CCryDX12UnorderedAccessView* uav = reinterpret_cast<CCryDX12UnorderedAccessView*>(*ppUnorderedAccessViews);
        if (compute.UnorderedAccessViews.Set(i, uav) && uav)
        {
            uav->BeginResourceStateTransition(m_DirectCommandList.get());
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CSSetShader(
    _In_opt_  ID3D11ComputeShader* pComputeShader,
    [[maybe_unused]] _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance* const* ppClassInstances,
    [[maybe_unused]] UINT NumClassInstances)
{
    DX12_FUNC_LOG
    m_PipelineState[DX12::CommandModeCompute].Stages[DX12::ESS_Compute].Shader.Set(reinterpret_cast<CCryDX12Shader*>(pComputeShader));
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CSSetSamplers(
    _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    _In_reads_opt_(NumSamplers)  ID3D11SamplerState* const* ppSamplers)
{
    DX12_FUNC_LOG
    for (UINT i = StartSlot, S = StartSlot + NumSamplers; i < S; ++i, ++ppSamplers)
    {
        m_PipelineState[DX12::CommandModeCompute].Stages[DX12::ESS_Compute].SamplerState.Set(i, reinterpret_cast<CCryDX12SamplerState*>(*ppSamplers));
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CSSetConstantBuffers(
    _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_(NumBuffers)  ID3D11Buffer* const* ppConstantBuffers)
{
    CSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, NULL, NULL);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::VSGetConstantBuffers(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    [[maybe_unused]] _Out_writes_opt_(NumBuffers)  ID3D11Buffer** ppConstantBuffers)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::PSGetShaderResources(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    [[maybe_unused]] _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView** ppShaderResourceViews)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::PSGetShader(
    [[maybe_unused]] _Out_  ID3D11PixelShader** ppPixelShader,
    [[maybe_unused]] _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance** ppClassInstances,
    [[maybe_unused]] _Inout_opt_  UINT* pNumClassInstances)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::PSGetSamplers(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    [[maybe_unused]] _Out_writes_opt_(NumSamplers)  ID3D11SamplerState** ppSamplers)
{
    DX12_FUNC_LOG
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::VSGetShader(
    [[maybe_unused]] _Out_  ID3D11VertexShader** ppVertexShader,
    [[maybe_unused]] _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance** ppClassInstances,
    [[maybe_unused]] _Inout_opt_  UINT* pNumClassInstances)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::PSGetConstantBuffers(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    [[maybe_unused]] _Out_writes_opt_(NumBuffers)  ID3D11Buffer** ppConstantBuffers)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::IAGetInputLayout(
    _Out_  ID3D11InputLayout** ppInputLayout)
{
    DX12_FUNC_LOG
    *ppInputLayout = m_PipelineState[DX12::CommandModeGraphics].InputAssembler.InputLayout;
    if (*ppInputLayout)
    {
        (*ppInputLayout)->AddRef();
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::IAGetVertexBuffers(
   [[maybe_unused]] _In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
   [[maybe_unused]] _In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumBuffers,
   [[maybe_unused]] _Out_writes_opt_(NumBuffers)  ID3D11Buffer** ppVertexBuffers,
   [[maybe_unused]] _Out_writes_opt_(NumBuffers)  UINT* pStrides,
   [[maybe_unused]] _Out_writes_opt_(NumBuffers)  UINT* pOffsets)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::IAGetIndexBuffer(
    _Out_opt_  ID3D11Buffer** pIndexBuffer,
    _Out_opt_  DXGI_FORMAT* Format,
    _Out_opt_  UINT* Offset)
{
    DX12_FUNC_LOG
    auto& inputAssembler = m_PipelineState[DX12::CommandModeGraphics].InputAssembler;
    * pIndexBuffer = inputAssembler.IndexBuffer;
    if (*pIndexBuffer)
    {
        (*pIndexBuffer)->AddRef();
    }

    *Format = inputAssembler.IndexBufferFormat.Get();
    *Offset = inputAssembler.IndexBufferOffset.Get();
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::GSGetConstantBuffers(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    [[maybe_unused]] _Out_writes_opt_(NumBuffers)  ID3D11Buffer** ppConstantBuffers)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::GSGetShader(
    [[maybe_unused]] _Out_  ID3D11GeometryShader** ppGeometryShader,
    [[maybe_unused]] _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance** ppClassInstances,
    [[maybe_unused]] _Inout_opt_  UINT* pNumClassInstances)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::IAGetPrimitiveTopology(
    _Out_  D3D11_PRIMITIVE_TOPOLOGY* pTopology)
{
    DX12_FUNC_LOG
    *pTopology = m_PipelineState[DX12::CommandModeGraphics].InputAssembler.PrimitiveTopology.Get();
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::VSGetShaderResources(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    [[maybe_unused]] _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView** ppShaderResourceViews)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::VSGetSamplers(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    [[maybe_unused]] _Out_writes_opt_(NumSamplers)  ID3D11SamplerState** ppSamplers)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::GetPredication(
    [[maybe_unused]] _Out_opt_  ID3D11Predicate** ppPredicate,
    [[maybe_unused]] _Out_opt_  BOOL* pPredicateValue)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::GSGetShaderResources(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    [[maybe_unused]] _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView** ppShaderResourceViews)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::GSGetSamplers(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    [[maybe_unused]] _Out_writes_opt_(NumSamplers)  ID3D11SamplerState** ppSamplers)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::OMGetRenderTargets(
    [[maybe_unused]] _In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumViews,
    [[maybe_unused]] _Out_writes_opt_(NumViews)  ID3D11RenderTargetView** ppRenderTargetViews,
    [[maybe_unused]] _Out_opt_  ID3D11DepthStencilView** ppDepthStencilView)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::OMGetRenderTargetsAndUnorderedAccessViews(
    [[maybe_unused]] _In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumRTVs,
    [[maybe_unused]] _Out_writes_opt_(NumRTVs)  ID3D11RenderTargetView** ppRenderTargetViews,
    [[maybe_unused]] _Out_opt_  ID3D11DepthStencilView** ppDepthStencilView,
    [[maybe_unused]] _In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  UINT UAVStartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot)  UINT NumUAVs,
    [[maybe_unused]] _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView** ppUnorderedAccessViews)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::OMGetBlendState(
    [[maybe_unused]] _Out_opt_  ID3D11BlendState** ppBlendState,
    [[maybe_unused]] _Out_opt_  FLOAT BlendFactor[4],
    [[maybe_unused]] _Out_opt_  UINT* pSampleMask)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::OMGetDepthStencilState(
    [[maybe_unused]] _Out_opt_  ID3D11DepthStencilState** ppDepthStencilState,
    [[maybe_unused]] _Out_opt_  UINT* pStencilRef)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::SOGetTargets(
    [[maybe_unused]] _In_range_(0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
    [[maybe_unused]] _Out_writes_opt_(NumBuffers)  ID3D11Buffer** ppSOTargets)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::RSGetState(
    [[maybe_unused]] _Out_  ID3D11RasterizerState** ppRasterizerState)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::RSGetViewports(
    [[maybe_unused]] _Inout_  UINT* pNumViewports,
    [[maybe_unused]] _Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT* pViewports)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::RSGetScissorRects(
    [[maybe_unused]] _Inout_  UINT* pNumRects,
    [[maybe_unused]] _Out_writes_opt_(*pNumRects)  D3D11_RECT* pRects)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::HSGetShaderResources(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    [[maybe_unused]] _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView** ppShaderResourceViews)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::HSGetShader(
    [[maybe_unused]] _Out_  ID3D11HullShader** ppHullShader,
    [[maybe_unused]] _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance** ppClassInstances,
    [[maybe_unused]] _Inout_opt_  UINT* pNumClassInstances)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::HSGetSamplers(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    [[maybe_unused]] _Out_writes_opt_(NumSamplers)  ID3D11SamplerState** ppSamplers)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::HSGetConstantBuffers(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    [[maybe_unused]] _Out_writes_opt_(NumBuffers)  ID3D11Buffer** ppConstantBuffers)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DSGetShaderResources(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    [[maybe_unused]] _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView** ppShaderResourceViews)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DSGetShader(
    [[maybe_unused]] _Out_  ID3D11DomainShader** ppDomainShader,
    [[maybe_unused]] _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance** ppClassInstances,
    [[maybe_unused]] _Inout_opt_  UINT* pNumClassInstances)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DSGetSamplers(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    [[maybe_unused]] _Out_writes_opt_(NumSamplers)  ID3D11SamplerState** ppSamplers)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::DSGetConstantBuffers(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    [[maybe_unused]] _Out_writes_opt_(NumBuffers)  ID3D11Buffer** ppConstantBuffers)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CSGetShaderResources(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    [[maybe_unused]] _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView** ppShaderResourceViews)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CSGetUnorderedAccessViews(
    [[maybe_unused]] _In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - StartSlot)  UINT NumUAVs,
    [[maybe_unused]] _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView** ppUnorderedAccessViews)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CSGetShader(
    [[maybe_unused]] _Out_  ID3D11ComputeShader** ppComputeShader,
    [[maybe_unused]] _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance** ppClassInstances,
    [[maybe_unused]] _Inout_opt_  UINT* pNumClassInstances)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CSGetSamplers(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    [[maybe_unused]] _Out_writes_opt_(NumSamplers)  ID3D11SamplerState** ppSamplers)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CSGetConstantBuffers(
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    [[maybe_unused]] _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    [[maybe_unused]] _Out_writes_opt_(NumBuffers)  ID3D11Buffer** ppConstantBuffers)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::ClearState()
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::Flush()
{
    DX12_FUNC_LOG

    SubmitAllCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC, m_CmdFenceSet.GetCurrentValues());
}

D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE CCryDX12DeviceContext::GetType()
{
    DX12_FUNC_LOG
    return D3D11_DEVICE_CONTEXT_IMMEDIATE;
}

UINT STDMETHODCALLTYPE CCryDX12DeviceContext::GetContextFlags()
{
    DX12_FUNC_LOG
    return 0;
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::ExecuteCommandList(
    [[maybe_unused]] _In_  ID3D11CommandList* pCommandList,
    [[maybe_unused]] _In_  BOOL RestoreContextState)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

HRESULT STDMETHODCALLTYPE CCryDX12DeviceContext::FinishCommandList(
    [[maybe_unused]] BOOL RestoreDeferredContextState,
    [[maybe_unused]] _Out_opt_  ID3D11CommandList** ppCommandList)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
    return E_FAIL;
}

#pragma endregion

#pragma region /* D3D 11.1 specific functions */

void STDMETHODCALLTYPE CCryDX12DeviceContext::CopySubresourceRegion1(
    _In_  ID3D11Resource* pDstResource,
    _In_  UINT DstSubresource,
    _In_  UINT DstX,
    _In_  UINT DstY,
    _In_  UINT DstZ,
    _In_  ID3D11Resource* pSrcResource,
    _In_  UINT SrcSubresource,
    _In_opt_  const D3D11_BOX* pSrcBox,
    _In_  UINT CopyFlags)
{
    DX12_FUNC_LOG

    ICryDX12Resource* dstDX12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pDstResource);
    ICryDX12Resource* srcDX12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pSrcResource);
    DX12::Resource& dstResource = dstDX12Resource->GetDX12Resource();
    DX12::Resource& srcResource = srcDX12Resource->GetDX12Resource();

    // TODO: copy command on the swap chain are special (can't execute on any other queue), make an API for that
    const bool bDirect = !m_bInCopyRegion;

    DX12::CommandListPool& rCmdListPool = bDirect ? m_DirectListPool :  m_CopyListPool;
    DX12::CommandList*     pCmdList     = bDirect ? m_DirectCommandList : m_CopyCommandList;

    // TODO: move into the command-list function
    UINT64 maxFenceValues[CMDQUEUE_NUM];
    switch (CopyFlags)
    {
    case D3D11_COPY_NO_OVERWRITE:
        // If the resource is currently used, the flag lied!
        AZ_Assert(!dstResource.IsUsed(rCmdListPool), "Destination resource is in use for non-discard copy!");

        SubmitAllCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC,
            srcResource.GetFenceValues(CMDTYPE_WRITE),
            rCmdListPool.GetFenceID());
        break;
    default:
        SubmitAllCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC, DX12::MaxFenceValues(
            srcResource.GetFenceValues(CMDTYPE_WRITE),
            dstResource.GetFenceValues(CMDTYPE_ANY),
            maxFenceValues),
            rCmdListPool.GetFenceID());

        // Block the GPU-thread until the resource is safe to be updated (unlike Map() we stage the copy and don't need to block the CPU)
        break;
    }

    D3D12_BOX box;
    if (pSrcBox)
    {
        box.top = pSrcBox->top;
        box.bottom = pSrcBox->bottom;
        box.left = pSrcBox->left;
        box.right = pSrcBox->right;
        box.front = pSrcBox->front;
        box.back = pSrcBox->back;
    }

    // TODO: copy from active render-target should be more elegant
    if (m_DirectCommandList->IsUsedByOutputViews(srcResource) ||
        m_DirectCommandList->IsUsedByOutputViews(dstResource))
    {
        // Make the render-targets rebind, so the resource-barrier is closed
        m_PipelineState[DX12::CommandModeGraphics].m_StateFlags |= EPSPB_OutputResources;
    }

    AZ_Assert(dstResource.GetDX12SwapChain() == nullptr, "Can't copy to swap chain buffer");
    pCmdList->CopySubresource(dstResource, DstSubresource, DstX, DstY, DstZ, srcResource, SrcSubresource, pSrcBox ? &box : NULL);

    if constexpr (DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC)
    {
        if (!bDirect)
        {
            SubmitCopyCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);
        }
        else
        {
            SubmitDirectCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::CopyResource1(
    _In_  ID3D11Resource* pDstResource,
    _In_  ID3D11Resource* pSrcResource,
    _In_  UINT CopyFlags)
{
    DX12_FUNC_LOG
    AZ_TRACE_METHOD();

    ICryDX12Resource* dstDX12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pDstResource);
    ICryDX12Resource* srcDX12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pSrcResource);
    DX12::Resource& dstResource = dstDX12Resource->GetDX12Resource();
    DX12::Resource& srcResource = srcDX12Resource->GetDX12Resource();

    const bool bDirect = !m_bInCopyRegion;
    DX12::CommandListPool& rCmdListPool = bDirect ? m_DirectListPool    : m_CopyListPool;
    DX12::CommandList*     pCmdList     = bDirect ? m_DirectCommandList : m_CopyCommandList;

    // TODO: move into the command-list function
    UINT64 maxFenceValues[CMDQUEUE_NUM];
    switch (CopyFlags)
    {
    case D3D11_COPY_NO_OVERWRITE:
        // If the resource is currently used, the flag lied!
        AZ_Assert(!dstResource.IsUsed(rCmdListPool), "Destination resource is in use for non-discard copy!");
        SubmitAllCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC,
            srcResource.GetFenceValues(CMDTYPE_WRITE),
            rCmdListPool.GetFenceID());
        break;
    default:
        SubmitAllCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC, DX12::MaxFenceValues(
            srcResource.GetFenceValues(CMDTYPE_WRITE),
            dstResource.GetFenceValues(CMDTYPE_ANY),
            maxFenceValues),
            rCmdListPool.GetFenceID());

        // Block the GPU-thread until the resource is safe to be updated (unlike Map() we stage the update and don't need to block the CPU)
        break;
    }

    // TODO: copy from active render-target should be more elegant
    if (m_DirectCommandList->IsUsedByOutputViews(srcResource) ||
        m_DirectCommandList->IsUsedByOutputViews(dstResource))
    {
        // Make the render-targets rebind, so the resource-barrier is closed
        m_PipelineState[DX12::CommandModeGraphics].m_StateFlags |= EPSPB_OutputResources;
    }

    AZ_Assert(dstResource.GetDX12SwapChain() == nullptr, "Can't copy to swap chain buffer");
    pCmdList->CopyResource(dstResource, srcResource);
    if constexpr (DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC)
    {
        if (!bDirect)
        {
            SubmitCopyCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);
        }
        else
        {
            SubmitDirectCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);
        }
    }
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::UpdateSubresource1(
    _In_  ID3D11Resource* pDstResource,
    _In_  UINT DstSubresource,
    _In_opt_  const D3D11_BOX* pDstBox,
    _In_  const void* pSrcData,
    _In_  UINT SrcRowPitch,
    _In_  UINT SrcDepthPitch,
    _In_  UINT CopyFlags)
{
    DX12_FUNC_LOG
    AZ_TRACE_METHOD();

    ICryDX12Resource* dx12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pDstResource);
    DX12::Resource& resource = dx12Resource->GetDX12Resource();

    // TODO: copy command on the swap chain are special (can't execute on any other queue), make an API for that
    const bool bDirect = !m_bInCopyRegion;

    DX12::CommandListPool& rCmdListPool = bDirect ?  m_DirectListPool :  m_CopyListPool;
    DX12::CommandList*     pCmdList     = bDirect ? m_DirectCommandList     : m_CopyCommandList;

    // TODO: move into the command-list function
    switch (CopyFlags)
    {
    case D3D11_COPY_NO_OVERWRITE:
        // If the resource is currently used, the flag lied!
        AZ_Assert(!resource.IsUsed(rCmdListPool), "Destination resource is in use for non-discard copy!");
        break;
    default:
        SubmitAllCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC, resource.GetFenceValues(CMDTYPE_ANY), rCmdListPool.GetFenceID());

        // Block the GPU-thread until the resource is safe to be updated (unlike Map() we stage the update and don't need to block the CPU)
        break;
    }

    D3D12_BOX box;
    if (pDstBox)
    {
        box.top = pDstBox->top;
        box.bottom = pDstBox->bottom;
        box.left = pDstBox->left;
        box.right = pDstBox->right;
        box.front = pDstBox->front;
        box.back = pDstBox->back;
    }

    // TODO: copy from active render-target should be more elegant
    if (m_DirectCommandList->IsUsedByOutputViews(resource))
    {
        // Make the render-targets rebind, so the resource-barrier is closed
        m_PipelineState[DX12::CommandModeGraphics].m_StateFlags |= EPSPB_OutputResources;
    }

    AZ_Assert(resource.GetDX12SwapChain() == nullptr, "Can't copy to swap chain buffer");
    pCmdList->UpdateSubresourceRegion(resource, DstSubresource, pDstBox ? &box : NULL, pSrcData, SrcRowPitch, SrcDepthPitch);

    if constexpr (DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC)
    {
        if (!bDirect)
        {
            SubmitCopyCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);
        }
        else
        {
            SubmitDirectCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);
        }
    }
}

void CCryDX12DeviceContext::DiscardResource(ID3D11Resource* pResource)
{
    DX12_FUNC_LOG

    ICryDX12Resource* dx12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pResource);
    DX12::Resource& resource = dx12Resource->GetDX12Resource();

    // Ensure the command-list using the resource is executed
    SubmitDirectCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC,
        resource.GetFenceValue(CMDQUEUE_GRAPHICS, CMDTYPE_ANY)
        );

    m_CopyCommandList->DiscardResource(resource, nullptr);
    SubmitCopyCommands(DX12_SUBMISSION_MODE == DX12_SUBMISSION_SYNC);
}

void CCryDX12DeviceContext::DiscardView([[maybe_unused]] ID3D11View* pResourceView)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void CCryDX12DeviceContext::VSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    DX12_FUNC_LOG
    SetConstantBuffers1(DX12::ESS_Vertex, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants, DX12::CommandModeGraphics);
}

void CCryDX12DeviceContext::HSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    DX12_FUNC_LOG
    SetConstantBuffers1(DX12::ESS_Hull, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants, DX12::CommandModeGraphics);
}

void CCryDX12DeviceContext::DSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    DX12_FUNC_LOG
    SetConstantBuffers1(DX12::ESS_Domain, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants, DX12::CommandModeGraphics);
}

void CCryDX12DeviceContext::GSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    DX12_FUNC_LOG
    SetConstantBuffers1(DX12::ESS_Geometry, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants, DX12::CommandModeGraphics);
}

void CCryDX12DeviceContext::PSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    DX12_FUNC_LOG
    SetConstantBuffers1(DX12::ESS_Pixel, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants, DX12::CommandModeGraphics);
}

void CCryDX12DeviceContext::CSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    DX12_FUNC_LOG
    SetConstantBuffers1(DX12::ESS_Compute, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants, DX12::CommandModeCompute);
}

void CCryDX12DeviceContext::SetConstantBuffers1(
    DX12::EShaderStage shaderStage,
    UINT StartSlot,
    UINT NumBuffers,
    ID3D11Buffer* const* ppConstantBuffers,
    const UINT* pFirstConstant,
    const UINT* pNumConstants,
    DX12::CommandMode commandMode)
{
    auto& stage = m_PipelineState[commandMode].Stages[shaderStage];
    for (UINT i = StartSlot, S = StartSlot + NumBuffers, j = 0; i < S; ++i, ++ppConstantBuffers, ++j)
    {
        CCryDX12Buffer* cb = reinterpret_cast<CCryDX12Buffer*>(*ppConstantBuffers);

        TRange < UINT > bindRange;
        if (pFirstConstant)
        {
            bindRange.start = pFirstConstant[j] * DX12::CONSTANT_BUFFER_ELEMENT_SIZE;
            bindRange.end = bindRange.start + pNumConstants[j] * DX12::CONSTANT_BUFFER_ELEMENT_SIZE;
        }

        stage.ConstBufferBindRange.Set(i, bindRange);
        if (stage.ConstantBufferViews.Set(i, cb) && cb)
        {
            cb->BeginResourceStateTransition(m_DirectCommandList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        }
    }
}

void CCryDX12DeviceContext::VSGetConstantBuffers1([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer** ppConstantBuffers, [[maybe_unused]] UINT* pFirstConstant, [[maybe_unused]] UINT* pNumConstants)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void CCryDX12DeviceContext::HSGetConstantBuffers1([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer** ppConstantBuffers, [[maybe_unused]] UINT* pFirstConstant, [[maybe_unused]] UINT* pNumConstants)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void CCryDX12DeviceContext::DSGetConstantBuffers1([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer** ppConstantBuffers, [[maybe_unused]] UINT* pFirstConstant, [[maybe_unused]] UINT* pNumConstants)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void CCryDX12DeviceContext::GSGetConstantBuffers1([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer** ppConstantBuffers, [[maybe_unused]] UINT* pFirstConstant, [[maybe_unused]] UINT* pNumConstants)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void CCryDX12DeviceContext::PSGetConstantBuffers1([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer** ppConstantBuffers, [[maybe_unused]] UINT* pFirstConstant, [[maybe_unused]] UINT* pNumConstants)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void CCryDX12DeviceContext::CSGetConstantBuffers1([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer** ppConstantBuffers, [[maybe_unused]] UINT* pFirstConstant, [[maybe_unused]] UINT* pNumConstants)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void CCryDX12DeviceContext::SwapDeviceContextState([[maybe_unused]] ID3DDeviceContextState* pState, [[maybe_unused]] ID3DDeviceContextState** ppPreviousState)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void CCryDX12DeviceContext::ClearView(ID3D11View* pView, const FLOAT Color[4], const D3D11_RECT* pRect, UINT NumRects)
{
    DX12_FUNC_LOG
    DX12_LOG("Clearing view: %p", pView);

    DX12::ResourceView* view = DX12_EXTRACT_DX12VIEW(pView);

    m_DirectCommandList->ClearView(*view, Color, NumRects, pRect);
}

void CCryDX12DeviceContext::DiscardView1([[maybe_unused]] ID3D11View* pResourceView, [[maybe_unused]] const D3D11_RECT* pRects, [[maybe_unused]] UINT NumRects)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::ClearRectsRenderTargetView(
    _In_  ID3D11RenderTargetView* pRenderTargetView,
    _In_  const FLOAT ColorRGBA[4],
    _In_  UINT NumRects,
    _In_reads_opt_(NumRects)  const D3D11_RECT* pRects)
{
    DX12_FUNC_LOG

    DX12::ResourceView* view = DX12_EXTRACT_DX12VIEW(pRenderTargetView);
#ifndef NDEBUG
    CCryDX12RenderTargetView* rtv = reinterpret_cast<CCryDX12RenderTargetView*>(pRenderTargetView);
    DX12_LOG("Clearing render target view: %p %s", pRenderTargetView, rtv->GetResourceName().c_str());
#endif

    m_DirectCommandList->ClearRenderTargetView(*view, ColorRGBA, NumRects, pRects);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::ClearRectsUnorderedAccessViewUint(
    _In_  ID3D11UnorderedAccessView* pUnorderedAccessView,
    _In_  const UINT Values[4],
    _In_  UINT NumRects,
    _In_reads_opt_(NumRects)  const D3D11_RECT* pRects)
{
    DX12_FUNC_LOG

    DX12::ResourceView* view = DX12_EXTRACT_DX12VIEW(pUnorderedAccessView);
#ifndef NDEBUG
    CCryDX12UnorderedAccessView* uav = reinterpret_cast<CCryDX12UnorderedAccessView*>(pUnorderedAccessView);
    DX12_LOG("Clearing unordered access view [int]: %p %s", pUnorderedAccessView, uav->GetResourceName().c_str());
#endif

    m_DirectCommandList->ClearUnorderedAccessView(*view, Values, NumRects, pRects);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::ClearRectsUnorderedAccessViewFloat(
    _In_  ID3D11UnorderedAccessView* pUnorderedAccessView,
    _In_  const FLOAT Values[4],
    _In_  UINT NumRects,
    _In_reads_opt_(NumRects)  const D3D11_RECT* pRects)
{
    DX12_FUNC_LOG

    DX12::ResourceView* view = DX12_EXTRACT_DX12VIEW(pUnorderedAccessView);
#ifndef NDEBUG
    CCryDX12UnorderedAccessView* uav = reinterpret_cast<CCryDX12UnorderedAccessView*>(pUnorderedAccessView);
    DX12_LOG("Clearing unordered access view [float]: %p %s", pUnorderedAccessView, uav->GetResourceName().c_str());
#endif

    m_DirectCommandList->ClearUnorderedAccessView(*view, Values, NumRects, pRects);
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::ClearRectsDepthStencilView(
    _In_  ID3D11DepthStencilView* pDepthStencilView,
    _In_  UINT ClearFlags,     // DX11 and DX12 clear flags are identical
    _In_  FLOAT Depth,
    _In_  UINT8 Stencil,
    _In_  UINT NumRects,
    _In_reads_opt_(NumRects)  const D3D11_RECT* pRects)
{
    DX12_FUNC_LOG

    DX12::ResourceView* view = DX12_EXTRACT_DX12VIEW(pDepthStencilView);
#ifndef NDEBUG
    CCryDX12DepthStencilView* dsv = reinterpret_cast<CCryDX12DepthStencilView*>(pDepthStencilView);
    DX12_LOG("Clearing depth stencil view: %p %s", pDepthStencilView, dsv->GetResourceName().c_str());
#endif

    m_DirectCommandList->ClearDepthStencilView(*view, D3D12_CLEAR_FLAGS(ClearFlags), Depth, Stencil, NumRects, pRects);
}

HRESULT STDMETHODCALLTYPE CCryDX12DeviceContext::CopyStagingResource(
    _In_  ID3D11Resource* pStagingResource,
    _In_  ID3D11Resource* pSourceResource,
    _In_  UINT SubResource,
    _In_  BOOL Upload)
{
    DX12_FUNC_LOG

    if (Upload)
    {
        CopySubresourceRegion1(pSourceResource, SubResource, 0, 0, 0, pStagingResource, 0, nullptr, D3D11_COPY_NO_OVERWRITE);
    }
    else
    {
        CopySubresourceRegion1(pStagingResource, 0, 0, 0, 0, pSourceResource, SubResource, nullptr, D3D11_COPY_NO_OVERWRITE);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CCryDX12DeviceContext::MapStagingResource(
    _In_  ID3D11Resource* pTextureResource,
    _In_  ID3D11Resource* pStagingResource,
    _In_  UINT SubResource,
    _In_  BOOL Upload,
    _Out_ void** ppStagingMemory,
    _Out_ uint32* pRowPitch)
{
    DX12_FUNC_LOG

    ICryDX12Resource* dx12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pStagingResource);
    DX12::Resource& rResource = dx12Resource->GetDX12Resource();

    ICryDX12Resource* dx12TextureResource = DX12_EXTRACT_ICRYDX12RESOURCE(pTextureResource);
    DX12::Resource& rTextureResource = dx12TextureResource->GetDX12Resource();

    // If this assert trips, you may need to increase MAX_SUBRESOURCES.  Just be wary of growing the stack too much.
    AZ_Assert(SubResource < MAX_SUBRESOURCES, "Too many sub resources: (sub ID %d requested, %d allowed)", SubResource, MAX_SUBRESOURCES);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[MAX_SUBRESOURCES];

    // From our source texture resource, get the offset and description information for all the subresources up through the one we're trying
    // to map.  Our staging resource will contain a buffer large enough for *all* the subresources, so we need to get the correct offset
    // into this buffer.  If we just get the Layout for the one SubResource, it will come back with an offset of 0, which is incorrect.
    GetDevice()->GetD3D12Device()->GetCopyableFootprints(&rTextureResource.GetDesc(), 0, SubResource + 1, 0, layouts, nullptr, nullptr, nullptr);

    const D3D12_RANGE sNoRead = { 0, 0 }; // It is valid to specify the CPU won't read any data by passing a range where End is less than or equal to Begin
    const D3D12_RANGE sFullRead = { 0, rResource.GetRequiredUploadSize(0, 1) }; // Our buffer only has 1 subresource, so get the full size of it to map.

    HRESULT result = rResource.GetD3D12Resource()->Map(0, Upload ? &sNoRead : &sFullRead, ppStagingMemory);

    // Map() will return a pointer to the start of our buffer.  Adjust the pointer to the offset of the desired mapped subresource within the buffer.
    *ppStagingMemory = *reinterpret_cast<uint8**>(ppStagingMemory) + layouts[SubResource].Offset;

    *pRowPitch = layouts[SubResource].Footprint.RowPitch;

    return result;
}

void STDMETHODCALLTYPE CCryDX12DeviceContext::UnmapStagingResource(
    _In_  ID3D11Resource* pStagingResource,
    _In_  BOOL Upload)
{
    DX12_FUNC_LOG

    ICryDX12Resource* dx12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pStagingResource);
    DX12::Resource& rResource = dx12Resource->GetDX12Resource();

    const D3D12_RANGE sNoWrite = { 0, 0 }; // It is valid to specify the CPU didn't write any data by passing a range where End is less than or equal to Begin.
    const D3D12_RANGE sFullWrite = { 0, rResource.GetRequiredUploadSize(0, 1) };

    rResource.GetD3D12Resource()->Unmap(0, Upload ? &sFullWrite : &sNoWrite);
}

HRESULT STDMETHODCALLTYPE CCryDX12DeviceContext::TestStagingResource(
    _In_  ID3D11Resource* pStagingResource)
{
    DX12_FUNC_LOG

    ICryDX12Resource* dx12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pStagingResource);
    DX12::Resource& rResource = dx12Resource->GetDX12Resource();

    return
        TestForFence(rResource.GetFenceValue(CMDQUEUE_GRAPHICS, CMDTYPE_ANY)) &&
        TestForFence(rResource.GetFenceValue(CMDQUEUE_COPY, CMDTYPE_ANY));
}

HRESULT STDMETHODCALLTYPE CCryDX12DeviceContext::WaitStagingResource(
    _In_  ID3D11Resource* pStagingResource)
{
    DX12_FUNC_LOG

    ICryDX12Resource* dx12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pStagingResource);
    DX12::Resource& rResource = dx12Resource->GetDX12Resource();

    if (rResource.GetFenceValue(CMDQUEUE_GRAPHICS, CMDTYPE_WRITE) == m_DirectListPool.GetCurrentFenceValue())
    {
        SubmitDirectCommands(false);
    }

    if (rResource.GetFenceValue(CMDQUEUE_COPY, CMDTYPE_WRITE) == m_CopyListPool.GetCurrentFenceValue())
    {
        SubmitCopyCommands(false);
    }

    rResource.WaitForUnused(m_DirectListPool, CMDTYPE_ANY);
    rResource.WaitForUnused(m_CopyListPool, CMDTYPE_ANY);

    return S_OK;
}

void CCryDX12DeviceContext::ResetCachedState()
{
    m_PipelineState[DX12::CommandModeGraphics].Invalidate();
    m_PipelineState[DX12::CommandModeCompute].Invalidate();

    m_CurrentPSO = nullptr;
    m_CurrentRootSignature[DX12::CommandModeGraphics] = m_CurrentRootSignature[DX12::CommandModeCompute] = nullptr;
}

#pragma endregion
