/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
//AZTF-SHARED
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ClickDetector.h>
#include <AzFramework/Viewport/ViewportId.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/Viewport/ViewportMessagesBus.h>
namespace AzFramework
{
    struct ScreenPoint;

    namespace RenderGeometry
    {
        struct RayRequest;
    }
} // namespace AzFramework

namespace AzToolsFramework
{
    namespace ViewportInteraction
    {
        //! Convenience method to call the above EditorModifierKeyRequestBus's QueryKeyModifiers event
        AZTF_API KeyboardModifiers QueryKeyboardModifiers();

        //! Utility function to return a viewport ray using the ViewportInteractionRequestBus.
        AZTF_API ProjectedViewportRay ViewportScreenToWorldRay(const AzFramework::ViewportId viewportId, const AzFramework::ScreenPoint& screenPoint);
    }

    //! Utility function to return EntityContextId.
    AZTF_API AzFramework::EntityContextId GetEntityContextId();

    //! Performs an intersection test against meshes in the scene, if there is a hit (the ray intersects
    //! a mesh), that position is returned, otherwise a point projected defaultDistance from the
    //! origin of the ray will be returned.
    //! @note The intersection will only consider visible objects.
    //! @param viewportId The id of the viewport the raycast into.
    //! @param screenPoint The starting point of the ray in screen space. (The ray will cast into the screen)
    //! @param rayLength The length of the ray in meters.
    //! @param defaultDistance The distance to use for the result point if no hit is found.
    //! @return The world position of the intersection point (either from a hit or from the default distance)
    AZTF_API AZ::Vector3 FindClosestPickIntersection(
        AzFramework::ViewportId viewportId, const AzFramework::ScreenPoint& screenPoint, float rayLength, float defaultDistance);

    //! Performs an intersection test against meshes in the scene and returns the his position only if there
    //! is a hit (the ray intersects a mesh).
    //! @note The intersection will only consider visible objects.
    //! @param viewportId The id of the viewport the raycast into.
    //! @param screenPoint The starting point of the ray in screen space. (The ray will cast into the screen)
    //! @param rayLength The length of the ray in meters.
    //! @return The world position of the intersection if there is a hit, or nothing if not.
    AZTF_API AZStd::optional<AZ::Vector3> FindClosestPickIntersection(
        AzFramework::ViewportId viewportId, const AzFramework::ScreenPoint& screenPoint, float rayLength);

    //! Overload of FindClosestPickIntersection taking a RenderGeometry::RayRequest directly.
    //! @note rayRequest must contain a valid ray/line segment (start/endWorldPosition must not be at the same position).
    //! @param rayRequest Information describing the start/end positions and which entities to raycast against.
    //! @param defaultDistance The distance to use for the result point if no hit is found.
    //! @return The world position of the intersection point (either from a hit or from the default distance)
    AZTF_API AZ::Vector3 FindClosestPickIntersection(const AzFramework::RenderGeometry::RayRequest& rayRequest, float defaultDistance);

    //! Overload of FindClosestPickIntersection taking a RenderGeometry::RayRequest directly.
    //! @note rayRequest must contain a valid ray/line segment (start/endWorldPosition must not be at the same position).
    //! @param rayRequest Information describing the start/end positions and which entities to raycast against.
    //! @return The world position of the intersection if there is a hit, or nothing if not.
    AZTF_API AZStd::optional<AZ::Vector3> FindClosestPickIntersection(const AzFramework::RenderGeometry::RayRequest& rayRequest);

    //! Update the in/out parameter rayRequest based on the latest viewport ray.
    AZTF_API void RefreshRayRequest(
        AzFramework::RenderGeometry::RayRequest& rayRequest, const ViewportInteraction::ProjectedViewportRay& viewportRay, float rayLength);

    //! Maps a mouse interaction event to a ClickDetector event.
    //! @note Function only cares about up or down events, all other events are mapped to Nil (ignored).
    AZTF_API AzFramework::ClickDetector::ClickEvent ClickDetectorEventFromViewportInteraction(
        const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

    //! Wrap EBus call to retrieve manipulator line bound width.
    //! @note It is possible to pass AzFramework::InvalidViewportId (the default) to perform a Broadcast as opposed to a targeted Event.
    AZTF_API float ManipulatorLineBoundWidth(AzFramework::ViewportId viewportId = AzFramework::InvalidViewportId);

    //! Wrap EBus call to retrieve manipulator circle bound width.
    //! @note It is possible to pass AzFramework::InvalidViewportId (the default) to perform a Broadcast as opposed to a targeted Event.
    AZTF_API float ManipulatorCicleBoundWidth(AzFramework::ViewportId viewportId = AzFramework::InvalidViewportId);
} // namespace AzToolsFramework
