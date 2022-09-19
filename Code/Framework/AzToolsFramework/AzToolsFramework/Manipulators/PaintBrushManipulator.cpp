/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Geometry2DUtils.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>
#include <AzToolsFramework/Manipulators/PaintBrushNotificationBus.h>
#include <AzToolsFramework/Manipulators/PaintBrushRequestBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsWindow.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    AZStd::shared_ptr<PaintBrushManipulator> PaintBrushManipulator::MakeShared(
        const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        return AZStd::shared_ptr<PaintBrushManipulator>(aznew PaintBrushManipulator(worldFromLocal, entityComponentIdPair));
    }

    PaintBrushManipulator::PaintBrushManipulator(
        const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_ownerEntityComponentId = entityComponentIdPair;

        SetSpace(worldFromLocal);

        // Make sure the Paint Brush Settings window is open
        AzToolsFramework::OpenViewPane(PaintBrush::s_paintBrushSettingsName);

        // Get the radius from the Paint Brush Settings window.
        float radius = 0.0f;
        PaintBrushSettingsRequestBus::BroadcastResult(radius, &PaintBrushSettingsRequestBus::Events::GetRadius);

        // The PaintBrush manipulator uses a circle projected into world space to represent the brush.
        const AZ::Color manipulatorColor = AZ::Colors::Red;
        const float manipulatorWidth = 0.05f;
        SetView(
            AzToolsFramework::CreateManipulatorViewProjectedCircle(*this, manipulatorColor, radius, manipulatorWidth));

        // Start listening for any changes to the Paint Brush Settings
        PaintBrushSettingsNotificationBus::Handler::BusConnect();

        // Notify listeners that we've entered the paint mode.
        PaintBrushNotificationBus::Broadcast(&PaintBrushNotificationBus::Events::OnPaintModeBegin, m_ownerEntityComponentId);
    }

    PaintBrushManipulator::~PaintBrushManipulator()
    {
        // Notify listeners that we've exited the paint mode.
        PaintBrushNotificationBus::Broadcast(&PaintBrushNotificationBus::Events::OnPaintModeEnd, m_ownerEntityComponentId);

        // Stop listening for any changes to the Paint Brush Settings
        PaintBrushSettingsNotificationBus::Handler::BusDisconnect();
    }

    void PaintBrushManipulator::Draw(
        const ManipulatorManagerState& managerState, AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState, const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        m_manipulatorView->Draw(
            GetManipulatorManagerId(), managerState, GetManipulatorId(),
            { GetSpace(), GetNonUniformScale(), AZ::Vector3::CreateZero(), MouseOver() }, debugDisplay, cameraState,
            mouseInteraction);
    }

    void PaintBrushManipulator::SetView(AZStd::shared_ptr<ManipulatorViewProjectedCircle> view)
    {
        m_manipulatorView = AZStd::move(view);
    }

    void PaintBrushManipulator::OnRadiusChanged(float radius)
    {
        m_manipulatorView->SetRadius(radius);
    }

    bool PaintBrushManipulator::HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Move)
        {
            const bool isFirstPaintedPoint = false;
            MovePaintBrush(
                mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId,
                mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates, isFirstPaintedPoint);

            // For move events, always return false so that mouse movements with right clicks can still affect the Editor camera.
            return false;
        }
        else if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down)
        {
            if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left())
            {
                m_isPainting = true;
                PaintBrushNotificationBus::Broadcast(&PaintBrushNotificationBus::Events::OnPaintBegin, m_ownerEntityComponentId);

                const bool isFirstPaintedPoint = true;
                MovePaintBrush(
                    mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId,
                    mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates, isFirstPaintedPoint);
                return true;
            }
        }
        else if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Up)
        {
            if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left())
            {
                m_isPainting = false;
                PaintBrushNotificationBus::Broadcast(&PaintBrushNotificationBus::Events::OnPaintEnd, m_ownerEntityComponentId);

                return true;
            }
        }
        return false;
    }

    void PaintBrushManipulator::MovePaintBrush(int viewportId, const AzFramework::ScreenPoint& screenCoordinates, bool isFirstPaintedPoint)
    {
        // Ray cast into the screen to find the closest collision point for the current mouse location.
        auto worldSurfacePosition =
            AzToolsFramework::FindClosestPickIntersection(viewportId, screenCoordinates, AzToolsFramework::EditorPickRayLength);

        // If the mouse isn't colliding with anything, don't move the paintbrush, just leave it at its last location.
        if (!worldSurfacePosition.has_value())
        {
            return;
        }

        // We found a collision point, so move the paintbrush.
        m_center = worldSurfacePosition.value();
        AZ::Transform space = AZ::Transform::CreateTranslation(m_center);
        SetSpace(space);
        PaintBrushNotificationBus::Broadcast(&PaintBrushNotificationBus::Events::OnWorldSpaceChanged, m_ownerEntityComponentId, space);

        // If we're currently painting, send off a paint notification.
        if (m_isPainting)
        {
            // Keep track of the previous painted point and the current one. We'll use that to create a brush stroke
            // that covers everything in-between.
            if (isFirstPaintedPoint)
            {
                m_previousCenter = m_center;
            }

            // Get our current paint brush settings.

            float radius = 0.0f;
            float intensity = 0.0f;
            float opacity = 0.0f;

            PaintBrushSettingsRequestBus::BroadcastResult(radius, &PaintBrushSettingsRequestBus::Events::GetRadius);
            PaintBrushSettingsRequestBus::BroadcastResult(intensity, &PaintBrushSettingsRequestBus::Events::GetIntensity);
            PaintBrushSettingsRequestBus::BroadcastResult(opacity, &PaintBrushSettingsRequestBus::Events::GetOpacity);

            // Create an AABB that contains both endpoints. By definition, it will contain all of the brush stroke
            // points that fall in-between as well.
            AZ::Aabb strokeRegion = AZ::Aabb::CreateCenterRadius(m_center, radius);
            strokeRegion.AddAabb(AZ::Aabb::CreateCenterRadius(m_previousCenter, radius));

            const float manipulatorRadiusSq = radius * radius;
            const AZ::Vector2 previousCenter2D(m_previousCenter);
            const AZ::Vector2 center2D(m_center);

            // Callback function that we pass into OnPaint so that paint handling code can request specific paint values
            // for the world positions it cares about.
            PaintBrushNotifications::ValueLookupFn valueLookupFn(
                [manipulatorRadiusSq, previousCenter2D, center2D, intensity, opacity](
                AZStd::span<const AZ::Vector3> points, AZStd::span<float> intensities,
                AZStd::span<float> opacities, AZStd::span<bool> validFlags)
            {
                for (size_t index = 0; index < points.size(); index++)
                {
                    // Any point that falls within our brush stroke is considered valid. Since our brush is a circle, the brush stroke
                    // ends up being a capsule with a circle on each end, and effectively a box connecting the two circles.
                    // We can easily check to see if the point falls in the capsule by getting the distance between the point
                    // and the line segment formed between the center of the start and end circles.
                    // If this distance is less than the radius of the circle, then it falls within the brush stroke.
                    // This works equally well whether the point is closest to an endpoint of the segment or a point along the segment.
                    if (AZ::Geometry2DUtils::ShortestDistanceSqPointSegment(AZ::Vector2(points[index]), previousCenter2D, center2D) <=
                         manipulatorRadiusSq)
                    {
                        intensities[index] = intensity;
                        opacities[index] = opacity;
                        validFlags[index] = true;
                    }
                    else
                    {
                        intensities[index] = 0.0f;
                        opacities[index] = 0.0f;
                        validFlags[index] = false;
                    }
                }
            });

            PaintBrushNotificationBus::Broadcast(
                &PaintBrushNotificationBus::Events::OnPaint, m_ownerEntityComponentId, strokeRegion, valueLookupFn);

            m_previousCenter = m_center;
        }
    }

} // namespace AzToolsFramework
