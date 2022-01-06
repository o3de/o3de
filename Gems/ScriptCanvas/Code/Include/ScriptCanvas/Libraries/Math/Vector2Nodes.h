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
    namespace Vector2Nodes
    {
        using namespace MathNodeUtilities;
        using namespace Data;
        static constexpr const char* k_categoryName = "Math/Vector2";

        AZ_INLINE Vector2Type Absolute(const Vector2Type source)
        {
            return source.GetAbs();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Absolute, k_categoryName, "{68DE5669-9D35-4414-AE17-51BF00ED6738}", "returns a vector with the absolute values of the elements of the source", "Source");

        AZ_INLINE Vector2Type Add(const Vector2Type lhs, const Vector2Type rhs)
        {
            return lhs + rhs;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Add, k_categoryName, "{24E5FD67-43D7-44C0-B9E8-0CA02A43777A}", "This node is deprecated, use Add (+), it provides contextual type and slots", "A", "B");

        AZ_INLINE Vector2Type Angle(NumberType angle)
        {
            return Vector2Type::CreateFromAngle(aznumeric_cast<float>(angle));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Angle, k_categoryName, "{4D77F825-C4CE-455C-802F-34F6C8B7A1C8}", "returns a unit length vector from an angle in radians", "Angle");

        AZ_INLINE Vector2Type Clamp(const Vector2Type source, const Vector2Type min, const Vector2Type max)
        {
            return source.GetClamp(min, max);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Clamp, k_categoryName, "{F2812289-F53C-4603-AE47-93902D9B06E0}", "returns vector clamped to [min, max] and equal to source if possible", "Source", "Min", "Max");

        AZ_INLINE NumberType Distance(const Vector2Type a, const Vector2Type b)
        {
            return a.GetDistance(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Distance, k_categoryName, "{6F37E3A7-8FBA-4DC3-83C0-659075E9F3E0}", "returns the distance from B to A, that is the magnitude of the vector (A - B)", "A", "B");

        AZ_INLINE NumberType DistanceSquared(const Vector2Type a, const Vector2Type b)
        {
            return a.GetDistanceSq(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(DistanceSquared, k_categoryName, "{23C6FD73-825E-4FFB-83B6-67FE1C9D1271}", "returns the distance squared from B to A, (generally faster than the actual distance if only needed for comparison)", "A", "B");

        AZ_INLINE Vector2Type DivideByNumber(const Vector2Type source, const NumberType divisor)
        {
            if (AZ::IsClose(divisor, Data::NumberType(0), std::numeric_limits<Data::NumberType>::epsilon()))
            {
                AZ_Error("Script Canvas", false, "Division by zero");
                return Vector2Type::CreateZero();
            }

            return source / aznumeric_cast<float>(divisor);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(DivideByNumber, k_categoryName, "{DEB8225C-2A9C-40A2-AC81-0FA105637AF9}", "returns the source with each element divided by Divisor", "Source", "Divisor");

        AZ_INLINE Vector2Type DivideByVector(const Vector2Type source, const Vector2Type divisor)
        {
            return source / divisor;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(DivideByVector, k_categoryName, "{6043B1B4-3E0A-455D-860B-588DE90C7C6C}", "This node is deprecated, use Divide (/), it provides contextual type and slots", "Numerator", "Divisor");

        AZ_INLINE NumberType Dot(const Vector2Type lhs, const Vector2Type rhs)
        {
            return lhs.Dot(rhs);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Dot, k_categoryName, "{F61FF592-E75D-4897-A081-AFE944DDFD58}", "returns the vector dot product of A dot B", "A", "B");

        AZ_INLINE Vector2Type FromElement(Vector2Type source, const NumberType index, const NumberType value)
        {
            source.SetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 1), aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromElement, k_categoryName, "{BE4D21C2-7485-4759-B215-32874D741FD9}", "returns a vector with the element corresponding to the index (0 -> x) (1 -> y)set to the value", "Source", "Index", "Value");

        AZ_INLINE Vector2Type FromLength(Vector2Type source, const NumberType length)
        {
            source.SetLength(aznumeric_cast<float>(length));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromLength, k_categoryName, "{542063A3-5A31-4CA2-A365-FC4201BF3896}", "returns a vector with the same direction as Source scaled to Length", "Source", "Length");

        AZ_INLINE Vector2Type FromValues(NumberType x, NumberType y)
        {
            return Vector2Type(aznumeric_cast<float>(x), aznumeric_cast<float>(y));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromValues, k_categoryName, "{7CF4EC50-45A9-436D-AE08-54F27EA979BB}", "returns a vector from elements", "X", "Y");

        AZ_INLINE NumberType GetElement(const Vector2Type source, const NumberType index)
        {
            return source.GetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 1));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetElement, k_categoryName, "{C29C47AC-3847-48DB-9CC0-4C403C1B276C}", "returns the element corresponding to the index (0 -> x) (1 -> y)", "Source", "Index");

        AZ_INLINE std::tuple<NumberType, NumberType> GetElements(const Vector2Type source)
        {
            return std::make_tuple(aznumeric_caster(source.GetX()), aznumeric_caster(source.GetY()));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(GetElements, k_categoryName, "{B924EE1C-DA56-4FE4-9193-989B3573262C}", "returns the elements of the source", "Source", "X", "Y");

        AZ_INLINE BooleanType IsClose(const Vector2Type a, const Vector2Type b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsClose, DefaultToleranceSIMD<2>, k_categoryName, "{3A0B3386-2BF9-43FB-A003-DE026DBD7DFA}", "returns true if the difference between A and B is less than tolerance, else false", "A", "B", "Tolerance");

        AZ_INLINE BooleanType IsFinite(const Vector2Type source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsFinite, k_categoryName, "{80578C30-DD70-448A-9DE5-662734E14335}", "returns true if every element in the source is finite, else false", "Source");

        AZ_INLINE BooleanType IsNormalized(const Vector2Type source, NumberType tolerance)
        {
            return source.IsNormalized(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsNormalized, DefaultToleranceSIMD<1>, k_categoryName, "{C9EF4543-CF4D-43D5-96B1-E2DBFEA929C8}", "returns true if the length of the source is within tolerance of 1.0, else false", "Source", "Tolerance");

        AZ_INLINE BooleanType IsZero(const Vector2Type source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsZero, DefaultToleranceEpsilon<1>, k_categoryName, "{0A74D60B-F59E-47E8-8D68-BE69843D865B}", "returns true if A is within tolerance of the zero vector, else false", "Source", "Tolerance");

        AZ_INLINE NumberType Length(const Vector2Type source)
        {
            return source.GetLength();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Length, k_categoryName, "{39887B90-753A-46F8-A46A-F8B237FEAE2B}", "returns the magnitude of source", "Source");

        AZ_INLINE NumberType LengthSquared(const Vector2Type source)
        {
            return source.GetLengthSq();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(LengthSquared, k_categoryName, "{AC956D8F-E66A-4D8C-B82D-A920732847EC}", "returns the magnitude squared of the source, generally faster than getting the exact length", "Source");

        AZ_INLINE Vector2Type Lerp(const Vector2Type& from, const Vector2Type& to, NumberType t)
        {
            return from.Lerp(to, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Lerp, k_categoryName, "{9BFB41C7-B665-4462-B237-1CD317DB1C7E}", "returns the linear interpolation (From + ((To - From) * T)", "From", "To", "T");

        AZ_INLINE Vector2Type Max(const Vector2Type a, const Vector2Type b)
        {
            return a.GetMax(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Max, k_categoryName, "{DFAA23D9-8D28-4746-B224-01807258A473}", "returns the vector (max(A.x, B.x), max(A.y, B.y))", "A", "B");

        AZ_INLINE Vector2Type Min(const Vector2Type a, const Vector2Type b)
        {
            return a.GetMin(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Min, k_categoryName, "{815685B8-B877-4D54-9E11-D0161185B4B9}", "returns the vector (min(A.x, B.x), min(A.y, B.y))", "A", "B");

        AZ_INLINE Vector2Type SetX(Vector2Type source, NumberType value)
        {
            source.SetX(aznumeric_caster(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(SetX, k_categoryName, "{A5C2933F-C871-4915-B3AA-0C31FCFFEC15}", "returns a the vector(X, Source.Y)", "Source", "X");

        AZ_INLINE Vector2Type SetY(Vector2Type source, NumberType value)
        {
            source.SetY(aznumeric_caster(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(SetY, k_categoryName, "{824BE8DB-BB03-49A2-A829-34DAE2C66AF4}", "returns a the vector(Source.X, Y)", "Source", "Y");

        AZ_INLINE Vector2Type MultiplyAdd(Vector2Type a, const Vector2Type b, const Vector2Type c)
        {
            return a.GetMadd(b, c);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyAdd, k_categoryName, "{2FC72973-CB69-4DC1-BD35-A699AC838AC4}", "returns the vector (A * B) + C", "A", "B", "C");

        AZ_INLINE Vector2Type MultiplyByNumber(const Vector2Type source, const NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyByNumber, k_categoryName, "{4B7A44C2-383E-4F41-B7F9-FA87F946B46B}", "returns the vector Source with each element multiplied by Multiplier", "Source", "Multiplier");

        AZ_INLINE Vector2Type MultiplyByVector(const Vector2Type source, const Vector2Type multiplier)
        {
            return source * multiplier;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(MultiplyByVector, k_categoryName, "{1C997C54-D457-4101-8210-6FAA48105E64}", "This node is deprecated, use Multiply (*), it provides contextual type and slots", "Source", "Multiplier");

        AZ_INLINE Vector2Type Negate(const Vector2Type source)
        {
            return -source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Negate, k_categoryName, "{AD35E721-1591-433D-8B88-0CC431C58EE6}", "returns the vector Source with each element multiplied by -1", "Source");

        AZ_INLINE Vector2Type Normalize(const Vector2Type source)
        {
            return source.GetNormalizedSafe();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Normalize, k_categoryName, "{2FB16EFF-5B3D-456E-B791-43F19C03BB83}", "returns a unit length vector in the same direction as the source, or (1,0,0) if the source length is too small", "Source");

        AZ_INLINE std::tuple<Vector2Type, NumberType> NormalizeWithLength(Vector2Type source)
        {
            NumberType length(source.NormalizeSafeWithLength());
            return std::make_tuple(source, length);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(NormalizeWithLength, k_categoryName, "{3D960919-D4F4-4CEF-AD8D-9FAC13D20B63}", "returns a unit length vector in the same direction as the source, and the length of source, or (1,0,0) if the source length is too small", "Source", "Normalized", "Length");

        AZ_INLINE Vector2Type Project(Vector2Type a, const Vector2Type b)
        {
            a.Project(b);
            return a;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Project, k_categoryName, "{67FA83DA-E026-4324-8034-067EC9505C7E}", "returns the vector of A projected onto B, (Dot(A, B)/(Dot(B, B)) * B", "A", "B");

        AZ_INLINE Vector2Type Slerp(const Vector2Type from, const Vector2Type to, const NumberType t)
        {
            return from.Slerp(to, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Slerp, k_categoryName, "{E8221B8F-AD1F-42B5-9389-7DEDE5C3B3C9}", "returns a vector that is the spherical linear interpolation T, between From and To", "From", "To", "T");

        AZ_INLINE Vector2Type Subtract(const Vector2Type& lhs, const Vector2Type& rhs)
        {
            return lhs - rhs;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Subtract, k_categoryName, "{3D87036A-D1BD-475E-85C7-66922F810885}", "This node is deprecated, use Subtract (-), it provides contextual type and slots", "A", "B");

        AZ_INLINE Vector2Type ToPerpendicular(const Vector2Type source)
        {
            return source.GetPerpendicular();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ToPerpendicular, k_categoryName, "{CC4DC102-8B50-4828-BA94-0586F34E0D37}", "returns the vector (-Source.y, Source.x), a 90 degree, positive rotation", "Source");

        AZ_INLINE void DirectionToDefaults(Node& node)
        {
            SetDefaultValuesByIndex<0>::_(node, Data::Vector2Type());
            SetDefaultValuesByIndex<1>::_(node, Data::Vector2Type());
            SetDefaultValuesByIndex<2>::_(node, Data::NumberType(1.));
        }

        AZ_INLINE std::tuple<Vector2Type, NumberType> DirectionTo(const Vector2Type from, const Vector2Type to, NumberType optionalScale = 1.f)
        {
            Vector2Type r = to - from;
            float length = r.NormalizeWithLength();
            r.SetLength(static_cast<float>(optionalScale));
            return std::make_tuple(r, length);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(DirectionTo, DirectionToDefaults, k_categoryName, "{49A2D7F6-6CD3-420E-8A79-D46B00DB6CED}", "Returns a direction vector between two points and the distance between them, by default the direction will be normalized, but it may be optionally scaled using the Scale parameter if different from 1.0", "From", "To", "Scale");

        using Registrar = RegistrarGeneric <
            AbsoluteNode
            , AddNode
            , AngleNode
            , ClampNode
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

#if ENABLE_EXTENDED_MATH_SUPPORT
            , GetElementsNode
#endif

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
            , SlerpNode
            , SubtractNode
            , ToPerpendicularNode
            , DirectionToNode
        > ;

    }
}

