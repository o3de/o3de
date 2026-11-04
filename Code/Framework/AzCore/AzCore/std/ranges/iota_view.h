/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/iterator/unreachable_sentinel.h>
#include <AzCore/std/ranges/iota_internal.h>
#include <AzCore/std/ranges/ranges_adaptor.h>

namespace AZStd::ranges
{
    //! Generates a sequence of elements by repeating incrementing an initial element W
    //! up to Bound
    template<weakly_incrementable W, semiregular Bound = unreachable_sentinel_t>
        requires ::AZStd::Internal::weakly_equality_comparable_with<W, Bound>
            && copyable<W>
    class iota_view;

    template<class W, class Bound>
    inline constexpr bool enable_borrowed_range<iota_view<W, Bound>> = true;
}

// views::iota customization point
namespace AZStd::ranges::views
{
    namespace Internal
    {
        struct iota_fn
            : Internal::range_adaptor_closure<iota_fn>
        {
            template<class W, class Bound>
            constexpr auto operator()(W&& value, Bound&& bound) const
            {
                return iota_view(AZStd::forward<W>(value), AZStd::forward<Bound>(bound));
            }

            template<class W>
            constexpr auto operator()(W&& value) const
            {
                return iota_view(AZStd::forward<W>(value));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::iota_fn iota{};
    }
}

namespace AZStd::ranges::Internal
{
    template<class I>
    concept decrementable =
        incrementable<I>
        && same_as<decltype(--declval<I&>()), I&>
        && same_as<decltype(declval<I&>()--), I>;

    template<class I>
    concept advanceable =
        decrementable<I>
        && totally_ordered<I>
        && same_as<decltype(declval<I&>() += declval<const IOTA_DIFF_T<I>>()), I&>
        && same_as<decltype(declval<I&>() -= declval<const IOTA_DIFF_T<I>>()), I&>
        && requires
        {
            I(declval<const I>() + declval<const IOTA_DIFF_T<I>>());
            I(declval<const IOTA_DIFF_T<I>>() + declval<const I>());
            I(declval<const I>() - declval<const IOTA_DIFF_T<I>>());
        }
        && convertible_to<decltype(declval<const I&>() - declval<const I&>()), IOTA_DIFF_T<I>>;
}

namespace AZStd::ranges
{
    template<weakly_incrementable W, semiregular Bound>
        requires ::AZStd::Internal::weakly_equality_comparable_with<W, Bound>
            && copyable<W>
    class iota_view
        : public view_interface<iota_view<W, Bound>>
    {
        struct iterator;
        struct sentinel;

    public:
        constexpr iota_view()
            requires default_initializable<W>
        {}

        explicit constexpr iota_view(W value)
            : m_value(AZStd::move(value))
        {
        }

        constexpr iota_view(type_identity_t<W> value, type_identity_t<Bound> bound)
            : m_value(value)
            , m_bound(bound)
        {
            if constexpr (totally_ordered_with<W, Bound>)
            {
                AZ_Assert(value <= bound, "W and Bound are totally ordered with and value is greater than bound");
            }
        }

        //! iterator and sentinel are not complete types at this point,
        //! so these construtors are defined after those types are defined
        constexpr iota_view(iterator first, iterator last)
            requires same_as<W, Bound>;

        constexpr iota_view(iterator first, Bound last)
            requires same_as<Bound, unreachable_sentinel_t>;

        constexpr iota_view(iterator first, sentinel last)
            requires (!same_as<W, Bound>)
                && (!same_as<Bound, unreachable_sentinel_t>);

        constexpr auto begin() const
        {
            return iterator(m_value);
        }

        constexpr auto end() const
        {
            if constexpr (same_as<W, Bound>)
            {
                return iterator{ m_bound };
            }
            else if constexpr (!same_as<Bound, unreachable_sentinel_t>)
            {
                return sentinel{ m_bound };
            }
            else
            {
                return unreachable_sentinel;
            }
        }

        constexpr auto size() const
            requires same_as<W, Bound>
                && Internal::advanceable<W>
                || (::AZStd::Internal::is_integer_like<W> && ::AZStd::Internal::is_integer_like<Bound>)
                || sized_sentinel_for<Bound, W>
        {
            if constexpr (::AZStd::Internal::is_integer_like<W> && ::AZStd::Internal::is_integer_like<Bound>)
            {
                return (m_value < 0) ? ((m_bound < 0)
                    ? Internal::to_unsigned_like(-m_value) - Internal::to_unsigned_like(-m_bound)
                    : Internal::to_unsigned_like(m_bound) + Internal::to_unsigned_like(m_value))
                    : Internal::to_unsigned_like(m_bound) - Internal::to_unsigned_like(m_value);
            }
            else
            {
                return Internal::to_unsigned_like(m_bound - m_value);
            }
        }

    private:
        W m_value{};
        Bound m_bound{};
    };

