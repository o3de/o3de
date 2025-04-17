/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Render/IntersectorInterface.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

DECLARE_EBUS_INSTANTIATION_WITH_TRAITS(AzToolsFramework::ViewportInteraction::ViewportInteractionRequests, AzToolsFramework::ViewportInteraction::ViewportRequestsEBusTraits);

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

    AZStd::optional<AZ::Vector3> FindClosestPickIntersection(const AzFramework::RenderGeometry::RayRequest& rayRequest)
    {
        using AzFramework::RenderGeometry::IntersectorBus;
        using AzFramework::RenderGeometry::RayResult;
        using AzFramework::RenderGeometry::RayResultClosestAggregator;
        using AzFramework::Terrain::TerrainDataRequestBus;

        // attempt a ray intersection with any visible mesh or terrain and return the intersection position if successful
        AZ::EBusReduceResult<RayResult, RayResultClosestAggregator> renderGeometryIntersectionResult;
        IntersectorBus::EventResult(
            renderGeometryIntersectionResult, AzToolsFramework::GetEntityContextId(), &IntersectorBus::Events::RayIntersect, rayRequest);
        TerrainDataRequestBus::BroadcastResult(
            renderGeometryIntersectionResult, &TerrainDataRequestBus::Events::GetClosestIntersection, rayRequest);

        if (renderGeometryIntersectionResult.value)
        {
            return renderGeometryIntersectionResult.value.m_worldPosition;
        }
        else
        {
            return {};
        }
    }

    AZ::Vector3 FindClosestPickIntersection(const AzFramework::RenderGeometry::RayRequest& rayRequest, const float defaultDistance)
    {
        auto result = FindClosestPickIntersection(rayRequest);

        if (result.has_value())
        {
            return result.value();
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
        rayRequest.m_startWorldPosition = viewportRay.m_origin;
        rayRequest.m_endWorldPosition = viewportRay.m_origin + viewportRay.m_direction * rayLength;
    }

    AZStd::optional<AZ::Vector3> FindClosestPickIntersection(
        const AzFramework::ViewportId viewportId, const AzFramework::ScreenPoint& screenPoint, const float rayLength)
    {
        AzFramework::RenderGeometry::RayRequest ray;
        ray.m_onlyVisible = true; // only consider visible objects

        RefreshRayRequest(ray, ViewportInteraction::ViewportScreenToWorldRay(viewportId, screenPoint), rayLength);

        return FindClosestPickIntersection(ray);
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

    namespace ViewportInteraction
    {
        MouseInteractionResult InternalMouseViewportRequests::InternalHandleAllMouseInteractions(
            const MouseInteractionEvent& mouseInteraction)
        {
            if (InternalHandleMouseManipulatorInteraction(mouseInteraction))
            {
                return MouseInteractionResult::Manipulator;
            }
            else if (InternalHandleMouseViewportInteraction(mouseInteraction))
            {
                return MouseInteractionResult::Viewport;
            }
            else
            {
                return MouseInteractionResult::None;
            }
        }

        KeyboardModifiers QueryKeyboardModifiers()
        {
            KeyboardModifiers keyboardModifiers;
            EditorModifierKeyRequestBus::BroadcastResult(keyboardModifiers, &EditorModifierKeyRequestBus::Events::QueryKeyboardModifiers);
            return keyboardModifiers;
        }

        ProjectedViewportRay ViewportScreenToWorldRay(const AzFramework::ViewportId viewportId, const AzFramework::ScreenPoint& screenPoint)
        {
            ProjectedViewportRay viewportRay{};
            ViewportInteractionRequestBus::EventResult(
                viewportRay, viewportId, &ViewportInteractionRequestBus::Events::ViewportScreenToWorldRay, screenPoint);

            return viewportRay;
        }
    }

    AzFramework::EntityContextId GetEntityContextId()
    {
        auto entityContextId = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(entityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);
        return entityContextId;
    }
} // namespace AzToolsFramework
