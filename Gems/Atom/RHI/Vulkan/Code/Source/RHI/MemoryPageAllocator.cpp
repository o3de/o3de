/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/MemoryPageAllocator.h>
#include <RHI/Device.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
namespace AZ
{
    namespace Vulkan
    {
        void MemoryPageFactory::Init(const Descriptor& descriptor)
        {
            AZ_Assert(descriptor.m_getHeapMemoryUsageFunction, "You must supply a valid function for getting heap memory usage.");

            m_descriptor = descriptor;
            m_debugName = AZ::Name("MemoryPage");
        }

        RHI::Ptr<Memory> MemoryPageFactory::CreateObject()
        {
            return CreateObject(m_descriptor.m_pageSizeInBytes);
        }

        RHI::Ptr<Memory> MemoryPageFactory::CreateObject(size_t sizeInBytes)
        {
            RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
            if (!heapMemoryUsage.TryReserveMemory(sizeInBytes))
            {
                return nullptr;
            }

            AZ_PROFILE_SCOPE(RHI, "Create Memory Page");

            const VkMemoryPropertyFlags flags = ConvertHeapMemoryLevel(m_descriptor.m_heapMemoryLevel) | m_descriptor.m_additionalMemoryPropertyFlags;
            RHI::Ptr<Memory> memory = GetDevice().AllocateMemory(sizeInBytes, m_descriptor.m_memoryTypeBits, flags);

            if (memory)
            {
                heapMemoryUsage.m_residentInBytes += sizeInBytes;

                memory->SetName(m_debugName);
            }
            else
            {
                heapMemoryUsage.m_reservedInBytes -= sizeInBytes;
            }

            return memory;
        }

        void MemoryPageFactory::ShutdownObject(Memory& memory, bool isPoolShutdown)
        {
            RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
            heapMemoryUsage.m_residentInBytes -= memory.GetDescriptor().m_sizeInBytes;
            heapMemoryUsage.m_reservedInBytes -= memory.GetDescriptor().m_sizeInBytes;

            if (isPoolShutdown)
            {
                GetDevice().QueueForRelease(&memory);
            }
        }

        bool MemoryPageFactory::CollectObject(Memory& memory)
        {
            AZ_UNUSED(memory);
            return m_descriptor.m_recycleOnCollect;
        }

        const MemoryPageFactory::Descriptor& MemoryPageFactory::GetDescriptor() const
        {
            return m_descriptor;
        }

        void MemoryPageFactory::SetDebugName(const AZStd::string_view& name) const
        {
            m_debugName = name;
        }

        Device& MemoryPageFactory::GetDevice() const
        {
            return *m_descriptor.m_device;
        }

    }
}
