/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorSelectionUtil.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

AZ_CVAR(
    float,
    ed_defaultEntityPlacementDistance,
    10.0f,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "The default distance to place an entity from the camera if no intersection is found");

namespace AzToolsFramework
{
    float GetDefaultEntityPlacementDistance()
    {
        return ed_defaultEntityPlacementDistance;
    }

    AZ::Vector3 CalculateCenterOffset(const AZ::EntityId entityId, const EditorTransformComponentSelectionRequests::Pivot pivot)
    {
        if (Centered(pivot))
        {
            const AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(entityId);
            if (const AZ::Aabb localBound = AzFramework::CalculateEntityLocalBoundsUnion(entity); localBound.IsValid())
            {
                return localBound.GetCenter();
            }
        }

        return AZ::Vector3::CreateZero();
    }

    float CalculateScreenToWorldMultiplier(const AZ::Vector3& worldPosition, const AzFramework::CameraState& cameraState)
    {
        const float apparentDistance = 10.0f;

        // compute the distance from the camera, projected onto the camera's forward direction
        // note: this keeps the scale value the same when positions are at the edge of the screen
        const float projectedCameraDistance = std::abs((cameraState.m_position - worldPosition).Dot(cameraState.m_forward));

        // author sizes of bounds/manipulators as they would appear
        // in perspective 10 meters from the camera.
        return AZ::GetMax(projectedCameraDistance, cameraState.m_nearClip) / apparentDistance;
    }

    bool AabbIntersectRay(const AZ::Vector3& origin, const AZ::Vector3& direction, const AZ::Aabb& aabb, float& distance)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        const AZ::Vector3 rayScaledDir = direction * EditorPickRayLength;

        float t, end;
        AZ::Vector3 startNormal;
        if (AZ::Intersect::IntersectRayAABB(origin, rayScaledDir, rayScaledDir.GetReciprocal(), aabb, t, end, startNormal) > 0)
        {
            distance = t * EditorPickRayLength;
            return true;
        }

        return false;
    }

    bool AabbIntersectMouseRay(const ViewportInteraction::MouseInteraction& mouseInteraction, const AZ::Aabb& aabb)
    {
        float unused;
        return AabbIntersectRay(mouseInteraction.m_mousePick.m_rayOrigin, mouseInteraction.m_mousePick.m_rayDirection, aabb, unused);
    }

    bool PickEntity(
        const AZ::EntityId entityId,
        const ViewportInteraction::MouseInteraction& mouseInteraction,
        float& closestDistance,
        const int viewportId)
    {
        AZ_PROFILE_FUNCTION(Entity);

        bool entityPicked = false;
        EditorComponentSelectionRequestsBus::EnumerateHandlersId(
            entityId,
            [mouseInteraction, &entityPicked, &closestDistance, viewportId](EditorComponentSelectionRequests* handler) -> bool
            {
                if (handler->SupportsEditorRayIntersect())
                {
                    float distance = std::numeric_limits<float>::max();
                    const bool intersection = handler->EditorSelectionIntersectRayViewport(
                        { viewportId }, mouseInteraction.m_mousePick.m_rayOrigin, mouseInteraction.m_mousePick.m_rayDirection, distance);

                    if (intersection && distance < closestDistance)
                    {
                        entityPicked = true;
                        closestDistance = distance;
                    }
                }

                return true; // iterate over all handlers
            });

        return entityPicked;
    }

    AzFramework::CameraState GetCameraState(const int viewportId)
    {
        AzFramework::CameraState cameraState;
        ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            cameraState, viewportId, &ViewportInteraction::ViewportInteractionRequestBus::Events::GetCameraState);

        return cameraState;
    }

    float GetScreenDisplayScaling(const int viewportId)
    {
        float scaling = 1.0f;
        ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            scaling, viewportId, &ViewportInteraction::ViewportInteractionRequestBus::Events::DeviceScalingFactor);

        return scaling;
    }

} // namespace AzToolsFramework
