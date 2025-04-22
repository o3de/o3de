/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Device_Platform.h>

#include <Atom/RHI/ObjectCache.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Format.h>
#include <RHI/CommandListPool.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/PipelineLayout.h>
#include <RHI/ReleaseQueue.h>
#include <RHI/StagingMemoryAllocator.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/Image.h>
#include <RHI/Sampler.h>
#include <Atom/RHI/ThreadLocalContext.h>
#include <AtomCore/std/containers/lru_cache.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

#define USE_AMD_D3D12MA
#ifdef USE_AMD_D3D12MA
#include <dx12ma/D3D12MemAlloc.h>

AZ_DX12_REFCOUNTED(D3D12MA::Allocator);
AZ_DX12_REFCOUNTED(D3D12MA::Allocation);
#endif

namespace AZ
{
    namespace RHI
    {
        struct ImageDescriptor;
        struct BufferDescriptor;
        struct ClearValue;
    }

    namespace DX12
    {
        class PhysicalDevice;
        class Buffer;
        class Image;

        class Device
            : public Device_Platform
        {
            using Base = Device_Platform;
        public:
            AZ_CLASS_ALLOCATOR(Device, AZ::SystemAllocator);
            AZ_RTTI(Device, "{89670048-DC28-434D-A0BE-C76165419769}", Base);

            static RHI::Ptr<Device> Create();

            RHI::ResultCode CreateSwapChain(
                IUnknown* window,
                const DXGI_SWAP_CHAIN_DESCX& swapChainDesc,
                RHI::Ptr<IDXGISwapChainX>& swapChain);

            RHI::ResultCode CreateSwapChain(
                const DXGI_SWAP_CHAIN_DESCX& swapChainDesc,
                AZStd::array<RHI::Ptr<ID3D12Resource>, RHI::Limits::Device::FrameCountMax>& outSwapChainResources);

            void GetImageAllocationInfo(
                const RHI::ImageDescriptor& descriptor,
                D3D12_RESOURCE_ALLOCATION_INFO& info);

            void GetPlacedImageAllocationInfo(
                const RHI::ImageDescriptor& descriptor,
                D3D12_RESOURCE_ALLOCATION_INFO& info);

#ifdef USE_AMD_D3D12MA
            MemoryView CreateD3d12maBuffer(
                const RHI::BufferDescriptor& bufferDescriptor,
                D3D12_RESOURCE_STATES initialState,
                D3D12_HEAP_TYPE heapType);
#endif
            MemoryView CreateBufferCommitted(
                const RHI::BufferDescriptor& bufferDescriptor,
                D3D12_RESOURCE_STATES initialState,
                D3D12_HEAP_TYPE heapType);

            MemoryView CreateImageCommitted(
                const RHI::ImageDescriptor& imageDescriptor,
                const RHI::ClearValue* optimizedClearValue,
                D3D12_RESOURCE_STATES initialState,
                D3D12_HEAP_TYPE heapType);

            MemoryView CreateBufferPlaced(
                const RHI::BufferDescriptor& bufferDescriptor,
                D3D12_RESOURCE_STATES initialState,
                ID3D12Heap* heap,
                size_t heapByteOffset);

            MemoryView CreateImagePlaced(
                const RHI::ImageDescriptor& imageDescriptor,
                const RHI::ClearValue* optimizedClearValue,
                D3D12_RESOURCE_STATES initialState,
                ID3D12Heap* heap,
                size_t heapByteOffset);

            MemoryView CreateImageReserved(
                const RHI::ImageDescriptor& imageDescriptor,
                D3D12_RESOURCE_STATES initialState,
                ImageTileLayout& imageTilingInfo);

            //! Queues a DX12 COM object for release (by taking a reference) after the current frame has flushed
            //! through the GPU.
            void QueueForRelease(RHI::Ptr<ID3D12Object> dx12Object);

            //! Queues the backing Memory instance of a MemoryView for release (by taking a reference) after the
            //! current frame has flushed through the GPU. The reference on the MemoryView itself is not released.
            void QueueForRelease(const MemoryView& memoryView);

            //! Allocates host memory from the internal frame allocator that is suitable for staging
            //! uploads to the GPU for the current frame. The memory is valid for the lifetime of
            //! the frame and is automatically reclaimed after the frame has completed on the GPU.
            MemoryView AcquireStagingMemory(size_t size, size_t alignment);

            //! Acquires a pipeline layout from the internal cache.
            RHI::ConstPtr<PipelineLayout> AcquirePipelineLayout(const RHI::PipelineLayoutDescriptor& descriptor);

            //! Acquires a new command list for the frame given the hardware queue class. The command list is
            //! automatically reclaimed after the current frame has flushed through the GPU.
            CommandList* AcquireCommandList(RHI::HardwareQueueClass hardwareQueueClass);

            //! Acquires a sampler from the internal cache.
            RHI::ConstPtr<Sampler> AcquireSampler(const RHI::SamplerState& state);

            const PhysicalDevice& GetPhysicalDevice() const;

            ID3D12DeviceX* GetDevice();

            MemoryPageAllocator& GetConstantMemoryPageAllocator();

            CommandQueueContext& GetCommandQueueContext();

            DescriptorContext& GetDescriptorContext();

            AsyncUploadQueue& GetAsyncUploadQueue();

            bool IsAftermathInitialized() const;

            //! Check the opResult return true if it was success
            //! If it's device lost, triggers device removal handling
            bool AssertSuccess(HRESULT opResult);

            // callback function which is called when device was removed
            void OnDeviceRemoved();

            // return the binding slot of the bindless srg
            uint32_t GetBindlessSrgSlot() const;

        private:
            Device();

            //////////////////////////////////////////////////////////////////////////
            // RHI::Device
            RHI::ResultCode InitInternal(RHI::PhysicalDevice& physicalDevice) override;
            RHI::ResultCode InitInternalBindlessSrg(const RHI::BindlessSrgDescriptor& bindlessSrgDesc) override;
            void ShutdownInternal() override;
            void CompileMemoryStatisticsInternal(RHI::MemoryStatisticsBuilder& builder) override;
            void UpdateCpuTimingStatisticsInternal() const override;
            RHI::ResultCode BeginFrameInternal() override;
            void EndFrameInternal() override;
            void WaitForIdleInternal() override;
            AZStd::chrono::microseconds GpuTimestampToMicroseconds(uint64_t gpuTimestamp, RHI::HardwareQueueClass queueClass) const override;
            AZStd::pair<uint64_t, uint64_t> GetCalibratedTimestamp(RHI::HardwareQueueClass queueClass) override;
            void FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities) override;
            RHI::ResultCode InitializeLimits() override;
            AZStd::vector<RHI::Format> GetValidSwapChainImageFormats(const RHI::WindowHandle& windowHandle) const override;
            void PreShutdown() override;
            RHI::ResourceMemoryRequirements GetResourceMemoryRequirements(const RHI::ImageDescriptor & descriptor) override;
            RHI::ResourceMemoryRequirements GetResourceMemoryRequirements(const RHI::BufferDescriptor & descriptor) override;
            void ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction) override;
            RHI::ShadingRateImageValue ConvertShadingRate(RHI::ShadingRate rate) const override;
            //////////////////////////////////////////////////////////////////////////

