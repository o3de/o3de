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
    namespace ColorNodes
    {
        using namespace Data;
        using namespace MathNodeUtilities;
        static constexpr const char* k_categoryName = "Math/Color";

        AZ_INLINE ColorType Add(ColorType a, ColorType b)
        {
            return a + b;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Add, k_categoryName, "{0A3B5BA4-81E6-4550-8163-737AA00DC029}", "This node is deprecated, use Add (+), it provides contextual type and slots", "A", "B");

        AZ_INLINE ColorType DivideByColor(ColorType a, ColorType b)
        {
            return a / b;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(DivideByColor, k_categoryName, "{9BB0BF87-A025-4CBA-B57D-9E3187D872CD}", "This node is deprecated, use Divide (/), it provides contextual type and slots", "A", "B");

        AZ_INLINE ColorType DivideByNumber(ColorType source, NumberType divisor)
        {
            if (AZ::IsClose(divisor, Data::NumberType(0), std::numeric_limits<Data::NumberType>::epsilon()))
            {
                AZ_Error("Script Canvas", false, "Division by zero");
                return ColorType::CreateZero();
            }

            return source / aznumeric_cast<float>(divisor);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(DivideByNumber, k_categoryName, "{1B8EBAAF-FEFE-4D1E-896D-4CAFD2D6426B}", "returns Source with each element divided by Divisor", "Source", "Divisor");

        AZ_INLINE NumberType Dot(ColorType a, ColorType b)
        {
            return a.Dot(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Dot, k_categoryName, "{5E0DB317-5885-4848-9CC1-F21651F31538}", "returns the 4-element dot product of A and B", "A", "B");

        AZ_INLINE NumberType Dot3(ColorType a, ColorType b)
        {
            return a.Dot3(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Dot3, k_categoryName, "{D9E99878-1C49-4836-82CF-AA194A8F41E0}", "returns the 3-element dot product of A and B, using only the R, G, B elements", "A", "B");

        AZ_INLINE ColorType FromValues(NumberType r, NumberType g, NumberType b, NumberType a)
        {
            return ColorType(aznumeric_cast<float>(r), aznumeric_cast<float>(g), aznumeric_cast<float>(b), aznumeric_cast<float>(a));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromValues, k_categoryName, "{37473690-CA2B-4F76-B101-40527EEEEC57}", "returns a Color from the R, G, B, A inputs", "R", "G", "B", "A");

        AZ_INLINE ColorType FromVector3(Vector3Type source)
        {
            return ColorType::CreateFromVector3(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromVector3, k_categoryName, "{C447E050-CD41-47D6-BD21-728DF2F1DB29}", "returns a Color with R, G, B set to X, Y, Z values of RGB, respectively. A is set to 1.0", "RGB");

        AZ_INLINE ColorType FromVector3AndNumber(Vector3Type RGB, NumberType A)
        {
            return ColorType::CreateFromVector3AndFloat(RGB, aznumeric_cast<float>(A));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromVector3AndNumber, k_categoryName, "{75D0C056-2FA4-40CC-B3B5-B3D8DC2C0738}", "returns a Color with R, G, B set to X, Y, Z values of RGB, respectively. A is set to A", "RGB", "A");

        AZ_INLINE ColorType FromVector4(Vector4Type RGBA)
        {
            return ColorType(RGBA);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromVector4, k_categoryName, "{6BB59B09-0A3C-4BF6-81C7-376511905441}", "returns a Color with R, G, B, A, set to X, Y, Z, W values of RGBA, respectively.", "RGBA");
        
        AZ_INLINE ColorType GammaToLinear(ColorType source)
        {
            return source.GammaToLinear();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GammaToLinear, k_categoryName, "{9C74D6FA-25ED-45AA-B577-94FC92A6D954}", "returns Source converted from gamma corrected to linear space", "Source");

        AZ_INLINE BooleanType IsClose(ColorType a, ColorType b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsClose, DefaultToleranceSIMD<2>, k_categoryName, "{81122289-14A8-4EF2-AF99-3A07D5FF746B}", "returns true if A is within Tolerance of B, else false", "A", "B", "Tolerance");

        AZ_INLINE BooleanType IsZero(ColorType source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsZero, DefaultToleranceSIMD<1>, k_categoryName, "{91C157DA-C2AC-405B-B341-E67DA0FD72B9}", "returns true if Source is within Tolerance of zero", "Source", "Tolerance");

        AZ_INLINE ColorType LinearToGamma(ColorType source)
        {
            return source.LinearToGamma();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(LinearToGamma, k_categoryName, "{0AB4D9F0-E905-41F1-9613-37877BB72EDA}", "returns Source converted from linear to gamma corrected space", "Source");

        AZ_INLINE ColorType ModR(ColorType source, NumberType value)
        {
            source.SetR(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModR, k_categoryName, "{F4A20135-372D-4F25-94BC-A36C1A47A840}", "returns a the color(R, Source.G, Source.B, Source.A)", "Source", "R");

        AZ_INLINE ColorType ModG(ColorType source, NumberType value)
        {
            source.SetG(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModG, k_categoryName, "{332BE325-5C52-4E36-842D-E34E691F9321}", "returns a the color(Source.R, G, Source.B, Source.A)", "Source", "G");

        AZ_INLINE ColorType ModB(ColorType source, NumberType value)
        {
            source.SetB(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModB, k_categoryName, "{046D3D03-3FB2-4696-B509-9E7DD88B8978}", "returns a the color(Source.R, Source.G, B, Source.A)", "Source", "B");

        AZ_INLINE ColorType ModA(ColorType source, NumberType value)
        {
            source.SetA(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModA, k_categoryName, "{7E885F96-7709-4B66-AC21-AF58D6AB9132}", "returns a the color(Source.R, Source.G, Source.B, A)", "Source", "A");

        AZ_INLINE ColorType MultiplyByColor(ColorType a, ColorType b)
        {
            return a * b;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(MultiplyByColor, k_categoryName, "{1D0268CE-1347-4D3A-8B04-2687937E4686}", "This node is deprecated, use Multiply (*), it provides contextual type and slots", "A", "B");

        AZ_INLINE ColorType MultiplyByNumber(ColorType source, NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyByNumber, k_categoryName, "{CF3CC496-6370-4A26-8D91-9B3B6ED63D07}", "returns Source with every elemented multiplied by Multiplier", "Source", "Multiplier");

        AZ_INLINE ColorType Negate(ColorType source)
        {
            return -source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Negate, k_categoryName, "{B29F22BE-2378-4DE0-A28A-CF6ABBC894DF}", "returns Source with every element multiplied by -1", "Source");

        AZ_INLINE ColorType One()
        {
            return ColorType::CreateOne();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(One, k_categoryName, "{A0951A32-BA75-4DA9-B788-79EDB7DA8CF4}", "returns a Color with every element set to 1");

        AZ_INLINE ColorType Subtract(ColorType a, ColorType b)
        {
            return a - b;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Subtract, k_categoryName, "{EA72B942-8C4B-4CD6-A9C1-8022F981199C}", "This node is deprecated, use Subtract (-), it provides contextual type and slots", "A", "B");

        using Registrar = RegistrarGeneric<
            AddNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , DivideByColorNode
#endif

            , DivideByNumberNode
            , DotNode
            , Dot3Node
            , FromValuesNode
            , FromVector3Node
            , FromVector3AndNumberNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , FromVector4Node
#endif

            , GammaToLinearNode
            , IsCloseNode
            , IsZeroNode
            , LinearToGammaNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , ModRNode
            , ModGNode
            , ModBNode
            , ModANode
#endif

            , MultiplyByColorNode
            , MultiplyByNumberNode
            , NegateNode
            , OneNode
            , SubtractNode
        >;

    } 
} 

