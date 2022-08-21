/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/allocator.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZStd
{
    //=========================================================================
    // allocate
    // [1/1/2008]
    //=========================================================================
    allocator::pointer
    allocator::allocate(size_type byteSize, size_type alignment)
    {
        return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().allocate(byteSize, alignment);
    }
    //=========================================================================
    // deallocate
    // [1/1/2008]
    //=========================================================================
    void
    allocator::deallocate(pointer ptr, size_type byteSize, size_type alignment)
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Get().deallocate(ptr, byteSize, alignment);
    }

    //=========================================================================
    // resize
    // [1/1/2008]
    //=========================================================================
    allocator::size_type
    allocator::resize(pointer ptr, size_type newSize)
    {
        return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Resize(ptr, newSize);
    }

    auto allocator::max_size() const -> size_type
    {
        return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetMaxContiguousAllocationSize();
    }

    //=========================================================================
    // get_allocated_size
    // [1/1/2008]
    //=========================================================================
    allocator::size_type
    allocator::get_allocated_size() const
    {
        return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().NumAllocatedBytes();
    }
}
