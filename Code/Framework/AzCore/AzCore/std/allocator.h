/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_ALLOCATOR_H
#define AZSTD_ALLOCATOR_H 1

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/RTTI/TypeInfoSimple.h>

namespace AZStd
{
    /**
     * \page Allocators
     *
     * AZStd uses simple and fast allocator model, it does not follow the \ref CStd. It is not templated on any type, thus is doesn't care about
     * construction or destruction. We do require
     * name support, in most cases should be just a pointer to a literal.
     *
     * This is the specification for an AZSTD allocator:
     * \code
     * class allocator
     * {
     * public:
     *  // Type of the returned pointer. Usually void*
     *  typedef <impl defined>  pointer;
     *  // Size type, any container using this allocator will use this size_type. Usually AZStd::size_t
     *  typedef <impl defined>  size_type;
     *  // Pointer difference type, usually AZStd::ptrdiff_t.
     *  typedef <impl defined>  difference_type;
     *
     *  allocator(const char* name = "AZSTD Allocator");
     *  allocator(const allocator& rhs);
     *  allocator(const allocator& rhs, const char* name);
     *  allocator& operator=(const allocator& rhs;
     *
     *  pointer      allocate(size_type byteSize, size_type alignment);
     *  void         deallocate(pointer ptr, size_type byteSize, size_type alignment);
     *  /// Tries to resize an existing memory chunck. Returns the resized memory block or 0 if resize is not supported.
     *  size_type    resize(pointer ptr, size_type newSize);
     *
     *  const char* get_name() const;
     *  void        set_name(const char* name);
     *
     *  // Returns theoretical maximum size of a single contiguous allocation from this allocator.
     *  size_type               max_size() const;
     *  <optional> size_type    get_allocated_size() const;
     * };
     *
     * bool operator==(const allocator& a, const allocator& b);
     * bool operator!=(const allocator& a, const allocator& b);
     * \endcode
     *
     * \li \ref allocator "Default Allocator"
     * \li \ref no_default_allocator "Invalid Default Allocator"
     * \li \ref static_buffer_allocator "Static Buffer Allocator"
     * \li \ref static_pool_allocator "Static Pool Allocator"
     * \li \ref allocator_ref "Allocator Reference"
     *
     */

    /**
     * All allocation will be piped to AZ::SystemAllocator, make sure it is created!
     */
    class allocator
    {
    public:
        using value_type = void;
        using pointer = void*;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;
        using align_type = AZStd::size_t;
        using propagate_on_container_copy_assignment = AZStd::true_type;
        using propagate_on_container_move_assignment = AZStd::true_type;

        AZ_FORCE_INLINE allocator() = default;
        AZ_FORCE_INLINE allocator(const char*) {};
        AZ_FORCE_INLINE allocator(const allocator& rhs) = default;
        AZ_FORCE_INLINE allocator([[maybe_unused]] const allocator& rhs, [[maybe_unused]] const char* name) {}
        AZ_FORCE_INLINE allocator& operator=(const allocator& rhs) = default;

        pointer allocate(size_type byteSize, size_type alignment);
        void deallocate(pointer ptr, size_type byteSize, size_type alignment);
        pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1);

        size_type max_size() const
        {
            return AZ_TRAIT_OS_MEMORY_MAX_ALLOCATOR_SIZE;
        }

        size_type get_allocated_size() const
        {
            return 0;
        }

        AZ_FORCE_INLINE bool is_lock_free()
        {
            return false;
        }

        AZ_FORCE_INLINE bool is_stale_read_allowed()
        {
            return false;
        }

        AZ_FORCE_INLINE bool is_delayed_recycling()
        {
            return false;
        }
    };

    AZ_FORCE_INLINE bool operator==(const AZStd::allocator& a, const AZStd::allocator& b)
    {
        (void)a;
        (void)b;
        return true;
    }

    AZ_FORCE_INLINE bool operator!=(const AZStd::allocator& a, const AZStd::allocator& b)
    {
        (void)a;
        (void)b;
        return false;
    }

    /**
    * No Default allocator implementation (invalid allocator).
    * - If you want to make sure we don't use default allocator, define \ref AZStd::allocator to AZStd::no_default_allocator (this allocator)
    *  the code which try to use it, will not compile.
    *
    * - If you have a compile error here, this means that
    * you did not provide allocator to your container. This
    * is intentional so you make sure where do you allocate from. If this is not
    * the AZStd integration this means that you might have defined in your
    * code a default allocator or even have predefined container types. Use them.
    */
    class no_default_allocator
    {
    public:
        using pointer = void*;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;

        AZ_FORCE_INLINE no_default_allocator(const char* name = "Invalid allocator") { (void)name; }
        AZ_FORCE_INLINE no_default_allocator(const allocator&) {}
        AZ_FORCE_INLINE no_default_allocator(const allocator&, const char*) {}

        // none of this functions are implemented we should get a link error if we use them!
        AZ_FORCE_INLINE allocator& operator=(const allocator& rhs);
        AZ_FORCE_INLINE pointer allocate(size_type byteSize, size_type alignment);
        AZ_FORCE_INLINE void  deallocate(pointer ptr, size_type byteSize, size_type alignment);
        AZ_FORCE_INLINE size_type    resize(pointer ptr, size_type newSize);

        AZ_FORCE_INLINE const char*  get_name() const;
        AZ_FORCE_INLINE void         set_name(const char* name);

        AZ_FORCE_INLINE size_type   max_size() const;

        AZ_FORCE_INLINE bool is_lock_free();
        AZ_FORCE_INLINE bool is_stale_read_allowed();
        AZ_FORCE_INLINE bool is_delayed_recycling();
    };
}

#endif // AZSTD_ALLOCATOR_H
#pragma once
