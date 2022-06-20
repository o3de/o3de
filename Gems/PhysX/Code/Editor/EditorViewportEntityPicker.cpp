
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/IntersectSegment.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>
#include <Editor/EditorViewportEntityPicker.h>

namespace PhysX
{
    EditorViewportEntityPicker::EditorViewportEntityPicker()
    {
        m_entityDataCache = AZStd::make_unique<AzToolsFramework::EditorVisibleEntityDataCache>();
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
    }

    EditorViewportEntityPicker::~EditorViewportEntityPicker()
    {
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();
    }

    AZ::EntityId EditorViewportEntityPicker::PickEntity(
        [[maybe_unused]] const AzFramework::CameraState& cameraState,
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
        AZ::Vector3& pickPosition,
        AZ::Aabb& pickAabb)
    {
        const int viewportId = mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId;

        // selecting new entities
        AZ::EntityId entityIdUnderCursor;
        pickPosition = AZ::Vector3::CreateZero();
        pickAabb = AZ::Aabb::CreateNull();

        for (size_t entityCacheIndex = 0; entityCacheIndex < m_entityDataCache->VisibleEntityDataCount(); ++entityCacheIndex)
        {
            const AZ::EntityId entityId = m_entityDataCache->GetVisibleEntityId(entityCacheIndex);

            if (m_entityDataCache->IsVisibleEntityLocked(entityCacheIndex) || !m_entityDataCache->IsVisibleEntityVisible(entityCacheIndex))
            {
                continue;
            }

            // Ignore the case where the mouse hovers over an icon. Proceed to check for intersection with entity's AABB.
            if (const AZ::Aabb aabb =
                    AzToolsFramework::CalculateEditorEntitySelectionBounds(entityId, AzFramework::ViewportInfo{ viewportId });
                aabb.IsValid())
            {
                const float pickRayLength = 1000.0f;
                const AZ::Vector3 rayScaledDir = mouseInteraction.m_mouseInteraction.m_mousePick.m_rayDirection * pickRayLength;

                AZ::Vector3 startNormal;
                float t, end;
                int intersectResult = AZ::Intersect::IntersectRayAABB(
                    mouseInteraction.m_mouseInteraction.m_mousePick.m_rayOrigin, rayScaledDir, rayScaledDir.GetReciprocal(), aabb, t, end,
                    startNormal);
                if (intersectResult > 0)
                {
                    entityIdUnderCursor = entityId;
                    pickPosition = mouseInteraction.m_mouseInteraction.m_mousePick.m_rayOrigin +
                        (mouseInteraction.m_mouseInteraction.m_mousePick.m_rayDirection * pickRayLength * t);
                    pickAabb = aabb;
                }
            }
        }

        return entityIdUnderCursor;
    }

    void EditorViewportEntityPicker::DisplayViewport(
        const AzFramework::ViewportInfo& viewportInfo, [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
    {
        m_entityDataCache->CalculateVisibleEntityDatas(viewportInfo);
    }
} // namespace PhysX
