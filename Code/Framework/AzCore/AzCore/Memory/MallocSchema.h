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
        : public IAllocator
    {
    public:
        /**
        * Description - configure the heap allocator. By default
        * we will allocate system memory using system calls. You can
        * provide arena (memory block) with pre-allocated memory.
        */

        MallocSchema();
        virtual ~MallocSchema();

        pointer         allocate(size_type byteSize, size_type alignment) override;
        void            deallocate(pointer ptr, size_type byteSize = 0, size_type alignment = 0) override;
        pointer         reallocate(pointer ptr, size_type newSize, size_type newAlignment) override;
        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override;

        size_type       NumAllocatedBytes() const override;

        /// Return unused memory to the OS. Don't call this unless you really need free memory, it is slow.
        void            GarbageCollect() override;

        static size_t GetMemoryGuardSize();
        static size_t GetFreeLinkSize();
    private:
        typedef void* (*MallocFn)(size_t);
        typedef void (*FreeFn)(void*);

        AZStd::atomic<size_t> m_bytesAllocated;
        MallocFn m_mallocFn;
        FreeFn m_freeFn;
    };
}
