/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/NodeGroupContextMenu.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeGroupMenuActions/NodeGroupContextMenuActions.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructPresetMenuActions.h>

namespace GraphCanvas
{
    /////////////////////////
    // NodeGroupContextMenu
    /////////////////////////
    
    NodeGroupContextMenu::NodeGroupContextMenu(EditorId editorId, QWidget* parent)
        : EditorContextMenu(editorId, parent)
    {
        m_editActionsGroup.PopulateMenu(this);

        AddActionGroup(EditGroupTitleMenuAction::GetNodeGroupContextMenuActionGroupId());
        AddMenuAction(aznew EditGroupTitleMenuAction(this));
        
        m_nodeGroupActionsGroup.PopulateMenu(this);
        m_alignmentActionsGroup.PopulateMenu(this);

        AddActionGroup(CreatePresetFromSelection::GetCreateConstructContextMenuActionGroupId());

        {
            // Preset Creation
            const ConstructTypePresetBucket* presetBucket = nullptr;
            AssetEditorSettingsRequestBus::EventResult(presetBucket, editorId, &AssetEditorSettingsRequests::GetConstructTypePresetBucket, ConstructType::NodeGroup);
            if (presetBucket)
            {
                m_createPresetFrom = aznew CreatePresetFromSelection(this);
                AddMenuAction(m_createPresetFrom);
            }
        }

        m_applyNodeGroupPresets.PopulateMenu(this);
    }
    
    void NodeGroupContextMenu::OnRefreshActions(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        AZ_UNUSED(graphId);
        AZ_UNUSED(targetId);

        m_editActionsGroup.SetPasteEnabled(false);

        m_nodeGroupActionsGroup.RefreshPresets();

        m_applyNodeGroupPresets.RefreshPresets();
        m_applyNodeGroupPresets.RefreshActionGroup(graphId, targetId);
    }
}
