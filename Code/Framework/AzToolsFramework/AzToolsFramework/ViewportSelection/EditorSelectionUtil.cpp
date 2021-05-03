/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "EditorSelectionUtil.h"

#include <AzCore/Math/Aabb.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    /// Default ray length for picking in the viewport.
    static const float s_pickRayLength = 1000.0f;

    AZ::Vector3 CalculateCenterOffset(
        const AZ::EntityId entityId, const EditorTransformComponentSelectionRequests::Pivot pivot)
    {
        if (Centered(pivot))
        {
            const AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(entityId);
            if (const AZ::Aabb localBound = AzFramework::CalculateEntityLocalBoundsUnion(entity);
                localBound.IsValid())
            {
                return localBound.GetCenter();
            }
        }

        return AZ::Vector3::CreateZero();
    }

    float CalculateScreenToWorldMultiplier(
        const AZ::Vector3& worldPosition, const AzFramework::CameraState& cameraState)
    {
        const float apparentDistance = 10.0f;

        // compute the distance from the camera, projected onto the camera's forward direction
        // note: this keeps the scale value the same when positions are at the edge of the screen
        const float projectedCameraDistance =
            std::abs((cameraState.m_position - worldPosition).Dot(cameraState.m_forward));

        // author sizes of bounds/manipulators as they would appear
        // in perspective 10 meters from the camera.
        return AZ::GetMax(projectedCameraDistance, cameraState.m_nearClip) / apparentDistance;
    }

    QPoint GetScreenPosition(const int viewportId, const AZ::Vector3& worldTranslation)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        QPoint screenPosition = QPoint();
        ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            screenPosition, viewportId,
            &ViewportInteraction::ViewportInteractionRequestBus::Events::ViewportWorldToScreen,
            worldTranslation);

        return screenPosition;
    }

    bool AabbIntersectMouseRay(
        const ViewportInteraction::MouseInteraction& mouseInteraction, const AZ::Aabb& aabb)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const AZ::Vector3 rayScaledDir =
            mouseInteraction.m_mousePick.m_rayDirection * s_pickRayLength;

        AZ::Vector3 startNormal;
        float t, end;
        return AZ::Intersect::IntersectRayAABB(
            mouseInteraction.m_mousePick.m_rayOrigin, rayScaledDir,
            rayScaledDir.GetReciprocal(), aabb, t, end, startNormal) > 0;
    }

    bool PickEntity(
        const AZ::EntityId entityId, const ViewportInteraction::MouseInteraction& mouseInteraction,
        float& closestDistance, const int viewportId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        bool entityPicked = false;
        EditorComponentSelectionRequestsBus::EnumerateHandlersId(
            entityId, [mouseInteraction, &entityPicked, &closestDistance, viewportId]
            (EditorComponentSelectionRequests* handler) -> bool
        {
            if (handler->SupportsEditorRayIntersect())
            {
                float distance = std::numeric_limits<float>::max();
                const bool intersection = handler->EditorSelectionIntersectRayViewport(
                    { viewportId }, mouseInteraction.m_mousePick.m_rayOrigin,
                    mouseInteraction.m_mousePick.m_rayDirection, distance);

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
            cameraState, viewportId,
            &ViewportInteraction::ViewportInteractionRequestBus::Events::GetCameraState);
        
        return cameraState;
    }
} // namespace AzToolsFramework
