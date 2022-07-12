/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/NumericData.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

// \todo hide the reflection of these from behavior context

namespace ScriptCanvas
{
    //! Vector3Nodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace Vector3Nodes
    {
        using namespace MathNodeUtilities;
        using namespace Data;
        static constexpr const char* k_categoryName = "Math/Vector3";

        AZ_INLINE Vector3Type Absolute(const Vector3Type source)
        {
            return source.GetAbs();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Absolute,
            k_categoryName,
            "{92A4801A-15FB-4529-80BA-B880D8C24989}", "ScriptCanvas_Vector3Functions_Absolute");

        AZ_INLINE std::tuple<Vector3Type, Vector3Type> BuildTangentBasis(Vector3Type source)
        {
            std::tuple<Vector3Type, Vector3Type> tangentBitangent;
            source.NormalizeSafe();
            source.BuildTangentBasis(std::get<0>(tangentBitangent), std::get<1>(tangentBitangent));
            return tangentBitangent;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            BuildTangentBasis,
            k_categoryName,
            "{3EBA365F-063A-45A0-BDD1-ED0F995AD310}", "ScriptCanvas_Vector3Functions_BuildTangentBasis");

        AZ_INLINE Vector3Type Clamp(const Vector3Type source, const Vector3Type min, const Vector3Type max)
        {
            return source.GetClamp(min, max);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Clamp,
            k_categoryName,
            "{28305F88-0940-43C8-B0A8-B8CEB3B0B82A}", "ScriptCanvas_Vector3Functions_Clamp");

        AZ_INLINE Vector3Type Cross(const Vector3Type lhs, const Vector3Type rhs)
        {
            return lhs.Cross(rhs);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Cross, k_categoryName, "{6FAF4ACA-A100-4B71-ACF8-F1DB4674F51C}", "ScriptCanvas_Vector3Functions_Cross");

        AZ_INLINE NumberType Distance(const Vector3Type a, const Vector3Type b)
        {
            return a.GetDistance(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Distance,
            k_categoryName,
            "{BFE43C43-3FDB-4E93-86D7-EB3766B75E7B}", "ScriptCanvas_Vector3Functions_Distance");

        AZ_INLINE NumberType DistanceSquared(const Vector3Type a, const Vector3Type b)
        {
            return a.GetDistanceSq(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            DistanceSquared,
            k_categoryName,
            "{D07DD389-31F7-435A-9329-903348B04DAB}", "ScriptCanvas_Vector3Functions_DistanceSquared");

        AZ_INLINE NumberType Dot(const Vector3Type lhs, const Vector3Type rhs)
        {
            return lhs.Dot(rhs);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Dot, k_categoryName, "{5DFA6260-C044-4798-A55C-3CF5F3DB45CE}", "ScriptCanvas_Vector3Functions_Dot");

        AZ_INLINE Vector3Type FromValues(NumberType x, NumberType y, NumberType z)
        {
            return Vector3Type(aznumeric_cast<float>(x), aznumeric_cast<float>(y), aznumeric_cast<float>(z));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromValues, k_categoryName, "{AA4B21AC-26B1-41E2-9AE4-19F4FFF050CC}", "ScriptCanvas_Vector3Functions_FromValues");

        AZ_INLINE NumberType GetElement(const Vector3Type source, const NumberType index)
        {
            return source.GetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 2));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetElement,
            k_categoryName,
            "{117EB15C-BDBA-41D2-8904-C7CE34E34BB9}", "ScriptCanvas_Vector3Functions_GetElement");

        AZ_INLINE BooleanType IsClose(const Vector3Type a, const Vector3Type b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsClose,
            k_categoryName,
            "{4E75F538-DC03-4AEB-B38D-102F7337F36D}", "ScriptCanvas_Vector3Functions_IsClose");

        AZ_INLINE BooleanType IsFinite(const Vector3Type source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsFinite,
            k_categoryName,
            "{6C1CB6E9-6EE3-4F6F-8B24-6DB4906B7DC7}", "ScriptCanvas_Vector3Functions_IsFinite");

        AZ_INLINE BooleanType IsNormalized(const Vector3Type source, NumberType tolerance)
        {
            return source.IsNormalized(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsNormalized,
            k_categoryName,
            "{EFFC389A-CCE7-4350-8E3F-C2B728CD03C6}", "ScriptCanvas_Vector3Functions_IsNormalized");

        AZ_INLINE BooleanType IsPerpendicular(const Vector3Type a, const Vector3Type b, NumberType tolerance)
        {
            return a.IsPerpendicular(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsPerpendicular,
            k_categoryName,
            "{D283EB50-8493-444E-9333-90E1F70565FF}", "ScriptCanvas_Vector3Functions_IsPerpendicular");

        AZ_INLINE BooleanType IsZero(const Vector3Type source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsZero,
            k_categoryName,
            "{AFCE279C-9BB2-446B-9C18-8A9D9FBCCD6C}", "ScriptCanvas_Vector3Functions_IsZero");

        AZ_INLINE NumberType Length(const Vector3Type source)
        {
            return source.GetLength();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Length, k_categoryName, "{4CD73E38-C98A-4B5A-9BAA-6E8B69AB7201}", "ScriptCanvas_Vector3Functions_Length");

        AZ_INLINE NumberType LengthReciprocal(const Vector3Type source)
        {
            return source.GetLengthReciprocal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            LengthReciprocal,
            k_categoryName,
            "{4B06E22C-E2B5-4624-88F1-1406CEC423A2}", "ScriptCanvas_Vector3Functions_LengthReciprocal");

        AZ_INLINE NumberType LengthSquared(const Vector3Type source)
        {
            return source.GetLengthSq();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            LengthSquared,
            k_categoryName,
            "{650E8F83-0FDD-4C97-A6CD-83D8688D2645}", "ScriptCanvas_Vector3Functions_LengthSquared");

        AZ_INLINE Vector3Type Lerp(const Vector3Type& from, const Vector3Type& to, NumberType t)
        {
            return from.Lerp(to, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Lerp,
            k_categoryName,
            "{AA063267-DA0F-4407-9356-30B4E89A9FA4}", "ScriptCanvas_Vector3Functions_Lerp");

        AZ_INLINE Vector3Type Max(const Vector3Type a, const Vector3Type b)
        {
            return a.GetMax(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Max,
            k_categoryName,
            "{1FA35DE2-9D82-4628-99D0-25968734E551}", "ScriptCanvas_Vector3Functions_Max");

        AZ_INLINE Vector3Type Min(const Vector3Type a, const Vector3Type b)
        {
            return a.GetMin(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Min,
            k_categoryName,
            "{16B21396-9677-437E-B894-089AE2EC0E13}", "ScriptCanvas_Vector3Functions_Min");

        AZ_INLINE Vector3Type SetX(Vector3Type source, NumberType value)
        {
            source.SetX(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            SetX, k_categoryName, "{835EC2C1-EF57-481E-9154-B65E86B1A388}", "ScriptCanvas_Vector3Functions_SetX");

        AZ_INLINE Vector3Type SetY(Vector3Type source, NumberType value)
        {
            source.SetY(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            SetY, k_categoryName, "{72A0941A-DA2C-47AB-BCC2-A26D14E980E2}", "ScriptCanvas_Vector3Functions_SetY");

        AZ_INLINE Vector3Type SetZ(Vector3Type source, NumberType value)
        {
            source.SetZ(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            SetZ, k_categoryName, "{813CFA11-5D90-4864-BFAC-A28F97B6D80F}", "ScriptCanvas_Vector3Functions_SetZ");

        AZ_INLINE Vector3Type MultiplyByNumber(const Vector3Type source, const NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            MultiplyByNumber,
            k_categoryName,
            "{47097B44-8B91-4589-AED2-83752300E0D7}", "ScriptCanvas_Vector3Functions_MultiplyByNumber");

        AZ_INLINE Vector3Type Negate(const Vector3Type source)
        {
            return -source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Negate,
            k_categoryName,
            "{017C6F84-DECC-489D-85E1-A999B9AD986B}", "ScriptCanvas_Vector3Functions_Negate");

        AZ_INLINE Vector3Type Normalize(const Vector3Type source)
        {
            return source.GetNormalizedSafe();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Normalize,
            k_categoryName,
            "{971E7456-4BDF-4FB3-A418-D6ECAC186FD5}", "ScriptCanvas_Vector3Functions_Normalize");

        AZ_INLINE Vector3Type Project(Vector3Type a, const Vector3Type b)
        {
            a.Project(b);
            return a;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Project,
            k_categoryName,
            "{DD53C3CF-5543-449B-8076-387CD3D66291}", "ScriptCanvas_Vector3Functions_Project");

        AZ_INLINE Vector3Type Reciprocal(const Vector3Type source)
        {
            return source.GetReciprocal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Reciprocal,
            k_categoryName,
            "{09B243E6-AAAF-4B30-BF22-FDB074700D05}", "ScriptCanvas_Vector3Functions_Reciprocal");

        AZ_INLINE Vector3Type Slerp(const Vector3Type from, const Vector3Type to, const NumberType t)
        {
            return from.Slerp(to, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Slerp,
            k_categoryName,
            "{F3EA1D86-33DD-46BA-8A88-9FE6AB181E01}", "ScriptCanvas_Vector3Functions_Slerp");

        AZ_INLINE std::tuple<Vector3Type, NumberType> DirectionTo(const Vector3Type from, const Vector3Type to, NumberType optionalScale = 1.f)
        {
            Vector3Type r = to - from;
            float length = r.NormalizeWithLength();
            r.SetLength(static_cast<float>(optionalScale));
            return std::make_tuple(r, length);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            DirectionTo,
            k_categoryName,
            "{28FBD529-4C9A-4E34-B8A0-A13B5DB3C331}", "ScriptCanvas_Vector3Functions_DirectionTo");

        using Registrar = RegistrarGeneric <
            AbsoluteNode
            , BuildTangentBasisNode
            , ClampNode
            , CrossNode
            , DistanceNode
            , DistanceSquaredNode
            , DotNode
            , FromValuesNode
            , GetElementNode
            , IsCloseNode
            , IsFiniteNode
            , IsNormalizedNode
            , IsPerpendicularNode
            , IsZeroNode
            , LengthNode
            , LengthReciprocalNode
            , LengthSquaredNode
            , LerpNode
            , MaxNode
            , MinNode
            , SetXNode
            , SetYNode
            , SetZNode
            , MultiplyByNumberNode
            , NegateNode
            , NormalizeNode
            , ProjectNode
            , ReciprocalNode
            , SlerpNode
            , DirectionToNode
        > ;

    }
}

