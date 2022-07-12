/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <type_traits>

namespace AZStd
{
    using std::is_constructible;
    using std::is_constructible_v;
    using std::is_trivially_constructible;
    using std::is_trivially_constructible_v;
    using std::is_nothrow_constructible;
    using std::is_nothrow_constructible_v;

    using std::is_default_constructible;
    using std::is_default_constructible_v;
    using std::is_trivially_default_constructible;
    using std::is_trivially_default_constructible_v;
    using std::is_nothrow_default_constructible;
    using std::is_nothrow_default_constructible_v;

    using std::is_copy_constructible;
    using std::is_copy_constructible_v;
    using std::is_trivially_copy_constructible;
    using std::is_trivially_copy_constructible_v;
    using std::is_nothrow_copy_constructible;
    using std::is_nothrow_copy_constructible_v;

    using std::is_move_constructible;
    using std::is_move_constructible_v;
    using std::is_trivially_move_constructible;
    using std::is_trivially_move_constructible_v;
    using std::is_nothrow_move_constructible;
    using std::is_nothrow_move_constructible_v;
}
