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
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

namespace GradientSignal
{
    class PaintBrushUndoBuffer;
    class ImageTileBuffer;

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

    protected:
        // PaintBrushNotificationBus overrides
        void OnPaintStrokeBegin(float intensity, float opacity) override;
        void OnPaintStrokeEnd() override;
        void OnPaint(const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn, BlendFn& blendFn) override;

        void BeginUndoBatch();
        void EndUndoBatch();

        void CreateSubModeSelectionCluster();
        void RemoveSubModeSelectionCluster();

    private:
        struct PaintStrokeData
        {
            //! A buffer to accumulate a single paint stroke into. This buffer is used to ensure that within a single paint stroke,
            //! we only perform an operation on a pixel once, not multiple times.
            //! After the paint stroke is complete, this buffer is handed off to the undo/redo batch so that we can undo/redo each stroke.
            AZStd::unique_ptr<ImageTileBuffer> m_strokeBuffer;

            //! The meters per pixel in each direction for this image gradient.
            //! These help us query the paintbrush for exactly one world position per image pixel.
            float m_metersPerPixelX = 0.0f;
            float m_metersPerPixelY = 0.0f;

            //! The intensity of the paint stroke (0 - 1)
            float m_intensity = 0.0f;

            //! The opacity of the paint stroke (0 - 1)
            float m_opacity = 0.0f;

            //! Track the dirty region for each paint stroke so that we can store it in the undo/redo buffer
            //! to send with change notifications.
            AZ::Aabb m_dirtyRegion = AZ::Aabb::CreateNull();
        };

        PaintStrokeData m_paintStrokeData;

        //! The entity/component that owns this paintbrush.
        AZ::EntityComponentIdPair m_ownerEntityComponentId;

        AZStd::shared_ptr<AzToolsFramework::PaintBrushManipulator> m_brushManipulator;

        //! The undo information for the in-progress painting brush stroke.
        AzToolsFramework::UndoSystem::URSequencePoint* m_undoBatch = nullptr;
        PaintBrushUndoBuffer* m_paintBrushUndoBuffer = nullptr;

        AzToolsFramework::ViewportUi::ClusterId m_paintBrushControlClusterId;
        AzToolsFramework::ViewportUi::ButtonId m_paintBrushSettingsButtonId;

        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_buttonSelectionHandler;
    };
} // namespace GradientSignal
