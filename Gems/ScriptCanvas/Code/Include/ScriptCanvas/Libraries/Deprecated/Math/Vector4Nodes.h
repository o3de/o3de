/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector4.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/NumericData.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvas
{
    //! Vector4Nodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace Vector4Nodes
    {
        using namespace MathNodeUtilities;
        using namespace Data;
        static constexpr const char* k_categoryName = "Math/Vector4";

        AZ_INLINE Vector4Type Absolute(const Vector4Type source)
        {
            return source.GetAbs();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Absolute,
            k_categoryName,
            "{05D75C7F-CB46-44D1-B63C-65C8A3BF640F}", "ScriptCanvas_Vector4Functions_Absolute");

        AZ_INLINE NumberType Dot(const Vector4Type lhs, const Vector4Type rhs)
        {
            return lhs.Dot(rhs);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Dot, k_categoryName, "{A798225B-F2D4-41A2-8DC2-717A0B087687}", "ScriptCanvas_Vector4Functions_Dot");

        AZ_INLINE Vector4Type FromValues(NumberType x, NumberType y, NumberType z, NumberType w)
        {
            return Vector4Type(aznumeric_cast<float>(x), aznumeric_cast<float>(y), aznumeric_cast<float>(z), aznumeric_cast<float>(w));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromValues, k_categoryName, "{725D79B8-1CB1-4473-8480-4DE584C75540}", "ScriptCanvas_Vector4Functions_FromValues");

        AZ_INLINE NumberType GetElement(const Vector4Type source, const NumberType index)
        {
            return source.GetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 3));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetElement,
            k_categoryName,
            "{8B63488C-24D9-4674-8DD5-1AC253A24656}", "ScriptCanvas_Vector4Functions_GetElement");

        AZ_INLINE BooleanType IsClose(const Vector4Type a, const Vector4Type b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsClose,
            k_categoryName,
            "{51BFE8D5-1D1D-42C1-BB61-C8A3B1782EFF}", "ScriptCanvas_Vector4Functions_IsClose");

        AZ_INLINE BooleanType IsFinite(const Vector4Type source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsFinite,
            k_categoryName,
            "{36F7B32E-F3A5-48E1-8FEB-B7ADE75BBDC3}", "ScriptCanvas_Vector4Functions_IsFinite");

        AZ_INLINE BooleanType IsNormalized(const Vector4Type source, NumberType tolerance)
        {
            return source.IsNormalized(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsNormalized,
            k_categoryName,
            "{CCBD0216-42E0-4DAF-B206-424EDAD2247B}", "ScriptCanvas_Vector4Functions_IsNormalized");

        AZ_INLINE BooleanType IsZero(const Vector4Type source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsZero,
            k_categoryName,
            "{FDD689A3-AFA1-4FED-9B47-3909B92302C5}", "ScriptCanvas_Vector4Functions_IsZero");

        AZ_INLINE NumberType Length(const Vector4Type source)
        {
            return source.GetLength();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Length, k_categoryName, "{387B14BE-408B-4F95-ADB9-BC1C5BF75810}", "ScriptCanvas_Vector4Functions_Length");

        AZ_INLINE NumberType LengthReciprocal(const Vector4Type source)
        {
            return source.GetLengthReciprocal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            LengthReciprocal,
            k_categoryName,
            "{45457D77-2B42-4B5B-B320-BB25B01DC95A}", "ScriptCanvas_Vector4Functions_LengthReciprocal");

        AZ_INLINE NumberType LengthSquared(const Vector4Type source)
        {
            return source.GetLengthSq();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            LengthSquared,
            k_categoryName,
            "{C2D83735-69EC-4DA4-81C7-AA2B3C45FAB3}", "ScriptCanvas_Vector4Functions_LengthSquared");

        AZ_INLINE Vector4Type SetW(Vector4Type source, NumberType value)
        {
            source.SetW(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            SetW,
            k_categoryName,
            "{BE23146C-03DF-4A5C-BFF8-724A5E588F2E}", "ScriptCanvas_Vector4Functions_SetW");

        AZ_INLINE Vector4Type SetX(Vector4Type source, NumberType value)
        {
            source.SetX(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            SetX,
            k_categoryName,
            "{A1D746E9-8B71-4452-AF07-96993547373C}", "ScriptCanvas_Vector4Functions_SetX");

        AZ_INLINE Vector4Type SetY(Vector4Type source, NumberType value)
        {
            source.SetY(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            SetY,
            k_categoryName,
            "{D7C5E214-8E73-471C-8026-CB35D2D2FD58}", "ScriptCanvas_Vector4Functions_SetY");

        AZ_INLINE Vector4Type SetZ(Vector4Type source, NumberType value)
        {
            source.SetZ(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            SetZ,
            k_categoryName,
            "{5458F2CC-B8E2-43E2-B59F-8825A4D1F70A}", "ScriptCanvas_Vector4Functions_SetZ");

        AZ_INLINE Vector4Type MultiplyByNumber(const Vector4Type source, const NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            MultiplyByNumber,
            k_categoryName,
            "{24A667EA-6381-4F05-A856-A817BD8B9343}", "ScriptCanvas_Vector4Functions_MultiplyByNumber");

        AZ_INLINE Vector4Type Negate(const Vector4Type source)
        {
            return -source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Negate,
            k_categoryName,
            "{E2EF2002-7F5B-4BF3-B19A-B160BE5C5ED3}", "ScriptCanvas_Vector4Functions_Negate");

        AZ_INLINE Vector4Type Normalize(const Vector4Type source)
        {
            return source.GetNormalizedSafe();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Normalize,
            k_categoryName,
            "{64F79CB7-08C9-4622-BFC3-553682454E92}", "ScriptCanvas_Vector4Functions_Normalize");

        AZ_INLINE Vector4Type Reciprocal(const Vector4Type source)
        {
            return source.GetReciprocal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Reciprocal,
            k_categoryName,
            "{F35F6E73-37BE-4A86-A7A8-E8A1253B7B45}", "ScriptCanvas_Vector4Functions_Reciprocal");

        AZ_INLINE std::tuple<Vector4Type, NumberType> DirectionTo(const Vector4Type from, const Vector4Type to, NumberType optionalScale = 1.f)
        {
            Vector4Type r = to - from;
            float length = r.NormalizeWithLength();
            r.SetLength(static_cast<float>(optionalScale));
            return std::make_tuple(r, length);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            DirectionTo,
            k_categoryName,
            "{463762DE-E541-4AFE-80C2-FED1C5273319}", "ScriptCanvas_Vector4Functions_DirectionTo");

        using Registrar = RegistrarGeneric <
            AbsoluteNode,
            DotNode,
            FromValuesNode,
            GetElementNode,
            IsCloseNode,
            IsFiniteNode,
            IsNormalizedNode,
            IsZeroNode,
            LengthNode,
            LengthReciprocalNode,
            LengthSquaredNode,
            SetWNode,
            SetXNode,
            SetYNode,
            SetZNode,
            MultiplyByNumberNode,
            NegateNode,
            NormalizeNode,
            ReciprocalNode,
            DirectionToNode
        > ;

    }
}

