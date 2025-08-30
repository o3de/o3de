/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace SimuCore {
    inline Matrix4::Matrix4(const Vector4& v1, const Vector4& v2, const Vector4& v3, const Vector4& v4)
        : value(AZ::Matrix4x4::CreateFromRows( v1.value, v2.value, v3.value, v4.value ))
    {
    }

    inline Matrix4::Matrix4(const AZ::Matrix4x4& mat)
        : value(mat)
    {
    }

    inline Matrix4::Matrix4(const Matrix3& mat)
    {
        // copies the first 3,3 of mat into this, and sets the last column to 0
        // sets the last row to 0,0,0,1
        value.SetRow(0, mat.value.GetRow(0));
        value.SetRow(1, mat.value.GetRow(1));
        value.SetRow(2, mat.value.GetRow(2));
        value.SetRow(3, 0.f, 0.f, 0.f, 1.f);
        value.SetColumn(3, 0.f, 0.f, 0.f, 1.f);
    }

    inline Matrix4& Matrix4::operator+=(const Matrix4& v)
    {
        value += v.value;
        return *this;
    }

    inline Matrix4& Matrix4::operator-=(const Matrix4& v)
    {
        value -= v.value;
        return *this;
    }

    inline Matrix4& Matrix4::operator*=(float v)
    {
        value *= v;
        return *this;
    }

    inline Matrix4& Matrix4::operator/=(float v)
    {
        value /= v;
        return *this;
    }

    inline const Vector4 Matrix4::operator[](size_t index) const
    {
        return Vector4(value.GetRow(static_cast<int32_t>(index)));
    }

    inline Matrix4 Matrix4::Inverse() const
    {
        return value.GetInverseFull();
    }

    inline Matrix4 Matrix4::Transpose() const
    {
        return value.GetTranspose();
    }

    inline Matrix4 Matrix4::operator*(const Matrix4& v) const
    {
        return value * v.value;
    }

    inline Vector4 Matrix4::operator*(const Vector4& v) const
    {
        return value * v.value;
    }

    inline Matrix3::Matrix3(const Vector3& v1, const Vector3& v2, const Vector3& v3)
        : value(AZ::Matrix3x3::CreateFromRows(v1.value, v2.value, v3.value))
    {
    }

     inline Matrix3::Matrix3(const AZ::Matrix3x3& mat)
        : value(mat)
    {
    }

    inline Matrix3::Matrix3(const Matrix4& mat)
        : value(AZ::Matrix3x3::CreateFromMatrix4x4(mat.value))
    {
    }

    inline Matrix3& Matrix3::operator+=(const Matrix3& v)
    {
        value += v.value;
        return *this;
    }

    inline Matrix3& Matrix3::operator-=(const Matrix3& v)
    {
        value -= v.value;
        return *this;
    }
    inline Matrix3& Matrix3::operator*=(float v)
    {
        value *= v;
        return *this;
    }
    inline Matrix3& Matrix3::operator/=(float v)
    {
        value /= v;
        return *this;
    }

    inline Matrix3 Matrix3::operator*(const Matrix3& v) const
    {
       return value * v.value;
    }

    inline Vector3 Matrix3::operator*(const Vector3& v) const
    {
        return value * v.value;
    }

    inline const Vector3 Matrix3::operator[](size_t index) const
    {
        return value.GetRow(static_cast<int32_t>(index));
    }

    inline float Matrix3::Determinant() const
    {
        return value.GetDeterminant();
    }

    inline void Matrix3::FromAxisRadian(const Vector3& direction)
    {
        Matrix3 xMatrix = Matrix3(Vector3(0.f), Vector3(0.f), Vector3(0.f));
        Matrix3 yMatrix = Matrix3(Vector3(0.f), Vector3(0.f), Vector3(0.f));
        Matrix3 zMatrix = Matrix3(Vector3(0.f), Vector3(0.f), Vector3(0.f));
        Vector3 vectorX = Vector3(1.f, 0.f, 0.f);
        Vector3 vectorY = Vector3(0.f, 1.f, 0.f);
        Vector3 vectorZ = Vector3(0.f, 0.f, 1.f);
        xMatrix.FromAxisRadian(vectorX, Math::AngleToRadians(direction.value.GetX()));
        yMatrix.FromAxisRadian(vectorY, Math::AngleToRadians(direction.value.GetY()));
        zMatrix.FromAxisRadian(vectorZ, Math::AngleToRadians(direction.value.GetZ()));
        *this = zMatrix * (yMatrix * xMatrix);
    }

    inline void Matrix3::FromAxisRadian(const Vector3& axis, const float& radian)
    {
        AZ::Quaternion quat = AZ::Quaternion::CreateFromAxisAngle(axis.value, radian);
        value = AZ::Matrix3x3::CreateFromQuaternion(quat);
    }
}
