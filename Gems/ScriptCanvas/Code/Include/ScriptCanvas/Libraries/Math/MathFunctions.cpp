/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MathFunctions.h"

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

#include <Include/ScriptCanvas/Libraries/Math/MathFunctions.generated.h>

namespace ScriptCanvas
{
    namespace MathFunctions
    {
        Data::NumberType MultiplyAndAdd(Data::NumberType multiplicand, Data::NumberType multiplier, Data::NumberType addend)
        {
            // result = src0 * src1 + src2
            return multiplicand * multiplier + addend;
        }

        Data::NumberType StringToNumber(const Data::StringType& stringValue)
        {
            return AZStd::stof(stringValue);
        }
    }

    namespace MathRandoms
    {
        Data::ColorType RandomColor(Data::ColorType minValue, Data::ColorType maxValue)
        {
            return Data::ColorType(MathNodeUtilities::GetRandomReal<float>(minValue.GetR(), maxValue.GetR()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetG(), maxValue.GetG()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetB(), maxValue.GetB()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetA(), maxValue.GetA()));
        }

        Data::ColorType RandomGrayscale(Data::NumberType minValue, Data::NumberType maxValue)
        {
            float rgb = MathNodeUtilities::GetRandomReal<float>(aznumeric_cast<float>(minValue) / 255.f, aznumeric_cast<float>(maxValue) / 255.f);
            return Data::ColorType(rgb, rgb, rgb, 1.f);
        }

        Data::NumberType RandomInteger(Data::NumberType minValue, Data::NumberType maxValue)
        {
            return MathNodeUtilities::GetRandomIntegral<int>(aznumeric_cast<int>(minValue), aznumeric_cast<int>(maxValue));
        }

        Data::NumberType RandomNumber(Data::NumberType minValue, Data::NumberType maxValue)
        {
            return MathNodeUtilities::GetRandomReal<Data::NumberType>(minValue, maxValue);
        }

        Data::Vector3Type RandomPointInBox(Data::Vector3Type dimensions)
        {
            Data::Vector3Type halfDimensions = 0.5f * dimensions;
            Data::Vector3Type point(MathNodeUtilities::GetRandomReal<float>(-halfDimensions.GetX(), halfDimensions.GetX()),
                MathNodeUtilities::GetRandomReal<float>(-halfDimensions.GetY(), halfDimensions.GetY()),
                MathNodeUtilities::GetRandomReal<float>(-halfDimensions.GetZ(), halfDimensions.GetZ()));
            return point;
        }

        Data::Vector3Type RandomPointOnCircle(Data::NumberType radius)
        {
            float fRadius = aznumeric_cast<float>(radius);
            float theta = MathNodeUtilities::GetRandomReal<float>(0.f, AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon());

            Data::Vector3Type point(fRadius * cosf(theta),
                fRadius * sinf(theta),
                0.f);
            return point;
        }

        Data::Vector3Type RandomPointInCone(Data::NumberType radius, Data::NumberType angleInDegrees)
        {
            float x, y, z;

            // Pick a random unit vector on the cone.
            {
                float halfAngleInRad = 0.5f * AZ::DegToRad(aznumeric_cast<float>(angleInDegrees));

                float theta = MathNodeUtilities::GetRandomReal<float>(0.f, (AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon())); //!< Range: [0, 2PI)
                z = cosf(MathNodeUtilities::GetRandomReal<float>(0.f, halfAngleInRad));
                float zz = sqrtf(1 - (z * z));
                x = zz * cosf(theta);
                y = zz * sinf(theta);
            }

            Data::Vector3Type normal(x, y, z);

            return normal * aznumeric_cast<float>(radius) * powf(MathNodeUtilities::GetRandomReal<float>(0.f, 1.f), 1.0f / 3.0f);
        }

        Data::Vector3Type RandomPointInCylinder(Data::NumberType radius, Data::NumberType height)
        {
            float fHeight = aznumeric_cast<float>(height);
            float fHalfHeight = fHeight * 0.5f;
            float fNegHalfHeight = fHeight * -0.5f;

            float r = aznumeric_cast<float>(radius) * sqrtf(MathNodeUtilities::GetRandomReal<float>(0.f, 1.f));
            float theta = MathNodeUtilities::GetRandomReal<float>(0.f, AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon());

            Data::Vector3Type point(r * cosf(theta),
                r * sinf(theta),
                MathNodeUtilities::GetRandomReal<float>(fNegHalfHeight, fHalfHeight));
            return point;
        }
 
        Data::Vector3Type RandomPointInCircle(Data::NumberType radius)
        {
            float r = aznumeric_cast<float>(radius) * sqrtf(MathNodeUtilities::GetRandomReal<float>(0.f, 1.f));
            float theta = MathNodeUtilities::GetRandomReal<float>(0.f, AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon());

            Data::Vector3Type point(r * cosf(theta),
                r * sinf(theta),
                0.f);
            return point;
        }

        Data::Vector3Type RandomPointInEllipsoid(Data::Vector3Type dimensions)
        {
            float x, y, z;

            // Pick a random unit vector on a sphere.
            {
                float theta = MathNodeUtilities::GetRandomReal<float>(0.f, (AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon())); //!< Range: [0, 2PI)
                z = MathNodeUtilities::GetRandomReal<float>(-1.f, 1.f);
                float zz = sqrtf(1 - (z * z));
                x = zz * cosf(theta);
                y = zz * sinf(theta);
            }

            Data::Vector3Type normal(x, y, z);

            return dimensions * normal * powf(MathNodeUtilities::GetRandomReal<float>(0.f, 1.f), 1.0f / 3.0f);
        }

