/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/tuple.h>
#include <ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvas
{
    namespace Vector3Functions
    {
        using namespace Data;

        Vector3Type Absolute(const Vector3Type source);

        AZStd::tuple<Vector3Type, Vector3Type> BuildTangentBasis(Vector3Type source);

        Vector3Type Clamp(const Vector3Type source, const Vector3Type min, const Vector3Type max);

        Vector3Type Cross(const Vector3Type lhs, const Vector3Type rhs);

        NumberType Distance(const Vector3Type a, const Vector3Type b);

        NumberType DistanceSquared(const Vector3Type a, const Vector3Type b);

        NumberType Dot(const Vector3Type lhs, const Vector3Type rhs);

        Vector3Type FromValues(NumberType x, NumberType y, NumberType z);

        NumberType GetElement(const Vector3Type source, const NumberType index);

        BooleanType IsClose(const Vector3Type a, const Vector3Type b, NumberType tolerance);

        BooleanType IsFinite(const Vector3Type source);

        BooleanType IsNormalized(const Vector3Type source, NumberType tolerance);

        BooleanType IsPerpendicular(const Vector3Type a, const Vector3Type b, NumberType tolerance);

        BooleanType IsZero(const Vector3Type source, NumberType tolerance);

        NumberType Length(const Vector3Type source);

        NumberType LengthReciprocal(const Vector3Type source);

        NumberType LengthSquared(const Vector3Type source);

        Vector3Type Lerp(const Vector3Type& from, const Vector3Type& to, NumberType t);

        Vector3Type Max(const Vector3Type a, const Vector3Type b);

        Vector3Type Min(const Vector3Type a, const Vector3Type b);

        Vector3Type SetX(Vector3Type source, NumberType value);

        Vector3Type SetY(Vector3Type source, NumberType value);

        Vector3Type SetZ(Vector3Type source, NumberType value);

        Vector3Type MultiplyByNumber(const Vector3Type source, const NumberType multiplier);

        Vector3Type Negate(const Vector3Type source);

        Vector3Type Normalize(const Vector3Type source);

        Vector3Type Project(Vector3Type a, const Vector3Type b);

        Vector3Type Reciprocal(const Vector3Type source);

        Vector3Type Slerp(const Vector3Type from, const Vector3Type to, const NumberType t);

        AZStd::tuple<Vector3Type, NumberType> DirectionTo(const Vector3Type from, const Vector3Type to, NumberType optionalScale = 1.f);
    } // namespace Vector3Functions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Math/Vector3.generated.h>
