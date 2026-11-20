/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/base.h>
#include <AzCore/std/iterator/iterator_primitives.h>

namespace AZStd
{
    // Bring in std utility functions into AZStd namespace
    using std::forward;
}

namespace AZStd::Internal
{
    template<class I, class = void>
    struct counted_iterator_value_type {};

    template<class I>
        requires indirectly_readable<I>
    struct counted_iterator_value_type<I>
    {
        using value_type = iter_value_t<I>;
    };

    template<class I, class = void>
    struct counted_iterator_iter_concept {};

    template<class I>
        requires requires { typename ITER_TRAITS<I>::iterator_concept; }
    struct counted_iterator_iter_concept<I>
    {
        using iterator_concept = typename ITER_TRAITS<I>::iterator_concept;
    };

    template<class I, class = void>
    struct counted_iterator_iter_category {};

    template<class I>
        requires requires { typename ITER_TRAITS<I>::iterator_category; }
    struct counted_iterator_iter_category<I>
    {
        using iterator_category = typename ITER_TRAITS<I>::iterator_category;
    };

    struct counted_iterator_requirements_fulfilled {};
}

namespace AZStd
{
    //! counted_iterator is an iterator adapter that behaves the same as the underlying iterator
    //! except its keeps track of the distance to the end of the range

