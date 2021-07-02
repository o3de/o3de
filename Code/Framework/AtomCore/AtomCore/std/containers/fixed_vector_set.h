/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomCore/std/containers/vector_set_base.h>

namespace AZStd
{
    template <typename Key, size_t Capacity, typename Compare = AZStd::less<Key>>
    class fixed_vector_set
        : public vector_set_base<Key, Compare, AZStd::fixed_vector<Key, Capacity>>
    {
        using base_type = vector_set_base<Key, Compare, AZStd::fixed_vector<Key, Capacity>>;
        using this_type = fixed_vector_set<Key, Capacity, Compare>;
    public:
        explicit fixed_vector_set() = default;

        template <typename InputIterator>
        fixed_vector_set(InputIterator first, InputIterator last)
        {
            base_type::assign(first, last);
        }

        fixed_vector_set(const AZStd::initializer_list<Key> list)
        {
            base_type::assign(list.begin(), list.end());
        }
    };
}
