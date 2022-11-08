/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/span.h>
#include <AzCore/std/ranges/subrange.h>
#include <AzCore/std/iterator/counted_iterator.h>

namespace AZStd::ranges::views
{
    namespace Internal
    {
        template<class T, class CountType, class = void>
        constexpr bool count_convertible_to_iter_difference_v = false;
        template<class T, class CountType>
        constexpr bool count_convertible_to_iter_difference_v<T, CountType,
            enable_if_t<convertible_to<CountType, iter_difference_t<decay_t<T>>> >> = true;

        struct counted_fn
            : Internal::range_adaptor_closure<counted_fn>
        {
            template<class Iterator, class DifferenceType, class = enable_if_t<
                count_convertible_to_iter_difference_v<Iterator, DifferenceType> >>
            constexpr decltype(auto) operator()(Iterator&& i, DifferenceType&& count) const
            {
                using DecayIterator = decay_t<Iterator>;
                using IteratorDiffType = iter_difference_t<DecayIterator>;
                if constexpr (contiguous_iterator<DecayIterator>)
                {
                    return span(to_address(i), static_cast<size_t>(static_cast<IteratorDiffType>(count)));
                }
                else if constexpr (random_access_iterator<DecayIterator>)
                {
                    return subrange(AZStd::forward<Iterator>(i),
                        AZStd::forward<Iterator>(i) + static_cast<IteratorDiffType>(AZStd::forward<DifferenceType>(count)));
                }
                else
                {
                    return subrange<counted_iterator<Iterator>, default_sentinel_t>(counted_iterator<Iterator>(AZStd::forward<Iterator>(i), AZStd::forward<DifferenceType>(count)),
                        default_sentinel);
                }
            }
        };
    }

    inline namespace customization_point_object
    {
        constexpr Internal::counted_fn counted{};
    }
}
