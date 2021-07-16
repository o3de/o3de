/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LinearAlgebra.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/MathUtils.h>

namespace NumericalMethods
{
    VectorVariable::VectorVariable(AZ::u32 dimension)
    {
        m_values.resize(dimension, 0.0);
    }

    VectorVariable VectorVariable::CreateFromVector(const AZStd::vector<double>& values)
    {
        VectorVariable result;
        result.m_values = values;
        return result;
    }

    AZ::u32 VectorVariable::GetDimension() const
    {
        return static_cast<AZ::u32>(m_values.size());
    }

    double& VectorVariable::operator[](AZ::u32 index)
    {
        AZ_Assert(index < m_values.size(), "Invalid VectorVariable index.");
        return m_values[index];
    }

    double VectorVariable::operator[](AZ::u32 index) const
    {
        AZ_Assert(index < m_values.size(), "Invalid VectorVariable index.");
        return m_values[index];
    }

    VectorVariable VectorVariable::operator+(const VectorVariable& rhs) const
    {
        const AZ::u32 dimension = GetDimension();
        AZ_Assert(dimension == rhs.GetDimension(), "VectorVariable dimensions do not match.");
        VectorVariable result(dimension);
        for (AZ::u32 i = 0; i < dimension; i++)
        {
            result.m_values[i] = m_values[i] + rhs[i];
        }
        return result;
    }

    VectorVariable VectorVariable::operator+=(const VectorVariable& rhs)
    {
        const AZ::u32 dimension = GetDimension();
        AZ_Assert(dimension == rhs.GetDimension(), "VectorVariable dimensions do not match.");
        for (AZ::u32 i = 0; i < dimension; i++)
        {
            m_values[i] += rhs[i];
        }
        return *this;
    }

    VectorVariable VectorVariable::operator-() const
    {
        const AZ::u32 dimension = GetDimension();
        VectorVariable result(dimension);
        for (AZ::u32 i = 0; i < dimension; i++)
        {
            result[i] = -m_values[i];
        }
        return result;
    }

    VectorVariable VectorVariable::operator-(const VectorVariable& rhs) const
    {
        const AZ::u32 dimension = GetDimension();
        AZ_Assert(dimension == rhs.GetDimension(), "VectorVariable dimensions do not match.");
        VectorVariable result(dimension);
        for (AZ::u32 i = 0; i < dimension; i++)
        {
            result[i] = m_values[i] - rhs[i];
        }
        return result;
    }

    VectorVariable VectorVariable::operator-=(const VectorVariable& rhs)
    {
        const AZ::u32 dimension = GetDimension();
        AZ_Assert(dimension == rhs.GetDimension(), "VectorVariable dimensions do not match.");
        for (AZ::u32 i = 0; i < dimension; i++)
        {
            m_values[i] -= rhs[i];
        }
        return *this;
    }

    VectorVariable VectorVariable::operator*(const double rhs) const
    {
        const AZ::u32 dimension = GetDimension();
        VectorVariable result(dimension);
        for (AZ::u32 i = 0; i < dimension; i++)
        {
            result[i] = m_values[i] * rhs;
        }
        return result;
    }

    double VectorVariable::Norm() const
    {
        const AZ::u32 dimension = GetDimension();
        double sumSquares = 0.0;
        for (AZ::u32 i = 0; i < dimension; i++)
        {
            sumSquares += m_values[i] * m_values[i];
        }
        return sqrt(sumSquares);
    }

    double VectorVariable::Dot(const VectorVariable& rhs) const
    {
        const AZ::u32 dimension = GetDimension();
        AZ_Assert(dimension == rhs.GetDimension(), "VectorVariable dimensions do not match.");
        double result = 0.0;
        for (AZ::u32 i = 0; i < dimension; i++)
        {
            result += m_values[i] * rhs[i];
        }
        return result;
    }

    const AZStd::vector<double>& VectorVariable::GetValues() const
    {
        return m_values;
    }

    VectorVariable operator*(const double lhs, const VectorVariable& rhs)
    {
        const AZ::u32 dimension = rhs.GetDimension();
        VectorVariable result(dimension);
        for (AZ::u32 i = 0; i < dimension; i++)
        {
            result[i] = lhs * rhs[i];
        }
        return result;
    }

    MatrixVariable::MatrixVariable(AZ::u32 numRows, AZ::u32 numColumns)
    {
        m_numRows = numRows;
        m_numColumns = numColumns;
        m_values.clear();
        m_values.resize(m_numRows * m_numColumns, 0.0);
    }

    double& MatrixVariable::Element(AZ::u32 row, AZ::u32 column)
    {
        AZ_Assert(row < m_numRows && column < m_numColumns, "Invalid matrix index.");
        return m_values[row * m_numColumns + column];
    }

