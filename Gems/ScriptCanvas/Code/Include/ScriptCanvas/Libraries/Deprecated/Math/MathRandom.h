/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace ScriptCanvas
{
    //! RandomNodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace RandomNodes
    {
        static constexpr const char* k_categoryName = "Math/Random";

        AZ_INLINE Data::ColorType RandomColor(Data::ColorType minValue, Data::ColorType maxValue)
        {
            return Data::ColorType(MathNodeUtilities::GetRandomReal<float>(minValue.GetR(), maxValue.GetR()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetG(), maxValue.GetG()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetB(), maxValue.GetB()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetA(), maxValue.GetA()));
        }

        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomColor, k_categoryName, "{0A984F40-322B-44A6-8753-6D2056A96659}", "ScriptCanvas_MathRandoms_RandomColor");

        AZ_INLINE Data::ColorType RandomGrayscale(Data::NumberType minValue, Data::NumberType maxValue)
        {
            float rgb = MathNodeUtilities::GetRandomReal<float>(aznumeric_cast<float>(minValue) / 255.f, aznumeric_cast<float>(maxValue) / 255.f);
            return Data::ColorType(rgb, rgb, rgb, 1.f);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomGrayscale, k_categoryName, "{0488EFC7-3291-483E-A087-81DE0C29B9B9}", "ScriptCanvas_MathRandoms_RandomGrayscale");

        AZ_INLINE Data::NumberType RandomInteger(Data::NumberType minValue, Data::NumberType maxValue)
        {
            return MathNodeUtilities::GetRandomIntegral<int>(aznumeric_cast<int>(minValue), aznumeric_cast<int>(maxValue));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomInteger, k_categoryName, "{7E2B8EF8-8129-4C43-9D09-C01C926B8F3E}", "ScriptCanvas_MathRandoms_RandomInteger");

        AZ_INLINE Data::NumberType RandomNumber(Data::NumberType minValue, Data::NumberType maxValue)
        {
            return MathNodeUtilities::GetRandomReal<Data::NumberType>(minValue, maxValue);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomNumber, k_categoryName, "{80C7BDFB-CBC4-481B-988E-86260F1CB24A}", "ScriptCanvas_MathRandoms_RandomNumber");

        AZ_INLINE Data::Vector3Type RandomPointInBox(Data::Vector3Type dimensions)
        {
            Data::Vector3Type halfDimensions = 0.5f * dimensions;
            Data::Vector3Type point(MathNodeUtilities::GetRandomReal<float>(-halfDimensions.GetX(), halfDimensions.GetX()),
                MathNodeUtilities::GetRandomReal<float>(-halfDimensions.GetY(), halfDimensions.GetY()),
                MathNodeUtilities::GetRandomReal<float>(-halfDimensions.GetZ(), halfDimensions.GetZ()));
            return point;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomPointInBox, k_categoryName, "{6785C5F8-2F87-4AD6-AE15-87FE5E72D142}", "ScriptCanvas_MathRandoms_RandomPointInBox");

        AZ_INLINE Data::Vector3Type RandomPointOnCircle(Data::NumberType radius)
        {
            float fRadius = aznumeric_cast<float>(radius);
            float theta = MathNodeUtilities::GetRandomReal<float>(0.f, AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon());

            Data::Vector3Type point(fRadius * cosf(theta),
                fRadius * sinf(theta),
                0.f);
            return point;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomPointOnCircle, k_categoryName, "{2F079E35-216D-42B3-AA81-C9823F732893}", "ScriptCanvas_MathRandoms_RandomPointOnCircle");

        AZ_INLINE Data::Vector3Type RandomPointInCone(Data::NumberType radius, Data::NumberType angleInDegrees)
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
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomPointInCone, k_categoryName, "{2CCD0FAA-A4C7-4CD8-AE12-B1DFF0BDDBB6}", "ScriptCanvas_MathRandoms_RandomPointInCone");

        AZ_INLINE Data::Vector3Type RandomPointInCylinder(Data::NumberType radius, Data::NumberType height)
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
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomPointInCylinder, k_categoryName, "{BD81133C-AAC0-44B0-9C9A-D06E780F4CCE}", "ScriptCanvas_MathRandoms_RandomPointInCylinder");

        AZ_INLINE Data::Vector3Type RandomPointInCircle(Data::NumberType radius)
        {
            float r = aznumeric_cast<float>(radius) * sqrtf(MathNodeUtilities::GetRandomReal<float>(0.f, 1.f));
            float theta = MathNodeUtilities::GetRandomReal<float>(0.f, AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon());

            Data::Vector3Type point(r * cosf(theta),
                r * sinf(theta),
                0.f);
            return point;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomPointInCircle, k_categoryName, "{93378981-85DD-42B9-9D2D-826BE68BBE8F}", "ScriptCanvas_MathRandoms_RandomPointInCircle");

        AZ_INLINE Data::Vector3Type RandomPointInEllipsoid(Data::Vector3Type dimensions)
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
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomPointInEllipsoid, k_categoryName, "{B12E1848-2CD0-4283-847E-761B14EDDC01}", "ScriptCanvas_MathRandoms_RandomPointInEllipsoid");

        AZ_INLINE Data::Vector3Type RandomPointInSphere(Data::NumberType radius)
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
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomPointInSphere, k_categoryName, "{ECDA9004-07B4-46DE-AEB2-381DC3736D4F}", "ScriptCanvas_MathRandoms_RandomPointInSphere");

        AZ_INLINE Data::Vector3Type RandomPointInSquare(Data::Vector2Type dimensions)
        {
            Data::Vector2Type halfDimensions = 0.5f * dimensions;
            Data::Vector3Type point(MathNodeUtilities::GetRandomReal<float>(-halfDimensions.GetX(), halfDimensions.GetX()),
                MathNodeUtilities::GetRandomReal<float>(-halfDimensions.GetY(), halfDimensions.GetY()),
                0.f);
            return point;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomPointInSquare, k_categoryName, "{B81B4049-CBD2-460E-A4AB-155AB8FFDCB9}", "ScriptCanvas_MathRandoms_RandomPointInSquare");

        AZ_INLINE Data::Vector3Type RandomPointOnSphere(Data::NumberType radius)
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
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomPointOnSphere, k_categoryName, "{D03DCCA3-2C87-4A71-ACE1-823E43DFF0CB}", "ScriptCanvas_MathRandoms_RandomPointOnSphere");

        AZ_INLINE Data::QuaternionType RandomQuaternion(Data::NumberType minValue, Data::NumberType maxValue)
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
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomQuaternion, k_categoryName, "{6C764974-4D1C-44FE-8465-706E24B9B027}", "ScriptCanvas_MathRandoms_RandomQuaternion");

        // RandomUnitVector2
        AZ_INLINE Data::Vector2Type RandomUnitVector2()
        {
            float theta = MathNodeUtilities::GetRandomReal<float>(0.f, AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon());
            return Data::Vector2Type(cosf(theta),
                sinf(theta));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomUnitVector2, k_categoryName, "{02CE950A-06F8-485D-87E9-77FDE808B160}", "ScriptCanvas_MathRandoms_RandomUnitVector2");

        // RandomUnitVector3
        AZ_INLINE Data::Vector3Type RandomUnitVector3()
        {
            float x, y, z;

            z = MathNodeUtilities::GetRandomReal<float>(-1.f, 1.f);
            float zz = sqrtf(1 - (z * z));

            float theta = MathNodeUtilities::GetRandomReal<float>(0.f, AZ::Constants::TwoPi - std::numeric_limits<float>::epsilon());
            x = zz * cosf(theta);
            y = zz * sinf(theta);

            return Data::Vector3Type(x, y, z);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomUnitVector3, k_categoryName, "{E548F1EA-51C5-462F-A76B-9C15FFBB6C41}", "ScriptCanvas_MathRandoms_RandomUnitVector3");

        AZ_INLINE Data::Vector2Type RandomVector2(Data::Vector2Type minValue, Data::Vector2Type maxValue)
        {
            return Data::Vector2Type(MathNodeUtilities::GetRandomReal<float>(minValue.GetX(), maxValue.GetX()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetY(), maxValue.GetY()));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomVector2, k_categoryName, "{6F9982F5-D6F6-4568-8A83-D5A35390D425}", "ScriptCanvas_MathRandoms_RandomVector2");

        AZ_INLINE Data::Vector3Type RandomVector3(Data::Vector3Type minValue, Data::Vector3Type maxValue)
        {
            return Data::Vector3Type(MathNodeUtilities::GetRandomReal<float>(minValue.GetX(), maxValue.GetX()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetY(), maxValue.GetY()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetZ(), maxValue.GetZ()));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomVector3, k_categoryName, "{FF5526DC-E56D-4101-B7DE-4E7283E31B10}", "ScriptCanvas_MathRandoms_RandomVector3");

        AZ_INLINE Data::Vector4Type RandomVector4(Data::Vector4Type minValue, Data::Vector4Type maxValue)
        {
            return Data::Vector4Type(MathNodeUtilities::GetRandomReal<float>(minValue.GetX(), maxValue.GetX()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetY(), maxValue.GetY()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetZ(), maxValue.GetZ()),
                MathNodeUtilities::GetRandomReal<float>(minValue.GetW(), maxValue.GetW()));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomVector4, k_categoryName, "{76FCA9CF-7BBF-471C-9D4A-67FE8E9C6298}", "ScriptCanvas_MathRandoms_RandomVector4");

        AZ_INLINE Data::Vector3Type RandomPointInArc(Data::Vector3Type origin, Data::Vector3Type direction, Data::Vector3Type normal, Data::NumberType length, Data::NumberType angle)
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

        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomPointInArc, k_categoryName, "{CD4BFC02-3214-4EB8-BD7E-60749B783D3B}", "ScriptCanvas_MathRandoms_RandomPointInArc");

        AZ_INLINE Data::Vector3Type RandomPointInWedge(Data::Vector3Type origin, Data::Vector3Type direction, Data::Vector3Type normal, Data::NumberType length, Data::NumberType height, Data::NumberType angle)
        {
            Data::Vector3Type randomPointInArc = RandomPointInArc(origin, direction, normal, length, angle);

            float randomHeight = MathNodeUtilities::GetRandomReal<float>(0, aznumeric_cast<float>(height));

            return randomPointInArc + (randomHeight * normal);
        }

        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(RandomPointInWedge, k_categoryName, "{F224DA37-240D-4ABB-A97A-3565197B94B4}", "ScriptCanvas_MathRandoms_RandomPointInWedge");

        using Registrar = RegistrarGeneric <
            RandomColorNode
            , RandomGrayscaleNode
            , RandomIntegerNode
            , RandomNumberNode
            , RandomPointInBoxNode
            , RandomPointOnCircleNode
            , RandomPointInConeNode
            , RandomPointInCylinderNode
            , RandomPointInCircleNode
            , RandomPointInEllipsoidNode
            , RandomPointInSphereNode
            , RandomPointInSquareNode
            , RandomPointOnSphereNode
            , RandomQuaternionNode
            , RandomUnitVector2Node
            , RandomUnitVector3Node
            , RandomVector2Node
            , RandomVector3Node
            , RandomVector4Node
            , RandomPointInArcNode
            , RandomPointInWedgeNode
        > ;
    }
}
