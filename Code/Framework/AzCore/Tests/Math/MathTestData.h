/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

namespace MathTestData
{
    static const AZ::Vector3 Vector3s[] = {
        AZ::Vector3::CreateZero(),
        AZ::Vector3::CreateOne(),
        AZ::Vector3(-2.5f, -1.8f, 9.3f),
        AZ::Vector3(1000.0f, 1e-3f, 0.0f)
    };

    static const float Angles[] = { 0.0f, 1.0f, AZ::Constants::Pi, 100.0f, -0.5f };

    static const AZ::Matrix3x3 Matrix3x3s[] = {
        AZ::Matrix3x3::CreateIdentity(),
        AZ::Matrix3x3::CreateRotationZ(0.3f),
        AZ::Matrix3x3::CreateFromQuaternion(AZ::Quaternion(-0.46f, 0.26f, -0.22f, 0.82f)),
        AZ::Matrix3x3::CreateScale(AZ::Vector3(0.7f, 1.3f, 0.9f))
    };

    using AxisPair = AZStd::pair<AZ::Constants::Axis, AZ::Vector3>;
    static const AxisPair Axes[] = {
        { AZ::Constants::Axis::XPositive, AZ::Vector3::CreateAxisX(1.0f) },
        { AZ::Constants::Axis::XNegative, AZ::Vector3::CreateAxisX(-1.0f) },
        { AZ::Constants::Axis::YPositive, AZ::Vector3::CreateAxisY(1.0f) },
        { AZ::Constants::Axis::YNegative, AZ::Vector3::CreateAxisY(-1.0f) },
        { AZ::Constants::Axis::ZPositive, AZ::Vector3::CreateAxisZ(1.0f) },
        { AZ::Constants::Axis::ZNegative, AZ::Vector3::CreateAxisZ(-1.0f) }
    };

    static const AZ::Matrix3x4 NonOrthogonalMatrix3x4s[] = {
        AZ::Matrix3x4::CreateScale(AZ::Vector3(2.4f, 0.3f, 1.7f)),
        AZ::Matrix3x4::CreateRotationX(2.2f) * AZ::Matrix3x4::CreateDiagonal(AZ::Vector3(0.2f, 0.8f, 1.4f))
    };

    static const AZ::Matrix3x4 OrthogonalMatrix3x4s[] = {
        AZ::Matrix3x4::CreateIdentity(),
        AZ::Matrix3x4::CreateRotationX(-0.6f),
        AZ::Matrix3x4::CreateFromQuaternion(AZ::Quaternion(0.24f, -0.08f, -0.48f, 0.84f)),
        AZ::Matrix3x4::CreateTranslation(AZ::Vector3(7.9f, 2.4f, -4.6f)),
        AZ::Matrix3x4::CreateFromQuaternionAndTranslation(AZ::Quaternion(0.12f, -0.24f, -0.72f, 0.64f), AZ::Vector3(2.3f, -5.2f, 0.7f))
    };

    static const AZ::Transform NonOrthogonalTransforms[] = {
    AZ::Transform::CreateScale(AZ::Vector3(2.4f, 0.3f, 1.7f)),
    AZ::Transform::CreateRotationX(2.2f) * AZ::Transform::CreateScale(AZ::Vector3(0.2f, 0.8f, 1.4f))
    };

    static const AZ::Transform OrthogonalTransforms[] = {
        AZ::Transform::CreateIdentity(),
        AZ::Transform::CreateRotationX(-0.6f),
        AZ::Transform::CreateFromQuaternion(AZ::Quaternion(0.24f, -0.08f, -0.48f, 0.84f)),
        AZ::Transform::CreateTranslation(AZ::Vector3(7.9f, 2.4f, -4.6f)),
        AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion(0.12f, -0.24f, -0.72f, 0.64f), AZ::Vector3(2.3f, -5.2f, 0.7f))
    };

    static const AZ::Vector3 EulerAnglesDegrees[] = {
        AZ::Vector3::CreateZero(),
        AZ::Vector3(70.0f, -32.0f, 119.0f),
        AZ::Vector3(1284.0f, -2734.0f, -1929.0f)
    };

    static const AZ::Vector3 EulerAnglesRadians[] = {
        AZ::Vector3::CreateZero(),
        AZ::Vector3(0.8f, -0.4f, 1.7f),
        AZ::Vector3(10.2f, 9.7f, -6.8f)
    };

    static const AZ::Quaternion UnitQuaternions[] = {
        AZ::Quaternion::CreateIdentity(),
        AZ::Quaternion(0.58f, -0.22f, -0.26f, 0.74f),
        AZ::Quaternion::CreateRotationX(0.2f)
    };
} // namespace MathTestData
