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
     *  typedef <impl defined>  pointer_type;
     *  // Size type, any container using this allocator will use this size_type. Usually AZStd::size_t
     *  typedef <impl defined>  size_type;
     *  // Pointer difference type, usually AZStd::ptrdiff_t.
     *  typedef <impl defined>  difference_type;
     *  // Allowing memory leaks will instruct allocator's users to never
     *  // even bother to call deallocate. This can result in a faster code. Usually is false_type.
     *  typedef true_type (or false_type) allow_memory_leaks;
     *
     *  allocator(const char* name = "AZSTD Allocator");
     *  allocator(const allocator& rhs);
     *  allocator(const allocator& rhs, const char* name);
     *  allocator& operator=(const allocator& rhs;
     *
     *  pointer_type allocate(size_type byteSize, size_type alignment, int flags = 0);
     *  void         deallocate(pointer_type ptr, size_type byteSize, size_type alignment);
     *  /// Tries to resize an existing memory chunck. Returns the resized memory block or 0 if resize is not supported.
     *  size_type    resize(pointer_type ptr, size_type newSize);
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
     * \attention allow_memory_leaks is important to be set to true for temporary memory buffers like: stack allocators, etc.
     * This will allow AZStd containers to have automatic "leak_and_reset" behavior, which will allow fast
     * destroy without memory deallocation. This is especially important for temporary containers
     * that make multiple allocations (like hash_maps, lists, etc.).
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

        AZ_TYPE_INFO(allocator, "{E9F5A3BE-2B3D-4C62-9E6B-4E00A13AB452}");

        typedef void*               pointer_type;
        typedef AZStd::size_t       size_type;
        typedef AZStd::ptrdiff_t    difference_type;
        typedef AZStd::false_type   allow_memory_leaks;

        AZ_FORCE_INLINE allocator(const char* name = "AZStd::allocator")
            : m_name(name) {}
        AZ_FORCE_INLINE allocator(const allocator& rhs)
            : m_name(rhs.m_name)    {}
        AZ_FORCE_INLINE allocator(const allocator& rhs, const char* name)
            : m_name(name) { (void)rhs; }

        AZ_FORCE_INLINE allocator& operator=(const allocator& rhs)      { m_name = rhs.m_name; return *this; }

        AZ_FORCE_INLINE const char*  get_name() const                   { return m_name; }
        AZ_FORCE_INLINE void         set_name(const char* name)         { m_name = name; }

        pointer_type    allocate(size_type byteSize, size_type alignment, int flags = 0);
        void            deallocate(pointer_type ptr, size_type byteSize, size_type alignment);
        size_type       resize(pointer_type ptr, size_type newSize);
        // max_size actually returns the true maximum size of a single allocation
        size_type       max_size() const;
        size_type       get_allocated_size() const;

        AZ_FORCE_INLINE bool is_lock_free()                             { return false; }
        AZ_FORCE_INLINE bool is_stale_read_allowed()                    { return false; }
        AZ_FORCE_INLINE bool is_delayed_recycling()                     { return false; }

    private:
        const char* m_name;
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
        typedef void*               pointer_type;
        typedef AZStd::size_t       size_type;
        typedef AZStd::ptrdiff_t    difference_type;
        typedef AZStd::false_type   allow_memory_leaks;

        AZ_FORCE_INLINE no_default_allocator(const char* name = "Invalid allocator") { (void)name; }
        AZ_FORCE_INLINE no_default_allocator(const allocator&) {}
        AZ_FORCE_INLINE no_default_allocator(const allocator&, const char*) {}

        // none of this functions are implemented we should get a link error if we use them!
        AZ_FORCE_INLINE allocator& operator=(const allocator& rhs);
        AZ_FORCE_INLINE pointer_type allocate(size_type byteSize, size_type alignment, int flags = 0);
        AZ_FORCE_INLINE void  deallocate(pointer_type ptr, size_type byteSize, size_type alignment);
        AZ_FORCE_INLINE size_type    resize(pointer_type ptr, size_type newSize);

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
