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

    //! The general matrix class.
    //! Stores all 4 rows and 4 columns, so can be used for all types of transformations.
    //! If you don't need perspective, consider using Transform.
    //! if you don't need translation, consider using Matrix3x3.
    //!
    //! When multiplying with a Vector3, it assumes the w-component of the Vector3 is 1.0.
    //! Use the Multiply3x3 functions to multiply by the upper 3x3 submatrix only, e.g. for transforming normals.
    class Matrix4x4
    {
    public:

        AZ_TYPE_INFO(Matrix4x4, "{157193C7-B673-4A2B-8B43-5681DCC3DEC3}");

        static constexpr int32_t RowCount = 4;
        static constexpr int32_t ColCount = 4;

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        //! Default constructor does not initialize the matrix.
        Matrix4x4() = default;
        Matrix4x4(const Matrix4x4& rhs) = default;

        Matrix4x4(Simd::Vec4::FloatArgType row0, Simd::Vec4::FloatArgType row1, Simd::Vec4::FloatArgType row2, Simd::Vec4::FloatArgType row3);

        static Matrix4x4 CreateIdentity();
        static Matrix4x4 CreateZero();

        //! Constructs a matrix with all components set to the specified value.
        static Matrix4x4 CreateFromValue(float value);

        //! Constructs from an array of 16 floats stored in row-major order.
        static Matrix4x4 CreateFromRowMajorFloat16(const float* values);
        static Matrix4x4 CreateFromRows(const Vector4& row0, const Vector4& row1, const Vector4& row2, const Vector4& row3);

        //! Constructs from an array of 16 floats stored in column-major order.
        static Matrix4x4 CreateFromColumnMajorFloat16(const float* values);
        static Matrix4x4 CreateFromColumns(const Vector4& col0, const Vector4& col1, const Vector4& col2, const Vector4& col3);

        //! Sets the matrix to be a rotation around a specified axis.
        //! @{
        static Matrix4x4 CreateRotationX(float angle);
        static Matrix4x4 CreateRotationY(float angle);
        static Matrix4x4 CreateRotationZ(float angle);
        //! @}

        //! Sets the matrix from a quaternion, translation is set to zero.
        static Matrix4x4 CreateFromQuaternion(const Quaternion& q);

        //! Sets the matrix from a quaternion and a translation.
        static Matrix4x4 CreateFromQuaternionAndTranslation(const Quaternion& q, const Vector3& p);

        //! Creates a Matrix4x4 from a Matrix3x4, setting the bottom row to (0, 0, 0, 1).
        static Matrix4x4 CreateFromMatrix3x4(const Matrix3x4& matrix3x4);

        //! Creates a Matrix3x4 from a Transform, setting the bottom row to (0, 0, 0, 1).
        static Matrix4x4 CreateFromTransform(const Transform& transform);

        //! Sets the matrix to be a scale matrix.
        static Matrix4x4 CreateScale(const Vector3& scale);

        //! Sets the matrix to be a diagonal matrix.
        static Matrix4x4 CreateDiagonal(const Vector4& diagonal);

        //! Sets the matrix to be a translation matrix.
        static Matrix4x4 CreateTranslation(const Vector3& translation);

        //! Creates a projection matrix using the vertical field of view and the aspect ratio.
        static Matrix4x4 CreateProjection(float fovY, float aspectRatio, float nearDist, float farDist);

        //! Creates a projection matrix using the vertical and horizontal field of view.
        //! Note that the relationship between field of view and aspect ratio is not linear, so prefer CreateProjection().
        static Matrix4x4 CreateProjectionFov(float fovX, float fovY, float nearDist, float farDist);

        //! Creates an off-center projection matrix.
        static Matrix4x4 CreateProjectionOffset(float left, float right, float bottom, float top, float nearDist, float farDist);

        //! Interpolates between two matrices; linearly for scale/translation, spherically for rotation.
        static Matrix4x4 CreateInterpolated(const Matrix4x4& m1, const Matrix4x4& m2, float t);

        //! Stores the matrix into to an array of 16 floats.
        //! The floats need only be 4 byte aligned, 16 byte alignment is not required.
        void StoreToRowMajorFloat16(float* values) const;

        //! Stores the matrix into to an array of 16 floats.
        //! The floats need only be 4 byte aligned, 16 byte alignment is not required.
        void StoreToColumnMajorFloat16(float* values) const;

        //! Indexed accessor functions.
        //! @{
        float GetElement(int32_t row, int32_t col) const;
        void SetElement(int32_t row, int32_t col, float value);
        //! @}

        //! Indexed access using operator().
        float operator()(int32_t row, int32_t col) const;

        //! Row access functions.
        //! @{
        Vector4 GetRow(int32_t row) const;
        Vector3 GetRowAsVector3(int32_t row) const;
        void GetRows(Vector4* row0, Vector4* row1, Vector4* row2, Vector4* row3) const;
        void SetRow(int32_t row, float x, float y, float z, float w);
        void SetRow(int32_t row, const Vector3& v);
        void SetRow(int32_t row, const Vector3& v, float w);
        void SetRow(int32_t row, const Vector4& v);
        void SetRows(const Vector4& row0, const Vector4& row1, const Vector4& row2, const Vector4& row3);
        //! @}

        //! Column access functions.
        //! @{
        Vector4 GetColumn(int32_t col) const;
        Vector3 GetColumnAsVector3(int32_t col) const;
        void GetColumns(Vector4* col0, Vector4* col1, Vector4* col2, Vector4* col3) const;
        void SetColumn(int32_t col, float x, float y, float z, float w);
        void SetColumn(int32_t col, const Vector3& v);
        void SetColumn(int32_t col, const Vector3& v, float w);
        void SetColumn(int32_t col, const Vector4& v);
        void SetColumns(const Vector4& col0, const Vector4& col1, const Vector4& col2, const Vector4& col3);
        //! @}

        //! Basis (column) access functions.
        //! @{
        Vector4 GetBasisX() const;
        Vector3 GetBasisXAsVector3() const;
        void SetBasisX(float x, float y, float z, float w);
        void SetBasisX(const Vector4& v);
        Vector4 GetBasisY() const;
        Vector3 GetBasisYAsVector3() const;
        void SetBasisY(float x, float y, float z, float w);
        void SetBasisY(const Vector4& v);
        Vector4 GetBasisZ() const;
        Vector3 GetBasisZAsVector3() const;
        void SetBasisZ(float x, float y, float z, float w);
        void SetBasisZ(const Vector4& v);
        void GetBasisAndTranslation(Vector4* basisX, Vector4* basisY, Vector4* basisZ, Vector4* pos) const;
        void SetBasisAndTranslation(const Vector4& basisX, const Vector4& basisY, const Vector4& basisZ, const Vector4& pos);
        //! @}

        //! Position (last column) access functions.
        //! @{
        Vector3 GetTranslation() const;
        void SetTranslation(float x, float y, float z);
        void SetTranslation(const Vector3& v);
        //! @}

        //! Operator for matrix-matrix addition.
        //! @{
        [[nodiscard]] Matrix4x4 operator+(const Matrix4x4& rhs) const;
        Matrix4x4& operator+=(const Matrix4x4& rhs);
        //! @}

        //! Operator for matrix-matrix substraction.
        //! @{
        [[nodiscard]] Matrix4x4 operator-(const Matrix4x4& rhs) const;
        Matrix4x4& operator-=(const Matrix4x4& rhs);
        //! @}

        //! Operator for matrix-matrix multiplication.
        //! @{
        [[nodiscard]] Matrix4x4 operator*(const Matrix4x4& rhs) const;
        Matrix4x4& operator*=(const Matrix4x4& rhs);
        //! @}

        //! Operator for multiplying all matrix's elements with a scalar
        //! @{
        [[nodiscard]] Matrix4x4 operator*(float multiplier) const;
        Matrix4x4& operator*=(float multiplier);
        //! @}

        //! Operator for dividing all matrix's elements with a scalar
        //! @{
        [[nodiscard]] Matrix4x4 operator/(float divisor) const;
        Matrix4x4& operator/=(float divisor);
        //! @}

        //! Operator for negating all matrix's elements
        [[nodiscard]] Matrix4x4 operator-() const;

        //! Post-multiplies the matrix by a vector.
        //! Assumes that the w-component of the Vector3 is 1.0.
        Vector3 operator*(const Vector3& rhs) const;

        //! Post-multiplies the matrix by a vector.
        Vector4 operator*(const Vector4& rhs) const;

        //! Pre-multiplies the matrix by a vector, using only the upper 3x3 submatrix.
        //! Note that this is not the usual multiplication order for transformations.
        Vector3 TransposedMultiply3x3(const Vector3& v) const;

        //! Post-multiplies the matrix by a vector, using only the upper 3x3 submatrix.
        Vector3 Multiply3x3(const Vector3& v) const;

        //! Transpose operations.
        //! @{
        Matrix4x4 GetTranspose() const;
        void Transpose();
        //! @}

        //! Performs a full inversion for an arbitrary 4x4 matrix.
        //! Using GetInverseTransform or GetFastInverse will often be possible, use them in preference to this.
        //! @{
        Matrix4x4 GetInverseFull() const;
        void  InvertFull();
        //! @}

        //! Gets the inverse of the matrix.
        //! Assumes that the last row is (0,0,0,1), use GetInverseFull if this is not true.
        //! @{
        Matrix4x4 GetInverseTransform() const;
        void InvertTransform();
        //! @}

        //! Fast inversion.
        //! Assumes the matrix consists of an upper 3x3 orthogonal matrix (i.e. a rotation) and a translation in the last column.
        //! @{
        Matrix4x4 GetInverseFast() const;
        void InvertFast();
        //! @}

        //! Gets the scale part of the transformation, i.e. the length of the scale components.
        [[nodiscard]] Vector3 RetrieveScale() const;

        //! Gets the squared scale part of the transformation (the squared length of the basis vectors).
        [[nodiscard]] Vector3 RetrieveScaleSq() const;

        //! Gets the scale part of the transformation as in RetrieveScale, and also removes this scaling from the matrix.
        Vector3 ExtractScale();

        //! Quick multiplication by a scale matrix, equivalent to m*=Matrix4x4::CreateScale(scale).
        void MultiplyByScale(const Vector3& scale);

        //! Returns a matrix with the reciprocal scale, keeping the same rotation and translation.
        [[nodiscard]] Matrix4x4 GetReciprocalScaled() const;

        bool IsClose(const Matrix4x4& rhs, float tolerance = Constants::Tolerance) const;

        bool operator==(const Matrix4x4& rhs) const;
        bool operator!=(const Matrix4x4& rhs) const;

        //! sets the upper 3x3 rotation part of the matrix from a quaternion.
        void SetRotationPartFromQuaternion(const Quaternion& q);

        Vector4 GetDiagonal() const;

        bool IsFinite() const;

        const Simd::Vec4::FloatType* GetSimdValues() const;

        Simd::Vec4::FloatType* GetSimdValues();

    private:

        static Matrix4x4 CreateProjectionInternal(float cotX, float cotY, float nearDist, float farDist);

        Vector4 m_rows[RowCount];
    };

    //! Pre-multiplies the matrix by a vector. Assumes that the w-component of the Vector3 is 1.0, and returns the result as
    //! a Vector3, without calculating the result w-component. Use the Vector4 version of this function to get the full result.
    //! Note that this is not the usual multiplication order for transformations.
    const Vector3 operator*(const Vector3& lhs, const Matrix4x4& rhs);

    //! Pre-multiplies the matrix by a vector in-place. Assumes that the w-component of the Vector3 is 1.0, and returns the result as
    //! a Vector3, without calculating the result w-component. Use the Vector4 version of this function to get the full result.
    //! Note that this is not the usual multiplication order for transformations.
    Vector3& operator*=(Vector3& lhs, const Matrix4x4& rhs);

    //! Pre-multiplies the matrix by a vector.
    //! Note that this is not the usual multiplication order for transformations.
    const Vector4 operator*(const Vector4& lhs, const Matrix4x4& rhs);

    //! Pre-multiplies the matrix by a vector in-place.
    //! Note that this is not the usual multiplication order for transformations.
    Vector4& operator*=(Vector4& lhs, const Matrix4x4& rhs);

    //! Pre-multiplies the matrix by a scalar.
    Matrix4x4 operator*(float lhs, const Matrix4x4& rhs);

} // namespace AZ

#include <AzCore/Math/Matrix4x4.inl>
