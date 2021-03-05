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

#include "DX12CommandListFence.hpp"
#include "DX12PSO.hpp"
#include "DX12DescriptorHeap.hpp"
#include "DX12QueryHeap.hpp"
#include "DX12View.hpp"
#include "DX12SamplerState.hpp"
#include "DX12AsyncCommandQueue.hpp"
#include "DX12Resource.hpp"
#include "DX12ResourceBarrierCache.h"
#include "DX12TimerHeap.h"

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_map.h>

#if defined(AZ_RESTRICTED_PLATFORM)
    #undef AZ_RESTRICTED_SECTION
    #define DX12COMMANDLIST_HPP_SECTION_1 1
#endif

namespace DX12
{
    class CommandList;

    class CommandListPool
    {
    public:
        CommandListPool(Device* device, CommandListFenceSet& rCmdFence, int nPoolFenceId);
        ~CommandListPool();

        bool Init(D3D12_COMMAND_LIST_TYPE eType = D3D12_COMMAND_LIST_TYPE_DIRECT);
        void AcquireCommandList(SmartPtr<CommandList> & result);
        void ForfeitCommandList(SmartPtr<CommandList> & result, bool bWait = false);
        void AcquireCommandLists(uint32 numCLs, SmartPtr<CommandList> * results);
        void ForfeitCommandLists(uint32 numCLs, SmartPtr<CommandList> * results, bool bWait = false);

        inline ID3D12CommandQueue* GetD3D12CommandQueue() const
        {
            return m_pCmdQueue;
        }

        inline D3D12_COMMAND_LIST_TYPE GetD3D12QueueType() const
        {
            return m_ePoolType;
        }

        inline DX12::AsyncCommandQueue& GetAsyncCommandQueue()
        {
            return m_AsyncCommandQueue;
        }

        inline Device* GetDevice() const
        {
            return m_pDevice;
        }

        inline CommandListFenceSet& GetFences()
        {
            return m_rCmdFences;
        }

        inline ID3D12Fence** GetD3D12Fences() const
        {
            return m_rCmdFences.GetD3D12Fences();
        }

        inline ID3D12Fence* GetD3D12Fence() const
        {
            return m_rCmdFences.GetD3D12Fence(m_nPoolFenceId);
        }

        inline int GetFenceID() const
        {
            return m_nPoolFenceId;
        }

        inline void SetSubmittedFenceValue(const UINT64 fenceValue)
        {
            return m_rCmdFences.SetSubmittedValue(fenceValue, m_nPoolFenceId);
        }

        inline UINT64 GetSubmittedFenceValue() const
        {
            return m_rCmdFences.GetSubmittedValue(m_nPoolFenceId);
        }

    private:
        inline void SetCurrentFenceValue(const UINT64 fenceValue)
        {
            return m_rCmdFences.SetCurrentValue(fenceValue, m_nPoolFenceId);
        }

    public:
        inline UINT64 GetLastCompletedFenceValue() const
        {
            return m_rCmdFences.GetLastCompletedFenceValue(m_nPoolFenceId);
        }

        inline UINT64 GetCurrentFenceValue() const
        {
            return m_rCmdFences.GetCurrentValue(m_nPoolFenceId);
        }

        inline void WaitForFenceOnGPU(const std::atomic<AZ::u64> (&fenceValues)[CMDQUEUE_NUM])
        {
            // The pool which waits for the fence can be omitted (in-order-execution)
            UINT64 fenceValuesMasked[CMDQUEUE_NUM];

            fenceValuesMasked[CMDQUEUE_COPY    ] = fenceValues[CMDQUEUE_COPY    ];
            fenceValuesMasked[CMDQUEUE_GRAPHICS] = fenceValues[CMDQUEUE_GRAPHICS];
            fenceValuesMasked[m_nPoolFenceId   ] = 0;

            if (!m_rCmdFences.IsCompleted(fenceValuesMasked))
            {
                DX12_LOG("Waiting for GPU fences (type: %d) [%d,%d] -> [%d,%d]",
                    m_nPoolFenceId,
                    m_rCmdFences.GetD3D12Fence(CMDQUEUE_COPY)->GetCompletedValue(),
                    m_rCmdFences.GetD3D12Fence(CMDQUEUE_GRAPHICS)->GetCompletedValue(),
                    static_cast<AZ::u64>(fenceValues[CMDQUEUE_COPY    ]),
                    static_cast<AZ::u64>(fenceValues[CMDQUEUE_GRAPHICS]));
#ifdef DX12_STATS
                m_NumWaitsGPU += 2;
#endif // DX12_STATS

                m_AsyncCommandQueue.Wait(m_rCmdFences.GetD3D12Fences(), fenceValuesMasked);
            }
        }

