/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace SimuCore {
    class SimplexNoise {
    public:
        static AZ::Matrix4x4 SampleSimplexNoise(const AZ::Vector3& input); // out 4 * Vector3
        static AZ::Matrix4x4 JacobianSimplexNoise(const AZ::Vector3& input); // out 3 * Vector4
        static AZ::Vector3 RandomPCG16(const AZ::Vector3& vec);
        static AZ::Vector4 SimplexSmooth(const AZ::Matrix4x4& offsetToCell); // in 4 * Vector3
        static AZ::Matrix4x4 SimplexDSmooth(const AZ::Matrix4x4& offsetToCell); // out 3 * Vector4, in 4 * Vector3
    };
} // namespace SimuCore
