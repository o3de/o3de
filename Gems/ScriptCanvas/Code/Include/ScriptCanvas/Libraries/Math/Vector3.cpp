/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Vector3.h"


namespace ScriptCanvas
{
    namespace Vector3Functions
    {
        using namespace Data;

        Vector3Type Absolute(const Vector3Type source)
        {
            return source.GetAbs();
        }

        AZStd::tuple<Vector3Type, Vector3Type> BuildTangentBasis(Vector3Type source)
        {
            AZStd::tuple<Vector3Type, Vector3Type> tangentBitangent;
            source.NormalizeSafe();
            source.BuildTangentBasis(AZStd::get<0>(tangentBitangent), AZStd::get<1>(tangentBitangent));
            return tangentBitangent;
        }

        Vector3Type Clamp(const Vector3Type source, const Vector3Type min, const Vector3Type max)
        {
            return source.GetClamp(min, max);
        }

        Vector3Type Cross(const Vector3Type lhs, const Vector3Type rhs)
        {
            return lhs.Cross(rhs);
        }

        NumberType Distance(const Vector3Type a, const Vector3Type b)
        {
            return a.GetDistance(b);
        }

        NumberType DistanceSquared(const Vector3Type a, const Vector3Type b)
        {
            return a.GetDistanceSq(b);
        }

        NumberType Dot(const Vector3Type lhs, const Vector3Type rhs)
        {
            return lhs.Dot(rhs);
        }

        Vector3Type FromValues(NumberType x, NumberType y, NumberType z)
        {
            return Vector3Type(aznumeric_cast<float>(x), aznumeric_cast<float>(y), aznumeric_cast<float>(z));
        }

        NumberType GetElement(const Vector3Type source, const NumberType index)
        {
            return source.GetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 2));
        }

        BooleanType IsClose(const Vector3Type a, const Vector3Type b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }

        BooleanType IsFinite(const Vector3Type source)
        {
            return source.IsFinite();
        }

        BooleanType IsNormalized(const Vector3Type source, NumberType tolerance)
        {
            return source.IsNormalized(aznumeric_cast<float>(tolerance));
        }

        BooleanType IsPerpendicular(const Vector3Type a, const Vector3Type b, NumberType tolerance)
        {
            return a.IsPerpendicular(b, aznumeric_cast<float>(tolerance));
        }

        BooleanType IsZero(const Vector3Type source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }

        NumberType Length(const Vector3Type source)
        {
            return source.GetLength();
        }

        NumberType LengthReciprocal(const Vector3Type source)
        {
            return source.GetLengthReciprocal();
        }

        NumberType LengthSquared(const Vector3Type source)
        {
            return source.GetLengthSq();
        }

        Vector3Type Lerp(const Vector3Type& from, const Vector3Type& to, NumberType t)
        {
            return from.Lerp(to, aznumeric_cast<float>(t));
        }

        Vector3Type Max(const Vector3Type a, const Vector3Type b)
        {
            return a.GetMax(b);
        }

        Vector3Type Min(const Vector3Type a, const Vector3Type b)
        {
            return a.GetMin(b);
        }

        Vector3Type SetX(Vector3Type source, NumberType value)
        {
            source.SetX(aznumeric_cast<float>(value));
            return source;
        }

        Vector3Type SetY(Vector3Type source, NumberType value)
        {
            source.SetY(aznumeric_cast<float>(value));
            return source;
        }

        Vector3Type SetZ(Vector3Type source, NumberType value)
        {
            source.SetZ(aznumeric_cast<float>(value));
            return source;
        }

        Vector3Type MultiplyByNumber(const Vector3Type source, const NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }

        Vector3Type Negate(const Vector3Type source)
        {
            return -source;
        }

        Vector3Type Normalize(const Vector3Type source)
        {
            return source.GetNormalizedSafe();
        }

        Vector3Type Project(Vector3Type a, const Vector3Type b)
        {
            a.Project(b);
            return a;
        }

        Vector3Type Reciprocal(const Vector3Type source)
        {
            return source.GetReciprocal();
        }

        Vector3Type Slerp(const Vector3Type from, const Vector3Type to, const NumberType t)
        {
            return from.Slerp(to, aznumeric_cast<float>(t));
        }

        AZStd::tuple<Vector3Type, NumberType> DirectionTo(const Vector3Type from, const Vector3Type to, NumberType optionalScale)
        {
            Vector3Type r = to - from;
            float length = r.NormalizeWithLength();
            r.SetLength(static_cast<float>(optionalScale));
            return AZStd::make_tuple(r, length);
        }
    } // namespace Vector3Functions
} // namespace ScriptCanvas
