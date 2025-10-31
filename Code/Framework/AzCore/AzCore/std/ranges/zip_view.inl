/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/ranges/all_view.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/std/tuple.h>

namespace AZStd::ranges
{
    // public zip_view functions
    template<class... Views>
    constexpr zip_view<Views...>::zip_view(Views... views)
        : m_views{ AZStd::move(views)... }
    {
    }

    template<class... Views>
    template<bool, class>
    constexpr auto zip_view<Views...>::begin()
    {
        return iterator<false>(ZipViewInternal::tuple_transform(ranges::begin, m_views));
    }
    template<class... Views>
    template<bool, class>
    constexpr auto zip_view<Views...>::begin() const
    {
        return iterator<true>(ZipViewInternal::tuple_transform(ranges::begin, m_views));
    }

    template<class... Views>
    template<bool, class>
    constexpr auto zip_view<Views...>::end()
    {
        if constexpr (!ZipViewInternal::zip_is_common<Views...>)
        {
            return sentinel<false>(ZipViewInternal::tuple_transform(ranges::end, m_views));
        }
        else if constexpr ((random_access_range<Views> && ...))
        {
            return begin() + iter_difference_t<iterator<false>>(size());
        }
        else
        {
            return iterator<false>(ZipViewInternal::tuple_transform(ranges::end, m_views));
        }
    }
    template<class... Views>
    template<bool, class>
    constexpr auto zip_view<Views...>::end() const
    {
        if constexpr (!ZipViewInternal::zip_is_common<const Views...>)
        {
            return sentinel<true>(ZipViewInternal::tuple_transform(ranges::end, m_views));
        }
        else if constexpr ((random_access_range<const Views> && ...))
        {
            return begin() + iter_difference_t<iterator<true>>(size());
        }
        else
        {
            return iterator<true>(ZipViewInternal::tuple_transform(ranges::end, m_views));
        }
    }

    template<class... Views>
    template<bool, class>
    constexpr auto zip_view<Views...>::size()
    {
        auto GetSizeForViews = [](auto... sizes)
        {
            using CommonType = make_unsigned_t<common_type_t<decltype(sizes)...>>;
            return ranges::min({ CommonType(sizes)... });
        };
        return AZStd::apply(AZStd::move(GetSizeForViews), ZipViewInternal::tuple_transform(ranges::size, m_views));
    }
    template<class... Views>
    template<bool, class>
    constexpr auto zip_view<Views...>::size() const
    {
        auto GetSizeForViews = [](auto... sizes)
        {
            using CommonType = make_unsigned_t<common_type_t<decltype(sizes)...>>;
            return ranges::min({ CommonType(sizes)... });
        };
        return AZStd::apply(AZStd::move(GetSizeForViews), ZipViewInternal::tuple_transform(ranges::size, m_views));
    }


    // public zip_view::iterator functions
    template<class... Views>
    template<bool Const>
    template<bool, class>
    constexpr zip_view<Views...>::iterator<Const>::iterator(iterator<!Const> other)
        : m_current(AZStd::move(other.m_current))
    {
    }

    template<class... Views>
    template<bool Const>
    constexpr auto zip_view<Views...>::iterator<Const>::operator*() const
    {
        auto TransformToReference = [](auto& i) -> decltype(auto)
        {
            return *i;
        };
        return ZipViewInternal::tuple_transform(AZStd::move(TransformToReference), m_current);
    }
    template<class... Views>
    template<bool Const>
    constexpr auto zip_view<Views...>::iterator<Const>::operator++() -> iterator&
    {
        auto PreIncrementIterator = [](auto& i)
        {
            ++i;
        };
        ZipViewInternal::tuple_for_each(AZStd::move(PreIncrementIterator), m_current);
        return *this;
    }

