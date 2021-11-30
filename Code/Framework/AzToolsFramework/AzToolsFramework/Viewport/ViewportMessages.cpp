/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Render/IntersectorInterface.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    AzFramework::ClickDetector::ClickEvent ClickDetectorEventFromViewportInteraction(
        const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left())
        {
            if (mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down)
            {
                return AzFramework::ClickDetector::ClickEvent::Down;
            }

            if (mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Up)
            {
                return AzFramework::ClickDetector::ClickEvent::Up;
            }
        }

        return AzFramework::ClickDetector::ClickEvent::Nil;
    }

    float ManipulatorLineBoundWidth(const AzFramework::ViewportId viewportId /*= AzFramework::InvalidViewportId*/)
    {
        float lineBoundWidth = 0.0f;
        if (viewportId != AzFramework::InvalidViewportId)
        {
            ViewportInteraction::ViewportSettingsRequestBus::EventResult(
                lineBoundWidth, viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::ManipulatorLineBoundWidth);
        }
        else
        {
            ViewportInteraction::ViewportSettingsRequestBus::BroadcastResult(
                lineBoundWidth, &ViewportInteraction::ViewportSettingsRequestBus::Events::ManipulatorLineBoundWidth);
        }

        return lineBoundWidth;
    }

    float ManipulatorCicleBoundWidth(const AzFramework::ViewportId viewportId /*= AzFramework::InvalidViewportId*/)
    {
        float circleBoundWidth = 0.0f;
        if (viewportId != AzFramework::InvalidViewportId)
        {
            ViewportInteraction::ViewportSettingsRequestBus::EventResult(
                circleBoundWidth, viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::ManipulatorCircleBoundWidth);
        }
        else
        {
            ViewportInteraction::ViewportSettingsRequestBus::BroadcastResult(
                circleBoundWidth, &ViewportInteraction::ViewportSettingsRequestBus::Events::ManipulatorCircleBoundWidth);
        }

        return circleBoundWidth;
    }

    AZ::Vector3 FindClosestPickIntersection(const AzFramework::RenderGeometry::RayRequest& rayRequest, const float defaultDistance)
    {
        AzFramework::RenderGeometry::RayResult renderGeometryIntersectionResult;
        AzFramework::RenderGeometry::IntersectorBus::EventResult(
            renderGeometryIntersectionResult, AzToolsFramework::GetEntityContextId(),
            &AzFramework::RenderGeometry::IntersectorBus::Events::RayIntersect, rayRequest);

        // attempt a ray intersection with any visible mesh and return the intersection position if successful
        if (renderGeometryIntersectionResult)
        {
            return renderGeometryIntersectionResult.m_worldPosition;
        }
        else
        {
            const AZ::Vector3 rayDirection = (rayRequest.m_endWorldPosition - rayRequest.m_startWorldPosition).GetNormalized();
            return rayRequest.m_startWorldPosition + rayDirection * defaultDistance;
        }
    }

    void RefreshRayRequest(
        AzFramework::RenderGeometry::RayRequest& rayRequest,
        const ViewportInteraction::ProjectedViewportRay& viewportRay,
        const float rayLength)
    {
        AZ_Assert(rayLength > 0.0f, "Invalid ray length passed to RefreshRayRequest");
        rayRequest.m_startWorldPosition = viewportRay.origin;
        rayRequest.m_endWorldPosition = viewportRay.origin + viewportRay.direction * rayLength;
    }

    AZ::Vector3 FindClosestPickIntersection(
        const AzFramework::ViewportId viewportId,
        const AzFramework::ScreenPoint& screenPoint,
        const float rayLength,
        const float defaultDistance)
    {
        AzFramework::RenderGeometry::RayRequest ray;
        ray.m_onlyVisible = true; // only consider visible objects

        RefreshRayRequest(ray, ViewportInteraction::ViewportScreenToWorldRay(viewportId, screenPoint), rayLength);

        return FindClosestPickIntersection(ray, defaultDistance);
    }
} // namespace AzToolsFramework
