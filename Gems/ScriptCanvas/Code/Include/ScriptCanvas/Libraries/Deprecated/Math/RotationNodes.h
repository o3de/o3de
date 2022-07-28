/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/NumericData.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvas
{
    //! QuaternionNodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace QuaternionNodes
    {
        using namespace Data;
        using namespace MathNodeUtilities;
        static constexpr const char* k_categoryName = "Math/Quaternion";

        AZ_INLINE QuaternionType Conjugate(QuaternionType source)
        {
            return source.GetConjugate();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Conjugate,
            k_categoryName, "{A1279F70-E211-41F2-8974-84E998206B0D}", "ScriptCanvas_QuaternionFunctions_Conjugate");

        AZ_INLINE QuaternionType ConvertTransformToRotation(const TransformType& transform)
        {
            return AZ::ConvertEulerRadiansToQuaternion(AZ::ConvertTransformToEulerRadians(transform));
        }

        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            ConvertTransformToRotation,
            k_categoryName,
            "{C878982F-1B6B-4555-8723-7FF3830C8032}",
            "ScriptCanvas_QuaternionFunctions_ConvertTransformToRotation");

        AZ_INLINE NumberType Dot(QuaternionType a, QuaternionType b)
        {
            return a.Dot(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Dot, k_categoryName, "{01FED020-6EB1-4A69-AFC7-7305FCA7FC97}", "ScriptCanvas_QuaternionFunctions_Dot");

        AZ_INLINE QuaternionType FromAxisAngleDegrees(Vector3Type axis, NumberType degrees)
        {
            return QuaternionType::CreateFromAxisAngle(axis, AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromAxisAngleDegrees,
            k_categoryName,
            "{109952D1-2DB7-48C3-970D-B8DB4C96FE54}",
            "ScriptCanvas_QuaternionFunctions_FromAxisAngleDegrees");

        AZ_INLINE QuaternionType FromMatrix3x3(const Matrix3x3Type& source)
        {
            return QuaternionType::CreateFromMatrix3x3(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromMatrix3x3, k_categoryName, "{AFB1A899-D71D-48C8-8C76-086146B7B6EE}", "ScriptCanvas_QuaternionFunctions_FromMatrix3x3");

        AZ_INLINE QuaternionType FromMatrix4x4(const Matrix4x4Type& source)
        {
            return QuaternionType::CreateFromMatrix4x4(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromMatrix4x4, k_categoryName, "{CD6F0D36-EC89-4D3E-920E-267D47F819BE}", "ScriptCanvas_QuaternionFunctions_FromMatrix4x4");

        AZ_INLINE QuaternionType FromTransform(const TransformType& source)
        {
            return source.GetRotation();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromTransform, k_categoryName, "{B6B224CC-7454-4D99-B473-C0A77D4FB885}", "ScriptCanvas_QuaternionFunctions_FromTransform");

        AZ_INLINE QuaternionType InvertFull(QuaternionType source)
        {
            return source.GetInverseFull();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            InvertFull, k_categoryName, "{DF936099-48C8-4924-A91D-6B93245D8F30}", "ScriptCanvas_QuaternionFunctions_InvertFull");

        AZ_INLINE BooleanType IsClose(QuaternionType a, QuaternionType b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsClose,
            k_categoryName,
            "{E0150AD6-6CBE-494E-9A1D-1E7E7C0A114F}",
            "ScriptCanvas_QuaternionFunctions_IsClose");

        AZ_INLINE BooleanType IsFinite(QuaternionType a)
        {
            return a.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsFinite, k_categoryName, "{503B1229-74E8-40FE-94DE-C4387806BDB0}", "ScriptCanvas_QuaternionFunctions_IsFinite");

        AZ_INLINE BooleanType IsIdentity(QuaternionType source, NumberType tolerance)
        {
            return source.IsIdentity(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsIdentity,
            k_categoryName,
            "{E7BB6123-E21A-4B51-B35E-BAA3DF239AB8}",
            "ScriptCanvas_QuaternionFunctions_IsIdentity");

        AZ_INLINE BooleanType IsZero(QuaternionType source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsZero,
            k_categoryName,
            "{8E71A7DC-5FCA-4569-A2C4-3A85B5070AA1}",
            "ScriptCanvas_QuaternionFunctions_IsZero");

        AZ_INLINE NumberType LengthReciprocal(QuaternionType source)
        {
            return source.GetLengthReciprocal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            LengthReciprocal, k_categoryName, "{C4019E78-59F8-4023-97F9-1FC6C2DC94C8}", "ScriptCanvas_QuaternionFunctions_LengthReciprocal");

        AZ_INLINE NumberType LengthSquared(QuaternionType source)
        {
            return source.GetLengthSq();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            LengthSquared, k_categoryName, "{825A0F09-CDFA-4C80-8177-003B154F213A}", "ScriptCanvas_QuaternionFunctions_LengthSquared");

        AZ_INLINE QuaternionType Lerp(QuaternionType a, QuaternionType b, NumberType t)
        {
            return a.Lerp(b, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Lerp, k_categoryName, "{91CF1C54-89C6-4A00-A53D-20C58454C4EC}", "ScriptCanvas_QuaternionFunctions_Lerp");

        AZ_INLINE QuaternionType MultiplyByNumber(QuaternionType source, NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            MultiplyByNumber, k_categoryName, "{B8911827-A1E7-4ECE-8503-9B31DD9C63C8}", "ScriptCanvas_QuaternionFunctions_MultiplyByNumber");

        AZ_INLINE QuaternionType Negate(QuaternionType source)
        {
            return -source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Negate, k_categoryName, "{5EA770E6-6F6C-4838-B2D8-B2C487BF32E7}", "ScriptCanvas_QuaternionFunctions_Negate");

        AZ_INLINE QuaternionType Normalize(QuaternionType source)
        {
            return source.GetNormalized();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Normalize, k_categoryName, "{1B01B185-50E0-4120-BD82-9331FC3117F9}", "ScriptCanvas_QuaternionFunctions_Normalize");

        AZ_INLINE QuaternionType RotationXDegrees(NumberType degrees)
        {
            return QuaternionType::CreateRotationX(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            RotationXDegrees, k_categoryName, "{9A017348-F803-43D7-A2A6-BE01359D5E15}", "ScriptCanvas_QuaternionFunctions_RotationXDegrees");

        AZ_INLINE QuaternionType RotationYDegrees(NumberType degrees)
        {
            return QuaternionType::CreateRotationY(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            RotationYDegrees, k_categoryName, "{6C69AA65-1A83-4C36-B010-ECB621790A6C}", "ScriptCanvas_QuaternionFunctions_RotationYDegrees");

        AZ_INLINE QuaternionType RotationZDegrees(NumberType degrees)
        {
            return QuaternionType::CreateRotationZ(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            RotationZDegrees, k_categoryName, "{8BC8B0FE-51A1-4ECC-AFF1-A828A0FC8F8F}", "ScriptCanvas_QuaternionFunctions_RotationZDegrees");

        AZ_INLINE QuaternionType ShortestArc(Vector3Type from, Vector3Type to)
        {
            return QuaternionType::CreateShortestArc(from, to);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            ShortestArc, k_categoryName, "{00CB739A-6BF9-4160-83F7-A243BD9D5093}", "ScriptCanvas_QuaternionFunctions_ShortestArc");

        AZ_INLINE QuaternionType Slerp(QuaternionType a, QuaternionType b, NumberType t)
        {
            return a.Slerp(b, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Slerp, k_categoryName, "{26234D44-9BA5-4E1B-8226-224E8A4A15CC}", "ScriptCanvas_QuaternionFunctions_Slerp");

        AZ_INLINE QuaternionType Squad(QuaternionType from, QuaternionType to, QuaternionType in, QuaternionType out, NumberType t)
        {
            return from.Squad(to, in, out, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Squad, k_categoryName, "{D354F41E-29E3-49EC-8F0E-C890000D32D6}", "ScriptCanvas_QuaternionFunctions_Squad");

        AZ_INLINE NumberType ToAngleDegrees(QuaternionType source)
        {
            return aznumeric_caster(AZ::RadToDeg(source.GetAngle()));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            ToAngleDegrees, k_categoryName, "{3EA78793-9AFA-4857-8CB8-CD0D47E97D25}", "ScriptCanvas_QuaternionFunctions_ToAngleDegrees");

        AZ_INLINE QuaternionType CreateFromEulerAngles(NumberType pitch, NumberType roll, NumberType yaw)
        {
            AZ::Vector3 eulerDegress = AZ::Vector3(static_cast<float>(pitch), static_cast<float>(roll), static_cast<float>(yaw));
            return AZ::ConvertEulerDegreesToQuaternion(eulerDegress);
        }

        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            CreateFromEulerAngles,
            k_categoryName,
            "{33974124-2882-499D-9FBE-A37EB687B30C}",
            "ScriptCanvas_QuaternionFunctions_CreateFromEulerAngles");

        AZ_INLINE Vector3Type RotateVector3(QuaternionType source, Vector3Type vector3)
        {
            return source.TransformVector(vector3);
        }

        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            RotateVector3, k_categoryName, "{DDF7C05C-7148-4860-93A3-D507C5896B6C}", "ScriptCanvas_QuaternionFunctions_RotateVector3");

        using Registrar = RegistrarGeneric <
            ConjugateNode
            , ConvertTransformToRotationNode
            , DotNode
            , FromAxisAngleDegreesNode
            , FromMatrix3x3Node
            , FromMatrix4x4Node
            , FromTransformNode
            , InvertFullNode
            , IsCloseNode
            , IsFiniteNode
            , IsIdentityNode
            , IsZeroNode
            , LengthReciprocalNode
            , LengthSquaredNode
            , LerpNode
            , MultiplyByNumberNode
            , NegateNode
            , NormalizeNode
            , RotationXDegreesNode
            , RotationYDegreesNode
            , RotationZDegreesNode
            , ShortestArcNode
            , SlerpNode
            , SquadNode
            , ToAngleDegreesNode
            , CreateFromEulerAnglesNode
            , RotateVector3Node
        > ;

    }
}

