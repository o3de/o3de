/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/BufferMemoryAllocator.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>

#include <AzCore/Casting/lossy_cast.h>

namespace AZ
{
    namespace DX12
    {
        namespace Platform
        {
            D3D12_RESOURCE_STATES GetRayTracingAccelerationStructureResourceState();
        }

        void BufferMemoryAllocator::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;

            // Buffers that can be attachments can independently transition state, which precludes them from
            // being sub-allocated from the same ID3D12Resource.  
            // [GFX-TODO][ATOM-6230] Investigate performance of InputAssembly buffers with the page allocator
            // The page allocator is currently disabled for InputAssembly buffers.
            // Based on performance we may want to enable the page allocator for InputAssembly buffers. In order to do that we will
            // need to make sure they are aligned correctly. There is a restriction on buffer alignment where the alignment
            // needs to be a multiple of elementsize as well as divisible by DX12::Alignment types.
            m_usePageAllocator = false;

            if (!RHI::CheckBitsAny(descriptor.m_bindFlags, RHI::BufferBindFlags::ShaderWrite | RHI::BufferBindFlags::CopyWrite | RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::DynamicInputAssembly))
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
            m_subAllocatorMutex.lock();
            m_subAllocator.GarbageCollect();
            m_subAllocatorMutex.unlock();

            m_pageAllocator.Collect();
        }

        BufferMemoryView BufferMemoryAllocator::Allocate(size_t sizeInBytes, size_t overrideSubAllocAlignment)
        {
            AZ_PROFILE_FUNCTION(RHI);
            
            BufferMemoryView bufferMemoryView;

            // First attempt to sub-allocate a buffer from the sub-allocator.
            if (m_usePageAllocator)
            {
                m_subAllocatorMutex.lock();

                size_t subAllocationAlignment = m_subAllocationAlignment;
                if(overrideSubAllocAlignment > 0)
                {
                    // Validate the override alignment
                    if ( (overrideSubAllocAlignment % m_subAllocationAlignment) != 0
                        && (m_subAllocationAlignment % overrideSubAllocAlignment) != 0 )
                    {
                        AZ_Error("RHI DX12", false,
                            "The buffer alignment %d should be either an integer multiple or a factor of "
                            "default alignment %d",
                            overrideSubAllocAlignment, m_subAllocationAlignment);
                    }

                    subAllocationAlignment = AZStd::max(overrideSubAllocAlignment, m_subAllocationAlignment);
                }                

                bufferMemoryView = BufferMemoryView(MemoryView(m_subAllocator.Allocate(sizeInBytes, subAllocationAlignment), MemoryViewType::Buffer), BufferMemoryType::SubAllocated);
                m_subAllocatorMutex.unlock();
            }

            // Next, try a unique buffer allocation.
            if (bufferMemoryView.IsValid())
            {
                RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
                heapMemoryUsage.m_usedResidentInBytes += bufferMemoryView.GetSize();
                heapMemoryUsage.Validate();
            }
            else
            {
                RHI::BufferDescriptor bufferDescriptor;
                bufferDescriptor.m_byteCount = azlossy_caster(sizeInBytes);
                bufferDescriptor.m_bindFlags = m_descriptor.m_bindFlags;
                bufferMemoryView = AllocateUnique(bufferDescriptor);
            }

            return bufferMemoryView;
        }

        void BufferMemoryAllocator::DeAllocate(const BufferMemoryView& memoryView)
        {
            switch (memoryView.GetType())
            {
            case BufferMemoryType::SubAllocated:
                m_subAllocatorMutex.lock();
                {
                    RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
                    heapMemoryUsage.m_usedResidentInBytes -= memoryView.GetSize();
                    heapMemoryUsage.Validate();
                }
                m_subAllocator.DeAllocate(memoryView.m_memoryAllocation);
                m_subAllocatorMutex.unlock();
                break;

            case BufferMemoryType::Unique:
                DeAllocateUnique(memoryView);
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
            const size_t alignedSize = RHI::AlignUp(bufferDescriptor.m_byteCount, Alignment::CommittedBuffer);

            RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
            if (!heapMemoryUsage.CanAllocate(alignedSize))
            {
                return BufferMemoryView();
            }
            D3D12_RESOURCE_STATES initialResourceState = ConvertInitialResourceState(m_descriptor.m_heapMemoryLevel, m_descriptor.m_hostMemoryAccess);
            if (RHI::CheckBitsAny(m_descriptor.m_bindFlags, RHI::BufferBindFlags::RayTracingAccelerationStructure))
            {
                initialResourceState = Platform::GetRayTracingAccelerationStructureResourceState();
            }

            const D3D12_HEAP_TYPE heapType = ConvertHeapType(m_descriptor.m_heapMemoryLevel, m_descriptor.m_hostMemoryAccess);

            MemoryView memoryView = m_descriptor.m_device->CreateBufferCommitted(bufferDescriptor, initialResourceState, heapType);
            if (memoryView.IsValid())
            {
                const size_t sizeInBytes = memoryView.GetSize();

                // Add the resident usage now that everything succeeded.
                heapMemoryUsage.m_totalResidentInBytes += sizeInBytes;
                heapMemoryUsage.m_usedResidentInBytes += sizeInBytes;
                heapMemoryUsage.m_uniqueAllocationBytes += sizeInBytes;
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
