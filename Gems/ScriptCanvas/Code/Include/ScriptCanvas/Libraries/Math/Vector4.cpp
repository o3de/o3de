/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Vector4.h"

#include <Include/ScriptCanvas/Libraries/Math/Vector4.generated.h>

namespace ScriptCanvas
{
    namespace Vector4Functions
    {
        using namespace Data;

        Vector4Type Absolute(const Vector4Type source)
        {
            return source.GetAbs();
        }

        NumberType Dot(const Vector4Type lhs, const Vector4Type rhs)
        {
            return lhs.Dot(rhs);
        }

        Vector4Type FromValues(NumberType x, NumberType y, NumberType z, NumberType w)
        {
            return Vector4Type(aznumeric_cast<float>(x), aznumeric_cast<float>(y), aznumeric_cast<float>(z), aznumeric_cast<float>(w));
        }

        NumberType GetElement(const Vector4Type source, const NumberType index)
        {
            return source.GetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 3));
        }

        BooleanType IsClose(const Vector4Type a, const Vector4Type b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }

        BooleanType IsFinite(const Vector4Type source)
        {
            return source.IsFinite();
        }

        BooleanType IsNormalized(const Vector4Type source, NumberType tolerance)
        {
            return source.IsNormalized(aznumeric_cast<float>(tolerance));
        }

        BooleanType IsZero(const Vector4Type source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }

        NumberType Length(const Vector4Type source)
        {
            return source.GetLength();
        }

        NumberType LengthReciprocal(const Vector4Type source)
        {
            return source.GetLengthReciprocal();
        }

        NumberType LengthSquared(const Vector4Type source)
        {
            return source.GetLengthSq();
        }

        Vector4Type SetW(Vector4Type source, NumberType value)
        {
            source.SetW(aznumeric_cast<float>(value));
            return source;
        }

        Vector4Type SetX(Vector4Type source, NumberType value)
        {
            source.SetX(aznumeric_cast<float>(value));
            return source;
        }

        Vector4Type SetY(Vector4Type source, NumberType value)
        {
            source.SetY(aznumeric_cast<float>(value));
            return source;
        }

        Vector4Type SetZ(Vector4Type source, NumberType value)
        {
            source.SetZ(aznumeric_cast<float>(value));
            return source;
        }

        Vector4Type MultiplyByNumber(const Vector4Type source, const NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }

        Vector4Type Negate(const Vector4Type source)
        {
            return -source;
        }

        Vector4Type Normalize(const Vector4Type source)
        {
            return source.GetNormalizedSafe();
        }

        Vector4Type Reciprocal(const Vector4Type source)
        {
            return source.GetReciprocal();
        }

        AZStd::tuple<Vector4Type, NumberType> DirectionTo(const Vector4Type from, const Vector4Type to, NumberType optionalScale)
        {
            Vector4Type r = to - from;
            float length = r.NormalizeWithLength();
            r.SetLength(static_cast<float>(optionalScale));
            return AZStd::make_tuple(r, length);
        }
    } // namespace Vector4Functions
} // namespace ScriptCanvas