        inline void WaitForFenceOnGPU(const UINT64 fenceValue, const int id)
        {
            if (!m_rCmdFences.IsCompleted(fenceValue, id))
            {
                DX12_LOG("Waiting for GPU fence %d -> %d", m_rCmdFences.GetD3D12Fence(id)->GetCompletedValue(), fenceValue);

#ifdef DX12_STATS
                m_NumWaitsGPU++;
#endif // DX12_STATS

                m_AsyncCommandQueue.Wait(m_rCmdFences.GetD3D12Fence(id), fenceValue);
            }
        }

        inline void WaitForFenceOnGPU(const UINT64 fenceValue)
        {
            return WaitForFenceOnGPU(fenceValue, m_nPoolFenceId);
        }

        inline void WaitForFenceOnCPU(const std::atomic<AZ::u64> (&fenceValues)[CMDQUEUE_NUM]) const
        {
            AZ_TRACE_METHOD();
            UINT64 fenceValuesCopied[CMDQUEUE_NUM];

            fenceValuesCopied[CMDQUEUE_COPY    ] = fenceValues[CMDQUEUE_COPY    ];
            fenceValuesCopied[CMDQUEUE_GRAPHICS] = fenceValues[CMDQUEUE_GRAPHICS];

            if (!m_rCmdFences.IsCompleted(fenceValuesCopied))
            {
#ifdef DX12_STATS
                m_NumWaitsCPU += 2;
#endif // DX12_STATS

                m_rCmdFences.WaitForFence(fenceValuesCopied);
            }
        }

        inline void WaitForFenceOnCPU(const UINT64 fenceValue, const int id) const
        {
            if (!m_rCmdFences.IsCompleted(fenceValue, id))
            {
#ifdef DX12_STATS
                m_NumWaitsCPU++;
#endif // DX12_STATS

                m_rCmdFences.WaitForFence(fenceValue, id);
            }
        }

        inline void WaitForFenceOnCPU(const UINT64 fenceValue) const
        {
            return WaitForFenceOnCPU(fenceValue, m_nPoolFenceId);
        }

        inline bool IsCompleted(const std::atomic<AZ::u64> (&fenceValues)[CMDQUEUE_NUM]) const
        {
            // The pool which checks for the fence can be omitted (in-order-execution)
            UINT64 fenceValuesMasked[CMDQUEUE_NUM];

            fenceValuesMasked[CMDQUEUE_COPY    ] = fenceValues[CMDQUEUE_COPY    ];
            fenceValuesMasked[CMDQUEUE_GRAPHICS] = fenceValues[CMDQUEUE_GRAPHICS];
            fenceValuesMasked[m_nPoolFenceId   ] = 0;

            return m_rCmdFences.IsCompleted(fenceValuesMasked);
        }

        inline bool IsCompleted(const UINT64 fenceValue, const int id) const
        {
            return m_rCmdFences.IsCompleted(fenceValue, id);
        }

        inline bool IsCompleted(const UINT64 fenceValue) const
        {
            return IsCompleted(fenceValue, m_nPoolFenceId);
        }

    private:
        void ScheduleCommandLists();
        void CreateOrReuseCommandList(SmartPtr<CommandList> & result);

        Device* m_pDevice;
        CommandListFenceSet& m_rCmdFences;
        int m_nPoolFenceId;
        D3D12_COMMAND_LIST_TYPE m_ePoolType;

        typedef std::deque<SmartPtr<CommandList>> TCommandLists;
        TCommandLists m_LiveCommandLists;
        TCommandLists m_BusyCommandLists;
        TCommandLists m_FreeCommandLists;

        SmartPtr<ID3D12CommandQueue> m_pCmdQueue;
        DX12::AsyncCommandQueue m_AsyncCommandQueue;

#ifdef DX12_STATS
        size_t m_NumWaitsGPU;
        mutable size_t m_NumWaitsCPU;
        size_t m_PeakNumCommandListsAllocated;
        size_t m_PeakNumCommandListsInFlight;
#endif // DX12_STATS
    };

