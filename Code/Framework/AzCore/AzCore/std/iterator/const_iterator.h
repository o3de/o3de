/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/base.h>
#include <AzCore/std/concepts/concepts.h>
#include <AzCore/std/iterator/iterator_primitives.h>

namespace AZStd
{
    // Bring in std utility functions into AZStd namespace
    using std::forward;
    using std::addressof;

    //! basic_const_iterator is an iterator adapter that behaves the same as the underlying iterator type
    //! except its indirection operator converts the value returned by the underlying iterators'
    //! indirection operator to a type such that this iterator is a constant_iterator
    template<input_or_output_iterator I>
    class basic_const_iterator;
}

namespace AZStd::Internal
{
    template<class T>
    constexpr bool is_basic_const_iterator = false;
    template<class I>
    constexpr bool is_basic_const_iterator<basic_const_iterator<I>> = true;

    template<class It>
    concept constant_iterator =
        input_iterator<It>
        && same_as<iter_const_reference_t<It>, iter_reference_t<It>>;

    template<class I, class = void>
    struct basic_const_iterator_iter_category {};

    template<forward_iterator I>
    struct basic_const_iterator_iter_category<I>
    {
        using iterator_category = typename ITER_TRAITS<I>::iterator_category;
    };
}

namespace AZStd
{
    template<class I>
    using const_iterator = enable_if_t<input_iterator<I>, conditional_t<Internal::constant_iterator<I>, I, basic_const_iterator<I>>>;

    template<class S>
    using const_sentinel = conditional_t<input_iterator<S>, const_iterator<S>, S>;

    template<class It>
    concept not_a_const_iterator =
        (!Internal::is_basic_const_iterator<remove_cvref_t<It>>)
        || Internal::is_primary_template<remove_cvref_t<It>>;

    template<class S, class I>
    concept basic_const_iterator_sentinel_for =
        (!Internal::is_basic_const_iterator<remove_cvref_t<S>>)
        && semiregular<S>
        && input_or_output_iterator<I>
        && Internal::weakly_equality_comparable_with<I, S>;

    template<class S, class I>
    concept basic_const_iterator_sized_sentinel_for_impl =
        requires(const S& s, const I& i)
        {
            { i - s } -> same_as<iter_difference_t<I>>;
            { s - i } -> same_as<iter_difference_t<I>>;
        };

    template<class S, class I>
    concept basic_const_iterator_sized_sentinel_for =
        (!Internal::is_basic_const_iterator<remove_cvref_t<S>>)
        && basic_const_iterator_sentinel_for<S, I>
        && basic_const_iterator_sized_sentinel_for_impl<S, I>;

    template<class I, class I2>
    concept basic_const_iterator_ordered_with =
        (!Internal::is_basic_const_iterator<remove_cvref_t<I2>>)
        && random_access_iterator<I>
        && Internal::partially_ordered_with_impl<I, I2>;

