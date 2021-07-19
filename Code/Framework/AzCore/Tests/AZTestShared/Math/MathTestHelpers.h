/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <ostream>

namespace AZ
{
    class Matrix3x3;
    class Matrix3x4;
    class Matrix4x4;
    class Quaternion;
    class Sphere;
    class Vector2;
    class Vector3;
    class Vector4;
    class Aabb;
    class Transform;
    class Color;

    std::ostream& operator<<(std::ostream& os, const Vector2& vec);
    std::ostream& operator<<(std::ostream& os, const Vector3& vec);
    std::ostream& operator<<(std::ostream& os, const Vector4& vec);
    std::ostream& operator<<(std::ostream& os, const Aabb& aabb);
    std::ostream& operator<<(std::ostream& os, const Quaternion& quat);
    std::ostream& operator<<(std::ostream& os, const Transform& transform);
    std::ostream& operator<<(std::ostream& os, const Color& transform);
    std::ostream& operator<<(std::ostream& os, const Sphere& sphere);
    std::ostream& operator<<(std::ostream& os, const Matrix3x3& mat);
    std::ostream& operator<<(std::ostream& os, const Matrix3x4& mat);
    std::ostream& operator<<(std::ostream& os, const Matrix4x4& mat);
} // namespace AZ

namespace UnitTest
{
    // is-close matcher to make tests easier to read and failures more useful
    // (more information is included in the output)
    MATCHER_P(IsClose, expected, "")
    {
        AZ_UNUSED(result_listener);

        if (arg.IsClose(expected))
        {
            return true;
        }

        return false;
    }

    // is-close matcher with tolerance to make tests easier to read and failures more useful
    // (more information is included in the output)
    MATCHER_P2(IsCloseTolerance, expected, tolerance, "")
    {
        AZ_UNUSED(result_listener);

        if (arg.IsClose(expected, tolerance))
        {
            return true;
        }

        return false;
    }

    // is-close matcher for use with Pointwise container comparisons
    MATCHER(ContainerIsClose, "")
    {
        AZ_UNUSED(result_listener);

        const auto& [expected, actual] = arg;
        if (expected.IsClose(actual))
        {
            return true;
        }

        return false;
    }

    // is-close matcher with tolerance for use with Pointwise container comparisons
    MATCHER_P(ContainerIsCloseTolerance, tolerance, "")
    {
        AZ_UNUSED(result_listener);

        const auto& [expected, actual] = arg;
        if (expected.IsClose(actual, tolerance))
        {
            return true;
        }

        return false;
    }

    // IsFinite matcher to make it easier to validate Vector2, Vector3, Vector4 and Quaternion.
    // For example:
    //     AZ::Quaternion rotation;
    //     EXPECT_THAT(rotation, IsFinite());
    //
    //     AZStd::vector<AZ::Vector3> positions;
    //     EXPECT_THAT(positions, ::testing::Each(IsFinite()));
    MATCHER(IsFinite, "")
    {
        AZ_UNUSED(result_listener);
        return arg.IsFinite();
    }
} // namespace UnitTest
