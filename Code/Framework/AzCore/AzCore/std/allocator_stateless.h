/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/RTTI/TypeInfoSimple.h>

namespace AZStd
{
    class stateless_allocator
    {
    public:

        AZ_TYPE_INFO(stateless_allocator, "{E4976C53-0B20-4F39-8D41-0A76F59A7D68}");

        using value_type = uint8_t;
        using pointer_type = void*;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using allow_memory_leaks = AZStd::true_type;

        stateless_allocator(const char* name = "AZStd::stateless_allocator");
        stateless_allocator(const stateless_allocator& rhs) = default;

        stateless_allocator& operator=(const stateless_allocator& rhs) = default;

        const char* get_name() const;
        void set_name(const char* name);

        pointer_type allocate(size_type byteSize);
        pointer_type allocate(size_type byteSize, size_type alignment, int flags = 0);
        void deallocate(pointer_type ptr, size_type alignment);
        void deallocate(pointer_type ptr, size_type byteSize, size_type alignment);

        // max_size actually returns the true maximum size of a single allocation
        size_type max_size() const;

        // Returns a copy of the allocator
        stateless_allocator select_on_container_copy_construction() const;

        //! extensions
        size_type resize(pointer_type ptr, size_type newSize);

        bool is_lock_free();
        bool is_stale_read_allowed();
        bool is_delayed_recycling();

    private:
        const char* m_name;
    };

    bool operator==(const stateless_allocator& left, const stateless_allocator& right);
    bool operator!=(const stateless_allocator& left, const stateless_allocator& right);
}
