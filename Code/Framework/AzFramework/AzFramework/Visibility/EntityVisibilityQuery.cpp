/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EntityVisibilityQuery.h"

#include <AzCore/Console/Console.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/IVisibilitySystem.h>
#include <AzFramework/Viewport/ViewportScreen.h>

AZ_CVAR(bool, ed_visibility_pinCamera, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Pin the camera in-place");
AZ_CVAR(
    bool, ed_visibility_showDebug, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Draw debug output for the IVisibilitySystem");
AZ_CVAR(
    bool, ed_visibility_debugFrustumFromPlanes, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Draw debug output for the camera view frustum from its interecting planes");
AZ_CVAR(
    bool, ed_visibility_debugFrustumFromAttributes, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Draw debug output for the camera view frustum from its attributes");
AZ_CVAR(
    bool, ed_visibility_debugOctree, true, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Draw debug output for the octree system (see also debugOctreeEntriesInBounds and debugOctreeEntriesInFrustum)");

namespace AzFramework
{
    void EntityVisibilityQuery::UpdateVisibility(const AzFramework::CameraState& cameraState)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        auto* visSystem = AZ::Interface<AzFramework::IVisibilitySystem>::Get();
        if (!visSystem)
        {
            return;
        }

        const bool updateLastCameraState = !ed_visibility_pinCamera || !m_lastCameraState.has_value();
        if (updateLastCameraState)
        {
            m_lastCameraState = cameraState;
        }

        const auto viewFrustum = AzFramework::FrustumFromCameraState(m_lastCameraState.value());

        m_octreeDebug.Clear();
        m_visibleEntityIds.clear();

        visSystem->GetDefaultVisibilityScene()->Enumerate(
            viewFrustum,
            [&viewFrustum, &visibleEntityIdsOut = m_visibleEntityIds,
             &octreeDebug = m_octreeDebug](const AzFramework::IVisibilityScene::NodeData& nodeData)
            {
                if (ed_visibility_showDebug)
                {
                    octreeDebug.m_nodeBounds.push_back(nodeData.m_bounds);
                }

                visibleEntityIdsOut.reserve(visibleEntityIdsOut.size() + nodeData.m_entries.size());
                for (const auto* visibilityEntry : nodeData.m_entries)
                {
                    if (ed_visibility_showDebug)
                    {
                        octreeDebug.m_entryAabbsInBounds.push_back(visibilityEntry->m_boundingVolume);
                    }

                    if (visibilityEntry->m_typeFlags != AzFramework::VisibilityEntry::TYPE_Entity)
                    {
                        continue;
                    }

                    if (!AZ::ShapeIntersection::Overlaps(viewFrustum, visibilityEntry->m_boundingVolume))
                    {
                        continue;
                    }

                    if (ed_visibility_showDebug)
                    {
                        octreeDebug.m_entryAabbsInFrustum.push_back(visibilityEntry->m_boundingVolume);
                    }

                    AZ::EntityId entityId = static_cast<AZ::Entity*>(visibilityEntry->m_userData)->GetId();
                    visibleEntityIdsOut.push_back(entityId);
                }
            });
    }

    void EntityVisibilityQuery::DisplayVisibility(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (ed_visibility_showDebug)
        {
            if (m_lastCameraState.has_value())
            {
                debugDisplay.SetColor(AZ::Colors::White);

                if (ed_visibility_debugFrustumFromPlanes)
                {
                    DisplayFrustum(AzFramework::FrustumFromCameraState(m_lastCameraState.value()), debugDisplay);
                }

                if (ed_visibility_debugFrustumFromAttributes)
                {
                    DisplayFrustum(
                        AzFramework::ViewFrustumAttributesFromCameraState(m_lastCameraState.value()), debugDisplay);
                }
            }

            if (ed_visibility_debugOctree)
            {
                DisplayOctreeDebug(m_octreeDebug, debugDisplay);
            }
        }
    }
} // namespace AzFramework