    class CommandList
        : public DeviceObject
    {
        friend class CommandListPool;

    public:
        virtual ~CommandList();

        bool Init(UINT64 currentFenceValue);
        bool Reset();

        inline ID3D12GraphicsCommandList* GetD3D12CommandList() const
        {
            return m_CommandList;
        }
        inline D3D12_COMMAND_LIST_TYPE GetD3D12ListType() const
        {
            return m_eListType;
        }
        inline ID3D12CommandAllocator* GetD3D12CommandAllocator() const
        {
            return m_pCmdAllocator;
        }
        inline ID3D12CommandQueue* GetD3D12CommandQueue() const
        {
            return m_pCmdQueue;
        }
        inline CommandListPool& GetCommandListPool() const
        {
            return m_rPool;
        }

        // Stage 1
        inline bool IsFree() const
        {
            return m_State == CLSTATE_FREE;
        }

        // Stage 2
        void Begin();
        inline bool IsUtilized() const
        {
#define CLCOUNT_DRAW        1
#define CLCOUNT_DISPATCH    1
#define CLCOUNT_COPY        1
#define CLCOUNT_CLEAR       1
#define CLCOUNT_DISCARD     1
#define CLCOUNT_BARRIER     1
#define CLCOUNT_QUERY       1
#define CLCOUNT_QUERY_TIMESTAMP 0
#define CLCOUNT_RESOLVE     1
#define CLCOUNT_SETIO       0
#define CLCOUNT_SETSTATE    0

            return m_Commands > 0;
        }

        void End();
        inline bool IsCompleted() const
        {
            return m_State >= CLSTATE_COMPLETED;
        }

        void Schedule();
        inline bool IsScheduled() const
        {
            return m_State == CLSTATE_SCHEDULED;
        }

        // Stage 3
        UINT64 SignalFenceOnGPU()
        {
            DX12_LOG("Signaling fence value on GPU (%d) : %d", m_rPool.GetFenceID(), m_CurrentFenceValue);
            m_rPool.GetAsyncCommandQueue().Signal(m_rPool.GetD3D12Fence(), m_CurrentFenceValue);
            return m_CurrentFenceValue;
        }

        UINT64 SignalFenceOnCPU()
        {
            DX12_LOG("Signaling fence value on CPU: %d", m_CurrentFenceValue);
            m_rPool.GetD3D12Fence()->Signal(m_CurrentFenceValue);
            return m_CurrentFenceValue;
        }

        void Submit();
        inline bool IsSubmitted() const
        {
            return m_State >= CLSTATE_SUBMITTED;
        }

        // Stage 4
        void WaitForFinishOnGPU()
        {
            DX12_ASSERT(m_State == CLSTATE_SUBMITTED, "GPU fence waits for itself to be complete: deadlock imminent!");
            m_rPool.WaitForFenceOnGPU(m_CurrentFenceValue);
        }

        void WaitForFinishOnCPU() const
        {
            DX12_ASSERT(m_State == CLSTATE_SUBMITTED, "CPU fence waits for itself to be complete: deadlock imminent!");
            m_rPool.WaitForFenceOnCPU(m_CurrentFenceValue);
        }

        inline bool IsFinished() const
        {
            if ((m_State == CLSTATE_SUBMITTED) && m_rPool.IsCompleted(m_CurrentFenceValue))
            {
                m_State = CLSTATE_FINISHED;
            }

            return m_State == CLSTATE_FINISHED;
        }

        inline UINT64 GetCurrentFenceValue() const
        {
            return m_CurrentFenceValue;
        }

        // Stage 5
        void Clear();
        inline bool IsClearing() const
        {
            return m_State == CLSTATE_CLEARING;
        }

        void SetPipelineState(const PipelineState* pso);
        void SetRootSignature(CommandMode commandMode, const RootSignature* rootSignature);

        void QueueUAVBarrier(Resource& resource);

        void QueueTransitionBarrier(Resource& resource, D3D12_RESOURCE_STATES desiredState);

        void QueueTransitionBarrierBegin(Resource& resource, D3D12_RESOURCE_STATES desiredState);

        void FlushBarriers();

        // Collect the highest fenceValues of all given Resource to wait before the command-list is finally submitted
        inline void MaxResourceFenceValue(Resource& resource, const int type)
        {
            resource.MaxFenceValues(m_UsedFenceValues, type);
        }

        // Mark resource with desired fence value, returns the previous fence value
        inline UINT64 SetResourceFenceValue(Resource& resource, const UINT64 fenceValue, const int type) const
        {
            return resource.SetFenceValue(fenceValue, m_rPool.GetFenceID(), type);
        }

        inline void TrackResourceUsage(Resource& resource, D3D12_RESOURCE_STATES stateUsage, int cmdBlocker = CMDTYPE_WRITE, int cmdUsage = CMDTYPE_READ)
        {
            resource.TryStagingUpload(this);
            MaxResourceFenceValue(resource, cmdBlocker);
            QueueTransitionBarrier(resource, stateUsage);
            SetResourceFenceValue(resource, m_CurrentFenceValue, cmdUsage);
        }

        inline void TrackResourceIBVUsage(Resource& resource) { TrackResourceUsage(resource, D3D12_RESOURCE_STATE_INDEX_BUFFER, CMDTYPE_WRITE, CMDTYPE_READ); }
        inline void TrackResourceVBVUsage(Resource& resource) { TrackResourceUsage(resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, CMDTYPE_WRITE, CMDTYPE_READ); }
        inline void TrackResourceCBVUsage(Resource& resource) { TrackResourceUsage(resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, CMDTYPE_WRITE, CMDTYPE_READ); }
        inline void TrackResourceSRVUsage(Resource& resource) { TrackResourceUsage(resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | GetPreserveFlags(resource), CMDTYPE_WRITE, CMDTYPE_READ); }
        inline void TrackResourceUAVUsage(Resource& resource) { TrackResourceUsage(resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS | GetPreserveFlags(resource), CMDTYPE_ANY, CMDTYPE_ANY); }
        inline void TrackResourceDSVUsage(Resource& resource) { TrackResourceUsage(resource, D3D12_RESOURCE_STATE_DEPTH_WRITE | GetPreserveFlags(resource), CMDTYPE_ANY, CMDTYPE_WRITE); }
        inline void TrackResourceDRVUsage(Resource& resource) { TrackResourceUsage(resource, D3D12_RESOURCE_STATE_DEPTH_READ | GetPreserveFlags(resource), CMDTYPE_WRITE, CMDTYPE_READ); }
        inline void TrackResourceRTVUsage(Resource& resource) { TrackResourceUsage(resource, D3D12_RESOURCE_STATE_RENDER_TARGET | GetPreserveFlags(resource), CMDTYPE_ANY, CMDTYPE_WRITE); }

        bool IsFull(size_t numResources, size_t numSamplers, size_t numRendertargets, size_t numDepthStencils) const;
        bool IsUsedByOutputViews(const Resource& res) const;

        inline void IncrementInputCursors(size_t numResources, size_t numSamplers)
        {
            GetResourceHeap().IncrementCursor(numResources);
            GetSamplerHeap().IncrementCursor(numSamplers);
        }

        inline void IncrementOutputCursors()
        {
            GetRenderTargetHeap().IncrementCursor(m_CurrentNumRTVs);
            if (m_pDSV)
            {
                GetDepthStencilHeap().IncrementCursor();
            }
        }

        void WriteShaderResourceDescriptor(const ResourceView* resourceView, INT descriptorOffset);
        void WriteUnorderedAccessDescriptor(const ResourceView* resourceView, INT descriptorOffset);
        void WriteConstantBufferDescriptor(const ResourceView* resourceView, INT descriptorOffset, UINT byteOffset, UINT byteCount);
        void WriteSamplerStateDescriptor(const SamplerState* state, INT descriptorOffset);

        void BindVertexBufferView(const ResourceView&view, INT offset, const TRange<UINT>&bindRange, UINT bindStride, D3D12_VERTEX_BUFFER_VIEW (&heap)[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT]);
        inline void BindVertexBufferView(const ResourceView& view, INT offset, const TRange<UINT>& bindRange, UINT bindStride)
        {
            BindVertexBufferView(view, offset, bindRange, bindStride, m_VertexBufferHeap);
        }

        void SetConstantBufferView(CommandMode commandMode, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
        void Set32BitConstants(CommandMode commandMode, UINT RootParameterIndex, UINT Num32BitValuesToSet, const void* pSrcData, UINT DestOffsetIn32BitValues);
        void SetDescriptorTable(CommandMode commandMode, UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

        void SetDescriptorTables(CommandMode commandMode, D3D12_DESCRIPTOR_HEAP_TYPE eType);

        void ClearVertexBufferHeap(UINT num);
        void SetVertexBufferHeap(UINT num);
        void BindAndSetIndexBufferView(const ResourceView& view, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, UINT offset = 0);

        void SetResourceAndSamplerStateHeaps();
        void BindAndSetOutputViews(INT numRTVs, const ResourceView** rtv, const ResourceView* dsv);

        void ClearDepthStencilView(const ResourceView& view, D3D12_CLEAR_FLAGS clearFlags, float depthValue, UINT stencilValue, UINT NumRects = 0U, const D3D12_RECT* pRect = nullptr);
        void ClearRenderTargetView(const ResourceView& view, const FLOAT rgba[4], UINT NumRects = 0U, const D3D12_RECT* pRect = nullptr);
        void ClearUnorderedAccessView(const ResourceView& view, const UINT rgba[4], UINT NumRects = 0U, const D3D12_RECT* pRect = nullptr);
        void ClearUnorderedAccessView(const ResourceView& view, const FLOAT rgba[4], UINT NumRects = 0U, const D3D12_RECT* pRect = nullptr);
        void ClearView(const ResourceView& view, const FLOAT rgba[4], UINT NumRects = 0U, const D3D12_RECT* pRect = nullptr);

        void CopyResource(Resource& pDstResource, Resource& pSrcResource);
        void CopySubresource(Resource& pDestResource, UINT destSubResource, UINT x, UINT y, UINT z, Resource& pSrcResource, UINT srcSubResource, const D3D12_BOX* srcBox);
        void UpdateSubresourceRegion(Resource& resource, UINT subResource, const D3D12_BOX* box, const void* data, UINT rowPitch, UINT depthPitch);
        void UpdateSubresources(Resource& rResource, UINT64 uploadBufferSize, UINT subResources, D3D12_SUBRESOURCE_DATA* subResourceData);
        void DiscardResource(Resource& resource, const D3D12_DISCARD_REGION* pRegion);

        void PresentRenderTargetView(SwapChain* pDX12SwapChain);

        inline void DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation)
        {
            DX12_COMMANDLIST_TIMER_DETAIL("DrawInstanced");
#ifdef DX12_STATS
            m_NumDraws++;
#endif // DX12_STATS

            FlushBarriers();
            m_CommandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
            m_Commands += CLCOUNT_DRAW;
        }

        inline void DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, UINT baseVertexLocation, UINT startInstanceLocation)
        {
            DX12_COMMANDLIST_TIMER_DETAIL("DrawIndexedInstanced");
#ifdef DX12_STATS
            m_NumDraws++;
#endif // DX12_STATS

            FlushBarriers();
            m_CommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
            m_Commands += CLCOUNT_DRAW;
        }

        inline void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
        {
            DX12_COMMANDLIST_TIMER_DETAIL("Dispatch");
#ifdef DX12_STATS
            m_NumDraws++;
#endif // DX12_STATS

            FlushBarriers();
            m_CommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
            m_Commands += CLCOUNT_DISPATCH;
        }

        inline void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topo)
        {
            m_CommandList->IASetPrimitiveTopology(topo);
            m_Commands += CLCOUNT_SETIO;
        }

