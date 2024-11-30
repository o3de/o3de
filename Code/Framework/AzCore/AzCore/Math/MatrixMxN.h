/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/VectorN.h>
#include <AzCore/Math/Matrix4x4.h>

namespace AZ
{
    class ReflectContext;

    //! Matrix with ROW_COUNT rows and COL_COUNT columns.
    class MatrixMxN final
    {
    public:

        AZ_TYPE_INFO(MatrixMxN, "{B751D885-87D0-40BF-A6B3-48EFF0147AFA}");

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        //! Default constructor for reflection and serialization support.
        MatrixMxN() = default;

        //! Default constructor does not initialize the matrix.
        MatrixMxN(AZStd::size_t rowCount, AZStd::size_t colCount);

        //! Constructs a matrix with all components set to the specified value.
        MatrixMxN(AZStd::size_t rowCount, AZStd::size_t colCount, float value);

        MatrixMxN(const MatrixMxN& rhs) = default;

        MatrixMxN(MatrixMxN&& rhs);

        ~MatrixMxN() = default;

        MatrixMxN& operator=(MatrixMxN&&) = default;

        MatrixMxN& operator=(const MatrixMxN&) = default;

        //! Creates an M by N matrix with all elements set to zero.
        static MatrixMxN CreateZero(AZStd::size_t rowCount, AZStd::size_t colCount);

        //! Creates an M by N matrix and loads the initial values from the packed float array.
        //! This assumes no padding of input elements to satisfy the 4x4 blocked matrix structure.
        static MatrixMxN CreateFromPackedFloats(AZStd::size_t rowCount, AZStd::size_t colCount, const float* inputs);

        //! Creates an M by N matrix with all elements set to random numbers in the range [0, 1).
        static MatrixMxN CreateRandom(AZStd::size_t rowCount, AZStd::size_t colCount);

        //! Returns the number of rows in the matrix.
        AZStd::size_t GetRowCount() const;

        //! Returns the number of columns in the matrix.
        AZStd::size_t GetColumnCount() const;

        //! Resizes the matrix to the provided row and column count.
        void Resize(AZStd::size_t rowCount, AZStd::size_t colCount);

        //! Indexed accessor functions.
        //! @{
        float GetElement(AZStd::size_t row, AZStd::size_t col) const;
        void SetElement(AZStd::size_t row, AZStd::size_t col, float value);
        //! @}

        //! Indexed access using operator().
        float operator()(AZStd::size_t row, AZStd::size_t col) const;

        //! Returns the transpose of the matrix.
        MatrixMxN GetTranspose() const;

        //! Floor/Ceil/Round functions, operate on each component individually, result will be a new MatrixMxN.
        //! @{
        MatrixMxN GetFloor() const;
        MatrixMxN GetCeil() const;
        MatrixMxN GetRound() const; // Ties to even (banker's rounding)
        //! @}

        //! Min/Max functions, operate on each component individually, result will be a new MatrixMxN.
        //! @{
        MatrixMxN GetMin(const MatrixMxN& m) const;
        MatrixMxN GetMax(const MatrixMxN& m) const;
        MatrixMxN GetClamp(const MatrixMxN& min, const MatrixMxN& max) const;
        //! @}

        //! Returns a new MatrixMxN containing the absolute value of all elements in the source MatrixMxN.
        MatrixMxN GetAbs() const;

        //! Returns a new MatrixMxN containing the square of all elements in the source MatrixMxN.
        MatrixMxN GetSquare() const;

        //! Quickly zeros all elements of the matrix to create a zero matrix.
        void SetZero();

        //! Aside from multiplication, these operators perform basic arithmetic on the individual elements within the respective matrices.
        //! @{
        MatrixMxN operator-() const;
        MatrixMxN operator+(const MatrixMxN& rhs) const;
        MatrixMxN operator-(const MatrixMxN& rhs) const;
        MatrixMxN operator*(const MatrixMxN& rhs) const;
        MatrixMxN operator*(float multiplier) const;
        MatrixMxN operator/(float divisor) const;

        MatrixMxN& operator+=(const MatrixMxN& rhs);
        MatrixMxN& operator-=(const MatrixMxN& rhs);

        MatrixMxN& operator+=(float sum);
        MatrixMxN& operator-=(float difference);
        MatrixMxN& operator*=(float multiplier);
        MatrixMxN& operator/=(float divisor);
        //! @}

        //! Returns the number blocks of 4x4 submatrices that span all the rows of the matrix.
        AZStd::size_t GetRowGroups() const;

        //! Returns the number blocks of 4x4 submatrices that span all the columns of the matrix.
        AZStd::size_t GetColumnGroups() const;

        //! Gets and sets individual 4x4 submatrices within the larger matrix.
        //! @{
        const Matrix4x4& GetSubmatrix(AZStd::size_t rowGroup, AZStd::size_t colGroup) const;
        Matrix4x4& GetSubmatrix(AZStd::size_t rowGroup, AZStd::size_t colGroup);
        void SetSubmatrix(AZStd::size_t rowGroup, AZStd::size_t colGroup, const Matrix4x4& subMatrix);
        AZStd::vector<Matrix4x4>& GetMatrixElements();
        //! @}

        //! Zeros out unused components of any submatrices
        void FixUnusedElements();

    private:

        //! Updates the matrix internals to reflect the current row and column counts.
        void OnSizeChanged();

        // Note that we compose the larger matrix out of a set of smaller 4x4 submatrices (blocked matrix)
        // Those blocks of 4x4 submatrices are arranged in a column-priority layout
        // This is done so we can efficiently perform operations like transpose, vector, and matrix multiplication entirely in simd
        // Within each of those submatrices, the elements are laid out in column major ordering
        // This allows us to perform vector-matrix multiplication in a highly efficient manner
        // The final result means the values of the larger matrix are actually laid out in a tiled manner in memory
        AZStd::size_t m_rowCount = 0;
        AZStd::size_t m_colCount = 0;
        AZStd::size_t m_numRowGroups = 0; // (RowCount + 3) / 4
        AZStd::size_t m_numColGroups = 0; // (ColCount + 3) / 4
        AZStd::vector<Matrix4x4> m_values;
    };

    //! Computes the outer product of two vectors to produce an MxN matrix.
    //! The dimensionality of the resulting matrix will be M = lhs.dimensionality, N = rhs.dimensionality
    void OuterProduct(const VectorN& lhs, const VectorN& rhs, MatrixMxN& output);

    //! Multiplies the input vector of dimensionality RowCount, with the current matrix and stores the result in the provided output vector of dimensionality ColCount.
    void VectorMatrixMultiply(const MatrixMxN& matrix, const VectorN& vector, VectorN& output);

    //! Left-multiplies the input vector of dimensionality ColCount, with the current matrix and stores the result in the provided output vector of dimensionality RowCount.
    void VectorMatrixMultiplyLeft(const VectorN& vector, const MatrixMxN& matrix, VectorN& output);

    //! Multiplies the two input matrices to produce the output matrix.
    //! The column count of the right-hand side matrix must match the row count of the left-hand side matrix
    //! The output matrix must have dimensionality rhs.rowCount x lhs.colCount
    void MatrixMatrixMultiply(const MatrixMxN& lhs, const MatrixMxN& rhs, MatrixMxN& output);
}

#include <AzCore/Math/MatrixMxN.inl>
