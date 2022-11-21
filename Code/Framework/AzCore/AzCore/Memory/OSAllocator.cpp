/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/OSAllocator.h>

namespace AZ
{
    OSAllocator::OSAllocator()
    {
        Create();
        PostCreate();
#if defined(AZ_ENABLE_TRACING)
        SetProfilingActive(true);
#endif
    }
    OSAllocator::~OSAllocator()
    {
        PreDestroy();
        Destroy();
    }

    //=========================================================================
    // Create
    // [9/2/2009]
    //=========================================================================
    bool OSAllocator::Create()
    {
        m_numAllocatedBytes = 0;
        return true;
    }

    //=========================================================================
    // Destroy
    // [9/2/2009]
    //=========================================================================
    void OSAllocator::Destroy()
    {
    }

    //=========================================================================
    // GetDebugConfig
    // [10/14/2018]
    //=========================================================================
    AllocatorDebugConfig OSAllocator::GetDebugConfig()
    {
        return AllocatorDebugConfig()
            .StackRecordLevels(O3DE_STACK_CAPTURE_DEPTH)
            .UsesMemoryGuards()
            .MarksUnallocatedMemory()
            .ExcludeFromDebugging(false);
    }

    //=========================================================================
    // Allocate
    // [9/2/2009]
    //=========================================================================
    OSAllocator::pointer OSAllocator::allocate(size_type byteSize, size_type alignment)
    {
        pointer address = AZ_OS_MALLOC(byteSize, alignment);

        if (address == nullptr && byteSize > 0)
        {
            AZ_Printf("Memory", "======================================================\n");
            AZ_Printf("Memory", "OSAllocator run out of system memory!\nWe can't track the debug allocator, since it's used for tracking and pipes trought the OS... here are the other allocator status:\n");
            OnOutOfMemory(byteSize, alignment);
        }

#if defined(AZ_ENABLE_TRACING)
        // We assume 1 alignment because alignment is sometimes not passed in deallocate. This does mean that we are under-reporting
        // for cases where alignment != 1 and the OS could not find a block specifically for that alignment (the OS will give use a
        // block that is byteSize+(alignment-1) and place the ptr in the first address that satisfies the alignment).
        const size_type allocatedSize = get_allocated_size(address, 1);
        m_numAllocatedBytes += allocatedSize;
        AZ_PROFILE_MEMORY_ALLOC_EX(MemoryReserved, fileName, lineNum, address, byteSize, name);
        AZ_MEMORY_PROFILE(ProfileAllocation(address, byteSize, alignment, 1));
#endif

        return address;
    }

    //=========================================================================
    // DeAllocate
    // [9/2/2009]
    //=========================================================================
    void OSAllocator::deallocate(pointer ptr, [[maybe_unused]] size_type byteSize, [[maybe_unused]] size_type alignment)
    {
#if defined(AZ_ENABLE_TRACING)
        if (ptr)
        {
            const size_type allocatedSize = get_allocated_size(ptr, 1);
            m_numAllocatedBytes -= allocatedSize;
            AZ_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
            AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));
        }
#endif
        AZ_OS_FREE(ptr);

    }

    OSAllocator::pointer OSAllocator::reallocate(pointer ptr, size_type newSize, align_type alignment)
    {
#if defined(AZ_ENABLE_TRACING)
        const size_type previouslyAllocatedSize = ptr ? get_allocated_size(ptr, 1) : 0;
#endif

        pointer newPtr = AZ_OS_REALLOC(ptr, newSize, static_cast<AZStd::size_t>(alignment));

#if defined(AZ_ENABLE_TRACING)
        const size_type allocatedSize = get_allocated_size(newPtr, 1);
        m_numAllocatedBytes += (allocatedSize - previouslyAllocatedSize);
        AZ_PROFILE_MEMORY_ALLOC_EX(MemoryReserved, fileName, lineNum, address, byteSize, name);
        AZ_MEMORY_PROFILE(ProfileReallocation(ptr, newPtr, allocatedSize, 1));
#endif

        return newPtr;
    }

    OSAllocator::size_type OSAllocator::get_allocated_size(pointer ptr, align_type alignment) const
    {
        return ptr ? AZ_OS_MSIZE(ptr, alignment) : 0;
    }
} // namespace AZ
