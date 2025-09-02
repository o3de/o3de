/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "core/platform/Platform.h"
#include "core/math/Math.h"

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace SimuCore {
    enum VectorEnum {
        VEC_X = 0,
        VEC_Y,
        VEC_Z,
        VEC_W
    };

    using Vector3 = AZ::Vector3;
    using Vector4 = AZ::Vector4;
    using Vector2 = AZ::Vector2;
}
