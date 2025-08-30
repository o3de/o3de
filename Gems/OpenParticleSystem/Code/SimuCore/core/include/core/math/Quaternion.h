/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cmath>
#include "core/platform/Platform.h"
#include "core/math/VectorX.h"
#include "core/math/SimdType.h"
#include "core/math/MatrixX.h"
#include "core/math/Math.h"

namespace SimuCore {
    class Quaternion {
    public:
        using MemType = AZ::Quaternion;
        MemType value;

        inline Quaternion();

        inline Quaternion(float qx, float qy, float qz, float qw);

        inline Quaternion(const Vector3& axis, float angle);

        inline Quaternion(const Quaternion& q);

        inline Quaternion(const MemType& q) : value(q) {};

        inline Quaternion& operator=(const Quaternion& q);

        inline Quaternion& operator*=(const Quaternion& q);

        inline Quaternion& operator+=(const Quaternion& q);

        inline Quaternion operator*(const float scale) const;

        inline Quaternion operator+(const Quaternion& q) const;

        inline Quaternion operator-(const Quaternion& q) const;

        inline Quaternion operator-() const;

        inline ~Quaternion() = default;

        inline bool operator==(const Quaternion& q) const;

        inline bool operator!=(const Quaternion& q) const;

        inline Quaternion& Normalize();

        inline void FromMatrix3(const Matrix3& m);

        inline Matrix3 ToMatrix3() const;

        inline Matrix4 ToMatrix() const;

        [[nodiscard]] inline Vector3 operator*(const Vector3& v) const;
        [[nodiscard]] inline Quaternion GetConjugate() const;

        inline Quaternion Inverse() const;

        inline void Slerp(const Quaternion& right, float factor);

        inline Vector3 RotateVector3(const Vector3& v) const;

        static inline Vector3 RotateVector3(const Quaternion& q, const Vector3& v);
    };

    inline Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs)
    {
        return lhs.value * rhs.value;
    }

    inline Quaternion Normalize(const Quaternion& lhs)
    {
        return lhs.value.GetNormalized();
    }

    inline Quaternion EulerToQuaternion(float yaw, float pitch, float roll) // z, y, x
    {
        float cy = cos(yaw   / 2.f);
        float sy = sin(yaw   / 2.f);
        float cp = cos(pitch / 2.f);
        float sp = sin(pitch / 2.f);
        float cr = cos(roll  / 2.f);
        float sr = sin(roll  / 2.f);

        return Quaternion(sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy);
    }

    inline Vector3 QuaternionToEulerAngles(const Quaternion& quat)
    {
        Vector3 eulerAngle;

        {
            double sc = 2 * (quat.value.GetW() * quat.value.GetX() + quat.value.GetY() * quat.value.GetZ());
            double cc = 1 - 2 * (quat.value.GetX() * quat.value.GetX() + quat.value.GetY() * quat.value.GetY());
            eulerAngle.SetElement(0, static_cast<float>(std::atan2(sc, cc)));
        }

        double sin = 2 * (quat.value.GetW() * quat.value.GetY() - quat.value.GetZ() * quat.value.GetX());
        if (std::abs(sin) >= 1) {
            eulerAngle.SetElement(1, static_cast<float>(std::copysign(Math::PI / 2.f, sin)));
        } else {
            eulerAngle.SetElement(1, static_cast<float>(std::asin(sin)));
        }

        {
            double sc = 2 * (quat.value.GetW() * quat.value.GetZ() + quat.value.GetX() * quat.value.GetY());
            double cc = 1 - 2 * (quat.value.GetY() * quat.value.GetY() + quat.value.GetZ() * quat.value.GetZ());
            eulerAngle.SetElement(2, static_cast<float>(std::atan2(sc, cc)));
        }
        return eulerAngle;
    }
}

#include "core/math/Quaternion.inl"
