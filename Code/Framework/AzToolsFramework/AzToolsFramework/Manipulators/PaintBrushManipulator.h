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
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsNotificationBus.h>

namespace AzToolsFramework
{
    class ManipulatorViewProjectedCircle;

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
        , protected PaintBrushSettingsNotificationBus::Handler
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

        //! Handle mouse events
        bool HandleMouseInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

        //! Returns the actions that we want any Component Mode using the Paint Brush Manipulator to support.
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl();
 
        void AdjustSize(float sizeDelta);
        void AdjustIntensityPercent(float intensityPercentDelta);
        void AdjustOpacityPercent(float opacityPercentDelta);

    private:
        void OnSettingsChanged(const PaintBrushSettings& newSettings) override;

        void MovePaintBrush(int viewportId, const AzFramework::ScreenPoint& screenCoordinates, bool isFirstPaintedPoint);

        AZStd::shared_ptr<ManipulatorViewProjectedCircle> m_manipulatorView;

        //! The entity/component that owns this paintbrush.
        AZ::EntityComponentIdPair m_ownerEntityComponentId;

        //! True if we're currently painting, false if not.
        bool m_isPainting = false;

        //! Location of the last mouse point that we processed while painting.
        AZ::Vector2 m_lastBrushCenter;

        //! Distance that the mouse has traveled since the last time we drew a paint stamp.
        float m_distanceSinceLastDraw = 0.0f;
    };
} // namespace AzToolsFramework
