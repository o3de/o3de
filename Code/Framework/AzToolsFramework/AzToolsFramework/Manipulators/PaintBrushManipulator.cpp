/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Geometry2DUtils.h>
#include <AzCore/std/sort.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>
#include <AzToolsFramework/Manipulators/PaintBrushNotificationBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
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

    AZ_CVAR(
        float,
        ed_paintBrushHardnessPercentAdjustAmount,
        5.0f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The amount to increase / decrease the paintbrush hardness in percent (0 - 100).");

    namespace
    {
        static constexpr AZ::Crc32 PaintbrushIncreaseSize = AZ_CRC_CE("org.o3de.action.paintbrush.increase_size");
        static constexpr AZ::Crc32 PaintbrushDecreaseSize = AZ_CRC_CE("org.o3de.action.paintbrush.decrease_size");
        static constexpr AZ::Crc32 PaintbrushIncreaseHardness = AZ_CRC_CE("org.o3de.action.paintbrush.increase_hardness_percent");
        static constexpr AZ::Crc32 PaintbrushDecreaseHardness = AZ_CRC_CE("org.o3de.action.paintbrush.decrease_hardness_percent");

        static constexpr const char* PaintbrushIncreaseSizeTitle = "Increase Size";
        static constexpr const char* PaintbrushDecreaseSizeTitle = "Decrease Size";
        static constexpr const char* PaintbrushIncreaseHardnessTitle = "Increase Hardness Percent";
        static constexpr const char* PaintbrushDecreaseHardnessTitle = "Decrease Hardness Percent";

        static constexpr const char* PaintbrushIncreaseSizeDesc = "Increases size of paintbrush";
        static constexpr const char* PaintbrushDecreaseSizeDesc = "Decreases size of paintbrush";
        static constexpr const char* PaintbrushIncreaseHardnessDesc = "Increases paintbrush hardness";
        static constexpr const char* PaintbrushDecreaseHardnessDesc = "Decreases paintbrush hardness";
    } // namespace

    AZStd::shared_ptr<PaintBrushManipulator> PaintBrushManipulator::MakeShared(
        const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair, PaintBrushColorMode colorMode)
    {
        return AZStd::shared_ptr<PaintBrushManipulator>(aznew PaintBrushManipulator(worldFromLocal, entityComponentIdPair, colorMode));
    }

    PaintBrushManipulator::PaintBrushManipulator(
        const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair, PaintBrushColorMode colorMode)
    {
        m_ownerEntityComponentId = entityComponentIdPair;

        SetSpace(worldFromLocal);

        // Make sure the Paint Brush Settings window is open
        AzToolsFramework::OpenViewPane(PaintBrush::s_paintBrushSettingsName);

        // Set the paint brush settings to use the requested color mode.
        PaintBrushSettingsRequestBus::Broadcast(&PaintBrushSettingsRequestBus::Events::SetBrushColorMode, colorMode);

        // Get the global Paint Brush Settings so that we can calculate our brush circle sizes.
        PaintBrushSettings brushSettings;
        PaintBrushSettingsRequestBus::BroadcastResult(brushSettings, &PaintBrushSettingsRequestBus::Events::GetSettings);

        const auto [innerRadius, outerRadius] = GetBrushRadii(brushSettings);

        // The PaintBrush manipulator uses two circles projected into world space to represent the brush.
        const AZ::Color innerCircleColor = AZ::Colors::Red;
        const AZ::Color outerCircleColor = AZ::Colors::Red;
        const float circleWidth = 0.05f;
        SetView(
            AzToolsFramework::CreateManipulatorViewProjectedCircle(*this, innerCircleColor, innerRadius, circleWidth),
            AzToolsFramework::CreateManipulatorViewProjectedCircle(*this, outerCircleColor, outerRadius, circleWidth));

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

    AZStd::pair<float, float> PaintBrushManipulator::GetBrushRadii(const PaintBrushSettings& settings) const
    {
        const float outerRadius = settings.GetSize() / 2.0f;

        if (settings.GetBrushMode() == PaintBrushMode::Eyedropper)
        {
            // For the eyedropper, we'll set the inner radius to an arbitrarily small percentage of the full brush size to help
            // visualize that we're only picking from the very center of the brush.
            const float eyedropperRadius = outerRadius * 0.02f;
            return { eyedropperRadius, outerRadius };
        }

        // For paint/smooth brushes, the inner circle represents the start of the hardness falloff,
        // and the outer circle is the full paintbrush size.
        const float hardnessRadius = outerRadius * (settings.GetHardnessPercent() / 100.0f);
        return { hardnessRadius, outerRadius };
    }


    void PaintBrushManipulator::Draw(
        const ManipulatorManagerState& managerState, AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState, const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        // Always set our manipulator state to say that the mouse isn't over the manipulator so that we always use our base
        // manipulator color. The paintbrush isn't a "selectable" manipulator, so it wouldn't make sense for it to change color when the
        // mouse is over it.
        constexpr bool mouseOver = false;

        m_innerCircle->Draw(
            GetManipulatorManagerId(), managerState, GetManipulatorId(),
            { GetSpace(), GetNonUniformScale(), AZ::Vector3::CreateZero(), mouseOver }, debugDisplay, cameraState,
            mouseInteraction);

        m_outerCircle->Draw(
            GetManipulatorManagerId(), managerState, GetManipulatorId(),
            { GetSpace(), GetNonUniformScale(), AZ::Vector3::CreateZero(), mouseOver }, debugDisplay, cameraState,
            mouseInteraction);
    }

    void PaintBrushManipulator::SetView(
        AZStd::shared_ptr<ManipulatorViewProjectedCircle> innerCircle, AZStd::shared_ptr<ManipulatorViewProjectedCircle> outerCircle)
    {
        m_innerCircle = AZStd::move(innerCircle);
        m_outerCircle = AZStd::move(outerCircle);
    }

    void PaintBrushManipulator::OnSettingsChanged(const PaintBrushSettings& newSettings)
    {
        const auto [innerRadius, outerRadius] = GetBrushRadii(newSettings);

        m_innerCircle->SetRadius(innerRadius);
        m_outerCircle->SetRadius(outerRadius);
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
                // Get the color, intensity and opacity to use for this brush stroke.
                AZ::Color color = AZ::Color::CreateZero();

                PaintBrushSettingsRequestBus::BroadcastResult(color, &PaintBrushSettingsRequestBus::Events::GetColor);

                // Notify that a paint stroke has begun, and provide the paint color including opacity.
                m_isInBrushStroke = true;
                PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnPaintStrokeBegin, color);

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
                if (m_isInBrushStroke)
                {
                    m_isInBrushStroke = false;
                    PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnPaintStrokeEnd);
                }

                return true;
            }
        }

        return false;
    }

    void PaintBrushManipulator::MovePaintBrush(
        int viewportId, const AzFramework::ScreenPoint& screenCoordinates, bool isFirstBrushStrokePoint)
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

        // If we're currently performing a brush stroke, then trigger the appropriate brush action.
        if (m_isInBrushStroke)
        {
            // Get our current paint brush settings.
            PaintBrushSettings brushSettings;
            PaintBrushSettingsRequestBus::BroadcastResult(brushSettings, &PaintBrushSettingsRequestBus::Events::GetSettings);

            switch (brushSettings.GetBrushMode())
            {
            case PaintBrushMode::Paintbrush:
                PerformPaintAction(brushCenter, brushSettings, isFirstBrushStrokePoint);
                break;
            case PaintBrushMode::Smooth:
                PerformSmoothAction(brushCenter, brushSettings, isFirstBrushStrokePoint);
                break;
            case PaintBrushMode::Eyedropper:
                PerformEyedropperAction(brushCenter, brushSettings);
                break;
            default:
                AZ_Assert(false, "Unsupported brush mode type: %u", brushSettings.GetBrushMode());
                break;
            }
        }
    }

    void PaintBrushManipulator::CalculateBrushStampCentersAndStrokeRegion(
        const AZ::Vector3& brushCenter,
        const PaintBrushSettings& brushSettings,
        bool isFirstBrushStrokePoint,
        AZStd::vector<AZ::Vector2>& brushStampCenters,
        AZ::Aabb& strokeRegion)
    {
        AZ::Vector2 currentCenter2D(brushCenter);

        brushStampCenters.clear();
        strokeRegion = AZ::Aabb::CreateNull();

        // Early out if we're completely transparent, there's no distance between brush stamps, or the brush stamp size is 0.
        if ((brushSettings.GetColor().GetA() == 0.0f) || (brushSettings.GetFlowPercent() == 0.0f) ||
            (brushSettings.GetDistancePercent() == 0.0f) || (brushSettings.GetSize() == 0.0f))
        {
            return;
        }

        // Get the distance between each brush stamp in world space.
        const float distanceBetweenBrushStamps = brushSettings.GetSize() * (brushSettings.GetDistancePercent() / 100.0f);

        // If this is the first point that we're painting, add this location to the list of brush stamps and use it
        // as the starting point.
        if (isFirstBrushStrokePoint)
        {
            brushStampCenters.emplace_back(currentCenter2D);
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
            brushStampCenters.emplace_back(stampCenter);

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
        if (brushStampCenters.empty())
        {
            return;
        }

        const float radius = brushSettings.GetSize() / 2.0f;

        // Create an AABB that contains every brush stamp.
        for (auto& brushStamp : brushStampCenters)
        {
            strokeRegion.AddAabb(AZ::Aabb::CreateCenterRadius(AZ::Vector3(brushStamp, 0.0f), radius));
        }
    }

    void PaintBrushManipulator::CalculatePointsInBrush(
        const PaintBrushSettings& brushSettings,
        const AZStd::vector<AZ::Vector2>& brushStampCenters,
        AZStd::span<const AZ::Vector3> points,
        AZStd::vector<AZ::Vector3>& validPoints,
        AZStd::vector<float>& opacities)
    {
        validPoints.clear();
        opacities.clear();

        validPoints.reserve(points.size());
        opacities.reserve(points.size());

        // Convert our settings into the 0-1 range
        const float hardness = brushSettings.GetHardnessPercent() / 100.0f;
        const float flow = brushSettings.GetFlowPercent() / 100.0f;

        const float radius = brushSettings.GetSize() / 2.0f;
        const float manipulatorRadiusSq = radius * radius;

        // Calculate 1/(1 - hardness) once to use for all points. If hardness is 1, set this to 0 instead of 1/0.
        const float inverseHardnessReciprocal = (hardness < 1.0f) ? (1.0f / (1.0f - hardness)) : 0.0f;

        // Loop through every point that's been provided and see if it has a valid paint opacity.
        for (size_t index = 0; index < points.size(); index++)
        {
            float opacity = 0.0f;
            AZ::Vector2 point2D(points[index]);

            // Loop through each stamp that we're drawing and accumulate the results for this point.
            for (auto& brushCenter : brushStampCenters)
            {
                // Since each stamp is a circle, we can just compare distance to the center of the circle vs radius.
                if (float shortestDistanceSquared = brushCenter.GetDistanceSq(point2D); shortestDistanceSquared <= manipulatorRadiusSq)
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
    }

    PaintBrushNotifications::BlendFn PaintBrushManipulator::GetBlendFunction(const PaintBrushSettings& brushSettings)
    {
        auto clampAndLerpFn = [](float baseValue, float newValue, float opacity)
        {
            return AZStd::clamp(AZStd::lerp(baseValue, newValue, opacity), 0.0f, 1.0f);
        };

        switch (brushSettings.GetBlendMode())
        {
        case PaintBrushBlendMode::Normal:
            return [&clampAndLerpFn](float baseValue, float intensity, float opacity)
            {
                return clampAndLerpFn(baseValue, intensity, opacity);
            };
            break;
        case PaintBrushBlendMode::Add:
            return [&clampAndLerpFn](float baseValue, float intensity, float opacity)
            {
                return clampAndLerpFn(baseValue, baseValue + intensity, opacity);
            };
            break;
        case PaintBrushBlendMode::Subtract:
            return [&clampAndLerpFn](float baseValue, float intensity, float opacity)
            {
                return clampAndLerpFn(baseValue, baseValue - intensity, opacity);
            };
            break;
        case PaintBrushBlendMode::Multiply:
            return [&clampAndLerpFn](float baseValue, float intensity, float opacity)
            {
                return clampAndLerpFn(baseValue, baseValue * intensity, opacity);
            };
            break;
        case PaintBrushBlendMode::Screen:
            return [&clampAndLerpFn](float baseValue, float intensity, float opacity)
            {
                const float operationResult = 1.0f - ((1.0f - baseValue) * (1.0f - intensity));
                return clampAndLerpFn(baseValue, operationResult, opacity);
            };
            break;
        case PaintBrushBlendMode::Darken:
            return [&clampAndLerpFn](float baseValue, float intensity, float opacity)
            {
                return clampAndLerpFn(baseValue, AZStd::min(baseValue, intensity), opacity);
            };
            break;
        case PaintBrushBlendMode::Lighten:
            return [&clampAndLerpFn](float baseValue, float intensity, float opacity)
            {
                return clampAndLerpFn(baseValue, AZStd::max(baseValue, intensity), opacity);
            };
            break;
        case PaintBrushBlendMode::Average:
            return [&clampAndLerpFn](float baseValue, float intensity, float opacity)
            {
                return clampAndLerpFn(baseValue, (baseValue + intensity) / 2.0f, opacity);
            };
            break;
        case PaintBrushBlendMode::Overlay:
            return [&clampAndLerpFn](float baseValue, float intensity, float opacity)
            {
                const float operationResult =
                    (baseValue >= 0.5f) ? (1.0f - (2.0f * (1.0f - baseValue) * (1.0f - intensity))) : (2.0f * baseValue * intensity);
                return clampAndLerpFn(baseValue, operationResult, opacity);
            };
            break;
        default:
            AZ_Assert(false, "Unknown PaintBrushBlendMode type: %u", brushSettings.GetBlendMode());
            break;
        }

        return {};
    }


    void PaintBrushManipulator::PerformPaintAction(
        const AZ::Vector3& brushCenter, const PaintBrushSettings& brushSettings, bool isFirstBrushStrokePoint)
    {
        // Track the list of center points for each brush stamp to draw for this mouse movement and the AABB around the stamps.
        AZStd::vector<AZ::Vector2> brushStampCenters;
        AZ::Aabb strokeRegion = AZ::Aabb::CreateNull(); 

        CalculateBrushStampCentersAndStrokeRegion(brushCenter, brushSettings, isFirstBrushStrokePoint, brushStampCenters, strokeRegion);

        // If we don't have any stamps on this mouse movement, then we're done.
        if (brushStampCenters.empty())
        {
            return;
        }

        // Callback function that we pass into OnPaint so that paint handling code can request specific paint values
        // for the world positions it cares about.
        PaintBrushNotifications::ValueLookupFn valueLookupFn(
            [&brushStampCenters, &brushSettings](
            AZStd::span<const AZ::Vector3> points,
            AZStd::vector<AZ::Vector3>& validPoints, AZStd::vector<float>& opacities)
        {
            CalculatePointsInBrush(brushSettings, brushStampCenters, points, validPoints, opacities);
        });

        // Set the blending operation based on the current paintbrush blend mode setting.
        PaintBrushNotifications::BlendFn blendFn = GetBlendFunction(brushSettings);

        // Trigger the OnPaint notification, provide the listener with the valueLookupFn to find out the paint brush
        // values at specific world positions, and provide the blendFn to perform blending operations based on the provided base and
        // paint brush values.
        PaintBrushNotificationBus::Event(
            m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnPaint, strokeRegion, valueLookupFn, blendFn);
    }

    void PaintBrushManipulator::PerformEyedropperAction(
        const AZ::Vector3& brushCenter, const PaintBrushSettings& brushSettings)
    {
        AZ::Color brushColor = brushSettings.GetColor();

        // Trigger the OnGetColor notification to get the current color at the given point.
        PaintBrushNotificationBus::EventResult(brushColor,
            m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnGetColor, brushCenter);

        // Set the color in our paintbrush settings to the color selected by the eyedropper.
        PaintBrushSettingsRequestBus::Broadcast(&PaintBrushSettingsRequestBus::Events::SetColor, brushColor);
    }

    AZStd::vector<float> PaintBrushManipulator::CalculateGaussianWeights(size_t smoothingRadius)
    {
        // The Gaussian function is G(x, y) = (1 / 2*pi*sigma^2) * e^(-(x^2 + y^2) / (2*sigma^2)).
        // This function produces a bell curve symmetric around the origin, where (1 / 2*pi*sigma^2) is the height of the peak,
        // and sigma (the standard deviation) controls the width of the peak.
        // x and y are the relative distances from the center.

        // This will contain all of the Gaussian weights that we compute and return.
        AZStd::vector<float> gaussianWeights;

        // We track the sum of all the weights so that we can normalize them at the end.
        float sum = 0.0f;

        // Keep a floating-point version of this so that we don't have to do int/float conversions in our loops below.
        const float radius = aznumeric_cast<float>(smoothingRadius);

        // In practice, only pixels within a distance of 3 * sigma affect the result, so we'll pick a sigma that's 1/3 of the distance
        // in each direction.
        const float sigma = AZStd::max(radius / 3.0f, 1.0f);

        // We can precompute the peakHeight and expConstant terms outside of the loop.
        const float peakHeight = (1.0f / (AZ::Constants::TwoPi * sigma * sigma));
        const float expConstant = -1.0f / (2.0f * sigma * sigma);

        // Preallocate space for all our weights.
        size_t kernelSize = (smoothingRadius * 2) + 1;
        gaussianWeights.reserve(kernelSize * kernelSize);

        // Compute the Gaussian weights for each point spreading outward from the origin.
        // The order in which we compute this is important, because we'll expect the pixel values that we're smoothing to have been
        // gathered in the same order.
        for (float y = -radius; y <= radius; y++)
        {
            const float ySquared = y * y;

            for (float x = -radius; x <= radius; x++)
            {
                float kernelValue = peakHeight * AZStd::exp(expConstant * ((x * x) + ySquared));
                sum += kernelValue;

                gaussianWeights.emplace_back(kernelValue);
            }
        }

        // Normalize all the values.
        AZStd::transform(gaussianWeights.begin(), gaussianWeights.end(), gaussianWeights.begin(),
            [sum](float value)
            {
                return value / sum;
            });

        return gaussianWeights;
    }

    void PaintBrushManipulator::PerformSmoothAction(
        const AZ::Vector3& brushCenter, const PaintBrushSettings& brushSettings, bool isFirstBrushStrokePoint)
    {
        // Track the list of center points for each brush stamp to draw for this mouse movement and the AABB around the stamps.
        AZStd::vector<AZ::Vector2> brushStampCenters;
        AZ::Aabb strokeRegion = AZ::Aabb::CreateNull();

        CalculateBrushStampCentersAndStrokeRegion(brushCenter, brushSettings, isFirstBrushStrokePoint, brushStampCenters, strokeRegion);

        // If we don't have any stamps on this mouse movement, then we're done.
        if (brushStampCenters.empty())
        {
            return;
        }

        // Callback function that we pass into OnSmooth so that smoothing code can request specific brush values
        // for the world positions it cares about.
        PaintBrushNotifications::ValueLookupFn valueLookupFn(
            [&brushStampCenters, &brushSettings](
                AZStd::span<const AZ::Vector3> points, AZStd::vector<AZ::Vector3>& validPoints, AZStd::vector<float>& opacities)
            {
                CalculatePointsInBrush(brushSettings, brushStampCenters, points, validPoints, opacities);
            });


        // The smoothing radius tells us the number of pixels in each direction to use,
        // so the kernelSize (the N x N size for the box of pixels) is (radius + center pixel + radius) in size.
        size_t smoothingRadius = brushSettings.GetSmoothingRadius();
        size_t kernelSize = (smoothingRadius * 2) + 1;

        // Even though this is only used by the Gaussian smoothing mode, it's declared outside the switch statement
        // so that it will have a lifetime that lasts throughout the OnSmooth call.
        AZStd::vector<float> gaussianWeights;

        // Using our brush settings, compute the relative world-space offsets for each value that we'll want to fetch.
        AZStd::vector<AZ::Vector3> valuePointOffsets;
        valuePointOffsets.reserve(kernelSize * kernelSize);

        // Our smoothing spacing setting describes how many values to skip. The "+ 1" is so that we're instead describing
        // how much to increment our location for each value.
        const float spacing = aznumeric_cast<float>(brushSettings.GetSmoothingSpacing()) + 1.0f;

        // This describes our spaced-out range of relative positions from the center value.
        const float range = aznumeric_cast<float>(smoothingRadius) * spacing;

        // Calculate our list of relative position offsets that the component mode will use to look up all the adjacent values that
        // the smoothFn is expecting. 
        for (float_t y = -range; y <= range; y += spacing)
        {
            for (float_t x = -range; x <= range; x += spacing)
            {
                valuePointOffsets.emplace_back(x, y, 0.0f);
            }
        }

        // Set the blending operation based on the current paintbrush blend mode setting.
        PaintBrushNotifications::BlendFn blendFn = GetBlendFunction(brushSettings);

        // Select the appropriate smoothing function.

        PaintBrushNotifications::SmoothFn smoothFn;

        switch (brushSettings.GetSmoothMode())
        {
        case AzToolsFramework::PaintBrushSmoothMode::Gaussian:
            gaussianWeights = CalculateGaussianWeights(brushSettings.GetSmoothingRadius());

            smoothFn = [&gaussianWeights, &blendFn](float baseValue, AZStd::span<float> kernelValues, float opacity) -> float
            {
                AZ_Assert(kernelValues.size() == gaussianWeights.size(), "Invalid number of points to smooth.");

                // Calculate a weighted Gaussian average value from the neighborhood of values surrounding (and including) the
                // baseValue.
                // Note that this makes the assumption that the kernelValues are stored sequentially in row order.
                float smoothedValue = 0.0f;
                for (size_t index = 0; index < kernelValues.size(); index++)
                {
                    smoothedValue += (kernelValues[index] * gaussianWeights[index]);
                }

                return blendFn(baseValue, smoothedValue, opacity);
            };
            break;
        case AzToolsFramework::PaintBrushSmoothMode::Mean:
            // We'll use a 3x3 kernel, but any kernel size here would work.
            smoothFn = [&blendFn](float baseValue, AZStd::span<float> kernelValues, float opacity) -> float
            {
                // Calculate the average value from the neighborhood of values surrounding (and including) the baseValue.
                float smoothedValue = 0.0f;
                for (size_t index = 0; index < kernelValues.size(); index++)
                {
                    smoothedValue += kernelValues[index];
                }

                return blendFn(baseValue, smoothedValue / kernelValues.size(), opacity);
            };
            break;
        case AzToolsFramework::PaintBrushSmoothMode::Median:
            smoothFn = [&blendFn](float baseValue, AZStd::span<float> kernelValues, float opacity) -> float
            {
                // Find the middle value from the neighborhood of values surrounding (and including) the baseValue.
                // This will change the order of the values in kernelValues!
                AZStd::nth_element(kernelValues.begin(), kernelValues.begin() + (kernelValues.size() / 2), kernelValues.end());
                float medianValue = kernelValues[kernelValues.size() / 2];

                return blendFn(baseValue, medianValue, opacity);
            };
            break;
        }

        // Trigger the OnSmooth notification. Provide the listener with the strokeRegion to describe the general area of the paint stroke,
        // the valueLookupFn to find out the paint brush values at specific world positions, the valuePointOffsets to describe
        // which relative values to gather for smoothing, and the smoothFn to perform smoothing operations based on the provided
        // base value, adjacent values, and paint brush settings.
        PaintBrushNotificationBus::Event(
            m_ownerEntityComponentId,
            &PaintBrushNotificationBus::Events::OnSmooth,
            strokeRegion,
            valueLookupFn,
            valuePointOffsets,
            smoothFn);
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
            AzToolsFramework::ActionOverride()
                .SetUri(PaintbrushIncreaseHardness)
                .SetKeySequence(QKeySequence{ Qt::Key_BraceRight })
                .SetTitle(PaintbrushIncreaseHardnessTitle)
                .SetTip(PaintbrushIncreaseHardnessDesc)
                .SetEntityComponentIdPair(m_ownerEntityComponentId)
                .SetCallback(
                    [this]()
                    {
                        AdjustHardnessPercent(ed_paintBrushHardnessPercentAdjustAmount);
                    }),
            AzToolsFramework::ActionOverride()
                .SetUri(PaintbrushDecreaseHardness)
                .SetKeySequence(QKeySequence{ Qt::Key_BraceLeft })
                .SetTitle(PaintbrushDecreaseHardnessTitle)
                .SetTip(PaintbrushDecreaseHardnessDesc)
                .SetEntityComponentIdPair(m_ownerEntityComponentId)
                .SetCallback(
                    [this]()
                    {
                        AdjustHardnessPercent(-ed_paintBrushHardnessPercentAdjustAmount);
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

    void PaintBrushManipulator::AdjustHardnessPercent(float hardnessPercentDelta)
    {
        float hardnessPercent = 0.0f;
        PaintBrushSettingsRequestBus::BroadcastResult(hardnessPercent, &PaintBrushSettingsRequestBus::Events::GetHardnessPercent);
        hardnessPercent = AZStd::clamp(hardnessPercent + hardnessPercentDelta, 0.0f, 100.0f);
        PaintBrushSettingsRequestBus::Broadcast(&PaintBrushSettingsRequestBus::Events::SetHardnessPercent, hardnessPercent);
    }

} // namespace AzToolsFramework
