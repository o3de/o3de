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
    namespace Vector2Functions
    {
        using namespace Data;

        Vector2Type Absolute(const Vector2Type source);

        Vector2Type Angle(NumberType angle);

        Vector2Type Clamp(const Vector2Type source, const Vector2Type min, const Vector2Type max);

        NumberType Distance(const Vector2Type a, const Vector2Type b);

        NumberType DistanceSquared(const Vector2Type a, const Vector2Type b);

        NumberType Dot(const Vector2Type lhs, const Vector2Type rhs);

        Vector2Type FromValues(NumberType x, NumberType y);

        NumberType GetElement(const Vector2Type source, const NumberType index);

        BooleanType IsClose(const Vector2Type a, const Vector2Type b, NumberType tolerance);

        BooleanType IsFinite(const Vector2Type source);

        BooleanType IsNormalized(const Vector2Type source, NumberType tolerance);

        BooleanType IsZero(const Vector2Type source, NumberType tolerance);

        NumberType Length(const Vector2Type source);

        NumberType LengthSquared(const Vector2Type source);

        Vector2Type Lerp(const Vector2Type& from, const Vector2Type& to, NumberType t);

        Vector2Type Max(const Vector2Type a, const Vector2Type b);

        Vector2Type Min(const Vector2Type a, const Vector2Type b);

        Vector2Type SetX(Vector2Type source, NumberType value);

        Vector2Type SetY(Vector2Type source, NumberType value);

        Vector2Type MultiplyByNumber(const Vector2Type source, const NumberType multiplier);

        Vector2Type Negate(const Vector2Type source);

        Vector2Type Normalize(const Vector2Type source);

        Vector2Type Project(Vector2Type a, const Vector2Type b);

        Vector2Type Slerp(const Vector2Type from, const Vector2Type to, const NumberType t);

        Vector2Type ToPerpendicular(const Vector2Type source);

        AZStd::tuple<Vector2Type, NumberType> DirectionTo(const Vector2Type from, const Vector2Type to, NumberType optionalScale = 1.f);
    } // namespace Vector2Functions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Math/Vector2.generated.h>
