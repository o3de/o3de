/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/allocator_stateless.h>
#include <AzCore/Memory/OSAllocator_Platform.h>

namespace AZStd
{
    stateless_allocator::pointer stateless_allocator::allocate(size_type byteSize, align_type alignment)
    {
        return AZ_OS_MALLOC(byteSize, alignment);
    }

    void stateless_allocator::deallocate(pointer ptr, [[maybe_unused]] size_type byteSize, [[maybe_unused]] align_type alignment)
    {
        AZ_OS_FREE(ptr);
    }

    stateless_allocator::pointer stateless_allocator::reallocate(pointer ptr, size_type newSize, align_type alignment)
    {
        return AZ_OS_REALLOC(ptr, newSize, alignment);
    }

    auto stateless_allocator::resize(pointer, size_type) -> size_type
    {
        return 0;
    }

    bool stateless_allocator::is_lock_free()
    {
        return false;
    }

    bool stateless_allocator::is_stale_read_allowed()
    {
        return false;
    }

    bool stateless_allocator::is_delayed_recycling()
    {
        return false;
    }

    // comparison operators
    bool operator==(const stateless_allocator&, const stateless_allocator&)
    {
        return true;
    }

    bool operator!=(const stateless_allocator&, const stateless_allocator&)
    {
        return false;
    }
}
