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
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/CollapsedNodeGroupContextMenu.h>

namespace GraphCanvas
{
    //////////////////////////////////
    // CollapsedNodeGroupContextMenu
    //////////////////////////////////
    
    CollapsedNodeGroupContextMenu::CollapsedNodeGroupContextMenu(EditorId editorId, QWidget* parent)
        : EditorContextMenu(editorId, parent)
    {
        m_editActionGroup.PopulateMenu(this);
        m_nodeGroupActionGroup.PopulateMenu(this);
        m_alignmentActionGroup.PopulateMenu(this);
    }
    
    void CollapsedNodeGroupContextMenu::OnRefreshActions(const GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        AZ_UNUSED(graphId);
        AZ_UNUSED(targetMemberId);

        m_editActionGroup.SetPasteEnabled(false);

        m_nodeGroupActionGroup.RefreshPresets();
    }
}