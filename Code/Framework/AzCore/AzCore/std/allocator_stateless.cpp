/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/allocator_stateless.h>
#include <AzCore/Memory/OSAllocator.h>

namespace AZStd
{
    stateless_allocator::stateless_allocator(const char* name)
        : m_name(name) {}

    const char* stateless_allocator::get_name() const
    {
        return m_name;
    }

    void stateless_allocator::set_name(const char* name)
    {
        m_name = name;
    }

    auto stateless_allocator::allocate(size_type byteSize) -> pointer_type
    {
        return allocate(byteSize, AZ_DEFAULT_ALIGNMENT, 0);
    }

    auto stateless_allocator::allocate(size_type byteSize, size_type alignment, int) -> pointer_type
    {
        pointer_type address = AZ_OS_MALLOC(byteSize, alignment);

        if (address == nullptr)
        {
            AZ_Error("Memory", false, "stateless_allocator ran out of system memory!\n");
        }

        return address;
    }

    void stateless_allocator::deallocate(pointer_type ptr, size_type)
    {
        AZ_OS_FREE(ptr);
    }

    void stateless_allocator::deallocate(pointer_type ptr, size_type, size_type)
    {
        AZ_OS_FREE(ptr);
    }

    auto stateless_allocator::max_size() const -> size_type
    {
        return AZ_CORE_MAX_ALLOCATOR_SIZE;
    }

    stateless_allocator stateless_allocator::select_on_container_copy_construction() const
    {
        return *this;
    }

    auto stateless_allocator::resize(pointer_type, size_type) -> size_type
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
