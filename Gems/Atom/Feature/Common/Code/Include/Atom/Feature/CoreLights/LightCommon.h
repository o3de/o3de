/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Culling.h>
#include <AzCore/Math/ShapeIntersection.h>

namespace AZ::Render::LightCommon
{
    template <typename BoundsType>
    struct EmptyFilter
    {
        constexpr bool operator()(BoundsType) const { return true; }
    };

    template <typename BoundsType, class Filter = EmptyFilter<BoundsType>>
    void MarkMeshesWithLightType(AZ::RPI::Scene* scene, AZStd::span<const BoundsType> bounds, AZ::RPI::Cullable::FlagType flag, Filter filter = {})
    {
        AzFramework::IVisibilityScene* visScene = scene->GetVisibilityScene();

        for (const BoundsType& lightBounds : bounds)
        {
            if (filter(lightBounds))
            {
                visScene->Enumerate(lightBounds, [flag, &lightBounds = lightBounds](const AzFramework::IVisibilityScene::NodeData& nodeData)
                    {
                        bool nodeIsContainedInFrustum = ShapeIntersection::Contains(lightBounds, nodeData.m_bounds);
                        for (auto* visibleEntry : nodeData.m_entries)
                        {
                            if (visibleEntry->m_typeFlags == AzFramework::VisibilityEntry::TYPE_RPI_Cullable)
                            {
                                RPI::Cullable* cullable = static_cast<RPI::Cullable*>(visibleEntry->m_userData);

                                if (nodeIsContainedInFrustum || ShapeIntersection::Overlaps(lightBounds, cullable->m_cullData.m_boundingSphere))
                                {
                                    cullable->m_flags.fetch_or(flag);
                                }
                            }
                        }
                    }
                );
            }
        }
    }

} // namespace AZ::Render::LightCommon

