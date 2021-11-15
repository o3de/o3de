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
    namespace Matrix3x3Nodes
    {
        static constexpr const char* k_categoryName = "Math/Matrix3x3";

        AZ_INLINE Data::Matrix3x3Type Add(const Data::Matrix3x3Type& lhs, const Data::Matrix3x3Type& rhs)
        {
            return lhs + rhs; 
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Add, k_categoryName, "{B4F65ADE-9F4D-4134-BB8B-CDBFB75C9E42}", "This node is deprecated, use Add (+), it provides contextual type and slots", "A", "B");

        AZ_INLINE Data::Matrix3x3Type DivideByNumber(const Data::Matrix3x3Type& source, Data::NumberType divisor)
        {
            if (AZ::IsClose(divisor, Data::NumberType(0), std::numeric_limits<Data::NumberType>::epsilon()))
            {
                AZ_Error("Script Canvas", false, "Division by zero");
                return Data::Matrix3x3Type::CreateIdentity();
            }

            return source / aznumeric_cast<float>(divisor);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(DivideByNumber, k_categoryName, "{8B696031-B2C6-463C-A051-B875464859DC}", "returns matrix created from multiply the source matrix by 1/Divisor", "Source", "Divisor");

        AZ_INLINE Data::Matrix3x3Type FromColumns(const Data::Vector3Type& col0, const Data::Vector3Type& col1, const Data::Vector3Type& col2)
        {
            return Data::Matrix3x3Type::CreateFromColumns(col0, col1, col2);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromColumns, k_categoryName, "{B6DCF2BC-DA3C-4BBD-ACF1-1AEF69AC2345}", "returns a rotation matrix based on angle around Z axis", "Column1", "Column2", "Column3");

        AZ_INLINE Data::Matrix3x3Type FromCrossProduct(const Data::Vector3Type& source)
        {
            return Data::Matrix3x3Type::CreateCrossProduct(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromCrossProduct, k_categoryName, "{C44601FB-8AE2-4B68-9EEA-732F00906695}", "returns a skew-symmetric cross product matrix based on supplied vector", "Source");

        AZ_INLINE Data::Matrix3x3Type FromDiagonal(const Data::Vector3Type& source)
        {
            return Data::Matrix3x3Type::CreateDiagonal(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromDiagonal, k_categoryName, "{B32389F0-AE61-4CEA-838B-C61363358C76}", "returns a diagonal matrix using the supplied vector", "Source");

        AZ_INLINE Data::Matrix3x3Type FromMatrix4x4(const Data::Matrix4x4Type& source)
        {
            return Data::Matrix3x3Type::CreateFromMatrix4x4(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromMatrix4x4, k_categoryName, "{C72D9F70-8CAD-4379-97E4-00830F22EB6E}", "returns a matrix from the first 3 rows of a Matrix3x3", "Source");

        AZ_INLINE Data::Matrix3x3Type FromQuaternion(const Data::QuaternionType& source)
        {
            return Data::Matrix3x3Type::CreateFromQuaternion(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromQuaternion, k_categoryName, "{08EACF1F-E32F-4099-8F87-DFC9BF485FBA}", "returns a rotation matrix using the supplied quaternion", "Source");

        AZ_INLINE Data::Matrix3x3Type FromRotationXDegrees(const Data::NumberType angle)
        {
            return Data::Matrix3x3Type::CreateRotationX(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromRotationXDegrees, k_categoryName, "{D4808156-62E8-44B4-A39F-A5D2568C23FB}", "returns a rotation matrix representing a rotation in degrees around X-axis", "Degrees");

        AZ_INLINE Data::Matrix3x3Type FromRotationYDegrees(const Data::NumberType angle)
        {
            return Data::Matrix3x3Type::CreateRotationY(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromRotationYDegrees, k_categoryName, "{F0AD7957-987F-4DE3-92B3-9C56D8B2855A}", "returns a rotation matrix representing a rotation in degrees around Y-axis", "Degrees");

        AZ_INLINE Data::Matrix3x3Type FromRotationZDegrees(const Data::NumberType angle)
        {
            return Data::Matrix3x3Type::CreateRotationZ(AZ::DegToRad(aznumeric_cast<float>(angle)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromRotationZDegrees, k_categoryName, "{4E2E0AF5-4853-4315-B5FE-5973722BDE97}", "returns a rotation matrix representing a rotation in degrees around Z-axis", "Degrees");

        AZ_INLINE Data::Matrix3x3Type FromRows(const Data::Vector3Type& row0, const Data::Vector3Type& row1, const Data::Vector3Type& row2)
        {
            return Data::Matrix3x3Type::CreateFromRows(row0, row1, row2);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromRows, k_categoryName, "{8C303C56-000A-4C2E-8A50-9DEF58007C1E}", "returns a matrix from three row", "Row1", "Row2", "Row3");

        AZ_INLINE Data::Matrix3x3Type FromScale(const Data::Vector3Type& source)
        {
            return Data::Matrix3x3Type::CreateScale(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromScale, k_categoryName, "{C6833356-DBDE-477D-8E4E-CAB6FE918D01}", "returns a scale matrix using the supplied vector", "Scale");

        AZ_INLINE Data::Matrix3x3Type FromTransform(const Data::TransformType& source)
        {
            return Data::Matrix3x3Type::CreateFromTransform(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromTransform, k_categoryName, "{F570C56A-2E7B-4FB7-8E86-FC2C52C0F5E8}", "returns a matrix using the supplied transform", "Transform");

        AZ_INLINE Data::Vector3Type GetColumn(const Data::Matrix3x3Type& source, Data::NumberType col)
        {
            const AZ::s32 numColumns(3);
            const auto colIndex = aznumeric_cast<AZ::s32>(col);
            return colIndex >= 0 && colIndex < numColumns ? source.GetColumn(colIndex) : Data::Vector3Type::CreateZero();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetColumn, k_categoryName, "{43051443-0691-461D-9615-EE9FDA54C826}", "returns vector from matrix corresponding to the Column index", "Source", "Column");

        AZ_INLINE std::tuple<Data::Vector3Type, Data::Vector3Type, Data::Vector3Type> GetColumns(const Data::Matrix3x3Type& source)
        {
            Data::Vector3Type col0, col1, col2;
            source.GetColumns(&col0, &col1, &col2);
            return std::make_tuple(col0, col1, col2);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(GetColumns, k_categoryName, "{3F00356D-4435-4554-9363-7F9CA599D287}", "returns all columns from matrix", "Source", "Column1", "Column2", "Column3");

        AZ_INLINE Data::Vector3Type GetDiagonal(const Data::Matrix3x3Type& source)
        {
            return source.GetDiagonal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetDiagonal, k_categoryName, "{BC0FBCF4-E063-4705-A46F-0C9F9B39DFC5}", "returns vector of matrix diagonal values", "Source");

        AZ_INLINE Data::NumberType GetElement(const Data::Matrix3x3Type& source, Data::NumberType row, Data::NumberType col)
        {
            const AZ::s32 numRows(3), numColumns(3);
            const auto rowIndex = aznumeric_cast<AZ::s32>(row);
            const auto colIndex = aznumeric_cast<AZ::s32>(col);
            return rowIndex >= 0 && rowIndex < numRows && colIndex >= 0 && colIndex < numColumns ? source.GetElement(rowIndex, colIndex) : Data::NumberType();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetElement, k_categoryName, "{FA9C7B9F-ECEF-48FB-9AB0-8CAF07B2EF82}", "returns scalar from matrix corresponding to the (Row,Column) pair", "Source", "Row", "Column");

        AZ_INLINE Data::Vector3Type GetRow(const Data::Matrix3x3Type& source, Data::NumberType row)
        {
            const AZ::s32 numRows(3);
            const auto rowIndex = aznumeric_cast<AZ::s32>(row);
            return rowIndex >= 0 && rowIndex < numRows ? source.GetRow(rowIndex) : Data::Vector3Type::CreateZero();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetRow, k_categoryName, "{C4E00343-3642-4B09-8CFA-2D2F1CA6D595}", "returns vector from matrix corresponding to the Row index", "Source", "Row");

        AZ_INLINE std::tuple<Data::Vector3Type, Data::Vector3Type, Data::Vector3Type> GetRows(const Data::Matrix3x3Type& source)
        {
            Data::Vector3Type row0, row1, row2;
            source.GetRows(&row0, &row1, &row2);
            return std::make_tuple(row0, row1, row2);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(GetRows, k_categoryName, "{DDF76F4C-0C79-4856-B577-7DBA092CE59B}", "returns all rows from matrix", "Source", "Row1", "Row2", "Row3");

        AZ_INLINE Data::Matrix3x3Type Invert(const Data::Matrix3x3Type& source)
        {
            return source.GetInverseFull();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Invert, k_categoryName, "{E2771F5F-C674-4E7C-82F1-CD19C2798CA0}", "returns inverse of Matrix", "Source");

        AZ_INLINE Data::BooleanType IsClose(const Data::Matrix3x3Type& lhs, const Data::Matrix3x3Type& rhs, const Data::NumberType tolerance)
        {
            return lhs.IsClose(rhs, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsClose, MathNodeUtilities::DefaultToleranceSIMD<2>, k_categoryName, "{020C2517-F02F-4D7E-9FE9-B6E91E0D6D3F}",
            "returns true if each element of both Matrix are equal within some tolerance", "A", "B", "Tolerance");

        AZ_INLINE Data::BooleanType IsFinite(const Data::Matrix3x3Type& source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsFinite, k_categoryName, "{63CF6F9A-3DC4-4393-B63C-91FF1FDE540F}", "returns true if all numbers in matrix is finite", "Source");

        AZ_INLINE Data::BooleanType IsOrthogonal(const Data::Matrix3x3Type& source)
        {
            return source.IsOrthogonal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsOrthogonal, k_categoryName, "{1277CE0F-2F2F-4DF8-9505-90A721DED72F}", "returns true if the matrix is orthogonal", "Source");

        AZ_INLINE Data::Matrix3x3Type MultiplyByNumber(const Data::Matrix3x3Type& source, Data::NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyByNumber, k_categoryName, "{E74EFEA0-68B1-4490-BF65-2A2A33BDA666}", "returns matrix created from multiply the source matrix by Multiplier", "Source", "Multiplier");

        AZ_INLINE Data::Matrix3x3Type MultiplyByMatrix(const Data::Matrix3x3Type& lhs, const Data::Matrix3x3Type& rhs)
        {
            return lhs * rhs;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(MultiplyByMatrix, k_categoryName, "{B5DAE90C-7F12-4454-AF7F-36A288E8CB8F}", "This node is deprecated, use Multiply (*), it provides contextual type and slots", "A", "B");

        AZ_INLINE Data::Vector3Type MultiplyByVector(const Data::Matrix3x3Type& lhs, const Data::Vector3Type& rhs)
        {
            return lhs * rhs;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyByVector, k_categoryName, "{EBED6839-291D-4474-95E6-B810B3564774}", "returns vector created by right left multiplying matrix by supplied vector", "Source", "Vector");

        AZ_INLINE Data::Matrix3x3Type Orthogonalize(const Data::Matrix3x3Type& source)
        {
            return source.GetOrthogonalized();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Orthogonalize, k_categoryName, "{593599AB-BAE4-4DA3-B03F-238210204D26}", "returns an orthogonal matrix from the Source matrix", "Source");

        AZ_INLINE Data::Matrix3x3Type Subtract(const Data::Matrix3x3Type& lhs, const Data::Matrix3x3Type& rhs)
        {
            return lhs - rhs;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Subtract, k_categoryName, "{42FF001E-81CE-45CE-B47A-862A50AE4F0A}", "This node is deprecated, use Subtract (-), it provides contextual type and slots", "A", "B");

        AZ_INLINE Data::Matrix3x3Type ToAdjugate(const Data::Matrix3x3Type& source)
        {
            return source.GetAdjugate();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ToAdjugate, k_categoryName, "{4ECACBEB-714D-4DF6-823F-CE3329B4C705}", "returns the transpose of Matrix of cofactors", "Source");

        AZ_INLINE Data::NumberType ToDeterminant(const Data::Matrix3x3Type& source)
        {
            return source.GetDeterminant();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ToDeterminant, k_categoryName, "{3D7F3B86-63ED-4FC3-8862-2D2721D1D61B}", "returns determinant of Matrix", "Source", "Determinant");

        AZ_INLINE Data::Vector3Type ToScale(const Data::Matrix3x3Type& source)
        {
            return source.RetrieveScale();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ToScale, k_categoryName, "{B5099DB1-9072-4781-AC20-839367AB97D1}", "returns scale part of the transformation matrix", "Source");

        AZ_INLINE Data::Matrix3x3Type Transpose(const Data::Matrix3x3Type& source)
        {
            return source.GetTranspose();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Transpose, k_categoryName, "{99EDAA4D-A427-4404-925D-7F8FF41646FB}", "returns transpose of Matrix", "Source");

        AZ_INLINE Data::Matrix3x3Type Zero()
        {
            return Data::Matrix3x3Type::CreateZero();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Zero, k_categoryName, "{E5E7A9DA-0B7B-49BA-9350-D5876CF870E6}", "returns the zero matrix");

        using Registrar = RegistrarGeneric<
            AddNode,
            DivideByNumberNode,
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
            MultiplyByMatrixNode,
            MultiplyByNumberNode,
            MultiplyByVectorNode,
            OrthogonalizeNode,
            SubtractNode,
            ToAdjugateNode,
            ToDeterminantNode,
            ToScaleNode,
            TransposeNode,
            ZeroNode
        > ;
    }
} 

