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
#include <TerrainRenderer/TerrainMacroMaterialBus.h>


namespace Terrain
{
    class PaintBrushUndoBuffer;
    class ImageTileBuffer;

    class EditorTerrainMacroMaterialComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , private TerrainMacroColorModificationNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(EditorTerrainMacroMaterialComponentMode, "{24B7280F-2344-4BB4-A0BC-4ADAD6715EE4}");
        AZ_CLASS_ALLOCATOR(EditorTerrainMacroMaterialComponentMode, AZ::SystemAllocator)

        EditorTerrainMacroMaterialComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~EditorTerrainMacroMaterialComponentMode() override;

        // EditorBaseComponentMode
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        AZStd::string GetComponentModeName() const override;
        AZ::Uuid GetComponentModeType() const override;

    protected:
        // TerrainMacroColorModificationNotificationBus overrides...
        void OnTerrainMacroColorBrushStrokeBegin() override;
        void OnTerrainMacroColorBrushStrokeEnd(
            AZStd::shared_ptr<ImageTileBuffer> changedDataBuffer, const AZ::Aabb& dirtyRegion) override;

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
} // namespace Terrain
