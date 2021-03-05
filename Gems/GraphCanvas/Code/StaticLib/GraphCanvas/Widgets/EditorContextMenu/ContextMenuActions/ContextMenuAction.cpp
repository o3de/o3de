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
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>

#include <GraphCanvas/Components/SceneBus.h>

namespace GraphCanvas
{
    //////////////////////
    // ContextMenuAction
    //////////////////////
    ContextMenuAction::ContextMenuAction(AZStd::string_view actionName, QObject* parent)
        : QAction(actionName.data(), parent)
    {
    }

    void ContextMenuAction::SetTarget(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        m_graphId = graphId;
        m_targetId = targetId;

        RefreshAction();
    }

    bool ContextMenuAction::IsInSubMenu() const
    {
        return false;
    }

    AZStd::string ContextMenuAction::GetSubMenuPath() const
    {
        return "";
    }

    const AZ::EntityId& ContextMenuAction::GetTargetId() const
    {
        return m_targetId;
    }

    const GraphId& ContextMenuAction::GetGraphId() const
    {
        return m_graphId;
    }

    EditorId ContextMenuAction::GetEditorId() const
    {
        EditorId editorId;
        SceneRequestBus::EventResult(editorId, GetGraphId(), &SceneRequests::GetEditorId);

        return editorId;
    }

    void ContextMenuAction::RefreshAction()
    {
        RefreshAction(m_graphId, m_targetId);
    }

    void ContextMenuAction::RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        AZ_UNUSED(graphId);
        AZ_UNUSED(targetId);

        setEnabled(true);
    }

#include <StaticLib/GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/moc_ContextMenuAction.cpp>
}