/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include "core/math/VectorX.h"
#include "core/math/MatrixX.h"
#include "core/math/Quaternion.h"

namespace SimuCore
{
    const AZ::Vector2 VEC2_ZERO = AZ::Vector2(0.f, 0.f);
    const AZ::Vector2 VEC2_ONE = AZ::Vector2(1.f, 1.f);
    const AZ::Vector2 VEC2_UNIT_X = AZ::Vector2(1.f, 0.f);
    const AZ::Vector2 VEC2_UNIT_Y = AZ::Vector2(0.f, 1.f);

    const AZ::Color COLOR_WHITE = {1.0f, 1.0f, 1.0f, 1.0f};
    const AZ::Color COLOR_RED = {1.0f, 0.0f, 0.0f, 1.0f};
    const AZ::Color COLOR_GREEN = {0.0f, 1.0f, 0.0f, 1.0f};
    const AZ::Color COLOR_BLUE = {0.0f, 0.0f, 1.0f, 1.0f};

    const Matrix3 MAT3_ZERO = Matrix3::CreateZero();
    const Matrix3 MAT3_IDENTITY = Matrix3::CreateIdentity();

    constexpr float ALMOST_ONE = 0.999f;
    constexpr float TAN_DEGREE_30 = 0.577f;
    constexpr uint32_t VERTEX_COUNT = 4;
    constexpr uint32_t VERTEX_SIZE = 2;
    constexpr uint32_t POWER_CUBE = 3;
    constexpr uint32_t FACE_DIMENSION = 3;
    constexpr float NOISE_COEFFICIENT = 0.0001f;
}
