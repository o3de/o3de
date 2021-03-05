/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef CRYINCLUDE_CRYCOMMON_ALGORITHM_H
#define CRYINCLUDE_CRYCOMMON_ALGORITHM_H
#pragma once
//short hand for using stl algorithms. same syntax (from users perspective) of c++17 range library. Only the shorthand algorithms from range library(N4128) though.
//Not all algorithms are covered. Add any as you need them. It would be a fair amount of work to add them all, so I'm just adding them as needed.
//Note Android doesn't have non member cbegin and cend yet.

#include <algorithm>
#include <numeric>
#include <iterator>

namespace std17
{
    template<typename Container, typename Callable>
    void for_each(const Container& con, Callable callable)
    {
        std::for_each(begin(con), end(con), callable);
    }

    template<typename Container, typename UnaryPredicate>
    bool any_of(const Container& con, UnaryPredicate pred)
    {
        return std::any_of(begin(con), end(con), pred);
    }

    template<typename Container, typename UnaryPredicate>
    bool all_of(const Container& con, UnaryPredicate pred)
    {
        return std::all_of(begin(con), end(con), pred);
    }

    template<typename Container, typename UnaryPredicate>
    bool none_of(const Container& con, UnaryPredicate pred)
    {
        return std::none_of(begin(con), end(con), pred);
    }

    template<typename Container, typename UnaryPredicate>
    typename Container::iterator find_if(Container& con, UnaryPredicate pred)
    {
        return std::find_if(begin(con), end(con), pred);
    }

    template <typename Container, typename T>
    T accumulate(const Container& con, T init)
    {
        return std::accumulate(begin(con), end(con), init);
    }

    template <typename Container, typename T, class BinaryOperation>
    T accumulate(const Container& con, T init, BinaryOperation binary_op)
    {
        return std::accumulate(begin(con), end(con), init, binary_op);
    }

    template <typename Container, typename UnaryPredicate>
    auto count_if(const Container&con, UnaryPredicate pred)->decltype(std::count_if(begin(con), end(con), pred))
    {
        return std::count_if(begin(con), end(con), pred);
    }
}

#endif // CRYINCLUDE_CRYCOMMON_ALGORITHM_H