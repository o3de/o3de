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
    namespace Matrix4x4Functions
    {
        Data::Matrix4x4Type FromColumns(const Data::Vector4Type& col0, const Data::Vector4Type& col1, const Data::Vector4Type& col2, const Data::Vector4Type& col3);

        Data::Matrix4x4Type FromDiagonal(const Data::Vector4Type& source);

        Data::Matrix4x4Type FromMatrix3x3(const Data::Matrix3x3Type& source);

        Data::Matrix4x4Type FromQuaternion(const Data::QuaternionType& source);

        Data::Matrix4x4Type FromQuaternionAndTranslation(const Data::QuaternionType& rotation, const Data::Vector3Type& translation);

        Data::Matrix4x4Type FromRotationXDegrees(const Data::NumberType angle);

        Data::Matrix4x4Type FromRotationYDegrees(const Data::NumberType angle);

        Data::Matrix4x4Type FromRotationZDegrees(const Data::NumberType angle);

        Data::Matrix4x4Type FromRows(const Data::Vector4Type& row0, const Data::Vector4Type& row1, const Data::Vector4Type& row2, const Data::Vector4Type& row3);

        Data::Matrix4x4Type FromScale(const Data::Vector3Type& source);

        Data::Matrix4x4Type FromTranslation(const Data::Vector3Type& source);

        Data::Matrix4x4Type FromTransform(const Data::TransformType& source);

        Data::Vector4Type GetColumn(const Data::Matrix4x4Type& source, Data::NumberType col);

        AZStd::tuple<Data::Vector4Type, Data::Vector4Type, Data::Vector4Type, Data::Vector4Type> GetColumns(const Data::Matrix4x4Type& source);

        Data::Vector4Type GetDiagonal(const Data::Matrix4x4Type& source);

        Data::NumberType GetElement(const Data::Matrix4x4Type& source, Data::NumberType row, Data::NumberType col);

        Data::Vector4Type GetRow(const Data::Matrix4x4Type& source, Data::NumberType row);

        Data::Vector3Type GetTranslation(const Data::Matrix4x4Type& source);

        AZStd::tuple<Data::Vector4Type, Data::Vector4Type, Data::Vector4Type, Data::Vector4Type> GetRows(const Data::Matrix4x4Type& source);

        Data::Matrix4x4Type Invert(const Data::Matrix4x4Type& source);

        Data::BooleanType IsClose(const Data::Matrix4x4Type& lhs, const Data::Matrix4x4Type& rhs, const Data::NumberType tolerance);

        Data::BooleanType IsFinite(const Data::Matrix4x4Type& source);

        Data::Vector4Type MultiplyByVector(const Data::Matrix4x4Type& lhs, const Data::Vector4Type& rhs);

        Data::Vector3Type ToScale(const Data::Matrix4x4Type& source);

        Data::Matrix4x4Type Transpose(const Data::Matrix4x4Type& source);

        Data::Matrix4x4Type Zero();

    } // namespace Matrix4x4Functions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Math/Matrix4x4.generated.h>
