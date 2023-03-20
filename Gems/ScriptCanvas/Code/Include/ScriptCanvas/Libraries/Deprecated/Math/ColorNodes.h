/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/NumericData.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvas
{
    //! ColorNodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace ColorNodes
    {
        using namespace Data;
        using namespace MathNodeUtilities;
        static constexpr const char* k_categoryName = "Math/Color";

        AZ_INLINE NumberType Dot(ColorType a, ColorType b)
        {
            return a.Dot(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(Dot, k_categoryName, "{5E0DB317-5885-4848-9CC1-F21651F31538}", "ScriptCanvas_ColorFunctions_Dot");

        AZ_INLINE NumberType Dot3(ColorType a, ColorType b)
        {
            return a.Dot3(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(Dot3, k_categoryName, "{D9E99878-1C49-4836-82CF-AA194A8F41E0}", "ScriptCanvas_ColorFunctions_Dot3");

        AZ_INLINE ColorType FromValues(NumberType r, NumberType g, NumberType b, NumberType a)
        {
            return ColorType(aznumeric_cast<float>(r), aznumeric_cast<float>(g), aznumeric_cast<float>(b), aznumeric_cast<float>(a));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(FromValues, k_categoryName, "{37473690-CA2B-4F76-B101-40527EEEEC57}", "ScriptCanvas_ColorFunctions_FromValues");

        AZ_INLINE ColorType FromVector3(Vector3Type source)
        {
            return ColorType::CreateFromVector3(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(FromVector3, k_categoryName, "{C447E050-CD41-47D6-BD21-728DF2F1DB29}", "ScriptCanvas_ColorFunctions_FromVector3");

        AZ_INLINE ColorType FromVector3AndNumber(Vector3Type RGB, NumberType A)
        {
            return ColorType::CreateFromVector3AndFloat(RGB, aznumeric_cast<float>(A));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(FromVector3AndNumber, k_categoryName, "{75D0C056-2FA4-40CC-B3B5-B3D8DC2C0738}", "ScriptCanvas_ColorFunctions_FromVector3AndNumber");

        AZ_INLINE ColorType GammaToLinear(ColorType source)
        {
            return source.GammaToLinear();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(GammaToLinear, k_categoryName, "{9C74D6FA-25ED-45AA-B577-94FC92A6D954}", "ScriptCanvas_ColorFunctions_GammaToLinear");

        AZ_INLINE BooleanType IsClose(ColorType a, ColorType b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(IsClose, k_categoryName, "{81122289-14A8-4EF2-AF99-3A07D5FF746B}", "ScriptCanvas_ColorFunctions_IsClose");

        AZ_INLINE BooleanType IsZero(ColorType source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(IsZero, k_categoryName, "{91C157DA-C2AC-405B-B341-E67DA0FD72B9}", "ScriptCanvas_ColorFunctions_IsZero");

        AZ_INLINE ColorType LinearToGamma(ColorType source)
        {
            return source.LinearToGamma();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(LinearToGamma, k_categoryName, "{0AB4D9F0-E905-41F1-9613-37877BB72EDA}", "ScriptCanvas_ColorFunctions_LinearToGamma");

        AZ_INLINE ColorType MultiplyByNumber(ColorType source, NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(MultiplyByNumber, k_categoryName, "{CF3CC496-6370-4A26-8D91-9B3B6ED63D07}", "ScriptCanvas_ColorFunctions_MultiplyByNumber");

        AZ_INLINE ColorType One()
        {
            return ColorType::CreateOne();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(One, k_categoryName, "{A0951A32-BA75-4DA9-B788-79EDB7DA8CF4}", "ScriptCanvas_ColorFunctions_One");

        using Registrar = RegistrarGeneric<
            DotNode
            , Dot3Node
            , FromValuesNode
            , FromVector3Node
            , FromVector3AndNumberNode
            , GammaToLinearNode
            , IsCloseNode
            , IsZeroNode
            , LinearToGammaNode
            , MultiplyByNumberNode
            , OneNode
        >;

    } 
} 

