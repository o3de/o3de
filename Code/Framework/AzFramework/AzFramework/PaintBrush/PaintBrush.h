/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/PaintBrush/PaintBrushNotificationBus.h>
#include <AzFramework/PaintBrush/PaintBrushSettings.h>

namespace AzFramework
{
    //! PaintBrush contains the core logic for painting functionality.
    //! It handles all of the specific calculations for determining the correct values to paint or smooth as a brush moves
    //! across the world in a generic way. It relies on a listener to the PaintBrushNotificationBus to provide the other half
    //! of the logic for fetching and applying the values themselves. 
    //!
    //! The general painting flow consists of the following:
    //! 1. For each brush movement, the paintbrush sends OnPaint messages with the AABB of the region that has changed
    //! and a callback for getting paint values for specific world positions.
    //! 2. The listener calls the callback for each position in the region that it cares about.
    //! 3. The paintbrush responds with the specific painted values for each of those positions based on the brush shape and settings.
    //! This back-and-forth is needed so that we can keep a clean separation between the paintbrush and the listener. The paintbrush
    //! doesn't have knowledge of which points in the world (or at which resolution) the listener needs, and the listener doesn't have
    //! knowledge of the exact shape, size, and hardness of the paintbrush. The separation allows the paintbrush to add
    //! features over time without having to change the listeners, and it enables more systems to use the paintbrush without requiring
    //! specialized logic inside the paintbrush itself.
    class PaintBrush
    {

    public:
        AZ_RTTI(PaintBrush, "{D003175F-12BF-4E7E-BD51-2F4B010C9B5E}");
        AZ_CLASS_ALLOCATOR(PaintBrush, AZ::SystemAllocator);

        PaintBrush() = delete;
        PaintBrush(const PaintBrush&) = delete;
        PaintBrush& operator=(const PaintBrush&) = delete;
        explicit PaintBrush(const AZ::EntityComponentIdPair& entityComponentIdPair);

        virtual ~PaintBrush();

        //! Tell the PaintBrush to begin paint mode, which will send out the proper notifications to any listeners.
        void BeginPaintMode();

        //! Tell the PaintBrush to end paint mode, which will send out the proper notifications to any listeners.
        void EndPaintMode();

        //! Returns whether or not the paint mode is currently active.
        //! @return True if paint mode is active, false if it isn't.
        bool IsInPaintMode() const;

        //! Start a brush stroke with the given brush settings for color/intensity/opacity.
        void BeginBrushStroke(const PaintBrushSettings& brushSettings);

        //! End the current brush stroke.
        void EndBrushStroke();

        //! Returns whether or not the brush is currently in a brush stroke.
        //! @return True if a brush stroke has been started, false if not.
        bool IsInBrushStroke() const;

        //! Reset the brush tracking so that the next action will be considered the start of a stroke movement instead of a continuation.
        //! This is useful for handling discontinuous movement (like moving off the edge of the surface on one side and back on from
        //! a different side).
        void ResetBrushStrokeTracking();

        //! Apply a paint color to the underlying data based on brush movement and settings.
        //! @param brushCenter The current center of the paintbrush.
        //! @param brushSettings The current paintbrush settings.
        void PaintToLocation(const AZ::Vector3& brushCenter, const PaintBrushSettings& brushSettings);

        //! Smooth the underlying data based on brush movement and settings.
        //! @param brushCenter The current center of the paintbrush.
        //! @param brushSettings The current paintbrush settings.
        void SmoothToLocation(const AZ::Vector3& brushCenter, const PaintBrushSettings& brushSettings);

        //! Get the color from the underlying data that's located at the brush center
        //! @param brushCenter The current center of the paintbrush.
        AZ::Color UseEyedropper(const AZ::Vector3& brushCenter);

    private:
        //! For now, the paintbrush will perform all of its calculations in 2D space.
        //! This could eventually get exposed as a paintbrush setting if we add a paintbrush consumer that needs to paint in 3D space.
        static constexpr bool PaintIn2D = true;

        //! Adjust the given location to either remain in 3D or get "flattened" to 2D with a Z value of 0
        //! depending on the value of PaintIn2D.
        static AZ::Vector3 OptionallyAdjustTo2D(const AZ::Vector3& location);

        //! Get an appropriate blend function for the requested blend mode.
        //! @param blendMode The blend mode to use.
        static PaintBrushNotifications::BlendFn GetBlendFunction(const PaintBrushBlendMode& blendMode);

        //! Calculate the Gaussian weights to use for combining all the sampled pixels for the smoothing function.
        static AZStd::vector<float> CalculateGaussianWeights(size_t smoothingRadius);

        //! Generates a list of brush stamp centers and an AABB around the brush stamps for the current brush stroke movement.
        //! @param brushCenter The current center of the paintbrush.
        //! @param brushSettings The current paintbrush settings.
        //! @param brushStampCenters [out] The list of brush centers to use for this brush stroke movement.
        //! @param strokeRegion [out] The AABB around the brush stamps in the brushStampCenters list.
        void CalculateBrushStampCentersAndStrokeRegion(
            const AZ::Vector3& brushCenter,
            const PaintBrushSettings& brushSettings,
            AZStd::vector<AZ::Vector3>& brushStampCenters,
            AZ::Aabb& strokeRegion);

        //! Determine which of the passed-in points are within our current brush stroke, and calculate the opacity at each point.
        //! @param brushSettings The current paintbrush settings.
        //! @param brushStampCenters The list of brush centers for each stamp in our current brush stroke
        //! @param points The list of points to calculate values for within our brush stroke
        //! @param validPoints [out] The subset of the input points that are within the brush stroke
        //! @param opacities [out] For each point in validPoints, the opacity of the brush at that point
        static void CalculatePointsInBrush(
            const PaintBrushSettings& brushSettings,
            const AZStd::vector<AZ::Vector3>& brushStampCenters,
            AZStd::span<const AZ::Vector3> points,
            AZStd::vector<AZ::Vector3>& validPoints,
            AZStd::vector<float>& opacities);

        //! The entity/component that owns this paintbrush.
        AZ::EntityComponentIdPair m_ownerEntityComponentId;

        //! Location of the last mouse point that we processed while painting.
        AZ::Vector3 m_lastBrushCenter;

        //! Distance that the mouse has traveled since the last time we drew a paint stamp.
        float m_distanceSinceLastDraw = 0.0f;

        //! Track whether or not the paintbrush is currently active.
        bool m_isInPaintMode = false;

        //! Track when a brush stroke is active.
        bool m_isInBrushStroke = false;

        //! The first point in a brush stroke movement starts the stroke, and subsequent points continue from the previous location.
        bool m_isFirstPointInBrushStrokeMovement = false;
    };
} // namespace AzFramework
