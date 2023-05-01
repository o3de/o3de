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
    template<class I>
    class basic_const_iterator;
}

namespace AZStd::Internal
{
    template<class It, class = void>
    /*concept*/ constexpr bool constant_iterator = false;
    template<class It>
    /*concept*/ constexpr bool constant_iterator<It, enable_if_t<input_iterator<It> &&
        same_as<iter_const_reference_t<It>, iter_reference_t<It>> >> = true;

    template<class I, class = void>
    struct basic_const_iterator_iter_category {};

    template<class I>
    struct basic_const_iterator_iter_category<I, enable_if_t<forward_iterator<I>>>
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

    // Not a const concept is false if the iterator is a specialization of the basic_const_iterator template
    template<class It>
    /*concept*/ constexpr bool not_a_const_iterator = true;
    template<class It>
    /*concept*/ constexpr bool not_a_const_iterator<basic_const_iterator<It>> = Internal::is_primary_template_v<basic_const_iterator<It>>;

    template<class I>
    class basic_const_iterator
        : public enable_if_t<input_or_output_iterator<I>, Internal::basic_const_iterator_iter_category<I>>
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

        template<bool Enable = default_initializable<I>, class = enable_if<Enable>>
        constexpr basic_const_iterator() {}
        constexpr basic_const_iterator(I i)
            : m_current{ AZStd::move(i) }
        {
        }

        template<class I2, class = enable_if_t<convertible_to<I2, I>>>
        constexpr basic_const_iterator(basic_const_iterator<I2> other)
            : m_current(AZStd::move(other.m_current))
        {
        }

        template<class T, class = enable_if_t<Internal::different_from<T, basic_const_iterator> && convertible_to<T, I>>>
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

        template<bool Enable = is_lvalue_reference_v<iter_reference_t<I>> &&
            same_as<remove_cvref_t<iter_reference_t<I>>, value_type>, class = enable_if_t<Enable>>
        constexpr const value_type* operator->() const
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

        template<bool Enable = random_access_iterator<I>, class = enable_if_t<Enable>>
        constexpr reference operator[](difference_type n) const
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

        template<bool Enable = bidirectional_iterator<I>, class = enable_if_t<Enable>>
        constexpr basic_const_iterator& operator--()
        {
            --m_current;
            return *this;
        }

        template<bool Enable = bidirectional_iterator<I>, class = enable_if_t<Enable>>
        constexpr basic_const_iterator operator--(int)
        {
            basic_const_iterator tmp = *this;
            --* this;
            return tmp;
        }

        // operator +=, -=
        template<bool Enable = random_access_iterator<I>, class = enable_if_t<Enable>>
        constexpr basic_const_iterator& operator+=(iter_difference_t<I> n)
        {
            m_current += n;
            return *this;
        }

        template<bool Enable = random_access_iterator<I>, class = enable_if_t<Enable>>
        constexpr basic_const_iterator& operator-=(iter_difference_t<I> n)
        {
            m_current -= n;
            return *this;
        }


        // comparison operations
        template<class S>
        friend constexpr auto operator==(const basic_const_iterator& i, const S& s)
            -> enable_if_t<sentinel_for<S, I>, bool>
        {
            return i.base() == s;
        }

        template<class S>
        friend constexpr auto operator!=(const basic_const_iterator& i, const S& s)
            -> enable_if_t<sentinel_for<S, I>, bool>
        {
            return !operator==(i, s);
        }

        // friend comparison functions
        template<class S>
        friend constexpr auto operator==(const S& s, const basic_const_iterator& i)
            -> enable_if_t<sentinel_for<S, I> && Internal::different_from<S, basic_const_iterator>, bool>
        {
            return operator==(i, s);
        }

        template<class S>
        friend constexpr auto operator!=(const S& s, const basic_const_iterator& i)
            -> enable_if_t<sentinel_for<S, I> && Internal::different_from<S, basic_const_iterator>, bool>
        {
            return !operator==(i, s);
        }

        template<bool Enable = random_access_iterator<I>, class = enable_if_t<Enable>>
        friend constexpr bool operator<(const basic_const_iterator& x, const basic_const_iterator& y)
        {
            return x.base() < y.base();
        }
        template<bool Enable = random_access_iterator<I>, class = enable_if_t<Enable>>
        friend constexpr bool operator>(const basic_const_iterator& x, const basic_const_iterator& y)
        {
            return x.base() > y.base();
        }
        template<bool Enable = random_access_iterator<I>, class = enable_if_t<Enable>>
        friend constexpr bool operator<=(const basic_const_iterator& x, const basic_const_iterator& y)
        {
            return x.base() <= y.base();
        }
        template<bool Enable = random_access_iterator<I>, class = enable_if_t<Enable>>
        friend constexpr bool operator>=(const basic_const_iterator& x, const basic_const_iterator& y)
        {
            return x.base() >= y.base();
        }

