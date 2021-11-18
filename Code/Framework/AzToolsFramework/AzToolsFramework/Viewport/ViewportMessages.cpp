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

    AZ::Vector3 FindClosestPickIntersection(
        AzFramework::ViewportId viewportId, const AzFramework::ScreenPoint& screenPoint, const float rayLength, const float defaultDistance)
    {
        using AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus;
        AzToolsFramework::ViewportInteraction::ProjectedViewportRay viewportRay{};
        ViewportInteractionRequestBus::EventResult(
            viewportRay, viewportId, &ViewportInteractionRequestBus::Events::ViewportScreenToWorldRay, screenPoint);

        AzFramework::RenderGeometry::RayRequest ray;
        ray.m_startWorldPosition = viewportRay.origin;
        ray.m_endWorldPosition = viewportRay.origin + viewportRay.direction * rayLength;
        ray.m_onlyVisible = true;

        AzFramework::RenderGeometry::RayResult renderGeometryIntersectionResult;
        AzFramework::RenderGeometry::IntersectorBus::EventResult(
            renderGeometryIntersectionResult, AzToolsFramework::GetEntityContextId(),
            &AzFramework::RenderGeometry::IntersectorBus::Events::RayIntersect, ray);

        // attempt a ray intersection with any visible mesh and return the intersection position if successful
        if (renderGeometryIntersectionResult)
        {
            return renderGeometryIntersectionResult.m_worldPosition;
        }
        else
        {
            return viewportRay.origin + viewportRay.direction * defaultDistance;
        }
    }
} // namespace AzToolsFramework
