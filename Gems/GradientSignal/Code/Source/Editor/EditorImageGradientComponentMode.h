/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>
#include <AzToolsFramework/Manipulators/PaintBrushNotificationBus.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

namespace GradientSignal
{
    class PaintBrushUndoBuffer;

    class EditorImageGradientComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , private AzToolsFramework::PaintBrushNotificationBus::Handler
    {
    public:
        EditorImageGradientComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~EditorImageGradientComponentMode() override;

        // EditorBaseComponentMode
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        AZStd::string GetComponentModeName() const override;

        void AdjustRadius(float radiusDelta);
        void AdjustIntensity(float intensityDelta);
        void AdjustOpacity(float opacityDelta);

    protected:
        // PaintBrushNotificationBus overrides
        void OnPaintBegin(const AZ::EntityComponentIdPair& id) override;
        void OnPaintEnd(const AZ::EntityComponentIdPair& id) override;
        void OnPaint(const AZ::EntityComponentIdPair& id, const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn) override;

        void BeginUndoBatch();
        void EndUndoBatch();

    private:
        //! The entity/component that owns this paintbrush.
        AZ::EntityComponentIdPair m_ownerEntityComponentId;

        AZStd::shared_ptr<AzToolsFramework::PaintBrushManipulator> m_brushManipulator;

        //! The undo information for the in-progress painting brush stroke.
        AzToolsFramework::UndoSystem::URSequencePoint* m_undoBatch = nullptr;
        PaintBrushUndoBuffer* m_paintBrushUndoBuffer = nullptr;
    };
} // namespace GradientSignal