            RHI::ResultCode InitSubPlatform(RHI::PhysicalDevice& physicalDevice);
            void ShutdownSubPlatform();

            void InitDeviceRemovalHandle();

#ifdef USE_AMD_D3D12MA
            RHI::ResultCode InitD3d12maAllocator();
#endif
            void InitFeatures();

            void ConvertBufferDescriptorToResourceDesc(
                const RHI::BufferDescriptor& bufferDescriptor,
                D3D12_RESOURCE_STATES initialState,
                D3D12_RESOURCE_DESC& output
            );

            RHI::Ptr<ID3D12DeviceX> m_dx12Device;
            RHI::Ptr<IDXGIAdapterX> m_dxgiAdapter;
            RHI::Ptr<IDXGIFactoryX> m_dxgiFactory;

#ifdef USE_AMD_D3D12MA
            RHI::Ptr<D3D12MA::Allocator> m_dx12MemAlloc;
            D3d12maReleaseQueue m_D3d12maReleaseQueue;
#endif

            ReleaseQueue m_releaseQueue;

            PipelineLayoutCache m_pipelineLayoutCache;

            StagingMemoryAllocator m_stagingMemoryAllocator;

            RHI::ThreadLocalContext<AZStd::lru_cache<uint64_t, D3D12_RESOURCE_ALLOCATION_INFO>> m_allocationInfoCache;

            AZStd::shared_ptr<DescriptorContext> m_descriptorContext;

            CommandListAllocator m_commandListAllocator;

            CommandQueueContext m_commandQueueContext;

            AsyncUploadQueue m_asyncUploadQueue;

            static const uint32_t SamplerCacheCapacity = 500;
            RHI::ObjectCache<Sampler> m_samplerCache;
            AZStd::mutex m_samplerCacheMutex;

            bool m_isAftermathInitialized = false;

            // device remover fence            
            RHI::Ptr<ID3D12Fence> m_deviceFence;
            bool m_onDeviceRemoved = false;
            AZStd::mutex m_onDeviceRemovedMutex;
            HANDLE m_waitHandle;

            // Cache bindless srg bind slot
            uint32_t m_bindlesSrgBindingSlot = AZ::RHI::InvalidIndex;
        };
    }
}
