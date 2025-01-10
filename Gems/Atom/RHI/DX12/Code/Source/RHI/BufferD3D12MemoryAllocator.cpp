/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/RHIBus.h>
#include <RHI/BufferD3D12MemoryAllocator.h>
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

        void BufferD3D12MemoryAllocator::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;
        }

        void BufferD3D12MemoryAllocator::Shutdown()
        {
        }

        void BufferD3D12MemoryAllocator::GarbageCollect()
        {
        }

        BufferMemoryView BufferD3D12MemoryAllocator::Allocate([[maybe_unused]] size_t sizeInBytes, [[maybe_unused]] size_t overrideSubAllocAlignment)
        {
            AZ_PROFILE_FUNCTION(RHI);

#ifdef USE_AMD_D3D12MA
            struct
            {
                size_t m_alignment = 0;
                void operator=(size_t value)
                {
                    m_alignment = AZStd::max(m_alignment, value);
                }
            } alignment;
            RHI::RHIRequirementRequestBus::BroadcastResult(
                alignment, &RHI::RHIRequirementsRequest::GetRequiredAlignment, *m_descriptor.m_device);
            if (alignment.m_alignment && sizeInBytes > alignment.m_alignment)
            {
                sizeInBytes = ((sizeInBytes + alignment.m_alignment - 1) / alignment.m_alignment) * alignment.m_alignment;
            }

            RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_byteCount = azlossy_caster(sizeInBytes);
            bufferDescriptor.m_bindFlags = m_descriptor.m_bindFlags;

            D3D12_RESOURCE_STATES initialResourceState = ConvertInitialResourceState(m_descriptor.m_heapMemoryLevel, m_descriptor.m_hostMemoryAccess);
            if (RHI::CheckBitsAny(m_descriptor.m_bindFlags, RHI::BufferBindFlags::RayTracingAccelerationStructure))
            {
                initialResourceState = Platform::GetRayTracingAccelerationStructureResourceState();
            }

            const D3D12_HEAP_TYPE heapType = ConvertHeapType(m_descriptor.m_heapMemoryLevel, m_descriptor.m_hostMemoryAccess);

            MemoryView memoryView = m_descriptor.m_device->CreateD3d12maBuffer(bufferDescriptor, initialResourceState, heapType);
            if (memoryView.IsValid())
            {
                const size_t sizeAllocated = memoryView.GetSize();

                // Add the resident usage now that everything succeeded.
                RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
                // D2D12MA allocates memory in 64MB heaps that are shared between all BufferPools, 
                // so this number is a reasonable approximation of how much memory this BufferPool has resident.
                heapMemoryUsage.m_totalResidentInBytes += sizeAllocated;
                heapMemoryUsage.m_usedResidentInBytes += sizeAllocated;
            }

            return BufferMemoryView(AZStd::move(memoryView), BufferMemoryType::Unique);
#else
            return BufferMemoryView();
#endif
        }

        void BufferD3D12MemoryAllocator::DeAllocate([[maybe_unused]] const BufferMemoryView& memoryView)
        {
#ifdef USE_AMD_D3D12MA
            AZ_Assert(memoryView.GetType() == BufferMemoryType::Unique, "This call only supports unique BufferMemoryView allocations.");
            const size_t sizeInBytes = memoryView.GetSize();

            RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
            heapMemoryUsage.m_totalResidentInBytes -= sizeInBytes;
            heapMemoryUsage.m_usedResidentInBytes -= sizeInBytes;
            heapMemoryUsage.m_uniqueAllocationBytes -= sizeInBytes;

            m_descriptor.m_device->QueueForRelease(memoryView);
#endif
        }

        float BufferD3D12MemoryAllocator::ComputeFragmentation() const
        {
            return 0.f;
        }
    }
}
