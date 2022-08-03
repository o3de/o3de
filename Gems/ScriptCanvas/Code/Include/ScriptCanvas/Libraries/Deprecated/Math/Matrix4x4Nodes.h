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
    //! Matrix4x4Nodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace Matrix4x4Nodes
    {
        static constexpr const char* k_categoryName = "Math/Matrix4x4";

        AZ_INLINE Data::Matrix4x4Type FromColumns(const Data::Vector4Type& col0, const Data::Vector4Type& col1, const Data::Vector4Type& col2, const Data::Vector4Type& col3)
        {
            return Data::Matrix4x4Type::CreateFromColumns(col0, col1, col2, col3);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromColumns,
            k_categoryName, "{7A5CDFCC-9BD1-493F-8959-B1868302CD1D}", "ScriptCanvas_Matrix4x4Functions_FromColumns");

        AZ_INLINE Data::Matrix4x4Type FromDiagonal(const Data::Vector4Type& source)
        {
            return Data::Matrix4x4Type::CreateDiagonal(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromDiagonal,
            k_categoryName, "{24F755B3-230C-45FE-86FC-C6A0ED219C9D}", "ScriptCanvas_Matrix4x4Functions_FromDiagonal");

        AZ_INLINE Data::Matrix4x4Type FromMatrix3x3(const Data::Matrix3x3Type& source)
        {
            Data::Vector3Type row0, row1, row2;
            source.GetRows(&row0, &row1, &row2);
            return Data::Matrix4x4Type::CreateFromRows(Data::Vector4Type::CreateFromVector3(row0),
                Data::Vector4Type::CreateFromVector3(row1),
                Data::Vector4Type::CreateFromVector3(row2),
                Data::Vector4Type::CreateAxisW());
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromMatrix3x3,
            k_categoryName, "{C7EB3FB9-CAA7-48B3-BFCA-F03BF14E9778}", "ScriptCanvas_Matrix4x4Functions_FromMatrix3x3");

        AZ_INLINE Data::Matrix4x4Type FromQuaternion(const Data::QuaternionType& source)
        {
            return Data::Matrix4x4Type::CreateFromQuaternion(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromQuaternion,
            k_categoryName, "{B838FE73-B028-45E0-B847-1621B22A2185}", "ScriptCanvas_Matrix4x4Functions_FromQuaternion");

        AZ_INLINE Data::Matrix4x4Type FromQuaternionAndTranslation(const Data::QuaternionType& rotation, const Data::Vector3Type& translation)
        {
            return Data::Matrix4x4Type::CreateFromQuaternionAndTranslation(rotation, translation);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromQuaternionAndTranslation,
            k_categoryName, "{2B16CA4D-8F05-4917-B708-BEF24CBFB1EB}", "ScriptCanvas_Matrix4x4Functions_FromQuaternionAndTranslation");

        AZ_INLINE Data::Matrix4x4Type FromRotationXDegrees(const Data::NumberType angle)
        {
            return Data::Matrix4x4Type::CreateRotationX(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromRotationXDegrees,
            k_categoryName, "{07107536-86B0-4927-A304-DF580419F839}", "ScriptCanvas_Matrix4x4Functions_FromRotationXDegrees");

        AZ_INLINE Data::Matrix4x4Type FromRotationYDegrees(const Data::NumberType angle)
        {
            return Data::Matrix4x4Type::CreateRotationY(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromRotationYDegrees,
            k_categoryName, "{11323B35-6BB0-4402-BDD6-98BE75E6CF99}", "ScriptCanvas_Matrix4x4Functions_FromRotationYDegrees");

        AZ_INLINE Data::Matrix4x4Type FromRotationZDegrees(const Data::NumberType angle)
        {
            return Data::Matrix4x4Type::CreateRotationZ(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromRotationZDegrees,
            k_categoryName, "{1D520FBD-EB39-4EA0-BFA0-CE5834C076F2}", "ScriptCanvas_Matrix4x4Functions_FromRotationZDegrees");

        AZ_INLINE Data::Matrix4x4Type FromRows(const Data::Vector4Type& row0, const Data::Vector4Type& row1, const Data::Vector4Type& row2, const Data::Vector4Type& row3)
        {
            return Data::Matrix4x4Type::CreateFromRows(row0, row1, row2, row3);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromRows,
            k_categoryName, "{F62339F3-0F88-4C23-86DA-920540488A46}", "ScriptCanvas_Matrix4x4Functions_FromRows");

        AZ_INLINE Data::Matrix4x4Type FromScale(const Data::Vector3Type& source)
        {
            return Data::Matrix4x4Type::CreateScale(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromScale,
            k_categoryName, "{017C6716-DCEE-4D49-A593-4F6B9B18954F}", "ScriptCanvas_Matrix4x4Functions_FromScale");

        AZ_INLINE Data::Matrix4x4Type FromTranslation(const Data::Vector3Type& source)
        {
            return Data::Matrix4x4Type::CreateTranslation(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromTranslation,
            k_categoryName, "{70E5B6E4-D071-4CD5-BDA0-90C8885B38A4}", "ScriptCanvas_Matrix4x4Functions_FromTranslation");

        AZ_INLINE Data::Matrix4x4Type FromTransform(const Data::TransformType& source)
        {
            return Data::Matrix4x4Type::CreateFromTransform(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromTransform,
            k_categoryName, "{34E3A8AF-D6B4-4841-A63B-A28B0DF07C1E}", "ScriptCanvas_Matrix4x4Functions_FromTransform");

        AZ_INLINE Data::Vector4Type GetColumn(const Data::Matrix4x4Type& source, Data::NumberType col)
        {
            const AZ::s32 numColumns(4);
            const auto colIndex = aznumeric_cast<AZ::s32>(col);
            return colIndex >= 0 && colIndex < numColumns ? source.GetColumn(colIndex) : Data::Vector4Type::CreateZero();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetColumn,
            k_categoryName, "{0FB960BE-8BDE-487F-A153-EDC5E22B1C02}", "ScriptCanvas_Matrix4x4Functions_GetColumn");

        AZ_INLINE std::tuple<Data::Vector4Type, Data::Vector4Type, Data::Vector4Type, Data::Vector4Type> GetColumns(const Data::Matrix4x4Type& source)
        {
            Data::Vector4Type col0, col1, col2, col3;
            source.GetColumns(&col0, &col1, &col2, &col3);
            return std::make_tuple(col0, col1, col2, col3);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetColumns,
            k_categoryName, "{7F1BDAEE-2BEE-4029-AC97-B5922A3CD3D4}", "ScriptCanvas_Matrix4x4Functions_GetColumns");

        AZ_INLINE Data::Vector4Type GetDiagonal(const Data::Matrix4x4Type& source)
        {
            return source.GetDiagonal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetDiagonal, k_categoryName, "{DA8E7E0C-7702-4E34-81EA-1F40B03356A3}", "ScriptCanvas_Matrix4x4Functions_GetDiagonal");

        AZ_INLINE Data::NumberType GetElement(const Data::Matrix4x4Type& source, Data::NumberType row, Data::NumberType col)
        {
            const AZ::s32 numRows(4), numColumns(4);
            const auto rowIndex = aznumeric_cast<AZ::s32>(row);
            const auto colIndex = aznumeric_cast<AZ::s32>(col);
            return rowIndex >= 0 && rowIndex < numRows && colIndex >= 0 && colIndex < numColumns ? source.GetElement(rowIndex, colIndex) : Data::NumberType();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetElement,
            k_categoryName, "{66428D28-2FE8-473E-B5B8-A2B4451CA4F3}", "ScriptCanvas_Matrix4x4Functions_GetElement");

        AZ_INLINE Data::Vector4Type GetRow(const Data::Matrix4x4Type& source, Data::NumberType row)
        {
            const AZ::s32 numRows(4);
            const auto rowIndex = aznumeric_cast<AZ::s32>(row);
            return rowIndex >= 0 && rowIndex < numRows ? source.GetRow(rowIndex) : Data::Vector4Type::CreateZero();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetRow,
            k_categoryName, "{D96FCA17-206B-4A19-A08B-28BB4FDBF75C}", "ScriptCanvas_Matrix4x4Functions_GetRow");

        AZ_INLINE Data::Vector3Type GetTranslation(const Data::Matrix4x4Type& source)
        {
            return source.GetTranslation();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetTranslation,
            k_categoryName, "{D69E296E-FC38-4273-8619-FC7D2F29BC19}", "ScriptCanvas_Matrix4x4Functions_GetTranslation");

        AZ_INLINE std::tuple<Data::Vector4Type, Data::Vector4Type, Data::Vector4Type, Data::Vector4Type> GetRows(const Data::Matrix4x4Type& source)
        {
            Data::Vector4Type row0, row1, row2, row3;
            source.GetRows(&row0, &row1, &row2, &row3);
            return std::make_tuple(row0, row1, row2, row3);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetRows,
            k_categoryName, "{594636A1-749C-4821-A0D3-ED43E112601E}", "ScriptCanvas_Matrix4x4Functions_GetRows");

        AZ_INLINE Data::Matrix4x4Type Invert(const Data::Matrix4x4Type& source)
        {
            return source.GetInverseFull();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Invert, k_categoryName, "{3C691878-0E1C-4D6A-8226-070A31F6FFD8}", "ScriptCanvas_Matrix4x4Functions_Invert");

        AZ_INLINE Data::BooleanType IsClose(const Data::Matrix4x4Type& lhs, const Data::Matrix4x4Type& rhs, const Data::NumberType tolerance)
        {
            return lhs.IsClose(rhs, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsClose, k_categoryName, "{D14CE5BA-67A8-44C8-8E5B-F19F270ACB0E}", "ScriptCanvas_Matrix4x4Functions_IsClose");

        AZ_INLINE Data::BooleanType IsFinite(const Data::Matrix4x4Type& source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsFinite,
            k_categoryName, "{5B9B6735-3593-41CE-9329-36F8B96893A2}", "ScriptCanvas_Matrix4x4Functions_IsFinite");

        AZ_INLINE Data::Vector4Type MultiplyByVector(const Data::Matrix4x4Type& lhs, const Data::Vector4Type& rhs)
        {
            return lhs * rhs;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            MultiplyByVector,
            k_categoryName, "{5D5CEC9B-DBBD-4C43-BC01-8C637A4B1F14}", "ScriptCanvas_Matrix4x4Functions_MultiplyByVector");

        AZ_INLINE Data::Vector3Type ToScale(const Data::Matrix4x4Type& source)
        {
            return source.RetrieveScale();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            ToScale, k_categoryName, "{A6B79A2C-4B73-4F62-81E4-28F5A013FCEE}", "ScriptCanvas_Matrix4x4Functions_ToScale");

        AZ_INLINE Data::Matrix4x4Type Transpose(const Data::Matrix4x4Type& source)
        {
            return source.GetTranspose();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Transpose, k_categoryName, "{B93735CF-F56B-4BB4-B10B-1CEEB0EB2C4A}", "ScriptCanvas_Matrix4x4Functions_Transpose");

        AZ_INLINE Data::Matrix4x4Type Zero()
        {
            return Data::Matrix4x4Type::CreateZero();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Zero, k_categoryName, "{E9EE8687-3A98-415C-A8D5-CACF34C056EA}", "ScriptCanvas_Matrix4x4Functions_Zero");

        using Registrar = RegistrarGeneric<
            FromColumnsNode,
            FromDiagonalNode,
            FromMatrix3x3Node,
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
            MultiplyByVectorNode,
            ToScaleNode,
            TransposeNode,
            ZeroNode
        > ;
    }
} 

