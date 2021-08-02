/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional_basic.h>
#include <queue>

namespace AZStd
{
    template<class T, class Container = AZStd::deque<T>>
    using queue = std::queue<T, Container>;
    template<class T, class Container = AZStd::vector<T>, class Compare = AZStd::less<typename Container::value_type>>
    using priority_queue = std::priority_queue<T, Container, Compare>;
}
