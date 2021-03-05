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

#include <AzCore/std/allocator.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZStd
{
    //=========================================================================
    // allocate
    // [1/1/2008]
    //=========================================================================
    allocator::pointer_type
    allocator::allocate(size_type byteSize, size_type alignment, int flags)
    {
        return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(byteSize, alignment, flags, m_name, __FILE__, __LINE__, 1);
    }
    //=========================================================================
    // deallocate
    // [1/1/2008]
    //=========================================================================
    void
    allocator::deallocate(pointer_type ptr, size_type byteSize, size_type alignment)
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(ptr, byteSize, alignment);
    }

    //=========================================================================
    // resize
    // [1/1/2008]
    //=========================================================================
    allocator::size_type
    allocator::resize(pointer_type ptr, size_type newSize)
    {
        return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Resize(ptr, newSize);
    }

    //=========================================================================
    // get_max_size
    // [1/1/2008]
    //=========================================================================
    allocator::size_type
    allocator::get_max_size() const
    {
        return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetMaxAllocationSize();
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
