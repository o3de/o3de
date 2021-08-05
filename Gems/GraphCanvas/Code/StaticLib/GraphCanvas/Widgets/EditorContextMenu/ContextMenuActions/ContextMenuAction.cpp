/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
