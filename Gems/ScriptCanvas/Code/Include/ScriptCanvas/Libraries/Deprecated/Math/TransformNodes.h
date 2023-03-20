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
    //! TransformNodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace TransformNodes
    {
        using namespace Data;
        using namespace MathNodeUtilities;
        static constexpr const char* k_categoryName = "Math/Transform";

        AZ_INLINE TransformType FromMatrix3x3(Matrix3x3Type source)
        {
            return TransformType::CreateFromMatrix3x3(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromMatrix3x3,
            k_categoryName,
            "{DA430502-CF75-41BA-BA41-6701994EFB64}", "ScriptCanvas_TransformFunctions_FromMatrix3x3");

        AZ_INLINE TransformType FromMatrix3x3AndTranslation(Matrix3x3Type matrix, Vector3Type translation)
        {
            return TransformType::CreateFromMatrix3x3AndTranslation(matrix, translation);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromMatrix3x3AndTranslation,
            k_categoryName,
            "{AD0725EB-0FF0-4F99-A45F-C3F8CBABF11D}",
            "ScriptCanvas_TransformFunctions_FromMatrix3x3AndTranslation");

        AZ_INLINE TransformType FromRotation(QuaternionType rotation)
        {
            return TransformType::CreateFromQuaternion(rotation);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromRotation,
            k_categoryName,
            "{8BBF4F22-EA7D-4E7B-81FD-7D11CA237BA6}", "ScriptCanvas_TransformFunctions_FromRotation");

        AZ_INLINE TransformType FromRotationAndTranslation(QuaternionType rotation, Vector3Type translation)
        {
            return TransformType::CreateFromQuaternionAndTranslation(rotation, translation);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromRotationAndTranslation,
            k_categoryName,
            "{99A4D55D-6EFB-4E24-8113-F5B46DE3A194}",
            "ScriptCanvas_TransformFunctions_FromRotationAndTranslation");

        AZ_INLINE TransformType FromScale(NumberType scale)
        {
            return TransformType::CreateUniformScale(static_cast<float>(scale));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromScale,
            k_categoryName,
            "{4B6454BC-015C-41BB-9C78-34ADBCF70187}", "ScriptCanvas_TransformFunctions_FromScale");

        AZ_INLINE TransformType FromTranslation(Vector3Type translation)
        {
            return TransformType::CreateTranslation(translation);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromTranslation,
            k_categoryName,
            "{A60083C8-AEEC-456E-A3F5-75D0E0D094E1}", "ScriptCanvas_TransformFunctions_FromTranslation");

        AZ_INLINE Vector3Type GetRight(const TransformType& source, NumberType scale)
        {
            Vector3Type vector = source.GetBasisX();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetRight,
            k_categoryName,
            "{65811752-711F-4566-869E-5AEF53206342}", "ScriptCanvas_TransformFunctions_GetRight");

        AZ_INLINE Vector3Type GetForward(const TransformType& source, NumberType scale)
        {
            Vector3Type vector = source.GetBasisY();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetForward,
            k_categoryName,
            "{3602a047-9f12-46d4-9648-8f53770c8130}", "ScriptCanvas_TransformFunctions_GetForward");

        AZ_INLINE Vector3Type GetUp(const TransformType& source, NumberType scale)
        {
            Vector3Type vector = source.GetBasisZ();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetUp,
            k_categoryName,
            "{F10F52D2-E6F2-4E39-84D5-B4A561F186D3}", "ScriptCanvas_TransformFunctions_GetUp");

        AZ_INLINE Vector3Type GetTranslation(const TransformType& source)
        {
            return source.GetTranslation();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetTranslation, k_categoryName, "{6C2AC46D-C92C-4A64-A2EB-48DA52002B8A}", "ScriptCanvas_TransformFunctions_GetTranslation");

        AZ_INLINE BooleanType IsClose(const TransformType& a, const TransformType& b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsClose,
            k_categoryName,
            "{52914912-5C4A-48A5-A675-11CF15B5FB4B}", "ScriptCanvas_TransformFunctions_IsClose");

        AZ_INLINE BooleanType IsFinite(const TransformType& source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsFinite,
            k_categoryName,
            "{B7D23934-0101-40B9-80E8-3D88C8580B25}", "ScriptCanvas_TransformFunctions_IsFinite");

        AZ_INLINE BooleanType IsOrthogonal(const TransformType& source, NumberType tolerance)
        {
            return source.IsOrthogonal(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsOrthogonal,
            k_categoryName,
            "{9A143AC1-ED6B-4D96-939E-40D9F6D01A76}", "ScriptCanvas_TransformFunctions_IsOrthogonal");

        AZ_INLINE TransformType MultiplyByUniformScale(TransformType source, NumberType scale)
        {
            source.MultiplyByUniformScale(static_cast<float>(scale));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            MultiplyByUniformScale,
            k_categoryName,
            "{90472D62-65A8-40C1-AB08-FA66D793F689}", "ScriptCanvas_TransformFunctions_MultiplyByUniformScale");

        AZ_INLINE Vector3Type MultiplyByVector3(const TransformType& source, const Vector3Type multiplier)
        {
            return source.TransformPoint(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            MultiplyByVector3,
            k_categoryName,
            "{147E4714-5028-49A3-A038-6BFB3ED45E0B}", "ScriptCanvas_TransformFunctions_MultiplyByVector3");

        AZ_INLINE Vector4Type MultiplyByVector4(const TransformType& source, const Vector4Type multiplier)
        {
            return source.TransformPoint(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            MultiplyByVector4,
            k_categoryName,
            "{7E21DC19-C924-4479-817C-A942A52C8B20}", "ScriptCanvas_TransformFunctions_MultiplyByVector4");

        AZ_INLINE TransformType Orthogonalize(const TransformType& source)
        {
            return source.GetOrthogonalized();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Orthogonalize,
            k_categoryName,
            "{2B4140CD-6E22-44D3-BDB5-309E69FE7CC2}", "ScriptCanvas_TransformFunctions_Orthogonalize");

        AZ_INLINE TransformType RotationXDegrees(NumberType degrees)
        {
            return TransformType::CreateRotationX(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            RotationXDegrees,
            k_categoryName,
            "{1C43EF69-D4BD-46BD-BB91-3AC93ECB878C}", "ScriptCanvas_TransformFunctions_RotationXDegrees");

        AZ_INLINE TransformType RotationYDegrees(NumberType degrees)
        {
            return TransformType::CreateRotationY(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            RotationYDegrees,
            k_categoryName,
            "{0426C64C-CC1D-415A-8FA8-2267DE8CA317}", "ScriptCanvas_TransformFunctions_RotationYDegrees");

        AZ_INLINE TransformType RotationZDegrees(NumberType degrees)
        {
            return TransformType::CreateRotationZ(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            RotationZDegrees,
            k_categoryName,
            "{F848306A-C07C-4586-B52F-BEEE489045D2}", "ScriptCanvas_TransformFunctions_RotationZDegrees");

        AZ_INLINE NumberType ToScale(const TransformType& source)
        {
            return source.GetUniformScale();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            ToScale, k_categoryName, "{063C58AD-F567-464D-A432-F298FE3953A6}", "ScriptCanvas_TransformFunctions_ToScale");

        using Registrar = RegistrarGeneric
            <
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
            , IsCloseNode
            , IsFiniteNode
            , IsOrthogonalNode
            , MultiplyByUniformScaleNode
            , MultiplyByVector3Node
            , MultiplyByVector4Node
            , OrthogonalizeNode
            , RotationXDegreesNode
            , RotationYDegreesNode
            , RotationZDegreesNode
            , ToScaleNode
            > ;

    }
}

