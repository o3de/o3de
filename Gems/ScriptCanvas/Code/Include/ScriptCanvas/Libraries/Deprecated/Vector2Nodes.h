/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/NumericData.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvas
{
    //! Vector2Nodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace Vector2Nodes
    {
        using namespace MathNodeUtilities;
        using namespace Data;
        static constexpr const char* k_categoryName = "Math/Vector2";

        AZ_INLINE Vector2Type Absolute(const Vector2Type source)
        {
            return source.GetAbs();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Absolute,
            k_categoryName,
            "{68DE5669-9D35-4414-AE17-51BF00ED6738}", "ScriptCanvas_Vector2Functions_Absolute");

        AZ_INLINE Vector2Type Angle(NumberType angle)
        {
            return Vector2Type::CreateFromAngle(aznumeric_cast<float>(angle));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Angle,
            k_categoryName,
            "{4D77F825-C4CE-455C-802F-34F6C8B7A1C8}", "ScriptCanvas_Vector2Functions_Angle");

        AZ_INLINE Vector2Type Clamp(const Vector2Type source, const Vector2Type min, const Vector2Type max)
        {
            return source.GetClamp(min, max);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Clamp,
            k_categoryName,
            "{F2812289-F53C-4603-AE47-93902D9B06E0}", "ScriptCanvas_Vector2Functions_Clamp");

        AZ_INLINE NumberType Distance(const Vector2Type a, const Vector2Type b)
        {
            return a.GetDistance(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Distance,
            k_categoryName,
            "{6F37E3A7-8FBA-4DC3-83C0-659075E9F3E0}", "ScriptCanvas_Vector2Functions_Distance");

        AZ_INLINE NumberType DistanceSquared(const Vector2Type a, const Vector2Type b)
        {
            return a.GetDistanceSq(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            DistanceSquared,
            k_categoryName,
            "{23C6FD73-825E-4FFB-83B6-67FE1C9D1271}", "ScriptCanvas_Vector2Functions_DistanceSquared");

        AZ_INLINE NumberType Dot(const Vector2Type lhs, const Vector2Type rhs)
        {
            return lhs.Dot(rhs);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Dot, k_categoryName, "{F61FF592-E75D-4897-A081-AFE944DDFD58}", "ScriptCanvas_Vector2Functions_Dot");

        AZ_INLINE Vector2Type FromValues(NumberType x, NumberType y)
        {
            return Vector2Type(aznumeric_cast<float>(x), aznumeric_cast<float>(y));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromValues, k_categoryName, "{7CF4EC50-45A9-436D-AE08-54F27EA979BB}", "ScriptCanvas_Vector2Functions_FromValues");

        AZ_INLINE NumberType GetElement(const Vector2Type source, const NumberType index)
        {
            return source.GetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 1));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetElement,
            k_categoryName,
            "{C29C47AC-3847-48DB-9CC0-4C403C1B276C}", "ScriptCanvas_Vector2Functions_GetElement");

        AZ_INLINE BooleanType IsClose(const Vector2Type a, const Vector2Type b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsClose,
            k_categoryName,
            "{3A0B3386-2BF9-43FB-A003-DE026DBD7DFA}", "ScriptCanvas_Vector2Functions_IsClose");

        AZ_INLINE BooleanType IsFinite(const Vector2Type source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsFinite,
            k_categoryName,
            "{80578C30-DD70-448A-9DE5-662734E14335}", "ScriptCanvas_Vector2Functions_IsFinite");

        AZ_INLINE BooleanType IsNormalized(const Vector2Type source, NumberType tolerance)
        {
            return source.IsNormalized(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsNormalized,
            k_categoryName,
            "{C9EF4543-CF4D-43D5-96B1-E2DBFEA929C8}", "ScriptCanvas_Vector2Functions_IsNormalized");

        AZ_INLINE BooleanType IsZero(const Vector2Type source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsZero,
            k_categoryName,
            "{0A74D60B-F59E-47E8-8D68-BE69843D865B}", "ScriptCanvas_Vector2Functions_IsZero");

        AZ_INLINE NumberType Length(const Vector2Type source)
        {
            return source.GetLength();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Length, k_categoryName, "{39887B90-753A-46F8-A46A-F8B237FEAE2B}", "ScriptCanvas_Vector2Functions_Length");

        AZ_INLINE NumberType LengthSquared(const Vector2Type source)
        {
            return source.GetLengthSq();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            LengthSquared,
            k_categoryName,
            "{AC956D8F-E66A-4D8C-B82D-A920732847EC}", "ScriptCanvas_Vector2Functions_LengthSquared");

        AZ_INLINE Vector2Type Lerp(const Vector2Type& from, const Vector2Type& to, NumberType t)
        {
            return from.Lerp(to, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Lerp,
            k_categoryName,
            "{9BFB41C7-B665-4462-B237-1CD317DB1C7E}", "ScriptCanvas_Vector2Functions_Lerp");

        AZ_INLINE Vector2Type Max(const Vector2Type a, const Vector2Type b)
        {
            return a.GetMax(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Max, k_categoryName, "{DFAA23D9-8D28-4746-B224-01807258A473}", "ScriptCanvas_Vector2Functions_Max");

        AZ_INLINE Vector2Type Min(const Vector2Type a, const Vector2Type b)
        {
            return a.GetMin(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Min, k_categoryName, "{815685B8-B877-4D54-9E11-D0161185B4B9}", "ScriptCanvas_Vector2Functions_Min");

        AZ_INLINE Vector2Type SetX(Vector2Type source, NumberType value)
        {
            source.SetX(aznumeric_caster(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            SetX, k_categoryName, "{A5C2933F-C871-4915-B3AA-0C31FCFFEC15}", "ScriptCanvas_Vector2Functions_SetX");

        AZ_INLINE Vector2Type SetY(Vector2Type source, NumberType value)
        {
            source.SetY(aznumeric_caster(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            SetY, k_categoryName, "{824BE8DB-BB03-49A2-A829-34DAE2C66AF4}", "ScriptCanvas_Vector2Functions_SetY");

        AZ_INLINE Vector2Type MultiplyByNumber(const Vector2Type source, const NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            MultiplyByNumber,
            k_categoryName,
            "{4B7A44C2-383E-4F41-B7F9-FA87F946B46B}", "ScriptCanvas_Vector2Functions_MultiplyByNumber");

        AZ_INLINE Vector2Type Negate(const Vector2Type source)
        {
            return -source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Negate,
            k_categoryName,
            "{AD35E721-1591-433D-8B88-0CC431C58EE6}", "ScriptCanvas_Vector2Functions_Negate");

        AZ_INLINE Vector2Type Normalize(const Vector2Type source)
        {
            return source.GetNormalizedSafe();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Normalize,
            k_categoryName,
            "{2FB16EFF-5B3D-456E-B791-43F19C03BB83}", "ScriptCanvas_Vector2Functions_Normalize");

        AZ_INLINE Vector2Type Project(Vector2Type a, const Vector2Type b)
        {
            a.Project(b);
            return a;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Project,
            k_categoryName,
            "{67FA83DA-E026-4324-8034-067EC9505C7E}", "ScriptCanvas_Vector2Functions_Project");

        AZ_INLINE Vector2Type Slerp(const Vector2Type from, const Vector2Type to, const NumberType t)
        {
            return from.Slerp(to, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Slerp,
            k_categoryName,
            "{E8221B8F-AD1F-42B5-9389-7DEDE5C3B3C9}", "ScriptCanvas_Vector2Functions_Slerp");

        AZ_INLINE Vector2Type ToPerpendicular(const Vector2Type source)
        {
            return source.GetPerpendicular();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            ToPerpendicular,
            k_categoryName,
            "{CC4DC102-8B50-4828-BA94-0586F34E0D37}", "ScriptCanvas_Vector2Functions_ToPerpendicular");

        AZ_INLINE std::tuple<Vector2Type, NumberType> DirectionTo(const Vector2Type from, const Vector2Type to, NumberType optionalScale = 1.f)
        {
            Vector2Type r = to - from;
            float length = r.NormalizeWithLength();
            r.SetLength(static_cast<float>(optionalScale));
            return std::make_tuple(r, length);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            DirectionTo,
            k_categoryName,
            "{49A2D7F6-6CD3-420E-8A79-D46B00DB6CED}", "ScriptCanvas_Vector2Functions_DirectionTo");

        using Registrar = RegistrarGeneric <
            AbsoluteNode
            , AngleNode
            , ClampNode
            , DistanceNode
            , DistanceSquaredNode
            , DotNode
            , FromValuesNode
            , GetElementNode
            , IsCloseNode
            , IsFiniteNode
            , IsNormalizedNode
            , IsZeroNode
            , LengthNode
            , LengthSquaredNode
            , LerpNode
            , MaxNode
            , MinNode
            , SetXNode
            , SetYNode
            , MultiplyByNumberNode
            , NegateNode
            , NormalizeNode
            , ProjectNode
            , SlerpNode
            , ToPerpendicularNode
            , DirectionToNode
        > ;

    }
}

