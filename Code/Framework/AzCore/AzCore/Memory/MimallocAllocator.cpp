/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/MimallocAllocator.h>
#include <AzCore/PlatformIncl.h>
#include <mimalloc.h>

namespace AZ
{
    MimallocSchema::MimallocSchema()
    {
        // TODO: set up some runtime options for mimalloc here if needed. For now, just leave as is.
    }

    AllocateAddress MimallocSchema::allocate(size_type byteSize, size_type alignment)
    {
        if (alignment == 0)
        {
            alignment = 1;
        }
        void* ptr = mi_malloc_aligned(byteSize, alignment);
        byteSize = mi_usable_size(ptr);
        m_allocatedBytes += byteSize;
        return AllocateAddress(ptr, byteSize);
    }

    MimallocSchema::size_type MimallocSchema::deallocate(pointer ptr, [[maybe_unused]] size_type byteSize, [[maybe_unused]] size_type alignment)
    {
        if (!ptr)
        {
            return 0;
        }
        byteSize = mi_usable_size(ptr);
        mi_free(ptr);
        m_allocatedBytes -= byteSize;
        return byteSize;
    }

    AllocateAddress MimallocSchema::reallocate(pointer ptr, size_type newSize, size_type newAlignment)
    {
        if (!ptr)
        {
            return allocate(newSize, newAlignment);
        }

        if (newAlignment == 0)
        {
            newAlignment = 1;
        }

        size_type oldSize = mi_usable_size(ptr);
        void* newPtr = mi_realloc_aligned(ptr, newSize, newAlignment);
        if (!newPtr)
        {
            return {};
        }

        const size_type allocatedSize = mi_usable_size(newPtr);
        if (allocatedSize >= oldSize)
        {
            m_allocatedBytes += allocatedSize - oldSize;
        }
        else
        {
            m_allocatedBytes -= oldSize - allocatedSize;
        }

        return AllocateAddress(newPtr, allocatedSize);
    }

    MimallocSchema::size_type MimallocSchema::get_allocated_size(pointer ptr, [[maybe_unused]] align_type alignment) const
    {
        return mi_usable_size(ptr);
    }

    MimallocSchema::size_type MimallocSchema::NumAllocatedBytes() const
    {
        return m_allocatedBytes;
    }

    void MimallocSchema::GarbageCollect()
    {
        mi_collect(false);
    }
} // namespace AZ
