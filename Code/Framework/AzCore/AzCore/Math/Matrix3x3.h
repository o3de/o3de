/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace AZ
{
    class Matrix3x3;
    class Matrix3x4;
    class Matrix4x4;
    class Transform;
    class ReflectContext;

    //! Matrix with 3 rows and 3 columns.
    //! See Matrix4x4 for general information about matrices.
    class Matrix3x3
    {
    public:

        AZ_TYPE_INFO(Matrix3x3, "{15A4332F-7C3F-4A58-AC35-50E1CE53FB9C}");

        static constexpr int32_t RowCount = 3;
        static constexpr int32_t ColCount = 3;

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        //! Default constructor does not initialize the matrix.
        Matrix3x3() = default;
        Matrix3x3(const Matrix3x3& rhs) = default;

        Matrix3x3(Simd::Vec3::FloatArgType row0, Simd::Vec3::FloatArgType row1, Simd::Vec3::FloatArgType row2);

        explicit Matrix3x3(const Quaternion& quaternion);

        static Matrix3x3 CreateIdentity();
        static Matrix3x3 CreateZero();

        //! Constructs a matrix with all components set to the specified value.
        static Matrix3x3 CreateFromValue(float value);

        //! Constructs from an array of 9 floats stored in row-major order.
        static Matrix3x3 CreateFromRowMajorFloat9(const float* values);
        static Matrix3x3 CreateFromRows(const Vector3& row0, const Vector3& row1, const Vector3& row2);

        //! Constructs from an array of 9 floats stored in column-major order.
        static Matrix3x3 CreateFromColumnMajorFloat9(const float* values);
        static Matrix3x3 CreateFromColumns(const Vector3& col0, const Vector3& col1, const Vector3& col2);

        //! Sets the matrix to be a rotation around a specified axis.
        //! @{
        static Matrix3x3 CreateRotationX(float angle);
        static Matrix3x3 CreateRotationY(float angle);
        static Matrix3x3 CreateRotationZ(float angle);
        //! @}

        //! Sets the matrix from the left 3x3 sub-matrix of a Matrix3x4
        static Matrix3x3 CreateFromMatrix3x4(const Matrix3x4& m);

        //! Sets the matrix from the upper 3x3 sub-matrix of a Matrix4x4.
        static Matrix3x3 CreateFromMatrix4x4(const Matrix4x4& m);

        //! Creates a matrix using the scale and orientation portions of a Transform.
        static Matrix3x3 CreateFromTransform(const Transform& t);

        //! Sets the matrix from a quaternion.
        static Matrix3x3 CreateFromQuaternion(const Quaternion& q);

        //! Sets the matrix to be a scale matrix.
        static Matrix3x3 CreateScale(const Vector3& scale);

        //! Sets the matrix to be a diagonal matrix.
        static Matrix3x3 CreateDiagonal(const Vector3& diagonal);

        //! Creates the skew-symmetric cross product matrix, i.e. M*a=p.Cross(a).
        static Matrix3x3 CreateCrossProduct(const Vector3& p);

        //! Stores the matrix into to an array of 9 floats.
        //! The floats need only be 4 byte aligned, 16 byte alignment is not required.
        void StoreToRowMajorFloat9(float* values) const;

        //! Stores the matrix into to an array of 2 float4 and 1 float3.
        //! Useful for GPU constant buffer packing rules, which pack a float3x3 as such:
        //!    [0, 0], [0, 1], [0, 2], <padding>,
        //!    [1, 0], [1, 1], [1, 2], <padding>,
        //!    [2, 0], [2, 1], [2, 2]
        //! Note that the last row is stored in 3 floats, not 4.
        void StoreToRowMajorFloat11(float* values) const;

        //! Stores the matrix into to an array of 2 float4 and 1 float3.
        //! Useful for GPU constant buffer packing rules, which pack a float3x3 as such:
        //!    [0, 0], [1, 0], [2, 0], <padding>,
        //!    [0, 1], [1, 1], [2, 1], <padding>,
        //!    [0, 2], [1, 2], [2, 2]
        //! Note that the last row is stored in 3 floats, not 4.
        void StoreToColumnMajorFloat11(float* values) const;

        //! Stores the matrix into to an array of 9 floats.
        //! The floats need only be 4 byte aligned, 16 byte alignment is not required.
        void StoreToColumnMajorFloat9(float* values) const;

        //! Indexed accessor functions.
        //! @{
        float GetElement(int32_t row, int32_t col) const;
        void SetElement(int32_t row, int32_t col, float value);
        //! @}

        //! Indexed access using operator().
        float operator()(int32_t row, int32_t col) const;

        //! Row access functions.
        //! @{
        Vector3 GetRow(int32_t row) const;
        void SetRow(int32_t row, float x, float y, float z);
        void SetRow(int32_t row, const Vector3& v);
        void GetRows(Vector3* row0, Vector3* row1, Vector3* row2) const;
        void SetRows(const Vector3& row0, const Vector3& row1, const Vector3& row2);
        //! @}

        //! Column access functions.
        //! @{
        Vector3 GetColumn(int32_t col) const;
        void SetColumn(int32_t col, float x, float y, float z);
        void SetColumn(int32_t col, const Vector3& v);
        void GetColumns(Vector3* col0, Vector3* col1, Vector3* col2) const;
        void SetColumns(const Vector3& col0, const Vector3& col1, const Vector3& col2);
        //! @}

        //! Basis (column) access functions.
        //! @{
        Vector3 GetBasisX() const;
        void SetBasisX(float x, float y, float z);
        void SetBasisX(const Vector3& v);
        Vector3 GetBasisY() const;
        void SetBasisY(float x, float y, float z);
        void SetBasisY(const Vector3& v);
        Vector3 GetBasisZ() const;
        void SetBasisZ(float x, float y, float z);
        void SetBasisZ(const Vector3& v);
        void GetBasis(Vector3* basisX, Vector3* basisY, Vector3* basisZ) const;
        void SetBasis(const Vector3& basisX, const Vector3& basisY, const Vector3& basisZ);
        //! @}

        //! Calculates (this->GetTranspose() * rhs).
        Matrix3x3 TransposedMultiply(const Matrix3x3& rhs) const;

        //! Post-multiplies the matrix by a vector.
        Vector3 operator*(const Vector3& rhs) const;

        //! Operator for matrix-matrix addition.
        //! @{
        [[nodiscard]] Matrix3x3 operator+(const Matrix3x3& rhs) const;
        Matrix3x3& operator+=(const Matrix3x3& rhs);
        //! @}

        //! Operator for matrix-matrix substraction.
        //! @{
        [[nodiscard]] Matrix3x3 operator-(const Matrix3x3& rhs) const;
        Matrix3x3& operator-=(const Matrix3x3& rhs);
        //! @}

        //! Operator for matrix-matrix multiplication.
        //! @{
        [[nodiscard]] Matrix3x3 operator*(const Matrix3x3& rhs) const;
        Matrix3x3& operator*=(const Matrix3x3& rhs);
        //! @}

        //! Operator for multiplying all matrix's elements with a scalar
        //! @{
        [[nodiscard]] Matrix3x3 operator*(float multiplier) const;
        Matrix3x3& operator*=(float multiplier);
        //! @}

        //! Operator for dividing all matrix's elements with a scalar
        //! @{
        [[nodiscard]] Matrix3x3 operator/(float divisor) const;
        Matrix3x3& operator/=(float divisor);
        //! @}

        //! Operator for negating all matrix's elements
        [[nodiscard]] Matrix3x3 operator-() const;

        bool operator==(const Matrix3x3& rhs) const;
        bool operator!=(const Matrix3x3& rhs) const;

        //! Transpose calculation, flips the rows and columns.
        //! @{
        Matrix3x3 GetTranspose() const;
        void Transpose();
        //! @}

        //! Gets the inverse of the matrix.
        //! Use GetInverseFast instead of this if the matrix is orthogonal.
        //! @{
        Matrix3x3 GetInverseFull() const;
        void InvertFull();
        //! @}

        //! Fast inversion assumes the matrix is orthogonal.
        //! @{
        Matrix3x3 GetInverseFast() const;
        void InvertFast();
        //! @}

        //! Gets the scale part of the transformation, i.e. the length of the scale components.
        [[nodiscard]] Vector3 RetrieveScale() const;

        //! Gets the squared scale part of the transformation (the squared length of the basis vectors).
        [[nodiscard]] Vector3 RetrieveScaleSq() const;

        //! Gets the scale part of the transformation as in RetrieveScale, and also removes this scaling from the matrix.
        Vector3 ExtractScale();

        //! Quick multiplication by a scale matrix, equivalent to m*=Matrix3x3::CreateScale(scale).
        void MultiplyByScale(const Vector3& scale);

        //! Returns a matrix with the reciprocal scale, keeping the same rotation and translation.
        [[nodiscard]] Matrix3x3 GetReciprocalScaled() const;

        //! Polar decomposition, M=U*H, U is orthogonal (unitary) and H is symmetric (hermitian).
        //! This function returns the orthogonal part only
        Matrix3x3 GetPolarDecomposition() const;

        //! Polar decomposition, M=U*H, U is orthogonal (unitary) and H is symmetric (hermitian).
        void GetPolarDecomposition(Matrix3x3* orthogonalOut, Matrix3x3* symmetricOut) const;

        bool IsOrthogonal(float tolerance = Constants::Tolerance) const;

        //! Adjusts an almost orthogonal matrix to be orthogonal.
        Matrix3x3 GetOrthogonalized() const;

        //! Adjusts an almost orthogonal matrix to be orthogonal.
        void Orthogonalize();

        bool IsClose(const Matrix3x3& rhs, float tolerance = Constants::Tolerance) const;

        void SetRotationPartFromQuaternion(const Quaternion& q);

        Vector3 GetDiagonal() const;

        float GetDeterminant() const;

        //! This is the transpose of the matrix of cofactors.
        //! Also known as the adjoint, adjugate is the modern name which avoids confusion with the adjoint conjugate transpose.
        Matrix3x3 GetAdjugate() const;

        bool IsFinite() const;

        const Simd::Vec3::FloatType* GetSimdValues() const;

        Simd::Vec3::FloatType* GetSimdValues();

    private:

        Vector3 m_rows[RowCount];
    };

    //! Pre-multiplies the matrix by a vector.
    //! Note that this is not the usual multiplication order for transformations.
    Vector3 operator*(const Vector3& lhs, const Matrix3x3& rhs);

    //! Pre-multiplies the matrix by a vector in-place.
    //! Note that this is not the usual multiplication order for transformations.
    Vector3& operator*=(Vector3& lhs, const Matrix3x3& rhs);

    //! Pre-multiplies the matrix by a scalar.
    Matrix3x3 operator*(float lhs, const Matrix3x3& rhs);

} // namespace AZ

#include <AzCore/Math/Matrix3x3.inl>
