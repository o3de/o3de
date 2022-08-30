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
        return AllocatorDebugConfig().ExcludeFromDebugging();
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

        m_numAllocatedBytes += byteSize;

        return address;
    }

    //=========================================================================
    // DeAllocate
    // [9/2/2009]
    //=========================================================================
    void OSAllocator::deallocate(pointer ptr, size_type byteSize, [[maybe_unused]] size_type alignment)
    {
        AZ_OS_FREE(ptr);

        m_numAllocatedBytes -= byteSize;
    }

    OSAllocator::pointer OSAllocator::reallocate(pointer ptr, size_type newSize, align_type alignment)
    {
        if (ptr)
        {
            m_numAllocatedBytes -= get_allocated_size(ptr, alignment);
        }

        // Realloc in most platforms doesnt support allocating from a nulltpr
        pointer newPtr = ptr ? AZ_OS_REALLOC(ptr, newSize, static_cast<AZStd::size_t>(alignment))
                             : AZ_OS_MALLOC(newSize, static_cast<AZStd::size_t>(alignment));

        m_numAllocatedBytes += get_allocated_size(newPtr, alignment);

        return newPtr;
    }

    OSAllocator::size_type OSAllocator::get_allocated_size(pointer ptr, align_type alignment) const
    {
        return ptr ? AZ_OS_MSIZE(ptr, alignment) : 0;
    }
} // namespace AZ
