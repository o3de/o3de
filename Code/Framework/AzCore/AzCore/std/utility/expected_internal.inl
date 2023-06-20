/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/createdestroy.h>

namespace AZStd
{
    using std::addressof;
}

namespace AZStd::Internal
{
    //! expected storage type in-place constructors
    template<class T, class E>
    template<class... Args>
    constexpr expected_storage<T, E>::expected_storage(in_place_t, Args&&... args)
        : m_hasValue{ true }
    {
        if constexpr (!is_void_v<T>)
        {
            construct_at(addressof(m_storage.m_value), AZStd::forward<Args>(args)...);
        }
    }

    template<class T, class E>
    template<class... Args>
    constexpr expected_storage<T, E>::expected_storage(unexpect_t, Args&&... args)
        : m_hasValue{}
    {
        construct_at(addressof(m_storage.m_unexpected), AZStd::forward<Args>(args)...);
    }
    //! expected non-trivial destructor
    template<class T, class E>
    expected_storage_destructor<T, E, special_member_availability::NonTrivial>::~expected_storage_destructor()
    {
        if (this->m_hasValue)
        {
            if constexpr (!is_void_v<T>)
            {
                AZStd::destroy_at(addressof(this->m_storage.m_value));
            }
        }
        else
        {
            AZStd::destroy_at(addressof(this->m_storage.m_unexpected));
        }
    }

    // Implementation of non-trivial copy constructor for copying the AZStd::expected data
    template<class T, class E>
    constexpr expected_storage_copy_constructor<T, E, special_member_availability::NonTrivial>::expected_storage_copy_constructor(
        const expected_storage_copy_constructor& rhs)
    {
        if (rhs.m_hasValue)
        {
            if constexpr (!is_void_v<T>)
            {
                construct_at(addressof(this->m_storage.m_value), rhs.m_storage.m_value);
            }
        }
        else
        {
            construct_at(addressof(this->m_storage.m_unexpected), rhs.m_storage.m_unexpected);
        }

        this->m_hasValue = rhs.m_hasValue;
    }

    // Implementation of non-trivial move constructor for copying the AZStd::expected data
    template<class T, class E>
    constexpr expected_storage_move_constructor<T, E, special_member_availability::NonTrivial>::expected_storage_move_constructor(
        expected_storage_move_constructor&& rhs)
    {
        if (rhs.m_hasValue)
        {
            if constexpr (!is_void_v<T>)
            {
                construct_at(addressof(this->m_storage.m_value), AZStd::move(rhs.m_storage.m_value));
            }
        }
        else
        {
            construct_at(addressof(this->m_storage.m_unexpected), AZStd::move(rhs.m_storage.m_unexpected));
        }

        this->m_hasValue = rhs.m_hasValue;
    }

    //! Helper function to destruct the value at the old and construct a value using the args
    template<class T, class U, class... Args>
    inline constexpr void reinit_expected(T& newval, U& oldval, Args&&... args)
    {
        if constexpr (is_nothrow_constructible_v<T, Args...>)
        {
            AZStd::destroy_at(addressof(oldval));
            construct_at(addressof(newval), AZStd::forward<Args>(args)...);
        }
        else if constexpr (is_nothrow_move_constructible_v<T>)
        {
            T tmp(AZStd::forward<Args>(args)...);
            AZStd::destroy_at(addressof(oldval));
            construct_at(addressof(newval), AZStd::move(tmp));
        }
        else
        {
            // exceptions aren't used in AZStd, no this is hte same as the first block
            AZStd::destroy_at(addressof(oldval));
            construct_at(addressof(newval), AZStd::forward<Args>(args)...);
        }
    }

    template<class T, class E>
    constexpr auto expected_storage_copy_assignment<T, E, special_member_availability::NonTrivial>::operator=(
        const expected_storage_copy_assignment& rhs) -> expected_storage_copy_assignment&
    {
        if (this->m_hasValue && rhs.m_hasValue)
        {
            // Both this instance and the rhs has value
            if constexpr (!is_void_v<T>)
            {
                this->m_storage.m_value = rhs.m_storage.m_value;
            }
        }
        else if (this->m_hasValue)
        {
            // this instance has value and rhs has error
            if constexpr (!is_void_v<T>)
            {
                Internal::reinit_expected(this->m_storage.m_unexpected, this->m_storage.m_value, rhs.m_storage.m_unexpected);
            }
            else
            {
                construct_at(addressof(this->m_storage.m_unexpected), rhs.m_storage.m_unexpected);
            }
        }
        else if (rhs.m_hasValue)
        {
            // this instance has error and rhs has error
            if constexpr (!is_void_v<T>)
            {
                Internal::reinit_expected(this->m_storage.m_value, this->m_storage.m_unexpected, rhs.m_storage.m_value);
            }
            else
            {
                AZStd::destroy_at(addressof(this->m_storage.m_unexpected));
            }
        }
        else
        {
            // Both this instance and the rhs has error
            this->m_storage.m_unexpected = rhs.m_storage.m_unexpected;
        }

        this->m_hasValue = rhs.m_hasValue;
        return *this;
    }

    template<class T, class E>
    constexpr auto expected_storage_move_assignment<T, E, special_member_availability::NonTrivial>::operator=(
        expected_storage_move_assignment&& rhs) -> expected_storage_move_assignment&
    {
        if (this->m_hasValue && rhs.m_hasValue)
        {
            // Both this instance and the rhs has value
            if constexpr (!is_void_v<T>)
            {
                this->m_storage.m_value = AZStd::move(rhs.m_storage.m_value);
            }
        }
        else if (this->m_hasValue)
        {
            // this instance has value and rhs has error
            if constexpr (!is_void_v<T>)
            {
                Internal::reinit_expected(this->m_storage.m_unexpected, this->m_storage.m_value, AZStd::move(rhs.m_storage.m_unexpected));
            }
            else
            {
                construct_at(addressof(this->m_storage.m_unexpected), AZStd::move(rhs.m_storage.m_unexpected));
            }
        }
        else if (rhs.m_hasValue)
        {
            // this instance has error and rhs has error
            if constexpr (!is_void_v<T>)
            {
                Internal::reinit_expected(this->m_storage.m_value, this->m_storage.m_unexpected, AZStd::move(rhs.m_storage.m_value));
            }
            else
            {
                AZStd::destroy_at(addressof(this->m_storage.m_unexpected));
            }
        }
        else
        {
            // Both this instance and the rhs has error
            this->m_storage.m_unexpected = AZStd::move(rhs.m_storage.m_unexpected);
        }

        this->m_hasValue = rhs.m_hasValue;
        return *this;
    }
}
