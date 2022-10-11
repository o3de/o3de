/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/forward_list.h>
#include <AzCore/std/allocator_static.h>

namespace AZStd
{
    /**
    * Fixed version of the \ref AZStd::list. All of their functionality is the same
    * except we do not have allocator and never allocate memory.
    * \note At the moment fixed_list is based on a list with static pool allocator (which might change),
    * so don't rely on it.
    *
    * Check the fixed_list \ref AZStdExamples.
    */
    template< class T, AZStd::size_t NumberOfNodes>
    class fixed_forward_list
        : public forward_list<T, static_pool_allocator<Internal::forward_list_node<T>, NumberOfNodes > >
    {
        using base_type = forward_list<T, static_pool_allocator<Internal::forward_list_node<T>, NumberOfNodes>>;
    public:
        using base_type::base_type;
        friend bool operator==(const fixed_forward_list& x, const fixed_forward_list& y)
        {
            return static_cast<const base_type&>(x) == static_cast<const base_type&>(y);
        }

        friend bool operator!=(const fixed_forward_list& x, const fixed_forward_list& y)
        {
            return !operator==(x, y);
        }

        friend void swap(fixed_forward_list& x, fixed_forward_list& y)
            noexcept(noexcept(x.swap(y)))
        {
            static_cast<base_type&>(x).swap(static_cast<base_type&>(y));
        }
    };
}
