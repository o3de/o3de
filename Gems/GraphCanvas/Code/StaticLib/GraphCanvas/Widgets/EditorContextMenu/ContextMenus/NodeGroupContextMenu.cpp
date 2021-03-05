/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    }
    
    void NodeGroupContextMenu::OnRefreshActions(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        AZ_UNUSED(graphId);
        AZ_UNUSED(targetId);

        m_editActionsGroup.SetPasteEnabled(false);

        m_nodeGroupActionsGroup.RefreshPresets();
    }
}
