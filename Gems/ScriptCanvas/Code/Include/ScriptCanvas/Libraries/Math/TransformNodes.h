/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/NumericData.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvas
{
    namespace TransformNodes
    {
        using namespace Data;
        using namespace MathNodeUtilities;
        static constexpr const char* k_categoryName = "Math/Transform";

        AZ_INLINE std::tuple<NumberType, TransformType> ExtractUniformScale(TransformType source)
        {
            auto scale(source.ExtractUniformScale());
            return std::make_tuple(scale, source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(ExtractUniformScale, k_categoryName, "{8DFE5247-0950-4CD1-87E6-0CAAD42F1637}", "returns the uniform scale as a float, and a transform with the scale extracted ", "Source", "Uniform Scale", "Extracted");

        AZ_INLINE TransformType FromMatrix3x3(Matrix3x3Type source)
        {
            return TransformType::CreateFromMatrix3x3(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromMatrix3x3, k_categoryName, "{DA430502-CF75-41BA-BA41-6701994EFB64}", "returns a transform with from 3x3 matrix and with the translation set to zero", "Source");

        AZ_INLINE TransformType FromMatrix3x3AndTranslation(Matrix3x3Type matrix, Vector3Type translation)
        {
            return TransformType::CreateFromMatrix3x3AndTranslation(matrix, translation);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromMatrix3x3AndTranslation, k_categoryName, "{AD0725EB-0FF0-4F99-A45F-C3F8CBABF11D}", "returns a transform from the 3x3 matrix and the translation", "Matrix", "Translation");

        AZ_INLINE TransformType FromRotation(QuaternionType rotation)
        {
            return TransformType::CreateFromQuaternion(rotation);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromRotation, k_categoryName, "{8BBF4F22-EA7D-4E7B-81FD-7D11CA237BA6}", "returns a transform from the rotation and with the translation set to zero", "Source");

        AZ_INLINE TransformType FromRotationAndTranslation(QuaternionType rotation, Vector3Type translation)
        {
            return TransformType::CreateFromQuaternionAndTranslation(rotation, translation);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromRotationAndTranslation, k_categoryName, "{99A4D55D-6EFB-4E24-8113-F5B46DE3A194}", "returns a transform from the rotation and the translation", "Rotation", "Translation");

        AZ_INLINE TransformType FromScale(NumberType scale)
        {
            return TransformType::CreateUniformScale(static_cast<float>(scale));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromScale, k_categoryName, "{4B6454BC-015C-41BB-9C78-34ADBCF70187}", "returns a transform which applies the specified uniform Scale, but no rotation or translation", "Scale");

        AZ_INLINE TransformType FromTranslation(Vector3Type translation)
        {
            return TransformType::CreateTranslation(translation);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromTranslation, k_categoryName, "{A60083C8-AEEC-456E-A3F5-75D0E0D094E1}", "returns a translation matrix and the rotation set to zero", "Translation");

        template<int t_Index>
        AZ_INLINE void DefaultScale(Node& node) { SetDefaultValuesByIndex<t_Index>::_(node, Data::One()); }

        AZ_INLINE Vector3Type GetRight(const TransformType& source, NumberType scale)
        {
            Vector3Type vector = source.GetBasisX();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(GetRight, DefaultScale<1>, k_categoryName, "{65811752-711F-4566-869E-5AEF53206342}", "returns the right direction vector from the specified transform scaled by a given value (O3DE uses Z up, right handed)", "Source", "Scale");

        AZ_INLINE Vector3Type GetForward(const TransformType& source, NumberType scale)
        {
            Vector3Type vector = source.GetBasisY();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(GetForward, DefaultScale<1>, k_categoryName, "{3602a047-9f12-46d4-9648-8f53770c8130}", "returns the forward direction vector from the specified transform scaled by a given value (O3DE uses Z up, right handed)", "Source", "Scale");

        AZ_INLINE Vector3Type GetUp(const TransformType& source, NumberType scale)
        {
            Vector3Type vector = source.GetBasisZ();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(GetUp, DefaultScale<1>, k_categoryName, "{F10F52D2-E6F2-4E39-84D5-B4A561F186D3}", "returns the up direction vector from the specified transform scaled by a given value (O3DE uses Z up, right handed)", "Source", "Scale");

        AZ_INLINE Vector3Type GetTranslation(const TransformType& source)
        {
            return source.GetTranslation();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetTranslation, k_categoryName, "{6C2AC46D-C92C-4A64-A2EB-48DA52002B8A}", "returns the translation of Source", "Source");

        AZ_INLINE TransformType InvertOrthogonal(const TransformType& source)
        {
            return source.GetInverse();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(InvertOrthogonal, k_categoryName, "{635F8FD0-6B16-4622-A893-463422D817CF}", "returns the inverse of the source assuming it only contains an orthogonal matrix, faster then InvertSlow, but won't handle scale, or skew.", "Source");

        AZ_INLINE BooleanType IsClose(const TransformType& a, const TransformType& b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsClose, DefaultToleranceSIMD<2>, k_categoryName, "{52914912-5C4A-48A5-A675-11CF15B5FB4B}", "returns true if every row of A is within Tolerance of corresponding row in B, else false", "A", "B", "Tolerance");

        AZ_INLINE BooleanType IsFinite(const TransformType& source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsFinite, k_categoryName, "{B7D23934-0101-40B9-80E8-3D88C8580B25}", "returns true if every row of source is finite, else false", "Source");

        AZ_INLINE BooleanType IsOrthogonal(const TransformType& source, NumberType tolerance)
        {
            return source.IsOrthogonal(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsOrthogonal, DefaultToleranceSIMD<1>, k_categoryName, "{9A143AC1-ED6B-4D96-939E-40D9F6D01A76}", "returns true if the upper 3x3 matrix of Source is within Tolerance of orthogonal, else false", "Source", "Tolerance");

        AZ_INLINE TransformType ModRotation(const TransformType& source, const Matrix3x3Type& rotation)
        {
            return TransformType::CreateFromMatrix3x3AndTranslation(rotation, source.GetTranslation());
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModRotation, k_categoryName, "{ECC408EB-32D7-4DA8-A907-3DB36E8E54A9}", "returns the transform with translation from Source, and rotation from Rotation", "Source", "Rotation");

        AZ_INLINE TransformType ModTranslation(TransformType source, Vector3Type translation)
        {
            source.SetTranslation(translation);
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModTranslation, k_categoryName, "{27BF9798-A6B3-4C2C-B19E-2AF90434090A}", "returns the transform with rotation from Source, and translation from Translation", "Source", "Translation");

        AZ_INLINE Vector3Type Multiply3x3ByVector3(const TransformType& source, const Vector3Type multiplier)
        {
            return source.TransformVector(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Multiply3x3ByVector3, k_categoryName, "{4F2ABFC6-2E93-4A9D-8639-C7967DB318DB}", "returns Source's 3x3 upper matrix post multiplied by Multiplier", "Source", "Multiplier");

        AZ_INLINE TransformType MultiplyByUniformScale(TransformType source, NumberType scale)
        {
            source.MultiplyByUniformScale(static_cast<float>(scale));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyByUniformScale, k_categoryName, "{90472D62-65A8-40C1-AB08-FA66D793F689}", "returns Source multiplied uniformly by Scale", "Source", "Scale");

        AZ_INLINE TransformType MultiplyByTransform(const TransformType& a, const TransformType& b)
        {
            return a * b;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(MultiplyByTransform, k_categoryName, "{66C3FBB9-498E-4E96-8683-63843F28AFE9}", "This node is deprecated, use Multiply (*), it provides contextual type and slots", "A", "B");

        AZ_INLINE Vector3Type MultiplyByVector3(const TransformType& source, const Vector3Type multiplier)
        {
            return source.TransformPoint(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyByVector3, k_categoryName, "{147E4714-5028-49A3-A038-6BFB3ED45E0B}", "returns Source post multiplied by Multiplier", "Source", "Multiplier");

        AZ_INLINE Vector4Type MultiplyByVector4(const TransformType& source, const Vector4Type multiplier)
        {
            return source.TransformPoint(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyByVector4, k_categoryName, "{7E21DC19-C924-4479-817C-A942A52C8B20}", "returns Source post multiplied by Multiplier", "Source", "Multiplier");

        AZ_INLINE TransformType Orthogonalize(const TransformType& source)
        {
            return source.GetOrthogonalized();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Orthogonalize, k_categoryName, "{2B4140CD-6E22-44D3-BDB5-309E69FE7CC2}", "returns an orthogonal matrix if the Source is almost orthogonal", "Source");

        AZ_INLINE TransformType RotationXDegrees(NumberType degrees)
        {
            return TransformType::CreateRotationX(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RotationXDegrees, k_categoryName, "{1C43EF69-D4BD-46BD-BB91-3AC93ECB878C}", "returns a transform representing a rotation Degrees around the X-Axis", "Degrees");

        AZ_INLINE TransformType RotationYDegrees(NumberType degrees)
        {
            return TransformType::CreateRotationY(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RotationYDegrees, k_categoryName, "{0426C64C-CC1D-415A-8FA8-2267DE8CA317}", "returns a transform representing a rotation Degrees around the Y-Axis", "Degrees");

        AZ_INLINE TransformType RotationZDegrees(NumberType degrees)
        {
            return TransformType::CreateRotationZ(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RotationZDegrees, k_categoryName, "{F848306A-C07C-4586-B52F-BEEE489045D2}", "returns a transform representing a rotation Degrees around the Z-Axis", "Degrees");

        AZ_INLINE NumberType ToScale(const TransformType& source)
        {
            return source.GetUniformScale();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ToScale, k_categoryName, "{063C58AD-F567-464D-A432-F298FE3953A6}", "returns the uniform scale of the Source", "Source");

        using Registrar = RegistrarGeneric
            <
#if ENABLE_EXTENDED_MATH_SUPPORT
            ExtractUniformScaleNode,
#endif
            FromMatrix3x3AndTranslationNode
            , FromMatrix3x3Node
            , FromRotationAndTranslationNode
            , FromRotationNode
            , FromScaleNode
            , FromTranslationNode
            , GetTranslationNode
            , GetUpNode
            , GetRightNode
            , GetForwardNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , InvertOrthogonalNode
#endif

            , IsCloseNode
            , IsFiniteNode
            , IsOrthogonalNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , ModRotationNode
            , ModTranslationNode
            , Multiply3x3ByVector3Node
#endif

            , MultiplyByUniformScaleNode
            , MultiplyByTransformNode
            , MultiplyByVector3Node
            , MultiplyByVector4Node
            , OrthogonalizeNode
            , RotationXDegreesNode
            , RotationYDegreesNode
            , RotationZDegreesNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , ToDeterminant3x3Node
#endif

            , ToScaleNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , Transpose3x3Node
            , TransposedMultiply3x3Node
            , ZeroNode
#endif
            > ;

    }
}

