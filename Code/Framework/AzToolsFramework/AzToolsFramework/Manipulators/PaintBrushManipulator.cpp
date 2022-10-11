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
                float intensity = 0.0f;
                float opacity = 0.0f;

                PaintBrushSettingsRequestBus::BroadcastResult(intensity, &PaintBrushSettingsRequestBus::Events::GetIntensity);
                PaintBrushSettingsRequestBus::BroadcastResult(opacity, &PaintBrushSettingsRequestBus::Events::GetOpacity);

                // Notify that a paint stroke has begun, and provide the stroke intensity and opacity.
                m_isPainting = true;
                PaintBrushNotificationBus::Event(
                    m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnPaintBegin, intensity, opacity);

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
                m_isPainting = false;
                PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnPaintEnd);

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

            float diameter = 0.0f;
            float opacity = 0.0f;
            float hardness = 0.0f;
            float flow = 0.0f;
            float distancePercent = 0.0f;
            PaintBrushBlendMode blendMode = PaintBrushBlendMode::Normal;

            PaintBrushSettingsRequestBus::BroadcastResult(diameter, &PaintBrushSettingsRequestBus::Events::GetSize);
            PaintBrushSettingsRequestBus::BroadcastResult(opacity, &PaintBrushSettingsRequestBus::Events::GetOpacity);
            PaintBrushSettingsRequestBus::BroadcastResult(hardness, &PaintBrushSettingsRequestBus::Events::GetHardness);
            PaintBrushSettingsRequestBus::BroadcastResult(flow, &PaintBrushSettingsRequestBus::Events::GetFlow);
            PaintBrushSettingsRequestBus::BroadcastResult(distancePercent, &PaintBrushSettingsRequestBus::Events::GetDistancePercent);
            PaintBrushSettingsRequestBus::BroadcastResult(blendMode, &PaintBrushSettingsRequestBus::Events::GetBlendMode);

            // Early out if we're completely transparent or if our brush daubs aren't supposed to move.
            if ((opacity == 0.0f) || (flow == 0.0f) || (distancePercent == 0.0f))
            {
                return;
            }

            // Get the distance between each brush daub in world space.
            const float distanceBetweenBrushDaubs = diameter * distancePercent;

            // Track the list of center points for each brush daub to draw for this mouse movement.
            AZStd::vector<AZ::Vector2> brushDaubs;

            // If this is the first point that we're painting, add this location to the list of brush daubs and use it
            // as the starting point.
            if (isFirstPaintedPoint)
            {
                brushDaubs.emplace_back(currentCenter2D);
                m_lastBrushCenter = currentCenter2D;
                m_distanceSinceLastDraw = 0.0f;
            }

            // Get the direction that we've moved the mouse since the last mouse movement we handled.
            AZ::Vector2 direction = (currentCenter2D - m_lastBrushCenter).GetNormalized();

            // Get the total distance that we've moved since the last time we drew a brush daub (which might
            // have been many small mouse movements ago).
            float totalDistance = m_lastBrushCenter.GetDistance(currentCenter2D) + m_distanceSinceLastDraw;

            // Find the location for each brush daub that we can draw based on the total distance the mouse moved since
            // the last time we drew a daub.
            for (; totalDistance >= distanceBetweenBrushDaubs; totalDistance -= distanceBetweenBrushDaubs)
            {
                // Add another daub to the list to draw this time.
                AZ::Vector2 daubCenter = m_lastBrushCenter + direction * (distanceBetweenBrushDaubs - m_distanceSinceLastDraw);
                brushDaubs.emplace_back(daubCenter);

                // Reset our tracking so that our next daub location will be based off of this one.
                m_distanceSinceLastDraw = 0.0f;
                m_lastBrushCenter = daubCenter;
            }

            // If we have any distance remaining that we haven't used, keep it for next time.
            // Note that totalDistance already includes the previous m_distanceSinceLastDraw, so we just replace it with our
            // leftovers here, we don't add them.
            m_distanceSinceLastDraw = totalDistance;

            // Save the current location as the last one we processed.
            m_lastBrushCenter = currentCenter2D;

            // If we don't have any daubs on this mouse movement, then we're done.
            if (brushDaubs.empty())
            {
                return;
            }

            const float radius = diameter / 2.0f;

            // Create an AABB that contains every brush daub.
            AZ::Aabb strokeRegion = AZ::Aabb::CreateNull(); 
            for (auto& brushDaub : brushDaubs)
            {
                strokeRegion.AddAabb(AZ::Aabb::CreateCenterRadius(AZ::Vector3(brushDaub, 0.0f), radius));
            }

            const float manipulatorRadiusSq = radius * radius;

            // Callback function that we pass into OnPaint so that paint handling code can request specific paint values
            // for the world positions it cares about.
            PaintBrushNotifications::ValueLookupFn valueLookupFn(
                [radius, manipulatorRadiusSq, &brushDaubs, hardness, flow](
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

                    // Loop through each daub that we're drawing and accumulate the results for this point.
                    for (auto& brushCenter : brushDaubs)
                    {
                        // Since each daub is a circle, we can just compare distance to the center of the circle vs radius.
                        if (float shortestDistanceSquared = brushCenter.GetDistanceSq(point2D);
                            shortestDistanceSquared <= manipulatorRadiusSq)
                        {
                            // It's a valid point, so calculate the opacity. The per-point opacity for a paint daub is a combination
                            // of the hardness falloff and the flow. The flow value gives the overall opacity for each daub, and the
                            // hardness falloff gives per-pixel opacity within the daub.
                            // For the falloff function, we use a nonlinear falloff that's approximately the same as a squared cosine:
                            // 2x^3 - 3x^2 + 1
                            float shortestDistanceNormalized = sqrt(shortestDistanceSquared) / radius;
                            float hardnessDistance = AZStd::max(shortestDistanceNormalized - hardness, 0.0f) * inverseHardnessReciprocal;
                            float curHardness = (2.0f * hardnessDistance * hardnessDistance * hardnessDistance) -
                                (3.0f * hardnessDistance * hardnessDistance) + 1.0f;

                            // For the opacity at this point, combine the opacity from previous daubs with the current hardness and flow.
                            opacity = AZStd::min(opacity + (curHardness * flow), 1.0f);
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
            switch (blendMode)
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
                    AZ_Assert(false, "Unknown PaintBrushBlendMode type: %u", blendMode);
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
        diameter = AZStd::clamp(diameter + sizeDelta, 0.01f, 1024.0f);
        PaintBrushSettingsRequestBus::Broadcast(&PaintBrushSettingsRequestBus::Events::SetSize, diameter);
    }

    void PaintBrushManipulator::AdjustIntensity(float intensityDelta)
    {
        float intensity = 0.0f;
        PaintBrushSettingsRequestBus::BroadcastResult(intensity, &PaintBrushSettingsRequestBus::Events::GetIntensity);
        intensity = AZStd::clamp(intensity + intensityDelta, 0.0f, 1.0f);
        PaintBrushSettingsRequestBus::Broadcast(&PaintBrushSettingsRequestBus::Events::SetIntensity, intensity);
    }

    void PaintBrushManipulator::AdjustOpacity(float opacityDelta)
    {
        float opacity = 0.0f;
        PaintBrushSettingsRequestBus::BroadcastResult(opacity, &PaintBrushSettingsRequestBus::Events::GetOpacity);
        opacity = AZStd::clamp(opacity + opacityDelta, 0.0f, 1.0f);
        PaintBrushSettingsRequestBus::Broadcast(&PaintBrushSettingsRequestBus::Events::SetOpacity, opacity);
    }

} // namespace AzToolsFramework
