/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Matrix4x4.h"

#include <Include/ScriptCanvas/Libraries/Math/Matrix4x4.generated.h>

namespace ScriptCanvas
{
    namespace Matrix4x4Functions
    {
        Data::Matrix4x4Type FromColumns(
            const Data::Vector4Type& col0, const Data::Vector4Type& col1, const Data::Vector4Type& col2, const Data::Vector4Type& col3)
        {
            return Data::Matrix4x4Type::CreateFromColumns(col0, col1, col2, col3);
        }

        Data::Matrix4x4Type FromDiagonal(const Data::Vector4Type& source)
        {
            return Data::Matrix4x4Type::CreateDiagonal(source);
        }

        Data::Matrix4x4Type FromMatrix3x3(const Data::Matrix3x3Type& source)
        {
            Data::Vector3Type row0, row1, row2;
            source.GetRows(&row0, &row1, &row2);
            return Data::Matrix4x4Type::CreateFromRows(
                Data::Vector4Type::CreateFromVector3(row0), Data::Vector4Type::CreateFromVector3(row1),
                Data::Vector4Type::CreateFromVector3(row2), Data::Vector4Type::CreateAxisW());
        }

        Data::Matrix4x4Type FromQuaternion(const Data::QuaternionType& source)
        {
            return Data::Matrix4x4Type::CreateFromQuaternion(source);
        }

        Data::Matrix4x4Type FromQuaternionAndTranslation(
            const Data::QuaternionType& rotation, const Data::Vector3Type& translation)
        {
            return Data::Matrix4x4Type::CreateFromQuaternionAndTranslation(rotation, translation);
        }

        Data::Matrix4x4Type FromRotationXDegrees(const Data::NumberType angle)
        {
            return Data::Matrix4x4Type::CreateRotationX(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }

        Data::Matrix4x4Type FromRotationYDegrees(const Data::NumberType angle)
        {
            return Data::Matrix4x4Type::CreateRotationY(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }

        Data::Matrix4x4Type FromRotationZDegrees(const Data::NumberType angle)
        {
            return Data::Matrix4x4Type::CreateRotationZ(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }

        Data::Matrix4x4Type FromRows(
            const Data::Vector4Type& row0, const Data::Vector4Type& row1, const Data::Vector4Type& row2, const Data::Vector4Type& row3)
        {
            return Data::Matrix4x4Type::CreateFromRows(row0, row1, row2, row3);
        }

        Data::Matrix4x4Type FromScale(const Data::Vector3Type& source)
        {
            return Data::Matrix4x4Type::CreateScale(source);
        }

        Data::Matrix4x4Type FromTranslation(const Data::Vector3Type& source)
        {
            return Data::Matrix4x4Type::CreateTranslation(source);
        }

        Data::Matrix4x4Type FromTransform(const Data::TransformType& source)
        {
            return Data::Matrix4x4Type::CreateFromTransform(source);
        }

        Data::Vector4Type GetColumn(const Data::Matrix4x4Type& source, Data::NumberType col)
        {
            const AZ::s32 numColumns(4);
            const auto colIndex = aznumeric_cast<AZ::s32>(col);
            return colIndex >= 0 && colIndex < numColumns ? source.GetColumn(colIndex) : Data::Vector4Type::CreateZero();
        }

        std::tuple<Data::Vector4Type, Data::Vector4Type, Data::Vector4Type, Data::Vector4Type> GetColumns(
            const Data::Matrix4x4Type& source)
        {
            Data::Vector4Type col0, col1, col2, col3;
            source.GetColumns(&col0, &col1, &col2, &col3);
            return std::make_tuple(col0, col1, col2, col3);
        }

        Data::Vector4Type GetDiagonal(const Data::Matrix4x4Type& source)
        {
            return source.GetDiagonal();
        }

        Data::NumberType GetElement(const Data::Matrix4x4Type& source, Data::NumberType row, Data::NumberType col)
        {
            const AZ::s32 numRows(4), numColumns(4);
            const auto rowIndex = aznumeric_cast<AZ::s32>(row);
            const auto colIndex = aznumeric_cast<AZ::s32>(col);
            return rowIndex >= 0 && rowIndex < numRows && colIndex >= 0 && colIndex < numColumns ? source.GetElement(rowIndex, colIndex)
                                                                                                 : Data::NumberType();
        }

        Data::Vector4Type GetRow(const Data::Matrix4x4Type& source, Data::NumberType row)
        {
            const AZ::s32 numRows(4);
            const auto rowIndex = aznumeric_cast<AZ::s32>(row);
            return rowIndex >= 0 && rowIndex < numRows ? source.GetRow(rowIndex) : Data::Vector4Type::CreateZero();
        }

        Data::Vector3Type GetTranslation(const Data::Matrix4x4Type& source)
        {
            return source.GetTranslation();
        }

        std::tuple<Data::Vector4Type, Data::Vector4Type, Data::Vector4Type, Data::Vector4Type> GetRows(
            const Data::Matrix4x4Type& source)
        {
            Data::Vector4Type row0, row1, row2, row3;
            source.GetRows(&row0, &row1, &row2, &row3);
            return std::make_tuple(row0, row1, row2, row3);
        }

        Data::Matrix4x4Type Invert(const Data::Matrix4x4Type& source)
        {
            return source.GetInverseFull();
        }

        Data::BooleanType IsClose(
            const Data::Matrix4x4Type& lhs, const Data::Matrix4x4Type& rhs, const Data::NumberType tolerance)
        {
            return lhs.IsClose(rhs, aznumeric_cast<float>(tolerance));
        }

        Data::BooleanType IsFinite(const Data::Matrix4x4Type& source)
        {
            return source.IsFinite();
        }

        Data::Vector4Type MultiplyByVector(const Data::Matrix4x4Type& lhs, const Data::Vector4Type& rhs)
        {
            return lhs * rhs;
        }

        Data::Vector3Type ToScale(const Data::Matrix4x4Type& source)
        {
            return source.RetrieveScale();
        }

        Data::Matrix4x4Type Transpose(const Data::Matrix4x4Type& source)
        {
            return source.GetTranspose();
        }

        Data::Matrix4x4Type Zero()
        {
            return Data::Matrix4x4Type::CreateZero();
        }
    } // namespace Matrix4x4Functions
} // namespace ScriptCanvas
