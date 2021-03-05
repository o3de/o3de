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

#include "DX12CommandList.hpp"

#if defined(AZ_RESTRICTED_PLATFORM)
    #undef AZ_RESTRICTED_SECTION
    #define DX12DEVICE_HPP_SECTION_1 1
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DX12DEVICE_HPP_SECTION_1
    #include AZ_RESTRICTED_FILE(DX12Device_hpp)
#else
    #include <d3d12.h>
#endif
#include "DX12/API/DX12PSO.hpp"

#include <AzCore/std/containers/unordered_map.h>

namespace DX12
{
    enum DescriptorPoolType
    {
        DescriptorPoolSRV,
        DescriptorPoolUAV,
        DescriptorPoolNum
    };
    class Device : public AzRHI::ReferenceCounted
    {
    public:
        enum class ResourceReleasePolicy
        {
            Immediate,
            Deferred
        };

        static Device* Create(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL* pFeatureLevel);
        virtual ~Device();

        inline ID3D12Device* GetD3D12Device() const
        {
            return m_Device;
        }

        inline PipelineStateCache& GetPSOCache()
        {
            return m_PipelineStateCache;
        }

        inline const PipelineStateCache& GetPSOCache() const
        {
            return m_PipelineStateCache;
        }

        inline RootSignatureCache& GetRootSignatureCache()
        {
            return m_RootSignatureCache;
        }

        inline const RootSignatureCache& GetRootSignatureCache() const
        {
            return m_RootSignatureCache;
        }

        DescriptorBlock GetGlobalDescriptorBlock(D3D12_DESCRIPTOR_HEAP_TYPE eType, AZ::u32 size);

        D3D12_CPU_DESCRIPTOR_HANDLE CacheSampler(const D3D12_SAMPLER_DESC* pDesc);
        D3D12_CPU_DESCRIPTOR_HANDLE CacheShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D12Resource* pResource);
        D3D12_CPU_DESCRIPTOR_HANDLE CacheUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc, ID3D12Resource* pResource);
        D3D12_CPU_DESCRIPTOR_HANDLE CacheDepthStencilView(const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc, ID3D12Resource* pResource);
        D3D12_CPU_DESCRIPTOR_HANDLE CacheRenderTargetView(const D3D12_RENDER_TARGET_VIEW_DESC* pDesc, ID3D12Resource* pResource);

        void RecycleDescriptorHandle(DescriptorPoolType poolType, D3D12_CPU_DESCRIPTOR_HANDLE handle);

        HRESULT STDMETHODCALLTYPE CreateOrReuseCommittedResource(
            _In_  const D3D12_HEAP_PROPERTIES* pHeapProperties,
            D3D12_HEAP_FLAGS HeapFlags,
            _In_  const D3D12_RESOURCE_DESC* pResourceDesc,
            D3D12_RESOURCE_STATES InitialResourceState,
            _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
            REFIID riidResource,
            _COM_Outptr_opt_  void** ppvResource,
            ResourceStates& resourceStates);

        void FlushReleaseHeap(ResourceReleasePolicy releasePolicy);
        void ReleaseLater(ID3D12Resource* pObject, D3D12_RESOURCE_STATES currentState, D3D12_RESOURCE_STATES announcedState);

        void FinishFrame();
        void CalibrateClocks(ID3D12CommandQueue* presentQueue);

        AZ::u64 MakeCpuTimestamp(AZ::u64 gpuTimestamp) const;
        AZ::u64 MakeCpuTimestampMicroseconds(AZ::u64 gpuTimestamp) const;

        inline AZ::u64 GetGpuTimestampFrequency() const
        {
            return m_CalibratedGpuTimestampFrequency;
        }

        inline D3D12_CPU_DESCRIPTOR_HANDLE GetNullShaderResourceView() const
        {
            return m_NullSRV;
        }

        inline D3D12_CPU_DESCRIPTOR_HANDLE GetNullUnorderedAccessView() const
        {
            return m_NullUAV;
        }

        inline D3D12_CPU_DESCRIPTOR_HANDLE GetNullSampler() const
        {
            return m_NullSampler;
        }

    private:
        Device(ID3D12Device* d3d12Device);

        SmartPtr<ID3D12Device> m_Device;

        PipelineStateCache m_PipelineStateCache;
        RootSignatureCache m_RootSignatureCache;

        DescriptorHeap m_SamplerCache;
        DescriptorHeap m_ShaderResourceDescriptorCache;
        DescriptorHeap m_UnorderedAccessDescriptorCache;
        DescriptorHeap m_DepthStencilDescriptorCache;
        DescriptorHeap m_RenderTargetDescriptorCache;
        DescriptorHeap m_GlobalDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

        D3D12_CPU_DESCRIPTOR_HANDLE m_NullSRV;
        D3D12_CPU_DESCRIPTOR_HANDLE m_NullUAV;
        D3D12_CPU_DESCRIPTOR_HANDLE m_NullSampler;

        // Free pool of descriptor handler offset
        AZStd::list<AZStd::pair<AZ::u32, AZ::u32>> m_DescriptorPools[DescriptorPoolNum];

        struct ReleaseInfo
        {
            ID3D12Resource* resource;
            ResourceStates resourceStates;
            AZ::s32 frameNumber;
        };

        AZStd::unordered_multimap<THash, ReleaseInfo> m_ReleaseHeap;
        AZStd::unordered_multimap<THash, ReleaseInfo> m_RecycleHeap;

        AZ::u32 m_FrameCounter;
        const AZ::u32 AllowedGPUFramesLatency = 5;

        AZ::u64 m_CalibratedGpuTimestampFrequency;
        AZ::u64 m_CalibratedGpuTimestamp;
        AZ::u64 m_CalibratedCpuTimestampFrequency;
        AZ::u64 m_CalibratedCpuTimestamp;
    };
}
