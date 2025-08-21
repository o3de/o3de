/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "core/math/VectorX.h"
#include "core/math/Math.h"
#include <array>

namespace SimuCore {
    struct Matrix3;

    struct Matrix4 {
        std::array<Vector4, 4> m;

        inline Matrix4() = default;
        inline Matrix4(const Vector4& v1, const Vector4& v2, const Vector4& v3, const Vector4& v4);
        inline explicit Matrix4(const Matrix3& mat);

        inline Matrix4& operator+=(const Matrix4& v);
        inline Matrix4& operator-=(const Matrix4& v);
        inline Matrix4& operator*=(float v);
        inline Matrix4& operator/=(float v);
        inline const Vector4& operator[](size_t index) const;
        inline Vector4& operator[](size_t index);
        inline Matrix4 Inverse() const;
        inline Matrix4 Transpose() const;
        [[nodiscard]]inline Matrix4 operator*(const Matrix4& v) const;
        [[nodiscard]]inline Vector4 operator*(const Vector4& v) const;
    };

    inline Matrix4 operator+(const Matrix4& lhs, const Matrix4& rhs)
    {
        return Matrix4(lhs) += rhs;
    }

    inline Matrix4 operator-(const Matrix4& lhs, const Matrix4& rhs)
    {
        return Matrix4(lhs) -= rhs;
    }

    inline Matrix4 operator*(const Matrix4& m, float v)
    {
        return Matrix4(m) *= v;
    }

    inline Matrix4 operator/(const Matrix4& m, float v)
    {
        return Matrix4(m) /= v;
    }

    inline Vector4 operator*(const Vector4& v, const Matrix4& matrix)
    {
        return Vector4(v.Dot(matrix.m[0]), v.Dot(matrix.m[1]), v.Dot(matrix.m[2]), v.Dot(matrix.m[3]));
    }

    struct Matrix3 {
        Vector3 m[3];

        inline Matrix3() = default;
        inline Matrix3(const Vector3& v1, const Vector3& v2, const Vector3& v3);
        inline explicit Matrix3(const Matrix4& mat);

        inline Matrix3& operator+=(const Matrix3& v);
        inline Matrix3& operator-=(const Matrix3& v);
        inline Matrix3& operator*=(float v);
        inline Matrix3& operator/=(float v);
        inline const Vector3& operator[](size_t index) const;
        inline Vector3& operator[](size_t index);

        [[nodiscard]] inline Matrix3 operator*(const Matrix3& v) const;
        [[nodiscard]] inline Vector3 operator*(const Vector3& v) const;
        inline float Determinant() const;
        inline void FromAxisRadian(const Vector3& direction);
        inline void FromAxisRadian(const Vector3& axis, const float& radian);
    };

    inline Matrix3 operator+(const Matrix3& lhs, const Matrix3& rhs)
    {
        return Matrix3(lhs) += rhs;
    }

    inline Matrix3 operator-(const Matrix3& lhs, const Matrix3& rhs)
    {
        return Matrix3(lhs) -= rhs;
    }

    inline Matrix3 operator*(const Matrix3& m, float v)
    {
        return Matrix3(m) *= v;
    }

    inline Matrix3 operator/(const Matrix3& m, float v)
    {
        return Matrix3(m) /= v;
    }
}

#include "core/math/MatrixX.inl"
