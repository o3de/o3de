/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/utils.h>

namespace AZStd
{
    // ref: https://en.cppreference.com/w/cpp/algorithm/accumulate
    template<typename InputIt, typename T>
    constexpr T accumulate(InputIt first, InputIt last, T init)
    {
        for (; first != last; ++first)
        {
            init = AZStd::move(init) + *first;
        }

        return init;
    }

    // ref: https://en.cppreference.com/w/cpp/algorithm/accumulate
    template<typename InputIt, typename T, typename BinaryOperation>
    constexpr T accumulate(InputIt first, InputIt last, T init, BinaryOperation op)
    {
        for (; first != last; ++first)
        {
            init = op(AZStd::move(init), *first);
        }

        return init;
    }

    // ref: https://en.cppreference.com/w/cpp/algorithm/inner_product
    template<class InputIt1, class InputIt2, class T>
    constexpr T inner_product(InputIt1 first1, InputIt1 last1, InputIt2 first2, T init)
    {
        while (first1 != last1)
        {
            init = AZStd::move(init) + *first1 * *first2;
            ++first1;
            ++first2;
        }

        return init;
    }

    // ref: https://en.cppreference.com/w/cpp/algorithm/inner_product
    template<class InputIt1, class InputIt2, class T, class BinaryOperation1, class BinaryOperation2>
    constexpr T inner_product(InputIt1 first1, InputIt1 last1, InputIt2 first2, T init, BinaryOperation1 op1, BinaryOperation2 op2)
    {
        while (first1 != last1)
        {
            init = op1(AZStd::move(init), op2(*first1, *first2));
            ++first1;
            ++first2;
        }

        return init;
    }
} // namespace AZStd