    template<input_or_output_iterator I>
    class basic_const_iterator
        : public Internal::basic_const_iterator_iter_category<I>
    {
    public:
        using _is_primary_template = basic_const_iterator;

        using iterator_concept = conditional_t<contiguous_iterator<I>, contiguous_iterator_tag,
            conditional_t<random_access_iterator<I>, random_access_iterator_tag,
            conditional_t<bidirectional_iterator<I>, bidirectional_iterator_tag,
            conditional_t<forward_iterator<I>, forward_iterator_tag, input_iterator_tag>
            >
            >
        >;

        using value_type = iter_value_t<I>;
        using difference_type = iter_difference_t<I>;

        //! reference type and pointer type is need to use with algorithms that use pre c++20 iterator_traits
        using reference = iter_const_reference_t<I>;
        using pointer = const value_type*;

        constexpr basic_const_iterator()
            requires default_initializable<I>
        {}
        constexpr basic_const_iterator(I i)
            : m_current{ AZStd::move(i) }
        {
        }

        template<convertible_to<I> I2>
        constexpr basic_const_iterator(basic_const_iterator<I2> other)
            : m_current(AZStd::move(other.m_current))
        {
        }

        template<Internal::different_from<basic_const_iterator> T>
            requires convertible_to<T, I>
        constexpr basic_const_iterator(T&& current)
            : m_current(AZStd::forward<T>(current))
        {
        }

        constexpr const I& base() const& noexcept
        {
            return m_current;
        }
        constexpr I base() &&
        {
            return AZStd::move(m_current);
        }

        constexpr reference operator*() const
        {
            return static_cast<reference>(*m_current);
        }

        constexpr const value_type* operator->() const
            requires is_lvalue_reference_v<iter_reference_t<I>>
                && same_as<remove_cvref_t<iter_reference_t<I>>, value_type>
        {
            if constexpr (contiguous_iterator<I>)
            {
                return to_address(m_current);
            }
            else
            {
                return addressof(*m_current);
            }
        }

        constexpr reference operator[](difference_type n) const
            requires random_access_iterator<I>
        {
            return static_cast<reference>(m_current[n]);
        }

        // operations
        constexpr basic_const_iterator& operator++()
        {
            ++m_current;
            return *this;
        }
        constexpr decltype(auto) operator++(int)
        {
            if constexpr (forward_iterator<I>)
            {
                basic_const_iterator tmp = *this;
                ++*this;
                return tmp;
            }
            else
            {
                // return type is void in this case
                ++m_current;
            }
        }

        constexpr basic_const_iterator& operator--()
            requires bidirectional_iterator<I>
        {
            --m_current;
            return *this;
        }

        constexpr basic_const_iterator operator--(int)
            requires bidirectional_iterator<I>
        {
            basic_const_iterator tmp = *this;
            --* this;
            return tmp;
        }

        // operator +=, -=
        constexpr basic_const_iterator& operator+=(iter_difference_t<I> n)
            requires random_access_iterator<I>
        {
            m_current += n;
            return *this;
        }

        constexpr basic_const_iterator& operator-=(iter_difference_t<I> n)
            requires random_access_iterator<I>
        {
            m_current -= n;
            return *this;
        }


        // comparison operations

        // NOTE: these heterogeneous sentinel comparisons keep the enable_if_t return-type SFINAE form on purpose.
        // If we use a requires constraint, the associated-constraint is re-entered through `==` overload resolution inside basic_const_iterator_sentinel_for (which checks weakly_equality_comparable_with),
        // defeating clangs self-dependency detection and producing an infinite constraint-satisfaction recursion for nested iterator adapters such as move_iterator<basic_const_iterator>.
        // Evaluating the concept as a value in enable_if_t keeps the satisfaction on the stack where the self-dependency is detected and the candidate is correctly discarded.
        template<class S>
        friend constexpr auto operator==(const basic_const_iterator& i, const S& s)
            -> enable_if_t<basic_const_iterator_sentinel_for<S, I>, bool>
        {
            return i.base() == s;
        }

        template<class S>
        friend constexpr auto operator!=(const basic_const_iterator& i, const S& s)
            -> enable_if_t<basic_const_iterator_sentinel_for<S, I>, bool>
        {
            return !operator==(i, s);
        }

        // friend comparison functions
        template<class S>
        friend constexpr auto operator==(const S& s, const basic_const_iterator& i)
            -> enable_if_t<basic_const_iterator_sentinel_for<S, I> && Internal::different_from<S, basic_const_iterator>, bool>
        {
            return operator==(i, s);
        }

        template<class S>
        friend constexpr auto operator!=(const S& s, const basic_const_iterator& i)
            -> enable_if_t<basic_const_iterator_sentinel_for<S, I> && Internal::different_from<S, basic_const_iterator>, bool>
        {
            return !operator==(i, s);
        }

        template<class I2>
            requires Internal::weakly_equality_comparable_with<I, I2>
        friend constexpr bool operator==(const basic_const_iterator& x, const basic_const_iterator<I2>& y)
        {
            return x.base() == y.base();
        }

        template<class I2>
            requires Internal::weakly_equality_comparable_with<I, I2>
        friend constexpr bool operator!=(const basic_const_iterator& x, const basic_const_iterator<I2>& y)
        {
            return !(x == y);
        }

        friend constexpr bool operator<(const basic_const_iterator& x, const basic_const_iterator& y)
            requires random_access_iterator<I>
        {
            return x.base() < y.base();
        }
        friend constexpr bool operator>(const basic_const_iterator& x, const basic_const_iterator& y)
            requires random_access_iterator<I>
        {
            return x.base() > y.base();
        }
        friend constexpr bool operator<=(const basic_const_iterator& x, const basic_const_iterator& y)
            requires random_access_iterator<I>
        {
            return x.base() <= y.base();
        }
        friend constexpr bool operator>=(const basic_const_iterator& x, const basic_const_iterator& y)
            requires random_access_iterator<I>
        {
            return x.base() >= y.base();
        }

        template<class I2>
            requires basic_const_iterator_ordered_with<I, I2>
        friend constexpr bool operator<(const basic_const_iterator& x, const basic_const_iterator<I2>& y)
        {
            return x.base() < y.base();
        }
        template<class I2>
            requires basic_const_iterator_ordered_with<I, I2>
        friend constexpr bool operator>(const basic_const_iterator& x, const basic_const_iterator<I2>& y)
        {
            return x.base() > y.base();
        }
        template<class I2>
            requires basic_const_iterator_ordered_with<I, I2>
        friend constexpr bool operator<=(const basic_const_iterator& x, const basic_const_iterator<I2>& y)
        {
            return x.base() <= y.base();
        }
        template<class I2>
            requires basic_const_iterator_ordered_with<I, I2>
        friend constexpr bool operator>=(const basic_const_iterator& x, const basic_const_iterator<I2>& y)
        {
            return x.base() >= y.base();
        }

        // comparison against iterators that are not the exact type of this class
        template<class I2>
            requires (!Internal::is_basic_const_iterator<remove_cvref_t<I2>>)
                && Internal::different_from<I2, basic_const_iterator>
                && basic_const_iterator_ordered_with<I, I2>
        friend constexpr bool operator<(const basic_const_iterator& x, const I2& y)
        {
            return x.base() < y;
        }
        template<class I2>
            requires (!Internal::is_basic_const_iterator<remove_cvref_t<I2>>)
                && Internal::different_from<I2, basic_const_iterator>
                && basic_const_iterator_ordered_with<I, I2>
        friend constexpr bool operator>(const basic_const_iterator& x, const I2& y)
        {
            return x.base() > y;
        }
        template<class I2>
            requires (!Internal::is_basic_const_iterator<remove_cvref_t<I2>>)
                && Internal::different_from<I2, basic_const_iterator>
                && basic_const_iterator_ordered_with<I, I2>
        friend constexpr bool operator<=(const basic_const_iterator& x, const I2& y)
        {
            return x.base() <= y;
        }
        template<class I2>
            requires (!Internal::is_basic_const_iterator<remove_cvref_t<I2>>)
                && Internal::different_from<I2, basic_const_iterator>
                && basic_const_iterator_ordered_with<I, I2>
        friend constexpr bool operator>=(const basic_const_iterator& x, const I2& y)
        {
            return x.base() >= y;
        }

        // compares a specialization of basic_const_iterator against this instance
        template<class I2>
            requires (!Internal::is_basic_const_iterator<remove_cvref_t<I2>>)
                && not_a_const_iterator<I2>
                && basic_const_iterator_ordered_with<I, I2>
        friend constexpr bool operator<(const I2&x, const basic_const_iterator& y)
        {
            return x < y.base();
        }
        template<class I2>
            requires (!Internal::is_basic_const_iterator<remove_cvref_t<I2>>)
                && not_a_const_iterator<I2>
                && basic_const_iterator_ordered_with<I, I2>
        friend constexpr bool operator>(const I2&x, const basic_const_iterator& y)
        {
            return x > y.base();
        }
        template<class I2>
            requires (!Internal::is_basic_const_iterator<remove_cvref_t<I2>>)
                && not_a_const_iterator<I2>
                && basic_const_iterator_ordered_with<I, I2>
        friend constexpr bool operator<=(const I2&x, const basic_const_iterator& y)
        {
            return x <= y.base();
        }
        template<class I2>
            requires (!Internal::is_basic_const_iterator<remove_cvref_t<I2>>)
                && not_a_const_iterator<I2>
                && basic_const_iterator_ordered_with<I, I2>
        friend constexpr bool operator>=(const I2&x, const basic_const_iterator& y)
        {
            return x >= y.base();
        }

        // friend arithmetic operators
        friend constexpr basic_const_iterator operator+(const basic_const_iterator& i, iter_difference_t<I> n)
            requires random_access_iterator<I>
        {
            return basic_const_iterator(i.base() + n);
        }
        friend constexpr basic_const_iterator operator+(iter_difference_t<I> n, const basic_const_iterator& i)
            requires random_access_iterator<I>
        {
            return i + n;
        }

        friend constexpr basic_const_iterator operator-(const basic_const_iterator& i, iter_difference_t<I> n)
            requires random_access_iterator<I>
        {
            return basic_const_iterator(i.base() - n);
        }

        template<sized_sentinel_for<I> I2>
        friend constexpr difference_type operator-(const basic_const_iterator& i, const basic_const_iterator<I2>& s)
        {
            return i.base() - s.base();
        }

        // friend navigation operators
        template<basic_const_iterator_sized_sentinel_for<I> S>
        friend constexpr difference_type operator-(const basic_const_iterator& i, const S& s)
        {
            return i.base() - s;
        }

        template<basic_const_iterator_sized_sentinel_for<I> S>
            requires Internal::different_from<S, basic_const_iterator>
        friend constexpr difference_type operator-(const S& s, const basic_const_iterator& i)
        {
            return s - i.base();
        }

    private:
        I m_current{};
    };

    template<class I>
    constexpr const_iterator<I> make_const_iterator(I it)
        requires input_iterator<I>
    {
        return it;
    }

    template<class S>
    constexpr const_sentinel<S> make_const_sentinel(S s)
    {
        return s;
    }
}

namespace std
{
    // As AZStd aliases std::common_type, the specializations need to be put in the std namespace
    template<class T, AZStd::common_with<T> U>
    struct common_type<AZStd::basic_const_iterator<T>, U>
    {
        using type = AZStd::basic_const_iterator<common_type_t<T, U>>;
    };
    template<class T, AZStd::common_with<T> U>
    struct common_type<U, AZStd::basic_const_iterator<T>>
    {
        using type = AZStd::basic_const_iterator<common_type_t<T, U>>;
    };
    template<class T, AZStd::common_with<T> U>
    struct common_type<AZStd::basic_const_iterator<T>, AZStd::basic_const_iterator<U>>
    {
        using type = AZStd::basic_const_iterator<common_type_t<T, U>>;
    };
}
