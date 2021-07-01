/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomCore/std/containers/vector_set_base.h>

#include <AzCore/std/containers/vector.h>

namespace AZStd
{
    template <typename Key, typename Compare = AZStd::less<Key>, typename Allocator = AZStd::allocator>
    class vector_set
        : public vector_set_base<Key, Compare, AZStd::vector<Key, Allocator>>
    {
        using base_type = vector_set_base<Key, Compare, AZStd::vector<Key, Allocator>>;
        using this_type = vector_set<Key, Compare, Allocator>;
    public:
        using allocator_type = Allocator;

        AZ_FORCE_INLINE vector_set() = default;

        AZ_FORCE_INLINE vector_set(const allocator_type& allocator)
            : base_type(allocator)
        {}

        template <typename InputIterator>
        vector_set(InputIterator first, InputIterator last)
            : base_type(allocator_type())
        {
            base_type::assign(first, last);
        }

        template <typename InputIterator>
        vector_set(InputIterator first, InputIterator last, const allocator_type& allocator)
            : base_type(allocator)
        {
            base_type::assign(first, last);
        }

        vector_set(const AZStd::initializer_list<Key> list)
            : base_type(allocator_type())
        {
            base_type::assign(list.begin(), list.end());
        }

        vector_set(const AZStd::initializer_list<Key> list, const allocator_type& allocator)
            : base_type(allocator)
        {
            base_type::assign(list.begin(), list.end());
        }

        void reserve(size_t capacity)
        {
            base_type::m_container.reserve(capacity);
        }

        void shrink_to_fit()
        {
            base_type::m_container.shrink_to_fit();
        }

        allocator_type&         get_allocator()         { return base_type::m_allocator; }
        const allocator_type&   get_allocator() const   { return base_type::m_allocator; }

        void set_allocator(const allocator_type& allocator)
        {
            base_type::m_container.set_allocator(allocator);
        }
    };
}
