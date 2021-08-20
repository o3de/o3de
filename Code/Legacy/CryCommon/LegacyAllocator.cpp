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
    LegacyAllocator::pointer_type LegacyAllocator::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
    {
        if (alignment == 0)
        {
            // Some STL containers, like std::vector, seem to have a requirement where a specific minimum alignment will be chosen when the alignment is set to 0
            // Take a look at _Allocate_manually_vector_aligned in xmemory0
            alignment = sizeof(void*) * 2; 
        }

        pointer_type ptr = m_schema->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);
        AZ_PROFILE_MEMORY_ALLOC_EX(MemoryReserved, fileName, lineNum, ptr, byteSize, name ? name : GetName());
        AZ_MEMORY_PROFILE(ProfileAllocation(ptr, byteSize, alignment, name, fileName, lineNum, suppressStackRecord));
        AZ_Assert(ptr || byteSize == 0, "OOM - Failed to allocate %zu bytes from LegacyAllocator", byteSize);
        return ptr;
    }

    // DeAllocate with file/line, to track when allocs were freed from Cry
    void LegacyAllocator::DeAllocate(pointer_type ptr, [[maybe_unused]] const char* file, [[maybe_unused]] const int line, size_type byteSize, size_type alignment)
    {
        AZ_PROFILE_MEMORY_FREE_EX(MemoryReserved, file, line, ptr);
        AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));
        m_schema->DeAllocate(ptr, byteSize, alignment);
    }

    // Realloc with file/line, because Cry uses realloc(nullptr) and realloc(ptr, 0) to mimic malloc/free
    LegacyAllocator::pointer_type LegacyAllocator::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment, [[maybe_unused]] const char* file, [[maybe_unused]] const int line)
    {
        if (newAlignment == 0)
        {
            // Some STL containers, like std::vector, seem to have a requirement where a specific minimum alignment will be chosen when the alignment is set to 0
            // Take a look at _Allocate_manually_vector_aligned in xmemory0
            newAlignment = sizeof(void*) * 2;
        }

        AZ_MEMORY_PROFILE(ProfileReallocationBegin(ptr, newSize));
        AZ_PROFILE_MEMORY_FREE_EX(MemoryReserved, file, line, ptr);
        pointer_type newPtr = m_schema->ReAllocate(ptr, newSize, newAlignment);
        AZ_PROFILE_MEMORY_ALLOC_EX(MemoryReserved, file, line, newPtr, newSize, "LegacyAllocator Realloc");
        AZ_MEMORY_PROFILE(ProfileReallocationEnd(ptr, newPtr, newSize, newAlignment));
        AZ_Assert(newPtr || newSize == 0, "OOM - Failed to reallocate %zu bytes from LegacyAllocator", newSize);
        return newPtr;
    }

    void LegacyAllocator::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
    {
        AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, 0, 0, nullptr));
        Base::DeAllocate(ptr, byteSize, alignment);
    }

    LegacyAllocator::pointer_type LegacyAllocator::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
    {
        if (newAlignment == 0)
        {
            // Some STL containers, like std::vector, seem to have a requirement where a specific minimum alignment will be chosen when the alignment is set to 0
            // Take a look at _Allocate_manually_vector_aligned in xmemory0
            newAlignment = sizeof(void*) * 2;
        }

        AZ_MEMORY_PROFILE(ProfileReallocationBegin(ptr, newSize));
        pointer_type newPtr = Base::ReAllocate(ptr, newSize, newAlignment);
        AZ_MEMORY_PROFILE(ProfileReallocationEnd(ptr, newPtr, newSize, newAlignment));
        AZ_Assert(newPtr || newSize == 0, "OOM - Failed to reallocate %zu bytes from LegacyAllocator", newSize);
        return newPtr;
    }
}
