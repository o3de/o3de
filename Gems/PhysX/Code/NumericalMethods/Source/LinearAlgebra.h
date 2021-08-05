/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>

// This provides just the functionality for arbitrary dimension matrices and vectors that is required by this gem.
// It is not intended to be a complete or optimized implementation.
// If we add support for arbitrary dimension matrices or use a third party library, that should replace what is here.

namespace NumericalMethods
{
    using ScalarVariable = double;

    //! Class for arbitrary sized vectors, providing only the functionality required by the numerical methods supported
    //! in this gem.
    class VectorVariable
    {
    public:
        VectorVariable() = default;
        explicit VectorVariable(AZ::u32 dimension);
        static VectorVariable CreateFromVector(const AZStd::vector<double>& values);
        AZ::u32 GetDimension() const;
        double& operator[](AZ::u32 index);
        double operator[](AZ::u32 index) const;
        VectorVariable operator+(const VectorVariable& rhs) const;
        VectorVariable operator+=(const VectorVariable& rhs);
        VectorVariable operator-() const;
        VectorVariable operator-(const VectorVariable& rhs) const;
        VectorVariable operator-=(const VectorVariable& rhs);
        VectorVariable operator*(const double rhs) const;
        double Norm() const;
        double Dot(const VectorVariable& rhs) const;
        const AZStd::vector<double>& GetValues() const;
    private:
        AZStd::vector<double> m_values;
    };

    VectorVariable operator*(const double lhs, const VectorVariable& rhs);

    //! Class for arbitrary sized matrices, providing only the functionality required by the numerical methods supported
    //! in this gem.
    class MatrixVariable
    {
    public:
        MatrixVariable() = default;
        MatrixVariable(AZ::u32 numRows, AZ::u32 numColumns);
        double& Element(AZ::u32 row, AZ::u32 column);
        double Element(AZ::u32 row, AZ::u32 column) const;
        AZ::u32 GetNumRows() const;
        AZ::u32 GetNumColumns() const;
        MatrixVariable operator+(const MatrixVariable& rhs) const;
        MatrixVariable operator+=(const MatrixVariable& rhs);
        MatrixVariable operator-(const MatrixVariable& rhs) const;
        MatrixVariable operator/(const double divisor) const;
    private:
        AZStd::vector<double> m_values;
        AZ::u32 m_numRows = 0;
        AZ::u32 m_numColumns = 0;
    };

    VectorVariable operator*(const MatrixVariable& lhs, const VectorVariable& rhs);
    MatrixVariable operator*(const MatrixVariable& lhs, const MatrixVariable& rhs);
    MatrixVariable operator*(double lhs, const MatrixVariable& rhs);
    MatrixVariable OuterProduct(const VectorVariable& x, const VectorVariable& y);
} // namespace NumericalMethods
