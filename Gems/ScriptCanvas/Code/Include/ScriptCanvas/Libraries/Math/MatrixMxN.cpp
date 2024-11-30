/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MatrixMxN.h"

#include <Include/ScriptCanvas/Libraries/Math/MatrixMxN.generated.h>

namespace ScriptCanvas
{
    namespace MatrixMxNFunctions
    {
        Data::MatrixMxNType Zero(Data::NumberType rows, Data::NumberType cols)
        {
            return Data::MatrixMxNType::CreateZero(aznumeric_cast<AZStd::size_t>(rows), aznumeric_cast<AZStd::size_t>(cols));
        }

        Data::MatrixMxNType Random(Data::NumberType rows, Data::NumberType cols)
        {
            return Data::MatrixMxNType::CreateRandom(aznumeric_cast<AZStd::size_t>(rows), aznumeric_cast<AZStd::size_t>(cols));
        }

        Data::NumberType GetRowCount(const Data::MatrixMxNType& source)
        {
            return static_cast<Data::NumberType>(source.GetRowCount());
        }

        Data::NumberType GetColumnCount(const Data::MatrixMxNType& source)
        {
            return static_cast<Data::NumberType>(source.GetColumnCount());
        }

        Data::NumberType GetElement(const Data::MatrixMxNType& source, Data::NumberType row, Data::NumberType col)
        {
            const auto rowIndex = aznumeric_cast<AZStd::size_t>(row);
            const auto colIndex = aznumeric_cast<AZStd::size_t>(col);
            return rowIndex < source.GetRowCount()
                && colIndex < source.GetColumnCount() ? source.GetElement(rowIndex, colIndex) : Data::NumberType();
        }

        Data::MatrixMxNType Transpose(const Data::MatrixMxNType& source)
        {
            return source.GetTranspose();
        }

        Data::MatrixMxNType OuterProduct(const Data::VectorNType& lhs, const Data::VectorNType& rhs)
        {
            Data::MatrixMxNType result(lhs.GetDimensionality(), rhs.GetDimensionality());
            AZ::OuterProduct(lhs, rhs, result);
            return result;
        }

        Data::VectorNType RightMultiplyByVector(const Data::MatrixMxNType& lhs, const Data::VectorNType& rhs)
        {
            if (rhs.GetDimensionality() == lhs.GetColumnCount())
            {
                Data::VectorNType result(lhs.GetRowCount());
                AZ::VectorMatrixMultiply(lhs, rhs, result);
                return result;
            }
            return Data::VectorNType(0);
        }

        Data::VectorNType LeftMultiplyByVector(const Data::VectorNType& lhs, const Data::MatrixMxNType& rhs)
        {
            if (lhs.GetDimensionality() == rhs.GetRowCount())
            {
                Data::VectorNType result(rhs.GetColumnCount());
                AZ::VectorMatrixMultiplyLeft(lhs, rhs, result);
                return result;
            }
            return Data::VectorNType(0);
        }

        Data::MatrixMxNType MultiplyByMatrix(const Data::MatrixMxNType& lhs, const Data::MatrixMxNType& rhs)
        {
            if (lhs.GetColumnCount() == rhs.GetRowCount())
            {
                Data::MatrixMxNType result(lhs.GetRowCount(), rhs.GetColumnCount());
                AZ::MatrixMatrixMultiply(lhs, rhs, result);
                return result;
            }
            return Data::MatrixMxNType(0, 0);
        }
    } // namespace MatrixMxNFunctions
} // namespace ScriptCanvas
