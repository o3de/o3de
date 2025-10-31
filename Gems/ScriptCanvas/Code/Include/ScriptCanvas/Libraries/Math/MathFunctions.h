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
    namespace MathFunctions
    {
        Data::NumberType MultiplyAndAdd(Data::NumberType multiplicand, Data::NumberType multiplier, Data::NumberType addend);
        Data::NumberType StringToNumber(const Data::StringType& stringValue);
    } // namespace MathFunctions

    namespace MathRandoms
    {
        Data::ColorType RandomColor(Data::ColorType minValue, Data::ColorType maxValue);
        Data::ColorType RandomGrayscale(Data::NumberType minValue, Data::NumberType maxValue);
        Data::NumberType RandomInteger(Data::NumberType minValue, Data::NumberType maxValue);
        Data::NumberType RandomNumber(Data::NumberType minValue, Data::NumberType maxValue);
        Data::Vector3Type RandomPointInBox(Data::Vector3Type dimensions);
        Data::Vector3Type RandomPointOnCircle(Data::NumberType radius);
        Data::Vector3Type RandomPointInCone(Data::NumberType radius, Data::NumberType angleInDegrees);
        Data::Vector3Type RandomPointInCylinder(Data::NumberType radius, Data::NumberType height);
        Data::Vector3Type RandomPointInCircle(Data::NumberType radius);
        Data::Vector3Type RandomPointInEllipsoid(Data::Vector3Type dimensions);
        Data::Vector3Type RandomPointInSphere(Data::NumberType radius);
        Data::Vector3Type RandomPointInSquare(Data::Vector2Type dimensions);
        Data::Vector3Type RandomPointOnSphere(Data::NumberType radius);
        Data::QuaternionType RandomQuaternion(Data::NumberType minValue, Data::NumberType maxValue);
        Data::Vector2Type RandomUnitVector2();
        Data::Vector3Type RandomUnitVector3();
        Data::Vector2Type RandomVector2(Data::Vector2Type minValue, Data::Vector2Type maxValue);
        Data::Vector3Type RandomVector3(Data::Vector3Type minValue, Data::Vector3Type maxValue);
        Data::Vector4Type RandomVector4(Data::Vector4Type minValue, Data::Vector4Type maxValue);
        Data::Vector3Type RandomPointInArc(
            Data::Vector3Type origin,
            Data::Vector3Type direction,
            Data::Vector3Type normal,
            Data::NumberType length,
            Data::NumberType angle);
        Data::Vector3Type RandomPointInWedge(
            Data::Vector3Type origin,
            Data::Vector3Type direction,
            Data::Vector3Type normal,
            Data::NumberType length,
            Data::NumberType height,
            Data::NumberType angle);
    } // namespace MathRandoms
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Math/MathFunctions.generated.h>
