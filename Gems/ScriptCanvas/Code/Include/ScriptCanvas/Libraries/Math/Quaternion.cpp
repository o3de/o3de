/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Quaternion.h"

#include <Include/ScriptCanvas/Libraries/Math/Quaternion.generated.h>

namespace ScriptCanvas
{
    namespace QuaternionFunctions
    {
        using namespace Data;

        QuaternionType Conjugate(QuaternionType source)
        {
            return source.GetConjugate();
        }

        QuaternionType ConvertTransformToRotation(const TransformType& transform)
        {
            return AZ::ConvertEulerRadiansToQuaternion(AZ::ConvertTransformToEulerRadians(transform));
        }

        NumberType Dot(QuaternionType a, QuaternionType b)
        {
            return a.Dot(b);
        }

        QuaternionType FromAxisAngleDegrees(Vector3Type axis, NumberType degrees)
        {
            return QuaternionType::CreateFromAxisAngle(axis, AZ::DegToRad(aznumeric_caster(degrees)));
        }

        QuaternionType FromMatrix3x3(const Matrix3x3Type& source)
        {
            return QuaternionType::CreateFromMatrix3x3(source);
        }

        QuaternionType FromMatrix4x4(const Matrix4x4Type& source)
        {
            return QuaternionType::CreateFromMatrix4x4(source);
        }

        QuaternionType FromTransform(const TransformType& source)
        {
            return source.GetRotation();
        }

        QuaternionType InvertFull(QuaternionType source)
        {
            return source.GetInverseFull();
        }

        BooleanType IsClose(QuaternionType a, QuaternionType b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }

        BooleanType IsFinite(QuaternionType a)
        {
            return a.IsFinite();
        }

        BooleanType IsIdentity(QuaternionType source, NumberType tolerance)
        {
            return source.IsIdentity(aznumeric_cast<float>(tolerance));
        }

        BooleanType IsZero(QuaternionType source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }

        NumberType LengthReciprocal(QuaternionType source)
        {
            return source.GetLengthReciprocal();
        }

        NumberType LengthSquared(QuaternionType source)
        {
            return source.GetLengthSq();
        }

        QuaternionType Lerp(QuaternionType a, QuaternionType b, NumberType t)
        {
            return a.Lerp(b, aznumeric_cast<float>(t));
        }

        QuaternionType MultiplyByNumber(QuaternionType source, NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }

        QuaternionType Negate(QuaternionType source)
        {
            return -source;
        }

        QuaternionType Normalize(QuaternionType source)
        {
            return source.GetNormalized();
        }

        QuaternionType RotationXDegrees(NumberType degrees)
        {
            return QuaternionType::CreateRotationX(AZ::DegToRad(aznumeric_caster(degrees)));
        }

        QuaternionType RotationYDegrees(NumberType degrees)
        {
            return QuaternionType::CreateRotationY(AZ::DegToRad(aznumeric_caster(degrees)));
        }

        QuaternionType RotationZDegrees(NumberType degrees)
        {
            return QuaternionType::CreateRotationZ(AZ::DegToRad(aznumeric_caster(degrees)));
        }

        QuaternionType ShortestArc(Vector3Type from, Vector3Type to)
        {
            return QuaternionType::CreateShortestArc(from, to);
        }

        QuaternionType Slerp(QuaternionType a, QuaternionType b, NumberType t)
        {
            return a.Slerp(b, aznumeric_cast<float>(t));
        }

        QuaternionType Squad(QuaternionType from, QuaternionType to, QuaternionType in, QuaternionType out, NumberType t)
        {
            return from.Squad(to, in, out, aznumeric_cast<float>(t));
        }

        NumberType ToAngleDegrees(QuaternionType source)
        {
            return aznumeric_caster(AZ::RadToDeg(source.GetAngle()));
        }

        QuaternionType CreateFromEulerAngles(NumberType pitch, NumberType roll, NumberType yaw)
        {
            AZ::Vector3 eulerDegress = AZ::Vector3(static_cast<float>(pitch), static_cast<float>(roll), static_cast<float>(yaw));
            return AZ::ConvertEulerDegreesToQuaternion(eulerDegress);
        }

        QuaternionType CreateFromValues(NumberType x, NumberType y, NumberType z, NumberType w)
        {
            return QuaternionType(aznumeric_cast<float>(x), aznumeric_cast<float>(y), aznumeric_cast<float>(z), aznumeric_cast<float>(w));
        }

        Vector3Type RotateVector3(QuaternionType source, Vector3Type vector3)
        {
            return source.TransformVector(vector3);
        }
    } // namespace QuaternionFunctions
} // namespace ScriptCanvas
