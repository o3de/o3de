/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/MemoryPageAllocator.h>
namespace AZ
{
    namespace Metal
    {
        void MemoryPageFactory::Init(const Descriptor& descriptor)
        {
            AZ_Assert(descriptor.m_getHeapMemoryUsageFunction, "You must supply a valid function for getting heap memory usage.");
            m_descriptor = descriptor;
        }

        RHI::Ptr<Memory> MemoryPageFactory::CreateObject()
        {
            RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
            if (!heapMemoryUsage.CanAllocate(m_descriptor.m_pageSizeInBytes))
            {
                return nullptr;
            }

            AZ_PROFILE_SCOPE(RHI, "Create Buffer Page");

            RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_byteCount = m_descriptor.m_pageSizeInBytes;
            bufferDescriptor.m_bindFlags = m_descriptor.m_bindFlags;
            MemoryView memoryView = m_descriptor.m_device->CreateBufferCommitted(bufferDescriptor, m_descriptor.m_heapMemoryLevel);

            if (memoryView.IsValid())
            {
                memoryView.SetName(AZStd::string::format("BufferPage_%s", AZ::Uuid::CreateRandom().ToString<AZStd::string>().c_str()));
                heapMemoryUsage.m_totalResidentInBytes += memoryView.GetSize();
            }

            return memoryView.GetMemory();
        }

        void MemoryPageFactory::ShutdownObject(Memory& memory, bool isPoolShutdown)
        {
            RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
            heapMemoryUsage.m_totalResidentInBytes -= m_descriptor.m_pageSizeInBytes;
            heapMemoryUsage.Validate();

            if (isPoolShutdown)
            {
                m_descriptor.m_device->QueueForRelease(&memory);
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
    }
}
