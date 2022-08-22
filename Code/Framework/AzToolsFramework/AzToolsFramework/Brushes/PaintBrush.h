/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <AzToolsFramework/Brushes/PaintBrushRequestBus.h>

namespace AzToolsFramework
{
    //! PaintBrush contains the core logic for painting functionality.
    //! It handles the paintbrush settings, the logic for converting mouse events into paintbrush actions,
    //! and the specific value calculations for what values are painted at each world space location.
    //!
    //! The rendering of the paintbrush in the viewport is handled separately by the PaintBrushManipulator,
    //! the component mode logic is handled separately by the component that is using the paintbrush.
    //!
    //! The general flow of painting consists of the following:
    //! 1. For each mouse movement while painting, the paintbrush sends OnPaint messages with the AABB of the region that has changed.
    //! 2. The listener calls PaintBrushRequestBus::GetValues() for each position in the region that it cares about.
    //! 3. The paintbrush responds with the specific painted values for each of those positions based on the brush shape and settings.
    //! This back-and-forth is needed so that we can keep a clean separation between the paintbrush and the listener. The paintbrush
    //! doesn't have knowledge of which points in the world (or at which resolution) the listener needs, and the listener doesn't have
    //! knowledge of the exact shape, size, and hardness of the paintbrush.
    class PaintBrush : public PaintBrushRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PaintBrush, AZ::SystemAllocator, 0);
        AZ_RTTI(PaintBrush, "{892AFC4A-2BF6-4D63-A405-AC93F5E4832B}");
        static void Reflect(AZ::ReflectContext* context);

        void Activate(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Deactivate();

        // PaintBrushRequestBus overrides for getting/setting the paintbrush settings...
        float GetRadius() const override;
        float GetIntensity() const override;
        float GetOpacity() const override;
        void SetRadius(float radius) override;
        void SetIntensity(float intensity) override;
        void SetOpacity(float opacity) override;

        // PaintBrushRequestBus overrides for handling mouse events...
        bool HandleMouseInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;

        // PaintBrushRequestBus overrides for getting specific painted values while painting...
        void GetValue(const AZ::Vector3& point, float& intensity, float& opacity, bool& isValid) override;
        void GetValues(
            AZStd::span<const AZ::Vector3> points,
            AZStd::span<float> intensities,
            AZStd::span<float> opacities,
            AZStd::span<bool> validFlags) override;

    private:
        void MovePaintBrush(int viewportId, const AzFramework::ScreenPoint& screenCoordinates);

        //! The entity/component that owns this paintbrush.
        AZ::EntityComponentIdPair m_ownerEntityComponentId;

        //! True if we're currently painting, false if not.
        bool m_isPainting = false;

        //! Tracks the previous location we painted so that we can generate a continuous brush stroke.
        AZ::Vector3 m_previousCenter;
        //! Determines if we need to generate a brush stroke yet or not.
        bool m_isFirstPaintedPoint = true;

        //! Current center of the paintbrush in world space.
        AZ::Vector3 m_center;

        //! Paintbrush radius
        float m_radius = 5.0f;
        //! Paintbrush intensity (black to white)
        float m_intensity = 1.0f;
        //! Paintbrush opacity (transparent to opaque)
        float m_opacity = 0.5f;

        AZ::u32 OnIntensityChange();
        AZ::u32 OnOpacityChange();
        AZ::u32 OnRadiusChange();
    };
} // namespace AzToolsFramework
