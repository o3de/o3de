/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Vector2.h"


namespace ScriptCanvas
{
    namespace Vector2Functions
    {
        using namespace Data;

        Vector2Type Absolute(const Vector2Type source)
        {
            return source.GetAbs();
        }

        Vector2Type Angle(NumberType angle)
        {
            return Vector2Type::CreateFromAngle(aznumeric_cast<float>(angle));
        }

        Vector2Type Clamp(const Vector2Type source, const Vector2Type min, const Vector2Type max)
        {
            return source.GetClamp(min, max);
        }

        NumberType Distance(const Vector2Type a, const Vector2Type b)
        {
            return a.GetDistance(b);
        }

        NumberType DistanceSquared(const Vector2Type a, const Vector2Type b)
        {
            return a.GetDistanceSq(b);
        }

        NumberType Dot(const Vector2Type lhs, const Vector2Type rhs)
        {
            return lhs.Dot(rhs);
        }

        Vector2Type FromValues(NumberType x, NumberType y)
        {
            return Vector2Type(aznumeric_cast<float>(x), aznumeric_cast<float>(y));
        }

        NumberType GetElement(const Vector2Type source, const NumberType index)
        {
            return source.GetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 1));
        }

        BooleanType IsClose(const Vector2Type a, const Vector2Type b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }

        BooleanType IsFinite(const Vector2Type source)
        {
            return source.IsFinite();
        }

        BooleanType IsNormalized(const Vector2Type source, NumberType tolerance)
        {
            return source.IsNormalized(aznumeric_cast<float>(tolerance));
        }

        BooleanType IsZero(const Vector2Type source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }

        NumberType Length(const Vector2Type source)
        {
            return source.GetLength();
        }

        NumberType LengthSquared(const Vector2Type source)
        {
            return source.GetLengthSq();
        }

        Vector2Type Lerp(const Vector2Type& from, const Vector2Type& to, NumberType t)
        {
            return from.Lerp(to, aznumeric_cast<float>(t));
        }

        Vector2Type Max(const Vector2Type a, const Vector2Type b)
        {
            return a.GetMax(b);
        }

        Vector2Type Min(const Vector2Type a, const Vector2Type b)
        {
            return a.GetMin(b);
        }

        Vector2Type SetX(Vector2Type source, NumberType value)
        {
            source.SetX(aznumeric_caster(value));
            return source;
        }

        Vector2Type SetY(Vector2Type source, NumberType value)
        {
            source.SetY(aznumeric_caster(value));
            return source;
        }

        Vector2Type MultiplyByNumber(const Vector2Type source, const NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }

        Vector2Type Negate(const Vector2Type source)
        {
            return -source;
        }

        Vector2Type Normalize(const Vector2Type source)
        {
            return source.GetNormalizedSafe();
        }

        Vector2Type Project(Vector2Type a, const Vector2Type b)
        {
            a.Project(b);
            return a;
        }

        Vector2Type Slerp(const Vector2Type from, const Vector2Type to, const NumberType t)
        {
            return from.Slerp(to, aznumeric_cast<float>(t));
        }

        Vector2Type ToPerpendicular(const Vector2Type source)
        {
            return source.GetPerpendicular();
        }

        AZStd::tuple<Vector2Type, NumberType> DirectionTo(const Vector2Type from, const Vector2Type to, NumberType optionalScale)
        {
            Vector2Type r = to - from;
            float length = r.NormalizeWithLength();
            r.SetLength(static_cast<float>(optionalScale));
            return AZStd::make_tuple(r, length);
        }
    } // namespace Vector2Functions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Math/Vector2.generated.h>
