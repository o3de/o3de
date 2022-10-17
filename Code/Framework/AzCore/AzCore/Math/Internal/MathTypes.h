/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <float.h>
#include <math.h>
#include <AzCore/Math/Internal/MathTypes_Platform.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/SimdMath.h>

// Tag type used to allow default initialization to be performed for math types
// https://en.cppreference.com/w/cpp/language/default_initialization
// Default initialize will initialize objects with automatic storage initialization to indeterminate reviews
namespace AZ::Math
{
    struct default_initialize_t
    {
    };
    inline constexpr default_initialize_t default_initialize{};
} // namespace AZ::Math
