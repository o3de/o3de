/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>

namespace AZ
{
    AZ_MATH_INLINE void ConvertTo4x4(const Simd::Vec4::FloatType* in3x4, Simd::Vec4::FloatType* out4x4)
    {
        out4x4[0] = in3x4[0];
        out4x4[1] = in3x4[1];
        out4x4[2] = in3x4[2];
        out4x4[3] = Simd::Vec4::LoadAligned(Simd::g_vec0001);
    }

    AZ_MATH_INLINE Matrix3x4::Matrix3x4(Simd::Vec4::FloatArgType row0, Simd::Vec4::FloatArgType row1, Simd::Vec4::FloatArgType row2)
    {
        m_rows[0] = Vector4(row0);
        m_rows[1] = Vector4(row1);
        m_rows[2] = Vector4(row2);
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateIdentity()
    {
        return Matrix3x4(Simd::Vec4::LoadAligned(Simd::g_vec1000)
                       , Simd::Vec4::LoadAligned(Simd::g_vec0100)
                       , Simd::Vec4::LoadAligned(Simd::g_vec0010));
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateZero()
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        Matrix3x4 result;
        memset(result.m_rows, 0, sizeof(result.m_rows));
        return result;
#else
        const Simd::Vec4::FloatType zeroVec = Simd::Vec4::ZeroFloat();
        return Matrix3x4(zeroVec, zeroVec, zeroVec);
#endif
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateFromValue(float value)
    {
        const Simd::Vec4::FloatType values = Simd::Vec4::Splat(value);
        return Matrix3x4(values, values, values);
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateFromRowMajorFloat12(const float values[12])
    {
        return CreateFromRows(
            Vector4::CreateFromFloat4(&values[0]),
            Vector4::CreateFromFloat4(&values[4]),
            Vector4::CreateFromFloat4(&values[8])
        );
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateFromRows(const Vector4& row0, const Vector4& row1, const Vector4& row2)
    {
        Matrix3x4 result;
        result.m_rows[0] = row0;
        result.m_rows[1] = row1;
        result.m_rows[2] = row2;
        return result;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateFromColumnMajorFloat12(const float values[12])
    {
        return CreateFromColumns(
            Vector3::CreateFromFloat3(&values[0]),
            Vector3::CreateFromFloat3(&values[3]),
            Vector3::CreateFromFloat3(&values[6]),
            Vector3::CreateFromFloat3(&values[9])
        );
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateFromColumns(const Vector3& col0, const Vector3& col1, const Vector3& col2, const Vector3& col3)
    {
        Matrix3x4 result;
        result.SetColumns(col0, col1, col2, col3);
        return result;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateFromColumnMajorFloat16(const float values[16])
    {
        return CreateFromColumns(
            Vector3::CreateFromFloat3(&values[0]),
            Vector3::CreateFromFloat3(&values[4]),
            Vector3::CreateFromFloat3(&values[8]),
            Vector3::CreateFromFloat3(&values[12])
        );
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateRotationX(float angle)
    {
        Matrix3x4 result;
        float s, c;
        SinCos(angle, s, c);
        result.m_rows[0] = Vector4(Simd::Vec4::LoadAligned(Simd::g_vec1000));
        result.SetRow(1, 0.0f, c, -s, 0.0f);
        result.SetRow(2, 0.0f, s, c, 0.0f);
        return result;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateRotationY(float angle)
    {
        Matrix3x4 result;
        float s, c;
        SinCos(angle, s, c);
        result.SetRow(0, c, 0.0f, s, 0.0f);
        result.m_rows[1] = Vector4(Simd::Vec4::LoadAligned(Simd::g_vec0100));
        result.SetRow(2, -s, 0.0f, c, 0.0f);
        return result;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateRotationZ(float angle)
    {
        Matrix3x4 result;
        float s, c;
        SinCos(angle, s, c);
        result.SetRow(0, c, -s, 0.0f, 0.0f);
        result.SetRow(1, s, c, 0.0f, 0.0f);
        result.m_rows[2] = Vector4(Simd::Vec4::LoadAligned(Simd::g_vec0010));
        return result;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateFromQuaternion(const Quaternion& quaternion)
    {
        Matrix3x4 result;
        result.SetRotationPartFromQuaternion(quaternion);
        result.SetTranslation(Vector3::CreateZero());
        return result;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateFromQuaternionAndTranslation(const Quaternion& quaternion, const Vector3& translation)
    {
        Matrix3x4 result;
        result.SetRotationPartFromQuaternion(quaternion);
        result.SetTranslation(translation);
        return result;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateFromMatrix3x3(const Matrix3x3& matrix3x3)
    {
        Matrix3x4 result;
        result.SetRow(0, matrix3x3.GetRow(0), 0.0f);
        result.SetRow(1, matrix3x3.GetRow(1), 0.0f);
        result.SetRow(2, matrix3x3.GetRow(2), 0.0f);
        return result;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateFromMatrix3x3AndTranslation(const Matrix3x3& matrix3x3, const Vector3& translation)
    {
        Matrix3x4 result;
        result.SetRows(
            Vector4::CreateFromVector3AndFloat(matrix3x3.GetRow(0), translation.GetElement(0)),
            Vector4::CreateFromVector3AndFloat(matrix3x3.GetRow(1), translation.GetElement(1)),
            Vector4::CreateFromVector3AndFloat(matrix3x3.GetRow(2), translation.GetElement(2))
        );
        return result;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateFromTransform(const Transform& transform)
    {
        return Matrix3x4::CreateFromColumns(transform.GetBasisX(), transform.GetBasisY(), transform.GetBasisZ(), transform.GetTranslation());
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::UnsafeCreateFromMatrix4x4(const Matrix4x4& matrix4x4)
    {
        Matrix3x4 result;
        result.SetRow(0, matrix4x4.GetRow(0));
        result.SetRow(1, matrix4x4.GetRow(1));
        result.SetRow(2, matrix4x4.GetRow(2));
        return result;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateScale(const Vector3& scale)
    {
        return CreateDiagonal(scale);
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateDiagonal(const Vector3& diagonal)
    {
        return CreateFromRows(Vector4::CreateAxisX(diagonal.GetX()), Vector4::CreateAxisY(diagonal.GetY()), Vector4::CreateAxisZ(diagonal.GetZ()));
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::CreateTranslation(const Vector3& translation)
    {
        Matrix3x4 result = CreateIdentity();
        result.SetTranslation(translation);
        return result;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::Identity()
    {
        return CreateIdentity();;
    }

    AZ_MATH_INLINE void Matrix3x4::StoreToRowMajorFloat12(float values[12]) const
    {
        GetRow(0).StoreToFloat4(values);
        GetRow(1).StoreToFloat4(values + 4);
        GetRow(2).StoreToFloat4(values + 8);
    }

    AZ_MATH_INLINE void Matrix3x4::StoreToColumnMajorFloat12(float values[12]) const
    {
        GetColumn(0).StoreToFloat3(values);
        GetColumn(1).StoreToFloat3(values + 3);
        GetColumn(2).StoreToFloat3(values + 6);
        GetColumn(3).StoreToFloat3(values + 9);
    }

    AZ_MATH_INLINE void Matrix3x4::StoreToColumnMajorFloat16(float values[16]) const
    {
        GetColumn(0).StoreToFloat4(values);
        GetColumn(1).StoreToFloat4(values + 4);
        GetColumn(2).StoreToFloat4(values + 8);
        GetColumn(3).StoreToFloat4(values + 12);
    }

    AZ_MATH_INLINE float Matrix3x4::GetElement(int32_t row, int32_t col) const
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access");
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access");
        return m_rows[row].GetElement(col);
    }

    AZ_MATH_INLINE void Matrix3x4::SetElement(int32_t row, int32_t col, float value)
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access");
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access");
        m_rows[row].SetElement(col, value);
    }

    AZ_MATH_INLINE float Matrix3x4::operator()(int32_t row, int32_t col) const
    {
        return GetElement(row, col);
    }

    AZ_MATH_INLINE Vector4 Matrix3x4::GetRow(int32_t row) const
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access");
        return m_rows[row];
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::GetRowAsVector3(int32_t row) const
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access");
        return m_rows[row].GetAsVector3();
    }

    AZ_MATH_INLINE void Matrix3x4::SetRow(int32_t row, float x, float y, float z, float w)
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access");
        m_rows[row].Set(x, y, z, w);
    }

    AZ_MATH_INLINE void Matrix3x4::SetRow(int32_t row, const Vector3& v, float w)
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access");
        m_rows[row].Set(v, w);
    }

    AZ_MATH_INLINE void Matrix3x4::SetRow(int32_t row, const Vector4& v)
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access");
        m_rows[row] = v;
    }

    AZ_MATH_INLINE void Matrix3x4::GetRows(Vector4* row0, Vector4* row1, Vector4* row2) const
    {
        *row0 = m_rows[0];
        *row1 = m_rows[1];
        *row2 = m_rows[2];
    }

    AZ_MATH_INLINE void Matrix3x4::SetRows(const Vector4& row0, const Vector4& row1, const Vector4& row2)
    {
        m_rows[0] = row0;
        m_rows[1] = row1;
        m_rows[2] = row2;
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::GetColumn(int32_t col) const
    {
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access");
        return Vector3(m_rows[0].GetElement(col), m_rows[1].GetElement(col), m_rows[2].GetElement(col));
    }

    AZ_MATH_INLINE void Matrix3x4::SetColumn(int32_t col, float x, float y, float z)
    {
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access");
        m_rows[0].SetElement(col, x);
        m_rows[1].SetElement(col, y);
        m_rows[2].SetElement(col, z);
    }

    AZ_MATH_INLINE void Matrix3x4::SetColumn(int32_t col, const Vector3& v)
    {
        SetColumn(col, v.GetX(), v.GetY(), v.GetZ());
    }

    AZ_MATH_INLINE void Matrix3x4::GetColumns(Vector3* col0, Vector3* col1, Vector3* col2, Vector3* col3) const
    {
        *col0 = GetColumn(0);
        *col1 = GetColumn(1);
        *col2 = GetColumn(2);
        *col3 = GetColumn(3);
    }

    AZ_MATH_INLINE void Matrix3x4::SetColumns(const Vector3& col0, const Vector3& col1, const Vector3& col2, const Vector3& col3)
    {
        for (int32_t row = 0; row < RowCount; ++row)
        {
            m_rows[row].Set(col0.GetElement(row), col1.GetElement(row), col2.GetElement(row), col3.GetElement(row));
        }
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::GetBasisX() const
    {
        return GetColumn(0);
    }

    AZ_MATH_INLINE void Matrix3x4::SetBasisX(float x, float y, float z)
    {
        SetColumn(0, x, y, z);
    }

    AZ_MATH_INLINE void Matrix3x4::SetBasisX(const Vector3& v)
    {
        SetColumn(0, v.GetX(), v.GetY(), v.GetZ());
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::GetBasisY() const
    {
        return GetColumn(1);
    }

    AZ_MATH_INLINE void Matrix3x4::SetBasisY(float x, float y, float z)
    {
        SetColumn(1, x, y, z);
    }

    AZ_MATH_INLINE void Matrix3x4::SetBasisY(const Vector3& v)
    {
        SetColumn(1, v.GetX(), v.GetY(), v.GetZ());
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::GetBasisZ() const
    {
        return GetColumn(2);
    }

    AZ_MATH_INLINE void Matrix3x4::SetBasisZ(float x, float y, float z)
    {
        SetColumn(2, x, y, z);
    }

    AZ_MATH_INLINE void Matrix3x4::SetBasisZ(const Vector3& v)
    {
        SetColumn(2, v.GetX(), v.GetY(), v.GetZ());
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::GetTranslation() const
    {
        return GetColumn(3);
    }

    AZ_MATH_INLINE void Matrix3x4::SetTranslation(float x, float y, float z)
    {
        SetColumn(3, x, y, z);
    }

    AZ_MATH_INLINE void Matrix3x4::SetTranslation(const Vector3& v)
    {
        SetColumn(3, v);
    }

    AZ_MATH_INLINE void Matrix3x4::GetBasisAndTranslation(Vector3* basisX, Vector3* basisY, Vector3* basisZ, Vector3* translation) const
    {
        *basisX = GetColumn(0);
        *basisY = GetColumn(1);
        *basisZ = GetColumn(2);
        *translation = GetColumn(3);
    }

    AZ_MATH_INLINE void Matrix3x4::SetBasisAndTranslation(const Vector3& basisX, const Vector3& basisY, const Vector3& basisZ, const Vector3& translation)
    {
        SetColumns(basisX, basisY, basisZ, translation);
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::operator+(const Matrix3x4& rhs) const
    {
        return Matrix3x4
        (
            Simd::Vec4::Add(m_rows[0].GetSimdValue(), rhs.m_rows[0].GetSimdValue()),
            Simd::Vec4::Add(m_rows[1].GetSimdValue(), rhs.m_rows[1].GetSimdValue()),
            Simd::Vec4::Add(m_rows[2].GetSimdValue(), rhs.m_rows[2].GetSimdValue())
        );
    }

    AZ_MATH_INLINE Matrix3x4& Matrix3x4::operator+=(const Matrix3x4& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::operator-(const Matrix3x4& rhs) const
    {
        return Matrix3x4
        (
            Simd::Vec4::Sub(m_rows[0].GetSimdValue(), rhs.m_rows[0].GetSimdValue()),
            Simd::Vec4::Sub(m_rows[1].GetSimdValue(), rhs.m_rows[1].GetSimdValue()),
            Simd::Vec4::Sub(m_rows[2].GetSimdValue(), rhs.m_rows[2].GetSimdValue())
        );
    }

    AZ_MATH_INLINE Matrix3x4& Matrix3x4::operator-=(const Matrix3x4& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::operator*(const Matrix3x4& rhs) const
    {
        Matrix3x4 result;
        Simd::Vec4::Mat3x4Multiply(GetSimdValues(), rhs.GetSimdValues(), result.GetSimdValues());
        return result;
    }

    AZ_MATH_INLINE Matrix3x4& Matrix3x4::operator*=(const Matrix3x4& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::operator*(float multiplier) const
    {
        const Simd::Vec4::FloatType mulVec = Simd::Vec4::Splat(multiplier);
        return Matrix3x4
        (
            Simd::Vec4::Mul(m_rows[0].GetSimdValue(), mulVec),
            Simd::Vec4::Mul(m_rows[1].GetSimdValue(), mulVec),
            Simd::Vec4::Mul(m_rows[2].GetSimdValue(), mulVec)
        );
    }

    AZ_MATH_INLINE Matrix3x4& Matrix3x4::operator*=(float multiplier)
    {
        *this = *this * multiplier;
        return *this;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::operator/(float divisor) const
    {
        const Simd::Vec4::FloatType divVec = Simd::Vec4::Splat(divisor);
        return Matrix3x4
        (
            Simd::Vec4::Div(m_rows[0].GetSimdValue(), divVec),
            Simd::Vec4::Div(m_rows[1].GetSimdValue(), divVec),
            Simd::Vec4::Div(m_rows[2].GetSimdValue(), divVec)
        );
    }

    AZ_MATH_INLINE Matrix3x4& Matrix3x4::operator/=(float divisor)
    {
        *this = *this / divisor;
        return *this;
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::operator-() const
    {
        const Simd::Vec4::FloatType zeroVec = Simd::Vec4::ZeroFloat();
        return Matrix3x4
        (
            Simd::Vec4::Sub(zeroVec, m_rows[0].GetSimdValue()),
            Simd::Vec4::Sub(zeroVec, m_rows[1].GetSimdValue()),
            Simd::Vec4::Sub(zeroVec, m_rows[2].GetSimdValue())
        );
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::operator*(const Vector3& rhs) const
    {
        return Vector3
        (
            m_rows[0].Dot3(rhs) + m_rows[0].GetElement(3),
            m_rows[1].Dot3(rhs) + m_rows[1].GetElement(3),
            m_rows[2].Dot3(rhs) + m_rows[2].GetElement(3)
        );
    }

    AZ_MATH_INLINE Vector4 Matrix3x4::operator*(const Vector4& rhs) const
    {
        return Vector4
        (
            m_rows[0].Dot(rhs),
            m_rows[1].Dot(rhs),
            m_rows[2].Dot(rhs),
            rhs.GetElement(3)
        );
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::Multiply3x3(const Vector3& rhs) const
    {
        return Vector3
        (
            m_rows[0].Dot3(rhs),
            m_rows[1].Dot3(rhs),
            m_rows[2].Dot3(rhs)
        );
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::TransformVector(const Vector3& rhs) const
    {
        return Multiply3x3(rhs);
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::TransformPoint(const Vector3& rhs) const
    {
        return Multiply3x3(rhs) + GetTranslation();
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::GetTranspose() const
    {
        Matrix3x4 result;
        Simd::Vec4::Mat3x4Transpose(GetSimdValues(), result.GetSimdValues());
        return result;
    }

    AZ_MATH_INLINE void Matrix3x4::Transpose()
    {
        *this = GetTranspose();
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::GetTranspose3x3() const
    {
        Matrix3x4 result;
        result.SetRow(0, GetColumn(0), m_rows[0].GetElement(3));
        result.SetRow(1, GetColumn(1), m_rows[1].GetElement(3));
        result.SetRow(2, GetColumn(2), m_rows[2].GetElement(3));
        return result;
    }

    AZ_MATH_INLINE void Matrix3x4::Transpose3x3()
    {
        *this = GetTranspose3x3();
    }

    AZ_MATH_INLINE void Matrix3x4::InvertFull()
    {
        *this = GetInverseFull();
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::GetInverseFast() const
    {
        Matrix3x4 result;
        Simd::Vec4::Mat3x4InverseFast(GetSimdValues(), result.GetSimdValues());
        return result;
    }

    AZ_MATH_INLINE void Matrix3x4::InvertFast()
    {
        *this = GetInverseFast();
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::RetrieveScale() const
    {
        return Vector3(GetColumn(0).GetLength(), GetColumn(1).GetLength(), GetColumn(2).GetLength()) *
            AZ::GetSign(GetDeterminant3x3());
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::RetrieveScaleSq() const
    {
        return Vector3(GetColumn(0).GetLengthSq(), GetColumn(1).GetLengthSq(), GetColumn(2).GetLengthSq());
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::ExtractScale()
    {
        const Vector3 scale = RetrieveScale();
        MultiplyByScale(scale.GetReciprocal());
        return scale;
    }

    AZ_MATH_INLINE void Matrix3x4::MultiplyByScale(const Vector3& scale)
    {
        const Simd::Vec4::FloatType vector4Scale = Simd::Vec4::ReplaceIndex3(Simd::Vec4::FromVec3(scale.GetSimdValue()), 1.0f);
        m_rows[0].Set(Simd::Vec4::Mul(m_rows[0].GetSimdValue(), vector4Scale));
        m_rows[1].Set(Simd::Vec4::Mul(m_rows[1].GetSimdValue(), vector4Scale));
        m_rows[2].Set(Simd::Vec4::Mul(m_rows[2].GetSimdValue(), vector4Scale));
    }

    AZ_MATH_INLINE Matrix3x4 Matrix3x4::GetReciprocalScaled() const
    {
        Matrix3x4 result = *this;
        result.MultiplyByScale(RetrieveScaleSq().GetReciprocal());
        return result;
    }

    AZ_MATH_INLINE void Matrix3x4::Orthogonalize()
    {
        *this = GetOrthogonalized();
    }

    AZ_MATH_INLINE bool Matrix3x4::IsClose(const Matrix3x4& rhs, float tolerance) const
    {
        return m_rows[0].IsClose(rhs.m_rows[0], tolerance) && m_rows[1].IsClose(rhs.m_rows[1], tolerance) && m_rows[2].IsClose(rhs.m_rows[2], tolerance);
    }

    AZ_MATH_INLINE bool Matrix3x4::operator==(const Matrix3x4& rhs) const
    {
        return m_rows[0] == rhs.m_rows[0] && m_rows[1] == rhs.m_rows[1] && m_rows[2] == rhs.m_rows[2];
    }

    AZ_MATH_INLINE bool Matrix3x4::operator!=(const Matrix3x4& rhs) const
    {
        return !(*this == rhs);
    }

    AZ_MATH_INLINE Vector3 Matrix3x4::GetEulerDegrees() const
    {
        return Vector3RadToDeg(GetEulerRadians());
    }

    AZ_MATH_INLINE void Matrix3x4::SetFromEulerDegrees(const Vector3& eulerDegrees)
    {
        SetFromEulerRadians(Vector3DegToRad(eulerDegrees));
    }

    AZ_MATH_INLINE float Matrix3x4::GetDeterminant3x3() const
    {
        return m_rows[0].GetElement(0) * (m_rows[1].GetElement(1) * m_rows[2].GetElement(2) - m_rows[1].GetElement(2) * m_rows[2].GetElement(1))
             + m_rows[1].GetElement(0) * (m_rows[2].GetElement(1) * m_rows[0].GetElement(2) - m_rows[2].GetElement(2) * m_rows[0].GetElement(1))
             + m_rows[2].GetElement(0) * (m_rows[0].GetElement(1) * m_rows[1].GetElement(2) - m_rows[0].GetElement(2) * m_rows[1].GetElement(1));
    }

    AZ_MATH_INLINE bool Matrix3x4::IsFinite() const
    {
        return m_rows[0].IsFinite() && m_rows[1].IsFinite() && m_rows[2].IsFinite();
    }

    AZ_MATH_INLINE const Simd::Vec4::FloatType* Matrix3x4::GetSimdValues() const
    {
        return reinterpret_cast<const Simd::Vec4::FloatType*>(m_rows);
    }

    AZ_MATH_INLINE Simd::Vec4::FloatType* Matrix3x4::GetSimdValues()
    {
        return reinterpret_cast<Simd::Vec4::FloatType*>(m_rows);
    }

    AZ_MATH_INLINE Matrix3x4 operator*(float lhs, const Matrix3x4& rhs)
    {
        return rhs * lhs;
    }
} // namespace AZ
