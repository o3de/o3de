/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "core/math/Quaternion.h"
#include "core/math/MatrixX.h"
#include "core/math/Constants.h"

#include <AzCore/Math/Transform.h>

namespace SimuCore {
    //! Limits for transform scale values.
    //! The scale should not be zero to avoid problems with inverting.
    constexpr float MIN_TRANSFORM_SCALE = 1e-2f;
    constexpr float MAX_TRANSFORM_SCALE = 1e9f;

    struct Transform {
        AZ::Transform value;

        Transform() : Transform(VEC3_ZERO, QUAT_IDENTITY, 1.0f) {}
        inline Transform(const Vector3& trans, const Quaternion& rot, float s);
        inline Transform(const AZ::Transform& transform) : value(transform) {}

        inline Transform& operator*=(const Transform& rhs);

        inline Transform operator*(const Transform& rhs) const;

        inline bool operator==(const Transform& rhs) const;

        inline bool operator!=(const Transform& rhs) const;

        //inline void FromMatrix(const Matrix4& trans);

        inline Matrix4 ToMatrix() const;

        [[nodiscard]] inline Transform Inverse() const;
        [[nodiscard]] inline Vector3 TransformPoint(const Vector3& rhs) const;
        [[nodiscard]] inline Vector3 TransformVector(const Vector3& rhs) const;

        [[nodiscard]] inline Vector3 GetTranslation() const;
        [[nodiscard]] inline Quaternion GetRotation() const;
        [[nodiscard]] inline float GetUniformScale() const;
        inline void LookAt(const Vector3& start, const Vector3& target, const AZ::Transform::Axis axisUp);
    };
}

#include "core/math/Transform.inl"
