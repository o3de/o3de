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
    template<class W, class Bound = unreachable_sentinel_t, class = enable_if_t<conjunction_v<
        bool_constant<move_constructible<W>>,
        bool_constant<semiregular<Bound>>,
        is_object<W>,
        bool_constant<same_as<W, remove_cv_t<W>>>,
        disjunction<bool_constant<::AZStd::Internal::is_integer_like<Bound>>,
            bool_constant<same_as<Bound, unreachable_sentinel_t>>
            >
    > >>
    class repeat_view;
}

// views::repeat customization point
namespace AZStd::ranges::views
{
    namespace Internal
    {
        struct repeat_fn
            : Internal::range_adaptor_closure<repeat_fn>
        {
            template<class W, class Bound>
            constexpr auto operator()(W&& value, Bound&& bound) const
            {
                return repeat_view(AZStd::forward<W>(value), AZStd::forward<Bound>(bound));
            }

            template<class W>
            constexpr auto operator()(W&& value) const
            {
                return repeat_view(AZStd::forward<W>(value));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::repeat_fn repeat{};
    }
}

namespace AZStd::ranges
{
    template<class W, class Bound, class>
    class repeat_view
        : public view_interface<repeat_view<W, Bound>>
    {
        struct iterator;

    public:
        template <bool Enable = default_initializable<W>,
            class = enable_if_t<Enable>>
        constexpr repeat_view() {}

        template<bool Enable = copy_constructible<W>, class = enable_if_t<Enable>>
        constexpr explicit repeat_view(const W& value, Bound bound = {})
            : m_value(value)
            , m_bound(bound)
        {
        }

        constexpr explicit repeat_view(W&& value, Bound bound = {})
            : m_value(AZStd::move(value))
            , m_bound(bound)
        {
        }

        template<class... WArgs, class... BoundArgs,
            bool Enable = constructible_from<W, WArgs...>&& constructible_from<Bound, BoundArgs...>,
            class = enable_if_t<Enable>>
        constexpr explicit repeat_view(piecewise_construct_t, tuple<WArgs...> valueArgs, tuple<BoundArgs...> boundArgs = {})
            : repeat_view(piecewise_construct, AZStd::move(valueArgs), AZStd::move(boundArgs),
                make_index_sequence<sizeof...(WArgs)>{}, make_index_sequence<sizeof...(BoundArgs)>{})
        {}

        constexpr auto begin() const
        {
            return iterator(addressof(*m_value));
        }

        constexpr auto end() const noexcept
        {
            if constexpr (!same_as<Bound, unreachable_sentinel_t>)
            {
                return iterator{ addressof(*m_value), m_bound };
            }
            else
            {
                return unreachable_sentinel;
            }
        }

        template<bool Enable = !same_as<Bound, unreachable_sentinel_t>, class = enable_if_t<Enable>>
        constexpr auto size() const
        {
            return Internal::to_unsigned_like(m_bound);
        }

    private:

        // piecewise construct value and bound by forwarding arguments from each tuple
        template<class... WArgs, class... BoundArgs, size_t... WIndices, size_t... BoundIndices>
        constexpr repeat_view(piecewise_construct_t, tuple<WArgs...> valueArgs, tuple<BoundArgs...> boundArgs,
            AZStd::index_sequence<WIndices...>, AZStd::index_sequence<BoundIndices...>)
            : m_value{ in_place, AZStd::forward<WArgs>(AZStd::get<BoundArgs>(valueArgs))... }
            , m_bound(AZStd::forward<BoundArgs>(AZStd::get<BoundIndices>(boundArgs))...)
        {}

        Internal::movable_box<W> m_value;
        Bound m_bound{};
    };