        Data::Vector3Type RandomPointInSphere(Data::NumberType radius)
        {
            float x, y, z;

            // Pick a random unit vector on a sphere.
            {
                float theta = MathNodeUtilities::GetRandomReal<float>(0.f, (AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon())); //!< Range: [0, 2PI)
                z = MathNodeUtilities::GetRandomReal<float>(-1.f, 1.f);
                float zz = sqrtf(1 - (z * z));
                x = zz * cosf(theta);
                y = zz * sinf(theta);
            }

            Data::Vector3Type normal(x, y, z);

            return normal * aznumeric_cast<float>(radius) * powf(MathNodeUtilities::GetRandomReal<float>(0.f, 1.f), 1.0f / 3.0f);
        }

        Data::Vector3Type RandomPointInSquare(Data::Vector2Type dimensions)
        {
            Data::Vector2Type halfDimensions = 0.5f * dimensions;
            Data::Vector3Type point(MathNodeUtilities::GetRandomReal<float>(-halfDimensions.GetX(), halfDimensions.GetX()),
                MathNodeUtilities::GetRandomReal<float>(-halfDimensions.GetY(), halfDimensions.GetY()),
                0.f);
            return point;
        }

        Data::Vector3Type RandomPointOnSphere(Data::NumberType radius)
        {
            float x, y, z;

            // Pick a random unit vector on a sphere.
            {
                float theta = MathNodeUtilities::GetRandomReal<float>(0.f, (AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon())); //!< Range: [0, 2PI)
                z = MathNodeUtilities::GetRandomReal<float>(-1.f, 1.f);
                float zz = sqrtf(1 - (z * z));
                x = zz * cosf(theta);
                y = zz * sinf(theta);
            }

            float fRadius = aznumeric_cast<float>(radius);

            return Data::Vector3Type(fRadius * x, fRadius * y, fRadius * z);
        }

        Data::QuaternionType RandomQuaternion(Data::NumberType minValue, Data::NumberType maxValue)
        {
            float x, y, z;

            // Pick a random unit vector on a sphere.
            {
                float theta = MathNodeUtilities::GetRandomReal<float>(0.f, (AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon())); //!< Range: [0, 2PI)
                z = MathNodeUtilities::GetRandomReal<float>(-1.f, 1.f);
                float zz = sqrtf(1 - (z * z));
                x = zz * cosf(theta);
                y = zz * sinf(theta);
            }

            // Pick a random rotation.
            float theta = MathNodeUtilities::GetRandomReal<float>(aznumeric_cast<float>(minValue), aznumeric_cast<float>(maxValue) - std::numeric_limits<float>::epsilon()); //!< Default range: [0, 2PI)
            return AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(x, y, z), theta);
        }

        Data::Vector2Type RandomUnitVector2()
        {
            float theta = MathNodeUtilities::GetRandomReal<float>(0.f, AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon());
            return Data::Vector2Type(cosf(theta),
                sinf(theta));
        }

        Data::Vector3Type RandomUnitVector3()
        {
            float x, y, z;

            z = MathNodeUtilities::GetRandomReal<float>(-1.f, 1.f);
            float zz = sqrtf(1 - (z * z));

            float theta = MathNodeUtilities::GetRandomReal<float>(0.f, AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon());
            x = zz * cosf(theta);
            y = zz * sinf(theta);

            return Data::Vector3Type(x, y, z);
        }

        Data::Vector2Type RandomVector2(Data::Vector2Type minValue, Data::Vector2Type maxValue)
        {
            return Data::Vector2Type(MathNodeUtilities::GetRandomReal<float>(minValue.GetX(), maxValue.GetX()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetY(), maxValue.GetY()));
        }

        Data::Vector3Type RandomVector3(Data::Vector3Type minValue, Data::Vector3Type maxValue)
        {
            return Data::Vector3Type(MathNodeUtilities::GetRandomReal<float>(minValue.GetX(), maxValue.GetX()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetY(), maxValue.GetY()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetZ(), maxValue.GetZ()));
        }

        Data::Vector4Type RandomVector4(Data::Vector4Type minValue, Data::Vector4Type maxValue)
        {
            return Data::Vector4Type(MathNodeUtilities::GetRandomReal<float>(minValue.GetX(), maxValue.GetX()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetY(), maxValue.GetY()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetZ(), maxValue.GetZ()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetW(), maxValue.GetW()));
        }

        Data::Vector3Type RandomPointInArc(Data::Vector3Type origin, Data::Vector3Type direction, Data::Vector3Type normal, Data::NumberType length, Data::NumberType angle)
        {
            float randomAngle = MathNodeUtilities::GetRandomReal<float>(0, aznumeric_cast<float>(angle));
            randomAngle -= aznumeric_cast<float>(angle) * 0.5f;

            Data::QuaternionType rotation = Data::QuaternionType::CreateFromAxisAngle(normal, AZ::DegToRad(randomAngle));

            Data::Vector3Type rotatedDirection = rotation.TransformVector(direction);
            rotatedDirection.Normalize();

            float randomLength = MathNodeUtilities::GetRandomReal<float>(0, aznumeric_cast<float>(length));
            rotatedDirection = randomLength * rotatedDirection;

            return rotatedDirection + origin;
        }

        Data::Vector3Type RandomPointInWedge(Data::Vector3Type origin, Data::Vector3Type direction, Data::Vector3Type normal, Data::NumberType length, Data::NumberType height, Data::NumberType angle)
        {
            Data::Vector3Type randomPointInArc = RandomPointInArc(origin, direction, normal, length, angle);

            float randomHeight = MathNodeUtilities::GetRandomReal<float>(0, aznumeric_cast<float>(height));

            return randomPointInArc + (randomHeight * normal);
        }
    }
}

