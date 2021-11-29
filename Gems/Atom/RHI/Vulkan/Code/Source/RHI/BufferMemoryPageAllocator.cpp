/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/BufferMemoryPageAllocator.h>
#include <RHI/Device.h>
#include <RHI/Conversion.h>
#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace Vulkan
    {
        void BufferMemoryPageFactory::Init(const Descriptor& descriptor)
        {
            AZ_Assert(descriptor.m_getHeapMemoryUsageFunction, "You must supply a valid function for getting heap memory usage.");

            m_descriptor = descriptor;
            m_debugName = AZ::Name("BufferMemoryPage");

            RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_byteCount = m_descriptor.m_pageSizeInBytes;
            bufferDescriptor.m_bindFlags = descriptor.m_bindFlags;
            bufferDescriptor.m_sharedQueueMask = descriptor.m_sharedQueueMask;
        }

        RHI::Ptr<BufferMemory> BufferMemoryPageFactory::CreateObject()
        {
            return CreateObject(m_descriptor.m_pageSizeInBytes);
        }

        RHI::Ptr<BufferMemory> BufferMemoryPageFactory::CreateObject(size_t sizeInBytes)
        {
            BufferMemory::Descriptor bufferDescriptor;
            bufferDescriptor.m_byteCount = sizeInBytes;
            bufferDescriptor.m_sharedQueueMask = m_descriptor.m_sharedQueueMask;
            bufferDescriptor.m_bindFlags = m_descriptor.m_bindFlags;

            VkBuffer vkBuffer = GetDevice().CreateBufferResouce(bufferDescriptor);
            VkMemoryRequirements memoryRequirements = {};
            vkGetBufferMemoryRequirements(GetDevice().GetNativeDevice(), vkBuffer, &memoryRequirements);

            RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
            if (!heapMemoryUsage.TryReserveMemory(memoryRequirements.size))
            {
                GetDevice().DestroyBufferResource(vkBuffer);
                return nullptr;
            }

            AZ_TRACE_METHOD_NAME("Create BufferMemory Page");

            RHI::Ptr<BufferMemory> bufferMemory;

            const VkMemoryPropertyFlags flags = ConvertHeapMemoryLevel(m_descriptor.m_heapMemoryLevel) | m_descriptor.m_additionalMemoryPropertyFlags;
            RHI::Ptr<Memory> memory = GetDevice().AllocateMemory(memoryRequirements.size, memoryRequirements.memoryTypeBits, flags, m_descriptor.m_bindFlags);
            if (memory)
            {
                
                bufferMemory = BufferMemory::Create();
                RHI::ResultCode result = bufferMemory->Init(GetDevice(), MemoryView(memory, 0, memoryRequirements.size, 0, MemoryAllocationType::Unique), bufferDescriptor);
                if (result != RHI::ResultCode::Success)
                {                    
                    bufferMemory = nullptr;
                }
            }

            if (bufferMemory)
            {
                heapMemoryUsage.m_residentInBytes += memoryRequirements.size;
                bufferMemory->SetName(m_debugName);
            }
            else
            {
                GetDevice().DestroyBufferResource(vkBuffer);
                heapMemoryUsage.m_reservedInBytes -= memoryRequirements.size;
            }

            return bufferMemory;
        }

        void BufferMemoryPageFactory::ShutdownObject(BufferMemory& memory, bool isPoolShutdown)
        {
            RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
            heapMemoryUsage.m_residentInBytes -= memory.GetDescriptor().m_byteCount;
            heapMemoryUsage.m_reservedInBytes -= memory.GetDescriptor().m_byteCount;

            if (isPoolShutdown)
            {
                GetDevice().QueueForRelease(&memory);
            }
        }

        bool BufferMemoryPageFactory::CollectObject(BufferMemory& memory)
        {
            AZ_UNUSED(memory);
            return m_descriptor.m_recycleOnCollect;
        }

        const BufferMemoryPageFactory::Descriptor& BufferMemoryPageFactory::GetDescriptor() const
        {
            return m_descriptor;
        }

        void BufferMemoryPageFactory::SetDebugName(const AZStd::string_view& name) const
        {
            m_debugName = name;
        }

        Device& BufferMemoryPageFactory::GetDevice() const
        {
            return *m_descriptor.m_device;
        }
    }
}