    // deduction guides
    template<class W, class Bound>
        requires (!::AZStd::Internal::is_integer_like<W>)
            || (!::AZStd::Internal::is_integer_like<Bound>)
            || (::AZStd::Internal::is_signed_integer_like<W> == ::AZStd::Internal::is_signed_integer_like<Bound>)
    iota_view(W, Bound) -> iota_view<W, Bound>;
}

namespace AZStd::ranges::Internal
{
    // Used as base type for iota_view iterator and sentinel
    struct iota_view_requirements_fulfilled {};
}

namespace AZStd::ranges
{
    // iota iterator
    template<weakly_incrementable W, semiregular Bound>
        requires ::AZStd::Internal::weakly_equality_comparable_with<W, Bound>
            && copyable<W>
    struct iota_view<W, Bound>::iterator
    {
    private:
        friend class iota_view;
        friend struct sentinel;

    public:

        using iterator_concept = conditional_t<Internal::advanceable<W>, random_access_iterator_tag,
            conditional_t<Internal::decrementable<W>, bidirectional_iterator_tag,
            conditional_t<incrementable<W>, forward_iterator_tag, input_iterator_tag>
            >>;

        using iterator_category = input_iterator_tag;

        using value_type = W;
        using difference_type = Internal::IOTA_DIFF_T<W>;

        // libstdc++ std::reverse_iterator use pre C++ concept when the concept feature is off
        // which requires that the iterator type has the aliases
        // of difference_type, value_type, pointer, reference, iterator_category,
        // With C++20, the iterator concept support is used to deduce the traits
        // needed, therefore alleviating the need to special std::iterator_traits
        // The following code allows std::reverse_iterator(which is aliased into AZStd namespace)
        // to work with the AZStd range views
#if !__cpp_lib_concepts
        using pointer = void;
        using reference = W;
#endif

        constexpr iterator()
            requires default_initializable<W>
        {}

        constexpr explicit iterator(W value)
            : m_value(value)
        {
        }

        constexpr W operator*() const noexcept(is_nothrow_copy_constructible_v<W>)
        {
            return m_value;
        }

        constexpr iterator& operator++()
        {
            ++m_value;
            return *this;
        }

        constexpr auto operator++(int)
        {
            if constexpr (incrementable<W>)
            {
                auto tmp = *this;
                ++m_value;
                return tmp;
            }
            else
            {
                ++m_value;
            }
        }

        constexpr iterator& operator--()
            requires Internal::decrementable<W>
        {
            --m_value;
            return *this;
        }

        constexpr iterator operator--(int)
            requires Internal::decrementable<W>
        {
            auto tmp = *this;
            --m_value;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type n)
            requires Internal::advanceable<W>
        {
            if constexpr (AZStd::Internal::is_integer_like<W>)
            {
                if constexpr (AZStd::Internal::is_signed_integer_like<W>)
                {
                    m_value = static_cast<W>(m_value + n);
                }
                else
                {
                    // If m_value is unsigned and n is signed, adding n to m_value would lead to a compile warning due to signed/unsigned
                    // mismatch. Converting n to unsigned, however, leads to wrong results for negative values of n, so two separate
                    // branches are required.
                    if (n >= difference_type(0))
                    {
                        m_value += static_cast<W>(n);
                    }
                    else
                    {
                        m_value -= static_cast<W>(-n);
                    }
                }
            }
            else
            {
                m_value += n;
            }
            return *this;
        }

        constexpr iterator& operator-=(difference_type n)
            requires Internal::advanceable<W>
        {
            if constexpr (AZStd::Internal::is_integer_like<W>)
            {
                if constexpr (AZStd::Internal::is_signed_integer_like<W>)
                {
                    m_value = static_cast<W>(m_value - n);
                }
                else
                {
                    // If m_value is unsigned and n is signed, subtracting n from m_value would lead to a compile warning due to
                    // signed/unsigned mismatch. Converting n to unsigned, however, leads to wrong results for negative values of n, so two
                    // separate branches are required.
                    if (n >= difference_type{ 0 })
                    {
                        m_value -= static_cast<W>(n);
                    }
                    else
                    {
                        m_value += static_cast<W>(-n);
                    }
                }
            }
            else
            {
                m_value -= n;
            }
            return *this;
        }

        constexpr W operator[](difference_type n) const
            requires Internal::advanceable<W>
        {
            if constexpr (AZStd::Internal::is_integer_like<W>)
            {
                return static_cast<W>(m_value + static_cast<W>(n));
            }
            else
            {
                return static_cast<W>(m_value + n);
            }
        }

        friend constexpr bool operator==(const iterator& x, const iterator& y)
            requires equality_comparable<W>
        {
            return x.m_value == y.m_value;
        }
        friend constexpr bool operator!=(const iterator& x, const iterator& y)
            requires equality_comparable<W>
        {
            return !operator==(x, y);
        }

