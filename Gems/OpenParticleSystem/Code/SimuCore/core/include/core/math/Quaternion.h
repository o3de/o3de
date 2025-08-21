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
        using MemType = VEC4_TYPE;
        union {
            struct {
                float x;
                float y;
                float z;
                float w;
            };
            MemType value;
        };

        inline Quaternion();

        inline Quaternion(float qx, float qy, float qz, float qw);

        inline Quaternion(const Vector3& axis, float angle);

        inline Quaternion(const Quaternion& q);

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
        return Quaternion(lhs) *= rhs;
    }

    inline Quaternion Normalize(const Quaternion& lhs)
    {
        Quaternion q(lhs);
        (void)q.Normalize();
        return q;
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
            double sc = 2 * (quat.w * quat.x + quat.y * quat.z);
            double cc = 1 - 2 * (quat.x * quat.x + quat.y * quat.y);
            eulerAngle.x = static_cast<float>(std::atan2(sc, cc));
        }

        double sin = 2 * (quat.w * quat.y - quat.z * quat.x);
        if (std::abs(sin) >= 1) {
            eulerAngle.y = static_cast<float>(std::copysign(Math::PI / 2.f, sin));
        } else {
            eulerAngle.y = static_cast<float>(std::asin(sin));
        }

        {
            double sc = 2 * (quat.w * quat.z + quat.x * quat.y);
            double cc = 1 - 2 * (quat.y * quat.y + quat.z * quat.z);
            eulerAngle.z = static_cast<float>(std::atan2(sc, cc));
        }
        return eulerAngle;
    }
}

#include "core/math/Quaternion.inl"