        inline void SetViewports(UINT NumViewports, const D3D12_VIEWPORT* pViewports)
        {
            m_CommandList->RSSetViewports(NumViewports, pViewports);
            m_Commands += CLCOUNT_SETSTATE;
        }

        inline void SetScissorRects(UINT NumRects, const D3D12_RECT* pRects)
        {
            m_CommandList->RSSetScissorRects(NumRects, pRects);
            m_Commands += CLCOUNT_SETSTATE;
        }

        inline void SetStencilRef(UINT Ref)
        {
            m_CommandList->OMSetStencilRef(Ref);
            m_Commands += CLCOUNT_SETSTATE;
        }

        inline void BeginQuery(QueryHeap& queryHeap, D3D12_QUERY_TYPE Type, UINT Index)
        {
            m_CommandList->BeginQuery(queryHeap.GetD3D12QueryHeap(), Type, Index);
            m_Commands += Type == D3D12_QUERY_TYPE_TIMESTAMP ? CLCOUNT_QUERY_TIMESTAMP : CLCOUNT_QUERY;
        }

        inline void EndQuery(QueryHeap& queryHeap, D3D12_QUERY_TYPE Type, UINT Index)
        {
            m_CommandList->EndQuery(queryHeap.GetD3D12QueryHeap(), Type, Index);
            m_Commands += Type == D3D12_QUERY_TYPE_TIMESTAMP ? CLCOUNT_QUERY_TIMESTAMP : CLCOUNT_QUERY;
        }

