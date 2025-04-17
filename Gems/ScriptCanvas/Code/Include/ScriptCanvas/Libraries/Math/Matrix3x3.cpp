/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Matrix3x3.h"

#include <Include/ScriptCanvas/Libraries/Math/Matrix3x3.generated.h>

namespace ScriptCanvas
{
    namespace Matrix3x3Functions
    {
        Data::Matrix3x3Type FromColumns(
            const Data::Vector3Type& col0, const Data::Vector3Type& col1, const Data::Vector3Type& col2)
        {
            return Data::Matrix3x3Type::CreateFromColumns(col0, col1, col2);
        }

        Data::Matrix3x3Type FromCrossProduct(const Data::Vector3Type& source)
        {
            return Data::Matrix3x3Type::CreateCrossProduct(source);
        }

        Data::Matrix3x3Type FromDiagonal(const Data::Vector3Type& source)
        {
            return Data::Matrix3x3Type::CreateDiagonal(source);
        }

        Data::Matrix3x3Type FromMatrix4x4(const Data::Matrix4x4Type& source)
        {
            return Data::Matrix3x3Type::CreateFromMatrix4x4(source);
        }

        Data::Matrix3x3Type FromQuaternion(const Data::QuaternionType& source)
        {
            return Data::Matrix3x3Type::CreateFromQuaternion(source);
        }

        Data::Matrix3x3Type FromRotationXDegrees(const Data::NumberType angle)
        {
            return Data::Matrix3x3Type::CreateRotationX(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }

        Data::Matrix3x3Type FromRotationYDegrees(const Data::NumberType angle)
        {
            return Data::Matrix3x3Type::CreateRotationY(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }

        Data::Matrix3x3Type FromRotationZDegrees(const Data::NumberType angle)
        {
            return Data::Matrix3x3Type::CreateRotationZ(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }

        Data::Matrix3x3Type FromRows(const Data::Vector3Type& row0, const Data::Vector3Type& row1, const Data::Vector3Type& row2)
        {
            return Data::Matrix3x3Type::CreateFromRows(row0, row1, row2);
        }

        Data::Matrix3x3Type FromScale(const Data::Vector3Type& source)
        {
            return Data::Matrix3x3Type::CreateScale(source);
        }

        Data::Matrix3x3Type FromTransform(const Data::TransformType& source)
        {
            return Data::Matrix3x3Type::CreateFromTransform(source);
        }

        Data::Vector3Type GetColumn(const Data::Matrix3x3Type& source, Data::NumberType col)
        {
            const AZ::s32 numColumns(3);
            const auto colIndex = aznumeric_cast<AZ::s32>(col);
            return colIndex >= 0 && colIndex < numColumns ? source.GetColumn(colIndex) : Data::Vector3Type::CreateZero();
        }

        AZStd::tuple<Data::Vector3Type, Data::Vector3Type, Data::Vector3Type> GetColumns(const Data::Matrix3x3Type& source)
        {
            Data::Vector3Type col0, col1, col2;
            source.GetColumns(&col0, &col1, &col2);
            return AZStd::make_tuple(col0, col1, col2);
        }

        Data::Vector3Type GetDiagonal(const Data::Matrix3x3Type& source)
        {
            return source.GetDiagonal();
        }

        Data::NumberType GetElement(const Data::Matrix3x3Type& source, Data::NumberType row, Data::NumberType col)
        {
            const AZ::s32 numRows(3), numColumns(3);
            const auto rowIndex = aznumeric_cast<AZ::s32>(row);
            const auto colIndex = aznumeric_cast<AZ::s32>(col);
            return rowIndex >= 0 && rowIndex < numRows && colIndex >= 0 && colIndex < numColumns ? source.GetElement(rowIndex, colIndex)
                                                                                                 : Data::NumberType();
        }

        Data::Vector3Type GetRow(const Data::Matrix3x3Type& source, Data::NumberType row)
        {
            const AZ::s32 numRows(3);
            const auto rowIndex = aznumeric_cast<AZ::s32>(row);
            return rowIndex >= 0 && rowIndex < numRows ? source.GetRow(rowIndex) : Data::Vector3Type::CreateZero();
        }

        AZStd::tuple<Data::Vector3Type, Data::Vector3Type, Data::Vector3Type> GetRows(const Data::Matrix3x3Type& source)
        {
            Data::Vector3Type row0, row1, row2;
            source.GetRows(&row0, &row1, &row2);
            return AZStd::make_tuple(row0, row1, row2);
        }

        Data::Matrix3x3Type Invert(const Data::Matrix3x3Type& source)
        {
            return source.GetInverseFull();
        }

        Data::BooleanType IsClose(
            const Data::Matrix3x3Type& lhs, const Data::Matrix3x3Type& rhs, const Data::NumberType tolerance)
        {
            return lhs.IsClose(rhs, aznumeric_cast<float>(tolerance));
        }

        Data::BooleanType IsFinite(const Data::Matrix3x3Type& source)
        {
            return source.IsFinite();
        }

        Data::BooleanType IsOrthogonal(const Data::Matrix3x3Type& source)
        {
            return source.IsOrthogonal();
        }

        Data::Matrix3x3Type MultiplyByNumber(const Data::Matrix3x3Type& source, Data::NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }

        Data::Vector3Type MultiplyByVector(const Data::Matrix3x3Type& lhs, const Data::Vector3Type& rhs)
        {
            return lhs * rhs;
        }

        Data::Matrix3x3Type Orthogonalize(const Data::Matrix3x3Type& source)
        {
            return source.GetOrthogonalized();
        }

        Data::Matrix3x3Type ToAdjugate(const Data::Matrix3x3Type& source)
        {
            return source.GetAdjugate();
        }

        Data::NumberType ToDeterminant(const Data::Matrix3x3Type& source)
        {
            return source.GetDeterminant();
        }

        Data::Vector3Type ToScale(const Data::Matrix3x3Type& source)
        {
            return source.RetrieveScale();
        }

        Data::Matrix3x3Type Transpose(const Data::Matrix3x3Type& source)
        {
            return source.GetTranspose();
        }

        Data::Matrix3x3Type Zero()
        {
            return Data::Matrix3x3Type::CreateZero();
        }
    } // namespace Matrix3x3Functions
} // namespace ScriptCanvas
