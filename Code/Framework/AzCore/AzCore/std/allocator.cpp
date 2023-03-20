/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/allocator.h>
#include <AzCore/Memory/AllocatorInstance.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZStd
{
    allocator::pointer allocator::allocate(size_type byteSize, size_type alignment)
    {
        return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().allocate(byteSize, alignment);
    }

    void allocator::deallocate(pointer ptr, size_type byteSize, size_type alignment)
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Get().deallocate(ptr, byteSize, alignment);
    }

    allocator::pointer allocator::reallocate(pointer ptr, size_type newSize, align_type newAlignment)
    {
        return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().reallocate(ptr, newSize, newAlignment);
    }
} // namespace AZStd
