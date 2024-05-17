/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
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

        BufferPoolResolver* BufferPool::GetResolver()
        {
            return static_cast<BufferPoolResolver*>(Base::GetResolver());
        }

        RHI::ResultCode BufferPool::InitInternal(RHI::Device& deviceBase, const RHI::BufferPoolDescriptor& descriptorBase) 
        {
            auto& device = static_cast<Device&>(deviceBase);
            if (descriptorBase.m_heapMemoryLevel == RHI::HeapMemoryLevel::Device)
            {
                SetResolver(AZStd::make_unique<BufferPoolResolver>(device, descriptorBase));
            }

            return RHI::ResultCode::Success;
        }

        void BufferPool::ShutdownInternal() 
        {
        }

        RHI::ResultCode BufferPool::InitBufferInternal(RHI::DeviceBuffer& bufferBase, const RHI::BufferDescriptor& bufferDescriptor)
        {
            auto& buffer = static_cast<Buffer&>(bufferBase);
            auto& device = static_cast<Device&>(GetDevice());

            VkMemoryRequirements requirements = device.GetBufferMemoryRequirements(bufferDescriptor);
            RHI::HeapMemoryLevel heapMemoryLevel = GetDescriptor().m_heapMemoryLevel;
            RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(heapMemoryLevel);
            if (!heapMemoryUsage.CanAllocate(requirements.size))
            {
                AZ_Error("Vulkan::BufferPool", false, "Failed to initialize buffer to due memory budget constraints");
                return RHI::ResultCode::OutOfMemory;
            }

            // Add the copy write flag since it's needed for staging copies and clear operations.
            RHI::BufferDescriptor descriptor = bufferDescriptor;
            descriptor.m_bindFlags |= RHI::BufferBindFlags::CopyWrite;

            RHI::Ptr<BufferMemory> memory = BufferMemory::Create();
            auto result = memory->Init(device, BufferMemory::Descriptor(descriptor, heapMemoryLevel));
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            result = buffer.Init(device, descriptor, BufferMemoryView(memory));
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            heapMemoryUsage.m_usedResidentInBytes += requirements.size;
            heapMemoryUsage.m_totalResidentInBytes += requirements.size;
            return result;
        }

        void BufferPool::ShutdownResourceInternal(RHI::DeviceResource& resource) 
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

            RHI::HeapMemoryLevel heapMemoryLevel = GetDescriptor().m_heapMemoryLevel;
            RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(heapMemoryLevel);
            size_t sizeInBytes = buffer.m_memoryView.GetSize();
            heapMemoryUsage.m_usedResidentInBytes -= sizeInBytes;
            heapMemoryUsage.m_totalResidentInBytes -= sizeInBytes;

            // Deallocate the BufferMemory
            device.QueueForRelease(buffer.m_memoryView.GetAllocation());
            buffer.m_memoryView = BufferMemoryView();
            buffer.Invalidate();
        }

        RHI::ResultCode BufferPool::OrphanBufferInternal(RHI::DeviceBuffer& bufferBase) 
        {
            auto& buffer = static_cast<Buffer&>(bufferBase);
            auto& device = static_cast<Device&>(GetDevice());

            // Deallocate the BufferMemory
            device.QueueForRelease(buffer.m_memoryView.GetAllocation());
            buffer.m_memoryView = BufferMemoryView();
            buffer.Invalidate();

            auto result  = InitBufferInternal(bufferBase, bufferBase.GetDescriptor());
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            buffer.InvalidateViews();
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode BufferPool::MapBufferInternal(const RHI::DeviceBufferMapRequest& mapRequest, RHI::DeviceBufferMapResponse& response) 
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

        void BufferPool::UnmapBufferInternal(RHI::DeviceBuffer& bufferBase) 
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

        RHI::ResultCode BufferPool::StreamBufferInternal(const RHI::DeviceBufferStreamRequest& request)
        {
            auto& device = static_cast<Device&>(GetDevice());
            device.GetAsyncUploadQueue().QueueUpload(request);
            return RHI::ResultCode::Success;
        }

        void BufferPool::ComputeFragmentation() const
        {
            // Since we use a per device memory allocator (VMA), there's no longer a per BufferPool fragmentation, only a global one.
        }
    }
}