    template<input_or_output_iterator I>
    class counted_iterator
        : public Internal::counted_iterator_value_type<I>
        , public Internal::counted_iterator_iter_concept<I>
        , public Internal::counted_iterator_iter_category<I>
        , private Internal::counted_iterator_requirements_fulfilled
    {
    public:
        using iterator_type = I;
        using difference_type = iter_difference_t<I>;

        //! reference type and pointer type is need to use with algorithms that use pre c++20 iterator_traits
        using reference = iter_reference_t<I>;
        using pointer = conditional_t<contiguous_iterator<I>, decltype(to_address(declval<I>())), void>;

        constexpr counted_iterator()
            requires default_initializable<I>
        {}
        constexpr counted_iterator(I i, iter_difference_t<I> length)
            : m_current{ AZStd::move(i) }
            , m_length{ length }
        {
        }

        template<class I2>
            requires convertible_to<const I2&, I>
        constexpr counted_iterator(const counted_iterator<I2>& other)
            : m_current(other.m_current)
            , m_length(other.m_length)
        {
        }

        template<class I2>
            requires assignable_from<I&, const I2&>
        constexpr counted_iterator& operator=(const counted_iterator<I2>& other)
        {
            m_current = other.m_current;
            m_length = other.m_length;
            return *this;
        }

        constexpr const I& base() const& noexcept
        {
            return m_current;
        }
        constexpr I base() &&
        {
            return AZStd::move(m_current);
        }

        constexpr iter_difference_t<I> count() const noexcept
        {
            return m_length;
        }

        constexpr decltype(auto) operator*()
        {
            AZ_Assert(m_length > 0, "Attempting to dereference a counted_iterator with a negative count");
            return *m_current;
        }
        constexpr decltype(auto) operator*() const
            requires Internal::dereferenceable<const I>
        {
            AZ_Assert(m_length > 0, "Attempting to dereference a counted_iterator with a negative count");
            return *m_current;
        }

        constexpr auto operator->() const noexcept
            requires contiguous_iterator<I>
        {
            return to_address(m_current);
        }

        constexpr decltype(auto) operator[](iter_difference_t<I> n) const noexcept
            requires random_access_iterator<I>
        {
            AZ_Assert(n < m_length, "index %lld is out of range count %lld", static_cast<AZ::s64>(n), static_cast<AZ::s64>(m_length));
            return m_current[n];
        }

        // navigation operators
        constexpr counted_iterator& operator++()
        {
            AZ_Assert(m_length > 0, "counted_iterator count is 0, it can no longer be incremented");
            ++m_current;
            --m_length;
            return *this;
        }
        constexpr decltype(auto) operator++(int)
        {
            AZ_Assert(m_length > 0, "counted_iterator count is 0, it can no longer be incremented");
            if constexpr (forward_iterator<I>)
            {
                counted_iterator tmp = *this;
                ++*this;
                return tmp;
            }
            else
            {
                --m_length;
                return m_current++;
            }
        }

        constexpr counted_iterator& operator--()
            requires bidirectional_iterator<I>
        {
            --m_current;
            ++m_length;
            return *this;
        }

        constexpr counted_iterator operator--(int)
            requires bidirectional_iterator<I>
        {
            counted_iterator tmp = *this;
            --*this;
            return tmp;
        }

        // operator+, +=
        constexpr counted_iterator operator+(iter_difference_t<I> n)
            requires random_access_iterator<I>
        {
            return counted_iterator(m_current + n, m_length - n);
        }

        constexpr counted_iterator& operator+=(iter_difference_t<I> n)
            requires random_access_iterator<I>
        {
            AZ_Assert(n <= m_length, "the input distance cannot advance the counted_iterator pass the end of the count");
            m_current += n;
            m_length -= n;
            return *this;
        }

        friend constexpr counted_iterator operator+(iter_difference_t<I> n, const counted_iterator& x)
            requires random_access_iterator<I>
        {
            return x + n;
        }

        // operator-, -=
        constexpr counted_iterator operator-(iter_difference_t<I> n)
            requires random_access_iterator<I>
        {
            return counted_iterator(m_current - n, m_length + n);
        }

        constexpr counted_iterator& operator-=(iter_difference_t<I> n)
            requires random_access_iterator<I>
        {
            AZ_Assert(-n <= m_length, "the input distance cannot advance the counted_iterator pass the end of the count");
            m_current -= n;
            m_length += n;
            return *this;
        }

        // friend navigation operators
        template<class I2>
            requires common_with<I2, I>
        friend constexpr iter_difference_t<I2> operator-(const counted_iterator& x, const counted_iterator<I2>& y)
        {
            return y.count() - x.count();
        }

        friend constexpr iter_difference_t<I> operator-(const counted_iterator& x, default_sentinel_t)
        {
            return -x.count();
        }

        friend constexpr iter_difference_t<I> operator-(default_sentinel_t, const counted_iterator& y)
        {
            return y.count();
        }

        // comparison operations
        template<class I2>
            requires common_with<I2, I>
        friend constexpr bool operator==(const counted_iterator& x, const counted_iterator<I2>& y)
        {
            return x.count() == y.count();
        }
        template<class I2>
            requires common_with<I2, I>
        friend constexpr bool operator!=(const counted_iterator& x, const counted_iterator<I2>& y)
        {
            return !operator==(x, y);
        }

        friend constexpr bool operator==(const counted_iterator& x, default_sentinel_t)
        {
            return x.count() == 0;
        }
        friend constexpr bool operator!=(const counted_iterator& x, default_sentinel_t)
        {
            return !operator==(x, default_sentinel);
        }
        friend constexpr bool operator==(default_sentinel_t, const counted_iterator& x)
        {
            return operator==(x, default_sentinel);
        }
        friend constexpr bool operator!=(default_sentinel_t, const counted_iterator& x)
        {
            return !operator==(x, default_sentinel);
        }

        template<class I2>
            requires common_with<I2, I>
        friend constexpr bool operator<(const counted_iterator& x, const counted_iterator<I2>& y)
        {
            // Comparison is reversed, due to length being decremented as the counted_iterator advances
            // therefore a counted_iterator referring to the same sequence is further
            // along if it has a lesser count value
            return y.count() < x.count();
        }
        template<class I2>
            requires common_with<I2, I>
        friend constexpr bool operator>(const counted_iterator& x, const counted_iterator<I2>& y)
        {
            return operator<(y, x);
        }
        template<class I2>
            requires common_with<I2, I>
        friend constexpr bool operator<=(const counted_iterator& x, const counted_iterator<I2>& y)
        {
            return !operator<(y, x);
        }
        template<class I2>
            requires common_with<I2, I>
        friend constexpr bool operator>=(const counted_iterator& x, const counted_iterator<I2>& y)
        {
            return !operator<(x, y);
        }

        friend constexpr iter_rvalue_reference_t<I> iter_move(const counted_iterator& i)
            noexcept(noexcept(ranges::iter_move(declval<const I&>())))
            requires input_iterator<I>
        {
            AZ_Assert(i.count() > 0, "count value must be greater than 0 in order for iter_move to safely dereference the iterator");
            return ranges::iter_move(i.base());
        }

        template<class I2>
            requires indirectly_swappable<I2, I>
        friend constexpr void iter_swap(const counted_iterator& x, const counted_iterator<I2>& y)
            noexcept(noexcept(ranges::iter_swap(declval<const I&>(), declval<const I2&>())))
        {
            AZ_Assert(x.count() > 0 && y.count(),
                "both iterators count must be greater than 0 in order for iter_swap to safely dereference the iterators");
            return ranges::iter_swap(x.base(), y.base());
        }

    private:
        I m_current{};
        iter_difference_t<I> m_length{};
    };
}

namespace AZStd::Internal
{
    template<class I>
    concept counted_iterator_trait_requirements =
        input_iterator<I> &&
        same_as<Internal::ITER_TRAITS<I>, iterator_traits<I>> &&
        requires { typename I::_is_primary_template; };
}

namespace AZStd
{
    template<class I>
        requires Internal::counted_iterator_trait_requirements<I>
    struct iterator_traits<counted_iterator<I>>
        : iterator_traits<I>
    {
        using pointer = conditional_t<contiguous_iterator<I>, add_pointer_t<iter_reference_t<I>>, void>;

    };
}
