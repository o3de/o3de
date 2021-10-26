/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "VisibilityDebug.h"

#include <AzCore/Console/Console.h>
#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

AZ_CVAR(
    bool, ed_visibility_debugOctreeEntriesInBounds, true, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Draw entry Aabbs returned from the broad-phase octree intersection");
AZ_CVAR(
    bool, ed_visibility_debugOctreeEntriesInFrustum, true, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Draw entry Aabbs returned from the narrow-phase (frustum) octree intersection");

namespace AzFramework
{
    struct ViewportBox
    {
        AZ::Vector3 m_topLeft, m_topRight, m_bottomRight, m_bottomLeft;
    };

    static void DrawFrustum(
        const ViewportBox& nearViewportBox, const ViewportBox& farViewportBox, DebugDisplayRequests& debugDisplay)
    {
        const auto drawBoxFn = [&debugDisplay](const ViewportBox& viewportBox)
        {
            debugDisplay.DrawLine(viewportBox.m_topLeft, viewportBox.m_topRight);
            debugDisplay.DrawLine(viewportBox.m_topLeft, viewportBox.m_bottomLeft);
            debugDisplay.DrawLine(viewportBox.m_topRight, viewportBox.m_bottomRight);
            debugDisplay.DrawLine(viewportBox.m_bottomLeft, viewportBox.m_bottomRight);
        };

        // draw near and far plane (wire box)
        drawBoxFn(nearViewportBox);
        drawBoxFn(farViewportBox);

        // draw connecting lines
        debugDisplay.DrawLine(nearViewportBox.m_topLeft, farViewportBox.m_topLeft);
        debugDisplay.DrawLine(nearViewportBox.m_topRight, farViewportBox.m_topRight);
        debugDisplay.DrawLine(nearViewportBox.m_bottomRight, farViewportBox.m_bottomRight);
        debugDisplay.DrawLine(nearViewportBox.m_bottomLeft, farViewportBox.m_bottomLeft);
    }

    void DisplayFrustum(const AZ::ViewFrustumAttributes& viewFrustumAttribs, DebugDisplayRequests& debugDisplay)
    {
        DisplayFrustum(
            viewFrustumAttribs.m_worldTransform, viewFrustumAttribs.m_aspectRatio,
            viewFrustumAttribs.m_verticalFovRadians, viewFrustumAttribs.m_nearClip, viewFrustumAttribs.m_farClip,
            debugDisplay);
    }

    void DisplayFrustum(
        const AZ::Transform& worldFromView, const float aspect, const float fovRadians, const float nearClip,
        const float farClip, DebugDisplayRequests& debugDisplay)
    {
        const float tanHalfFov = std::tan(fovRadians * 0.5f);
        const float nearPlaneHalfHeight = tanHalfFov * nearClip;
        const float nearPlaneHalfWidth = nearPlaneHalfHeight * aspect;
        const float farPlaneHalfHeight = tanHalfFov * farClip;
        const float farPlaneHalfWidth = farPlaneHalfHeight * aspect;

        const auto worldViewportBoxFn =
            [&worldFromView](const float halfHeight, const float halfWidth, const float clip)
        {
            return ViewportBox
                {worldFromView.TransformPoint(AZ::Vector3(-halfWidth, clip, halfHeight)),
                 worldFromView.TransformPoint(AZ::Vector3(halfWidth, clip, halfHeight)),
                 worldFromView.TransformPoint(AZ::Vector3(halfWidth, clip, -halfHeight)),
                 worldFromView.TransformPoint(AZ::Vector3(-halfWidth, clip, -halfHeight))};
        };

        const auto nearBox = worldViewportBoxFn(nearPlaneHalfHeight, nearPlaneHalfWidth, nearClip);
        const auto farBox = worldViewportBoxFn(farPlaneHalfHeight, farPlaneHalfWidth, farClip);

        DrawFrustum(nearBox, farBox, debugDisplay);
    }

    void DisplayFrustum(const AZ::Frustum& frustum, DebugDisplayRequests& debugDisplay)
    {
        const auto frustumCornerFn = [&frustum](
                                         const AZ::Frustum::PlaneId planeId1, const AZ::Frustum::PlaneId planeId2,
                                         const AZ::Frustum::PlaneId planeId3)
        {
            AZ::Vector3 corner = AZ::Vector3::CreateZero();
            [[maybe_unused]] const auto intersectionOkay = AZ::ShapeIntersection::IntersectThreePlanes(
                frustum.GetPlane(planeId1), frustum.GetPlane(planeId2), frustum.GetPlane(planeId3), corner);
            AZ_Assert(intersectionOkay, "Plane intersection of Frustum failed");

            return corner;
        };

        const auto boxCenterFn = [](const ViewportBox& box)
        {
            return (box.m_bottomLeft + box.m_bottomRight + box.m_topLeft + box.m_topRight) / 4.0f;
        };

        const AZ::Vector3 farTopLeft = frustumCornerFn(AZ::Frustum::Far, AZ::Frustum::Top, AZ::Frustum::Left);
        const AZ::Vector3 farTopRight = frustumCornerFn(AZ::Frustum::Far, AZ::Frustum::Top, AZ::Frustum::Right);
        const AZ::Vector3 farBottomLeft = frustumCornerFn(AZ::Frustum::Far, AZ::Frustum::Bottom, AZ::Frustum::Left);
        const AZ::Vector3 farBottomRight = frustumCornerFn(AZ::Frustum::Far, AZ::Frustum::Bottom, AZ::Frustum::Right);
        const AZ::Vector3 nearTopLeft = frustumCornerFn(AZ::Frustum::Near, AZ::Frustum::Top, AZ::Frustum::Left);
        const AZ::Vector3 nearTopRight = frustumCornerFn(AZ::Frustum::Near, AZ::Frustum::Top, AZ::Frustum::Right);
        const AZ::Vector3 nearBottomLeft = frustumCornerFn(AZ::Frustum::Near, AZ::Frustum::Bottom, AZ::Frustum::Left);
        const AZ::Vector3 nearBottomRight = frustumCornerFn(AZ::Frustum::Near, AZ::Frustum::Bottom, AZ::Frustum::Right);

        const ViewportBox farBox = {farTopLeft, farTopRight, farBottomRight, farBottomLeft};
        const ViewportBox nearBox = {nearTopLeft, nearTopRight, nearBottomRight, nearBottomLeft};
        const ViewportBox leftBox = {nearTopLeft, farTopLeft, farBottomLeft, nearBottomLeft};
        const ViewportBox rightBox = {nearTopRight, farTopRight, farBottomRight, nearBottomRight};
        const ViewportBox topBox = {nearTopLeft, farTopLeft, farTopRight, nearTopRight};
        const ViewportBox bottomBox = {nearBottomLeft, farBottomLeft, farBottomRight, nearBottomRight};

        debugDisplay.DrawLine(
            boxCenterFn(farBox), boxCenterFn(farBox) + frustum.GetPlane(AZ::Frustum::Far).GetNormal());
        debugDisplay.DrawLine(
            boxCenterFn(nearBox), boxCenterFn(nearBox) + frustum.GetPlane(AZ::Frustum::Near).GetNormal());
        debugDisplay.DrawLine(
            boxCenterFn(leftBox), boxCenterFn(leftBox) + frustum.GetPlane(AZ::Frustum::Left).GetNormal());
        debugDisplay.DrawLine(
            boxCenterFn(rightBox), boxCenterFn(rightBox) + frustum.GetPlane(AZ::Frustum::Right).GetNormal());
        debugDisplay.DrawLine(
            boxCenterFn(topBox), boxCenterFn(topBox) + frustum.GetPlane(AZ::Frustum::Top).GetNormal());
        debugDisplay.DrawLine(
            boxCenterFn(bottomBox), boxCenterFn(bottomBox) + frustum.GetPlane(AZ::Frustum::Bottom).GetNormal());

        DrawFrustum(nearBox, farBox, debugDisplay);
    }

    void DisplayOctreeDebug(const OctreeDebug& octreeDebug, DebugDisplayRequests& debugDisplay)
    {
        if (ed_visibility_debugOctreeEntriesInBounds)
        {
            debugDisplay.SetColor(AZ::Colors::White);
            for (const auto& entryAabb : octreeDebug.m_entryAabbsInBounds)
            {
                debugDisplay.DrawWireBox(entryAabb.GetMin(), entryAabb.GetMax());
            }
        }

        if (ed_visibility_debugOctreeEntriesInFrustum)
        {
            debugDisplay.SetColor(AZ::Colors::LightGreen);
            for (const auto& entryAabb : octreeDebug.m_entryAabbsInFrustum)
            {
                debugDisplay.DrawWireBox(entryAabb.GetMin(), entryAabb.GetMax());
            }
        }

        debugDisplay.SetColor(AZ::Colors::Grey);
        for (const auto& nodeBound : octreeDebug.m_nodeBounds)
        {
            debugDisplay.DrawWireBox(nodeBound.GetMin(), nodeBound.GetMax());
        }
    }
} // namespace AzFramework
