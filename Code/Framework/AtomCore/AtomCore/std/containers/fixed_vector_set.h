/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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