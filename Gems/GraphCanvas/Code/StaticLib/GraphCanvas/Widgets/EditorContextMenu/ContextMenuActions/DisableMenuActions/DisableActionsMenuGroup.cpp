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
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/DisableMenuActions/DisableActionsMenuGroup.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/DisableMenuActions/DisableMenuActions.h>

namespace GraphCanvas
{
    ////////////////////////////
    // DisableActionsMenuGroup
    ////////////////////////////

    DisableActionsMenuGroup::DisableActionsMenuGroup()
        : m_setSelectionEnableState(nullptr)
    {
    }

    DisableActionsMenuGroup::~DisableActionsMenuGroup()
    {
    }

    void DisableActionsMenuGroup::PopulateMenu(EditorContextMenu* contextMenu)
    {
        EditorId editorId = contextMenu->GetEditorId();

        bool allowDisabling = false;
        AssetEditorSettingsRequestBus::EventResult(allowDisabling, editorId, &AssetEditorSettingsRequests::AllowNodeDisabling);

        if (allowDisabling)
        {
            contextMenu->AddActionGroup(DisableContextMenuAction::GetDisableContextMenuActionGroupId());

            m_setSelectionEnableState = aznew SetEnabledStateMenuAction(contextMenu);
            contextMenu->AddMenuAction(m_setSelectionEnableState);
        }
    }

    void DisableActionsMenuGroup::RefreshActions(const GraphId& graphId)
    {
        if (m_setSelectionEnableState)
        {
            AZStd::vector< NodeId > selectedNodes;
            SceneRequestBus::EventResult(selectedNodes, graphId, &SceneRequests::GetSelectedNodes);

            bool isEnabled = true;

            for (const NodeId& nodeId : selectedNodes)
            {
                RootGraphicsItemEnabledState enabledState = RootGraphicsItemEnabledState::ES_Enabled;
                RootGraphicsItemRequestBus::EventResult(enabledState, nodeId, &RootGraphicsItemRequests::GetEnabledState);

                if (enabledState == RootGraphicsItemEnabledState::ES_Disabled)
                {
                    isEnabled = false;
                    break;
                }
            }

            static_cast<SetEnabledStateMenuAction*>(m_setSelectionEnableState)->SetEnableState(!isEnabled);
        }
    }
}