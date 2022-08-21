/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CryCommon/LegacyAllocator.h>

namespace AZ
{
    LegacyAllocator::pointer LegacyAllocator::allocate(size_type byteSize, size_type alignment)
    {
        if (alignment == 0)
        {
            // Some STL containers, like std::vector, seem to have a requirement where a specific minimum alignment will be chosen when the alignment is set to 0
            // Take a look at _Allocate_manually_vector_aligned in xmemory0
            alignment = sizeof(void*) * 2; 
        }

        pointer ptr = m_schema->allocate(byteSize, alignment);
        AZ_PROFILE_MEMORY_ALLOC_EX(MemoryReserved, fileName, lineNum, ptr, byteSize, name ? name : GetName());
        AZ_MEMORY_PROFILE(ProfileAllocation(ptr, byteSize, alignment, 1));
        AZ_Assert(ptr || byteSize == 0, "OOM - Failed to allocate %zu bytes from LegacyAllocator", byteSize);
        return ptr;
    }

    // DeAllocate with file/line, to track when allocs were freed from Cry
    void LegacyAllocator::DeAllocate(pointer ptr, [[maybe_unused]] const char* file, [[maybe_unused]] const int line, size_type byteSize, size_type alignment)
    {
        AZ_PROFILE_MEMORY_FREE_EX(MemoryReserved, file, line, ptr);
        AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));
        m_schema->DeAllocate(ptr, byteSize, alignment);
    }

    // Realloc with file/line, because Cry uses realloc(nullptr) and realloc(ptr, 0) to mimic malloc/free
    LegacyAllocator::pointer LegacyAllocator::ReAllocate(pointer ptr, size_type newSize, size_type newAlignment, [[maybe_unused]] const char* file, [[maybe_unused]] const int line)
    {
        if (newAlignment == 0)
        {
            // Some STL containers, like std::vector, seem to have a requirement where a specific minimum alignment will be chosen when the alignment is set to 0
            // Take a look at _Allocate_manually_vector_aligned in xmemory0
            newAlignment = sizeof(void*) * 2;
        }

        AZ_MEMORY_PROFILE(ProfileReallocationBegin(ptr, newSize));
        AZ_PROFILE_MEMORY_FREE_EX(MemoryReserved, file, line, ptr);
        pointer newPtr = m_schema->ReAllocate(ptr, newSize, newAlignment);
        AZ_PROFILE_MEMORY_ALLOC_EX(MemoryReserved, file, line, newPtr, newSize, "LegacyAllocator Realloc");
        AZ_MEMORY_PROFILE(ProfileReallocationEnd(ptr, newPtr, newSize, newAlignment));
        AZ_Assert(newPtr || newSize == 0, "OOM - Failed to reallocate %zu bytes from LegacyAllocator", newSize);
        return newPtr;
    }

    void LegacyAllocator::deallocate(pointer ptr, size_type byteSize, size_type alignment)
    {
        AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, 0, 0, nullptr));
        Base::deallocate(ptr, byteSize, alignment);
    }

    LegacyAllocator::pointer LegacyAllocator::reallocate(pointer ptr, size_type newSize, size_type newAlignment)
    {
        if (newAlignment == 0)
        {
            // Some STL containers, like std::vector, seem to have a requirement where a specific minimum alignment will be chosen when the alignment is set to 0
            // Take a look at _Allocate_manually_vector_aligned in xmemory0
            newAlignment = sizeof(void*) * 2;
        }

        AZ_MEMORY_PROFILE(ProfileReallocationBegin(ptr, newSize));
        pointer newPtr = Base::reallocate(ptr, newSize, newAlignment);
        AZ_MEMORY_PROFILE(ProfileReallocationEnd(ptr, newPtr, newSize, newAlignment));
        AZ_Assert(newPtr || newSize == 0, "OOM - Failed to reallocate %zu bytes from LegacyAllocator", newSize);
        return newPtr;
    }
}
