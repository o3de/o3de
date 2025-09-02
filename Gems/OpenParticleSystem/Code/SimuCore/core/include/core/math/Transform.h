/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "core/math/Quaternion.h"
#include "core/math/Constants.h"

#include <AzCore/Math/Transform.h>

namespace SimuCore {
    //! Limits for transform scale values.
    //! The scale should not be zero to avoid problems with inverting.
    constexpr float MIN_TRANSFORM_SCALE = 1e-2f;
    constexpr float MAX_TRANSFORM_SCALE = 1e9f;
    using Transform = AZ::Transform;
}

