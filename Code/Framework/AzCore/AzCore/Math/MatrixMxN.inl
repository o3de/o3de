/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ
{
    AZ_MATH_INLINE MatrixMxN::MatrixMxN(AZStd::size_t rowCount, AZStd::size_t colCount)
        : m_rowCount(rowCount)
        , m_colCount(colCount)
    {
        OnSizeChanged();
    }

    AZ_MATH_INLINE MatrixMxN::MatrixMxN(AZStd::size_t rowCount, AZStd::size_t colCount, float value)
        : MatrixMxN(rowCount, colCount)
    {
        Simd::Vec4::FloatType splatValue = Simd::Vec4::Splat(value);
        for (Matrix4x4& element : m_values)
        {
            Simd::Vec4::FloatType* elements = element.GetSimdValues();
            elements[0] = splatValue;
            elements[1] = splatValue;
            elements[2] = splatValue;
            elements[3] = splatValue;
        }
        FixUnusedElements();
    }

    AZ_MATH_INLINE MatrixMxN::MatrixMxN(MatrixMxN&& rhs)
        : m_rowCount(rhs.m_rowCount)
        , m_colCount(rhs.m_colCount)
        , m_numRowGroups(rhs.m_numRowGroups)
        , m_numColGroups(rhs.m_numColGroups)
        , m_values(AZStd::move(rhs.m_values))
    {
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::CreateZero(AZStd::size_t rowCount, AZStd::size_t colCount)
    {
        MatrixMxN returnValue(rowCount, colCount);
        for (Matrix4x4& element : returnValue.m_values)
        {
            Simd::Vec4::FloatType* elements = element.GetSimdValues();
            elements[0] = Simd::Vec4::ZeroFloat();
            elements[1] = Simd::Vec4::ZeroFloat();
            elements[2] = Simd::Vec4::ZeroFloat();
            elements[3] = Simd::Vec4::ZeroFloat();
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::CreateFromPackedFloats(AZStd::size_t rowCount, AZStd::size_t colCount, const float* inputs)
    {
        MatrixMxN returnValue(rowCount, colCount);
        for (AZStd::size_t rowIter = 0; rowIter < rowCount; ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < colCount; ++colIter)
            {
                returnValue.SetElement(rowIter, colIter, *inputs);
                ++inputs;
            }
        }
        returnValue.FixUnusedElements();
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::CreateRandom(AZStd::size_t rowCount, AZStd::size_t colCount)
    {
        SimpleLcgRandomVec4 randGen;
        MatrixMxN returnValue(rowCount, colCount);
        for (Matrix4x4& element : returnValue.m_values)
        {
            Simd::Vec4::FloatType* elements = element.GetSimdValues();
            elements[0] = randGen.GetRandomFloat4();
            elements[1] = randGen.GetRandomFloat4();
            elements[2] = randGen.GetRandomFloat4();
            elements[3] = randGen.GetRandomFloat4();
        }
        returnValue.FixUnusedElements();
        return returnValue;
    }

    AZ_MATH_INLINE AZStd::size_t MatrixMxN::GetRowCount() const
    {
        return m_rowCount;
    }

    AZ_MATH_INLINE AZStd::size_t MatrixMxN::GetColumnCount() const
    {
        return m_colCount;
    }

    AZ_MATH_INLINE void MatrixMxN::Resize(AZStd::size_t rowCount, AZStd::size_t colCount)
    {
        m_rowCount = rowCount;
        m_colCount = colCount;
        OnSizeChanged();
    }

    AZ_MATH_INLINE float MatrixMxN::GetElement(AZStd::size_t row, AZStd::size_t col) const
    {
        AZ_Assert(row < m_rowCount, "Out of bounds element requested");
        AZ_Assert(col < m_colCount, "Out of bounds element requested");
        // Note that we compose the larger matrix out of a set of smaller 4x4 submatrices
        // Those submatrices are laid out in column major ordering
        // Additionally, we store the elements within each 4x4 submatrix in column major ordering
        const AZStd::size_t rowLookup = row >> 2; // Compute which matrix holds the value we want
        const AZStd::size_t colLookup = col >> 2; // Compute which matrix holds the value we want
        const Matrix4x4& matrixElement = GetSubmatrix(rowLookup, colLookup); // Retrieve the specific submatrix holding our value
        return matrixElement.GetElement(col & 0x3, row & 0x3); // Retrieve the specific element within the 4x4 submatrix
    }

    AZ_MATH_INLINE void MatrixMxN::SetElement(AZStd::size_t row, AZStd::size_t col, float value)
    {
        AZ_Assert(row < m_rowCount, "Out of bounds element requested");
        AZ_Assert(col < m_colCount, "Out of bounds element requested");
        // Note that we compose the larger matrix out of a set of smaller 4x4 submatrices
        // Those submatrices are laid out in column major ordering
        // Additionally, we store the elements within each 4x4 submatrix in column major ordering
        const AZStd::size_t rowLookup = row >> 2; // Compute which matrix holds the value we want
        const AZStd::size_t colLookup = col >> 2; // Compute which matrix holds the value we want
        Matrix4x4& matrixElement = GetSubmatrix(rowLookup, colLookup); // Retrieve the specific submatrix holding our value
        matrixElement.SetElement(col & 0x3, row & 0x3, value); // Set the specific element within the 4x4 submatrix
    }

    AZ_MATH_INLINE float MatrixMxN::operator()(AZStd::size_t row, AZStd::size_t col) const
    {
        return GetElement(row, col);
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::GetTranspose() const
    {
        // Returns the transpose of this matrix
        // This is a bit tricky, and so deserves a comment
        // We're transposing the elements inside each 4x4 submatrix, while at the same time transposing the submatrices themselves
        // Subdividing like this allows us to take full advantage of the simd units to perform the transpose
        MatrixMxN returnValue(m_colCount, m_rowCount);
        for (AZStd::size_t colIter = 0; colIter < m_numColGroups; ++colIter)
        {
            for (AZStd::size_t rowIter = 0; rowIter < m_numRowGroups; ++rowIter)
            {
                returnValue.SetSubmatrix(colIter, rowIter, GetSubmatrix(rowIter, colIter).GetTranspose());
            }
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::GetFloor() const
    {
        MatrixMxN returnValue(m_rowCount, m_colCount);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i].SetRow(0, m_values[i].GetRow(0).GetFloor());
            returnValue.m_values[i].SetRow(1, m_values[i].GetRow(1).GetFloor());
            returnValue.m_values[i].SetRow(2, m_values[i].GetRow(2).GetFloor());
            returnValue.m_values[i].SetRow(3, m_values[i].GetRow(3).GetFloor());
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::GetCeil() const
    {
        MatrixMxN returnValue(m_rowCount, m_colCount);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i].SetRow(0, m_values[i].GetRow(0).GetCeil());
            returnValue.m_values[i].SetRow(1, m_values[i].GetRow(1).GetCeil());
            returnValue.m_values[i].SetRow(2, m_values[i].GetRow(2).GetCeil());
            returnValue.m_values[i].SetRow(3, m_values[i].GetRow(3).GetCeil());
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::GetRound() const
    {
        MatrixMxN returnValue(m_rowCount, m_colCount);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i].SetRow(0, m_values[i].GetRow(0).GetRound());
            returnValue.m_values[i].SetRow(1, m_values[i].GetRow(1).GetRound());
            returnValue.m_values[i].SetRow(2, m_values[i].GetRow(2).GetRound());
            returnValue.m_values[i].SetRow(3, m_values[i].GetRow(3).GetRound());
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::GetMin(const MatrixMxN& m) const
    {
        AZ_Assert(m_rowCount == m.m_rowCount, "Dimensionality must be equal");
        AZ_Assert(m_colCount == m.m_colCount, "Dimensionality must be equal");
        MatrixMxN returnValue(m_rowCount, m_colCount);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i].SetRow(0, m_values[i].GetRow(0).GetMin(m.m_values[i].GetRow(0)));
            returnValue.m_values[i].SetRow(1, m_values[i].GetRow(1).GetMin(m.m_values[i].GetRow(1)));
            returnValue.m_values[i].SetRow(2, m_values[i].GetRow(2).GetMin(m.m_values[i].GetRow(2)));
            returnValue.m_values[i].SetRow(3, m_values[i].GetRow(3).GetMin(m.m_values[i].GetRow(3)));
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::GetMax(const MatrixMxN& m) const
    {
        AZ_Assert(m_rowCount == m.m_rowCount, "Dimensionality must be equal");
        AZ_Assert(m_colCount == m.m_colCount, "Dimensionality must be equal");
        MatrixMxN returnValue(m_rowCount, m_colCount);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i].SetRow(0, m_values[i].GetRow(0).GetMax(m.m_values[i].GetRow(0)));
            returnValue.m_values[i].SetRow(1, m_values[i].GetRow(1).GetMax(m.m_values[i].GetRow(1)));
            returnValue.m_values[i].SetRow(2, m_values[i].GetRow(2).GetMax(m.m_values[i].GetRow(2)));
            returnValue.m_values[i].SetRow(3, m_values[i].GetRow(3).GetMax(m.m_values[i].GetRow(3)));
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::GetClamp(const MatrixMxN& min, const MatrixMxN& max) const
    {
        AZ_Assert(m_rowCount == min.m_rowCount, "Dimensionality must be equal");
        AZ_Assert(m_colCount == min.m_colCount, "Dimensionality must be equal");
        AZ_Assert(m_rowCount == max.m_rowCount, "Dimensionality must be equal");
        AZ_Assert(m_colCount == max.m_colCount, "Dimensionality must be equal");
        MatrixMxN returnValue(m_rowCount, m_colCount);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i].SetRow(0, m_values[i].GetRow(0).GetClamp(min.m_values[i].GetRow(0), max.m_values[i].GetRow(0)));
            returnValue.m_values[i].SetRow(1, m_values[i].GetRow(1).GetClamp(min.m_values[i].GetRow(1), max.m_values[i].GetRow(1)));
            returnValue.m_values[i].SetRow(2, m_values[i].GetRow(2).GetClamp(min.m_values[i].GetRow(2), max.m_values[i].GetRow(2)));
            returnValue.m_values[i].SetRow(3, m_values[i].GetRow(3).GetClamp(min.m_values[i].GetRow(3), max.m_values[i].GetRow(3)));
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::GetAbs() const
    {
        MatrixMxN returnValue(m_rowCount, m_colCount);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i].SetRow(0, m_values[i].GetRow(0).GetAbs());
            returnValue.m_values[i].SetRow(1, m_values[i].GetRow(1).GetAbs());
            returnValue.m_values[i].SetRow(2, m_values[i].GetRow(2).GetAbs());
            returnValue.m_values[i].SetRow(3, m_values[i].GetRow(3).GetAbs());
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::GetSquare() const
    {
        MatrixMxN returnValue(m_rowCount, m_colCount);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i].SetRow(0, m_values[i].GetRow(0) * m_values[i].GetRow(0));
            returnValue.m_values[i].SetRow(1, m_values[i].GetRow(1) * m_values[i].GetRow(1));
            returnValue.m_values[i].SetRow(2, m_values[i].GetRow(2) * m_values[i].GetRow(2));
            returnValue.m_values[i].SetRow(3, m_values[i].GetRow(3) * m_values[i].GetRow(3));
        }
        return returnValue;
    }

    AZ_MATH_INLINE void MatrixMxN::SetZero()
    {
        AZ::Matrix4x4* data = m_values.data();
        memset(data, 0, sizeof(AZ::Matrix4x4) * m_values.size());
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::operator-() const
    {
        MatrixMxN returnValue(m_rowCount, m_colCount);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = -m_values[i];
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::operator+(const MatrixMxN& rhs) const
    {
        MatrixMxN returnValue(m_rowCount, m_colCount);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i] + rhs.m_values[i];
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::operator-(const MatrixMxN& rhs) const
    {
        MatrixMxN returnValue(m_rowCount, m_colCount);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i] - rhs.m_values[i];
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::operator*(const MatrixMxN& rhs) const
    {
        MatrixMxN result(GetRowCount(), rhs.GetColumnCount());
        MatrixMatrixMultiply(*this, rhs, result);
        return result;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::operator*(float multiplier) const
    {
        MatrixMxN returnValue(m_rowCount, m_colCount);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i] * multiplier;
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN MatrixMxN::operator/(float divisor) const
    {
        AZ_Assert(fabs(divisor) > Constants::FloatEpsilon, "Potential division by zero");
        MatrixMxN returnValue(m_rowCount, m_colCount);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i] / divisor;
        }
        return returnValue;
    }

    AZ_MATH_INLINE MatrixMxN& MatrixMxN::operator+=(const MatrixMxN& rhs)
    {
        AZ_Assert(m_rowCount == rhs.m_rowCount, "Dimensionality must be equal");
        AZ_Assert(m_colCount == rhs.m_colCount, "Dimensionality must be equal");
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            m_values[i] += rhs.m_values[i];
        }
        return *this;
    }

    AZ_MATH_INLINE MatrixMxN& MatrixMxN::operator-=(const MatrixMxN& rhs)
    {
        AZ_Assert(m_rowCount == rhs.m_rowCount, "Dimensionality must be equal");
        AZ_Assert(m_colCount == rhs.m_colCount, "Dimensionality must be equal");
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            m_values[i] -= rhs.m_values[i];
        }
        return *this;
    }

    AZ_MATH_INLINE MatrixMxN& MatrixMxN::operator+=(float sum)
    {
        const AZ::Vector4 sumRow(sum);
        for (Matrix4x4& element : m_values)
        {
            element.SetRow(0,element.GetRow(0) + sumRow);
            element.SetRow(1,element.GetRow(1) + sumRow);
            element.SetRow(2,element.GetRow(2) + sumRow);
            element.SetRow(3,element.GetRow(3) + sumRow);
        }
        FixUnusedElements();
        return *this;
    }

    AZ_MATH_INLINE MatrixMxN& MatrixMxN::operator-=(float difference)
    {
        const AZ::Vector4 diffRow(difference);
        for (Matrix4x4& element : m_values)
        {
            element.SetRow(0, element.GetRow(0) - diffRow);
            element.SetRow(1, element.GetRow(1) - diffRow);
            element.SetRow(2, element.GetRow(2) - diffRow);
            element.SetRow(3, element.GetRow(3) - diffRow);
        }
        FixUnusedElements();
        return *this;
    }

    AZ_MATH_INLINE MatrixMxN& MatrixMxN::operator*=(float multiplier)
    {
        for (Matrix4x4& element : m_values)
        {
            element *= multiplier;
        }
        return *this;
    }

    AZ_MATH_INLINE MatrixMxN& MatrixMxN::operator/=(float divisor)
    {
        for (Matrix4x4& element : m_values)
        {
            element /= divisor;
        }
        FixUnusedElements();
        return *this;
    }

    AZ_MATH_INLINE AZStd::size_t MatrixMxN::GetRowGroups() const
    {
        return m_numRowGroups;
    }

    AZ_MATH_INLINE AZStd::size_t MatrixMxN::GetColumnGroups() const
    {
        return m_numColGroups;
    }

    AZ_MATH_INLINE const Matrix4x4& MatrixMxN::GetSubmatrix(AZStd::size_t rowGroup, AZStd::size_t colGroup) const
    {
        AZ_Assert(rowGroup < m_numRowGroups, "Out of range row group requested");
        AZ_Assert(colGroup < m_numColGroups, "Out of range column group requested");
        return m_values[rowGroup * m_numColGroups + colGroup];
    }

    AZ_MATH_INLINE Matrix4x4& MatrixMxN::GetSubmatrix(AZStd::size_t rowGroup, AZStd::size_t colGroup)
    {
        AZ_Assert(rowGroup < m_numRowGroups, "Out of range row group requested");
        AZ_Assert(colGroup < m_numColGroups, "Out of range column group requested");
        return m_values[rowGroup * m_numColGroups + colGroup];
    }

    AZ_MATH_INLINE void MatrixMxN::SetSubmatrix(AZStd::size_t rowGroup, AZStd::size_t colGroup, const Matrix4x4& subMatrix)
    {
        AZ_Assert(rowGroup < m_numRowGroups, "Out of range row group requested");
        AZ_Assert(colGroup < m_numColGroups, "Out of range column group requested");
        m_values[rowGroup * m_numColGroups + colGroup] = subMatrix;
    }

    AZ_MATH_INLINE AZStd::vector<Matrix4x4>& MatrixMxN::GetMatrixElements()
    {
        return m_values;
    }

    AZ_MATH_INLINE void MatrixMxN::FixUnusedElements()
    {
        if (m_values.empty())
        {
            return;
        }

        const AZStd::size_t lastRowGroup = m_numRowGroups - 1;
        const AZStd::size_t lastColGroup = m_numColGroups - 1;
        const Simd::Vec4::FloatType zero = Simd::Vec4::ZeroFloat();
        const uint32_t masks[] =
        {
            0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
            0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
            0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000,
            0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000
        };

        // Fix last row values
        {
            const AZStd::size_t trailingZeroElements = 4 * (m_rowCount % 4);
            const Simd::Vec4::FloatType mask = Simd::Vec4::LoadAligned(reinterpret_cast<const float*>(&masks[trailingZeroElements]));

            for (AZStd::size_t colIter = 0; colIter < GetColumnGroups(); ++colIter)
            {
                Matrix4x4& lastBlock = GetSubmatrix(lastRowGroup, colIter);
                Simd::Vec4::FloatType* lastBlockElements = lastBlock.GetSimdValues();
                lastBlockElements[0] = Simd::Vec4::And(lastBlockElements[0], mask);
                lastBlockElements[1] = Simd::Vec4::And(lastBlockElements[1], mask);
                lastBlockElements[2] = Simd::Vec4::And(lastBlockElements[2], mask);
                lastBlockElements[3] = Simd::Vec4::And(lastBlockElements[3], mask);
            }
        }

        // Fix last column values
        {
            for (AZStd::size_t rowIter = 0; rowIter < GetRowGroups(); ++rowIter)
            {
                Matrix4x4& lastBlock = GetSubmatrix(rowIter, lastColGroup);
                Simd::Vec4::FloatType* lastBlockElements = lastBlock.GetSimdValues();
                // Fallthrough is intentional
                switch (m_colCount % 4)
                {
                case 1:
                    lastBlockElements[1] = zero;
                case 2:
                    lastBlockElements[2] = zero;
                case 3:
                    lastBlockElements[3] = zero;
                case 0:
                    break;
                }
            }
        }
    }

    AZ_MATH_INLINE void OuterProduct(const AZ::VectorN& lhs, const AZ::VectorN& rhs, AZ::MatrixMxN& output)
    {
        output.Resize(lhs.GetDimensionality(), rhs.GetDimensionality());
        for (AZStd::size_t colIter = 0; colIter < output.GetColumnGroups(); ++colIter)
        {
            AZ::Simd::Vec4::FloatType rhsElement = rhs.GetVectorValues()[colIter].GetSimdValue();
            AZ::Simd::Vec4::FloatType splat0 = AZ::Simd::Vec4::SplatIndex0(rhsElement);
            AZ::Simd::Vec4::FloatType splat1 = AZ::Simd::Vec4::SplatIndex1(rhsElement);
            AZ::Simd::Vec4::FloatType splat2 = AZ::Simd::Vec4::SplatIndex2(rhsElement);
            AZ::Simd::Vec4::FloatType splat3 = AZ::Simd::Vec4::SplatIndex3(rhsElement);
            for (AZStd::size_t rowIter = 0; rowIter < output.GetRowGroups(); ++rowIter)
            {
                AZ::Simd::Vec4::FloatType lhsElement = lhs.GetVectorValues()[rowIter].GetSimdValue();
                AZ::Matrix4x4& outputElement = output.GetSubmatrix(rowIter, colIter);
                outputElement.GetSimdValues()[0] = AZ::Simd::Vec4::Madd(lhsElement, splat0, outputElement.GetSimdValues()[0]);
                outputElement.GetSimdValues()[1] = AZ::Simd::Vec4::Madd(lhsElement, splat1, outputElement.GetSimdValues()[1]);
                outputElement.GetSimdValues()[2] = AZ::Simd::Vec4::Madd(lhsElement, splat2, outputElement.GetSimdValues()[2]);
                outputElement.GetSimdValues()[3] = AZ::Simd::Vec4::Madd(lhsElement, splat3, outputElement.GetSimdValues()[3]);
            }
        }
        output.FixUnusedElements();
    }

    AZ_MATH_INLINE void VectorMatrixMultiply(const MatrixMxN& matrix, const VectorN& vector, VectorN& output)
    {
        // Because we've stored our matrix in column major ordering, we can perform a vector matrix product using only multiply-accumulate
        // There is no requirement for horizontal sums
        // Note we don't clear the output vector prior to starting the madd loops, this means the original value of output is included in the final accumulation
        // This works out extremely well for having bias vectors, since if we initialize output to the bias, we include the bias in the final result for free
        AZ_Assert(matrix.GetColumnCount() == vector.GetDimensionality(), "Input vector dimensionality must match matrix column count");
        output.Resize(matrix.GetRowCount());
        for (AZStd::size_t colIter = 0; colIter < matrix.GetColumnGroups(); ++colIter)
        {
            Simd::Vec4::FloatType vectorElement = vector.GetVectorValues()[colIter].GetSimdValue();
            Simd::Vec4::FloatType splat0 = Simd::Vec4::SplatIndex0(vectorElement);
            Simd::Vec4::FloatType splat1 = Simd::Vec4::SplatIndex1(vectorElement);
            Simd::Vec4::FloatType splat2 = Simd::Vec4::SplatIndex2(vectorElement);
            Simd::Vec4::FloatType splat3 = Simd::Vec4::SplatIndex3(vectorElement);
            for (AZStd::size_t rowIter = 0; rowIter < matrix.GetRowGroups(); ++rowIter)
            {
                Vector4& outputElement = output.GetVectorValues()[rowIter];
                // Our 4x4 matrices are laid out in column priority ordering, so that we can re-use the same splat vectors without recalculating them
                // Additionally, the values within each 4x4 matrix are also laid out with column ordering so we can avoid any horizontal accumulations
                // Hence, fetching a row from a 4x4 submatrix is actually retrieving a subset of column values from the larger matrix
                const Matrix4x4& matrixElement = matrix.GetSubmatrix(rowIter, colIter);
                outputElement.SetSimdValue(Simd::Vec4::Madd(splat0, matrixElement.GetRow(0).GetSimdValue(), outputElement.GetSimdValue()));
                outputElement.SetSimdValue(Simd::Vec4::Madd(splat1, matrixElement.GetRow(1).GetSimdValue(), outputElement.GetSimdValue()));
                outputElement.SetSimdValue(Simd::Vec4::Madd(splat2, matrixElement.GetRow(2).GetSimdValue(), outputElement.GetSimdValue()));
                outputElement.SetSimdValue(Simd::Vec4::Madd(splat3, matrixElement.GetRow(3).GetSimdValue(), outputElement.GetSimdValue()));
            }
        }
        output.FixLastVectorElement();
    }

    AZ_MATH_INLINE void VectorMatrixMultiplyLeft(const VectorN& vector, const MatrixMxN& matrix, VectorN& output)
    {
        AZ_Assert(matrix.GetRowCount() == vector.GetDimensionality(), "Input vector dimensionality must match matrix row count");
        output.Resize(matrix.GetColumnCount());
        for (AZStd::size_t colIter = 0; colIter < matrix.GetColumnGroups(); ++colIter)
        {
            Vector4& outputElement = output.GetVectorValues()[colIter];
            for (AZStd::size_t rowIter = 0; rowIter < matrix.GetRowGroups(); ++rowIter)
            {
                Simd::Vec4::FloatType vectorElement = vector.GetVectorValues()[rowIter].GetSimdValue();
                const Matrix4x4& matrixElement = matrix.GetSubmatrix(rowIter, colIter);
                outputElement.SetSimdValue(Simd::Vec4::Add(outputElement.GetSimdValue(), Simd::Vec4::Mat4x4TransformVector(matrixElement.GetSimdValues(), vectorElement)));
            }
        }
        output.FixLastVectorElement();
    }

    AZ_MATH_INLINE void MatrixMatrixMultiply(const MatrixMxN& lhs, const MatrixMxN& rhs, MatrixMxN& output)
    {
        AZ_Assert(lhs.GetColumnCount() == rhs.GetRowCount(), "Left-hand side column count must match right-hand side row count");
        output.Resize(lhs.GetRowCount(), rhs.GetColumnCount());
        for (AZStd::size_t rowIter = 0; rowIter < lhs.GetRowGroups(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < rhs.GetColumnGroups(); ++colIter)
            {
                Matrix4x4& outputElement = output.GetSubmatrix(rowIter, colIter);
                outputElement = Matrix4x4::CreateZero();
                for (AZStd::size_t subIter = 0; subIter < lhs.GetColumnGroups(); ++subIter)
                {
                    const Matrix4x4& lhsElement = lhs.GetSubmatrix(rowIter, subIter);
                    const Matrix4x4& rhsElement = rhs.GetSubmatrix(subIter, colIter);
                    // The submatrices are in column-major orientation, however Mat4x4Multiply expects row-major orientation
                    // For this reason, we swap the order of operands to produce the correct calcuations
                    Simd::Vec4::Mat4x4MultiplyAdd(rhsElement.GetSimdValues(), lhsElement.GetSimdValues(), outputElement.GetSimdValues(), outputElement.GetSimdValues());
                }
            }
        }
    }

    AZ_MATH_INLINE void MatrixMxN::OnSizeChanged()
    {
        m_numRowGroups = (m_rowCount + 3) / 4;
        m_numColGroups = (m_colCount + 3) / 4;
        m_values.resize(m_numColGroups * m_numRowGroups);
        FixUnusedElements();
    }
}