        friend constexpr bool operator<(const iterator& x, const iterator& y)
            requires totally_ordered<W>
        {
            return x.m_value < y.m_value;
        }
        friend constexpr bool operator>(const iterator& x, const iterator& y)
            requires totally_ordered<W>
        {
            return operator<(y, x);
        }
        friend constexpr bool operator<=(const iterator& x, const iterator& y)
            requires totally_ordered<W>
        {
            return !operator<(y, x);
        }
        friend constexpr bool operator>=(const iterator& x, const iterator& y)
            requires totally_ordered<W>
        {
            return !operator<(x, y);
        }

        friend constexpr iterator operator+(iterator i, difference_type n)
            requires Internal::advanceable<W>
        {
            i += n;
            return i;
        }
        friend constexpr iterator operator+(difference_type n, iterator i)
            requires Internal::advanceable<W>
        {
            i += n;
            return i;
        }

        friend constexpr iterator operator-(iterator i, difference_type n)
            requires Internal::advanceable<W>
        {
            i -= n;
            return i;
        }

        friend constexpr difference_type operator-(const iterator& x, const iterator& y)
            requires Internal::advanceable<W>
        {
            if constexpr (::AZStd::Internal::is_integer_like<W>)
            {
                if constexpr (::AZStd::Internal::is_signed_integer_like<W>)
                {
                    return static_cast<difference_type>(difference_type(x.m_value) - difference_type(y.m_value));
                }
                else
                {
                    return y.m_value > x._value ? static_cast<difference_type>(-difference_type(y.m_value - x.m_value))
                        : static_cast<difference_type>(x.m_value - y.m_value);
                }
            }
            else
            {
                return x.m_value - y.m_value;
            }
        }

    private:
        //! current value
        W m_value{};
    };

    // sentinel type for iota
    template<weakly_incrementable W, semiregular Bound>
        requires ::AZStd::Internal::weakly_equality_comparable_with<W, Bound>
            && copyable<W>
    struct iota_view<W, Bound>::sentinel
    {
    private:
        friend class iota_view;

    public:
        sentinel() = default;
        constexpr explicit sentinel(Bound bound)
            : m_boundSentinel(bound)
        {}

        // comparison operators
        friend constexpr bool operator==(const iterator& x, const sentinel& y)
        {
            return iterator_accessor(x) == y.m_end;
        }
        friend constexpr bool operator==(const sentinel& y, const iterator& x)
        {
            return operator==(x, y);
        }
        friend constexpr bool operator!=(const iterator& x, const sentinel& y)
        {
            return !operator==(x, y);
        }
        friend constexpr bool operator!=(const sentinel& y, const iterator& x)
        {
            return !operator==(x, y);
        }

        friend constexpr iter_difference_t<W> operator-(const iterator& x, const sentinel& y)
            requires sized_sentinel_for<Bound, W>
        {
            return iterator_accessor(x) - y.m_bound;
        }

        friend constexpr iter_difference_t<W> operator-(const sentinel& x, const iterator& y)
            requires sized_sentinel_for<Bound, W>
        {
            return -(y - x);
        }

    private:
        // On MSVC The friend functions are can only access the sentinel struct members
        // The iterator struct which is a friend of the sentinel struct is NOT a friend
        // of the friend functions
        // So a shim is added to provide access to the iterator m_current member
        static constexpr auto iterator_accessor(const iterator& i)
        {
            return i.m_value;
        }

        Bound m_boundSentinel{};
    };

    //! iota_view iterator and sentinel constructor definitions
    template<weakly_incrementable W, semiregular Bound>
        requires ::AZStd::Internal::weakly_equality_comparable_with<W, Bound>
            && copyable<W>
    constexpr iota_view<W, Bound>::iota_view(iterator first, iterator last)
        requires same_as<W, Bound>
        : m_value(AZStd::move(first.m_value))
        , m_bound(AZStd::move(last.m_value))
    {}

    template<weakly_incrementable W, semiregular Bound>
        requires ::AZStd::Internal::weakly_equality_comparable_with<W, Bound>
            && copyable<W>
    constexpr iota_view<W, Bound>::iota_view(iterator first, Bound last)
        requires same_as<Bound, unreachable_sentinel_t>
        : m_value(AZStd::move(first.m_value))
        , m_bound(unreachable_sentinel)
    {}

    template<weakly_incrementable W, semiregular Bound>
        requires ::AZStd::Internal::weakly_equality_comparable_with<W, Bound>
            && copyable<W>
    constexpr iota_view<W, Bound>::iota_view(iterator first, sentinel last)
        requires (!same_as<W, Bound>)
            && (!same_as<Bound, unreachable_sentinel_t>)
        : m_value(AZStd::move(first.m_value))
        , m_bound(AZStd::move(last.m_boundSentinel))
    {}
}
