/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZStd
{
    template<class E>
    template<class Err, class>
    constexpr unexpected<E>::unexpected(Err&& err)
        : m_unexpected(AZStd::forward<Err>(err))
    {
    }

    template<class E>
    template<class... Args, class>
    constexpr unexpected<E>::unexpected(in_place_t, Args&&... args)
        : m_unexpected{ AZStd::forward<Args>(args)... }
    {
    }

    template<class E>
    template<class U, class... Args, class>
    constexpr unexpected<E>::unexpected(in_place_t, initializer_list<U> il, Args&&... args)
        : m_unexpected{ il, AZStd::forward<Args>(args)... }
    {
    }

    template<class E>
    constexpr const E& unexpected<E>::error() const& noexcept
    {
        return m_unexpected;
    }
    template<class E>
    constexpr E& unexpected<E>::error() & noexcept
    {
        return m_unexpected;
    }
    template<class E>
    constexpr const E&& unexpected<E>::error() const&& noexcept
    {
        return AZStd::move(m_unexpected);
    }
    template<class E>
    constexpr E&& unexpected<E>::error() && noexcept
    {
        return AZStd::move(m_unexpected);
    }

    template<class E>
    constexpr void unexpected<E>::swap(unexpected& other) noexcept(is_nothrow_swappable_v<E>)
    {
        using AZStd::swap;
        swap(m_unexpected, other.m_unexpected);
    }

    template<class E, class E2>
    constexpr bool operator==(const unexpected<E>& x, const unexpected<E2>& y)
    {
        static_assert(
            Internal::boolean_testable<decltype(x.error() == y.error())>,
            "Cannot invoke operator= on"
            " error types as they are not comparable");
        return x.error() == y.error();
    }

    template<class E, class E2>
    constexpr bool operator!=(const unexpected<E>& x, const unexpected<E2>& y)
    {
        return !operator==(x, y);
    }

    template<class E>
    constexpr void swap(unexpected<E>& x, unexpected<E>& y) noexcept(noexcept(x.swap(y)))
    {
        static_assert(is_swappable_v<E>, "Cannot invoke swap with an error type that is not swappable");
        x.swap(y);
    }

    //! AZStd::expected
    //! expected constructors
    template<class T, class E>
    template <bool, class>
    constexpr expected<T, E>::expected() noexcept
        : base_type{ in_place }
    {}

    //! expected<U, G> copy conversion constructor
    template<class T, class E>
    template<class U, class G, class>
    constexpr expected<T, E>::expected(const expected<U, G>& rhs)
    {
        if (rhs.has_value())
        {
            if constexpr (!is_void_v<T>)
            {
                construct_at(addressof(this->m_storage.m_value), AZStd::forward<const U&>(rhs.value()));
            }
        }
        else
        {
            construct_at(addressof(this->m_storage.m_unexpected), AZStd::forward<const G&>(rhs.error()));
        }

        this->m_hasValue = rhs.has_value();
    }


    //! expected<U, G> move conversion constructor
    template<class T, class E>
    template<class U, class G, class>
    constexpr expected<T, E>::expected(expected<U, G>&& rhs)
    {
        if (rhs.has_value())
        {
            if constexpr (!is_void_v<T>)
            {
                construct_at(addressof(this->m_storage.m_value), AZStd::forward<U>(rhs.value()));
            }
        }
        else
        {
            construct_at(addressof(this->m_storage.m_unexpected), AZStd::forward<G>(rhs.error()));
        }

        this->m_hasValue = rhs.has_value();
    }

    //! value direct initialization constructor
    template<class T, class E>
    template<class U, class>
    constexpr expected<T, E>::expected(U&& rhs)
        : base_type{ in_place, AZStd::forward<U>(rhs) }
    {
    }

    //! error unexpected<G> copy constructor
    template<class T, class E>
    template<class G, class>
    constexpr expected<T, E>::expected(const unexpected<G>& rhs)
        : base_type{ unexpect, AZStd::forward<const G&>(rhs.error()) }
    {
    }


    //! error unexpected<G> move constructor
    template<class T, class E>
    template< class G, class>
    constexpr expected<T, E>::expected(unexpected<G>&& rhs)
        : base_type{ unexpect, AZStd::forward<G>(rhs.error()) }
    {
    }

    template<class T, class E>
    template<class... Args, class>
    constexpr expected<T, E>::expected(in_place_t, Args&&... args)
        : base_type{ in_place, AZStd::forward<Args>(args)... }
    {
    }

    template<class T, class E>
    template<class U, class... Args, class>
    constexpr expected<T, E>::expected(in_place_t, initializer_list<U> il, Args&&... args)
        : base_type{ in_place, il, AZStd::forward<Args>(args)... }
    {
    }

    //! expected<void, E> specializtion in-place constructor
    template<class T, class E>
    template<bool, class>
    constexpr expected<T, E>::expected(in_place_t)
        : expected{}
    {
    }

    template<class T, class E>
    template<class... Args, class>
    constexpr expected<T, E>::expected(unexpect_t, Args&&... args)
        : base_type{ unexpect, AZStd::forward<Args>(args)... }
    {
    }

    template<class T, class E>
    template<class U, class... Args, class>
    constexpr expected<T, E>::expected(unexpect_t, initializer_list<U> il, Args&&... args)
        : base_type{ unexpect, il, AZStd::forward<Args>(args)... }
    {
    }

    template<class T, class E>
    template<class U, class>
    constexpr auto expected<T, E>::operator=(U&& value) -> expected&
    {
        if (has_value())
        {
            // direct intialize from value
            this->m_storage.m_value = AZStd::forward<U>(value);
        }
        else
        {
            // destroy the error type and direct initialize the value type
            Internal::reinit_expected(this->m_storage.m_value, this->m_storage.m_unexpected, AZStd::forward<U>(value));
            this->m_hasValue = true;
        }

        return *this;
    }

    template<class T, class E>
    template<class G>
    constexpr auto expected<T, E>::operator=(const unexpected<G>& err) -> enable_if_t<is_constructible_v<E, const G&>
        && is_assignable_v<E&, const G&>,
        expected&>
    {
        if (!has_value())
        {
            // direct intialize from error
            this->m_storage.m_unexpected = AZStd::forward<const G&>(err.error());
        }
        else
        {
            // destroy the value type and copy the error type
            if constexpr (!is_void_v<T>)
            {
                Internal::reinit_expected(this->m_storage.m_unexpected, this->m_storage.m_value, AZStd::forward<const G&>(err.error()));
            }
            else
            {
                construct_at(addressof(this->m_storage.m_unexpected), AZStd::forward<const G&>(err.error()));
            }
            this->m_hasValue = false;
        }

        return *this;
    }

    template<class T, class E>
    template<class G>
    constexpr auto expected<T, E>::operator=(unexpected<G>&& err) ->enable_if_t < is_constructible_v<E, G>
        && is_assignable_v<E&, G>,
        expected&>
    {
        if (!has_value())
        {
            // direct intialize from error
            this->m_storage.m_unexpected = AZStd::forward<G>(err.error());
        }
        else
        {
            // destroy the value type and move the error type
            if constexpr (!is_void_v<T>)
            {
                Internal::reinit_expected(this->m_storage.m_unexpected, this->m_storage.m_value, AZStd::forward<G>(err.error()));
            }
            else
            {
                construct_at(addressof(this->m_storage.m_unexpected), AZStd::forward<G>(err.error()));
            }
            this->m_hasValue = false;
        }

        return *this;
    }

    //! expected emplace
    template<class T, class E>
    template<class... Args>
    constexpr decltype(auto) expected<T, E>::emplace(Args&&... args) noexcept
    {
        static_assert(!is_void_v<T> && is_constructible_v<T, Args...>);
        if (has_value())
        {
            AZStd::destroy_at(addressof(this->m_storage.m_value));
        }
        else
        {
            AZStd::destroy_at(addressof(this->m_storage.m_unexpected));
            this->m_hasValue = true;
        }

        return *construct_at(addressof(this->m_storage.m_value), AZStd::forward<Args>(args)...);
    }

    template<class T, class E>
    template<class U, class... Args>
    constexpr decltype(auto) expected<T, E>::emplace(initializer_list<U> il, Args&&... args) noexcept
    {
        static_assert(!is_void_v<T>&& is_constructible_v<T, initializer_list<U>&, Args...>);
        if (has_value())
        {
            AZStd::destroy_at(addressof(this->m_storage.m_value));
        }
        else
        {
            AZStd::destroy_at(addressof(this->m_storage.m_unexpected));
            this->m_hasValue = true;
        }

        return *construct_at(addressof(this->m_storage.m_value), il, AZStd::forward<Args>(args)...);
    }

    //! expected emplace for void type `expected<void, E>` specialization
    //! It destroys the error value and set has_value() to true
    template<class T, class E>
    template<bool, class>
    constexpr void expected<T, E>::emplace() noexcept
    {
        if (!has_value())
        {
            AZStd::destroy_at(addressof(this->m_storage.m_unexpected));
            this->m_hasValue = true;
        }
    }

    //! expected swap
    template<class T, class E>
    template<bool Enable>
    constexpr auto expected<T, E>::swap(expected& rhs) noexcept(is_nothrow_move_constructible_v<T>
        && is_nothrow_swappable_v<T>
        && is_nothrow_move_constructible_v<E>
        && is_nothrow_swappable_v<E>) -> enable_if_t<Enable>
    {
        if (has_value())
        {
            if (rhs.has_value())
            {
                // Both expected have a value so swap the value instance
                using AZStd::swap;
                swap(this->m_storage.m_value, rhs.m_storage.m_value);
            }
            else
            {
                // Swap the value type from this instance with the error type from the other
                if constexpr (is_nothrow_move_constructible_v<E>)
                {
                    E tmp(AZStd::move(rhs.m_storage.m_unexpected));
                    AZStd::destroy_at(addressof(rhs.m_storage.m_unexpected));
                    construct_at(addressof(rhs.m_storage.m_value), AZStd::move(this->m_storage.m_value));
                    AZStd::destroy_at(addressof(this->m_storage.m_value));
                    construct_at(addressof(this->m_storage.m_unexpected), AZStd::move(tmp));
                }
                else
                {
                    T tmp(AZStd::move(this->m_storage.m_value));
                    AZStd::destroy_at(addressof(this->m_storage.m_value));
                    construct_at(addressof(this->m_storage.m_unexpected), AZStd::move(rhs.m_storage.m_unexpected));
                    AZStd::destroy_at(addressof(rhs.m_storage.m_unexpected));
                    construct_at(addressof(rhs.m_storage.m_value), AZStd::move(tmp));
                }

                this->m_hasValue = false;
                rhs.m_hasValue = true;
            }
        }
        else
        {
            if (rhs.has_value())
            {
                // Call swap on the rhs to have the (*this->m_storage.has_value() && !rhs.has_value()) case above triggered
                rhs.swap(*this);
            }
            else
            {
                // Neither expected has a value so swap the unexpected instance
                using AZStd::swap;
                swap(this->m_storage.m_unexpected, rhs.m_storage.m_unexpected);
            }
        }
    }


    //! expected observer functions
    //! Returns whether the expected contains a value type(true) or error type(false)
    template<class T, class E>
    constexpr expected<T, E>::operator bool() const noexcept
    {
        return this->m_hasValue;
    }
    template<class T, class E>
    constexpr bool expected<T, E>::has_value() const noexcept
    {
        return this->m_hasValue;
    }

    template<class T, class E>
    template<bool Enable>
    constexpr auto expected<T, E>::operator->() const noexcept -> enable_if_t<Enable, const T*>
    {
        AZ_Assert(has_value(), "expected object doesn't have a value, a pointer to unitialized value type will be returned");
        return addressof(this->m_storage.m_value);
    }

    template<class T, class E>
    template<bool Enable>
    constexpr auto expected<T, E>::operator->() noexcept -> enable_if_t<Enable, T*>
    {
        AZ_Assert(has_value(), "expected object doesn't have a value, a pointer to unitialized value type will be returned");
        return addressof(this->m_storage.m_value);
    }

    //! expected value observers
    template<class T, class E>
    template<bool Enable>
    constexpr auto expected<T, E>::operator*() const & noexcept -> enable_if_t<Enable, add_lvalue_reference_t<const T>>
    {
        AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
        return this->m_storage.m_value;
    }

    template<class T, class E>
    template<bool Enable>
    constexpr auto expected<T, E>::operator*() & noexcept -> enable_if_t<Enable, add_lvalue_reference_t<T>>
    {
        AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
        return this->m_storage.m_value;
    }

    template<class T, class E>
    template<bool Enable>
    constexpr auto expected<T, E>::operator*() const && noexcept -> enable_if_t<Enable, add_rvalue_reference_t<const T>>
    {
        AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
        return AZStd::move(this->m_storage.m_value);
    }

    template<class T, class E>
    template<bool Enable>
    constexpr auto expected<T, E>::operator*() && noexcept -> enable_if_t<Enable, add_rvalue_reference_t<T>>
    {
        AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
        return AZStd::move(this->m_storage.m_value);
    }

    template<class T, class E>
    template<bool Enable>
    constexpr auto expected<T, E>::value() const & -> enable_if_t<Enable, add_lvalue_reference_t<const T>>
    {
        AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
        return this->m_storage.m_value;
    }

    template<class T, class E>
    template<bool Enable>
    constexpr auto expected<T, E>::value() & -> enable_if_t<Enable, add_lvalue_reference_t<T>>
    {
        AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
        return this->m_storage.m_value;
    }

    template<class T, class E>
    template<bool Enable>
    constexpr auto expected<T, E>::value() const && -> enable_if_t<Enable, add_rvalue_reference_t<const T>>
    {
        AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
        return AZStd::move(this->m_storage.m_value);
    }

    template<class T, class E>
    template<bool Enable>
    constexpr auto expected<T, E>::value() && -> enable_if_t<Enable, add_rvalue_reference_t<T>>
    {
        AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
        return AZStd::move(this->m_storage.m_value);
    }

    //! expected<void, E> specialization value observers
    template<class T, class E>
    template<bool Enable, enable_if_t<Enable>*>
    constexpr void expected<T, E>::operator*() const noexcept
    {
        AZ_Assert(has_value(), "expected doesn't have a value, void will be returned");
    }

    template<class T, class E>
    template<bool Enable, enable_if_t<Enable>*>
    constexpr void expected<T, E>::value() const &
    {
        AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
    }

    template<class T, class E>
    template<bool Enable, enable_if_t<Enable>*>
    constexpr void expected<T, E>::value() &&
    {
        AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
    }

    //! expected error observers
    template<class T, class E>
    constexpr const E& expected<T, E>::error() const & noexcept
    {
        AZ_Assert(!has_value(), "expected has have a value, an error type cannot be returned");
        return this->m_storage.m_unexpected;

    }

    template<class T, class E>
    constexpr E& expected<T, E>::error() & noexcept
    {
        AZ_Assert(!has_value(), "expected has have a value, an error type cannot be returned");
        return this->m_storage.m_unexpected;
    }

    template<class T, class E>
    constexpr const E&& expected<T, E>::error() const && noexcept
    {
        AZ_Assert(!has_value(), "expected has have a value, an error type cannot be returned");
        return AZStd::move(this->m_storage.m_unexpected);
    }

    template<class T, class E>
    constexpr E&& expected<T, E>::error() && noexcept
    {
        AZ_Assert(!has_value(), "expected has have a value, an error type cannot be returned");
        return AZStd::move(this->m_storage.m_unexpected);
    }

    template<class T, class E>
    template<class U>
    constexpr auto expected<T, E>::value_or(U&& value) const & -> enable_if_t<Internal::sfinae_trigger_v<U> && !is_void_v<T>, T>
    {
        static_assert(is_copy_constructible_v<T> && is_convertible_v<U, T>);
        return has_value() ? (**this) : static_cast<T>(AZStd::forward<U>(value));
    }

    template<class T, class E>
    template<class U>
    constexpr auto expected<T, E>::value_or(U&& value) && -> enable_if_t<Internal::sfinae_trigger_v<U> && !is_void_v<T>, T>
    {
        static_assert(is_move_constructible_v<T> && is_convertible_v<U, T>);
        return has_value() ? AZStd::move(**this) : static_cast<T>(AZStd::forward<U>(value));
    }

    //! expected operator== internal implementations
    template<class T1, class E1>
    template<class T2, class E2>
    constexpr bool expected<T1, E1>::compare_equal_internal(const expected<T2, E2>& y) const
    {
        if (has_value() != y.has_value())
        {
            return false;
        }

        if constexpr (is_void_v<T1>)
        {
            // Both arguments must be void in this case
            static_assert(is_void_v<T2>, "Both expected objects value_type must be void");
            return has_value() || error() == y.error();
        }
        else
        {
            static_assert(!is_void_v<T2>, "Neither expected objects value_type can be void");
            return has_value() ? *this == *y : error() == y.error();
        }
    }

    template<class T1, class E1>
    template<class T2>
    constexpr bool expected<T1, E1>::compare_equal_internal(const T2& value) const
    {
        return has_value() && bool(**this == value);
    }

    template<class T1, class E1>
    template<class E2>
    constexpr bool expected<T1, E1>::compare_equal_internal(const unexpected<E2>& err) const
    {
        return !has_value() && bool(error() == err.error());
    }
} // namespace AZStd
