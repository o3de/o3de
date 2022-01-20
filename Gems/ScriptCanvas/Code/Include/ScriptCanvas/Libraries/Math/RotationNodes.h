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
    namespace QuaternionNodes
    {
        using namespace Data;
        using namespace MathNodeUtilities;
        static constexpr const char* k_categoryName = "Math/Quaternion";

        AZ_INLINE QuaternionType Add(QuaternionType a, QuaternionType b)
        {
            return a + b;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Add, k_categoryName, "{D20FAD3C-39CD-4369-BA0D-32AD5E6E23EB}", "This node is deprecated, use Add (+), it provides contextual type and slots", "A", "B");

        AZ_INLINE QuaternionType Conjugate(QuaternionType source)
        {
            return source.GetConjugate();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Conjugate, k_categoryName, "{A1279F70-E211-41F2-8974-84E998206B0D}", "returns the conjugate of the source, (-x, -y, -z, w)", "Source");

        AZ_INLINE QuaternionType ConvertTransformToRotation(const TransformType& transform)
        {
            return AZ::ConvertEulerRadiansToQuaternion(AZ::ConvertTransformToEulerRadians(transform));
        }

        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ConvertTransformToRotation, k_categoryName, "{C878982F-1B6B-4555-8723-7FF3830C8032}", "", "Transform");

        AZ_INLINE QuaternionType DivideByNumber(QuaternionType source, NumberType divisor)
        {
            if (AZ::IsClose(divisor, Data::NumberType(0), std::numeric_limits<Data::NumberType>::epsilon()))
            {
                AZ_Error("Script Canvas", false, "Division by zero");
                return QuaternionType::CreateIdentity();
            }

            return source / aznumeric_cast<float>(divisor);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(DivideByNumber, k_categoryName, "{94C8A813-C20E-4194-98B6-8618CE872BAA}", "returns the Numerator with each element divided by Divisor", "Numerator", "Divisor");

        AZ_INLINE NumberType Dot(QuaternionType a, QuaternionType b)
        {
            return a.Dot(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Dot, k_categoryName, "{01FED020-6EB1-4A69-AFC7-7305FCA7FC97}", "returns the Dot product of A and B", "A", "B");

        AZ_INLINE QuaternionType FromAxisAngleDegrees(Vector3Type axis, NumberType degrees)
        {
            return QuaternionType::CreateFromAxisAngle(axis, AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromAxisAngleDegrees, k_categoryName, "{109952D1-2DB7-48C3-970D-B8DB4C96FE54}", "returns the rotation created from Axis the angle Degrees", "Axis", "Degrees");

        AZ_INLINE QuaternionType FromElement(QuaternionType source, NumberType index, NumberType value)
        {
            source.SetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 3), aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromElement, k_categoryName, "{86F85D23-FB6E-4364-AE1E-8260D26988E0}", "returns a rotation with a the element corresponding to the index (0 -> x)(1 -> y)(2 -> z)(3 -> w)", "Source", "Index", "Value");

        AZ_INLINE QuaternionType FromElements(NumberType x, NumberType y, NumberType z, NumberType w)
        {
            return QuaternionType(aznumeric_cast<float>(x), aznumeric_cast<float>(y), aznumeric_cast<float>(z), aznumeric_cast<float>(w));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromElements, k_categoryName, "{9E5A648C-1378-4EDE-B28C-F867CBC89968}", "returns a rotation from elements", "X", "Y", "Z", "W");

        AZ_INLINE QuaternionType FromMatrix3x3(const Matrix3x3Type& source)
        {
            return QuaternionType::CreateFromMatrix3x3(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromMatrix3x3, k_categoryName, "{AFB1A899-D71D-48C8-8C76-086146B7B6EE}", "returns a rotation created from the 3x3 matrix source", "Source");

        AZ_INLINE QuaternionType FromMatrix4x4(const Matrix4x4Type& source)
        {
            return QuaternionType::CreateFromMatrix4x4(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromMatrix4x4, k_categoryName, "{CD6F0D36-EC89-4D3E-920E-267D47F819BE}", "returns a rotation created from the 4x4 matrix source", "Source");

        AZ_INLINE QuaternionType FromTransform(const TransformType& source)
        {
            return source.GetRotation();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromTransform, k_categoryName, "{B6B224CC-7454-4D99-B473-C0A77D4FB885}", "returns a rotation created from the rotation part of the transform source", "Source");

        AZ_INLINE QuaternionType FromVector3(Vector3Type source)
        {
            return QuaternionType::CreateFromVector3(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromVector3, k_categoryName, "{5FA694EA-B2EA-4403-9144-9171A7AA8636}", "returns a rotation with the imaginary elements set to the Source, and the real element set to 0", "Source");

        AZ_INLINE QuaternionType FromVector3AndValue(Vector3Type imaginary, NumberType real)
        {
            return QuaternionType::CreateFromVector3AndValue(imaginary, aznumeric_cast<float>(real));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromVector3AndValue, k_categoryName, "{955FE6EB-7C38-4587-BBB7-9C886ACEAF94}", "returns a rotation with the imaginary elements from Imaginary and the real element from Real", "Imaginary", "Real");

        AZ_INLINE NumberType GetElement(QuaternionType source, NumberType index)
        {
            return source.GetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 3));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetElement, k_categoryName, "{1B1452DA-E23C-43DC-A0AD-37AAC36E38FA}", "returns the element of Source corresponding to the Index (0 -> x)(1 -> y)(2 -> z)(3 -> w)", "Source", "Index");

        AZ_INLINE std::tuple<NumberType, NumberType, NumberType, NumberType> GetElements(const QuaternionType source)
        {
            return std::make_tuple(source.GetX(), source.GetY(), source.GetZ(), source.GetW());
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(GetElements, k_categoryName, "{1384FAFE-9435-49C8-941A-F2694A4D3EA4}", "returns the elements of the source", "Source", "X", "Y", "Z", "W");

        AZ_INLINE QuaternionType InvertFull(QuaternionType source)
        {
            return source.GetInverseFull();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(InvertFull, k_categoryName, "{DF936099-48C8-4924-A91D-6B93245D8F30}", "returns the inverse for any rotation, not just unit rotations", "Source");

        AZ_INLINE BooleanType IsClose(QuaternionType a, QuaternionType b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsClose, DefaultToleranceSIMD<2>, k_categoryName, "{E0150AD6-6CBE-494E-9A1D-1E7E7C0A114F}", "returns true if A and B are within Tolerance of each other", "A", "B", "Tolerance");

        AZ_INLINE BooleanType IsFinite(QuaternionType a)
        {
            return a.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsFinite, k_categoryName, "{503B1229-74E8-40FE-94DE-C4387806BDB0}", "returns true if every element in Source is finite", "Source");

        AZ_INLINE BooleanType IsIdentity(QuaternionType source, NumberType tolerance)
        {
            return source.IsIdentity(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsIdentity, DefaultToleranceSIMD<1>, k_categoryName, "{E7BB6123-E21A-4B51-B35E-BAA3DF239AB8}", "returns true if Source is within Tolerance of the Identity rotation", "Source", "Tolerance");

        AZ_INLINE BooleanType IsZero(QuaternionType source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsZero, DefaultToleranceSIMD<1>, k_categoryName, "{8E71A7DC-5FCA-4569-A2C4-3A85B5070AA1}", "returns true if Source is within Tolerance of the Zero rotation", "Source", "Tolerance");

        AZ_INLINE NumberType Length(QuaternionType source)
        {
            return source.GetLength();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Length, k_categoryName, "{61025A32-F17E-4945-95AC-6F12C1A77B7F}", "This node is deprecated, use the Length node, it provides contextual type and slot configurations", "Source");

        AZ_INLINE NumberType LengthReciprocal(QuaternionType source)
        {
            return source.GetLengthReciprocal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(LengthReciprocal, k_categoryName, "{C4019E78-59F8-4023-97F9-1FC6C2DC94C8}", "returns the reciprocal length of Source", "Source");

        AZ_INLINE NumberType LengthSquared(QuaternionType source)
        {
            return source.GetLengthSq();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(LengthSquared, k_categoryName, "{825A0F09-CDFA-4C80-8177-003B154F213A}", "returns the square of the length of Source", "Source");

        AZ_INLINE QuaternionType Lerp(QuaternionType a, QuaternionType b, NumberType t)
        {
            return a.Lerp(b, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Lerp, k_categoryName, "{91CF1C54-89C6-4A00-A53D-20C58454C4EC}", "returns a the linear interpolation between From and To by the amount T", "From", "To", "T");

        AZ_INLINE QuaternionType ModX(QuaternionType source, NumberType value)
        {
            source.SetX(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModX, k_categoryName, "{567CDD18-027E-4DA1-81D1-CDA7FFD9DB8B}", "returns a the rotation(X, Source.Y, Source.Z, Source.W)", "Source", "X");

        AZ_INLINE QuaternionType ModY(QuaternionType source, NumberType value)
        {
            source.SetY(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModY, k_categoryName, "{64BD2718-D004-40CA-A5B0-4F68A5D823A0}", "returns a the rotation(Source.X, Y, Source.Z, Source.W)", "Source", "Y");

        AZ_INLINE QuaternionType ModZ(QuaternionType source, NumberType value)
        {
            source.SetZ(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModZ, k_categoryName, "{0CDD1B61-4DC4-480C-A9EE-97251712705B}", "returns a the rotation(Source.X, Source.Y, Z, Source.W)", "Source", "Z");

        AZ_INLINE QuaternionType ModW(QuaternionType source, NumberType value)
        {
            source.SetW(aznumeric_cast<float>(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModW, k_categoryName, "{FC2B0283-7530-4927-8AFA-155E0C53C5D9}", "returns a the rotation(Source.X, Source.Y, Source.Z, W)", "Source", "W");

        AZ_INLINE QuaternionType MultiplyByNumber(QuaternionType source, NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyByNumber, k_categoryName, "{B8911827-A1E7-4ECE-8503-9B31DD9C63C8}", "returns the Source with each element multiplied by Multiplier", "Source", "Multiplier");

        AZ_INLINE QuaternionType MultiplyByRotation(QuaternionType A, QuaternionType B)
        {
            return A * B;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(MultiplyByRotation, k_categoryName, "{F4E19446-CBC1-46BF-AEC3-17FCC3FA9DEE}", "This node is deprecated, use Multiply (*), it provides contextual type and slots", "A", "B");

        AZ_INLINE QuaternionType Negate(QuaternionType source)
        {
            return -source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Negate, k_categoryName, "{5EA770E6-6F6C-4838-B2D8-B2C487BF32E7}", "returns the Source with each element negated", "Source");

        AZ_INLINE QuaternionType Normalize(QuaternionType source)
        {
            return source.GetNormalized();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Normalize, k_categoryName, "{1B01B185-50E0-4120-BD82-9331FC3117F9}", "returns the normalized version of Source", "Source");

        AZ_INLINE std::tuple<QuaternionType, NumberType> NormalizeWithLength(QuaternionType source)
        {
            const float length = source.NormalizeWithLength();
            return std::make_tuple(source, length);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(NormalizeWithLength, k_categoryName, "{E1A7F3F8-854E-4BA1-9DEA-7507BEC6D369}", "returns the normalized version of Source, and the length of Source", "Source", "Normalized", "Length");

        AZ_INLINE QuaternionType RotationXDegrees(NumberType degrees)
        {
            return QuaternionType::CreateRotationX(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RotationXDegrees, k_categoryName, "{9A017348-F803-43D7-A2A6-BE01359D5E15}", "creates a rotation of Degrees around the x-axis", "Degrees");

        AZ_INLINE QuaternionType RotationYDegrees(NumberType degrees)
        {
            return QuaternionType::CreateRotationY(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RotationYDegrees, k_categoryName, "{6C69AA65-1A83-4C36-B010-ECB621790A6C}", "creates a rotation of Degrees around the y-axis", "Degrees");

        AZ_INLINE QuaternionType RotationZDegrees(NumberType degrees)
        {
            return QuaternionType::CreateRotationZ(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RotationZDegrees, k_categoryName, "{8BC8B0FE-51A1-4ECC-AFF1-A828A0FC8F8F}", "creates a rotation of Degrees around the z-axis", "Degrees");

        AZ_INLINE QuaternionType ShortestArc(Vector3Type from, Vector3Type to)
        {
            return QuaternionType::CreateShortestArc(from, to);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ShortestArc, k_categoryName, "{00CB739A-6BF9-4160-83F7-A243BD9D5093}", "creates a rotation representing the shortest arc between From and To", "From", "To");

        AZ_INLINE QuaternionType Slerp(QuaternionType a, QuaternionType b, NumberType t)
        {
            return a.Slerp(b, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Slerp, k_categoryName, "{26234D44-9BA5-4E1B-8226-224E8A4A15CC}", "returns the spherical linear interpolation between From and To by the amount T, the result is NOT normalized", "From", "To", "T");

        AZ_INLINE QuaternionType Squad(QuaternionType from, QuaternionType to, QuaternionType in, QuaternionType out, NumberType t)
        {
            return from.Squad(to, in, out, aznumeric_cast<float>(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Squad, k_categoryName, "{D354F41E-29E3-49EC-8F0E-C890000D32D6}", "returns the quadratic interpolation, that is: Squad(From, To, In, Out, T) = Slerp(Slerp(From, Out, T), Slerp(To, In, T), 2(1 - T)T)", "From", "To", "In", "Out", "T");

        AZ_INLINE QuaternionType Subtract(QuaternionType a, QuaternionType b)
        {
            return a - b;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(Subtract, k_categoryName, "{238538F8-D8C9-4348-89CC-E35F5DF11358}", "This node is deprecated, use Subtract (-), it provides contextual type and slots", "A", "B");

        AZ_INLINE NumberType ToAngleDegrees(QuaternionType source)
        {
            return aznumeric_caster(AZ::RadToDeg(source.GetAngle()));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ToAngleDegrees, k_categoryName, "{3EA78793-9AFA-4857-8CB8-CD0D47E97D25}", "returns the angle of angle-axis pair that Source represents in degrees", "Source");

        AZ_INLINE Vector3Type ToImaginary(QuaternionType source)
        {
            return source.GetImaginary();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ToImaginary, k_categoryName, "{86754CA3-ADBA-4D5C-AAB6-C4AA6B079CFD}", "returns the imaginary portion of Source, that is (x, y, z)", "Source");

        AZ_INLINE QuaternionType CreateFromEulerAngles(NumberType pitch, NumberType roll, NumberType yaw)
        {
            AZ::Vector3 eulerDegress = AZ::Vector3(static_cast<float>(pitch), static_cast<float>(roll), static_cast<float>(yaw));
            return AZ::ConvertEulerDegreesToQuaternion(eulerDegress);
        }

        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(CreateFromEulerAngles, k_categoryName, "{33974124-2882-499D-9FBE-A37EB687B30C}", "Returns a new Quaternion initialized with the specified Angles", "Pitch", "Roll", "Yaw");

        AZ_INLINE Vector3Type RotateVector3(QuaternionType source, Vector3Type vector3)
        {
            return source.TransformVector(vector3);
        }

        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RotateVector3, k_categoryName, "{DDF7C05C-7148-4860-93A3-D507C5896B6C}", "Returns a new Vector3 that is the source vector3 rotated by the given Quaternion", "Quaternion", "Vector");

        using Registrar = RegistrarGeneric <
            AddNode
            , ConjugateNode
            , ConvertTransformToRotationNode
            , DivideByNumberNode
            , DotNode
            , FromAxisAngleDegreesNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , FromElementNode
            , FromElementsNode
#endif

            , FromMatrix3x3Node
            , FromMatrix4x4Node
            , FromTransformNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , FromVector3Node
            , FromVector3AndValueNode
            , GetElementNode
            , GetElementsNode
#endif

            , InvertFullNode
            , IsCloseNode
            , IsFiniteNode
            , IsIdentityNode
            , IsZeroNode
            , LengthNode
            , LengthReciprocalNode
            , LengthSquaredNode
            , LerpNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , ModXNode
            , ModYNode
            , ModZNode
            , ModWNode
#endif

            , MultiplyByNumberNode
            , MultiplyByRotationNode
            , NegateNode
            , NormalizeNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , NormalizeWithLengthNode
#endif

            , RotationXDegreesNode
            , RotationYDegreesNode
            , RotationZDegreesNode
            , ShortestArcNode
            , SlerpNode
            , SquadNode
            , SubtractNode
            , ToAngleDegreesNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , ToImaginaryNode
#endif

            , CreateFromEulerAnglesNode
            , RotateVector3Node
        > ;

    }
}

