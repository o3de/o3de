/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_FIXED_SLIST_H
#define AZSTD_FIXED_SLIST_H 1

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
        : public forward_list<T, static_pool_allocator<typename Internal::forward_list_node<T>, NumberOfNodes > >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef fixed_forward_list<T, NumberOfNodes> this_type;
        typedef forward_list<T, static_pool_allocator<typename Internal::forward_list_node<T>, NumberOfNodes> > base_type;
    public:
        //////////////////////////////////////////////////////////////////////////
        // 23.2.2 construct/copy/destroy
        AZ_FORCE_INLINE explicit fixed_forward_list() {}
        AZ_FORCE_INLINE explicit fixed_forward_list(typename base_type::size_type numElements, typename base_type::const_reference value = typename base_type::value_type())
            : base_type(numElements, value) {}
        template<class InputIterator>
        AZ_FORCE_INLINE fixed_forward_list(const InputIterator& first, const InputIterator& last)
            : base_type(first, last)   {}
        AZ_FORCE_INLINE fixed_forward_list(const this_type& rhs)
            : base_type(rhs)   {}
        AZ_FORCE_INLINE fixed_forward_list(std::initializer_list<T> list)
            : base_type(list) {}
        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
        {
            if (this != &rhs)
            {
                base_type::assign(rhs.begin(), rhs.end());
            }
            return *this;
        }

        //AZ_FORCE_INLINE void * data() const ...
    private:
        // You are not allowed to change the allocator type.
        void                        set_allocator(const typename base_type::allocator_type& allocator) { (void)allocator; }
    };

    template< class T, AZStd::size_t NumberOfNodes >
    AZ_FORCE_INLINE bool operator==(const fixed_forward_list<T, NumberOfNodes>& left, const fixed_forward_list<T, NumberOfNodes>& right)
    {
        return (left.size() == right.size() && equal(left.begin(), left.end(), right.begin()));
    }

    template< class T, AZStd::size_t NumberOfNodes >
    AZ_FORCE_INLINE bool operator!=(const fixed_forward_list<T, NumberOfNodes>& left, const fixed_forward_list<T, NumberOfNodes>& right)
    {
        return !(left == right);
    }
}

#endif // AZSTD_FIXED_SLIST_H
#pragma once
