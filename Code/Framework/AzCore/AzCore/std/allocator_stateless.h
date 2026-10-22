/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/base.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/Memory/IAllocator.h>

namespace AZStd
{
    struct stateless_allocator
    {
        AZ_TYPE_INFO(stateless_allocator, "{E4976C53-0B20-4F39-8D41-0A76F59A7D68}");

        AZ_ALLOCATOR_DEFAULT_TRAITS

        [[nodiscard]] AZCORE_API pointer allocate(size_type byteSize, align_type alignment = 1);
        AZCORE_API void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0);
        [[nodiscard]] AZCORE_API pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1);

        constexpr size_type resize(
            [[maybe_unused]] pointer ptr,
            [[maybe_unused]] size_type newSize) const
        {
            return 0;
        }

        constexpr size_type max_size() const
        {
            return AZ_TRAIT_OS_MEMORY_MAX_ALLOCATOR_SIZE;
        }

        constexpr bool is_lock_free() const
        {
            return false;
        }

        constexpr bool is_stale_read_allowed() const
        {
            return false;
        }

        constexpr bool is_delayed_recycling() const
        {
            return false;
        }

        friend bool operator==(const stateless_allocator&, const stateless_allocator&) = default;
    };
}
