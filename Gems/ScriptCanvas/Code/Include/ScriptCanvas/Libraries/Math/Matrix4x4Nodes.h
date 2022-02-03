/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/NodeFunctionGeneric.h>

namespace ScriptCanvas
{
    namespace Matrix4x4Nodes
    {
        static constexpr const char* k_categoryName = "Math/Matrix4x4";

        AZ_INLINE Data::Matrix4x4Type FromColumns(const Data::Vector4Type& col0, const Data::Vector4Type& col1, const Data::Vector4Type& col2, const Data::Vector4Type& col3)
        {
            return Data::Matrix4x4Type::CreateFromColumns(col0, col1, col2, col3);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromColumns, k_categoryName, "{7A5CDFCC-9BD1-493F-8959-B1868302CD1D}", "returns a rotation matrix based on angle around Z axis", "Column1", "Column2", "Column3", "Column4");

        AZ_INLINE Data::Matrix4x4Type FromDiagonal(const Data::Vector4Type& source)
        {
            return Data::Matrix4x4Type::CreateDiagonal(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromDiagonal, k_categoryName, "{24F755B3-230C-45FE-86FC-C6A0ED219C9D}", "returns a diagonal matrix using the supplied vector", "Source");

        AZ_INLINE Data::Matrix4x4Type FromMatrix3x3(const Data::Matrix3x3Type& source)
        {
            Data::Vector3Type row0, row1, row2;
            source.GetRows(&row0, &row1, &row2);
            return Data::Matrix4x4Type::CreateFromRows(Data::Vector4Type::CreateFromVector3(row0),
                Data::Vector4Type::CreateFromVector3(row1),
                Data::Vector4Type::CreateFromVector3(row2),
                Data::Vector4Type::CreateAxisW());
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromMatrix3x3, k_categoryName, "{C7EB3FB9-CAA7-48B3-BFCA-F03BF14E9778}", "returns a matrix from the from the Matrix3x3", "Source");

        AZ_INLINE Data::Matrix4x4Type FromQuaternion(const Data::QuaternionType& source)
        {
            return Data::Matrix4x4Type::CreateFromQuaternion(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromQuaternion, k_categoryName, "{B838FE73-B028-45E0-B847-1621B22A2185}", "returns a rotation matrix using the supplied quaternion", "Source");

        AZ_INLINE Data::Matrix4x4Type FromQuaternionAndTranslation(const Data::QuaternionType& rotation, const Data::Vector3Type& translation)
        {
            return Data::Matrix4x4Type::CreateFromQuaternionAndTranslation(rotation, translation);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromQuaternionAndTranslation, k_categoryName, "{2B16CA4D-8F05-4917-B708-BEF24CBFB1EB}", "returns a skew-symmetric cross product matrix based on supplied vector", "Rotation", "Translation");

        AZ_INLINE Data::Matrix4x4Type FromProjection(const Data::NumberType fovY, const Data::NumberType aspectRatio, const Data::NumberType nearDist, const Data::NumberType farDist)
        {
            return Data::Matrix4x4Type::CreateProjection(aznumeric_cast<float>(fovY), aznumeric_cast<float>(aspectRatio), aznumeric_cast<float>(nearDist), aznumeric_cast<float>(farDist));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromProjection, k_categoryName, "{8684FF66-D8EE-4AB7-B9A2-3EE833ADFF87}", "returns a projection matrix using the supplied vertical FOV and aspect ratio", "Vertical FOV", "Aspect Ratio", "Near", "Far");

        AZ_INLINE Data::Matrix4x4Type FromProjectionFov(const Data::NumberType fovX, const Data::NumberType fovY, const Data::NumberType nearDist, const Data::NumberType farDist)
        {
            return Data::Matrix4x4Type::CreateProjectionFov(aznumeric_cast<float>(fovX), aznumeric_cast<float>(fovY), aznumeric_cast<float>(nearDist), aznumeric_cast<float>(farDist));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromProjectionFov, k_categoryName, "{3525FAB0-F490-4081-BC63-F5713A637383}", "returns a projection matrix using the supplied vertical FOV and horizontal FOV", "Vertical FOV", "Horizontal FOV", "Near", "Far");

        AZ_INLINE Data::Matrix4x4Type FromProjectionVolume(const Data::NumberType left, const Data::NumberType right, const Data::NumberType bottom, const Data::NumberType top, const Data::NumberType nearDist, const Data::NumberType farDist)
        {
            return Data::Matrix4x4Type::CreateProjectionOffset(aznumeric_cast<float>(left), aznumeric_cast<float>(right), aznumeric_cast<float>(bottom), aznumeric_cast<float>(top), aznumeric_cast<float>(nearDist), aznumeric_cast<float>(farDist));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromProjectionVolume, k_categoryName, "{39F295E7-E835-4491-A5F0-5B44C3E47BDF}", "returns a projection matrix center around the (left, right, bottom, top) values", "Left", "Right", "Bottom", "Top", "Near", "Far");

        AZ_INLINE Data::Matrix4x4Type FromRotationXDegrees(const Data::NumberType angle)
        {
            return Data::Matrix4x4Type::CreateRotationX(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromRotationXDegrees, k_categoryName, "{07107536-86B0-4927-A304-DF580419F839}", "returns a rotation matrix representing a rotation in degrees around X-axis", "Degrees");

        AZ_INLINE Data::Matrix4x4Type FromRotationYDegrees(const Data::NumberType angle)
        {
            return Data::Matrix4x4Type::CreateRotationY(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromRotationYDegrees, k_categoryName, "{11323B35-6BB0-4402-BDD6-98BE75E6CF99}", "returns a rotation matrix representing a rotation in degrees around Y-axis", "Degrees");

        AZ_INLINE Data::Matrix4x4Type FromRotationZDegrees(const Data::NumberType angle)
        {
            return Data::Matrix4x4Type::CreateRotationZ(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromRotationZDegrees, k_categoryName, "{1D520FBD-EB39-4EA0-BFA0-CE5834C076F2}", "returns a rotation matrix representing a rotation in degrees around Z-axis", "Degrees");

        AZ_INLINE Data::Matrix4x4Type FromRows(const Data::Vector4Type& row0, const Data::Vector4Type& row1, const Data::Vector4Type& row2, const Data::Vector4Type& row3)
        {
            return Data::Matrix4x4Type::CreateFromRows(row0, row1, row2, row3);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromRows, k_categoryName, "{F62339F3-0F88-4C23-86DA-920540488A46}", "returns a matrix from three row", "Row1", "Row2", "Row3", "Row4");

        AZ_INLINE Data::Matrix4x4Type FromScale(const Data::Vector3Type& source)
        {
            return Data::Matrix4x4Type::CreateScale(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromScale, k_categoryName, "{017C6716-DCEE-4D49-A593-4F6B9B18954F}", "returns a scale matrix using the supplied vector", "Scale");

        AZ_INLINE Data::Matrix4x4Type FromTranslation(const Data::Vector3Type& source)
        {
            return Data::Matrix4x4Type::CreateTranslation(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromTranslation, k_categoryName, "{70E5B6E4-D071-4CD5-BDA0-90C8885B38A4}", "returns a skew-symmetric cross product matrix based on supplied vector", "Source");

        AZ_INLINE Data::Matrix4x4Type FromTransform(const Data::TransformType& source)
        {
            return Data::Matrix4x4Type::CreateFromTransform(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromTransform, k_categoryName, "{34E3A8AF-D6B4-4841-A63B-A28B0DF07C1E}", "returns a matrix using the supplied transform", "Transform");

        AZ_INLINE Data::Vector4Type GetColumn(const Data::Matrix4x4Type& source, Data::NumberType col)
        {
            const AZ::s32 numColumns(4);
            const auto colIndex = aznumeric_cast<AZ::s32>(col);
            return colIndex >= 0 && colIndex < numColumns ? source.GetColumn(colIndex) : Data::Vector4Type::CreateZero();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetColumn, k_categoryName, "{0FB960BE-8BDE-487F-A153-EDC5E22B1C02}", "returns vector from matrix corresponding to the Column index", "Source", "Column");

        AZ_INLINE std::tuple<Data::Vector4Type, Data::Vector4Type, Data::Vector4Type, Data::Vector4Type> GetColumns(const Data::Matrix4x4Type& source)
        {
            Data::Vector4Type col0, col1, col2, col3;
            source.GetColumns(&col0, &col1, &col2, &col3);
            return std::make_tuple(col0, col1, col2, col3);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(GetColumns, k_categoryName, "{7F1BDAEE-2BEE-4029-AC97-B5922A3CD3D4}", "returns all columns from matrix", "Source", "Column1", "Column2", "Column3", "Column4");

        AZ_INLINE Data::Vector4Type GetDiagonal(const Data::Matrix4x4Type& source)
        {
            return source.GetDiagonal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetDiagonal, k_categoryName, "{DA8E7E0C-7702-4E34-81EA-1F40B03356A3}", "returns vector of matrix diagonal values", "Source");

        AZ_INLINE Data::NumberType GetElement(const Data::Matrix4x4Type& source, Data::NumberType row, Data::NumberType col)
        {
            const AZ::s32 numRows(4), numColumns(4);
            const auto rowIndex = aznumeric_cast<AZ::s32>(row);
            const auto colIndex = aznumeric_cast<AZ::s32>(col);
            return rowIndex >= 0 && rowIndex < numRows && colIndex >= 0 && colIndex < numColumns ? source.GetElement(rowIndex, colIndex) : Data::NumberType();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetElement, k_categoryName, "{66428D28-2FE8-473E-B5B8-A2B4451CA4F3}", "returns scalar from matrix corresponding to the (Row,Column) pair", "Source", "Row", "Column");

        AZ_INLINE Data::Vector4Type GetRow(const Data::Matrix4x4Type& source, Data::NumberType row)
        {
            const AZ::s32 numRows(4);
            const auto rowIndex = aznumeric_cast<AZ::s32>(row);
            return rowIndex >= 0 && rowIndex < numRows ? source.GetRow(rowIndex) : Data::Vector4Type::CreateZero();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetRow, k_categoryName, "{D96FCA17-206B-4A19-A08B-28BB4FDBF75C}", "returns vector from matrix corresponding to the Row index", "Source", "Row");

        AZ_INLINE Data::Vector3Type GetTranslation(const Data::Matrix4x4Type& source)
        {
            return source.GetTranslation();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetTranslation, k_categoryName, "{D69E296E-FC38-4273-8619-FC7D2F29BC19}", "returns translation vector from the matrix", "Source");

        AZ_INLINE std::tuple<Data::Vector4Type, Data::Vector4Type, Data::Vector4Type, Data::Vector4Type> GetRows(const Data::Matrix4x4Type& source)
        {
            Data::Vector4Type row0, row1, row2, row3;
            source.GetRows(&row0, &row1, &row2, &row3);
            return std::make_tuple(row0, row1, row2, row3);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(GetRows, k_categoryName, "{594636A1-749C-4821-A0D3-ED43E112601E}", "returns all rows from matrix", "Source", "Row1", "Row2", "Row3", "Row4");

        AZ_INLINE Data::Matrix4x4Type Invert(const Data::Matrix4x4Type& source)
        {
            return source.GetInverseFull();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Invert, k_categoryName, "{3C691878-0E1C-4D6A-8226-070A31F6FFD8}", "returns inverse of Matrix", "Source");

        AZ_INLINE Data::BooleanType IsClose(const Data::Matrix4x4Type& lhs, const Data::Matrix4x4Type& rhs, const Data::NumberType tolerance)
        {
            return lhs.IsClose(rhs, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsClose, MathNodeUtilities::DefaultToleranceSIMD<2>, k_categoryName, "{D14CE5BA-67A8-44C8-8E5B-F19F270ACB0E}",
            "returns true if each element of both Matrix are equal within some tolerance", "A", "B", "Tolerance");

        AZ_INLINE Data::BooleanType IsFinite(const Data::Matrix4x4Type& source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsFinite, k_categoryName, "{5B9B6735-3593-41CE-9329-36F8B96893A2}", "returns true if all numbers in matrix is finite", "Source");

        AZ_INLINE Data::Matrix4x4Type MultiplyByMatrix(const Data::Matrix4x4Type& lhs, const Data::Matrix4x4Type& rhs)
        {
            return lhs * rhs;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(MultiplyByMatrix, k_categoryName, "{72A041D0-BEB3-415B-9771-E869797B293B}", "This node is deprecated, use Multiply (*), it provides contextual type and slots", "A", "B");

        AZ_INLINE Data::Vector4Type MultiplyByVector(const Data::Matrix4x4Type& lhs, const Data::Vector4Type& rhs)
        {
            return lhs * rhs;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyByVector, k_categoryName, "{5D5CEC9B-DBBD-4C43-BC01-8C637A4B1F14}", "returns vector created by right left multiplying matrix by supplied vector", "Source", "Vector");

        AZ_INLINE Data::Vector3Type ToScale(const Data::Matrix4x4Type& source)
        {
            return source.RetrieveScale();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ToScale, k_categoryName, "{A6B79A2C-4B73-4F62-81E4-28F5A013FCEE}", "returns scale part of the transformation matrix", "Source");

        AZ_INLINE Data::Matrix4x4Type Transpose(const Data::Matrix4x4Type& source)
        {
            return source.GetTranspose();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Transpose, k_categoryName, "{B93735CF-F56B-4BB4-B10B-1CEEB0EB2C4A}", "returns transpose of Matrix", "Source");

        AZ_INLINE Data::Matrix4x4Type Zero()
        {
            return Data::Matrix4x4Type::CreateZero();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Zero, k_categoryName, "{E9EE8687-3A98-415C-A8D5-CACF34C056EA}", "returns the zero matrix");

        using Registrar = RegistrarGeneric<
            FromColumnsNode,
            FromDiagonalNode,
            FromMatrix3x3Node,

#if ENABLE_EXTENDED_MATH_SUPPORT
            FromProjectionFovNode,
            FromProjectionNode,
            FromProjectionVolumeNode,
#endif

            FromQuaternionAndTranslationNode,
            FromQuaternionNode,
            FromRotationXDegreesNode,            
            FromRotationYDegreesNode,
            FromRotationZDegreesNode,
            FromRowsNode,
            FromScaleNode,
            FromTransformNode,
            FromTranslationNode,
            GetColumnNode,
            GetColumnsNode,
            GetDiagonalNode,
            GetElementNode,
            GetRowNode,
            GetRowsNode,
            GetTranslationNode,
            InvertNode,
            IsCloseNode,
            IsFiniteNode,
            MultiplyByMatrixNode,
            MultiplyByVectorNode,
            ToScaleNode,
            TransposeNode,
            ZeroNode
        > ;
    }
} 

