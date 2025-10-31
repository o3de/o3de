/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <tuple>

// Add aliases of the std::tuple class and utility functions into the AZStd namespace
// The forwarding file is used to prevent a circular include between std/tuple.h, std/utils.h and std/ranges/subrange.h
namespace AZStd
{
    using std::tuple;
    using std::tuple_size;
    using std::tuple_size_v;
    using std::tuple_element;
    using std::tuple_element_t;

    // Placeholder structure that can be assigned any value with no effect.
    using std::ignore;

    using std::make_tuple;
    using std::tie;
    using std::forward_as_tuple;
    using std::tuple_cat;
    using std::get;
}
