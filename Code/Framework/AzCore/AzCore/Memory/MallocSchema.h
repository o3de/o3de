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
        : public IAllocatorAllocate
    {
    public:
        AZ_TYPE_INFO("MallocSchema", "{2A21D120-A42A-484C-997C-5735DCCA5FE9}");

        typedef void*       pointer_type;
        typedef size_t      size_type;
        typedef ptrdiff_t   difference_type;

        struct Descriptor
        {
            Descriptor(bool useAZMalloc = true)
                : m_useAZMalloc(useAZMalloc)
            {
            }

            bool m_useAZMalloc;
        };

        MallocSchema(const Descriptor& desc = Descriptor());
        virtual ~MallocSchema();

        //---------------------------------------------------------------------
        // IAllocatorAllocate
        //---------------------------------------------------------------------
        pointer_type Allocate(size_type byteSize, size_type alignment, int flags, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override;
        void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override;
        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override;
        size_type Resize(pointer_type ptr, size_type newSize) override;
        size_type AllocationSize(pointer_type ptr) override;

        size_type NumAllocatedBytes() const override;
        size_type Capacity() const override;
        size_type GetMaxAllocationSize() const override;
        size_type GetMaxContiguousAllocationSize() const override;
        IAllocatorAllocate* GetSubAllocator() override;
        void GarbageCollect() override;

    private:
        typedef void* (*MallocFn)(size_t);
        typedef void (*FreeFn)(void*);

        AZStd::atomic<size_t> m_bytesAllocated;
        MallocFn m_mallocFn;
        FreeFn m_freeFn;
    };
}
