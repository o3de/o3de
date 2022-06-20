/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_ALLOCATOR_REF_H
#define AZSTD_ALLOCATOR_REF_H 1

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>

namespace AZStd
{
    /**
    * This allocator allows us to share allocator instance between different containers.
    */
    template<class Allocator>
    class allocator_ref
    {
        typedef allocator_ref<Allocator> this_type;
    public:
        typedef typename Allocator::pointer_type        pointer_type;
        typedef typename Allocator::size_type           size_type;
        typedef typename Allocator::difference_type     difference_type;
        typedef typename Allocator::allow_memory_leaks  allow_memory_leaks;
        typedef Allocator*                              allocator_pointer;
        typedef Allocator&                              allocator_reference;

        AZ_FORCE_INLINE allocator_ref(allocator_reference allocator, const char* name = "AZStd::allocator_ref")
            : m_name(name)
            , m_allocator(&allocator) {}
        AZ_FORCE_INLINE allocator_ref(const this_type& rhs)
            : m_name(rhs.m_name)
            , m_allocator(rhs.m_allocator)              {}
        AZ_FORCE_INLINE allocator_ref(const this_type& rhs, const char* name)
            : m_name(name)
            , m_allocator(rhs.m_allocator)  {}

        AZ_FORCE_INLINE const char*  get_name() const           { return m_name; }
        AZ_FORCE_INLINE void         set_name(const char* name) { m_name = name; }

        constexpr size_type          max_size() const { return m_allocator->max_size(); }

        AZ_FORCE_INLINE size_type   get_allocated_size() const  { return m_allocator->get_allocated_size(); }

        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)              { m_name = rhs.m_name; m_allocator = rhs.m_allocator; return *this; }
        AZ_FORCE_INLINE pointer_type allocate(size_type byteSize, size_type alignment, int flags = 0) { return m_allocator->allocate(byteSize, alignment, flags); }

        AZ_FORCE_INLINE void  deallocate(pointer_type ptr, size_type byteSize, size_type alignment) { m_allocator->deallocate(ptr, byteSize, alignment); }

        AZ_FORCE_INLINE size_type    resize(pointer_type ptr, size_type newSize)                    { return m_allocator->resize(ptr, newSize); }

        AZ_FORCE_INLINE allocator_reference get_allocator() const           { return *m_allocator; }
    private:

        const char* m_name;
        allocator_pointer m_allocator;
    };

    template<class Allocator>
    AZ_FORCE_INLINE bool operator==(const AZStd::allocator_ref<Allocator>& a, const AZStd::allocator_ref<Allocator>& b)
    {
        return (a.get_allocator() == b.get_allocator());
    }

    template<class Allocator>
    AZ_FORCE_INLINE bool operator!=(const AZStd::allocator_ref<Allocator>& a, const AZStd::allocator_ref<Allocator>& b)
    {
        return (a.get_allocator() != b.get_allocator());
    }
}

#endif // AZSTD_ALLOCATOR_REF_H
#pragma once
