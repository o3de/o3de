/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/MallocSchema.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/algorithm.h>

namespace AZ::Internal
{
    struct Header
    {
        uint32_t offset;
        uint32_t size;
    };
} // namespace AZ::Internal

namespace AZ
{
    //---------------------------------------------------------------------
    // MallocSchema methods
    //---------------------------------------------------------------------

    MallocSchema::MallocSchema(const Descriptor& desc)
        : m_bytesAllocated(0)
    {
        if (desc.m_useAZMalloc)
        {
            static const int DEFAULT_ALIGNMENT = sizeof(void*) * 2; // Default malloc alignment

            m_mallocFn = [](size_t byteSize)
            {
                return AZ_OS_MALLOC(byteSize, DEFAULT_ALIGNMENT);
            };
            m_freeFn = [](void* ptr)
            {
                AZ_OS_FREE(ptr);
            };
        }
        else
        {
            m_mallocFn = &malloc;
            m_freeFn = &free;
        }
    }

    MallocSchema::~MallocSchema()
    {
    }

    MallocSchema::pointer_type MallocSchema::Allocate(
        size_type byteSize,
        size_type alignment,
        int flags,
        const char* name,
        const char* fileName,
        int lineNum,
        unsigned int suppressStackRecord)
    {
        (void)flags;
        (void)name;
        (void)fileName;
        (void)lineNum;
        (void)suppressStackRecord;

        if (!byteSize)
        {
            return nullptr;
        }

        if (alignment == 0)
        {
            alignment = sizeof(void*) * 2; // Default malloc alignment
        }

        AZ_Assert(byteSize < 0x100000000ull, "Malloc allocator only allocates up to 4GB");

        size_type required = byteSize + sizeof(Internal::Header) +
            ((alignment > sizeof(double))
                 ? alignment
                 : 0); // Malloc will align to a minimum boundary for native objects, so we only pad if aligning to a large value
        void* data = (*m_mallocFn)(required);
        void* result = PointerAlignUp(reinterpret_cast<void*>(reinterpret_cast<size_t>(data) + sizeof(Internal::Header)), alignment);
        Internal::Header* header = PointerAlignDown<Internal::Header>(
            (Internal::Header*)(reinterpret_cast<size_t>(result) - sizeof(Internal::Header)), AZStd::alignment_of<Internal::Header>::value);

        header->offset = static_cast<uint32_t>(reinterpret_cast<size_type>(result) - reinterpret_cast<size_type>(data));
        header->size = static_cast<uint32_t>(byteSize);
        m_bytesAllocated += byteSize;

        return result;
    }

    void MallocSchema::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
    {
        (void)byteSize;
        (void)alignment;

        if (!ptr)
        {
            return;
        }

        Internal::Header* header = PointerAlignDown(
            reinterpret_cast<Internal::Header*>(reinterpret_cast<size_t>(ptr) - sizeof(Internal::Header)),
            AZStd::alignment_of<Internal::Header>::value);
        void* freePtr = reinterpret_cast<void*>(reinterpret_cast<size_t>(ptr) - static_cast<size_t>(header->offset));

        m_bytesAllocated -= header->size;
        (*m_freeFn)(freePtr);
    }

    MallocSchema::pointer_type MallocSchema::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
    {
        void* newPtr = Allocate(newSize, newAlignment, 0);
        size_t oldSize = AllocationSize(ptr);

        memcpy(newPtr, ptr, AZStd::min(oldSize, newSize));
        DeAllocate(ptr, 0, 0);

        return newPtr;
    }

    MallocSchema::size_type MallocSchema::Resize(pointer_type ptr, size_type newSize)
    {
        (void)ptr;
        (void)newSize;

        return 0;
    }

    MallocSchema::size_type MallocSchema::AllocationSize(pointer_type ptr)
    {
        if (!ptr)
        {
            return 0;
        }
        Internal::Header* header = PointerAlignDown(
            reinterpret_cast<Internal::Header*>(reinterpret_cast<size_t>(ptr) - sizeof(Internal::Header)),
            AZStd::alignment_of<Internal::Header>::value);
        return header->size;
    }

    MallocSchema::size_type MallocSchema::NumAllocatedBytes() const
    {
        return m_bytesAllocated;
    }

    MallocSchema::size_type MallocSchema::Capacity() const
    {
        return 0;
    }

    MallocSchema::size_type MallocSchema::GetMaxAllocationSize() const
    {
        return 0xFFFFFFFFull;
    }

    MallocSchema::size_type MallocSchema::GetMaxContiguousAllocationSize() const
    {
        return AZ_CORE_MAX_ALLOCATOR_SIZE;
    }

    IAllocatorAllocate* MallocSchema::GetSubAllocator()
    {
        return nullptr;
    }

    void MallocSchema::GarbageCollect()
    {
    }

} // namespace AZ
