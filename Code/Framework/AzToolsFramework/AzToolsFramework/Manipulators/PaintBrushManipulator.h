/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "BaseManipulator.h"

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/PaintBrush/PaintBrush.h>
#include <AzToolsFramework/PaintBrush/PaintBrushNotificationBus.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettings.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsNotificationBus.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    class ManipulatorViewProjectedCircle;

    //! PaintBrushManipulator contains the core logic for managing the paintbrush in the Editor.
    //! It uses the global paintbrush settings for painting and provides logic for converting mouse events into paintbrush actions.
    //! It also draws the paintbrush circles in the viewport and provides the UX for switching paint modes.
    //! The PaintBrushManipulator relies on a PaintBrush instance to do the actual painting logic and on a component-specific
    //! Component Mode to manage the usage of the PaintBrushManipulator (and PaintBrush) to apply the painted values to a specific
    //! data source and to save the data appropriately.
    //! 
    class PaintBrushManipulator
        : public BaseManipulator
        , public ManipulatorSpace
        , protected PaintBrushSettingsNotificationBus::Handler
    {
        //! Private constructor.
        PaintBrushManipulator(
            const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair, PaintBrushColorMode colorMode);

    public:
        AZ_RTTI(PaintBrushManipulator, "{0621CB58-21FD-474A-A296-5B1192E714E7}", BaseManipulator);
        AZ_CLASS_ALLOCATOR(PaintBrushManipulator, AZ::SystemAllocator, 0);

        PaintBrushManipulator() = delete;
        PaintBrushManipulator(const PaintBrushManipulator&) = delete;
        PaintBrushManipulator& operator=(const PaintBrushManipulator&) = delete;

        ~PaintBrushManipulator() override;

        //! A Manipulator must only be created and managed through a shared_ptr.
        static AZStd::shared_ptr<PaintBrushManipulator> MakeShared(
            const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair, PaintBrushColorMode colorMode);

        //! Draw the current manipulator state.
        void Draw(
            const ManipulatorManagerState& managerState, AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState, const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        //! Handle mouse events
        bool HandleMouseInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

        //! Returns the actions that we want any Component Mode using the Paint Brush Manipulator to support.
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl();

        //! Adjusts the size of the paintbrush
        void AdjustSize(float sizeDelta);

        //! Adjusts the paintbrush hardness percent
        void AdjustHardnessPercent(float hardnessPercentDelta);

    private:
        //! Create the manipulator view(s) for the paintbrush.
        void SetView(
            AZStd::shared_ptr<ManipulatorViewProjectedCircle> innerCircle, AZStd::shared_ptr<ManipulatorViewProjectedCircle> outerCircle);

        //! Calculate the radius for the inner and out circles for the paintbrush manipulator views based on the given brush settings.
        //! @param settings The paint brush settings to use for calculating the two radii
        //! @return The inner radius and the outer radius for the brush manipulator views
        AZStd::pair<float, float> GetBrushRadii(const PaintBrushSettings& settings) const;

        //! PaintBrushSettingsNotificationBus overrides...
        void OnSettingsChanged(const PaintBrushSettings& newSettings) override;

        //! Move the paint brush and perform any appropriate brush actions if in the middle of a brush stroke.
        //! @param viewportId The viewport to move the paint brush in.
        //! @param screenCoordinates The screen coordinates of the current mouse location.
        void MovePaintBrush(int viewportId, const AzFramework::ScreenPoint& screenCoordinates);

        AZStd::shared_ptr<ManipulatorViewProjectedCircle> m_innerCircle;
        AZStd::shared_ptr<ManipulatorViewProjectedCircle> m_outerCircle;

        //! The entity/component that owns this paintbrush.
        AZ::EntityComponentIdPair m_ownerEntityComponentId;

        PaintBrush m_paintBrush;
    };
} // namespace AzToolsFramework
