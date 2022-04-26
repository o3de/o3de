/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvas
{
    //! Matrix3x3Nodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace Matrix3x3Nodes
    {
        static constexpr const char* k_categoryName = "Math/Matrix3x3";

        AZ_INLINE Data::Matrix3x3Type FromColumns(const Data::Vector3Type& col0, const Data::Vector3Type& col1, const Data::Vector3Type& col2)
        {
            return Data::Matrix3x3Type::CreateFromColumns(col0, col1, col2);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromColumns,
            k_categoryName, "{B6DCF2BC-DA3C-4BBD-ACF1-1AEF69AC2345}", "ScriptCanvas_Matrix3x3Functions_FromColumns");

        AZ_INLINE Data::Matrix3x3Type FromCrossProduct(const Data::Vector3Type& source)
        {
            return Data::Matrix3x3Type::CreateCrossProduct(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromCrossProduct,
            k_categoryName, "{C44601FB-8AE2-4B68-9EEA-732F00906695}", "ScriptCanvas_Matrix3x3Functions_FromCrossProduct");

        AZ_INLINE Data::Matrix3x3Type FromDiagonal(const Data::Vector3Type& source)
        {
            return Data::Matrix3x3Type::CreateDiagonal(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromDiagonal,
            k_categoryName, "{B32389F0-AE61-4CEA-838B-C61363358C76}", "ScriptCanvas_Matrix3x3Functions_FromDiagonal");

        AZ_INLINE Data::Matrix3x3Type FromMatrix4x4(const Data::Matrix4x4Type& source)
        {
            return Data::Matrix3x3Type::CreateFromMatrix4x4(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromMatrix4x4,
            k_categoryName, "{C72D9F70-8CAD-4379-97E4-00830F22EB6E}", "ScriptCanvas_Matrix3x3Functions_FromMatrix4x4");

        AZ_INLINE Data::Matrix3x3Type FromQuaternion(const Data::QuaternionType& source)
        {
            return Data::Matrix3x3Type::CreateFromQuaternion(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromQuaternion,
            k_categoryName, "{08EACF1F-E32F-4099-8F87-DFC9BF485FBA}", "ScriptCanvas_Matrix3x3Functions_FromQuaternion");

        AZ_INLINE Data::Matrix3x3Type FromRotationXDegrees(const Data::NumberType angle)
        {
            return Data::Matrix3x3Type::CreateRotationX(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromRotationXDegrees,
            k_categoryName, "{D4808156-62E8-44B4-A39F-A5D2568C23FB}", "ScriptCanvas_Matrix3x3Functions_FromRotationXDegrees");

        AZ_INLINE Data::Matrix3x3Type FromRotationYDegrees(const Data::NumberType angle)
        {
            return Data::Matrix3x3Type::CreateRotationY(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromRotationYDegrees,
            k_categoryName, "{F0AD7957-987F-4DE3-92B3-9C56D8B2855A}", "ScriptCanvas_Matrix3x3Functions_FromRotationYDegrees");

        AZ_INLINE Data::Matrix3x3Type FromRotationZDegrees(const Data::NumberType angle)
        {
            return Data::Matrix3x3Type::CreateRotationZ(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromRotationZDegrees,
            k_categoryName, "{4E2E0AF5-4853-4315-B5FE-5973722BDE97}", "ScriptCanvas_Matrix3x3Functions_FromRotationZDegrees");

        AZ_INLINE Data::Matrix3x3Type FromRows(const Data::Vector3Type& row0, const Data::Vector3Type& row1, const Data::Vector3Type& row2)
        {
            return Data::Matrix3x3Type::CreateFromRows(row0, row1, row2);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromRows, k_categoryName, "{8C303C56-000A-4C2E-8A50-9DEF58007C1E}", "ScriptCanvas_Matrix3x3Functions_FromRows");

        AZ_INLINE Data::Matrix3x3Type FromScale(const Data::Vector3Type& source)
        {
            return Data::Matrix3x3Type::CreateScale(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromScale,
            k_categoryName, "{C6833356-DBDE-477D-8E4E-CAB6FE918D01}", "ScriptCanvas_Matrix3x3Functions_FromScale");

        AZ_INLINE Data::Matrix3x3Type FromTransform(const Data::TransformType& source)
        {
            return Data::Matrix3x3Type::CreateFromTransform(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromTransform,
            k_categoryName, "{F570C56A-2E7B-4FB7-8E86-FC2C52C0F5E8}", "ScriptCanvas_Matrix3x3Functions_FromTransform");

        AZ_INLINE Data::Vector3Type GetColumn(const Data::Matrix3x3Type& source, Data::NumberType col)
        {
            const AZ::s32 numColumns(3);
            const auto colIndex = aznumeric_cast<AZ::s32>(col);
            return colIndex >= 0 && colIndex < numColumns ? source.GetColumn(colIndex) : Data::Vector3Type::CreateZero();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetColumn,
            k_categoryName, "{43051443-0691-461D-9615-EE9FDA54C826}", "ScriptCanvas_Matrix3x3Functions_GetColumn");

        AZ_INLINE std::tuple<Data::Vector3Type, Data::Vector3Type, Data::Vector3Type> GetColumns(const Data::Matrix3x3Type& source)
        {
            Data::Vector3Type col0, col1, col2;
            source.GetColumns(&col0, &col1, &col2);
            return std::make_tuple(col0, col1, col2);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetColumns,
            k_categoryName, "{3F00356D-4435-4554-9363-7F9CA599D287}", "ScriptCanvas_Matrix3x3Functions_GetColumns");

        AZ_INLINE Data::Vector3Type GetDiagonal(const Data::Matrix3x3Type& source)
        {
            return source.GetDiagonal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetDiagonal, k_categoryName, "{BC0FBCF4-E063-4705-A46F-0C9F9B39DFC5}", "ScriptCanvas_Matrix3x3Functions_GetDiagonal");

        AZ_INLINE Data::NumberType GetElement(const Data::Matrix3x3Type& source, Data::NumberType row, Data::NumberType col)
        {
            const AZ::s32 numRows(3), numColumns(3);
            const auto rowIndex = aznumeric_cast<AZ::s32>(row);
            const auto colIndex = aznumeric_cast<AZ::s32>(col);
            return rowIndex >= 0 && rowIndex < numRows && colIndex >= 0 && colIndex < numColumns ? source.GetElement(rowIndex, colIndex) : Data::NumberType();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetElement,
            k_categoryName, "{FA9C7B9F-ECEF-48FB-9AB0-8CAF07B2EF82}", "ScriptCanvas_Matrix3x3Functions_GetElement");

        AZ_INLINE Data::Vector3Type GetRow(const Data::Matrix3x3Type& source, Data::NumberType row)
        {
            const AZ::s32 numRows(3);
            const auto rowIndex = aznumeric_cast<AZ::s32>(row);
            return rowIndex >= 0 && rowIndex < numRows ? source.GetRow(rowIndex) : Data::Vector3Type::CreateZero();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetRow,
            k_categoryName, "{C4E00343-3642-4B09-8CFA-2D2F1CA6D595}", "ScriptCanvas_Matrix3x3Functions_GetRow");

        AZ_INLINE std::tuple<Data::Vector3Type, Data::Vector3Type, Data::Vector3Type> GetRows(const Data::Matrix3x3Type& source)
        {
            Data::Vector3Type row0, row1, row2;
            source.GetRows(&row0, &row1, &row2);
            return std::make_tuple(row0, row1, row2);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetRows,
            k_categoryName, "{DDF76F4C-0C79-4856-B577-7DBA092CE59B}", "ScriptCanvas_Matrix3x3Functions_GetRows");

        AZ_INLINE Data::Matrix3x3Type Invert(const Data::Matrix3x3Type& source)
        {
            return source.GetInverseFull();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Invert, k_categoryName, "{E2771F5F-C674-4E7C-82F1-CD19C2798CA0}", "ScriptCanvas_Matrix3x3Functions_Invert");

        AZ_INLINE Data::BooleanType IsClose(const Data::Matrix3x3Type& lhs, const Data::Matrix3x3Type& rhs, const Data::NumberType tolerance)
        {
            return lhs.IsClose(rhs, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsClose,
            k_categoryName, "{020C2517-F02F-4D7E-9FE9-B6E91E0D6D3F}", "ScriptCanvas_Matrix3x3Functions_IsClose");

        AZ_INLINE Data::BooleanType IsFinite(const Data::Matrix3x3Type& source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsFinite,
            k_categoryName, "{63CF6F9A-3DC4-4393-B63C-91FF1FDE540F}", "ScriptCanvas_Matrix3x3Functions_IsFinite");

        AZ_INLINE Data::BooleanType IsOrthogonal(const Data::Matrix3x3Type& source)
        {
            return source.IsOrthogonal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsOrthogonal, k_categoryName, "{1277CE0F-2F2F-4DF8-9505-90A721DED72F}", "ScriptCanvas_Matrix3x3Functions_IsOrthogonal");

        AZ_INLINE Data::Matrix3x3Type MultiplyByNumber(const Data::Matrix3x3Type& source, Data::NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            MultiplyByNumber,
            k_categoryName, "{E74EFEA0-68B1-4490-BF65-2A2A33BDA666}", "ScriptCanvas_Matrix3x3Functions_MultiplyByNumber");

        AZ_INLINE Data::Vector3Type MultiplyByVector(const Data::Matrix3x3Type& lhs, const Data::Vector3Type& rhs)
        {
            return lhs * rhs;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            MultiplyByVector,
            k_categoryName, "{EBED6839-291D-4474-95E6-B810B3564774}", "ScriptCanvas_Matrix3x3Functions_MultiplyByVector");

        AZ_INLINE Data::Matrix3x3Type Orthogonalize(const Data::Matrix3x3Type& source)
        {
            return source.GetOrthogonalized();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Orthogonalize,
            k_categoryName, "{593599AB-BAE4-4DA3-B03F-238210204D26}", "ScriptCanvas_Matrix3x3Functions_Orthogonalize");

        AZ_INLINE Data::Matrix3x3Type ToAdjugate(const Data::Matrix3x3Type& source)
        {
            return source.GetAdjugate();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            ToAdjugate, k_categoryName, "{4ECACBEB-714D-4DF6-823F-CE3329B4C705}", "ScriptCanvas_Matrix3x3Functions_ToAdjugate");

        AZ_INLINE Data::NumberType ToDeterminant(const Data::Matrix3x3Type& source)
        {
            return source.GetDeterminant();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            ToDeterminant,
            k_categoryName, "{3D7F3B86-63ED-4FC3-8862-2D2721D1D61B}", "ScriptCanvas_Matrix3x3Functions_ToDeterminant");

        AZ_INLINE Data::Vector3Type ToScale(const Data::Matrix3x3Type& source)
        {
            return source.RetrieveScale();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            ToScale, k_categoryName, "{B5099DB1-9072-4781-AC20-839367AB97D1}", "ScriptCanvas_Matrix3x3Functions_ToScale");

        AZ_INLINE Data::Matrix3x3Type Transpose(const Data::Matrix3x3Type& source)
        {
            return source.GetTranspose();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Transpose, k_categoryName, "{99EDAA4D-A427-4404-925D-7F8FF41646FB}", "ScriptCanvas_Matrix3x3Functions_Transpose");

        AZ_INLINE Data::Matrix3x3Type Zero()
        {
            return Data::Matrix3x3Type::CreateZero();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Zero, k_categoryName, "{E5E7A9DA-0B7B-49BA-9350-D5876CF870E6}", "ScriptCanvas_Matrix3x3Functions_Zero");

        using Registrar = RegistrarGeneric<
            FromColumnsNode,
            FromCrossProductNode,
            FromDiagonalNode,
            FromMatrix4x4Node,
            FromQuaternionNode,
            FromRotationXDegreesNode,
            FromRotationYDegreesNode,
            FromRotationZDegreesNode,
            FromRowsNode,
            FromScaleNode,
            FromTransformNode,
            GetColumnNode,
            GetColumnsNode,
            GetDiagonalNode,
            GetElementNode,
            GetRowNode,
            GetRowsNode,
            InvertNode,
            IsCloseNode,
            IsFiniteNode,
            IsOrthogonalNode,
            MultiplyByNumberNode,
            MultiplyByVectorNode,
            OrthogonalizeNode,
            ToAdjugateNode,
            ToDeterminantNode,
            ToScaleNode,
            TransposeNode,
            ZeroNode
        > ;
    }
} 

