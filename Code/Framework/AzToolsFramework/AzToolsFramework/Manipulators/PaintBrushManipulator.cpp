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
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsNotificationBus.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsRequestBus.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsWindow.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    AZ_CVAR(
        float,
        ed_paintBrushSizeAdjustAmount,
        0.25f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The amount to increase / decrease the paintbrush size in meters.");

    namespace
    {
        static constexpr AZ::Crc32 PaintbrushIncreaseSize = AZ_CRC_CE("org.o3de.action.paintbrush.increase_size");
        static constexpr AZ::Crc32 PaintbrushDecreaseSize = AZ_CRC_CE("org.o3de.action.paintbrush.decrease_size");

        static constexpr const char* PaintbrushIncreaseSizeTitle = "Increase Size";
        static constexpr const char* PaintbrushDecreaseSizeTitle = "Decrease Size";

        static constexpr const char* PaintbrushIncreaseSizeDesc = "Increases size of paintbrush";
        static constexpr const char* PaintbrushDecreaseSizeDesc = "Decreases size of paintbrush";
    } // namespace

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

        // Get the diameter from the global Paint Brush Settings.
        float diameter = 0.0f;
        PaintBrushSettingsRequestBus::BroadcastResult(diameter, &PaintBrushSettingsRequestBus::Events::GetSize);
        const float radius = diameter / 2.0f;

        // The PaintBrush manipulator uses a circle projected into world space to represent the brush.
        const AZ::Color manipulatorColor = AZ::Colors::Red;
        const float manipulatorWidth = 0.05f;
        SetView(AzToolsFramework::CreateManipulatorViewProjectedCircle(*this, manipulatorColor, radius, manipulatorWidth));

        // Start listening for any changes to the Paint Brush Settings
        PaintBrushSettingsNotificationBus::Handler::BusConnect();

        // Notify listeners that we've entered the paint mode.
        PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnPaintModeBegin);
    }

    PaintBrushManipulator::~PaintBrushManipulator()
    {
        // Notify listeners that we've exited the paint mode.
        PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnPaintModeEnd);

        // Stop listening for any changes to the Paint Brush Settings
        PaintBrushSettingsNotificationBus::Handler::BusDisconnect();

        // Make sure the Paint Brush Settings window is closed
        AzToolsFramework::CloseViewPane(PaintBrush::s_paintBrushSettingsName);
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

    void PaintBrushManipulator::OnSettingsChanged(const PaintBrushSettings& newSettings)
    {
        float diameter = newSettings.GetSize();
        m_manipulatorView->SetRadius(diameter / 2.0f);
    }

    bool PaintBrushManipulator::HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(Entity);

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
                // Get the intensity and opacity to use for this brush stroke.
                float intensityPercent = 0.0f;
                float opacityPercent = 0.0f;

                PaintBrushSettingsRequestBus::BroadcastResult(intensityPercent, &PaintBrushSettingsRequestBus::Events::GetIntensityPercent);
                PaintBrushSettingsRequestBus::BroadcastResult(opacityPercent, &PaintBrushSettingsRequestBus::Events::GetOpacityPercent);

                // Convert intensity and opacity to the 0-1 range.
                const float intensity = AZStd::clamp(intensityPercent / 100.0f, 0.0f, 1.0f);
                const float opacity = AZStd::clamp(opacityPercent / 100.0f, 0.0f, 1.0f);

                // Notify that a paint stroke has begun, and provide the stroke intensity and opacity in the 0-1 range.
                m_isPainting = true;
                PaintBrushNotificationBus::Event(
                    m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnPaintStrokeBegin, intensity, opacity);

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
                // Notify that the paint stroke has ended.
                // We need to verify that we're currently painting, because clicks/double-clicks can cause us to end up here
                // without ever getting the mouse Down event.
                if (m_isPainting)
                {
                    m_isPainting = false;
                    PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnPaintStrokeEnd);
                }

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
        AZ::Vector3 brushCenter = worldSurfacePosition.value();
        AZ::Transform space = AZ::Transform::CreateTranslation(brushCenter);
        SetSpace(space);
        PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnWorldSpaceChanged, space);

        // If we're currently painting, send off a paint notification.
        if (m_isPainting)
        {
            AZ::Vector2 currentCenter2D(brushCenter);

            // Get our current paint brush settings.

            PaintBrushSettings currentSettings;
            PaintBrushSettingsRequestBus::BroadcastResult(currentSettings, &PaintBrushSettingsRequestBus::Events::GetSettings);

            // Early out if we're completely transparent, there's no distance between brush stamps, or the brush stamp size is 0.
            if ((currentSettings.GetOpacityPercent() == 0.0f) ||
                (currentSettings.GetFlowPercent() == 0.0f) ||
                (currentSettings.GetDistancePercent() == 0.0f) ||
                (currentSettings.GetSize() == 0.0f))
            {
                return;
            }

            // Convert our settings into the 0-1 range
            const float hardness = currentSettings.GetHardnessPercent() / 100.0f;
            const float flow = currentSettings.GetFlowPercent() / 100.0f;

            // Get the distance between each brush stamp in world space.
            const float distanceBetweenBrushStamps = currentSettings.GetSize() * (currentSettings.GetDistancePercent() / 100.0f);

            // Track the list of center points for each brush stamp to draw for this mouse movement.
            AZStd::vector<AZ::Vector2> brushStamps;

            // If this is the first point that we're painting, add this location to the list of brush stamps and use it
            // as the starting point.
            if (isFirstPaintedPoint)
            {
                brushStamps.emplace_back(currentCenter2D);
                m_lastBrushCenter = currentCenter2D;
                m_distanceSinceLastDraw = 0.0f;
            }

            // Get the direction that we've moved the mouse since the last mouse movement we handled.
            AZ::Vector2 direction = (currentCenter2D - m_lastBrushCenter).GetNormalizedSafe();

            // Get the total distance that we've moved since the last time we drew a brush stamp (which might
            // have been many small mouse movements ago).
            float totalDistance = m_lastBrushCenter.GetDistance(currentCenter2D) + m_distanceSinceLastDraw;

            // Keep adding brush stamps to the list for as long as the total remaining mouse distance is
            // greater than the stamp distance. If the mouse moved a large enough distance in one frame,
            // this will add multiple stamps. If the mouse moved a tiny amount, it's possible that no stamps
            // will get added, and we'll just save the accumulated distance for next frame.
            for (; totalDistance >= distanceBetweenBrushStamps; totalDistance -= distanceBetweenBrushStamps)
            {
                // Add another stamp to the list to draw this time.
                AZ::Vector2 stampCenter = m_lastBrushCenter + direction * (distanceBetweenBrushStamps - m_distanceSinceLastDraw);
                brushStamps.emplace_back(stampCenter);

                // Reset our tracking so that our next stamp location will be based off of this one.
                m_distanceSinceLastDraw = 0.0f;
                m_lastBrushCenter = stampCenter;
            }

            // If we have any distance remaining that we haven't used, keep it for next time.
            // Note that totalDistance already includes the previous m_distanceSinceLastDraw, so we just replace it with our
            // leftovers here, we don't add them.
            m_distanceSinceLastDraw = totalDistance;

            // Save the current location as the last one we processed.
            m_lastBrushCenter = currentCenter2D;

            // If we don't have any stamps on this mouse movement, then we're done.
            if (brushStamps.empty())
            {
                return;
            }

            const float radius = currentSettings.GetSize() / 2.0f;

            // Create an AABB that contains every brush stamp.
            AZ::Aabb strokeRegion = AZ::Aabb::CreateNull(); 
            for (auto& brushStamp : brushStamps)
            {
                strokeRegion.AddAabb(AZ::Aabb::CreateCenterRadius(AZ::Vector3(brushStamp, 0.0f), radius));
            }

            const float manipulatorRadiusSq = radius * radius;

            // Callback function that we pass into OnPaint so that paint handling code can request specific paint values
            // for the world positions it cares about.
            PaintBrushNotifications::ValueLookupFn valueLookupFn(
                [radius, manipulatorRadiusSq, &brushStamps, hardness, flow](
                AZStd::span<const AZ::Vector3> points,
                AZStd::vector<AZ::Vector3>& validPoints, AZStd::vector<float>& opacities)
            {
                validPoints.clear();
                opacities.clear();

                validPoints.reserve(points.size());
                opacities.reserve(points.size());

                // Calculate 1/(1 - hardness) once to use for all points. If hardness is 1, set this to 0 instead of 1/0.
                const float inverseHardnessReciprocal = (hardness < 1.0f) ? (1.0f / (1.0f - hardness)) : 0.0f;

                // Loop through every point that's been provided and see if it has a valid paint opacity.
                for (size_t index = 0; index < points.size(); index++)
                {
                    float opacity = 0.0f;
                    AZ::Vector2 point2D(points[index]);

                    // Loop through each stamp that we're drawing and accumulate the results for this point.
                    for (auto& brushCenter : brushStamps)
                    {
                        // Since each stamp is a circle, we can just compare distance to the center of the circle vs radius.
                        if (float shortestDistanceSquared = brushCenter.GetDistanceSq(point2D);
                            shortestDistanceSquared <= manipulatorRadiusSq)
                        {
                            // It's a valid point, so calculate the opacity. The per-point opacity for a paint stamp is a combination
                            // of the hardness falloff and the flow. The flow value gives the overall opacity for each stamp, and the
                            // hardness falloff gives per-pixel opacity within the stamp.

                            // Normalize the distance into the 0-1 range, where 0 is the center of the stamp, and 1 is the edge.
                            float shortestDistanceNormalized = AZStd::sqrt(shortestDistanceSquared) / radius;

                            // The hardness parameter describes what % of the total distance gets the falloff curve.
                            // i.e. hardness=0.25 means that distances < 0.25 will have no falloff, and the falloff curve will
                            // be mapped to distances of 0.25 to 1.
                            // This scaling basically just uses the ratio "(dist - hardness) / (1 - hardness)" and clamps the
                            // minimum to 0, so our output hardnessDistance is the 0 to 1 number that we input into the falloff function.
                            float hardnessDistance = AZStd::max(shortestDistanceNormalized - hardness, 0.0f) * inverseHardnessReciprocal;

                            // For the falloff function itself, we use a nonlinear falloff that's approximately the same
                            // as a squared cosine: 2x^3 - 3x^2 + 1 . This produces a nice backwards S curve that starts at 1, ends at 0,
                            // and has a midpoint at (0.5, 0.5).
                            float perPixelOpacity = ((hardnessDistance * hardnessDistance) * (2.0f * hardnessDistance - 3.0f)) + 1.0f;

                            // For the opacity at this point, combine any opacity from previous stamps with the
                            // currently-computed perPixelOpacity and flow.
                            opacity = AZStd::min(opacity + (1.0f - opacity) * (perPixelOpacity * flow), 1.0f);
                        }
                    }

                    // As long as our opacity isn't transparent, return this as a valid point and opacity.
                    if (opacity > 0.0f)
                    {
                        validPoints.emplace_back(points[index]);
                        opacities.emplace_back(opacity);
                    }
                }
            });

            // Set the blending operation based on the current paintbrush blend mode setting.
            PaintBrushNotifications::BlendFn blendFn;
            switch (currentSettings.GetBlendMode())
            {
                case PaintBrushBlendMode::Normal:
                    blendFn = [](float baseValue, float intensity, float opacity)
                    {
                        const float operationResult = intensity;
                        return AZStd::clamp(AZStd::lerp(baseValue, operationResult, opacity), 0.0f, 1.0f);
                    };
                    break;
                case PaintBrushBlendMode::Add:
                    blendFn = [](float baseValue, float intensity, float opacity)
                    {
                        const float operationResult = baseValue + intensity;
                        return AZStd::clamp(AZStd::lerp(baseValue, operationResult, opacity), 0.0f, 1.0f);
                    };
                    break;
                case PaintBrushBlendMode::Subtract:
                    blendFn = [](float baseValue, float intensity, float opacity)
                    {
                        const float operationResult = baseValue - intensity;
                        return AZStd::clamp(AZStd::lerp(baseValue, operationResult, opacity), 0.0f, 1.0f);
                    };
                    break;
                case PaintBrushBlendMode::Multiply:
                    blendFn = [](float baseValue, float intensity, float opacity)
                    {
                        const float operationResult = baseValue * intensity;
                        return AZStd::clamp(AZStd::lerp(baseValue, operationResult, opacity), 0.0f, 1.0f);
                    };
                    break;
                case PaintBrushBlendMode::Screen:
                    blendFn = [](float baseValue, float intensity, float opacity)
                    {
                        const float operationResult = 1.0f - ((1.0f - baseValue) * (1.0f - intensity));
                        return AZStd::clamp(AZStd::lerp(baseValue, operationResult, opacity), 0.0f, 1.0f);
                    };
                    break;
                case PaintBrushBlendMode::Darken:
                    blendFn = [](float baseValue, float intensity, float opacity)
                    {
                        const float operationResult = AZStd::min(baseValue, intensity);
                        return AZStd::clamp(AZStd::lerp(baseValue, operationResult, opacity), 0.0f, 1.0f);
                    };
                    break;
                case PaintBrushBlendMode::Lighten:
                    blendFn = [](float baseValue, float intensity, float opacity)
                    {
                        const float operationResult = AZStd::max(baseValue, intensity);
                        return AZStd::clamp(AZStd::lerp(baseValue, operationResult, opacity), 0.0f, 1.0f);
                    };
                    break;
                case PaintBrushBlendMode::Average:
                    blendFn = [](float baseValue, float intensity, float opacity)
                    {
                        const float operationResult = (baseValue + intensity) / 2.0f;
                        return AZStd::clamp(AZStd::lerp(baseValue, operationResult, opacity), 0.0f, 1.0f);
                    };
                    break;
                case PaintBrushBlendMode::Overlay:
                    blendFn = [](float baseValue, float intensity, float opacity)
                    {
                        const float operationResult = (baseValue >= 0.5f) ? (1.0f - (2.0f * (1.0f - baseValue) * (1.0f - intensity)))
                                                                          : (2.0f * baseValue * intensity);
                        return AZStd::clamp(AZStd::lerp(baseValue, operationResult, opacity), 0.0f, 1.0f);
                    };
                    break;
                default:
                    AZ_Assert(false, "Unknown PaintBrushBlendMode type: %u", currentSettings.GetBlendMode());
                    break;
            }

            // Trigger the OnPaint notification, and provider the listener with the valueLookupFn to find out the paint brush
            // values at specific world positions, and the blendFn to perform blending operations based on the provided base and
            // paint brush values.
            PaintBrushNotificationBus::Event(
                m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnPaint, strokeRegion, valueLookupFn, blendFn);
        }
    }

    AZStd::vector<AzToolsFramework::ActionOverride> PaintBrushManipulator::PopulateActionsImpl()
    {
        // Paint brush manipulators should be able to easily adjust the radius of the brush with the [ and ] keys
        return {
            AzToolsFramework::ActionOverride()
                .SetUri(PaintbrushIncreaseSize)
                .SetKeySequence(QKeySequence{ Qt::Key_BracketRight })
                .SetTitle(PaintbrushIncreaseSizeTitle)
                .SetTip(PaintbrushIncreaseSizeDesc)
                .SetEntityComponentIdPair(m_ownerEntityComponentId)
                .SetCallback(
                    [this]()
                    {
                        AdjustSize(ed_paintBrushSizeAdjustAmount);
                    }),
            AzToolsFramework::ActionOverride()
                .SetUri(PaintbrushDecreaseSize)
                .SetKeySequence(QKeySequence{ Qt::Key_BracketLeft })
                .SetTitle(PaintbrushDecreaseSizeTitle)
                .SetTip(PaintbrushDecreaseSizeDesc)
                .SetEntityComponentIdPair(m_ownerEntityComponentId)
                .SetCallback(
                    [this]()
                    {
                        AdjustSize(-ed_paintBrushSizeAdjustAmount);
                    }),
        };
    }

    void PaintBrushManipulator::AdjustSize(float sizeDelta)
    {
        float diameter = 0.0f;
        PaintBrushSettingsRequestBus::BroadcastResult(diameter, &PaintBrushSettingsRequestBus::Events::GetSize);
        diameter = AZStd::clamp(diameter + sizeDelta, 0.0f, 1024.0f);
        PaintBrushSettingsRequestBus::Broadcast(&PaintBrushSettingsRequestBus::Events::SetSize, diameter);
    }

    void PaintBrushManipulator::AdjustIntensityPercent(float intensityPercentDelta)
    {
        float intensity = 0.0f;
        PaintBrushSettingsRequestBus::BroadcastResult(intensity, &PaintBrushSettingsRequestBus::Events::GetIntensityPercent);
        intensity = AZStd::clamp(intensity + intensityPercentDelta, 0.0f, 100.0f);
        PaintBrushSettingsRequestBus::Broadcast(&PaintBrushSettingsRequestBus::Events::SetIntensityPercent, intensity);
    }

    void PaintBrushManipulator::AdjustOpacityPercent(float opacityPercentDelta)
    {
        float opacity = 0.0f;
        PaintBrushSettingsRequestBus::BroadcastResult(opacity, &PaintBrushSettingsRequestBus::Events::GetOpacityPercent);
        opacity = AZStd::clamp(opacity + opacityPercentDelta, 0.0f, 100.0f);
        PaintBrushSettingsRequestBus::Broadcast(&PaintBrushSettingsRequestBus::Events::SetOpacityPercent, opacity);
    }

} // namespace AzToolsFramework
