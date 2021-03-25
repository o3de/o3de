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

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/AlignmentMenuActions/AlignmentContextMenuActions.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>
#include <GraphCanvas/Utils/GraphUtils.h>

namespace GraphCanvas
{
    /////////////////////////////
    // AlignSelectionMenuAction
    /////////////////////////////
    
    AlignSelectionMenuAction::AlignSelectionMenuAction(AZStd::string_view name, GraphUtils::VerticalAlignment verAlign, GraphUtils::HorizontalAlignment horAlign, QObject* parent)
        : AlignmentContextMenuAction(name, parent)
        , m_verAlign(verAlign)
        , m_horAlign(horAlign)
    {
    }
    
    bool AlignSelectionMenuAction::IsInSubMenu() const
    {
        return true;
    }
    
    AZStd::string AlignSelectionMenuAction::GetSubMenuPath() const
    {
        return "Align";
    }
    
    ContextMenuAction::SceneReaction AlignSelectionMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const GraphId& graphId = GetGraphId();
        EditorId editorId = GetEditorId();       

        AlignConfig alignConfig;

        alignConfig.m_verAlign = m_verAlign;
        alignConfig.m_horAlign = m_horAlign;

        AssetEditorSettingsRequestBus::EventResult(alignConfig.m_alignTime, editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        AZStd::vector< NodeId > selectedNodes;
        SceneRequestBus::EventResult(selectedNodes, graphId, &SceneRequests::GetSelectedNodes);

        GraphUtils::AlignNodes(selectedNodes, alignConfig);

        return ContextMenuAction::SceneReaction::PostUndo;
    }
}
