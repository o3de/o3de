/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "BaseManipulator.h"

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Manipulators/PaintBrushRequestBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    class ManipulatorViewProjectedCircle;

    //! PaintBrushConfig exposes the paint brush configuration properties so that we can edit them via the component editor.
    //! Currently, for this to work, we end up needing two copies of the configuration. One needs to be on the Editor component
    //! that's supporting the painting, and one is internal to the PaintBrushManipulator. The two are manually kept in sync
    //! through PaintBrush Request/Notification EBus calls.
    //!
    //! If we ever add support for modifying these via in-viewport UX, we could then make these settings internal to the
    //! manipulator and simplify the logic.
    class PaintBrushConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(PaintBrushConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(PaintBrushConfig, "{CE5EFFE2-14E5-4A9F-9B0F-695F66744A50}");
        static void Reflect(AZ::ReflectContext* context);

        virtual ~PaintBrushConfig() = default;

        //! Paintbrush radius
        float m_radius = 5.0f;
        //! Paintbrush intensity (black to white)
        float m_intensity = 1.0f;
        //! Paintbrush opacity (transparent to opaque)
        float m_opacity = 0.5f;

        //! The entity/component that owns this paintbrush.
        AZ::EntityComponentIdPair m_ownerEntityComponentId;

        AZ::u32 OnIntensityChange();
        AZ::u32 OnOpacityChange();
        AZ::u32 OnRadiusChange();
    };

    //! PaintBrushManipulator contains the core logic for painting functionality.
    //! It handles the paintbrush settings, the logic for converting mouse events into paintbrush actions,
    //! the rendering of the paintbrush into the viewport,  and the specific value calculations for what
    //! values are painted at each world space location.
    //!
    //! The component mode logic is handled separately by the component that is using the paintbrush.
    //!
    //! The general painting flow consists of the following:
    //! 1. For each mouse movement while painting, the paintbrush sends OnPaint messages with the AABB of the region that has changed
    //! and a callback for getting paint values for specific world positions.
    //! 2. The listener calls the callback for each position in the region that it cares about.
    //! 3. The paintbrush responds with the specific painted values for each of those positions based on the brush shape and settings.
    //! This back-and-forth is needed so that we can keep a clean separation between the paintbrush and the listener. The paintbrush
    //! doesn't have knowledge of which points in the world (or at which resolution) the listener needs, and the listener doesn't have
    //! knowledge of the exact shape, size, and hardness of the paintbrush. The separation allows the paintbrush manipulator to add
    //! features over time without having to change the listeners, and it enables more systems to use the paintbrush without requiring
    //! specialized logic inside the paintbrush itself.
    class PaintBrushManipulator
        : public BaseManipulator
        , public ManipulatorSpace
        , public PaintBrushRequestBus::Handler
    {
        //! Private constructor.
        PaintBrushManipulator(
            const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair);

    public:
        AZ_RTTI(PaintBrushManipulator, "{0621CB58-21FD-474A-A296-5B1192E714E7}", BaseManipulator);
        AZ_CLASS_ALLOCATOR(PaintBrushManipulator, AZ::SystemAllocator, 0);

        PaintBrushManipulator() = delete;
        PaintBrushManipulator(const PaintBrushManipulator&) = delete;
        PaintBrushManipulator& operator=(const PaintBrushManipulator&) = delete;

        ~PaintBrushManipulator() override;

        //! A Manipulator must only be created and managed through a shared_ptr.
        static AZStd::shared_ptr<PaintBrushManipulator> MakeShared(
            const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair);

        //! Draw the current manipulator state.
        void Draw(
            const ManipulatorManagerState& managerState, AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState, const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetView(AZStd::shared_ptr<ManipulatorViewProjectedCircle> view);

        // Handle mouse events
        bool HandleMouseInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

        // PaintBrushRequestBus overrides for getting/setting the paintbrush settings...
        float GetRadius() const override;
        float GetIntensity() const override;
        float GetOpacity() const override;
        void SetRadius(float radius) override;
        void SetIntensity(float intensity) override;
        void SetOpacity(float opacity) override;

    private:
        void MovePaintBrush(int viewportId, const AzFramework::ScreenPoint& screenCoordinates, bool isFirstPaintedPoint);

        AZStd::shared_ptr<ManipulatorViewProjectedCircle> m_manipulatorView;

        //! The entity/component that owns this paintbrush.
        AZ::EntityComponentIdPair m_ownerEntityComponentId;

        //! True if we're currently painting, false if not.
        bool m_isPainting = false;

        //! Tracks the previous location we painted so that we can generate a continuous brush stroke.
        AZ::Vector3 m_previousCenter;

        //! Current center of the paintbrush in world space.
        AZ::Vector3 m_center;

        PaintBrushConfig m_config;
    };
} // namespace AzToolsFramework