        // comparison against iterators that are not the exact type of this class
        template<class I2>
        friend constexpr auto operator<(const basic_const_iterator& x, const I2& y)
            -> enable_if_t<Internal::different_from<I2, basic_const_iterator> && random_access_iterator<I> && totally_ordered_with<I, I2>,
            bool>
        {
            return x.base() < y;
        }
        template<class I2>
        friend constexpr auto operator>(const basic_const_iterator& x, const I2& y)
            -> enable_if_t<Internal::different_from<I2, basic_const_iterator> && random_access_iterator<I>&& totally_ordered_with<I, I2>,
            bool>
        {
            return x.base() > y;
        }
        template<class I2>
        friend constexpr auto operator<=(const basic_const_iterator& x, const I2& y)
            -> enable_if_t<Internal::different_from<I2, basic_const_iterator> && random_access_iterator<I>&& totally_ordered_with<I, I2>,
            bool>
        {
            return x.base() <= y;
        }
        template<class I2>
        friend constexpr auto operator>=(const basic_const_iterator& x, const I2& y)
            -> enable_if_t<Internal::different_from<I2, basic_const_iterator> && random_access_iterator<I>&& totally_ordered_with<I, I2>,
            bool>
        {
            return x.base() >= y;
        }

        // compares a specialization of basic_const_iterator against this instance
        template<class I2>
        friend constexpr auto operator<(const I2&x, const basic_const_iterator& y)
            -> enable_if_t<not_a_const_iterator<I2> && random_access_iterator<I>&& totally_ordered_with<I, I2>,
            bool>
        {
            return x < y.base();
        }
        template<class I2>
        friend constexpr auto operator>(const I2&x, const basic_const_iterator& y)
            -> enable_if_t<not_a_const_iterator<I2> && random_access_iterator<I>&& totally_ordered_with<I, I2>,
            bool>
        {
            return x > y.base();
        }
        template<class I2>
        friend constexpr auto operator<=(const I2&x, const basic_const_iterator& y)
            -> enable_if_t<not_a_const_iterator<I2> && random_access_iterator<I>&& totally_ordered_with<I, I2>,
            bool>
        {
            return x <= y.base();
        }
        template<class I2>
        friend constexpr auto operator>=(const I2&x, const basic_const_iterator& y)
            -> enable_if_t<not_a_const_iterator<I2> && random_access_iterator<I>&& totally_ordered_with<I, I2>,
            bool>
        {
            return x >= y.base();
        }

        // friend arithmetic operators
        template<bool Enable = random_access_iterator<I>, class = enable_if_t<Enable>>
        friend constexpr basic_const_iterator operator+(const basic_const_iterator& i, iter_difference_t<I> n)
        {
            return basic_const_iterator(i.base() + n);
        }
        template<bool Enable = random_access_iterator<I>, class = enable_if_t<Enable>>
        friend constexpr basic_const_iterator operator+(iter_difference_t<I> n, const basic_const_iterator& i)
        {
            return i + n;
        }

        template<bool Enable = random_access_iterator<I>, class = enable_if_t<Enable>>
        friend constexpr basic_const_iterator operator-(const basic_const_iterator& i, iter_difference_t<I> n)
        {
            return basic_const_iterator(i.base() - n);
        }

        // friend navigation operators
        template<class S>
        friend constexpr auto operator-(const basic_const_iterator& i, const S& s)
            -> enable_if_t<sized_sentinel_for<S, I>, difference_type>
        {
            return i.base() - s;
        }

        template<class S>
        friend constexpr auto operator-(const S& s, const basic_const_iterator& i)
            -> enable_if_t<sized_sentinel_for<S, I> && Internal::different_from<S, basic_const_iterator>, difference_type>
        {
            return s - i.base();
        }

    private:
        I m_current{};
    };

    template<class I>
    constexpr auto make_const_iterator(I it)
        -> enable_if_t<input_iterator<I>, const_iterator<I>>
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
    template<class T, class U>
    struct common_type<AZStd::basic_const_iterator<T>, U>
    {
        using type = AZStd::enable_if_t<AZStd::common_with<U, T>, AZStd::basic_const_iterator<common_type_t<T, U>>>;
    };
    template<class T, class U>
    struct common_type<U, AZStd::basic_const_iterator<T>>
    {
        using type = AZStd::enable_if_t<AZStd::common_with<U, T>, AZStd::basic_const_iterator<common_type_t<T, U>>>;
    };
    template<class T, class U>
    struct common_type<AZStd::basic_const_iterator<T>, AZStd::basic_const_iterator<U>>
    {
        using type = AZStd::enable_if_t<AZStd::common_with<U, T>, AZStd::basic_const_iterator<common_type_t<T, U>>>;
    };
}
