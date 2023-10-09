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

#include <cstdint>

namespace AZ
{
    namespace Internal
    {
        struct Header
        {
            uint32_t offset;
            uint32_t size;
        };
    }
}

//---------------------------------------------------------------------
// MallocSchema methods
//---------------------------------------------------------------------

AZ::MallocSchema::MallocSchema() : 
    m_bytesAllocated(0)
{
    m_mallocFn = &malloc;
    m_freeFn = &free;
}

AZ::MallocSchema::~MallocSchema()
{
}

AZ::MallocSchema::pointer AZ::MallocSchema::allocate(size_type byteSize, size_type alignment)
{
    if (!byteSize)
    {
        return nullptr;
    }

    if (alignment == 0)
    {
        alignment = sizeof(void*) * 2;  // Default malloc alignment
    }

    AZ_Assert(byteSize < 0x100000000ull, "Malloc allocator only allocates up to 4GB");

    size_type required = byteSize + sizeof(Internal::Header) + ((alignment > sizeof(double)) ? alignment : 0);  // Malloc will align to a minimum boundary for native objects, so we only pad if aligning to a large value
    void* data = (*m_mallocFn)(required);
    void* result = PointerAlignUp(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(data) + sizeof(Internal::Header)), alignment);
    Internal::Header* header = PointerAlignDown<Internal::Header>((Internal::Header*)(reinterpret_cast<std::uintptr_t>(result) - sizeof(Internal::Header)), AZStd::alignment_of<Internal::Header>::value);

    header->offset = static_cast<uint32_t>(reinterpret_cast<std::uintptr_t>(result) - reinterpret_cast<std::uintptr_t>(data));
    header->size = static_cast<uint32_t>(byteSize);
    m_bytesAllocated += byteSize;

    return result;
}

void AZ::MallocSchema::deallocate(pointer ptr, size_type byteSize, size_type alignment)
{
    (void)byteSize;
    (void)alignment;

    if (!ptr)
    {
        return;
    }

    Internal::Header* header = PointerAlignDown(reinterpret_cast<Internal::Header*>(reinterpret_cast<std::uintptr_t>(ptr) - sizeof(Internal::Header)), AZStd::alignment_of<Internal::Header>::value);
    void* freePtr = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(ptr) - static_cast<size_t>(header->offset));

    m_bytesAllocated -= header->size;
    (*m_freeFn)(freePtr);
}

AZ::MallocSchema::pointer AZ::MallocSchema::reallocate(pointer ptr, size_type newSize, size_type newAlignment)
{
    void* newPtr = Allocate(newSize, newAlignment, 0);
    size_t oldSize = AllocationSize(ptr);

    memcpy(newPtr, ptr, AZStd::min(oldSize, newSize));
    DeAllocate(ptr, 0, 0);

    return newPtr;
}

AZ::MallocSchema::size_type AZ::MallocSchema::get_allocated_size(pointer ptr, align_type /* alignment */) const
{
    size_type result = 0;

    if (ptr)
    {
        Internal::Header* header = PointerAlignDown(reinterpret_cast<Internal::Header*>(reinterpret_cast<size_t>(ptr) - sizeof(Internal::Header)), AZStd::alignment_of<Internal::Header>::value);
        result = header->size;
    }

    return result;
}

AZ::MallocSchema::size_type AZ::MallocSchema::NumAllocatedBytes() const
{
    return m_bytesAllocated;
}

void AZ::MallocSchema::GarbageCollect()
{
}