    double MatrixVariable::Element(AZ::u32 row, AZ::u32 column) const
    {
        AZ_Assert(row < m_numRows && column < m_numColumns, "Invalid matrix index.");
        return m_values[row * m_numColumns + column];
    }

    AZ::u32 MatrixVariable::GetNumRows() const
    {
        return m_numRows;
    }

    AZ::u32 MatrixVariable::GetNumColumns() const
    {
        return m_numColumns;
    }

    MatrixVariable MatrixVariable::operator+(const MatrixVariable& rhs) const
    {
        AZ_Assert(m_numRows == rhs.m_numRows && m_numColumns == rhs.m_numColumns, "Matrix dimensions do not match.");
        MatrixVariable result(m_numRows, m_numColumns);
        for (AZ::u32 row = 0; row < m_numRows; row++)
        {
            for (AZ::u32 column = 0; column < m_numColumns; column++)
            {
                result.Element(row, column) = Element(row, column) + rhs.Element(row, column);
            }
        }
        return result;
    }

    MatrixVariable MatrixVariable::operator+=(const MatrixVariable& rhs)
    {
        AZ_Assert(m_numRows == rhs.m_numRows && m_numColumns == rhs.m_numColumns, "Matrix dimensions do not match.");
        for (AZ::u32 row = 0; row < m_numRows; row++)
        {
            for (AZ::u32 column = 0; column < m_numColumns; column++)
            {
                Element(row, column) += rhs.Element(row, column);
            }
        }
        return *this;
    }

    MatrixVariable MatrixVariable::operator-(const MatrixVariable& rhs) const
    {
        MatrixVariable result(m_numRows, m_numColumns);
        for (AZ::u32 row = 0; row < m_numRows; row++)
        {
            for (AZ::u32 column = 0; column < m_numColumns; column++)
            {
                result.Element(row, column) = Element(row, column) - rhs.Element(row, column);
            }
        }
        return result;
    }

    MatrixVariable MatrixVariable::operator/(const double divisor) const
    {
        MatrixVariable result(m_numRows, m_numColumns);
        for (AZ::u32 row = 0; row < m_numRows; row++)
        {
            for (AZ::u32 column = 0; column < m_numColumns; column++)
            {
                result.Element(row, column) = Element(row, column) / divisor;
            }
        }
        return result;
    }

    VectorVariable operator*(const MatrixVariable& lhs, const VectorVariable& rhs)
    {
        AZ_Assert(lhs.GetNumColumns() == rhs.GetDimension(), "Matrix and vector dimensions do not match.");
        VectorVariable result(lhs.GetNumRows());
        for (AZ::u32 row = 0; row < lhs.GetNumRows(); row++)
        {
            result[row] = 0.0;
            for (AZ::u32 column = 0; column < lhs.GetNumColumns(); column++)
            {
                result[row] += lhs.Element(row, column) * rhs[column];
            }
        }

        return result;
    }

    MatrixVariable operator*(const MatrixVariable& lhs, const MatrixVariable& rhs)
    {
        AZ_Assert(lhs.GetNumColumns() == rhs.GetNumRows(), "Invalid matrix dimensions for multiplication.");
        MatrixVariable result(lhs.GetNumRows(), rhs.GetNumColumns());
        for (AZ::u32 row = 0; row < lhs.GetNumRows(); row++)
        {
            for (AZ::u32 column = 0; column < rhs.GetNumColumns(); column++)
            {
                result.Element(row, column) = 0.0;
                for (AZ::u32 i = 0; i < lhs.GetNumColumns(); i++)
                {
                    result.Element(row, column) += lhs.Element(row, i) * rhs.Element(i, column);
                }
            }
        }
        return result;
    }

    MatrixVariable operator*(double lhs, const MatrixVariable& rhs)
    {
        MatrixVariable result(rhs.GetNumRows(), rhs.GetNumColumns());
        const AZ::u32 numRows = rhs.GetNumRows();
        const AZ::u32 numColumns = rhs.GetNumColumns();
        for (AZ::u32 row = 0; row < numRows; row++)
        {
            for (AZ::u32 column = 0; column < numColumns; column++)
            {
                result.Element(row, column) = lhs * rhs.Element(row, column);
            }
        }
        return result;
    }

    MatrixVariable OuterProduct(const VectorVariable& x, const VectorVariable& y)
    {
        MatrixVariable result(x.GetDimension(), y.GetDimension());
        for (AZ::u32 r = 0; r < x.GetDimension(); r++)
        {
            for (AZ::u32 c = 0; c < y.GetDimension(); c++)
            {
                result.Element(r, c) = x[r] * y[c];
            }
        }
        return result;
    }
} // namespace NumericalMethods
