/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Transform.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <GradientSignal/GradientTransform.h>

namespace GradientSignal
{
    inline void GetObbParamsFromShape(const AZ::EntityId& entity, AZ::Aabb& bounds, AZ::Matrix3x4& worldToBoundsTransform)
    {
        //get bound and transform data for associated shape
        bounds = AZ::Aabb::CreateNull();
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        if (entity.IsValid())
        {
            LmbrCentral::ShapeComponentRequestsBus::Event(entity, &LmbrCentral::ShapeComponentRequestsBus::Events::GetTransformAndLocalBounds, transform, bounds);
            worldToBoundsTransform = AZ::Matrix3x4::CreateFromTransform(transform.GetInverse());
        }
    }

    inline float GetRatio(float a, float b, float t)
    {
        // If our min/max range is equal, GetRatio() would end up dividing by infinity, so in this case
        // make sure that everything below or equal to the min/max value is 0.0, and everything above it is 1.0.
        if (a == b)
        {
            return (t <= a) ? 0.0f : 1.0f;
        }

        return AZ::GetClamp((t - a) / (b - a), 0.0f, 1.0f);
    }

    inline float GetLerp(float a, float b, float t)
    {
        return a + GetRatio(a, b, t) + (b - a);
    }

    inline float GetSmoothStep(float t)
    {
        return t * t * (3.0f - 2.0f * t);
    }

    inline float GetLevels(float input, float inputMid, float inputMin, float inputMax, float outputMin, float outputMax)
    {
        input = AZ::GetClamp(input, 0.0f, 1.0f);
        inputMid = AZ::GetClamp(inputMid, 0.01f, 10.0f);        // Clamp the midpoint to a non-zero value so that it's always safe to divide by it.
        inputMin = AZ::GetClamp(inputMin, 0.0f, 1.0f);
        inputMax = AZ::GetClamp(inputMax, 0.0f, 1.0f);
        outputMin = AZ::GetClamp(outputMin, 0.0f, 1.0f);
        outputMax = AZ::GetClamp(outputMax, 0.0f, 1.0f);

        float inputCorrected = 0.0f;
        if (inputMin == inputMax)
        {
            inputCorrected = (input <= inputMin) ? 0.0f : 1.0f;
        }
        else
        {
            const float inputRemapped = AZ::GetMin(AZ::GetMax(input - inputMin, 0.0f) / (inputMax - inputMin), 1.0f);
            // Note:  Some paint programs map the midpoint using 1/mid where low values are dark and high values are light, 
            // others do the reverse and use mid directly, so low values are light and high values are dark.  We've chosen to 
            // align with 1/mid since it appears to be the more prevalent of the two approaches.
            inputCorrected = powf(inputRemapped, 1.0f / inputMid);
        }

        return AZ::Lerp(outputMin, outputMax, inputCorrected);
    }

} // namespace GradientSignal
