
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/CommandQueueContext.h>
#include <RHI/PipelineLayout.h>
#include <RHI/Sampler.h>
#include <Atom/RHI/Device.h>
#include <Atom/RHI/ObjectCache.h>
#include <Atom/RHI/ObjectCollector.h>

namespace AZ::WebGPU
{
    class Buffer;
    class CommandList;
    class BufferPool;

    class Device final
        : public RHI::Device
    {
        using Base = RHI::Device;
    public:
        AZ_CLASS_ALLOCATOR(Device, AZ::SystemAllocator);
        AZ_RTTI(Device, "{32DEC5E3-493C-4645-8270-8090350279EB}", Base);

        static RHI::Ptr<Device> Create();
        const wgpu::Device& GetNativeDevice() const;
        wgpu::Device& GetNativeDevice();

        CommandQueueContext& GetCommandQueueContext();
        //! Acquires a new CommandList to be used during the frame. All acquired
        //! command lists are deallocated at the end of the frame.
        CommandList* AcquireCommandList(RHI::HardwareQueueClass hardwareQueueClass);
        //! Acquires a PipelineLayout from a cache, or creates a new one if needed.
        RHI::Ptr<PipelineLayout> AcquirePipelineLayout(const PipelineLayout::Descriptor& descriptor);
        //! Acquires a PipelineLayout from a cache, or creates a new one if needed.
        RHI::Ptr<Sampler> AcquireSampler(const Sampler::Descriptor& descriptor);
        //! Acquires a buffer for stating.
        RHI::Ptr<Buffer> AcquireStagingBuffer(AZStd::size_t byteCount, AZStd::size_t alignment = 1);
        //! Retunrs the buffer pool used for SRG constants.
        RHI::Ptr<BufferPool> GetConstantBufferPool();

    private:
        Device();
            
        //////////////////////////////////////////////////////////////////////////
        // RHI::Device
        RHI::ResultCode InitInternal(RHI::PhysicalDevice& physicalDevice) override;
        RHI::ResultCode InitInternalBindlessSrg([[maybe_unused]] const RHI::BindlessSrgDescriptor& bindlessSrgDesc) override { return RHI::ResultCode::Success;}
        void ShutdownInternal() override {}
        void CompileMemoryStatisticsInternal([[maybe_unused]] RHI::MemoryStatisticsBuilder& builder) override {}
        void UpdateCpuTimingStatisticsInternal() const override {}
        RHI::ResultCode BeginFrameInternal() override;
        void EndFrameInternal() override;
        void WaitForIdleInternal() override {}
        AZStd::chrono::microseconds GpuTimestampToMicroseconds([[maybe_unused]] uint64_t gpuTimestamp, [[maybe_unused]] RHI::HardwareQueueClass queueClass) const override { return AZStd::chrono::microseconds();}
        void FillFormatsCapabilitiesInternal([[maybe_unused]] FormatCapabilitiesList& formatsCapabilities) override;
        RHI::ResultCode InitializeLimits() override;
        void PreShutdown() override;
        RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const RHI::ImageDescriptor& descriptor) override { return RHI::ResourceMemoryRequirements();}
        RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const RHI::BufferDescriptor& descriptor) override { return RHI::ResourceMemoryRequirements();}
        void ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction) override;
        RHI::ShadingRateImageValue ConvertShadingRate(RHI::ShadingRate rate) const override;
        //////////////////////////////////////////////////////////////////////////

        template<typename ObjectType>
        using ObjectCache = std::pair<RHI::ObjectCache<ObjectType>, WebGPU::mutex>;

        template<typename ObjectType, typename... Args>
        RHI::Ptr<ObjectType> AcquireObjectFromCache(ObjectCache<ObjectType>& cache, const size_t hash, Args... args);

        static void DeviceLostCallback(const wgpu::Device&, wgpu::DeviceLostReason reason, const char* message);
        static void ErrorCallback(const wgpu::Device& , wgpu::ErrorType type, const char* message);

        //! Native descriptor used for creating the device.
        wgpu::DeviceDescriptor m_wgpuDeviceDesc = {};
        //! Native device
        wgpu::Device m_wgpuDevice = nullptr;

        //! Command queue context use for accessing the Command Queues
        CommandQueueContext m_commandQueueContext;
        //! BufferPool for SRG constant buffers
        RHI::Ptr<BufferPool> m_constantBufferPool;
        //! BufferPool for stating buffers
        RHI::Ptr<BufferPool> m_stagingBufferPool;
        //! List of created command lists during the frame. Command list can't be
        //! reused on WebGPU so we release them at the end of the frame.
        AZStd::vector<RHI::Ptr<CommandList>> m_frameCommandLists;

        static const uint32_t SamplerCacheCapacity = 1000;
        static const uint32_t PipelineLayoutCacheCapacity = 1000;

        //! Cache for sampler objects.
        ObjectCache<Sampler> m_samplerCache;
        //! Cache for pipeline layout objects.
        ObjectCache<PipelineLayout> m_pipelineLayoutCache;
    };

    template<typename ObjectType, typename... Args>
    inline RHI::Ptr<ObjectType> Device::AcquireObjectFromCache(ObjectCache<ObjectType>& cache, const size_t hash, Args... args)
    {
        AZStd::lock_guard<decltype(cache.second)> lock(cache.second);
        auto& cacheContainer = cache.first;
        ObjectType* objectRawPtr = cacheContainer.Find(hash);
        if (!objectRawPtr)
        {
            RHI::Ptr<ObjectType> objectPtr = ObjectType::Create();
            if (objectPtr->Init(*this, AZStd::forward<Args>(args)...) == RHI::ResultCode::Success)
            {
                objectRawPtr = objectPtr.get();
                cacheContainer.Insert(hash, AZStd::move(objectPtr));
            }
            else
            {
                AZ_Error("WebGPU", false, "Failed to create a cached object");
            }
        }
        return RHI::Ptr<ObjectType>(objectRawPtr);
    }
}


