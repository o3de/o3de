/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>

namespace AZ
{
    /**
     * Malloc allocator scheme
     * Uses malloc internally. Mainly intended for debugging using host operating system features.
     */
    class MallocSchema
        : public IAllocatorSchema
    {
    public:
        AZ_TYPE_INFO("MallocSchema", "{2A21D120-A42A-484C-997C-5735DCCA5FE9}");

        typedef void*       pointer_type;
        typedef size_t      size_type;
        typedef ptrdiff_t   difference_type;

        struct Descriptor {
            AZ_TYPE_INFO(Descriptor, "{85B79120-D60D-40BE-92DA-094B4784FD07}")
        };

        MallocSchema(const Descriptor& desc = Descriptor());
        virtual ~MallocSchema();

        //---------------------------------------------------------------------
        // IAllocatorSchema
        //---------------------------------------------------------------------
        pointer_type allocate(size_type byteSize, size_type alignment) override;
        void deallocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override;
        pointer_type reallocate(pointer_type ptr, size_type newSize, size_type newAlignment) override;
        size_type Resize(pointer_type ptr, size_type newSize) override;
        size_type AllocationSize(pointer_type ptr) override;

        size_type NumAllocatedBytes() const override;
        size_type Capacity() const override;
        size_type GetMaxAllocationSize() const override;
        size_type GetMaxContiguousAllocationSize() const override;
        void GarbageCollect() override;

    private:
        AZStd::atomic<size_t> m_bytesAllocated;
    };
}