    // deduction guides
    template<class W, class Bound>
    repeat_view(W, Bound) -> repeat_view<W, Bound>;
}

namespace AZStd::ranges::Internal
{
    struct repeat_view_requirements_fulfilled {};
}

namespace AZStd::ranges
{
    // repeat iterator
    template<class W, class Bound, class ViewEnable>
    struct repeat_view<W, Bound, ViewEnable>::iterator
        : enable_if_t<conjunction_v<
        bool_constant<move_constructible<W>>,
        bool_constant<semiregular<Bound>>,
        is_object<W>,
        bool_constant<same_as<W, remove_cv_t<W>>>,
        disjunction<bool_constant<::AZStd::Internal::is_integer_like<Bound>>,
            bool_constant<same_as<Bound, unreachable_sentinel_t>>
            >
        >, Internal::repeat_view_requirements_fulfilled>
    {
    private:
        friend class repeat_view;

        using index_type = conditional_t<same_as<Bound, unreachable_sentinel_t>, ptrdiff_t, Bound>;

    public:

        using iterator_concept = random_access_iterator_tag;
        using iterator_category = random_access_iterator_tag;

        using value_type = W;
        using difference_type = conditional_t<::AZStd::Internal::is_signed_integer_like<index_type>,
            index_type, Internal::IOTA_DIFF_T<W>>;

        // libstdc++ std::reverse_iterator use pre C++ concept when the concept feature is off
        // which requires that the iterator type has the aliases
        // of difference_type, value_type, pointer, reference, iterator_category,
        // With C++20, the iterator concept support is used to deduce the traits
        // needed, therefore alleviating the need to special std::iterator_traits
        // The following code allows std::reverse_iterator(which is aliased into AZStd namespace)
        // to work with the AZStd range views
#if !__cpp_lib_concepts
        using pointer = void;
        using reference = const W&;
#endif

        template<bool Enable = default_initializable<W>,
            class = enable_if_t<Enable>>
        constexpr iterator() {}

        constexpr explicit iterator(const W* value, index_type b = {})
            : m_value(value)
            , m_current{ b }
        {
        }

        constexpr const W& operator*() const noexcept
        {
            return *m_value;
        }

        constexpr iterator& operator++()
        {
            ++m_current;
            return *this;
        }

        constexpr iterator& operator++(int)
        {
            auto tmp = *this;
            ++m_current;
            return tmp;
        }

        constexpr iterator& operator--()
        {
            --m_current;
            return *this;
        }

        constexpr iterator operator--(int)
        {
            auto tmp = *this;
            --m_current;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type n)
        {
            m_current += n;
            return *this;
        }

        constexpr iterator& operator-=(difference_type n)
        {
            m_current -= n;
            return *this;
        }

        constexpr const W& operator[](difference_type n) const
        {
            return *(*this + n);
        }

        friend constexpr bool operator==(const iterator& x, const iterator& y)
        {
            return x.m_current == y.m_current;
        }
        friend constexpr bool operator!=(const iterator& x, const iterator& y)
        {
            return !operator==(x, y);
        }

        friend constexpr bool operator<(const iterator& x, const iterator& y)
        {
            return x.m_current < y.m_current;
        }
        friend constexpr bool operator>(const iterator& x, const iterator& y)
        {
            return operator<(y, x);
        }
        friend constexpr bool operator<=(const iterator& x, const iterator& y)
        {
            return !operator<(y, x);
        }
        friend constexpr bool operator>=(const iterator& x, const iterator& y)
        {
            return !operator<(x, y);
        }

        friend constexpr iterator operator+(iterator i, difference_type n)
        {
            i += n;
            return i;
        }
        friend constexpr iterator operator+(difference_type n, iterator i)
        {
            i += n;
            return i;
        }

        friend constexpr iterator operator-(iterator i, difference_type n)
        {
            i -= n;
            return i;
        }

        friend constexpr difference_type operator-(const iterator& x, const iterator& y)
        {
            return static_cast<difference_type>(x.m_current) - static_cast<difference_type>(y.m_current);
        }

    private:
        const W* m_value{};
        index_type m_current{};
    };
}
