/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/ResourcePoolDescriptor.h>
#include <Atom/RHI.Reflect/MemoryEnums.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    class BufferPoolDescriptor
        : public ResourcePoolDescriptor
    {
    public:
        virtual ~BufferPoolDescriptor() = default;
        AZ_CLASS_ALLOCATOR(BufferPoolDescriptor, SystemAllocator)
        AZ_RTTI(BufferPoolDescriptor, "{53074556-41D1-4246-8BF0-E5B096997C65}", ResourcePoolDescriptor);
        static void Reflect(AZ::ReflectContext* context);

        BufferPoolDescriptor() = default;

        //! The memory heap to store final buffer allocations. Currently, there are two supported options:
        //!
        //!  - 'Device' memory is stored local to GPU hardware. On certain platforms this may require a transfer
        //!    from Host memory through a DMA controller. In this scenario, Host memory will be used to stage the
        //!    transfer.
        //!
        //!  - 'Host' memory is stored local to the CPU. This guarantees fast CPU access and will not require
        //!    an explicit staging transfer. However, GPU read access will be slower as a result.
        HeapMemoryLevel m_heapMemoryLevel = HeapMemoryLevel::Device;

        //! If the heap memory type is host memory, this specifies the CPU access pattern from the user
        //! in system memory.
        //!
        //!  - Host 'Write' pools are written by the CPU and read by the GPU.
        //!  - Host 'Read' pools are written by the GPU and read by the CPU.
        //!
        //! If the Device heap is used, this value must be 'Write', as only 'Write' operations are permitted
        //! for Map operations. Attempting to assign 'Read' when the 'Device' heap is specified will result in
        //! an error.
        HostMemoryAccess m_hostMemoryAccess = HostMemoryAccess::Write;

        //! The set of buffer bind flags supported by this pool. Buffers must be initialized with the
        //! exact bind flags as the parent pool.
        BufferBindFlags m_bindFlags = BufferBindFlags::None;

        //! Specifies the largest allocation required of the pool. Useful if you are allocating buffers
        //! larger than the RHI default page size.
        AZ::u64 m_largestPooledAllocationSizeInBytes = 0;

        //! The mask of queue classes supporting shared access of this pool.
        HardwareQueueClassMask m_sharedQueueMask = HardwareQueueClassMask::All;
    };
}
