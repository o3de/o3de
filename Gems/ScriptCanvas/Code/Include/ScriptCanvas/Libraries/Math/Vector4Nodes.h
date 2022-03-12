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
    namespace Vector4Nodes
    {
        using namespace MathNodeUtilities;
        using namespace Data;
        static constexpr const char* k_categoryName = "Math/Vector4";

        AZ_INLINE Vector4Type Absolute(const Vector4Type source)
        {
            return source.GetAbs();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Absolute, k_categoryName, "{05D75C7F-CB46-44D1-B63C-65C8A3BF640F}", "returns a vector with the absolute values of the elements of the source", "Source");

        AZ_INLINE Vector4Type Add(const Vector4Type lhs, const Vector4Type rhs)
        {
            return lhs + rhs;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Add, k_categoryName, "{900CABAF-BAA9-4C4D-A6AB-9ABD49270F3C}", "This node is deprecated, use Add (+), it provides contextual type and slots", "A", "B");

        AZ_INLINE Vector4Type DivideByNumber(const Vector4Type source, const NumberType divisor)
        {
            if (AZ::IsClose(divisor, Data::NumberType(0), std::numeric_limits<Data::NumberType>::epsilon()))
            {
                AZ_Error("Script Canvas", false, "Division by zero");
                return Vector4Type::CreateZero();
            }

            return source / aznumeric_cast<float>(divisor);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(DivideByNumber, k_categoryName, "{2AA9BBC5-B2A6-405B-BED3-59317DFDEE70}", "returns the source with each element divided by Divisor", "Source", "Divisor");

        AZ_INLINE Vector4Type DivideByVector(const Vector4Type source, const Vector4Type divisor)
        {
            return source / divisor;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(DivideByVector, k_categoryName, "{63033E8A-2FD1-4796-AE8B-0A8DE2A53C4D}", "This node is deprecated, use Divide (/), it provides contextual type and slots", "Numerator", "Divisor");

        AZ_INLINE NumberType Dot(const Vector4Type lhs, const Vector4Type rhs)
        {
            return lhs.Dot(rhs);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Dot, k_categoryName, "{A798225B-F2D4-41A2-8DC2-717A0B087687}", "returns the vector dot product of A dot B", "A", "B");

        AZ_INLINE NumberType Dot3(const Vector4Type lhs, const Vector3Type rhs)
        {
            return lhs.Dot3(rhs);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Dot3, k_categoryName, "{BB36F6BD-3E2F-430C-AE22-8EA9E8C6B309}", "returns the 3D vector dot product of A dot B, using the x,y,z elements", "A", "B");

        AZ_INLINE Vector4Type FromElement(Vector4Type source, const NumberType index, const NumberType value)
        {
            source.SetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 3), aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromElement, k_categoryName, "{117B2442-0B00-48FB-B0BA-591E1963903B}", "returns a vector with the element corresponding to the index (0 -> x) (1 -> y) (2 -> z)(3 -> w) set to the value", "Source", "Index", "Value");

        AZ_INLINE Vector4Type FromValues(NumberType x, NumberType y, NumberType z, NumberType w)
        {
            return Vector4Type(aznumeric_cast<float>(x), aznumeric_cast<float>(y), aznumeric_cast<float>(z), aznumeric_cast<float>(w));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromValues, k_categoryName, "{725D79B8-1CB1-4473-8480-4DE584C75540}", "returns a vector from elements", "X", "Y", "Z", "W");

        AZ_INLINE Vector4Type FromVector3AndNumber(Vector3Type source, const NumberType w)
        {
            return Vector4Type::CreateFromVector3AndFloat(source, aznumeric_cast<float>(w));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromVector3AndNumber, k_categoryName, "{577E2B26-BEC1-4CC7-B23B-5172ED1BFF6E}", "returns a vector with x,y,z from Source and w from W", "Source", "W");

        AZ_INLINE NumberType GetElement(const Vector4Type source, const NumberType index)
        {
            return source.GetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 3));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetElement, k_categoryName, "{8B63488C-24D9-4674-8DD5-1AC253A24656}", "returns the element corresponding to the index (0 -> x) (1 -> y) (2 -> z) (3 -> w)", "Source", "Index");

        AZ_INLINE std::tuple<NumberType, NumberType, NumberType, NumberType> GetElements(const Vector4Type source)
        {
            return std::make_tuple(source.GetX(), source.GetY(), source.GetZ(), source.GetW());
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(GetElements, k_categoryName, "{043DA46B-68A8-4810-B5F2-3B15CF639943}", "returns the elements of the source", "Source", "X", "Y", "Z", "W");

        AZ_INLINE Vector4Type Homogenize(Vector4Type source)
        {
            source.Homogenize();
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Homogenize, k_categoryName, "{9A3FAB19-0442-44A5-8454-12003BA146EE}", "returns a vector with all components divided by the w element", "Source");

        AZ_INLINE BooleanType IsClose(const Vector4Type a, const Vector4Type b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsClose, DefaultToleranceSIMD<2>, k_categoryName, "{51BFE8D5-1D1D-42C1-BB61-C8A3B1782EFF}", "returns true if the difference between A and B is less than tolerance, else false", "A", "B", "Tolerance");

        AZ_INLINE BooleanType IsFinite(const Vector4Type source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsFinite, k_categoryName, "{36F7B32E-F3A5-48E1-8FEB-B7ADE75BBDC3}", "returns true if every element in the source is finite, else false", "Source");

        AZ_INLINE BooleanType IsNormalized(const Vector4Type source, NumberType tolerance)
        {
            return source.IsNormalized(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsNormalized, DefaultToleranceSIMD<1>, k_categoryName, "{CCBD0216-42E0-4DAF-B206-424EDAD2247B}", "returns true if the length of the source is within tolerance of 1.0, else false", "Source", "Tolerance");

        AZ_INLINE BooleanType IsZero(const Vector4Type source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsZero, DefaultToleranceEpsilon<1>, k_categoryName, "{FDD689A3-AFA1-4FED-9B47-3909B92302C5}", "returns true if A is within tolerance of the zero vector, else false", "Source", "Tolerance");

        AZ_INLINE NumberType Length(const Vector4Type source)
        {
            return source.GetLength();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Length, k_categoryName, "{387B14BE-408B-4F95-ADB9-BC1C5BF75810}", "returns the magnitude of source", "Source");

        AZ_INLINE NumberType LengthReciprocal(const Vector4Type source)
        {
            return source.GetLengthReciprocal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(LengthReciprocal, k_categoryName, "{45457D77-2B42-4B5B-B320-BB25B01DC95A}", "returns the 1 / magnitude of the source", "Source");

        AZ_INLINE NumberType LengthSquared(const Vector4Type source)
        {
            return source.GetLengthSq();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(LengthSquared, k_categoryName, "{C2D83735-69EC-4DA4-81C7-AA2B3C45FAB3}", "returns the magnitude squared of the source, generally faster than getting the exact length", "Source");

        AZ_INLINE Vector4Type SetW(Vector4Type source, NumberType value)
        {
            source.SetW(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(SetW, k_categoryName, "{BE23146C-03DF-4A5C-BFF8-724A5E588F2E}", "returns a the vector(Source.X, Source.Y, Source.Z, W)", "Source", "W");

        AZ_INLINE Vector4Type SetX(Vector4Type source, NumberType value)
        {
            source.SetX(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(SetX, k_categoryName, "{A1D746E9-8B71-4452-AF07-96993547373C}", "returns a the vector(X, Source.Y, Source.Z, Source.W)", "Source", "X");

        AZ_INLINE Vector4Type SetY(Vector4Type source, NumberType value)
        {
            source.SetY(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(SetY, k_categoryName, "{D7C5E214-8E73-471C-8026-CB35D2D2FD58}", "returns a the vector(Source.X, Y, Source.Z, Source.W)", "Source", "Y");

        AZ_INLINE Vector4Type SetZ(Vector4Type source, NumberType value)
        {
            source.SetZ(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(SetZ, k_categoryName, "{5458F2CC-B8E2-43E2-B59F-8825A4D1F70A}", "returns a the vector(Source.X, Source.Y, Z, Source.W)", "Source", "Z");

        AZ_INLINE Vector4Type MultiplyByNumber(const Vector4Type source, const NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyByNumber, k_categoryName, "{24A667EA-6381-4F05-A856-A817BD8B9343}", "returns the vector Source with each element multiplied by Multipler", "Source", "Multiplier");

        AZ_INLINE Vector4Type MultiplyByVector(const Vector4Type source, const Vector4Type multiplier)
        {
            return source * multiplier;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(MultiplyByVector, k_categoryName, "{5F75288F-F7D5-4963-ABB3-14414DEB61E6}", "This node is deprecated, use Multiply (*), it provides contextual type and slots", "Source", "Multiplier");

        AZ_INLINE Vector4Type Negate(const Vector4Type source)
        {
            return -source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Negate, k_categoryName, "{E2EF2002-7F5B-4BF3-B19A-B160BE5C5ED3}", "returns the vector Source with each element multiplied by -1", "Source");

        AZ_INLINE Vector4Type Normalize(const Vector4Type source)
        {
            return source.GetNormalizedSafe();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Normalize, k_categoryName, "{64F79CB7-08C9-4622-BFC3-553682454E92}", "returns a unit length vector in the same direction as the source, or (1,0,0,0) if the source length is too small", "Source");

        AZ_INLINE std::tuple<Vector4Type, NumberType> NormalizeWithLength(Vector4Type source)
        {
            NumberType length(source.NormalizeSafeWithLength());
            return std::make_tuple(source, length);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(NormalizeWithLength, k_categoryName, "{7C7C479E-1235-4B0C-9102-481D52D9F13D}", "returns a unit length vector in the same direction as the source, and the length of source, or (1,0,0,0) if the source length is too small", "Source", "Normalized", "Length");

        AZ_INLINE Vector4Type Reciprocal(const Vector4Type source)
        {
            return source.GetReciprocal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Reciprocal, k_categoryName, "{F35F6E73-37BE-4A86-A7A8-E8A1253B7B45}", "returns the vector (1/x, 1/y, 1/z, 1/w) with elements from Source", "Source");

        AZ_INLINE Vector4Type Subtract(const Vector4Type& lhs, const Vector4Type& rhs)
        {
            return lhs - rhs;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Subtract, k_categoryName, "{A5FA6465-9C39-4A44-BD7C-E8ECF9503E46}", "This node is deprecated, use Subtract (-), it provides contextual type and slots", "A", "B");

        AZ_INLINE void DirectionToDefaults(Node& node)
        {
            SetDefaultValuesByIndex<0>::_(node, Data::Vector4Type());
            SetDefaultValuesByIndex<1>::_(node, Data::Vector4Type());
            SetDefaultValuesByIndex<2>::_(node, Data::NumberType(1.));
        }

        AZ_INLINE std::tuple<Vector4Type, NumberType> DirectionTo(const Vector4Type from, const Vector4Type to, NumberType optionalScale = 1.f)
        {
            Vector4Type r = to - from;
            float length = r.NormalizeWithLength();
            r.SetLength(static_cast<float>(optionalScale));
            return std::make_tuple(r, length);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(DirectionTo, DirectionToDefaults, k_categoryName, "{463762DE-E541-4AFE-80C2-FED1C5273319}", "Returns a direction vector between two points and the distance between them, by default the direction will be normalized, but it may be optionally scaled using the Scale parameter if different from 1.0", "From", "To", "Scale");

        using Registrar = RegistrarGeneric <
            AbsoluteNode,
            AddNode,
            DivideByNumberNode,
            DivideByVectorNode,
            DotNode,

#if ENABLE_EXTENDED_MATH_SUPPORT
            Dot3Node,
            FromElementNode,
#endif

            FromValuesNode,

#if ENABLE_EXTENDED_MATH_SUPPORT
            FromVector3AndNumberNode,
#endif

            GetElementNode,

#if ENABLE_EXTENDED_MATH_SUPPORT
            GetElementsNode,
            HomogenizeNode,
#endif

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
            MultiplyByVectorNode,
            NegateNode,
            NormalizeNode,

#if ENABLE_EXTENDED_MATH_SUPPORT
            NormalizeWithLengthNode,
#endif

            ReciprocalNode,
            SubtractNode,
            DirectionToNode
        > ;

    }
}

