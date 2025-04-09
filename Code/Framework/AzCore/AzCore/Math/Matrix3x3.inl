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
    AZ_MATH_INLINE Matrix3x3::Matrix3x3(Simd::Vec3::FloatArgType row0, Simd::Vec3::FloatArgType row1, Simd::Vec3::FloatArgType row2)
    {
        m_rows[0] = Vector3(row0);
        m_rows[1] = Vector3(row1);
        m_rows[2] = Vector3(row2);
    }

    AZ_MATH_INLINE Matrix3x3::Matrix3x3(const Quaternion& quaternion)
    {
        *this = CreateFromQuaternion(quaternion);
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateIdentity()
    {
        return Matrix3x3(Simd::Vec3::LoadAligned(Simd::g_vec1000)
                       , Simd::Vec3::LoadAligned(Simd::g_vec0100)
                       , Simd::Vec3::LoadAligned(Simd::g_vec0010));
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateZero()
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        Matrix3x3 result;
        memset(result.m_rows, 0, sizeof(result.m_rows));
        return result;
#else
        const Simd::Vec3::FloatType zeroVec = Simd::Vec3::ZeroFloat();
        return Matrix3x3(zeroVec, zeroVec, zeroVec);
#endif
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateFromValue(float value)
    {
        const Simd::Vec3::FloatType values = Simd::Vec3::Splat(value);
        return Matrix3x3(values, values, values);
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateFromRowMajorFloat9(const float* values)
    {
        return CreateFromRows(Vector3::CreateFromFloat3(&values[0]), Vector3::CreateFromFloat3(&values[3]), Vector3::CreateFromFloat3(&values[6]));
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateFromRows(const Vector3& row0, const Vector3& row1, const Vector3& row2)
    {
        return Matrix3x3(row0.GetSimdValue(), row1.GetSimdValue(), row2.GetSimdValue());
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateFromColumnMajorFloat9(const float* values)
    {
        return CreateFromColumns(Vector3::CreateFromFloat3(&values[0]), Vector3::CreateFromFloat3(&values[3]), Vector3::CreateFromFloat3(&values[6]));
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateFromColumns(const Vector3& col0, const Vector3& col1, const Vector3& col2)
    {
        Matrix3x3 m;
        m.SetColumns(col0, col1, col2);
        return m;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateRotationX(float angle)
    {
        Matrix3x3 result;
        float s, c;
        SinCos(angle, s, c);
        result.m_rows[0] = Vector3(Simd::Vec3::LoadAligned(Simd::g_vec1000));
        result.SetRow(1, 0.0f, c, -s);
        result.SetRow(2, 0.0f, s, c);
        return result;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateRotationY(float angle)
    {
        Matrix3x3 result;
        float s, c;
        SinCos(angle, s, c);
        result.SetRow(0, c, 0.0f, s);
        result.m_rows[1] = Vector3(Simd::Vec3::LoadAligned(Simd::g_vec0100));
        result.SetRow(2, -s, 0.0f, c);
        return result;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateRotationZ(float angle)
    {
        Matrix3x3 result;
        float s, c;
        SinCos(angle, s, c);
        result.SetRow(0, c, -s, 0.0f);
        result.SetRow(1, s, c, 0.0f);
        result.m_rows[2] = Vector3(Simd::Vec3::LoadAligned(Simd::g_vec0010));
        return result;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateFromQuaternion(const Quaternion& q)
    {
        Matrix3x3 result;
        result.SetRotationPartFromQuaternion(q);
        return result;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateFromMatrix3x4(const Matrix3x4& m)
    {
        Matrix3x3 result;
        result.SetRow(0, m.GetRowAsVector3(0));
        result.SetRow(1, m.GetRowAsVector3(1));
        result.SetRow(2, m.GetRowAsVector3(2));
        return result;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateFromMatrix4x4(const Matrix4x4& m)
    {
        Matrix3x3 result;
        result.m_rows[0] = m.GetRow(0).GetAsVector3();
        result.m_rows[1] = m.GetRow(1).GetAsVector3();
        result.m_rows[2] = m.GetRow(2).GetAsVector3();
        return result;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateFromTransform(const Transform& t)
    {
        return Matrix3x3::CreateFromColumns(t.GetBasisX(), t.GetBasisY(), t.GetBasisZ());
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateScale(const Vector3& scale)
    {
        return CreateDiagonal(scale);
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateDiagonal(const Vector3& diagonal)
    {
        Matrix3x3 result;
        result.SetRow(0, diagonal.GetX(), 0.0f, 0.0f);
        result.SetRow(1, 0.0f, diagonal.GetY(), 0.0f);
        result.SetRow(2, 0.0f, 0.0f, diagonal.GetZ());
        return result;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::CreateCrossProduct(const Vector3& p)
    {
        Matrix3x3 result;
        result.SetRow(0, 0.0f, -p.GetZ(), p.GetY());
        result.SetRow(1, p.GetZ(), 0.0f, -p.GetX());
        result.SetRow(2, -p.GetY(), p.GetX(), 0.0f);
        return result;
    }

    AZ_MATH_INLINE void Matrix3x3::StoreToRowMajorFloat9(float* values) const
    {
        // note: StoreToFloat4 is potentially faster, even if we overwrite the last value immediately
        GetRow(0).StoreToFloat4(values);
        GetRow(1).StoreToFloat4(values + 3);
        GetRow(2).StoreToFloat3(values + 6);
    }

    AZ_MATH_INLINE void Matrix3x3::StoreToRowMajorFloat11(float* values) const
    {
        // for GPU constant - buffers each float3 is aligned on a 16 bit border
        GetRow(0).StoreToFloat4(values);
        GetRow(1).StoreToFloat4(values + 4);
        GetRow(2).StoreToFloat3(values + 8);
    }

    AZ_MATH_INLINE void Matrix3x3::StoreToColumnMajorFloat9(float* values) const
    {
        // note: StoreToFloat4 is potentially faster, even if we overwrite the last value immediately
        GetColumn(0).StoreToFloat4(values);
        GetColumn(1).StoreToFloat4(values + 3);
        GetColumn(2).StoreToFloat3(values + 6);
    }

    AZ_MATH_INLINE void Matrix3x3::StoreToColumnMajorFloat11(float* values) const
    {
        GetColumn(0).StoreToFloat4(values);
        GetColumn(1).StoreToFloat4(values + 4);
        GetColumn(2).StoreToFloat3(values + 8);
    }

    AZ_MATH_INLINE float Matrix3x3::GetElement(int32_t row, int32_t col) const
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access.\n");
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access.\n");
        return m_rows[row].GetElement(col);
    }

    AZ_MATH_INLINE void Matrix3x3::SetElement(int32_t row, int32_t col, float value)
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access.\n");
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access.\n");
        m_rows[row].SetElement(col, value);
    }

    AZ_MATH_INLINE float Matrix3x3::operator()(int32_t row, int32_t col) const
    {
        return GetElement(row, col);
    }

    AZ_MATH_INLINE Vector3 Matrix3x3::GetRow(int32_t row) const
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access.\n");
        return m_rows[row];
    }

    AZ_MATH_INLINE void Matrix3x3::SetRow(int32_t row, float x, float y, float z)
    {
        SetRow(row, Vector3(x, y, z));
    }

    AZ_MATH_INLINE void Matrix3x3::SetRow(int32_t row, const Vector3& v)
    {
        AZ_MATH_ASSERT((row >= 0) && (row < RowCount), "Invalid index for component access.\n");
        m_rows[row] = v;
    }

    AZ_MATH_INLINE void Matrix3x3::GetRows(Vector3* row0, Vector3* row1, Vector3* row2) const
    {
        *row0 = GetRow(0);
        *row1 = GetRow(1);
        *row2 = GetRow(2);
    }

    AZ_MATH_INLINE void Matrix3x3::SetRows(const Vector3& row0, const Vector3& row1, const Vector3& row2)
    {
        SetRow(0, row0);
        SetRow(1, row1);
        SetRow(2, row2);
    }

    AZ_MATH_INLINE Vector3 Matrix3x3::GetColumn(int32_t col) const
    {
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access.\n");
        return Vector3(m_rows[0].GetElement(col), m_rows[1].GetElement(col), m_rows[2].GetElement(col));
    }

    AZ_MATH_INLINE void Matrix3x3::SetColumn(int32_t col, float x, float y, float z)
    {
        m_rows[0].SetElement(col, x);
        m_rows[1].SetElement(col, y);
        m_rows[2].SetElement(col, z);
    }

    AZ_MATH_INLINE void Matrix3x3::SetColumn(int32_t col, const Vector3& v)
    {
        AZ_MATH_ASSERT((col >= 0) && (col < ColCount), "Invalid index for component access.\n");
        m_rows[0].SetElement(col, v.GetX());
        m_rows[1].SetElement(col, v.GetY());
        m_rows[2].SetElement(col, v.GetZ());
    }

    AZ_MATH_INLINE void Matrix3x3::GetColumns(Vector3* col0, Vector3* col1, Vector3* col2) const
    {
        *col0 = GetColumn(0);
        *col1 = GetColumn(1);
        *col2 = GetColumn(2);
    }

    AZ_MATH_INLINE void Matrix3x3::SetColumns(const Vector3& col0, const Vector3& col1, const Vector3& col2)
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        SetColumn(0, col0);
        SetColumn(1, col1);
        SetColumn(2, col2);
#else
        // In Simd it's faster to set the columns as rows and transpose
        SetRow(0, col0);
        SetRow(1, col1);
        SetRow(2, col2);
        Transpose();
#endif
    }

    AZ_MATH_INLINE Vector3 Matrix3x3::GetBasisX() const
    {
        return GetColumn(0);
    }

    AZ_MATH_INLINE void Matrix3x3::SetBasisX(float x, float y, float z)
    {
        SetColumn(0, x, y, z);
    }

    AZ_MATH_INLINE void Matrix3x3::SetBasisX(const Vector3& v)
    {
        SetColumn(0, v);
    }

    AZ_MATH_INLINE Vector3 Matrix3x3::GetBasisY() const
    {
        return GetColumn(1);
    }

    AZ_MATH_INLINE void Matrix3x3::SetBasisY(float x, float y, float z)
    {
        SetColumn(1, x, y, z);
    }

    AZ_MATH_INLINE void Matrix3x3::SetBasisY(const Vector3& v)
    {
        SetColumn(1, v);
    }

    AZ_MATH_INLINE Vector3 Matrix3x3::GetBasisZ() const
    {
        return GetColumn(2);
    }

    AZ_MATH_INLINE void Matrix3x3::SetBasisZ(float x, float y, float z)
    {
        SetColumn(2, x, y, z);
    }

    AZ_MATH_INLINE void Matrix3x3::SetBasisZ(const Vector3& v)
    {
        SetColumn(2, v);
    }

    AZ_MATH_INLINE void Matrix3x3::GetBasis(Vector3* basisX, Vector3* basisY, Vector3* basisZ) const
    {
        GetColumns(basisX, basisY, basisZ);
    }

    AZ_MATH_INLINE void Matrix3x3::SetBasis(const Vector3& basisX, const Vector3& basisY, const Vector3& basisZ)
    {
        SetColumns(basisX, basisY, basisZ);
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::TransposedMultiply(const Matrix3x3& rhs) const
    {
        Matrix3x3 result;
        Simd::Vec3::Mat3x3TransposeMultiply(GetSimdValues(), rhs.GetSimdValues(), result.GetSimdValues());
        return result;
    }

    AZ_MATH_INLINE Vector3 Matrix3x3::operator*(const Vector3& rhs) const
    {
        return Vector3(Simd::Vec3::Mat3x3TransformVector(GetSimdValues(), rhs.GetSimdValue()));
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::operator+(const Matrix3x3& rhs) const
    {
        return Matrix3x3
        (
            Simd::Vec3::Add(m_rows[0].GetSimdValue(), rhs.m_rows[0].GetSimdValue()),
            Simd::Vec3::Add(m_rows[1].GetSimdValue(), rhs.m_rows[1].GetSimdValue()),
            Simd::Vec3::Add(m_rows[2].GetSimdValue(), rhs.m_rows[2].GetSimdValue())
        );
    }

    AZ_MATH_INLINE Matrix3x3& Matrix3x3::operator+=(const Matrix3x3& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::operator-(const Matrix3x3& rhs) const
    {
        return Matrix3x3
        (
            Simd::Vec3::Sub(m_rows[0].GetSimdValue(), rhs.m_rows[0].GetSimdValue()),
            Simd::Vec3::Sub(m_rows[1].GetSimdValue(), rhs.m_rows[1].GetSimdValue()),
            Simd::Vec3::Sub(m_rows[2].GetSimdValue(), rhs.m_rows[2].GetSimdValue())
        );
    }

    AZ_MATH_INLINE Matrix3x3& Matrix3x3::operator-=(const Matrix3x3& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::operator*(const Matrix3x3& rhs) const
    {
        Matrix3x3 result;
        Simd::Vec3::Mat3x3Multiply(GetSimdValues(), rhs.GetSimdValues(), result.GetSimdValues());
        return result;
    }

    AZ_MATH_INLINE Matrix3x3& Matrix3x3::operator*=(const Matrix3x3& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::operator*(float multiplier) const
    {
        const Simd::Vec3::FloatType mulVec = Simd::Vec3::Splat(multiplier);
        return Matrix3x3
        (
            Simd::Vec3::Mul(m_rows[0].GetSimdValue(), mulVec),
            Simd::Vec3::Mul(m_rows[1].GetSimdValue(), mulVec),
            Simd::Vec3::Mul(m_rows[2].GetSimdValue(), mulVec)
        );
    }

    AZ_MATH_INLINE Matrix3x3& Matrix3x3::operator*=(float multiplier)
    {
        *this = *this * multiplier;
        return *this;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::operator/(float divisor) const
    {
        const Simd::Vec3::FloatType divVec = Simd::Vec3::Splat(divisor);
        return Matrix3x3
        (
            Simd::Vec3::Div(m_rows[0].GetSimdValue(), divVec),
            Simd::Vec3::Div(m_rows[1].GetSimdValue(), divVec),
            Simd::Vec3::Div(m_rows[2].GetSimdValue(), divVec)
        );
    }

    AZ_MATH_INLINE Matrix3x3& Matrix3x3::operator/=(float divisor)
    {
        *this = *this / divisor;
        return *this;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::operator-() const
    {
        const Simd::Vec3::FloatType zeroVec = Simd::Vec3::ZeroFloat();
        return Matrix3x3
        (
            Simd::Vec3::Sub(zeroVec, m_rows[0].GetSimdValue()),
            Simd::Vec3::Sub(zeroVec, m_rows[1].GetSimdValue()),
            Simd::Vec3::Sub(zeroVec, m_rows[2].GetSimdValue())
        );
    }

    AZ_MATH_INLINE bool Matrix3x3::operator==(const Matrix3x3& rhs) const
    {
        return (Simd::Vec3::CmpAllEq(m_rows[0].GetSimdValue(), rhs.m_rows[0].GetSimdValue())
             && Simd::Vec3::CmpAllEq(m_rows[1].GetSimdValue(), rhs.m_rows[1].GetSimdValue())
             && Simd::Vec3::CmpAllEq(m_rows[2].GetSimdValue(), rhs.m_rows[2].GetSimdValue()));
    }

    AZ_MATH_INLINE bool Matrix3x3::operator!=(const Matrix3x3& rhs) const
    {
        return !(*this == rhs);
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::GetTranspose() const
    {
        Matrix3x3 result;
        Simd::Vec3::Mat3x3Transpose(GetSimdValues(), result.GetSimdValues());
        return result;
    }

    AZ_MATH_INLINE void Matrix3x3::Transpose()
    {
        *this = GetTranspose();
    }

    AZ_MATH_INLINE void Matrix3x3::InvertFull()
    {
        *this = GetInverseFull();
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::GetInverseFull() const
    {
        Matrix3x3 result;
        Simd::Vec3::Mat3x3Inverse(GetSimdValues(), result.GetSimdValues());
        return result;
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::GetInverseFast() const
    {
        return GetTranspose();
    }

    AZ_MATH_INLINE void Matrix3x3::InvertFast()
    {
        *this = GetInverseFast();
    }

    AZ_MATH_INLINE Vector3 Matrix3x3::RetrieveScale() const
    {
        return Vector3(GetBasisX().GetLength(), GetBasisY().GetLength(), GetBasisZ().GetLength());
    }

    AZ_MATH_INLINE Vector3 Matrix3x3::RetrieveScaleSq() const
    {
        return Vector3(GetBasisX().GetLengthSq(), GetBasisY().GetLengthSq(), GetBasisZ().GetLengthSq());
    }

    AZ_MATH_INLINE Vector3 Matrix3x3::ExtractScale()
    {
        const Vector3 x = GetBasisX();
        const Vector3 y = GetBasisY();
        const Vector3 z = GetBasisZ();
        const float lengthX = x.GetLength();
        const float lengthY = y.GetLength();
        const float lengthZ = z.GetLength();
        SetBasisX(x / lengthX);
        SetBasisY(y / lengthY);
        SetBasisZ(z / lengthZ);
        return Vector3(lengthX, lengthY, lengthZ);
    }

    AZ_MATH_INLINE void Matrix3x3::MultiplyByScale(const Vector3& scale)
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        SetBasisX(scale.GetX() * GetBasisX());
        SetBasisY(scale.GetY() * GetBasisY());
        SetBasisZ(scale.GetZ() * GetBasisZ());
#else
        Simd::Vec3::FloatType transposed[3];
        Simd::Vec3::Mat3x3Transpose(GetSimdValues(), transposed);
        transposed[0] = Simd::Vec3::Mul(transposed[0], Simd::Vec3::Splat(scale.GetX()));
        transposed[1] = Simd::Vec3::Mul(transposed[1], Simd::Vec3::Splat(scale.GetY()));
        transposed[2] = Simd::Vec3::Mul(transposed[2], Simd::Vec3::Splat(scale.GetZ()));
        Simd::Vec3::Mat3x3Transpose(transposed, GetSimdValues());
#endif
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::GetReciprocalScaled() const
    {
        Matrix3x3 result = *this;
        result.MultiplyByScale(RetrieveScaleSq().GetReciprocal());
        return result;
    }

    AZ_MATH_INLINE void Matrix3x3::GetPolarDecomposition(Matrix3x3* orthogonalOut, Matrix3x3* symmetricOut) const
    {
        *orthogonalOut = GetPolarDecomposition();
        // Could also refine symmetricOut further, by taking H = 0.5*(H + H^t)
        *symmetricOut = orthogonalOut->TransposedMultiply(*this);
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::GetOrthogonalized() const
    {
        const Simd::Vec3::FloatType row0 = Simd::Vec3::NormalizeSafe(Simd::Vec3::Cross(m_rows[1].GetSimdValue(), m_rows[2].GetSimdValue()), Constants::Tolerance);
        const Simd::Vec3::FloatType row1 = Simd::Vec3::NormalizeSafe(Simd::Vec3::Cross(m_rows[2].GetSimdValue(), row0), Constants::Tolerance);
        const Simd::Vec3::FloatType row2 = Simd::Vec3::NormalizeSafe(Simd::Vec3::Cross(row0, row1), Constants::Tolerance);
        return Matrix3x3(row0, row1, row2);
    }

    AZ_MATH_INLINE void Matrix3x3::Orthogonalize()
    {
        *this = GetOrthogonalized();
    }

    AZ_MATH_INLINE bool Matrix3x3::IsClose(const Matrix3x3& rhs, float tolerance) const
    {
        const Simd::Vec3::FloatType vecTolerance = Simd::Vec3::Splat(tolerance);
        for (int32_t row = 0; row < RowCount; ++row)
        {
            const Simd::Vec3::FloatType compare = Simd::Vec3::Abs(Simd::Vec3::Sub(m_rows[row].GetSimdValue(), rhs.m_rows[row].GetSimdValue()));
            if (!Simd::Vec3::CmpAllLt(compare, vecTolerance))
            {
                return false;
            }
        }
        return true;
    }

    AZ_MATH_INLINE Vector3 Matrix3x3::GetDiagonal() const
    {
        return Vector3(GetElement(0, 0), GetElement(1, 1), GetElement(2, 2));
    }

    AZ_MATH_INLINE float Matrix3x3::GetDeterminant() const
    {
        return m_rows[0].GetElement(0) * (m_rows[1].GetElement(1) * m_rows[2].GetElement(2) - m_rows[1].GetElement(2) * m_rows[2].GetElement(1))
             + m_rows[1].GetElement(0) * (m_rows[2].GetElement(1) * m_rows[0].GetElement(2) - m_rows[2].GetElement(2) * m_rows[0].GetElement(1))
             + m_rows[2].GetElement(0) * (m_rows[0].GetElement(1) * m_rows[1].GetElement(2) - m_rows[0].GetElement(2) * m_rows[1].GetElement(1));
    }

    AZ_MATH_INLINE Matrix3x3 Matrix3x3::GetAdjugate() const
    {
        Matrix3x3 result;
        Simd::Vec3::Mat3x3Adjugate(GetSimdValues(), result.GetSimdValues());
        return result;
    }

    AZ_MATH_INLINE bool Matrix3x3::IsFinite() const
    {
        return GetRow(0).IsFinite() && GetRow(1).IsFinite() && GetRow(2).IsFinite();
    }

    AZ_MATH_INLINE const Simd::Vec3::FloatType* Matrix3x3::GetSimdValues() const
    {
        return reinterpret_cast<const Simd::Vec3::FloatType*>(m_rows);
    }

    AZ_MATH_INLINE Simd::Vec3::FloatType* Matrix3x3::GetSimdValues()
    {
        return reinterpret_cast<Simd::Vec3::FloatType*>(m_rows);
    }

    AZ_MATH_INLINE Vector3 operator*(const Vector3& lhs, const Matrix3x3& rhs)
    {
        return Vector3(lhs(0) * rhs(0, 0) + lhs(1) * rhs(1, 0) + lhs(2) * rhs(2, 0)
                     , lhs(0) * rhs(0, 1) + lhs(1) * rhs(1, 1) + lhs(2) * rhs(2, 1)
                     , lhs(0) * rhs(0, 2) + lhs(1) * rhs(1, 2) + lhs(2) * rhs(2, 2));
    }

    AZ_MATH_INLINE Vector3& operator*=(Vector3& lhs, const Matrix3x3& rhs)
    {
        lhs = lhs * rhs;
        return lhs;
    }

    AZ_MATH_INLINE Matrix3x3 operator*(float lhs, const Matrix3x3& rhs)
    {
        return rhs * lhs;
    }
} // namespace AZ
