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
#include <AzFramework/PaintBrush/PaintBrush.h>
#include <AzFramework/PaintBrush/PaintBrushNotificationBus.h>

namespace AzFramework
{
    PaintBrush::PaintBrush(const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_ownerEntityComponentId(entityComponentIdPair)
        , m_isInPaintMode(false)
        , m_isInBrushStroke(false)
        , m_isFirstPointInBrushStrokeMovement(false)
    {
    }

    PaintBrush::~PaintBrush()
    {
        // Make sure we end cleanly by sending off notifications to end the brush stroke and/or paint mode if either is active.

        if (m_isInBrushStroke)
        {
            EndBrushStroke();
        }

        if (m_isInPaintMode)
        {
            EndPaintMode();
        }
    }

    void PaintBrush::BeginPaintMode()
    {
        if (m_isInPaintMode)
        {
            AZ_Assert(!m_isInPaintMode, "BeginPaintMode() called on a brush that's already in paint mode.");
            return;
        }

        m_isInPaintMode = true;

        // Notify listeners that we've entered the paint mode.
        PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnPaintModeBegin);
    }

    void PaintBrush::EndPaintMode()
    {
        if ((!m_isInPaintMode) || (m_isInBrushStroke))
        {
            AZ_Assert(!m_isInBrushStroke, "EndPaintMode() called while a brush stroke is still active.");
            AZ_Assert(m_isInPaintMode, "EndPaintMode() called on a brush that isn't in paint mode.");
            return;
        }

        m_isInPaintMode = false;
        // Notify listeners that we've exited the paint mode.
        PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnPaintModeEnd);
    }

    bool PaintBrush::IsInPaintMode() const
    {
        return m_isInPaintMode;
    }

    void PaintBrush::BeginBrushStroke(const PaintBrushSettings& brushSettings)
    {
        if ((!m_isInPaintMode) || (m_isInBrushStroke))
        {
            AZ_Assert(m_isInPaintMode, "BeginBrushStroke() called on a brush that isn't in paint mode.");
            AZ_Assert(!m_isInBrushStroke, "BeginBrushStroke() called on a brush that's already in a brush stroke.");
            return;
        }

        m_isInBrushStroke = true;
        m_isFirstPointInBrushStrokeMovement = true;

        // Notify listeners that the brush stroke is beginning with the given color/intensity and opacity settings.
        // The color/intensity and opacity is expected to remain invariant over the entire brush stroke.
        PaintBrushNotificationBus::Event(
            m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnBrushStrokeBegin, brushSettings.GetColor());
    }

    void PaintBrush::EndBrushStroke()
    {
        if ((!m_isInPaintMode) || (!m_isInBrushStroke))
        {
            AZ_Assert(m_isInPaintMode, "EndBrushStroke() called on a brush that isn't in paint mode.");
            AZ_Assert(m_isInBrushStroke, "EndBrushStroke() called on a brush that isn't in a brush stroke.");
            return;
        }

        m_isInBrushStroke = false;
        m_isFirstPointInBrushStrokeMovement = false;
        // Notify listeners that the brush stroke has ended.
        PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnBrushStrokeEnd);
    }

    bool PaintBrush::IsInBrushStroke() const
    {
        return m_isInBrushStroke;
    }

    void PaintBrush::ResetBrushStrokeTracking()
    {
        m_isFirstPointInBrushStrokeMovement = true;
    }

    PaintBrushNotifications::BlendFn PaintBrush::GetBlendFunction(const PaintBrushBlendMode& blendMode)
    {
        auto clampAndLerpFn = [](float baseValue, float newValue, float opacity)
        {
            return AZStd::clamp(AZStd::lerp(baseValue, newValue, opacity), 0.0f, 1.0f);
        };

        switch (blendMode)
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
                // Note that this does NOT clamp the addition before applying opacity.
                // This matches Photoshop's behavior, but other paint programs make different choices here.
                return clampAndLerpFn(baseValue, baseValue + intensity, opacity);
            };
            break;
        case PaintBrushBlendMode::Subtract:
            return [&clampAndLerpFn](float baseValue, float intensity, float opacity)
            {
                // Note that this does NOT clamp the subtraction before applying opacity.
                // This matches Photoshop's behavior, but other paint programs make different choices here.
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
            AZ_Assert(false, "Unknown PaintBrushBlendMode type: %u", blendMode);
            break;
        }

        return {};
    }

    AZStd::vector<float> PaintBrush::CalculateGaussianWeights(size_t smoothingRadius)
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
        AZStd::transform(
            gaussianWeights.begin(),
            gaussianWeights.end(),
            gaussianWeights.begin(),
            [sum](float value)
            {
                return value / sum;
            });

        return gaussianWeights;
    }

    AZ::Vector3 PaintBrush::OptionallyAdjustTo2D(const AZ::Vector3& location)
    {
        return PaintIn2D ? AZ::Vector3(AZ::Vector2(location), 0.0f) : location;
    }

    void PaintBrush::CalculateBrushStampCentersAndStrokeRegion(
        const AZ::Vector3& brushCenter,
        const PaintBrushSettings& brushSettings,
        AZStd::vector<AZ::Vector3>& brushStampCenters,
        AZ::Aabb& strokeRegion)
    {
        AZ::Vector3 currentAdjustedCenter = OptionallyAdjustTo2D(brushCenter);

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
        if (m_isFirstPointInBrushStrokeMovement)
        {
            brushStampCenters.push_back(currentAdjustedCenter);
            m_lastBrushCenter = currentAdjustedCenter;
            m_distanceSinceLastDraw = 0.0f;
            m_isFirstPointInBrushStrokeMovement = false;
        }

        // Get the direction that we've moved the mouse since the last mouse movement we handled.
        AZ::Vector3 direction = (currentAdjustedCenter - m_lastBrushCenter).GetNormalizedSafe();

        // Get the total distance that we've moved since the last time we drew a brush stamp (which might
        // have been many small mouse movements ago).
        float totalDistance = m_lastBrushCenter.GetDistance(currentAdjustedCenter) + m_distanceSinceLastDraw;

        // Keep adding brush stamps to the list for as long as the total remaining mouse distance is
        // greater than the stamp distance. If the mouse moved a large enough distance in one frame,
        // this will add multiple stamps. If the mouse moved a tiny amount, it's possible that no stamps
        // will get added, and we'll just save the accumulated distance for next frame.
        for (; totalDistance >= distanceBetweenBrushStamps; totalDistance -= distanceBetweenBrushStamps)
        {
            // Add another stamp to the list to draw this time.
            AZ::Vector3 stampCenter = m_lastBrushCenter + direction * (distanceBetweenBrushStamps - m_distanceSinceLastDraw);
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
        m_lastBrushCenter = currentAdjustedCenter;

        // If we don't have any stamps on this mouse movement, then we're done.
        if (brushStampCenters.empty())
        {
            return;
        }

        const float radius = brushSettings.GetSize() / 2.0f;

        // Create an AABB that contains every brush stamp. Since we're drawing in a line, we don't need to add in *all* the
        // brush stamp centers - all we need to add is our first and last brush stamp, and the rest are guaranteed to
        // be in-between somewhere.
        strokeRegion.AddAabb(AZ::Aabb::CreateCenterRadius(brushStampCenters.front(), radius));
        strokeRegion.AddAabb(AZ::Aabb::CreateCenterRadius(brushStampCenters.back(), radius));
    }

    void PaintBrush::CalculatePointsInBrush(
        const PaintBrushSettings& brushSettings,
        const AZStd::vector<AZ::Vector3>& brushStampCenters,
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
            const AZ::Vector3 adjustedPoint = OptionallyAdjustTo2D(points[index]);

            // Loop through each stamp that we're drawing and accumulate the results for this point.
            for (auto& brushCenter : brushStampCenters)
            {
                // Since each stamp is a circle, we can just compare distance to the center of the circle vs radius.
                if (float shortestDistanceSquared = brushCenter.GetDistanceSq(adjustedPoint);
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
    }

    void PaintBrush::PaintToLocation(
        const AZ::Vector3& brushCenter, const PaintBrushSettings& brushSettings)
    {
        if ((!m_isInPaintMode) || (!m_isInBrushStroke))
        {
            AZ_Assert(m_isInPaintMode, "PaintToLocation() called on a brush that isn't in paint mode.");
            AZ_Assert(m_isInBrushStroke, "PaintToLocation() called on a brush that isn't in a brush stroke.");
            return;
        }

        // Track the list of center points for each brush stamp to draw for this mouse movement and the AABB around the stamps.
        AZStd::vector<AZ::Vector3> brushStampCenters;
        AZ::Aabb strokeRegion = AZ::Aabb::CreateNull(); 

        CalculateBrushStampCentersAndStrokeRegion(brushCenter, brushSettings, brushStampCenters, strokeRegion);

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
        PaintBrushNotifications::BlendFn blendFn = GetBlendFunction(brushSettings.GetBlendMode());

        // Trigger the OnPaint notification, provide the listener with the valueLookupFn to find out the paint brush
        // values at specific world positions, and provide the blendFn to perform blending operations based on the provided base and
        // paint brush values.
        PaintBrushNotificationBus::Event(
            m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnPaint,
            brushSettings.GetColor(), strokeRegion, valueLookupFn, blendFn);
    }

    void PaintBrush::SmoothToLocation(
        const AZ::Vector3& brushCenter, const PaintBrushSettings& brushSettings)
    {
        if ((!m_isInPaintMode) || (!m_isInBrushStroke))
        {
            AZ_Assert(m_isInPaintMode, "SmoothToLocation() called on a brush that isn't in paint mode.");
            AZ_Assert(m_isInBrushStroke, "SmoothToLocation() called on a brush that isn't in a brush stroke.");
            return;
        }

        // Track the list of center points for each brush stamp to draw for this mouse movement and the AABB around the stamps.
        AZStd::vector<AZ::Vector3> brushStampCenters;
        AZ::Aabb strokeRegion = AZ::Aabb::CreateNull();

        CalculateBrushStampCentersAndStrokeRegion(brushCenter, brushSettings, brushStampCenters, strokeRegion);

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
        PaintBrushNotifications::BlendFn blendFn = GetBlendFunction(brushSettings.GetBlendMode());

        // Select the appropriate smoothing function.

        PaintBrushNotifications::SmoothFn smoothFn;

        switch (brushSettings.GetSmoothMode())
        {
        case AzFramework::PaintBrushSmoothMode::Gaussian:
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
        case AzFramework::PaintBrushSmoothMode::Mean:
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
        case AzFramework::PaintBrushSmoothMode::Median:
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
            brushSettings.GetColor(),
            strokeRegion,
            valueLookupFn,
            valuePointOffsets,
            smoothFn);
    }

    AZ::Color PaintBrush::UseEyedropper(const AZ::Vector3& brushCenter)
    {
        AZ::Color brushColor(0.0f);

        // Trigger the OnGetColor notification to get the current color at the given point.
        PaintBrushNotificationBus::EventResult(
            brushColor, m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnGetColor, OptionallyAdjustTo2D(brushCenter));

        return brushColor;
    }

} // namespace AzFramework
