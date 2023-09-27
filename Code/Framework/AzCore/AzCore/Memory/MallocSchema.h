/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
