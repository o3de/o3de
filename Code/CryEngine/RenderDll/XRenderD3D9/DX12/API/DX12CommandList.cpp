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
#include "DX12CommandList.hpp"
#include "DX12Resource.hpp"
#include "DX12View.hpp"
#include "DX12SamplerState.hpp"
#include "DX12/GI/CCryDX12SwapChain.hpp"
#include "AzCore/Debug/EventTraceDrillerBus.h"

namespace DX12
{
    namespace EventTrace
    {
        const AZStd::thread_id GpuDetailThreadId{ (size_t)2 };
        const char* GpuDetailName = "GPU Detail";
        const char* GpuDetailCategory = "GPU";
    }

    const char* StateToString(D3D12_RESOURCE_STATES state)
    {
        static int buf = 0;
        static char str[2][512];
        static char *ret;

        ret = str[buf ^= 1];
        *ret = '\0';

        if (!state)
        {
            azstrcat(ret, AZStd::size(str[0]), " Common/Present");
            return ret;
        }

        if ((state & D3D12_RESOURCE_STATE_GENERIC_READ) == D3D12_RESOURCE_STATE_GENERIC_READ)
        {
            azstrcat(ret, AZStd::size(str[0]), " Generic Read");
            return ret;
        }

        if (state & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
        {
            azstrcat(ret, AZStd::size(str[0]), " V/C Buffer");
        }
        if (state & D3D12_RESOURCE_STATE_INDEX_BUFFER)
        {
            azstrcat(ret, AZStd::size(str[0]), " I Buffer");
        }
        if (state & D3D12_RESOURCE_STATE_RENDER_TARGET)
        {
            azstrcat(ret, AZStd::size(str[0]), " RT");
        }
        if (state & D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        {
            azstrcat(ret, AZStd::size(str[0]), " UA");
        }
        if (state & D3D12_RESOURCE_STATE_DEPTH_WRITE)
        {
            azstrcat(ret, AZStd::size(str[0]), " DepthW");
        }
        if (state & D3D12_RESOURCE_STATE_DEPTH_READ)
        {
            azstrcat(ret, AZStd::size(str[0]), " DepthR");
        }
        if (state & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        {
            azstrcat(ret, AZStd::size(str[0]), " NoPixelR");
        }
        if (state & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
        {
            azstrcat(ret, AZStd::size(str[0]), " PixelR");
        }
        if (state & D3D12_RESOURCE_STATE_STREAM_OUT)
        {
            azstrcat(ret, AZStd::size(str[0]), " Stream Out");
        }
        if (state & D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
        {
            azstrcat(ret, AZStd::size(str[0]), " Indirect Arg");
        }
        if (state & D3D12_RESOURCE_STATE_COPY_DEST)
        {
            azstrcat(ret, AZStd::size(str[0]), " CopyD");
        }
        if (state & D3D12_RESOURCE_STATE_COPY_SOURCE)
        {
            azstrcat(ret, AZStd::size(str[0]), " CopyS");
        }
        if (state & D3D12_RESOURCE_STATE_RESOLVE_DEST)
        {
            azstrcat(ret, AZStd::size(str[0]), " ResolveD");
        }
        if (state & D3D12_RESOURCE_STATE_RESOLVE_SOURCE)
        {
            azstrcat(ret, AZStd::size(str[0]), " ResolveS");
        }

        return ret;
    }

    CommandList::CommandList(CommandListPool& pool)
        : DeviceObject(pool.GetDevice())
        , m_rPool(pool)
        , m_pCmdQueue{}
        , m_pCmdAllocator{}
        , m_CommandList{}
        , m_pD3D12Device{}
        , m_CurrentNumRTVs{}
        , m_CurrentNumVertexBuffers{}
        , m_CurrentFenceValue{}
        , m_eListType(pool.GetD3D12QueueType())
        , m_State(CLSTATE_FREE)
        , m_nodeMask{}
#if DX12_GPU_PROFILE_MODE != DX12_GPU_PROFILE_MODE_OFF
        , m_Timers{ *GetDevice() }
#endif
        , m_DescriptorHeaps
        {
            {
                GetDevice()->GetGlobalDescriptorBlock(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024)
            },
            {
                GetDevice()->GetGlobalDescriptorBlock(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1024)
            },
            {
                GetDevice()->GetGlobalDescriptorBlock(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1024)
            },
            {
                GetDevice()->GetGlobalDescriptorBlock(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1024)
            }
        }
    {
        ResetStateTracking(CommandModeCompute);
        ResetStateTracking(CommandModeGraphics);
    }

    CommandList::~CommandList()
    {
    }

    bool CommandList::Init(UINT64 currentFenceValue)
    {
        m_State = CLSTATE_STARTED;
        m_Commands = 0;
        m_CurrentPipelineState = nullptr;
        m_CurrentRootSignature[DX12::CommandModeCompute] = m_CurrentRootSignature[DX12::CommandModeGraphics] = nullptr;
        m_pDSV = nullptr;
        m_CurrentNumRTVs = 0;
        m_CurrentNumVertexBuffers = 0;

        m_CurrentFenceValue = currentFenceValue;
        ZeroMemory(m_UsedFenceValues, sizeof(m_UsedFenceValues));

        m_pD3D12Device = GetDevice()->GetD3D12Device();

        for (D3D12_DESCRIPTOR_HEAP_TYPE eType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; eType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; eType = D3D12_DESCRIPTOR_HEAP_TYPE(eType + 1))
        {
            m_DescriptorHeaps[eType].Reset();
        }

        if (!IsInitialized())
        {
            m_pCmdQueue = m_rPool.GetD3D12CommandQueue();

            D3D12_COMMAND_LIST_TYPE eCmdListType = m_pCmdQueue->GetDesc().Type;

            ID3D12CommandAllocator* pCmdAllocator = nullptr;
            if (S_OK != m_pD3D12Device->CreateCommandAllocator(eCmdListType, IID_GRAPHICS_PPV_ARGS(&pCmdAllocator)))
            {
                DX12_ERROR("Could not create command allocator!");
                return false;
            }

            m_pCmdAllocator = pCmdAllocator;
            pCmdAllocator->Release();

            ID3D12GraphicsCommandList* pCmdList = nullptr;
            if (S_OK != m_pD3D12Device->CreateCommandList(m_nodeMask, eCmdListType, m_pCmdAllocator, nullptr, IID_GRAPHICS_PPV_ARGS(&pCmdList)))
            {
                DX12_ERROR("Could not create command list");
                return false;
            }

            m_CommandList = pCmdList;
            pCmdList->Release();

#if DX12_GPU_PROFILE_MODE != DX12_GPU_PROFILE_MODE_OFF
            m_Timers.Init(TimerCountMax);
#endif

            IsInitialized(true);
        }

#ifdef DX12_STATS
        m_NumWaitsGPU = 0;
        m_NumWaitsCPU = 0;
#endif // DX12_STATS

        return true;
    }

    void CommandList::Begin()
    {
#if DX12_GPU_PROFILE_MODE != DX12_GPU_PROFILE_MODE_OFF
        m_Timers.Begin();
        m_TimerHandle = m_Timers.BeginTimer(*this, "CommandList");
#endif
    }

    void CommandList::End()
    {
        {
            DX12_COMMANDLIST_TIMER("End");
            FlushBarriers();
        }

        if (IsUtilized())
        {
#if DX12_GPU_PROFILE_MODE != DX12_GPU_PROFILE_MODE_OFF
            m_Timers.EndTimer(*this, m_TimerHandle);
            m_Timers.End(*this);
#endif

#ifndef NDEBUG
            HRESULT res =
#endif
                m_CommandList->Close();
            DX12_ASSERT(res == S_OK, "Could not close command list!");
        }

        m_State = CLSTATE_COMPLETED;
    }

    void CommandList::Schedule()
    {
        if (m_State < CLSTATE_SCHEDULED)
        {
            m_State = CLSTATE_SCHEDULED;
        }
    }

    void CommandList::Submit()
    {
        if (IsUtilized())
        {
            // Inject a Wait() into the CommandQueue prior to executing it to wait for all required resources being available either readable or writable
#ifdef DX12_IN_ORDER_SUBMISSION
            m_UsedFenceValues[CMDTYPE_ANY][CMDQUEUE_COPY    ] = std::max(m_UsedFenceValues[CMDTYPE_READ][CMDQUEUE_COPY    ], m_UsedFenceValues[CMDTYPE_WRITE][CMDQUEUE_COPY    ]);
            m_UsedFenceValues[CMDTYPE_ANY][CMDQUEUE_GRAPHICS] = std::max(m_UsedFenceValues[CMDTYPE_READ][CMDQUEUE_GRAPHICS], m_UsedFenceValues[CMDTYPE_WRITE][CMDQUEUE_GRAPHICS]);
#else
            MaxFenceValue(m_UsedFenceValues[CMDTYPE_ANY][CMDQUEUE_COPY    ], std::max(m_UsedFenceValues[CMDTYPE_READ][CMDQUEUE_COPY    ], m_UsedFenceValues[CMDTYPE_WRITE][CMDQUEUE_COPY    ]));
            MaxFenceValue(m_UsedFenceValues[CMDTYPE_ANY][CMDQUEUE_GRAPHICS], std::max(m_UsedFenceValues[CMDTYPE_READ][CMDQUEUE_GRAPHICS], m_UsedFenceValues[CMDTYPE_WRITE][CMDQUEUE_GRAPHICS]));
#endif

            m_rPool.WaitForFenceOnGPU(m_UsedFenceValues[CMDTYPE_ANY]);

            // Then inject the Execute() which is possibly blocked by the Wait()
            ID3D12CommandList* ppCommandLists[] = { GetD3D12CommandList() };
            m_rPool.GetAsyncCommandQueue().ExecuteCommandLists(1, ppCommandLists); // TODO: allow to submit multiple command-lists in one go
        }

        // Inject the signal of the utilized fence to unblock code which picked up the fence of the command-list (even if it doesn't have contents)
        m_rPool.SetSubmittedFenceValue(SignalFenceOnGPU());
        m_State = CLSTATE_SUBMITTED;
    }

    void CommandList::Clear()
    {
        m_State = CLSTATE_CLEARING;
        FlushBarriers();
        m_rPool.GetAsyncCommandQueue().ResetCommandList(this);
    }

    void CommandList::SubmitTimers()
    {
#if DX12_GPU_PROFILE_MODE != DX12_GPU_PROFILE_MODE_OFF
        m_Timers.ReadbackTimers();

        for (const Timer& timer : m_Timers.GetTimers())
        {
            EBUS_QUEUE_EVENT(AZ::Debug::EventTraceDrillerBus, RecordSlice, timer.m_Name, EventTrace::GpuDetailCategory, EventTrace::GpuDetailThreadId, timer.m_Timestamp, timer.m_Duration);
        }
#endif
    }

    bool CommandList::Reset()
    {
        if (IsUtilized())
        {
            SubmitTimers();

            HRESULT ret = 0;

            // reset the allocator before the list re-occupies it, otherwise the whole state of the allocator starts leaking
            if (m_pCmdAllocator)
            {
                ret = m_pCmdAllocator->Reset();
            }
            // make the list re-occupy this allocator, reseting the list will _not_ reset the allocator
            if (m_CommandList)
            {
                ret = m_CommandList->Reset(m_pCmdAllocator, NULL);
            }
        }

        m_State = CLSTATE_FREE;
        m_Commands = 0;
        return true;
    }

    void CommandList::ResetStateTracking(CommandMode commandMode)
    {
        memset(&m_CurrentRootParameters[commandMode], ~0, sizeof(m_CurrentRootParameters[commandMode]));
    }

    void CommandList::SetPipelineState(const PipelineState* pso)
    {
        if (m_CurrentPipelineState != pso)
        {
            m_CurrentPipelineState = pso;

            DX12_COMMANDLIST_TIMER_DETAIL("SetPipelineState");
            m_CommandList->SetPipelineState(pso->GetD3D12PipelineState());
            m_Commands += CLCOUNT_SETIO;
        }
    }

    void CommandList::SetRootSignature(CommandMode commandMode, const RootSignature* rootSignature)
    {
        if (m_CurrentRootSignature[commandMode] != rootSignature)
        {
            m_CurrentRootSignature[commandMode] = rootSignature;

            if (rootSignature)
            {
                ResetStateTracking(commandMode);
                DX12_COMMANDLIST_TIMER_DETAIL("SetRootSignature");
                switch (commandMode)
                {
                case CommandModeGraphics:
                    m_CommandList->SetGraphicsRootSignature(rootSignature->GetD3D12RootSignature());
                    break;

                case CommandModeCompute:
                    m_CommandList->SetComputeRootSignature(rootSignature->GetD3D12RootSignature());
                    break;
                }
                m_Commands += CLCOUNT_SETIO;
            }
        }
    }

    bool CommandList::IsFull(size_t numResources, size_t numSamplers, size_t numRendertargets, size_t numDepthStencils) const
    {
        return
            (GetResourceHeap().GetCursor() + numResources >= GetResourceHeap().GetCapacity()) ||
            (GetSamplerHeap().GetCursor() + numSamplers >= GetSamplerHeap().GetCapacity()) ||
            (GetRenderTargetHeap().GetCursor() + numRendertargets >= GetRenderTargetHeap().GetCapacity()) ||
            (GetDepthStencilHeap().GetCursor() + numDepthStencils >= GetDepthStencilHeap().GetCapacity());
    }

    void CommandList::SetDescriptorTable(CommandMode commandMode, UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor)
    {
        if (m_CurrentRootParameters[commandMode][RootParameterIndex].descriptorHandle.ptr != BaseDescriptor.ptr)
        {
            DX12_ASSERT(RootParameterIndex < RootParameterCountMax, "Too many root parameters");
            m_CurrentRootParameters[commandMode][RootParameterIndex].descriptorHandle = BaseDescriptor;

            switch (commandMode)
            {
            case CommandModeGraphics:
                m_CommandList->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
                break;

            case CommandModeCompute:
                m_CommandList->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
                break;
            }
            m_Commands += CLCOUNT_SETIO;
        }
    }

    void CommandList::SetConstantBufferView(CommandMode commandMode, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation)
    {
        if (m_CurrentRootParameters[commandMode][RootParameterIndex].gpuAddress != BufferLocation)
        {
            DX12_ASSERT(RootParameterIndex < RootParameterCountMax, "Too many root parameters");
            m_CurrentRootParameters[commandMode][RootParameterIndex].gpuAddress = BufferLocation;
            switch (commandMode)
            {
            case CommandModeGraphics:
                m_CommandList->SetGraphicsRootConstantBufferView(RootParameterIndex, BufferLocation);
                break;

            case CommandModeCompute:
                m_CommandList->SetComputeRootConstantBufferView(RootParameterIndex, BufferLocation);
                break;
            }
            m_Commands += CLCOUNT_SETIO;
        }
    }

    void CommandList::Set32BitConstants(CommandMode commandMode, UINT RootParameterIndex, UINT Num32BitValuesToSet, const void* pSrcData, UINT DestOffsetIn32BitValues)
    {
        switch (commandMode)
        {
        case CommandModeGraphics:
            m_CommandList->SetGraphicsRoot32BitConstants(RootParameterIndex, Num32BitValuesToSet, pSrcData, DestOffsetIn32BitValues);
            break;

        case CommandModeCompute:
            m_CommandList->SetComputeRoot32BitConstants(RootParameterIndex, Num32BitValuesToSet, pSrcData, DestOffsetIn32BitValues);
            break;
        }
        m_Commands += CLCOUNT_SETIO;
    }

    void CommandList::SetDescriptorTables(CommandMode commandMode, D3D12_DESCRIPTOR_HEAP_TYPE eType)
    {
        DX12_ASSERT(m_CurrentRootSignature[commandMode]);

        const PipelineLayout& layout = m_CurrentRootSignature[commandMode]->GetPipelineLayout();
        for (AZ::u32 rootParameterIdx = 0, tableIdx = 0; rootParameterIdx < layout.m_NumRootParameters; ++rootParameterIdx)
        {
            const CD3DX12_ROOT_PARAMETER& rootParameter = layout.m_RootParameters[rootParameterIdx];

            if (rootParameter.ParameterType != D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            {
                continue;
            }

            if (layout.m_DescriptorTables[tableIdx].m_Type == eType)
            {
                switch (commandMode)
                {
                case CommandModeCompute:
                    m_CommandList->SetComputeRootDescriptorTable(rootParameterIdx, m_DescriptorHeaps[eType].GetHandleOffsetGPU(layout.m_DescriptorTables[tableIdx].m_Offset));
                    break;

                case CommandModeGraphics:
                    m_CommandList->SetGraphicsRootDescriptorTable(rootParameterIdx, m_DescriptorHeaps[eType].GetHandleOffsetGPU(layout.m_DescriptorTables[tableIdx].m_Offset));
                    break;
                }
                m_Commands += CLCOUNT_SETIO;
            }

            ++tableIdx;
        }
    }

    void CommandList::BindDepthStencilView(const ResourceView& dsv)
    {
        Resource& resource = dsv.GetDX12Resource();
        const D3D12_CPU_DESCRIPTOR_HANDLE handle = GetDepthStencilHeap().GetHandleOffsetCPU(0);

        if (INVALID_CPU_DESCRIPTOR_HANDLE != dsv.GetDescriptorHandle())
        {
            m_pD3D12Device->CopyDescriptorsSimple(1, handle, dsv.GetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        }
        else
        {
            m_pD3D12Device->CreateDepthStencilView(resource.GetD3D12Resource(), dsv.HasDesc() ? &(dsv.GetDSVDesc()) : NULL, handle);
        }

        if (dsv.GetDSVDesc().Flags & D3D12_DSV_FLAG_READ_ONLY_DEPTH)
        {
            TrackResourceDRVUsage(resource);
        }
        else
        {
            TrackResourceDSVUsage(resource);
        }
    }

    void CommandList::PresentRenderTargetView(SwapChain* pDX12SwapChain)
    {
        QueueTransitionBarrier(pDX12SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
    }

    void CommandList::BindRenderTargetView(const ResourceView& renderTargetView)
    {
        Resource& resource = renderTargetView.GetDX12Resource();

        const D3D12_CPU_DESCRIPTOR_HANDLE handle = GetRenderTargetHeap().GetHandleOffsetCPU(0);
        if (INVALID_CPU_DESCRIPTOR_HANDLE != renderTargetView.GetDescriptorHandle())
        {
            m_pD3D12Device->CopyDescriptorsSimple(1, handle, renderTargetView.GetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }
        else
        {
            m_pD3D12Device->CreateRenderTargetView(resource.GetD3D12Resource(), renderTargetView.HasDesc() ? &(renderTargetView.GetRTVDesc()) : NULL, handle);
        }

        TrackResourceRTVUsage(resource);
    }

    void CommandList::WriteUnorderedAccessDescriptor(const ResourceView* resourceView, INT descriptorOffset)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = GetResourceHeap().GetHandleOffsetCPU(descriptorOffset);

        if (resourceView)
        {
            Resource& resource = resourceView->GetDX12Resource();

            if (INVALID_CPU_DESCRIPTOR_HANDLE != resourceView->GetDescriptorHandle())
            {
                m_pD3D12Device->CopyDescriptorsSimple(1, dstHandle, resourceView->GetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
            else
            {
                m_pD3D12Device->CreateUnorderedAccessView(resource.GetD3D12Resource(), nullptr, resourceView->HasDesc() ? &(resourceView->GetUAVDesc()) : NULL, dstHandle);
            }

            TrackResourceUAVUsage(resource);
        }
        else
        {
            m_pD3D12Device->CopyDescriptorsSimple(1, dstHandle, GetDevice()->GetNullUnorderedAccessView(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
    }

    void CommandList::WriteShaderResourceDescriptor(const ResourceView* resourceView, INT descriptorOffset)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = GetResourceHeap().GetHandleOffsetCPU(descriptorOffset);

        if (resourceView)
        {
            Resource& resource = resourceView->GetDX12Resource();
            DX12_ASSERT(resource.GetD3D12Resource(), "No resource to create SRV from!");

            if (INVALID_CPU_DESCRIPTOR_HANDLE != resourceView->GetDescriptorHandle())
            {
                m_pD3D12Device->CopyDescriptorsSimple(1, dstHandle, resourceView->GetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
            else
            {
                m_pD3D12Device->CreateShaderResourceView(resource.GetD3D12Resource(), &resourceView->GetSRVDesc(), dstHandle);
            }

            TrackResourceSRVUsage(resource);
        }
        else
        {
            m_pD3D12Device->CopyDescriptorsSimple(1, dstHandle, GetDevice()->GetNullShaderResourceView(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
    }

    void CommandList::WriteConstantBufferDescriptor(const ResourceView* resourceView, INT descriptorOffset, UINT byteOffset, UINT byteCount)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = GetResourceHeap().GetHandleOffsetCPU(descriptorOffset);

        if (resourceView)
        {
            if (INVALID_CPU_DESCRIPTOR_HANDLE != resourceView->GetDescriptorHandle())
            {
                m_pD3D12Device->CopyDescriptorsSimple(1, dstHandle, resourceView->GetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
            else
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
                cbvDesc = resourceView->GetCBVDesc();
                cbvDesc.BufferLocation += byteOffset;
                cbvDesc.SizeInBytes = byteCount ? byteCount : cbvDesc.SizeInBytes - byteOffset;
                DX12_ASSERT(byteCount < D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * DX12::CONSTANT_BUFFER_ELEMENT_SIZE, "exceeded allowed size of constant buffer");
                m_pD3D12Device->CreateConstantBufferView(&cbvDesc, dstHandle);
            }

            TrackResourceCBVUsage(resourceView->GetDX12Resource());
        }
        else
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
            m_pD3D12Device->CreateConstantBufferView(&desc, dstHandle);
        }
    }

    void CommandList::WriteSamplerStateDescriptor(const SamplerState* state, INT descriptorOffset)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = GetSamplerHeap().GetHandleOffsetCPU(descriptorOffset);

        if (state)
        {
            if (INVALID_CPU_DESCRIPTOR_HANDLE != state->GetDescriptorHandle())
            {
                m_pD3D12Device->CopyDescriptorsSimple(1, dstHandle, state->GetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            }
            else
            {
                m_pD3D12Device->CreateSampler(&state->GetSamplerDesc(), dstHandle);
            }
        }
        else
        {
            m_pD3D12Device->CopyDescriptorsSimple(1, dstHandle, GetDevice()->GetNullSampler(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        }
    }

    void CommandList::SetResourceAndSamplerStateHeaps()
    {
        DX12_COMMANDLIST_TIMER_DETAIL("SetResourceAndSamplerStateHeaps");
        ID3D12DescriptorHeap* heaps[] =
        {
            GetResourceHeap().GetDescriptorHeap()->GetD3D12DescriptorHeap(),
            GetSamplerHeap().GetDescriptorHeap()->GetD3D12DescriptorHeap()
        };

        m_CommandList->SetDescriptorHeaps(2, heaps);
        m_Commands += CLCOUNT_SETIO;
    }

    void CommandList::BindAndSetOutputViews(INT numRTVs, const ResourceView** rtv, const ResourceView* dsv)
    {
        DX12_COMMANDLIST_TIMER_DETAIL("BindAndSetOutputViews");
        if (dsv)
        {
            BindDepthStencilView(*dsv);
            GetDepthStencilHeap().IncrementCursor();
        }

        m_pDSV = dsv;

        for (INT i = 0; i < numRTVs; ++i)
        {
            BindRenderTargetView(*rtv[i]);
            GetRenderTargetHeap().IncrementCursor();

            m_pRTV[i] = rtv[i];
        }

        m_CurrentNumRTVs = numRTVs;

        D3D12_CPU_DESCRIPTOR_HANDLE RTVDescriptors[] = { GetRenderTargetHeap().GetHandleOffsetCPU(-numRTVs) };
        D3D12_CPU_DESCRIPTOR_HANDLE DSVDescriptors[] = { GetDepthStencilHeap().GetHandleOffsetCPU(-1) };

        m_CommandList->OMSetRenderTargets(numRTVs, numRTVs ? RTVDescriptors : NULL, true, dsv ? DSVDescriptors : NULL);
        m_Commands += CLCOUNT_SETIO;
    }

    bool CommandList::IsUsedByOutputViews(const Resource& res) const
    {
        if (m_pDSV && (&m_pDSV->GetDX12Resource() == &res))
        {
            return true;
        }

        for (INT i = 0, n = m_CurrentNumRTVs; i < n; ++i)
        {
            if (m_pRTV[i] && (&m_pRTV[i]->GetDX12Resource() == &res))
            {
                return true;
            }
        }

        return false;
    }

    void CommandList::BindVertexBufferView(const ResourceView& view, INT offset, const TRange<UINT>& bindRange, UINT bindStride, D3D12_VERTEX_BUFFER_VIEW (&heap)[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT])
    {
        Resource& resource = view.GetDX12Resource();

        D3D12_VERTEX_BUFFER_VIEW& vbvDesc = heap[offset];
        vbvDesc = view.GetVBVDesc();
        vbvDesc.BufferLocation += bindRange.start;
        vbvDesc.SizeInBytes = bindRange.Length() > 0 ? bindRange.Length() : vbvDesc.SizeInBytes - bindRange.start;
        vbvDesc.StrideInBytes = bindStride;

        TrackResourceVBVUsage(resource);
    }

    void CommandList::ClearVertexBufferHeap(UINT num)
    {
        memset(m_VertexBufferHeap, 0, std::max(num, m_CurrentNumVertexBuffers) * sizeof(m_VertexBufferHeap[0]));
    }

    void CommandList::SetVertexBufferHeap(UINT num)
    {
        m_CommandList->IASetVertexBuffers(0, std::max(num, m_CurrentNumVertexBuffers), m_VertexBufferHeap);
        m_Commands += CLCOUNT_SETIO;

        m_CurrentNumVertexBuffers = num;
    }

    void CommandList::BindAndSetIndexBufferView(const ResourceView& view, DXGI_FORMAT format, UINT offset)
    {
        Resource& resource = view.GetDX12Resource();

        D3D12_INDEX_BUFFER_VIEW ibvDesc;
        ibvDesc = view.GetIBVDesc();
        ibvDesc.BufferLocation += offset;
        ibvDesc.SizeInBytes = ibvDesc.SizeInBytes - offset;
        ibvDesc.Format = (format == DXGI_FORMAT_UNKNOWN ? resource.GetDesc().Format : format);

        TrackResourceIBVUsage(resource);

        m_CommandList->IASetIndexBuffer(&ibvDesc);
        m_Commands += CLCOUNT_SETIO;
    }

    void CommandList::ClearDepthStencilView(const ResourceView& view, D3D12_CLEAR_FLAGS clearFlags, float depthValue, UINT stencilValue, UINT NumRects, const D3D12_RECT* pRect)
    {
        DX12_ASSERT(INVALID_CPU_DESCRIPTOR_HANDLE != view.GetDescriptorHandle(), "View has no descriptor handle, that is not allowed!");
        DX12_COMMANDLIST_TIMER_DETAIL("ClearDepthStencilView");

        Resource& resource = view.GetDX12Resource();

        TrackResourceDSVUsage(resource);
        FlushBarriers();

        m_CommandList->ClearDepthStencilView(view.GetDescriptorHandle(), clearFlags, depthValue, stencilValue, NumRects, pRect);
        m_Commands += CLCOUNT_CLEAR;
    }

    void CommandList::ClearRenderTargetView(const ResourceView& inputView, const FLOAT rgba[4], UINT NumRects, const D3D12_RECT* pRect)
    {
        DX12_ASSERT(INVALID_CPU_DESCRIPTOR_HANDLE != inputView.GetDescriptorHandle(), "View has no descriptor handle, that is not allowed!");
        DX12_COMMANDLIST_TIMER_DETAIL("ClearRenderTargetView");

        TrackResourceRTVUsage(inputView.GetDX12Resource());
        FlushBarriers();

        m_CommandList->ClearRenderTargetView(inputView.GetDescriptorHandle(), rgba, NumRects, pRect);
        m_Commands += CLCOUNT_CLEAR;
    }

    void CommandList::ClearUnorderedAccessView(const ResourceView& view, const UINT rgba[4], UINT NumRects, const D3D12_RECT* pRect)
    {
        DX12_ASSERT(INVALID_CPU_DESCRIPTOR_HANDLE != view.GetDescriptorHandle(), "View has no descriptor handle, that is not allowed!");
        DX12_COMMANDLIST_TIMER_DETAIL("ClearUnorderedAccessView");

        TrackResourceUAVUsage(view.GetDX12Resource());
        FlushBarriers();

        m_CommandList->ClearUnorderedAccessViewUint(GetResourceHeap().GetHandleGPUFromCPU(view.GetDescriptorHandle()), view.GetDescriptorHandle(), view.GetD3D12Resource(), rgba, NumRects, pRect);
        m_Commands += CLCOUNT_CLEAR;
    }

    void CommandList::ClearUnorderedAccessView(const ResourceView& view, const FLOAT rgba[4], UINT NumRects, const D3D12_RECT* pRect)
    {
        DX12_ASSERT(INVALID_CPU_DESCRIPTOR_HANDLE != view.GetDescriptorHandle(), "View has no descriptor handle, that is not allowed!");
        DX12_COMMANDLIST_TIMER_DETAIL("ClearUnorderedAccessView");

        TrackResourceUAVUsage(view.GetDX12Resource());
        FlushBarriers();

        m_CommandList->ClearUnorderedAccessViewFloat(GetResourceHeap().GetHandleGPUFromCPU(view.GetDescriptorHandle()), view.GetDescriptorHandle(), view.GetD3D12Resource(), rgba, NumRects, pRect);
        m_Commands += CLCOUNT_CLEAR;
    }

    void CommandList::ClearView(const ResourceView& view, const FLOAT rgba[4], UINT NumRects, const D3D12_RECT* pRect)
    {
        DX12_ASSERT(INVALID_CPU_DESCRIPTOR_HANDLE != view.GetDescriptorHandle(), "View has no descriptor handle, that is not allowed!");

        switch (view.GetType())
        {
        case EVT_UnorderedAccessView:
            ClearUnorderedAccessView(view, rgba, NumRects, pRect);
            break;

        case EVT_DepthStencilView: // Optional implementation under DX11.1
            ClearDepthStencilView(view, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, FLOAT(rgba[0]), UINT(rgba[1]), NumRects, pRect);
            break;

        case EVT_RenderTargetView:
            ClearRenderTargetView(view, rgba, NumRects, pRect);
            break;

        case EVT_ShaderResourceView:
        case EVT_ConstantBufferView:
        case EVT_VertexBufferView:
        default:
            DX12_ASSERT(0, "Unsupported resource-type for input!");
            break;
        }
    }

    void CommandList::CopyResource(Resource& dstResource, Resource& srcResource)
    {
        if (srcResource.InitHasBeenDeferred())
        {
            srcResource.TryStagingUpload(this);
        }

        DX12_COMMANDLIST_TIMER_DETAIL("CopyResource");
        MaxResourceFenceValue(dstResource, CMDTYPE_ANY);
        MaxResourceFenceValue(srcResource, CMDTYPE_WRITE);

        QueueTransitionBarrier(dstResource, D3D12_RESOURCE_STATE_COPY_DEST);
        QueueTransitionBarrier(srcResource, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushBarriers();

        DX12_ASSERT(srcResource.GetDesc().Dimension == dstResource.GetDesc().Dimension, "Can't copy resources of different dimension");
        m_CommandList->CopyResource(dstResource.GetD3D12Resource(), srcResource.GetD3D12Resource());
        m_Commands += CLCOUNT_COPY;

        SetResourceFenceValue(dstResource, m_CurrentFenceValue, CMDTYPE_WRITE);
        SetResourceFenceValue(srcResource, m_CurrentFenceValue, CMDTYPE_READ);
    }

    void CommandList::CopySubresource(Resource& dstResource, UINT dstSubResource, UINT x, UINT y, UINT z, Resource& srcResource, UINT srcSubResource, const D3D12_BOX* srcBox)
    {
        if (srcResource.InitHasBeenDeferred())
        {
            srcResource.TryStagingUpload(this);
        }

        DX12_COMMANDLIST_TIMER_DETAIL("CopySubresource");
        MaxResourceFenceValue(dstResource, CMDTYPE_ANY);
        MaxResourceFenceValue(srcResource, CMDTYPE_WRITE);

        // TODO: if we know early that the resource(s) will be DEST and SOURCE we can begin the barrier early and end it here
        QueueTransitionBarrier(dstResource, D3D12_RESOURCE_STATE_COPY_DEST);
        QueueTransitionBarrier(srcResource, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushBarriers();

        if (srcResource.GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && dstResource.GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            UINT64 offset = 0;
            UINT64 length = srcResource.GetDesc().Width;

            if (srcBox)
            {
                length = srcBox->right - srcBox->left;
                offset = srcBox->left;
            }

            m_CommandList->CopyBufferRegion(dstResource.GetD3D12Resource(), x, srcResource.GetD3D12Resource(), offset, length);
            m_Commands += CLCOUNT_COPY;
        }
        else if (srcResource.GetDesc().Dimension != D3D12_RESOURCE_DIMENSION_BUFFER && dstResource.GetDesc().Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            CD3DX12_TEXTURE_COPY_LOCATION src(srcResource.GetD3D12Resource(), srcSubResource);
            CD3DX12_TEXTURE_COPY_LOCATION dst(dstResource.GetD3D12Resource(), dstSubResource);

            m_CommandList->CopyTextureRegion(&dst, x, y, z, &src, srcBox);
            m_Commands += CLCOUNT_COPY;
        }
        else if (dstResource.GetDesc().Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            // If this assert trips, you may need to increase MAX_SUBRESOURCES.  Just be wary of growing the stack too much.
            AZ_Assert(dstSubResource < CCryDX12DeviceContext::MAX_SUBRESOURCES, "Too many sub resources: (sub ID %d requested, %d allowed)", dstSubResource, CCryDX12DeviceContext::MAX_SUBRESOURCES);
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[CCryDX12DeviceContext::MAX_SUBRESOURCES];

            // From our "regular" resource, get the offset and description information for the subresource so we can copy the correct
            // part of the buffer resource.  We need to get Layouts for subresource 0 through dstSubResource so that the offset is set
            // correctly.  If we just get the Layout for dstSubResource, it will have an offset of 0.
            GetDevice()->GetD3D12Device()->GetCopyableFootprints(&dstResource.GetDesc(), 0, dstSubResource + 1, 0, layouts, nullptr, nullptr, nullptr);
            
            CD3DX12_TEXTURE_COPY_LOCATION src(srcResource.GetD3D12Resource(), layouts[dstSubResource]);
            CD3DX12_TEXTURE_COPY_LOCATION dst(dstResource.GetD3D12Resource(), dstSubResource);

            m_CommandList->CopyTextureRegion(&dst, x, y, z, &src, srcBox);
            m_Commands += CLCOUNT_COPY;
        }
        else if (srcResource.GetDesc().Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            // If this assert trips, you may need to increase MAX_SUBRESOURCES.  Just be wary of growing the stack too much.
            AZ_Assert(dstSubResource < CCryDX12DeviceContext::MAX_SUBRESOURCES, "Too many sub resources: (sub ID %d requested, %d allowed)", dstSubResource, CCryDX12DeviceContext::MAX_SUBRESOURCES);
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[CCryDX12DeviceContext::MAX_SUBRESOURCES];

            // From our "regular" resource, get the offset and description information for the subresource so we can copy the correct
            // part of the buffer resource.  We need to get Layouts for subresource 0 through dstSubResource so that the offset is set
            // correctly.  If we just get the Layout for dstSubResource, it will have an offset of 0.
            GetDevice()->GetD3D12Device()->GetCopyableFootprints(&srcResource.GetDesc(), 0, srcSubResource + 1, 0, layouts, nullptr, nullptr, nullptr);

            CD3DX12_TEXTURE_COPY_LOCATION src(srcResource.GetD3D12Resource(), srcSubResource);
            CD3DX12_TEXTURE_COPY_LOCATION dst(dstResource.GetD3D12Resource(), layouts[srcSubResource]);

            m_CommandList->CopyTextureRegion(&dst, x, y, z, &src, srcBox);
            m_Commands += CLCOUNT_COPY;
        }

        SetResourceFenceValue(dstResource, m_CurrentFenceValue, CMDTYPE_WRITE);
        SetResourceFenceValue(srcResource, m_CurrentFenceValue, CMDTYPE_READ);
    }

    void CommandList::UpdateSubresourceRegion(Resource& resource, UINT subResource, const D3D12_BOX* box, const void* data, UINT rowPitch, UINT slicePitch)
    {
        if (resource.InitHasBeenDeferred())
        {
            resource.TryStagingUpload(this);
        }

        DX12_COMMANDLIST_TIMER_DETAIL("UpdateSubresourceRegion");
        MaxResourceFenceValue(resource, CMDTYPE_ANY);

        ID3D12Resource* res12 = resource.GetD3D12Resource();
        const D3D12_RESOURCE_DESC& desc = resource.GetDesc();

        // Determine temporary upload buffer size (mostly only pow2-alignment of textures)
        UINT64 uploadBufferSize;
        D3D12_RESOURCE_DESC uploadDesc = desc;

        uploadDesc.Width  = box ? box->right  - box->left : uploadDesc.Width;
        uploadDesc.Height = box ? box->bottom - box->top  : uploadDesc.Height;

        GetDevice()->GetD3D12Device()->GetCopyableFootprints(&uploadDesc, subResource, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

        DX12_ASSERT((desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) || (box == nullptr), "Box used for buffer update, that's not supported!");
        DX12_ASSERT((desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) || (slicePitch != 0), "Slice-pitch is missing for UpdateSubresourceRegion(), this is a required parameter!");
        ID3D12Resource* uploadBuffer;

        D3D12_SUBRESOURCE_DATA subData;
        ZeroMemory(&subData, sizeof(D3D12_SUBRESOURCE_DATA));
        subData.pData = data;
        subData.RowPitch = rowPitch;
        subData.SlicePitch = slicePitch;
        assert(subData.pData != nullptr);

        ResourceStates resourceStates;
        const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
        const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
        if (S_OK != GetDevice()->CreateOrReuseCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(&uploadBuffer),
                resourceStates))
        {
            DX12_ERROR("Could not create intermediate upload buffer!");
            return;
        }

        // If it is reused resource,  we have to transition the recycled state to new state
        AZ_Assert(resourceStates.m_AnnouncedState == -1, "Recycled resource should not be in split state");
        QueueTransitionBarrier(uploadBuffer, resourceStates.m_CurrentState, D3D12_RESOURCE_STATE_GENERIC_READ);
        QueueTransitionBarrier(resource, D3D12_RESOURCE_STATE_COPY_DEST);
        FlushBarriers();

        ::UpdateSubresources<1>(m_CommandList, res12, uploadBuffer, 0, subResource, 1, &subData, box);
        m_Commands += CLCOUNT_COPY;

        SetResourceFenceValue(resource, m_CurrentFenceValue, CMDTYPE_WRITE);

        uploadBuffer->SetName(L"UpdateSubresourceRegion");
        GetDevice()->ReleaseLater(uploadBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, static_cast<D3D12_RESOURCE_STATES>(-1));
    }

    void CommandList::UpdateSubresources(Resource& resource, UINT64 uploadBufferSize, UINT subResources, D3D12_SUBRESOURCE_DATA* subResourceData)
    {
        if (resource.InitHasBeenDeferred())
        {
            resource.TryStagingUpload(this);
        }

        DX12_COMMANDLIST_TIMER_DETAIL("UpdateSubresources");
        MaxResourceFenceValue(resource, CMDTYPE_ANY);

        ID3D12Resource* res12 = resource.GetD3D12Resource();
        ID3D12Resource* uploadBuffer = NULL;
        ResourceStates resourceStates;
        const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
        const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
        if (S_OK != GetDevice()->CreateOrReuseCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(&uploadBuffer),
                resourceStates))
        {
            DX12_ERROR("Could not create intermediate upload buffer!");
        }

        // If it is reused resource,  we have to transition the recycled state to new state
        AZ_Assert(resourceStates.m_AnnouncedState == -1, "Recycled resource should not be in split state");
        QueueTransitionBarrier(uploadBuffer, resourceStates.m_CurrentState, D3D12_RESOURCE_STATE_GENERIC_READ);
        QueueTransitionBarrier(resource, D3D12_RESOURCE_STATE_COPY_DEST);
        FlushBarriers();

        ::UpdateSubresources(m_CommandList, res12, uploadBuffer, 0, 0, subResources, subResourceData, nullptr);
        m_Commands += CLCOUNT_COPY;

        SetResourceFenceValue(resource, m_CurrentFenceValue, CMDTYPE_WRITE);

        uploadBuffer->SetName(L"UpdateSubresources");
        GetDevice()->ReleaseLater(uploadBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, static_cast<D3D12_RESOURCE_STATES>(-1));
    }

    void CommandList::DiscardResource(Resource& resource, const D3D12_DISCARD_REGION* pRegion)
    {
        DX12_COMMANDLIST_TIMER_DETAIL("DiscardResource");
        FlushBarriers();
        MaxResourceFenceValue(resource, CMDTYPE_ANY);

        m_CommandList->DiscardResource(resource.GetD3D12Resource(), pRegion);
        m_Commands += CLCOUNT_DISCARD;
    }

    void CommandList::QueueTransitionBarrier(Resource& resource, D3D12_RESOURCE_STATES finalState)
    {
        if (resource.IsOffCard())
        {
            if (finalState != D3D12_RESOURCE_STATE_COMMON)
            {
                finalState = resource.GetRequiredResourceState();
            }
        }

        D3D12_RESOURCE_STATES currentState = resource.GetCurrentState();
        D3D12_RESOURCE_STATES announcedState = resource.GetAnnouncedState();
        bool bFinishedSplit = false;

        // Finish a split barrier.
        if (announcedState != static_cast<D3D12_RESOURCE_STATES>(-1))
        {
            m_Commands += CLCOUNT_BARRIER;

            //AZ_Printf("DX12", "QueueTransitionBarrier: Split END %p %s -> %s", resource.GetD3D12Resource(), StateToString(currentState), StateToString(announcedState));

            D3D12_RESOURCE_TRANSITION_BARRIER transition;
            transition.pResource = resource.GetD3D12Resource();
            transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            transition.StateBefore = currentState;
            transition.StateAfter = announcedState;
            m_BarrierCache.EnqueueTransition(m_CommandList, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY, transition);

            resource.SetAnnouncedState(static_cast<D3D12_RESOURCE_STATES>(-1));
            currentState = announcedState;
            resource.SetCurrentState(currentState);
            bFinishedSplit = true;
        }

        // Now check if we still need to transition to final state.
        if (currentState != finalState)
        {
            //AZ_Printf("DX12", "QueueTransitionBarrier: Transition %p %s -> %s", resource.GetD3D12Resource(), StateToString(currentState), StateToString(finalState));

            m_Commands += CLCOUNT_BARRIER;

            D3D12_RESOURCE_TRANSITION_BARRIER transition;
            transition.pResource = resource.GetD3D12Resource();
            transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            transition.StateBefore = currentState;
            transition.StateAfter = finalState;
            m_BarrierCache.EnqueueTransition(m_CommandList, D3D12_RESOURCE_BARRIER_FLAG_NONE, transition);

            resource.SetCurrentState(finalState);
        }

        // Only need UAV barrier if we didn't do a split transition here.
        else if (finalState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS && !bFinishedSplit)
        {
            QueueUAVBarrier(resource);
        }
    }


    void CommandList::QueueTransitionBarrierBegin(Resource& resource, D3D12_RESOURCE_STATES finalState)
    {
        if (resource.IsOffCard())
        {
            if (finalState != D3D12_RESOURCE_STATE_COMMON)
            {
                finalState = resource.GetRequiredResourceState();
            }
        }

        if (resource.GetAnnouncedState() != static_cast<D3D12_RESOURCE_STATES>(-1))
        {
            QueueTransitionBarrier(resource, resource.GetAnnouncedState());
        }

        D3D12_RESOURCE_STATES stateInitial = resource.GetCurrentState();
        D3D12_RESOURCE_STATES stateAnnounced = resource.GetAnnouncedState();

        if (stateInitial != finalState && stateAnnounced != finalState)
        {
            m_Commands += CLCOUNT_BARRIER;

            //AZ_Printf("DX12", "QueueTransitionBarrierBegin: Split Barrier BEGIN %p %s -> %s", resource.GetD3D12Resource(), StateToString(stateInitial), StateToString(finalState));

            D3D12_RESOURCE_TRANSITION_BARRIER transition;
            transition.pResource = resource.GetD3D12Resource();
            transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            transition.StateBefore = stateInitial;
            transition.StateAfter = finalState;

            m_BarrierCache.EnqueueTransition(m_CommandList, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY, transition);

            resource.SetAnnouncedState(finalState);
        }
    }

    void CommandList::QueueUAVBarrier(Resource& resource)
    {
        m_Commands += CLCOUNT_BARRIER;

        m_BarrierCache.EnqueueUAV(m_CommandList, resource);
    }

    void CommandList::FlushBarriers()
    {
        if (m_BarrierCache.IsFlushNeeded())
        {
            DX12_COMMANDLIST_TIMER_DETAIL("FlushBarriers");
            m_BarrierCache.Flush(m_CommandList);
        }
    }

    void CommandList::QueueTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
    {
        if (stateBefore == stateAfter)
            return;

        D3D12_RESOURCE_TRANSITION_BARRIER transition;
        transition.pResource = resource;
        transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        transition.StateBefore = stateBefore;
        transition.StateAfter = stateAfter;

        m_BarrierCache.EnqueueTransition(m_CommandList, D3D12_RESOURCE_BARRIER_FLAG_NONE, transition);
    }

    CommandListPool::CommandListPool(Device* device, CommandListFenceSet& rCmdFence, int nPoolFenceId)
        : m_pDevice(device)
        , m_rCmdFences(rCmdFence)
        , m_nPoolFenceId(nPoolFenceId)
    {
#ifdef DX12_STATS
        m_PeakNumCommandListsAllocated = 0;
        m_PeakNumCommandListsInFlight = 0;
        m_NumWaitsGPU = 0;
        m_NumWaitsCPU = 0;
#endif // DX12_STATS
    }

    CommandListPool::~CommandListPool()
    {
    }

    bool CommandListPool::Init(D3D12_COMMAND_LIST_TYPE eType)
    {
        if (!m_pCmdQueue)
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};

            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.Type = m_ePoolType = eType;
            queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            queueDesc.NodeMask = 0; // One GPU?

            ID3D12CommandQueue* pCmdQueue = NULL;
            if (S_OK != m_pDevice->GetD3D12Device()->CreateCommandQueue(&queueDesc, IID_GRAPHICS_PPV_ARGS(&pCmdQueue)))
            {
                DX12_ERROR("Could not create command queue");
                return false;
            }

            m_pCmdQueue = pCmdQueue;
            pCmdQueue->Release();

            EBUS_EVENT(AZ::Debug::EventTraceDrillerSetupBus, SetThreadName, EventTrace::GpuDetailThreadId, EventTrace::GpuDetailName);
        }

        m_AsyncCommandQueue.Init(this);

#ifdef DX12_STATS
        m_PeakNumCommandListsAllocated = 0;
        m_PeakNumCommandListsInFlight = 0;
#endif // DX12_STATS

        return true;
    }

    void CommandListPool::ScheduleCommandLists()
    {
        // Remove finished command-lists from the head of the live-list
        while (m_LiveCommandLists.size())
        {
            SmartPtr<CommandList> pCmdList = m_LiveCommandLists.front();

            // free -> complete -> submitted -> finished -> clearing -> free
            if (pCmdList->IsFinished() && !pCmdList->IsClearing())
            {
                m_LiveCommandLists.pop_front();
                m_BusyCommandLists.push_back(pCmdList);

                pCmdList->Clear();
            }
            else
            {
                break;
            }
        }

        // Submit completed but not yet submitted command-lists from the head of the live-list
        for (uint32 t = 0; t < m_LiveCommandLists.size(); ++t)
        {
            SmartPtr<CommandList> pCmdList = m_LiveCommandLists[t];

            if (pCmdList->IsScheduled() && !pCmdList->IsSubmitted())
            {
                pCmdList->Submit();
            }

            if (!pCmdList->IsSubmitted())
            {
                break;
            }
        }

        // Remove cleared/deallocated command-lists from the head of the busy-list
        while (m_BusyCommandLists.size())
        {
            SmartPtr<CommandList> pCmdList = m_BusyCommandLists.front();

            // free -> complete -> submitted -> finished -> clearing -> free
            if (pCmdList->IsFree())
            {
                m_BusyCommandLists.pop_front();
                m_FreeCommandLists.push_back(pCmdList);
            }
            else
            {
                break;
            }
        }
    }

    void CommandListPool::CreateOrReuseCommandList(SmartPtr<CommandList>& result)
    {
        if (m_FreeCommandLists.empty())
        {
            result = new CommandList(*this);
        }
        else
        {
            result = m_FreeCommandLists.front();

            m_FreeCommandLists.pop_front();
        }

        // Increment fence value on allocation, this has the effect that
        // acquired CommandLists need to be submitted in-order to prevent
        // dead-locking
        SetCurrentFenceValue(GetCurrentFenceValue() + 1);

        result->Init(GetCurrentFenceValue());
        m_LiveCommandLists.push_back(result);

#ifdef DX12_STATS
        m_PeakNumCommandListsAllocated = std::max(m_PeakNumCommandListsAllocated, m_LiveCommandLists.size() + m_BusyCommandLists.size() + m_FreeCommandLists.size());
        m_PeakNumCommandListsInFlight = std::max(m_PeakNumCommandListsInFlight, m_LiveCommandLists.size());
#endif // DX12_STATS
    }

    void CommandListPool::AcquireCommandList(SmartPtr<CommandList>& result)
    {
        static CryCriticalSectionNonRecursive csThreadSafeScope;
        CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(csThreadSafeScope);

        ScheduleCommandLists();

        CreateOrReuseCommandList(result);
    }

    void CommandListPool::ForfeitCommandList(SmartPtr<CommandList>& result, bool bWait)
    {
        static CryCriticalSectionNonRecursive csThreadSafeScope;
        CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(csThreadSafeScope);
        SmartPtr<CommandList> pWaitable = result;

        DX12_ASSERT(result->IsCompleted(), "It's not possible to forfeit an unclosed command list!");
        result->Schedule();
        result = nullptr;

        ScheduleCommandLists();

        if (bWait)
        {
            pWaitable->WaitForFinishOnCPU();
        }
    }

    void CommandListPool::AcquireCommandLists(uint32 numCLs, SmartPtr<CommandList>* results)
    {
        static CryCriticalSectionNonRecursive csThreadSafeScope;
        CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(csThreadSafeScope);

        ScheduleCommandLists();

        for (uint32 i = 0; i < numCLs; ++i)
        {
            CreateOrReuseCommandList(results[i]);
        }
    }

    void CommandListPool::ForfeitCommandLists(uint32 numCLs, SmartPtr<CommandList>* results, bool bWait)
    {
        static CryCriticalSectionNonRecursive csThreadSafeScope;
        CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(csThreadSafeScope);
        SmartPtr<CommandList> pWaitable = results[numCLs - 1];

        for (uint32 i = 0; i < numCLs; ++i)
        {
            DX12_ASSERT(results[i]->IsCompleted(), "It's not possible to forfeit an unclosed command list!");
            results[i]->Schedule();
            results[i] = nullptr;
        }

        ScheduleCommandLists();

        if (bWait)
        {
            pWaitable->WaitForFinishOnCPU();
        }
    }
}
