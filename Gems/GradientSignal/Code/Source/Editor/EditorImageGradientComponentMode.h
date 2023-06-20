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
#include <AzToolsFramework/PaintBrush/PaintBrushSubModeCluster.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientModificationBus.h>

namespace GradientSignal
{
    class PaintBrushUndoBuffer;
    class ImageTileBuffer;

    class EditorImageGradientComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , private ImageGradientModificationNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(EditorImageGradientComponentMode, "{49957D52-F1C3-4C34-AA84-7661BC418AB2}", EditorBaseComponentMode)

        EditorImageGradientComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~EditorImageGradientComponentMode() override;

        static void Reflect(AZ::ReflectContext* context);

        static void RegisterActions();
        static void BindActionsToModes();
        static void BindActionsToMenus();

        // EditorBaseComponentMode overrides...
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        AZStd::string GetComponentModeName() const override;
        AZ::Uuid GetComponentModeType() const override;

    protected:
        // ImageGradientModificationNotificationBus overrides...
        void OnImageGradientBrushStrokeBegin() override;
        void OnImageGradientBrushStrokeEnd(AZStd::shared_ptr<ImageTileBuffer> changedDataBuffer, const AZ::Aabb& dirtyRegion) override;

        void BeginUndoBatch();
        void EndUndoBatch();

    private:
        //! The core paintbrush manipulator and painting logic.
        AZStd::shared_ptr<AzToolsFramework::PaintBrushManipulator> m_brushManipulator;

        //! The undo information for the in-progress painting brush stroke.
        AzToolsFramework::UndoSystem::URSequencePoint* m_undoBatch = nullptr;
        PaintBrushUndoBuffer* m_paintBrushUndoBuffer = nullptr;

        //! The paint brush cluster that manages switching between paint/smooth/eyedropper modes
        AzToolsFramework::PaintBrushSubModeCluster m_subModeCluster;
    };
} // namespace GradientSignal