        inline void ResolveQueryData(QueryHeap& queryHeap, D3D12_QUERY_TYPE Type,
            _In_  UINT StartIndex,
            _In_  UINT NumQueries,
            _In_  ID3D12Resource* pDestinationBuffer,
            _In_  UINT64 AlignedDestinationBufferOffset)
        {
            m_CommandList->ResolveQueryData(queryHeap.GetD3D12QueryHeap(), Type, StartIndex, NumQueries, pDestinationBuffer, AlignedDestinationBufferOffset);
            m_Commands += CLCOUNT_RESOLVE;
        }

    private:
        CommandList(CommandListPool& pPool);
        CommandListPool& m_rPool;

        void ResetStateTracking(CommandMode commandMode);
        void BindDepthStencilView(const ResourceView& dsv);
        void BindRenderTargetView(const ResourceView& rtv);

        D3D12_RESOURCE_STATES GetPreserveFlags([[maybe_unused]] Resource& resource)
        {
            D3D12_RESOURCE_STATES preserveFlags = D3D12_RESOURCE_STATES(0U);
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DX12COMMANDLIST_HPP_SECTION_1
    #include AZ_RESTRICTED_FILE(DX12CommandList_hpp)
#endif
            return preserveFlags;
        }

        void QueueTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);
        void SubmitTimers();

        ID3D12Device* m_pD3D12Device;
        SmartPtr<ID3D12GraphicsCommandList> m_CommandList;
        SmartPtr<ID3D12CommandAllocator> m_pCmdAllocator;
        SmartPtr<ID3D12CommandQueue> m_pCmdQueue;
        std::atomic<AZ::u64> m_UsedFenceValues[CMDTYPE_NUM][CMDQUEUE_NUM];

        ResourceBarrierCache m_BarrierCache;

        // Only used by IsUsedByOutputViews()
        const ResourceView* m_pDSV;
        const ResourceView* m_pRTV[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
        INT m_CurrentNumRTVs;

        UINT64 m_CurrentFenceValue;
        D3D12_COMMAND_LIST_TYPE m_eListType;

        DescriptorBlock m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

        inline       DescriptorBlock& GetResourceHeap    ()       { return m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]; }
        inline       DescriptorBlock& GetSamplerHeap     ()       { return m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]; }
        inline       DescriptorBlock& GetRenderTargetHeap()       { return m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]; }
        inline       DescriptorBlock& GetDepthStencilHeap()       { return m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]; }

        inline const DescriptorBlock& GetResourceHeap    () const { return m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]; }
        inline const DescriptorBlock& GetSamplerHeap     () const { return m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]; }
        inline const DescriptorBlock& GetRenderTargetHeap() const { return m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]; }
        inline const DescriptorBlock& GetDepthStencilHeap() const { return m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]; }

        UINT m_CurrentNumVertexBuffers;
        D3D12_VERTEX_BUFFER_VIEW m_VertexBufferHeap[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];

        static const AZ::u32 RootParameterCountMax = 16;
        struct RootParameter
        {
            union
            {
                D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
                D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle;
            };
        };

        RootParameter m_CurrentRootParameters[CommandModeCount][RootParameterCountMax];
        const PipelineState* m_CurrentPipelineState;
        const RootSignature* m_CurrentRootSignature[CommandModeCount];

        UINT m_Commands;
        mutable enum
        {
            CLSTATE_FREE,
            CLSTATE_STARTED,
            CLSTATE_UTILIZED,
            CLSTATE_COMPLETED,
            CLSTATE_SCHEDULED,
            CLSTATE_SUBMITTED,
            CLSTATE_FINISHED,
            CLSTATE_CLEARING
        }
        m_State;

        UINT m_nodeMask;

#ifdef DX12_STATS
        size_t m_NumDraws;
        size_t m_NumWaitsGPU;
        size_t m_NumWaitsCPU;
#endif // DX12_STATS

#if DX12_GPU_PROFILE_MODE != DX12_GPU_PROFILE_MODE_OFF
        TimerHeap m_Timers;
        TimerHandle m_TimerHandle;
        static const AZ::u32 TimerCountMax = 2048;
#endif
    };
}
