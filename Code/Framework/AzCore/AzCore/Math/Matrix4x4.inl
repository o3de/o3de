/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>

namespace AZ
{
    AZ_MATH_INLINE Matrix4x4::Matrix4x4(Simd::Vec4::FloatArgType row0, Simd::Vec4::FloatArgType row1, Simd::Vec4::FloatArgType row2, Simd::Vec4::FloatArgType row3)
    {
        m_rows[0] = Vector4(row0);
        m_rows[1] = Vector4(row1);
        m_rows[2] = Vector4(row2);
        m_rows[3] = Vector4(row3);
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateIdentity()
    {
        return Matrix4x4(Simd::Vec4::LoadAligned(Simd::g_vec1000)
                       , Simd::Vec4::LoadAligned(Simd::g_vec0100)
                       , Simd::Vec4::LoadAligned(Simd::g_vec0010)
                       , Simd::Vec4::LoadAligned(Simd::g_vec0001));
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateZero()
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        Matrix4x4 result;
        memset(result.m_rows, 0, sizeof(result.m_rows));
        return result;
#else
        const Simd::Vec4::FloatType zeroVec = Simd::Vec4::ZeroFloat();
        return Matrix4x4(zeroVec, zeroVec, zeroVec, zeroVec);
#endif
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateFromValue(float value)
    {
        const Simd::Vec4::FloatType values = Simd::Vec4::Splat(value);
        return Matrix4x4(values, values, values, values);
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateFromRowMajorFloat16(const float* values)
    {
        return CreateFromRows(Vector4::CreateFromFloat4(&values[0])
                            , Vector4::CreateFromFloat4(&values[4])
                            , Vector4::CreateFromFloat4(&values[8])
                            , Vector4::CreateFromFloat4(&values[12]));
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateFromRows(const Vector4& row0, const Vector4& row1, const Vector4& row2, const Vector4& row3)
    {
        Matrix4x4 m;
        m.SetRows(row0, row1, row2, row3);
        return m;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateFromColumnMajorFloat16(const float* values)
    {
        return CreateFromColumns(Vector4::CreateFromFloat4(&values[0])
                               , Vector4::CreateFromFloat4(&values[4])
                               , Vector4::CreateFromFloat4(&values[8])
                               , Vector4::CreateFromFloat4(&values[12]));
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateFromColumns(const Vector4& col0, const Vector4& col1, const Vector4& col2, const Vector4& col3)
    {
        Matrix4x4 m;
        m.SetColumns(col0, col1, col2, col3);
        return m;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateRotationX(float angle)
    {
        Matrix4x4 result;
        float s, c;
        SinCos(angle, s, c);
        result.m_rows[0] = Vector4(Simd::Vec4::LoadAligned(Simd::g_vec1000));
        result.SetRow(1, 0.0f, c, -s, 0.0f);
        result.SetRow(2, 0.0f, s, c, 0.0f);
        result.m_rows[3] = Vector4(Simd::Vec4::LoadAligned(Simd::g_vec0001));
        return result;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateRotationY(float angle)
    {
        Matrix4x4 result;
        float s, c;
        SinCos(angle, s, c);
        result.SetRow(0, c, 0.0f, s, 0.0f);
        result.m_rows[1] = Vector4(Simd::Vec4::LoadAligned(Simd::g_vec0100));
        result.SetRow(2, -s, 0.0f, c, 0.0f);
        result.m_rows[3] = Vector4(Simd::Vec4::LoadAligned(Simd::g_vec0001));
        return result;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateRotationZ(float angle)
    {
        Matrix4x4 result;
        float s, c;
        SinCos(angle, s, c);
        result.SetRow(0, c, -s, 0.0f, 0.0f);
        result.SetRow(1, s, c, 0.0f, 0.0f);
        result.m_rows[2] = Vector4(Simd::Vec4::LoadAligned(Simd::g_vec0010));
        result.m_rows[3] = Vector4(Simd::Vec4::LoadAligned(Simd::g_vec0001));
        return result;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateFromQuaternion(const Quaternion& q)
    {
        Matrix4x4 result;
        result.SetRotationPartFromQuaternion(q);
        result.SetTranslation(Vector3(Simd::Vec3::ZeroFloat()));
        result.m_rows[3] = Vector4(Simd::Vec4::LoadAligned(Simd::g_vec0001));
        return result;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateFromQuaternionAndTranslation(const Quaternion& q, const Vector3& p)
    {
        Matrix4x4 result;
        result.SetRotationPartFromQuaternion(q);
        result.SetTranslation(p);
        result.m_rows[3] = Vector4(Simd::Vec4::LoadAligned(Simd::g_vec0001));
        return result;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateFromMatrix3x4(const Matrix3x4& matrix3x4)
    {
        Matrix4x4 result;
        result.SetRow(0, matrix3x4.GetRow(0));
        result.SetRow(1, matrix3x4.GetRow(1));
        result.SetRow(2, matrix3x4.GetRow(2));
        result.m_rows[3] = Vector4(Simd::Vec4::LoadAligned(Simd::g_vec0001));
        return result;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateFromTransform(const Transform& transform)
    {
        return CreateFromMatrix3x4(Matrix3x4::CreateFromTransform(transform));
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateScale(const Vector3& scale)
    {
        return CreateDiagonal(Vector4(scale.GetX(), scale.GetY(), scale.GetZ(), 1.0f));
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateDiagonal(const Vector4& diagonal)
    {
        Matrix4x4 result;
        result.SetRow(0, diagonal.GetX(), 0.0f, 0.0f, 0.0f);
        result.SetRow(1, 0.0f, diagonal.GetY(), 0.0f, 0.0f);
        result.SetRow(2, 0.0f, 0.0f, diagonal.GetZ(), 0.0f);
        result.SetRow(3, 0.0f, 0.0f, 0.0f, diagonal.GetW());
        return result;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::CreateTranslation(const Vector3& translation)
    {
        Matrix4x4 result;
        result.SetRow(0, 1.0f, 0.0f, 0.0f, translation.GetX());
        result.SetRow(1, 0.0f, 1.0f, 0.0f, translation.GetY());
        result.SetRow(2, 0.0f, 0.0f, 1.0f, translation.GetZ());
        result.m_rows[3] = Vector4(Simd::Vec4::LoadAligned(Simd::g_vec0001));
        return result;
    }

    AZ_MATH_INLINE void Matrix4x4::StoreToRowMajorFloat16(float* values) const
    {
        GetRow(0).StoreToFloat4(values);
        GetRow(1).StoreToFloat4(values + 4);
        GetRow(2).StoreToFloat4(values + 8);
        GetRow(3).StoreToFloat4(values + 12);
    }

    AZ_MATH_INLINE void Matrix4x4::StoreToColumnMajorFloat16(float* values) const
    {
        GetColumn(0).StoreToFloat4(values);
        GetColumn(1).StoreToFloat4(values + 4);
        GetColumn(2).StoreToFloat4(values + 8);
        GetColumn(3).StoreToFloat4(values + 12);
    }

    AZ_MATH_INLINE float Matrix4x4::GetElement(int32_t row, int32_t col) const
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access.\n");
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access.\n");
        return m_rows[row].GetElement(col);
    }

    AZ_MATH_INLINE void Matrix4x4::SetElement(int32_t row, int32_t col, float value)
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access.\n");
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access.\n");
        m_rows[row].SetElement(col, value);
    }

    AZ_MATH_INLINE float Matrix4x4::operator()(int32_t row, int32_t col) const
    {
        return GetElement(row, col);
    }

    AZ_MATH_INLINE Vector4 Matrix4x4::GetRow(int32_t row) const
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access.\n");
        return m_rows[row];
    }

    AZ_MATH_INLINE Vector3 Matrix4x4::GetRowAsVector3(int32_t row) const
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access.\n");
        return m_rows[row].GetAsVector3();
    }

    AZ_MATH_INLINE void Matrix4x4::GetRows(Vector4* row0, Vector4* row1, Vector4* row2, Vector4* row3) const
    {
        *row0 = GetRow(0);
        *row1 = GetRow(1);
        *row2 = GetRow(2);
        *row3 = GetRow(3);
    }

    AZ_MATH_INLINE void Matrix4x4::SetRow(int32_t row, float x, float y, float z, float w)
    {
        SetRow(row, Vector4(x, y, z, w));
    }

    AZ_MATH_INLINE void Matrix4x4::SetRow(int32_t row, const Vector3& v)
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access.\n");
        m_rows[row].SetElement(0, v.GetX());
        m_rows[row].SetElement(1, v.GetY());
        m_rows[row].SetElement(2, v.GetZ());
    }

    AZ_MATH_INLINE void Matrix4x4::SetRow(int32_t row, const Vector3& v, float w)
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access.\n");
        m_rows[row] = Vector4(Simd::Vec4::FromVec3(v.GetSimdValue()));
        m_rows[row].SetElement(3, w);
    }

    AZ_MATH_INLINE void Matrix4x4::SetRow(int32_t row, const Vector4& v)
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access.\n");
        m_rows[row] = v;
    }

    AZ_MATH_INLINE void Matrix4x4::SetRows(const Vector4& row0, const Vector4& row1, const Vector4& row2, const Vector4& row3)
    {
        SetRow(0, row0);
        SetRow(1, row1);
        SetRow(2, row2);
        SetRow(3, row3);
    }

    AZ_MATH_INLINE Vector4 Matrix4x4::GetColumn(int32_t col) const
    {
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access.\n");
        return Vector4(m_rows[0].GetElement(col), m_rows[1].GetElement(col), m_rows[2].GetElement(col), m_rows[3].GetElement(col));
    }

    AZ_MATH_INLINE Vector3 Matrix4x4::GetColumnAsVector3(int32_t col) const
    {
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access.\n");
        return Vector3(m_rows[0].GetElement(col), m_rows[1].GetElement(col), m_rows[2].GetElement(col));
    }

    AZ_MATH_INLINE void Matrix4x4::GetColumns(Vector4* col0, Vector4* col1, Vector4* col2, Vector4* col3) const
    {
        *col0 = GetColumn(0);
        *col1 = GetColumn(1);
        *col2 = GetColumn(2);
        *col3 = GetColumn(3);
    }

    AZ_MATH_INLINE void Matrix4x4::SetColumn(int32_t col, float x, float y, float z, float w)
    {
        SetColumn(col, Vector4(x, y, z, w));
    }

    AZ_MATH_INLINE void Matrix4x4::SetColumn(int32_t col, const Vector3& v)
    {
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access.\n");
        m_rows[0].SetElement(col, v.GetX());
        m_rows[1].SetElement(col, v.GetY());
        m_rows[2].SetElement(col, v.GetZ());
    }

    AZ_MATH_INLINE void Matrix4x4::SetColumn(int32_t col, const Vector3& v, float w)
    {
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access.\n");
        m_rows[0].SetElement(col, v.GetX());
        m_rows[1].SetElement(col, v.GetY());
        m_rows[2].SetElement(col, v.GetZ());
        m_rows[3].SetElement(col, w);
    }

    AZ_MATH_INLINE void Matrix4x4::SetColumn(int32_t col, const Vector4& v)
    {
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access.\n");
        m_rows[0].SetElement(col, v.GetX());
        m_rows[1].SetElement(col, v.GetY());
        m_rows[2].SetElement(col, v.GetZ());
        m_rows[3].SetElement(col, v.GetW());
    }

    AZ_MATH_INLINE void Matrix4x4::SetColumns(const Vector4& col0, const Vector4& col1, const Vector4& col2, const Vector4& col3)
    {
        SetColumn(0, col0);
        SetColumn(1, col1);
        SetColumn(2, col2);
        SetColumn(3, col3);
    }

    AZ_MATH_INLINE Vector4 Matrix4x4::GetBasisX() const
    {
        return GetColumn(0);
    }

    AZ_MATH_INLINE Vector3 Matrix4x4::GetBasisXAsVector3() const
    {
        return GetColumnAsVector3(0);
    }

    AZ_MATH_INLINE void Matrix4x4::SetBasisX(float x, float y, float z, float w)
    {
        SetColumn(0, x, y, z, w);
    }

    AZ_MATH_INLINE void Matrix4x4::SetBasisX(const Vector4& v)
    {
        SetColumn(0, v);
    }

    AZ_MATH_INLINE Vector4 Matrix4x4::GetBasisY() const
    {
        return GetColumn(1);
    }

    AZ_MATH_INLINE Vector3 Matrix4x4::GetBasisYAsVector3() const
    {
        return GetColumnAsVector3(1);
    }

    AZ_MATH_INLINE void Matrix4x4::SetBasisY(float x, float y, float z, float w)
    {
        SetColumn(1, x, y, z, w);
    }

    AZ_MATH_INLINE void Matrix4x4::SetBasisY(const Vector4& v)
    {
        SetColumn(1, v);
    }

    AZ_MATH_INLINE Vector4 Matrix4x4::GetBasisZ() const
    {
        return GetColumn(2);
    }

    AZ_MATH_INLINE Vector3 Matrix4x4::GetBasisZAsVector3() const
    {
        return GetColumnAsVector3(2);
    }

    AZ_MATH_INLINE void Matrix4x4::SetBasisZ(float x, float y, float z, float w)
    {
        SetColumn(2, x, y, z, w);
    }

    AZ_MATH_INLINE void Matrix4x4::SetBasisZ(const Vector4& v)
    {
        SetColumn(2, v);
    }

    AZ_MATH_INLINE void Matrix4x4::GetBasisAndTranslation(Vector4* basisX, Vector4* basisY, Vector4* basisZ, Vector4* pos) const
    {
        GetColumns(basisX, basisY, basisZ, pos);
    }

    AZ_MATH_INLINE void Matrix4x4::SetBasisAndTranslation(const Vector4& basisX, const Vector4& basisY, const Vector4& basisZ, const Vector4& pos)
    {
        SetColumns(basisX, basisY, basisZ, pos);
    }

    AZ_MATH_INLINE Vector3 Matrix4x4::GetTranslation() const
    {
        return GetColumnAsVector3(3);
    }

    AZ_MATH_INLINE void Matrix4x4::SetTranslation(float x, float y, float z)
    {
        SetTranslation(Vector3(x, y, z));
    }

    AZ_MATH_INLINE void Matrix4x4::SetTranslation(const Vector3& v)
    {
        SetColumn(3, v);
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::operator+(const Matrix4x4& rhs) const
    {
        return Matrix4x4
        (
            Simd::Vec4::Add(m_rows[0].GetSimdValue(), rhs.m_rows[0].GetSimdValue()),
            Simd::Vec4::Add(m_rows[1].GetSimdValue(), rhs.m_rows[1].GetSimdValue()),
            Simd::Vec4::Add(m_rows[2].GetSimdValue(), rhs.m_rows[2].GetSimdValue()),
            Simd::Vec4::Add(m_rows[3].GetSimdValue(), rhs.m_rows[3].GetSimdValue())
        );
    }

    AZ_MATH_INLINE Matrix4x4& Matrix4x4::operator+=(const Matrix4x4& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::operator-(const Matrix4x4& rhs) const
    {
        return Matrix4x4
        (
            Simd::Vec4::Sub(m_rows[0].GetSimdValue(), rhs.m_rows[0].GetSimdValue()),
            Simd::Vec4::Sub(m_rows[1].GetSimdValue(), rhs.m_rows[1].GetSimdValue()),
            Simd::Vec4::Sub(m_rows[2].GetSimdValue(), rhs.m_rows[2].GetSimdValue()),
            Simd::Vec4::Sub(m_rows[3].GetSimdValue(), rhs.m_rows[3].GetSimdValue())
        );
    }

    AZ_MATH_INLINE Matrix4x4& Matrix4x4::operator-=(const Matrix4x4& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::operator*(const Matrix4x4& rhs) const
    {
        Matrix4x4 result;
        Simd::Vec4::Mat4x4Multiply(GetSimdValues(), rhs.GetSimdValues(), result.GetSimdValues());
        return result;
    }

    AZ_MATH_INLINE Matrix4x4& Matrix4x4::operator*=(const Matrix4x4& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::operator*(float multiplier) const
    {
        const Simd::Vec4::FloatType mulVec = Simd::Vec4::Splat(multiplier);
        return Matrix4x4
        (
            Simd::Vec4::Mul(m_rows[0].GetSimdValue(), mulVec),
            Simd::Vec4::Mul(m_rows[1].GetSimdValue(), mulVec),
            Simd::Vec4::Mul(m_rows[2].GetSimdValue(), mulVec),
            Simd::Vec4::Mul(m_rows[3].GetSimdValue(), mulVec)
        );
    }

    AZ_MATH_INLINE Matrix4x4& Matrix4x4::operator*=(float multiplier)
    {
        *this = *this * multiplier;
        return *this;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::operator/(float divisor) const
    {
        const Simd::Vec4::FloatType divVec = Simd::Vec4::Splat(divisor);
        return Matrix4x4
        (
            Simd::Vec4::Div(m_rows[0].GetSimdValue(), divVec),
            Simd::Vec4::Div(m_rows[1].GetSimdValue(), divVec),
            Simd::Vec4::Div(m_rows[2].GetSimdValue(), divVec),
            Simd::Vec4::Div(m_rows[3].GetSimdValue(), divVec)
        );
    }

    AZ_MATH_INLINE Matrix4x4& Matrix4x4::operator/=(float divisor)
    {
        *this = *this / divisor;
        return *this;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::operator-() const
    {
        const Simd::Vec4::FloatType zeroVec = Simd::Vec4::ZeroFloat();
        return Matrix4x4
        (
            Simd::Vec4::Sub(zeroVec, m_rows[0].GetSimdValue()),
            Simd::Vec4::Sub(zeroVec, m_rows[1].GetSimdValue()),
            Simd::Vec4::Sub(zeroVec, m_rows[2].GetSimdValue()),
            Simd::Vec4::Sub(zeroVec, m_rows[3].GetSimdValue())
        );
    }

    AZ_MATH_INLINE Vector3 Matrix4x4::operator*(const Vector3& rhs) const
    {
        return Vector3(Simd::Vec4::Mat4x4TransformPoint3(GetSimdValues(), rhs.GetSimdValue()));
    }

    AZ_MATH_INLINE Vector4 Matrix4x4::operator*(const Vector4& rhs) const
    {
        return Vector4(Simd::Vec4::Mat4x4TransformVector(GetSimdValues(), rhs.GetSimdValue()));
    }

    AZ_MATH_INLINE Vector3 Matrix4x4::TransposedMultiply3x3(const Vector3& v) const
    {
        const Simd::Vec3::FloatType rows[3] = { Simd::Vec4::ToVec3(m_rows[0].GetSimdValue()), Simd::Vec4::ToVec3(m_rows[1].GetSimdValue()), Simd::Vec4::ToVec3(m_rows[2].GetSimdValue()) };
        return Vector3(Simd::Vec3::Mat3x3TransposeTransformVector(rows, v.GetSimdValue()));
    }

    AZ_MATH_INLINE Vector3 Matrix4x4::Multiply3x3(const Vector3& v) const
    {
        const Simd::Vec3::FloatType rows[3] = { Simd::Vec4::ToVec3(m_rows[0].GetSimdValue()), Simd::Vec4::ToVec3(m_rows[1].GetSimdValue()), Simd::Vec4::ToVec3(m_rows[2].GetSimdValue()) };
        return Vector3(Simd::Vec3::Mat3x3TransformVector(rows, v.GetSimdValue()));
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::GetTranspose() const
    {
        Matrix4x4 result;
        Simd::Vec4::Mat4x4Transpose(GetSimdValues(), result.GetSimdValues());
        return result;
    }

    AZ_MATH_INLINE void Matrix4x4::Transpose()
    {
        *this = GetTranspose();
    }

    AZ_MATH_INLINE void Matrix4x4::InvertFull()
    {
        *this = GetInverseFull();
    }

    AZ_MATH_INLINE void Matrix4x4::InvertTransform()
    {
        *this = GetInverseTransform();
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::GetInverseFast() const
    {
        Matrix4x4 out;
        Simd::Vec4::Mat4x4InverseFast(GetSimdValues(), out.GetSimdValues());
        return out;
    }

    AZ_MATH_INLINE void Matrix4x4::InvertFast()
    {
        *this = GetInverseFast();
    }

    AZ_MATH_INLINE Vector3 Matrix4x4::RetrieveScale() const
    {
        return Vector3(GetBasisX().GetLength(), GetBasisY().GetLength(), GetBasisZ().GetLength());
    }

    AZ_MATH_INLINE Vector3 Matrix4x4::RetrieveScaleSq() const
    {
        return Vector3(GetBasisX().GetLengthSq(), GetBasisY().GetLengthSq(), GetBasisZ().GetLengthSq());
    }

    AZ_MATH_INLINE Vector3 Matrix4x4::ExtractScale()
    {
        Vector4 x = GetBasisX();
        Vector4 y = GetBasisY();
        Vector4 z = GetBasisZ();
        float lengthX = x.NormalizeWithLength();
        float lengthY = y.NormalizeWithLength();
        float lengthZ = z.NormalizeWithLength();
        SetBasisX(x);
        SetBasisY(y);
        SetBasisZ(z);
        return Vector3(lengthX, lengthY, lengthZ);
    }

    AZ_MATH_INLINE void Matrix4x4::MultiplyByScale(const Vector3& scale)
    {
        const Vector4 scalars = Vector4::CreateFromVector3AndFloat(scale, 1.0f);
        m_rows[0] = m_rows[0] * scalars;
        m_rows[1] = m_rows[1] * scalars;
        m_rows[2] = m_rows[2] * scalars;
    }

    AZ_MATH_INLINE Matrix4x4 Matrix4x4::GetReciprocalScaled() const
    {
        Matrix4x4 result = *this;
        result.MultiplyByScale(RetrieveScaleSq().GetReciprocal());
        return result;
    }

    AZ_MATH_INLINE bool Matrix4x4::IsClose(const Matrix4x4& rhs, float tolerance) const
    {
        const Simd::Vec4::FloatType vecTolerance = Simd::Vec4::Splat(tolerance);
        for (int32_t row = 0; row < RowCount; ++row)
        {
            const Simd::Vec4::FloatType compare = Simd::Vec4::Abs(Simd::Vec4::Sub(m_rows[row].GetSimdValue(), rhs.m_rows[row].GetSimdValue()));
            if (!Simd::Vec4::CmpAllLt(compare, vecTolerance))
            {
                return false;
            }
        }
        return true;
    }

    AZ_MATH_INLINE bool Matrix4x4::operator==(const Matrix4x4& rhs) const
    {
        return (Simd::Vec4::CmpAllEq(m_rows[0].GetSimdValue(), rhs.m_rows[0].GetSimdValue())
             && Simd::Vec4::CmpAllEq(m_rows[1].GetSimdValue(), rhs.m_rows[1].GetSimdValue())
             && Simd::Vec4::CmpAllEq(m_rows[2].GetSimdValue(), rhs.m_rows[2].GetSimdValue())
             && Simd::Vec4::CmpAllEq(m_rows[3].GetSimdValue(), rhs.m_rows[3].GetSimdValue()));
    }

    AZ_MATH_INLINE bool Matrix4x4::operator!=(const Matrix4x4& rhs) const
    {
        return !operator==(rhs);
    }

    AZ_MATH_INLINE Vector4 Matrix4x4::GetDiagonal() const
    {
        return Vector4(GetElement(0, 0), GetElement(1, 1), GetElement(2, 2), GetElement(3, 3));
    }

    AZ_MATH_INLINE bool Matrix4x4::IsFinite() const
    {
        return GetRow(0).IsFinite() && GetRow(1).IsFinite() && GetRow(2).IsFinite() && GetRow(3).IsFinite();
    }

    AZ_MATH_INLINE const Simd::Vec4::FloatType* Matrix4x4::GetSimdValues() const
    {
        return reinterpret_cast<const Simd::Vec4::FloatType*>(m_rows);
    }

    AZ_MATH_INLINE Simd::Vec4::FloatType* Matrix4x4::GetSimdValues()
    {
        return reinterpret_cast<Simd::Vec4::FloatType*>(m_rows);
    }

    AZ_MATH_INLINE const Vector3 operator*(const Vector3& lhs, const Matrix4x4& rhs)
    {
        return Vector3(lhs(0) * rhs(0, 0) + lhs(1) * rhs(1, 0) + lhs(2) * rhs(2, 0) + rhs(3, 0),
                       lhs(0) * rhs(0, 1) + lhs(1) * rhs(1, 1) + lhs(2) * rhs(2, 1) + rhs(3, 1),
                       lhs(0) * rhs(0, 2) + lhs(1) * rhs(1, 2) + lhs(2) * rhs(2, 2) + rhs(3, 2));
    }

    AZ_MATH_INLINE Vector3& operator*=(Vector3& lhs, const Matrix4x4& rhs)
    {
        lhs = lhs * rhs;
        return lhs;
    }

    AZ_MATH_INLINE const Vector4 operator*(const Vector4& lhs, const Matrix4x4& rhs)
    {
        return Vector4(lhs(0) * rhs(0, 0) + lhs(1) * rhs(1, 0) + lhs(2) * rhs(2, 0) + lhs(3) * rhs(3, 0),
                       lhs(0) * rhs(0, 1) + lhs(1) * rhs(1, 1) + lhs(2) * rhs(2, 1) + lhs(3) * rhs(3, 1),
                       lhs(0) * rhs(0, 2) + lhs(1) * rhs(1, 2) + lhs(2) * rhs(2, 2) + lhs(3) * rhs(3, 2),
                       lhs(0) * rhs(0, 3) + lhs(1) * rhs(1, 3) + lhs(2) * rhs(2, 3) + lhs(3) * rhs(3, 3));
    }

    AZ_MATH_INLINE Vector4& operator*=(Vector4& lhs, const Matrix4x4& rhs)
    {
        lhs = lhs * rhs;
        return lhs;
    }

    AZ_MATH_INLINE Matrix4x4 operator*(float lhs, const Matrix4x4& rhs)
    {
        return rhs * lhs;
    }
} // namespace AZ
