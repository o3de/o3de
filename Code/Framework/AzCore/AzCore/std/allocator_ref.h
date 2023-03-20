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
        using pointer = typename Allocator::pointer;
        using size_type = typename Allocator::size_type;
        using difference_type = typename Allocator::difference_type;
        using allocator_pointer = Allocator *;
        using allocator_reference = Allocator &;

        AZ_FORCE_INLINE allocator_ref(allocator_reference allocator)
            : m_allocator(&allocator) {}
        AZ_FORCE_INLINE allocator_ref(const allocator_ref& rhs) = default;
        AZ_FORCE_INLINE allocator_ref(allocator_ref&& rhs) = default;
        AZ_FORCE_INLINE allocator_ref& operator=(const allocator_ref& rhs) = default;
        AZ_FORCE_INLINE allocator_ref& operator=(allocator_ref&& rhs) = default;

        constexpr size_type          max_size() const { return m_allocator->max_size(); }

        AZ_FORCE_INLINE size_type   get_allocated_size() const  { return m_allocator->get_allocated_size(); }

        AZ_FORCE_INLINE pointer allocate(size_type byteSize, size_type alignment) { return m_allocator->allocate(byteSize, alignment); }

        AZ_FORCE_INLINE void  deallocate(pointer ptr, size_type byteSize, size_type alignment) { m_allocator->deallocate(ptr, byteSize, alignment); }

        AZ_FORCE_INLINE size_type    resize(pointer ptr, size_type newSize)                    { return m_allocator->resize(ptr, newSize); }

        AZ_FORCE_INLINE allocator_reference get_allocator() const           { return *m_allocator; }
    private:
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
