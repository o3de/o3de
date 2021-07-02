/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <GradientSignal/Util.h>


namespace GradientSignal
{
    // To keep things behaving consistently between clamped and unbounded uv ranges, we
    // we want our clamped uvs to use a range of [min, max), so we'll actually clamp to
    // [min, max - epsilon]. Since our floating-point numbers are likely in the 
    // -16384 to 16384 range, an epsilon of 0.001 will work without rounding to 0.  
    static const float uvEpsilon = 0.001f;

    AZ::Vector3 GetUnboundedPointInAabb(const AZ::Vector3& point, const AZ::Aabb& /*bounds*/)
    {
        return point;
    }

    AZ::Vector3 GetClampedPointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds)
    {
        // We want the clamped sampling states to clamp uvs to the [min, max) range.
        return AZ::Vector3(
            AZ::GetClamp(point.GetX(), bounds.GetMin().GetX(), bounds.GetMax().GetX() - uvEpsilon),
            AZ::GetClamp(point.GetY(), bounds.GetMin().GetY(), bounds.GetMax().GetY() - uvEpsilon),
            AZ::GetClamp(point.GetZ(), bounds.GetMin().GetZ(), bounds.GetMax().GetZ() - uvEpsilon));
    }

    float GetMirror(float value, float min, float max)
    {
        float relativeValue = value - min;
        float range = max - min;
        float rangeX2 = range * 2.0f;
        if (relativeValue < 0.0)
        {
            relativeValue = rangeX2 - fmod(-relativeValue, rangeX2);
        }
        else
        {
            relativeValue = fmod(relativeValue, rangeX2);
        }
        if (relativeValue >= range)
        {
            // Since we want our uv range to stay in the [min, max) range, 
            // it means that for mirroring, we want both the "forward" values 
            // and the "mirrored" values to be in [0, range).  We don't want 
            // relativeValue == range, so we shift relativeValue by a small epsilon
            // in the mirrored case.
            relativeValue = rangeX2 - (relativeValue + uvEpsilon);
        }

        return relativeValue + min;
    }

    AZ::Vector3 GetMirroredPointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds)
    {
        return AZ::Vector3(
            GetMirror(point.GetX(), bounds.GetMin().GetX(), bounds.GetMax().GetX()),
            GetMirror(point.GetY(), bounds.GetMin().GetY(), bounds.GetMax().GetY()),
            GetMirror(point.GetZ(), bounds.GetMin().GetZ(), bounds.GetMax().GetZ()));
    }

    AZ::Vector3 GetRelativePointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds)
    {
        return point - bounds.GetMin();
    }
}
