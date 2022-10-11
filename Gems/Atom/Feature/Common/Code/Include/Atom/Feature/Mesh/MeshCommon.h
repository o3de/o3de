/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/ShapeIntersection.h>

#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ::Render
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
                            cullable->m_flags.fetch_or(flag);
                        }
                    }
                }
            }
        );
    }

}
