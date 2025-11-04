/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/tuple.h>
#include <ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvas
{
    namespace Matrix3x3Functions
    {
        Data::Matrix3x3Type FromColumns(const Data::Vector3Type& col0, const Data::Vector3Type& col1, const Data::Vector3Type& col2);

        Data::Matrix3x3Type FromCrossProduct(const Data::Vector3Type& source);

        Data::Matrix3x3Type FromDiagonal(const Data::Vector3Type& source);

        Data::Matrix3x3Type FromMatrix4x4(const Data::Matrix4x4Type& source);

        Data::Matrix3x3Type FromQuaternion(const Data::QuaternionType& source);

        Data::Matrix3x3Type FromRotationXDegrees(const Data::NumberType angle);

        Data::Matrix3x3Type FromRotationYDegrees(const Data::NumberType angle);

        Data::Matrix3x3Type FromRotationZDegrees(const Data::NumberType angle);

        Data::Matrix3x3Type FromRows(const Data::Vector3Type& row0, const Data::Vector3Type& row1, const Data::Vector3Type& row2);

        Data::Matrix3x3Type FromScale(const Data::Vector3Type& source);

        Data::Matrix3x3Type FromTransform(const Data::TransformType& source);

        Data::Vector3Type GetColumn(const Data::Matrix3x3Type& source, Data::NumberType col);

        AZStd::tuple<Data::Vector3Type, Data::Vector3Type, Data::Vector3Type> GetColumns(const Data::Matrix3x3Type& source);

        Data::Vector3Type GetDiagonal(const Data::Matrix3x3Type& source);

        Data::NumberType GetElement(const Data::Matrix3x3Type& source, Data::NumberType row, Data::NumberType col);

        Data::Vector3Type GetRow(const Data::Matrix3x3Type& source, Data::NumberType row);

        AZStd::tuple<Data::Vector3Type, Data::Vector3Type, Data::Vector3Type> GetRows(const Data::Matrix3x3Type& source);

        Data::Matrix3x3Type Invert(const Data::Matrix3x3Type& source);

        Data::BooleanType IsClose(const Data::Matrix3x3Type& lhs, const Data::Matrix3x3Type& rhs, const Data::NumberType tolerance);

        Data::BooleanType IsFinite(const Data::Matrix3x3Type& source);

        Data::BooleanType IsOrthogonal(const Data::Matrix3x3Type& source);

        Data::Matrix3x3Type MultiplyByNumber(const Data::Matrix3x3Type& source, Data::NumberType multiplier);

        Data::Vector3Type MultiplyByVector(const Data::Matrix3x3Type& lhs, const Data::Vector3Type& rhs);

        Data::Matrix3x3Type Orthogonalize(const Data::Matrix3x3Type& source);

        Data::Matrix3x3Type ToAdjugate(const Data::Matrix3x3Type& source);

        Data::NumberType ToDeterminant(const Data::Matrix3x3Type& source);

        Data::Vector3Type ToScale(const Data::Matrix3x3Type& source);

        Data::Matrix3x3Type Transpose(const Data::Matrix3x3Type& source);

        Data::Matrix3x3Type Zero();
    } // namespace Matrix3x3Functions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Math/Matrix3x3.generated.h>
