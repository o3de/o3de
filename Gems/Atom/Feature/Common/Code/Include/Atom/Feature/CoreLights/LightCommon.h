/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/Hemisphere.h>
#include <AzCore/Math/Sphere.h>
#include <AzCore/std/containers/variant.h>

#include <Atom/Feature/Mesh/MeshCommon.h>

namespace AZ::RPI
{
    class Scene;
}

namespace AZ::Render::LightCommon
{
    using LightBounds = AZStd::variant<Sphere, Hemisphere, Frustum, Aabb>;

    template <typename BoundsType>
    struct EmptyFilter
    {
        constexpr bool operator()(const BoundsType&) const { return true; }
    };

    template <typename BoundsType, class Filter = EmptyFilter<BoundsType>>
    void MarkMeshesWithLightType(AZ::RPI::Scene* scene, AZStd::span<const BoundsType> bounds, AZ::RPI::Cullable::FlagType flag, Filter filter = {})
    {
        for (const BoundsType& lightBounds : bounds)
        {
            if (filter(lightBounds))
            {
                MarkMeshesForBounds(scene, lightBounds, flag);
            }
        }
    }

    template <class Filter = EmptyFilter<LightBounds>>
    void MarkMeshesWithLightType(AZ::RPI::Scene* scene, AZStd::span<const LightBounds> bounds, AZ::RPI::Cullable::FlagType flag, Filter filter = {})
    {
        for (const LightBounds& lightBounds : bounds)
        {
            if (filter(lightBounds))
            {
                if (AZStd::holds_alternative<Sphere>(lightBounds))
                {
                    MarkMeshesForBounds(scene, AZStd::get<Sphere>(lightBounds), flag);
                }
                else if(AZStd::holds_alternative<Hemisphere>(lightBounds))
                {
                    MarkMeshesForBounds(scene, AZStd::get<Hemisphere>(lightBounds), flag);
                }
                else if (AZStd::holds_alternative<Frustum>(lightBounds))
                {
                    MarkMeshesForBounds(scene, AZStd::get<Frustum>(lightBounds), flag);
                }
                else if (AZStd::holds_alternative<Aabb>(lightBounds))
                {
                    MarkMeshesForBounds(scene, AZStd::get<Aabb>(lightBounds), flag);
                }
            }
        }
    }

    inline float GetRadiusFromInvRadiusSquared(float invRadiusSqaured)
    {
        return (invRadiusSqaured <= 0.0f) ? 1.0f : sqrt(1.0f / invRadiusSqaured);
    }

} // namespace AZ::Render::LightCommon