    template<class... Views>
    template<bool Const>
    constexpr decltype(auto) zip_view<Views...>::iterator<Const>::operator++(int)
    {
        if constexpr (ZipViewInternal::all_forward<Const, Views...>)
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }
        else
        {
            ++(*this);
        }
    }

    template<class... Views>
    template<bool Const>
    template<bool, class>
    constexpr auto zip_view<Views...>::iterator<Const>::operator--() -> iterator&
    {
        auto PreDecrementIterator = [](auto& i)
        {
            --i;
        };
        ZipViewInternal::tuple_for_each(AZStd::move(PreDecrementIterator), m_current);
        return *this;
    }
    template<class... Views>
    template<bool Const>
    template<bool, class>
    constexpr auto zip_view<Views...>::iterator<Const>::operator--(int) -> iterator
    {
        auto tmp = *this;
        --* this;
        return tmp;
    }

    template<class... Views>
    template<bool Const>
    template<bool, class>
    constexpr auto zip_view<Views...>::iterator<Const>::operator+=(difference_type x) -> iterator&
    {
        auto AddIterator = [&](auto& i)
        {
            i += iter_difference_t<decltype(i)>(x);
        };
        ZipViewInternal::tuple_for_each(AZStd::move(AddIterator), m_current);
        return *this;
    }
    template<class... Views>
    template<bool Const>
    template<bool, class>
    constexpr auto zip_view<Views...>::iterator<Const>::operator-=(difference_type x) -> iterator&
    {
        auto AddIterator = [&](auto& i)
        {
            i -= iter_difference_t<decltype(i)>(x);
        };
        ZipViewInternal::tuple_for_each(AZStd::move(AddIterator), m_current);
        return *this;
    }

    template<class... Views>
    template<bool Const>
    template<bool, class>
    constexpr auto zip_view<Views...>::iterator<Const>::operator[](difference_type n) const
    {
        return view_iterator_to_value_tuple(n);
    }

    // private zip_view::iterator functions
    template<class... Views>
    template<bool Const>
    constexpr zip_view<Views...>::iterator<Const>::iterator(
        ZipViewInternal::tuple_or_pair<iterator_t<::AZStd::ranges::Internal::maybe_const<Const, Views>>...> current)
        : m_current(AZStd::move(current))
    {
    }

    template<class... Views>
    template<bool Const>
    constexpr auto zip_view<Views...>::iterator<Const>::view_iterator_to_value_tuple(difference_type n) const
    {
        auto TransformToValue = [&](auto& i) -> decltype(auto)
        {
            using I = decltype(i);
            return i[iter_difference_t<I>(n)];
        };
        return ZipViewInternal::tuple_transform(AZStd::move(TransformToValue), m_current);
    }

    template<class... Views>
    template<bool Const>
    template<size_t... Indices>
    constexpr auto zip_view<Views...>::iterator<Const>::any_iterator_equal(const iterator& x, const iterator& y,
        AZStd::index_sequence<Indices...>)
    {
        return (... || (AZStd::get<Indices>(x.m_current) == AZStd::get<Indices>(y.m_current)));
    }

    template<class... Views>
    template<bool Const>
    template<size_t... Indices>
    constexpr auto zip_view<Views...>::iterator<Const>::min_distance_in_views(const iterator& x, const iterator& y,
        AZStd::index_sequence<Indices...>)
    {
        AZStd::array iterDistances{
            ((AZStd::get<Indices>(x.m_current) - AZStd::get<Indices>(y.m_current)), ...) };
        if (iterDistances.empty())
        {
            return difference_type{};
        }

        auto first = iterDistances.begin();
        difference_type minDistance = *first++;
        auto last = iterDistances.end();
        for (; first != last; ++first)
        {
            difference_type absMinDistance = minDistance < 0 ? -minDistance : minDistance;
            difference_type absDistance = *first < 0 ? -*first : *first;
            minDistance = absDistance < absMinDistance ? *first : minDistance;
        }

        return minDistance;
    }

    // private zip_view::sentinel functions
    template<class... Views>
    template<bool Const>
    constexpr zip_view<Views...>::sentinel<Const>::sentinel(
        ZipViewInternal::tuple_or_pair<sentinel_t<::AZStd::ranges::Internal::maybe_const<Const, Views>>...> end)
        : m_end(end)
    {
    }

    template<class... Views>
    template<bool Const>
    template<bool, class>
    constexpr zip_view<Views...>::sentinel<Const>::sentinel(sentinel<!Const> other)
        : m_end(AZStd::move(other.m_end))
    {
    }

    template<class... Views>
    template<bool Const>
    template<bool OtherConst, size_t... Indices>
    constexpr auto zip_view<Views...>::sentinel<Const>::min_distance_between_view_iterators(
        const typename zip_view<Views...>::template iterator<OtherConst>& x,
        const typename zip_view<Views...>::template sentinel<Const>& y,
        AZStd::index_sequence<Indices...>) ->
        common_type_t<range_difference_t<::AZStd::ranges::Internal::maybe_const<OtherConst, Views>>...>
    {
        using difference_type = common_type_t<range_difference_t<::AZStd::ranges::Internal::maybe_const<OtherConst, Views>>...>;
        // Tracks if any iterator of the Views is equal to a sentinel of the views
        AZStd::array iterDistances{
            ((AZStd::get<Indices>(x.m_current) - AZStd::get<Indices>(y.m_end)), ...) };
        if (iterDistances.empty())
        {
            return difference_type{};
        }

        auto first = iterDistances.begin();
        difference_type minDistance = *first++;
        auto last = iterDistances.end();
        for (; first != last; ++first)
        {
            difference_type absMinDistance = minDistance < 0 ? -minDistance : minDistance;
            difference_type absDistance = *first < 0 ? -*first : *first;
            minDistance = absDistance < absMinDistance ? *first : minDistance;
        }

        return minDistance;
    }
    
    template<class... Views>
    template<bool Const>
    template<bool OtherConst>
    constexpr auto zip_view<Views...>::sentinel<Const>::iterator_accessor(const iterator<OtherConst>& it)
    {
        return it.m_current;
    }
} // namespace AZStd::ranges
