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
        virtual pointer_type Allocate(size_type byteSize, size_type alignment, int flags, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override;
        virtual void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override;
        virtual pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override;
        virtual size_type Resize(pointer_type ptr, size_type newSize) override;
        virtual size_type AllocationSize(pointer_type ptr) override;

        virtual size_type NumAllocatedBytes() const override;
        virtual size_type Capacity() const override;
        virtual size_type GetMaxAllocationSize() const override;
        virtual IAllocatorAllocate* GetSubAllocator() override;
        virtual void GarbageCollect() override;

    private:
        typedef void* (*MallocFn)(size_t);
        typedef void (*FreeFn)(void*);

        AZStd::atomic<size_t> m_bytesAllocated;
        MallocFn m_mallocFn;
        FreeFn m_freeFn;
    };
}
