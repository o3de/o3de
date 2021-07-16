/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeGroupMenuActions/NodeGroupContextMenuActions.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/GraphCanvasBus.h>

#include <GraphCanvas/Utils/GraphUtils.h>

namespace GraphCanvas
{
    //////////////////////////////
    // CreateNodeGroupMenuAction
    //////////////////////////////

    CreateNodeGroupMenuAction::CreateNodeGroupMenuAction(QObject* parent, bool collapseGroup)
        : NodeGroupContextMenuAction("", parent)
        , m_collapseGroup(collapseGroup)
    {
        if (!collapseGroup)
        {
            setText("Group");
            setToolTip("Will create a Node Group for the selected nodes.");
        }
        else
        {
            setText("Group [Collapsed]");
            setToolTip("Will create a Node Group for the selected nodes, and then collapse the group to a single node.");
        }
    }

    void CreateNodeGroupMenuAction::RefreshAction()
    {
        const GraphId& graphId = GetGraphId();
        const AZ::EntityId& targetId = GetTargetId();

        bool hasSelection = false;
        SceneRequestBus::EventResult(hasSelection, graphId, &SceneRequests::HasSelectedItems);

        if (hasSelection)
        {
            hasSelection = !GraphUtils::IsNodeGroup(targetId);
        }

        setEnabled(hasSelection);
    }

    ContextMenuAction::SceneReaction CreateNodeGroupMenuAction::TriggerAction(const AZ::Vector2& scenePos)
    {
        const GraphId& graphId = GetGraphId();

        bool hasSelection = false;
        SceneRequestBus::EventResult(hasSelection, graphId, &SceneRequests::HasSelectedItems);

        AZStd::vector< AZ::EntityId > selectedNodes;

        if (hasSelection)
        {
            SceneRequestBus::EventResult(selectedNodes, graphId, &SceneRequests::GetSelectedNodes);
        }

        AZ::EntityId groupId = GraphUtils::CreateGroupForElements(graphId, selectedNodes, scenePos);

        if (groupId.IsValid())
        {
            SceneRequestBus::Event(graphId, &SceneRequests::ClearSelection);

            if (!m_collapseGroup)
            {
                SceneMemberUIRequestBus::Event(groupId, &SceneMemberUIRequests::SetSelected, true);
                CommentUIRequestBus::Event(groupId, &CommentUIRequests::SetEditable, true);
            }
            else
            {
                NodeGroupRequestBus::Event(groupId, &NodeGroupRequests::CollapseGroup);
            }

            return SceneReaction::PostUndo;
        }
        else
        {
            return SceneReaction::Nothing;
        }
    }

    ///////////////////////////////
    // UngroupNodeGroupMenuAction
    ///////////////////////////////

    UngroupNodeGroupMenuAction::UngroupNodeGroupMenuAction(QObject* parent)
        : NodeGroupContextMenuAction("Ungroup", parent)
    {

    }

    void UngroupNodeGroupMenuAction::RefreshAction()
    {
        const AZ::EntityId& targetId = GetTargetId();

        setEnabled((GraphUtils::IsNodeGroup(targetId) || GraphUtils::IsCollapsedNodeGroup(targetId)));
    }

    ContextMenuAction::SceneReaction UngroupNodeGroupMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const GraphId& graphId = GetGraphId();
        AZ::EntityId groupTarget = GetTargetId();

        if (GraphUtils::UngroupGroup(graphId, groupTarget))
        {
            return SceneReaction::PostUndo;
        }
        else
        {
            return SceneReaction::Nothing;
        }
    }

    ////////////////////////////////
    // CollapseNodeGroupMenuAction
    ////////////////////////////////

    CollapseNodeGroupMenuAction::CollapseNodeGroupMenuAction(QObject* parent)
        : NodeGroupContextMenuAction("Collapse", parent)
    {
        setToolTip("Collapses the selected group");
    }

    void CollapseNodeGroupMenuAction::RefreshAction()
    {
        const AZ::EntityId& targetId = GetTargetId();

        setEnabled(GraphUtils::IsNodeGroup(targetId));
    }

    ContextMenuAction::SceneReaction CollapseNodeGroupMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const AZ::EntityId& targetId = GetTargetId();

        NodeGroupRequestBus::Event(targetId, &NodeGroupRequests::CollapseGroup);

        return SceneReaction::PostUndo;
    }

    //////////////////////////////
    // ExpandNodeGroupMenuAction
    //////////////////////////////

    ExpandNodeGroupMenuAction::ExpandNodeGroupMenuAction(QObject* parent)
        : NodeGroupContextMenuAction("Expand", parent)
    {
        setToolTip("Expands the selected group");
    } 

    void ExpandNodeGroupMenuAction::RefreshAction()
    {
        const AZ::EntityId& targetId = GetTargetId();
        setEnabled(GraphUtils::IsCollapsedNodeGroup(targetId));
    }

    ContextMenuAction::SceneReaction ExpandNodeGroupMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const AZ::EntityId& targetId = GetTargetId();
        CollapsedNodeGroupRequestBus::Event(targetId, &CollapsedNodeGroupRequests::ExpandGroup);

        return SceneReaction::PostUndo;
    }

    /////////////////////////////
    // EditGroupTitleMenuAction
    /////////////////////////////

    EditGroupTitleMenuAction::EditGroupTitleMenuAction(QObject* parent)
        : NodeGroupContextMenuAction("Edit group title", parent)
    {
        setToolTip("Edits the selected group title");
    }

    void EditGroupTitleMenuAction::RefreshAction()
    {
        const AZ::EntityId& targetId = GetTargetId();

        setEnabled(GraphUtils::IsNodeGroup(targetId));
    }

    ContextMenuAction::SceneReaction EditGroupTitleMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const AZ::EntityId& targetId = GetTargetId();

        CommentUIRequestBus::Event(targetId, &CommentUIRequests::SetEditable, true);

        return SceneReaction::Nothing;
    }
}
