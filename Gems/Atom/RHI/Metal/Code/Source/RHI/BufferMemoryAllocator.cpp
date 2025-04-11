/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/lossy_cast.h>
#include <RHI/BufferMemoryAllocator.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Metal
    {
        void BufferMemoryAllocator::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;

            m_usePageAllocator = false;

            if (!RHI::CheckBitsAny(descriptor.m_bindFlags, RHI::BufferBindFlags::ShaderWrite | RHI::BufferBindFlags::CopyWrite))
            {
                m_usePageAllocator = true;

                const RHI::HeapMemoryUsage* heapMemoryUsage = descriptor.m_getHeapMemoryUsageFunction();

                const size_t budgetInBytes = heapMemoryUsage->m_budgetInBytes;
                if (budgetInBytes)
                {
                    // The buffer page size should not exceed the budget.
                    m_descriptor.m_pageSizeInBytes = static_cast<uint32_t>(AZStd::min<size_t>(budgetInBytes, m_descriptor.m_pageSizeInBytes));
                }

                m_pageAllocator.Init(m_descriptor);

                // Constant buffers have stricter alignment requirements.
                m_subAllocationAlignment = RHI::CheckBitsAny(m_descriptor.m_bindFlags, RHI::BufferBindFlags::Constant)
                    ? Alignment::Constant
                    : Alignment::Buffer;

                MemoryFreeListSubAllocator::Descriptor pageAllocatorDescriptor;
                pageAllocatorDescriptor.m_alignmentInBytes = m_subAllocationAlignment;
                pageAllocatorDescriptor.m_garbageCollectLatency = RHI::Limits::Device::FrameCountMax;
                pageAllocatorDescriptor.m_inactivePageCycles = 1;
                m_subAllocator.Init(pageAllocatorDescriptor, m_pageAllocator);
            }
        }

        void BufferMemoryAllocator::Shutdown()
        {
            if (m_usePageAllocator)
            {
                m_subAllocator.Shutdown();
                m_pageAllocator.Shutdown();
            }
        }

        void BufferMemoryAllocator::GarbageCollect()
        {
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
                m_subAllocator.GarbageCollect();
            }

            m_pageAllocator.Collect();
        }

        BufferMemoryView BufferMemoryAllocator::Allocate(size_t sizeInBytes)
        {
            BufferMemoryView bufferMemoryView;

            // First attempt to sub-allocate a buffer from the sub-allocator.
            if (m_usePageAllocator)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);

                bufferMemoryView = BufferMemoryView(MemoryView(m_subAllocator.Allocate(sizeInBytes, m_subAllocationAlignment)), BufferMemoryType::SubAllocated);
            }

            if (bufferMemoryView.IsValid())
            {
                RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
                heapMemoryUsage.m_usedResidentInBytes += bufferMemoryView.GetSize();
                heapMemoryUsage.Validate();
            }
            else
            {
                // Next, try a unique buffer allocation.
                RHI::BufferDescriptor bufferDescriptor;
                bufferDescriptor.m_byteCount = azlossy_caster(sizeInBytes);
                bufferDescriptor.m_bindFlags = m_descriptor.m_bindFlags;
                
                AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
                bufferMemoryView = AllocateUnique(bufferDescriptor);
            }

            return bufferMemoryView;
        }

        void BufferMemoryAllocator::DeAllocate(const BufferMemoryView& memoryView)
        {
            switch (memoryView.GetType())
            {
            case BufferMemoryType::SubAllocated:
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
                    RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
                    heapMemoryUsage.m_usedResidentInBytes -= memoryView.GetSize();
                    m_subAllocator.DeAllocate(memoryView.m_memoryAllocation);
                }
                break;

            case BufferMemoryType::Unique:
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
                    DeAllocateUnique(memoryView);
                }
                break;
            }
        }

        float BufferMemoryAllocator::ComputeFragmentation() const
        {
            if (m_usePageAllocator)
            {
                return m_subAllocator.ComputeFragmentation();
            }

            return 0.f;
        }


        BufferMemoryView BufferMemoryAllocator::AllocateUnique(const RHI::BufferDescriptor& bufferDescriptor)
        {
            AZ_PROFILE_FUNCTION(RHI);
            const size_t alignedSize = RHI::AlignUp(bufferDescriptor.m_byteCount, Alignment::Buffer);
            
            RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
            if (!heapMemoryUsage.CanAllocate(alignedSize))
            {
                return BufferMemoryView();
            }
            
            MemoryView memoryView = m_descriptor.m_device->CreateBufferCommitted(bufferDescriptor, m_descriptor.m_heapMemoryLevel);
            if (memoryView.IsValid())
            {
                heapMemoryUsage.m_totalResidentInBytes += alignedSize;
                heapMemoryUsage.m_usedResidentInBytes += alignedSize;
                heapMemoryUsage.m_uniqueAllocationBytes += alignedSize;
            }
            return BufferMemoryView(AZStd::move(memoryView), BufferMemoryType::Unique);
        }

        void BufferMemoryAllocator::DeAllocateUnique(const BufferMemoryView& memoryView)
        {
            AZ_Assert(memoryView.GetType() == BufferMemoryType::Unique, "This call only supports unique BufferMemoryView allocations.");
            const size_t sizeInBytes = memoryView.GetSize();
            
            RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
            heapMemoryUsage.m_totalResidentInBytes -= sizeInBytes;
            heapMemoryUsage.m_usedResidentInBytes -= sizeInBytes;
            heapMemoryUsage.m_uniqueAllocationBytes -= sizeInBytes;
            
            m_descriptor.m_device->QueueForRelease(memoryView);
        }
    }
}
