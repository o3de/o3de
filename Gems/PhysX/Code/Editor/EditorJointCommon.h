/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/utils.h>

namespace PhysX
{
    //! Pair of floating point values for angular limits.
    using AngleLimitsFloatPair = AZStd::pair<float, float>;

    //! Pair of floating point values for linear limits.
    using LinearLimitsFloatPair = AZStd::pair<float, float>;
} // namespace PhysX
