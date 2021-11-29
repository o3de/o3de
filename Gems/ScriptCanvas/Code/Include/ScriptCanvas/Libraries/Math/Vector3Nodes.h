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
    namespace Vector3Nodes
    {
        using namespace MathNodeUtilities;
        using namespace Data;
        static constexpr const char* k_categoryName = "Math/Vector3";

        AZ_INLINE Vector3Type Absolute(const Vector3Type source)
        {
            return source.GetAbs();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Absolute, k_categoryName, "{92A4801A-15FB-4529-80BA-B880D8C24989}", "returns a vector with the absolute values of the elements of the source", "Source");

        AZ_INLINE Vector3Type Add(const Vector3Type lhs, const Vector3Type rhs)
        {
            return lhs + rhs;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Add, k_categoryName, "{0F554E23-9AB6-4D17-A517-C885ECD024F0}", "This node is deprecated, use Add (+), it provides contextual type and slots", "A", "B");

        AZ_INLINE Vector3Type AngleMod(const Vector3Type source)
        {
            return source.GetAngleMod();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(AngleMod, k_categoryName, "{BF5C12A8-F42D-43E7-9CE3-D16D30B182D2}", "wraps the angle in each element into the range [-pi, pi]", "Source")

            AZ_INLINE std::tuple<Vector3Type, Vector3Type> BuildTangentBasis(Vector3Type source)
        {
            std::tuple<Vector3Type, Vector3Type> tangentBitangent;
            source.NormalizeSafe();
            source.BuildTangentBasis(std::get<0>(tangentBitangent), std::get<1>(tangentBitangent));
            return tangentBitangent;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(BuildTangentBasis, k_categoryName, "{3EBA365F-063A-45A0-BDD1-ED0F995AD310}", "returns a tangent basis from the normal", "Normal", "Tangent", "Bitangent");

        AZ_INLINE Vector3Type Clamp(const Vector3Type source, const Vector3Type min, const Vector3Type max)
        {
            return source.GetClamp(min, max);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Clamp, k_categoryName, "{28305F88-0940-43C8-B0A8-B8CEB3B0B82A}", "returns vector clamped to [min, max] and equal to source if possible", "Source", "Min", "Max");

        AZ_INLINE Vector3Type Cosine(const Vector3Type source)
        {
            return source.GetCos();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Cosine, k_categoryName, "{556C428F-BE98-418D-9FE7-E9CBD30C0BDB}", "returns a vector from the cosine of each element from the source", "Source");

        AZ_INLINE Vector3Type Cross(const Vector3Type lhs, const Vector3Type rhs)
        {
            return lhs.Cross(rhs);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Cross, k_categoryName, "{6FAF4ACA-A100-4B71-ACF8-F1DB4674F51C}", "returns the vector cross product of A X B", "A", "B");

        AZ_INLINE Vector3Type CrossXAxis(const Vector3Type source)
        {
            return source.CrossXAxis();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(CrossXAxis, k_categoryName, "{41BF3063-26A3-4184-A482-35D6AC378B5B}", "returns the vector cross product of Source X X-Axis", "Source");

        AZ_INLINE Vector3Type CrossYAxis(const Vector3Type source)
        {
            return source.CrossYAxis();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(CrossYAxis, k_categoryName, "{2DC2D833-BB26-4F2B-96CF-D099718120F2}", "returns the vector cross product of Source X Y-Axis", "Source");

        AZ_INLINE Vector3Type CrossZAxis(const Vector3Type source)
        {
            return source.CrossZAxis();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(CrossZAxis, k_categoryName, "{1A960CF1-5790-4345-A3D3-31FBD59BC06F}", "returns the vector cross product of Source X Z-Axis", "Source");

        AZ_INLINE NumberType Distance(const Vector3Type a, const Vector3Type b)
        {
            return a.GetDistance(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Distance, k_categoryName, "{BFE43C43-3FDB-4E93-86D7-EB3766B75E7B}", "returns the distance from B to A, that is the magnitude of the vector (A - B)", "A", "B");

        AZ_INLINE NumberType DistanceSquared(const Vector3Type a, const Vector3Type b)
        {
            return a.GetDistanceSq(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(DistanceSquared, k_categoryName, "{D07DD389-31F7-435A-9329-903348B04DAB}", "returns the distance squared from B to A, (generally faster than the actual distance if only needed for comparison)", "A", "B");

        AZ_INLINE Vector3Type DivideByNumber(const Vector3Type source, const NumberType divisor)
        {
            if (AZ::IsClose(divisor, Data::NumberType(0), std::numeric_limits<Data::NumberType>::epsilon()))
            {
                AZ_Error("Script Canvas", false, "Division by zero");
                return Vector3Type::CreateZero();
            }

            return source / aznumeric_cast<float>(divisor);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(DivideByNumber, k_categoryName, "{16CC9068-93DA-44E0-83E4-78474DCE4046}", "returns the source with each element divided by Divisor", "Source", "Divisor");

        AZ_INLINE Vector3Type DivideByVector(const Vector3Type source, const Vector3Type divisor)
        {
            return source / divisor;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(DivideByVector, k_categoryName, "{61AD3E39-22B9-43C2-BC9F-E0EA4A7B0F8C}", "This node is deprecated, use Divide (/), it provides contextual type and slot configurations.", "Numerator", "Divisor");

        AZ_INLINE NumberType Dot(const Vector3Type lhs, const Vector3Type rhs)
        {
            return lhs.Dot(rhs);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Dot, k_categoryName, "{5DFA6260-C044-4798-A55C-3CF5F3DB45CE}", "returns the vector dot product of A dot B", "A", "B");

        AZ_INLINE Vector3Type FromElement(Vector3Type source, const NumberType index, const NumberType value)
        {
            source.SetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 2), aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromElement, k_categoryName, "{C35B5119-40B4-48ED-93B1-D70446985A51}", "returns a vector with the element corresponding to the index (0 -> x) (1 -> y) (2 -> z) set to the value", "Source", "Index", "Value");

        AZ_INLINE Vector3Type FromLength(Vector3Type source, const NumberType length)
        {
            source.SetLength(aznumeric_cast<float>(length));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromLength, k_categoryName, "{D10C2172-CB42-44E3-9C16-FA51F8A5A235}", "returns a vector with the same direction as Source scaled to Length", "Source", "Length");

        AZ_INLINE Vector3Type FromValues(NumberType x, NumberType y, NumberType z)
        {
            return Vector3Type(aznumeric_cast<float>(x), aznumeric_cast<float>(y), aznumeric_cast<float>(z));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromValues, k_categoryName, "{AA4B21AC-26B1-41E2-9AE4-19F4FFF050CC}", "returns a vector from elements", "X", "Y", "Z");

        AZ_INLINE NumberType GetElement(const Vector3Type source, const NumberType index)
        {
            return source.GetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 2));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetElement, k_categoryName, "{117EB15C-BDBA-41D2-8904-C7CE34E34BB9}", "returns the element corresponding to the index (0 -> x) (1 -> y) (2 -> z)", "Source", "Index");

        AZ_INLINE BooleanType IsClose(const Vector3Type a, const Vector3Type b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsClose, DefaultToleranceSIMD<2>, k_categoryName, "{4E75F538-DC03-4AEB-B38D-102F7337F36D}", "returns true if the difference between A and B is less than tolerance, else false", "A", "B", "Tolerance");

        AZ_INLINE BooleanType IsFinite(const Vector3Type source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsFinite, k_categoryName, "{6C1CB6E9-6EE3-4F6F-8B24-6DB4906B7DC7}", "returns true if every element in the source is finite, else false", "Source");

        AZ_INLINE BooleanType IsNormalized(const Vector3Type source, NumberType tolerance)
        {
            return source.IsNormalized(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsNormalized, DefaultToleranceSIMD<1>, k_categoryName, "{EFFC389A-CCE7-4350-8E3F-C2B728CD03C6}", "returns true if the length of the source is within tolerance of 1.0, else false", "Source", "Tolerance");

        AZ_INLINE BooleanType IsPerpendicular(const Vector3Type a, const Vector3Type b, NumberType tolerance)
        {
            return a.IsPerpendicular(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsPerpendicular, DefaultToleranceSIMD<2>, k_categoryName, "{D283EB50-8493-444E-9333-90E1F70565FF}", "returns true if A is within tolerance of perpendicular with B, that is if Dot(A, B) < tolerance, else false", "A", "B", "Tolerance");

        AZ_INLINE BooleanType IsZero(const Vector3Type source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsZero, DefaultToleranceEpsilon<1>, k_categoryName, "{AFCE279C-9BB2-446B-9C18-8A9D9FBCCD6C}", "returns true if A is within tolerance of the zero vector, else false", "Source", "Tolerance");

        AZ_INLINE NumberType Length(const Vector3Type source)
        {
            return source.GetLength();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Length, k_categoryName, "{4CD73E38-C98A-4B5A-9BAA-6E8B69AB7201}", "returns the magnitude of source", "Source");

        AZ_INLINE NumberType LengthReciprocal(const Vector3Type source)
        {
            return source.GetLengthReciprocal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(LengthReciprocal, k_categoryName, "{4B06E22C-E2B5-4624-88F1-1406CEC423A2}", "returns the 1 / magnitude of the source", "Source");

        AZ_INLINE NumberType LengthSquared(const Vector3Type source)
        {
            return source.GetLengthSq();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(LengthSquared, k_categoryName, "{650E8F83-0FDD-4C97-A6CD-83D8688D2645}", "returns the magnitude squared of the source, generally faster than getting the exact length", "Source");

        AZ_INLINE Vector3Type Lerp(const Vector3Type& from, const Vector3Type& to, NumberType t)
        {
            return from.Lerp(to, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Lerp, k_categoryName, "{AA063267-DA0F-4407-9356-30B4E89A9FA4}", "returns the linear interpolation (From + ((To - From) * T)", "From", "To", "T");

        AZ_INLINE Vector3Type Max(const Vector3Type a, const Vector3Type b)
        {
            return a.GetMax(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Max, k_categoryName, "{1FA35DE2-9D82-4628-99D0-25968734E551}", "returns the vector (max(A.x, B.x), max(A.y, B.y), max(A.z, B.z))", "A", "B");

        AZ_INLINE Vector3Type Min(const Vector3Type a, const Vector3Type b)
        {
            return a.GetMin(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Min, k_categoryName, "{16B21396-9677-437E-B894-089AE2EC0E13}", "returns the vector (min(A.x, B.x), min(A.y, B.y), min(A.z, B.z))", "A", "B");

        AZ_INLINE Vector3Type SetX(Vector3Type source, NumberType value)
        {
            source.SetX(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(SetX, k_categoryName, "{835EC2C1-EF57-481E-9154-B65E86B1A388}", "returns a the vector(X, Source.Y, Source.Z)", "Source", "X");

        AZ_INLINE Vector3Type SetY(Vector3Type source, NumberType value)
        {
            source.SetY(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(SetY, k_categoryName, "{72A0941A-DA2C-47AB-BCC2-A26D14E980E2}", "returns a the vector(Source.X, Y, Source.Z)", "Source", "Y");

        AZ_INLINE Vector3Type SetZ(Vector3Type source, NumberType value)
        {
            source.SetZ(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(SetZ, k_categoryName, "{813CFA11-5D90-4864-BFAC-A28F97B6D80F}", "returns a the vector(Source.X, Source.Y, Z)", "Source", "Z");

        AZ_INLINE Vector3Type MultiplyAdd(Vector3Type a, const Vector3Type b, const Vector3Type c)
        {
            return a.GetMadd(b, c);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyAdd, k_categoryName, "{54541149-91D5-42E9-82B7-9674E5BDED12}", "returns the vector (A * B) + C", "A", "B", "C");

        AZ_INLINE Vector3Type MultiplyByNumber(const Vector3Type source, const NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyByNumber, k_categoryName, "{47097B44-8B91-4589-AED2-83752300E0D7}", "returns the vector Source with each element multiplied by Multipler", "Source", "Multiplier");

        AZ_INLINE Vector3Type MultiplyByVector(const Vector3Type source, const Vector3Type multiplier)
        {
            return source * multiplier;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(MultiplyByVector, k_categoryName, "{42847AC6-8790-4DA9-9B9C-E704AC957883}", "This node is deprecated, use Multiply (*), it provides contextual type and slots", "Source", "Multiplier");

        AZ_INLINE Vector3Type Negate(const Vector3Type source)
        {
            return -source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Negate, k_categoryName, "{017C6F84-DECC-489D-85E1-A999B9AD986B}", "returns the vector Source with each element multiplied by -1", "Source");

        AZ_INLINE Vector3Type Normalize(const Vector3Type source)
        {
            return source.GetNormalizedSafe();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Normalize, k_categoryName, "{971E7456-4BDF-4FB3-A418-D6ECAC186FD5}", "returns a unit length vector in the same direction as the source, or (1,0,0) if the source length is too small", "Source");

        AZ_INLINE std::tuple<Vector3Type, NumberType> NormalizeWithLength(Vector3Type source)
        {
            NumberType length(source.NormalizeSafeWithLength());
            return std::make_tuple(source, length);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(NormalizeWithLength, k_categoryName, "{A9F29CC6-7FBF-400F-92C6-18F28AD256B9}", "returns a unit length vector in the same direction as the source, and the length of source, or (1,0,0) if the source length is too small", "Source", "Normalized", "Length");

        AZ_INLINE Vector3Type Project(Vector3Type a, const Vector3Type b)
        {
            a.Project(b);
            return a;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Project, k_categoryName, "{DD53C3CF-5543-449B-8076-387CD3D66291}", "returns the vector of A projected onto B, (Dot(A, B)/(Dot(B, B)) * B", "A", "B");

        AZ_INLINE Vector3Type Reciprocal(const Vector3Type source)
        {
            return source.GetReciprocal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Reciprocal, k_categoryName, "{09B243E6-AAAF-4B30-BF22-FDB074700D05}", "returns the vector (1/x, 1/y, 1/z) with elements from Source", "Source");

        AZ_INLINE Vector3Type Sine(const Vector3Type source)
        {
            return source.GetSin();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Sine, k_categoryName, "{3FAF95BE-9757-42BF-9553-950615A25CC3}", "returns a vector from the sine of each element from the source", "Source");

        AZ_INLINE std::tuple<Vector3Type, Vector3Type> SineCosine(const Vector3Type source)
        {
            Vector3Type sin, cos;
            source.GetSinCos(sin, cos);
            return std::make_tuple(sin, cos);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(SineCosine, k_categoryName, "{04EE253D-680D-4F95-A451-837EAE104E88}", "returns a vector from the sine of each element from the source, and from the cosine of each element from the source", "Source", "Sine", "Cosine");

        AZ_INLINE Vector3Type Slerp(const Vector3Type from, const Vector3Type to, const NumberType t)
        {
            return from.Slerp(to, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Slerp, k_categoryName, "{F3EA1D86-33DD-46BA-8A88-9FE6AB181E01}", "returns a vector that is the spherical linear interpolation T, between From and To", "From", "To", "T");

        AZ_INLINE Vector3Type Subtract(const Vector3Type& lhs, const Vector3Type& rhs)
        {
            return lhs - rhs;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Subtract, k_categoryName, "{0DE69020-4DB2-4559-9C29-6CD8EAC05F1E}", "This node is deprecated, use Subtract (-), it provides contextual type and slots", "A", "B");

        AZ_INLINE Vector3Type XAxisCross(const Vector3Type source)
        {
            return source.XAxisCross();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(XAxisCross, k_categoryName, "{C414932E-3709-43C6-843F-53ECE0EF8230}", "returns the vector cross product of X-Axis X Source", "Source");

        AZ_INLINE Vector3Type YAxisCross(const Vector3Type source)
        {
            return source.YAxisCross();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(YAxisCross, k_categoryName, "{AD4811A8-4DFE-4660-8638-1E981545D758}", "returns the vector cross product of Y-Axis X Source", "Source");

        AZ_INLINE Vector3Type ZAxisCross(const Vector3Type source)
        {
            return source.ZAxisCross();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ZAxisCross, k_categoryName, "{29206E84-392C-412E-9DD5-781B2759260D}", "returns the vector cross product of Z-Axis X Source", "Source");

        AZ_INLINE void DirectionToDefaults(Node& node)
        {
            SetDefaultValuesByIndex<0>::_(node, Data::Vector3Type());
            SetDefaultValuesByIndex<1>::_(node, Data::Vector3Type());
            SetDefaultValuesByIndex<2>::_(node, Data::NumberType(1.));
        }

        AZ_INLINE std::tuple<Vector3Type, NumberType> DirectionTo(const Vector3Type from, const Vector3Type to, NumberType optionalScale = 1.f)
        {
            Vector3Type r = to - from;
            float length = r.NormalizeWithLength();
            r.SetLength(static_cast<float>(optionalScale));
            return std::make_tuple(r, length);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(DirectionTo, DirectionToDefaults, k_categoryName, "{28FBD529-4C9A-4E34-B8A0-A13B5DB3C331}", "Returns a direction vector between two points and the distance between them, by default the direction will be normalized, but it may be optionally scaled using the Scale parameter if different from 1.0", "From", "To", "Scale");


        using Registrar = RegistrarGeneric <
            AbsoluteNode
            , AddNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , AngleModNode
#endif

            , BuildTangentBasisNode
            , ClampNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , CosineNode
#endif

            , CrossNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , CrossXAxisNode
            , CrossYAxisNode
            , CrossZAxisNode
#endif

            , DistanceNode
            , DistanceSquaredNode
            , DivideByNumberNode
            , DivideByVectorNode
            , DotNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , FromElementNode
            , FromLengthNode
#endif

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

#if ENABLE_EXTENDED_MATH_SUPPORT
            , MultiplyAddNode
#endif

            , MultiplyByNumberNode
            , MultiplyByVectorNode
            , NegateNode
            , NormalizeNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , NormalizeWithLengthNode
#endif

            , ProjectNode
            , ReciprocalNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , SineNode
            , SineCosineNode
#endif

            , SlerpNode
            , SubtractNode
            , DirectionToNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , XAxisCrossNode
            , YAxisCrossNode
            , ZAxisCrossNode
#endif

        > ;

    }
}

