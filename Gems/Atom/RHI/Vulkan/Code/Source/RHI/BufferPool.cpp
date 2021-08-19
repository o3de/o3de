/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <Atom/RHI.Reflect/Vulkan/BufferPoolDescriptor.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/BufferPool.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPoolResolver.h>
#include <RHI/Device.h>
#include <RHI/MemoryView.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<BufferPool> BufferPool::Create()
        {
            return aznew BufferPool();
        }

        Device& BufferPool::GetDevice() const
        {
            return static_cast<Device&>(Base::GetDevice());
        }

        void BufferPool::GarbageCollect()
        {
            m_memoryAllocator.GarbageCollect();
        }

        BufferPoolResolver* BufferPool::GetResolver()
        {
            return static_cast<BufferPoolResolver*>(Base::GetResolver());
        }

        void BufferPool::OnFrameEnd()
        {
            GarbageCollect();
            Base::OnFrameEnd();
        }

        RHI::ResultCode BufferPool::InitInternal(RHI::Device& deviceBase, const RHI::BufferPoolDescriptor& descriptorBase) 
        {
            auto& device = static_cast<Device&>(deviceBase);

            VkDeviceSize bufferPageSizeInBytes = device.GetDescriptor().m_platformLimitsDescriptor->m_platformDefaultValues.m_bufferPoolPageSizeInBytes;
            VkMemoryPropertyFlags additionalMemoryPropertyFlags = 0;
            if (const auto* descriptor = azrtti_cast<const BufferPoolDescriptor*>(&descriptorBase))
            {
                bufferPageSizeInBytes = descriptor->m_bufferPoolPageSizeInBytes;
                additionalMemoryPropertyFlags = descriptor->m_additionalMemoryPropertyFlags;
            }

            if (descriptorBase.m_largestPooledAllocationSizeInBytes > 0)
            {
                bufferPageSizeInBytes = AZStd::max(bufferPageSizeInBytes, aznumeric_cast<VkDeviceSize>(descriptorBase.m_largestPooledAllocationSizeInBytes));
            }

            // Add the copy write flag since it's needed for staging copies and clear operations.
            RHI::BufferBindFlags bindFlags = descriptorBase.m_bindFlags | RHI::BufferBindFlags::CopyWrite;

            RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(descriptorBase.m_heapMemoryLevel);
            BufferMemoryAllocator::Descriptor memoryAllocDescriptor;
            memoryAllocDescriptor.m_device = &device;
            memoryAllocDescriptor.m_pageSizeInBytes = static_cast<size_t>(bufferPageSizeInBytes);
            memoryAllocDescriptor.m_heapMemoryLevel = descriptorBase.m_heapMemoryLevel;
            memoryAllocDescriptor.m_hostMemoryAccess = descriptorBase.m_hostMemoryAccess;
            memoryAllocDescriptor.m_additionalMemoryPropertyFlags = additionalMemoryPropertyFlags;
            memoryAllocDescriptor.m_getHeapMemoryUsageFunction = [&]() { return &heapMemoryUsage; };
            memoryAllocDescriptor.m_recycleOnCollect = true;
            memoryAllocDescriptor.m_collectLatency = RHI::Limits::Device::FrameCountMax;
            memoryAllocDescriptor.m_bindFlags = bindFlags;
            memoryAllocDescriptor.m_sharedQueueMask = descriptorBase.m_sharedQueueMask;
            m_memoryAllocator.Init(memoryAllocDescriptor);

            if (descriptorBase.m_heapMemoryLevel == RHI::HeapMemoryLevel::Device)
            {
                SetResolver(AZStd::make_unique<BufferPoolResolver>(device, descriptorBase));
            }

            return RHI::ResultCode::Success;
        }

        void BufferPool::ShutdownInternal() 
        {
            m_memoryAllocator.Shutdown();
        }

        RHI::ResultCode BufferPool::InitBufferInternal(RHI::Buffer& bufferBase, const RHI::BufferDescriptor& bufferDescriptor)
        {
            auto& buffer = static_cast<Buffer&>(bufferBase);
            auto& device = static_cast<Device&>(GetDevice());

            // Note that InputAssembly, RayTracingAccelerationStructure, and RayTracingShaderBindingTable buffers must be unique allocations.
            // This is necessary since the alignment on a paged memory allocation is not compatible with the required raytracing alignment.
            bool forceUnique = RHI::CheckBitsAny(
                bufferDescriptor.m_bindFlags,
                RHI::BufferBindFlags::InputAssembly |
                RHI::BufferBindFlags::DynamicInputAssembly |
                RHI::BufferBindFlags::RayTracingAccelerationStructure |
                RHI::BufferBindFlags::RayTracingShaderTable);

            BufferMemoryView memoryView = m_memoryAllocator.Allocate(bufferDescriptor.m_byteCount, 1, forceUnique);
            if (!memoryView.IsValid())
            {
                return RHI::ResultCode::OutOfMemory;
            }

            RHI::ResultCode result = buffer.Init(device, bufferDescriptor, memoryView);
            return result;
        }

        void BufferPool::ShutdownResourceInternal(RHI::Resource& resource) 
        {
            auto& buffer = static_cast<Buffer&>(resource);
            auto& device = static_cast<Device&>(GetDevice());

            // Wait for any pending streaming upload.
            device.GetAsyncUploadQueue().WaitForUpload(buffer.GetUploadHandle());

            if (auto* resolver = GetResolver())
            {
                ResourcePoolResolver* poolResolver = static_cast<ResourcePoolResolver*>(resolver);
                poolResolver->OnResourceShutdown(resource);
            }

            m_memoryAllocator.DeAllocate(buffer.m_memoryView);
            buffer.m_memoryView = BufferMemoryView();
            buffer.Invalidate();
        }

        RHI::ResultCode BufferPool::OrphanBufferInternal(RHI::Buffer& bufferBase) 
        {
            auto& buffer = static_cast<Buffer&>(bufferBase);
            auto& device = static_cast<Device&>(GetDevice());

            // Deallocate the BufferMemory
            m_memoryAllocator.DeAllocate(buffer.m_memoryView);
            buffer.m_memoryView = BufferMemoryView();
            buffer.Invalidate();

            // Allocate a new BufferMemmory
            BufferMemoryView memoryView = m_memoryAllocator.Allocate(buffer.GetDescriptor().m_byteCount, 1);
            if (!memoryView.IsValid())
            {
                return RHI::ResultCode::OutOfMemory;
            }

            RHI::ResultCode result = buffer.Init(device, bufferBase.GetDescriptor(), memoryView);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            
            buffer.InvalidateViews();
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode BufferPool::MapBufferInternal(const RHI::BufferMapRequest& mapRequest, RHI::BufferMapResponse& response) 
        {
            const RHI::BufferPoolDescriptor& descriptor = GetDescriptor();
            auto* buffer = static_cast<Buffer*>(mapRequest.m_buffer);

            void* mappedData = nullptr;
            switch (descriptor.m_heapMemoryLevel)
            {
            case RHI::HeapMemoryLevel::Host:
            {
                if (!buffer->GetBufferMemoryView())
                {
                    return RHI::ResultCode::InvalidOperation;
                }
                mappedData = buffer->GetBufferMemoryView()->Map(descriptor.m_hostMemoryAccess);
                if (mappedData)
                {
                    mappedData = static_cast<int8_t*>(mappedData) + mapRequest.m_byteOffset;
                }
                else
                {
                    return RHI::ResultCode::Fail;
                }
                break;
            }
            case RHI::HeapMemoryLevel::Device:
            {
                mappedData = GetResolver()->MapBuffer(mapRequest);
                if (mappedData)
                {
                    m_memoryUsage.m_transferPull.m_bytesPerFrame += mapRequest.m_byteCount;
                }
                else
                {
                    return RHI::ResultCode::OutOfMemory;
                }
                break;
            }
            default:
                AZ_Assert(false, "HeapMemoryLevel is illegal.");
                return RHI::ResultCode::InvalidArgument;
            }

            response.m_data = mappedData;
            return RHI::ResultCode::Success;
        }

        void BufferPool::UnmapBufferInternal(RHI::Buffer& bufferBase) 
        {
            const RHI::BufferPoolDescriptor& descriptor = GetDescriptor();
            auto& buffer = static_cast<Buffer&>(bufferBase);
            switch (descriptor.m_heapMemoryLevel)
            {
            case RHI::HeapMemoryLevel:: Host:
            {
                buffer.GetBufferMemoryView()->Unmap(descriptor.m_hostMemoryAccess);
                return;
            }
            case RHI::HeapMemoryLevel::Device:
            {
                // do nothing
                return;
            }
            default:
                AZ_Assert(false, "HeapMemoryLevel is illegal.");
            }
        }

        RHI::ResultCode BufferPool::StreamBufferInternal(const RHI::BufferStreamRequest& request)
        {
            auto& device = static_cast<Device&>(GetDevice());
            device.GetAsyncUploadQueue().QueueUpload(request);
            return RHI::ResultCode::Success;
        }

        void BufferPool::SetNameInternal(const AZStd::string_view& name)
        {
             m_memoryAllocator.SetName(AZ::Name{ name });
        }
    }
}
