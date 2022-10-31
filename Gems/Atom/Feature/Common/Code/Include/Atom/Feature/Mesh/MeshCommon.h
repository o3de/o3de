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
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/Math/Sphere.h>
#include <AzCore/std/containers/variant.h>

#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ::Render::MeshCommon
{
    template <typename BoundsType>
    void MarkMeshesForBounds(AZ::RPI::Scene* scene, const BoundsType& bounds, AZ::RPI::Cullable::FlagType flag)
    {
        AzFramework::IVisibilityScene* visScene = scene->GetVisibilityScene();

        visScene->Enumerate(bounds, [flag, &boundsRef = bounds](const AzFramework::IVisibilityScene::NodeData& nodeData)
            {
                bool nodeIsContainedInBounds = ShapeIntersection::Contains(boundsRef, nodeData.m_bounds);
                for (auto* visibleEntry : nodeData.m_entries)
                {
                    if (visibleEntry->m_typeFlags == AzFramework::VisibilityEntry::TYPE_RPI_Cullable)
                    {
                        RPI::Cullable* cullable = static_cast<RPI::Cullable*>(visibleEntry->m_userData);

                        if (nodeIsContainedInBounds || ShapeIntersection::Overlaps(boundsRef, cullable->m_cullData.m_boundingSphere))
                        {
                            // This flag is cleared by the mesh feature processor each frame in OnEndPrepareRender()
                            cullable->m_flags.fetch_or(flag);
                        }
                    }
                }
            }
        );
    }

    using BoundsVariant = AZStd::variant<Sphere, Hemisphere, Frustum, Aabb, Capsule>;

    template <typename BoundsType>
    struct EmptyFilter
    {
        constexpr bool operator()(const BoundsType&) const { return true; }
    };

    template <typename BoundsType, class Filter = EmptyFilter<BoundsType>>
    void MarkMeshesWithFlag(AZ::RPI::Scene* scene, AZStd::span<const BoundsType> boundsCollection, AZ::RPI::Cullable::FlagType flag, Filter filter = {})
    {
        for (const BoundsType& bounds : boundsCollection)
        {
            if (filter(bounds))
            {
                MarkMeshesForBounds(scene, bounds, flag);
            }
        }
    }

    template <class Filter = EmptyFilter<BoundsVariant>>
    void MarkMeshesWithFlag(AZ::RPI::Scene* scene, AZStd::span<const BoundsVariant> boundsCollection, AZ::RPI::Cullable::FlagType flag, Filter filter = {})
    {
        for (const BoundsVariant& bounds : boundsCollection)
        {
            if (filter(bounds))
            {
                if (AZStd::holds_alternative<Sphere>(bounds))
                {
                    MarkMeshesForBounds(scene, AZStd::get<Sphere>(bounds), flag);
                }
                else if (AZStd::holds_alternative<Hemisphere>(bounds))
                {
                    MarkMeshesForBounds(scene, AZStd::get<Hemisphere>(bounds), flag);
                }
                else if (AZStd::holds_alternative<Frustum>(bounds))
                {
                    MarkMeshesForBounds(scene, AZStd::get<Frustum>(bounds), flag);
                }
                else if (AZStd::holds_alternative<Aabb>(bounds))
                {
                    MarkMeshesForBounds(scene, AZStd::get<Aabb>(bounds), flag);
                }
                else if (AZStd::holds_alternative<Capsule>(bounds))
                {
                    MarkMeshesForBounds(scene, AZStd::get<Capsule>(bounds), flag);
                }
            }
        }
    }

}
