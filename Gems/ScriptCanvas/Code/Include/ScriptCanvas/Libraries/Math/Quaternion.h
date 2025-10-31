/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvas
{
    namespace QuaternionFunctions
    {
        using namespace Data;

        QuaternionType Conjugate(QuaternionType source);

        QuaternionType ConvertTransformToRotation(const TransformType& transform);

        NumberType Dot(QuaternionType a, QuaternionType b);

        QuaternionType FromAxisAngleDegrees(Vector3Type axis, NumberType degrees);

        QuaternionType FromMatrix3x3(const Matrix3x3Type& source);

        QuaternionType FromMatrix4x4(const Matrix4x4Type& source);

        QuaternionType FromTransform(const TransformType& source);

        QuaternionType InvertFull(QuaternionType source);

        BooleanType IsClose(QuaternionType a, QuaternionType b, NumberType tolerance);

        BooleanType IsFinite(QuaternionType a);

        BooleanType IsIdentity(QuaternionType source, NumberType tolerance);

        BooleanType IsZero(QuaternionType source, NumberType tolerance);

        NumberType LengthReciprocal(QuaternionType source);

        NumberType LengthSquared(QuaternionType source);

        QuaternionType Lerp(QuaternionType a, QuaternionType b, NumberType t);

        QuaternionType MultiplyByNumber(QuaternionType source, NumberType multiplier);

        QuaternionType Negate(QuaternionType source);

        QuaternionType Normalize(QuaternionType source);

        QuaternionType RotationXDegrees(NumberType degrees);

        QuaternionType RotationYDegrees(NumberType degrees);

        QuaternionType RotationZDegrees(NumberType degrees);

        QuaternionType ShortestArc(Vector3Type from, Vector3Type to);

        QuaternionType Slerp(QuaternionType a, QuaternionType b, NumberType t);

        QuaternionType Squad(QuaternionType from, QuaternionType to, QuaternionType in, QuaternionType out, NumberType t);

        NumberType ToAngleDegrees(QuaternionType source);

        QuaternionType CreateFromEulerAngles(NumberType pitch, NumberType roll, NumberType yaw);

        QuaternionType CreateFromValues(NumberType x, NumberType y, NumberType z, NumberType w);

        Vector3Type RotateVector3(QuaternionType source, Vector3Type vector3);
    } // namespace QuaternionFunctions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Math/Quaternion.generated.h>
