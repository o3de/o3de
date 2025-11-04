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
    namespace Vector4Functions
    {
        using namespace Data;

        Vector4Type Absolute(const Vector4Type source);

        NumberType Dot(const Vector4Type lhs, const Vector4Type rhs);

        Vector4Type FromValues(NumberType x, NumberType y, NumberType z, NumberType w);

        NumberType GetElement(const Vector4Type source, const NumberType index);

        BooleanType IsClose(const Vector4Type a, const Vector4Type b, NumberType tolerance);

        BooleanType IsFinite(const Vector4Type source);

        BooleanType IsNormalized(const Vector4Type source, NumberType tolerance);

        BooleanType IsZero(const Vector4Type source, NumberType tolerance);

        NumberType Length(const Vector4Type source);

        NumberType LengthReciprocal(const Vector4Type source);

        NumberType LengthSquared(const Vector4Type source);

        Vector4Type SetW(Vector4Type source, NumberType value);

        Vector4Type SetX(Vector4Type source, NumberType value);

        Vector4Type SetY(Vector4Type source, NumberType value);

        Vector4Type SetZ(Vector4Type source, NumberType value);

        Vector4Type MultiplyByNumber(const Vector4Type source, const NumberType multiplier);

        Vector4Type Negate(const Vector4Type source);

        Vector4Type Normalize(const Vector4Type source);

        Vector4Type Reciprocal(const Vector4Type source);

        AZStd::tuple<Vector4Type, NumberType> DirectionTo(const Vector4Type from, const Vector4Type to, NumberType optionalScale = 1.f);
    } // namespace Vector4Functions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Math/Vector4.generated.h>
