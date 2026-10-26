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
    /**
     * \page Allocators
     *
     * AZStd uses a simple, non-templated allocator model.
     * Unlike std::allocator, AZStd allocators are not parameterized on a value type,
     * they operate on raw bytes with explicit size and alignment parameters.
     * This makes them lightweight and avoids per-type allocator instantiation.
     *
     * AZStd allocators support alignment directly in allocate/deallocate,
     * which differs from std::allocator where alignment is handled implicitly via operator new.
     *
     * Allocator type requirements for use with AZStd containers:
     * \code
     * struct allocator
     * {
     *     using value_type = void;
     *     using size_type = size_t;
     *     using difference_type = ptrdiff_t;
     *     using align_type = size_t;
     *
     *     [[nodiscard]] void* allocate(size_type byteSize, size_type alignment);
     *     void deallocate(void* ptr, size_type byteSize, size_type alignment);
     *
     *     // Optional
     *     [[nodiscard]] void* reallocate(void* ptr, size_type newSize, align_type alignment = 1);
     *     size_type max_size() const;
     *     size_type get_allocated_size() const;
     *
     *     friend bool operator==(const allocator&, const allocator&);
     * };
     * \endcode
     *
     * \li \ref allocator "Default Allocator"
     * \li \ref no_default_allocator "Invalid Default Allocator"
     * \li \ref static_buffer_allocator "Static Buffer Allocator"
     * \li \ref static_pool_allocator "Static Pool Allocator"
     * \li \ref allocator_ref "Allocator Reference"
     */

    /**
     * Default allocator
     *
     * All allocations are forwarded to AZ::SystemAllocator.
     * This is a stateless allocator; all instances compare equal.
     */
    struct AZCORE_API allocator
    {
        using value_type = void;
        using pointer = void*;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using align_type = size_t;

        allocator() = default;
        allocator(const allocator&) = default;
        allocator& operator=(const allocator&) = default;

        [[nodiscard]] pointer allocate(size_type byteSize, size_type alignment);
        void deallocate(pointer ptr, size_type byteSize, size_type alignment);
        [[nodiscard]] pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1);

        constexpr size_type max_size() const
        {
            return AZ_TRAIT_OS_MEMORY_MAX_ALLOCATOR_SIZE;
        }

        constexpr size_type get_allocated_size() const
        {
            return 0;
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

        friend bool operator==(const allocator&, const allocator&) = default;
    };

    /**
     * Intentionally broken allocator used by fixed-size containers.
     *
     * If a container attempts to allocate through this,
     * you get a compile-time error indicating that you must provide a real allocator.
     */
    struct AZCORE_API no_default_allocator final
    {
        using pointer = void*;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        no_default_allocator() = default;
        no_default_allocator(const no_default_allocator&) = default;
        no_default_allocator& operator=(const no_default_allocator&) = default;

        // All operations are deleted to produce compile-time errors if used
        pointer allocate(size_type byteSize, size_type alignment) = delete;
        void deallocate(pointer ptr, size_type byteSize, size_type alignment) = delete;
        size_type resize(pointer ptr, size_type newSize) = delete;
        size_type max_size() const = delete;
        bool is_lock_free() = delete;
        bool is_stale_read_allowed() = delete;
        bool is_delayed_recycling() = delete;
    };
}
