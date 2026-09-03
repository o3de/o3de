/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// Qt
#include <QAction>

// GraphModel
#include <GraphModel/GraphModelBus.h>

// GraphCanvas
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Utils/GraphUtils.h>

// Landscape Canvas
#include <Editor/Core/Core.h>
#include <Editor/Nodes/BaseNode.h>
#include <Editor/Nodes/RerouteNode.h>
#include <Editor/Menus/NodeContextMenu.h>

namespace LandscapeCanvasEditor
{
    namespace
    {
        bool CanCreateRerouteOnConnections(const AZStd::vector<GraphCanvas::ConnectionId>& connectionIds)
        {
            if (connectionIds.empty())
            {
                return false;
            }

            GraphCanvas::Endpoint sharedSourceEndpoint;
            for (const GraphCanvas::ConnectionId& connectionId : connectionIds)
            {
                if (!GraphCanvas::GraphUtils::IsSpliceableConnection(connectionId))
                {
                    return false;
                }

                GraphCanvas::ConnectionEndpoints endpoints;
                GraphCanvas::ConnectionRequestBus::EventResult(
                    endpoints, connectionId, &GraphCanvas::ConnectionRequests::GetEndpoints);
                if (!endpoints.m_sourceEndpoint.IsValid() || !endpoints.m_targetEndpoint.IsValid())
                {
                    return false;
                }

                GraphCanvas::SlotType slotType = GraphCanvas::SlotTypes::Invalid;
                GraphCanvas::SlotRequestBus::EventResult(
                    slotType, endpoints.m_sourceEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetSlotType);
                if (slotType != GraphCanvas::SlotTypes::DataSlot)
                {
                    return false;
                }

                if (!sharedSourceEndpoint.IsValid())
                {
                    sharedSourceEndpoint = endpoints.m_sourceEndpoint;
                }
                else if (endpoints.m_sourceEndpoint != sharedSourceEndpoint)
                {
                    return false;
                }
            }
            return true;
        }

        AZStd::vector<GraphCanvas::ConnectionId> GetConnectionsForContextTarget(
            const GraphCanvas::GraphId& graphId, const GraphCanvas::ConnectionId& targetConnectionId)
        {
            AZStd::vector<GraphCanvas::ConnectionId> selectedConnections;
            GraphCanvas::SceneRequestBus::EventResult(
                selectedConnections, graphId, &GraphCanvas::SceneRequests::GetSelectedConnections);
            if (AZStd::find(selectedConnections.begin(), selectedConnections.end(), targetConnectionId) != selectedConnections.end())
            {
                return selectedConnections;
            }
            return { targetConnectionId };
        }

        bool IsRerouteNode(const GraphCanvas::GraphId& graphId, const GraphCanvas::NodeId& nodeId)
        {
            GraphModel::NodePtr node;
            GraphModelIntegration::GraphControllerRequestBus::EventResult(
                node, graphId, &GraphModelIntegration::GraphControllerRequests::GetNodeById, nodeId);
            return node && azrtti_istypeof<LandscapeCanvas::RerouteNode>(node.get());
        }

        bool CanDissolveRerouteNode(const GraphCanvas::GraphId& graphId, const GraphCanvas::NodeId& nodeId)
        {
            if (!IsRerouteNode(graphId, nodeId))
            {
                return false;
            }

            AZStd::vector<AZ::EntityId> slotIds;
            GraphCanvas::NodeRequestBus::EventResult(slotIds, nodeId, &GraphCanvas::NodeRequests::GetSlotIds);
            bool hasConnectedInput = false;
            bool hasConnectedOutput = false;
            for (const AZ::EntityId& slotId : slotIds)
            {
                AZStd::vector<AZ::EntityId> connectionIds;
                GraphCanvas::SlotRequestBus::EventResult(
                    connectionIds, slotId, &GraphCanvas::SlotRequests::GetConnections);
                if (connectionIds.empty())
                {
                    continue;
                }

                GraphCanvas::ConnectionType connectionType = GraphCanvas::ConnectionType::CT_Invalid;
                GraphCanvas::SlotRequestBus::EventResult(
                    connectionType, slotId, &GraphCanvas::SlotRequests::GetConnectionType);
                hasConnectedInput = hasConnectedInput || connectionType == GraphCanvas::ConnectionType::CT_Input;
                hasConnectedOutput = hasConnectedOutput || connectionType == GraphCanvas::ConnectionType::CT_Output;
            }
            return hasConnectedInput && hasConnectedOutput;
        }
    } // namespace

    bool CreateRerouteOnConnections(
        const GraphCanvas::GraphId& graphId,
        const AZStd::vector<GraphCanvas::ConnectionId>& connectionIds,
        const AZ::Vector2& scenePosition)
    {
        if (!CanCreateRerouteOnConnections(connectionIds))
        {
            return false;
        }

        AZStd::vector<GraphCanvas::ConnectionEndpoints> originalConnections;
        originalConnections.reserve(connectionIds.size());
        for (const GraphCanvas::ConnectionId& connectionId : connectionIds)
        {
            GraphCanvas::ConnectionEndpoints endpoints;
            GraphCanvas::ConnectionRequestBus::EventResult(
                endpoints, connectionId, &GraphCanvas::ConnectionRequests::GetEndpoints);
            originalConnections.push_back(endpoints);
        }

        GraphModel::GraphPtr graph;
        GraphModelIntegration::GraphManagerRequestBus::BroadcastResult(
            graph, &GraphModelIntegration::GraphManagerRequests::GetGraph, graphId);
        if (!graph)
        {
            return false;
        }

        GraphCanvas::ScopedGraphUndoBatch undoBatch(graphId);
        auto rerouteNode = AZStd::make_shared<LandscapeCanvas::RerouteNode>(graph);
        AZ::Vector2 dropPosition = scenePosition;
        GraphCanvas::NodeId nodeId;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(
            nodeId, graphId, &GraphModelIntegration::GraphControllerRequests::AddNode, rerouteNode, dropPosition);
        if (!nodeId.IsValid())
        {
            return false;
        }

        GraphCanvas::VisualRequestBus::Event(nodeId, &GraphCanvas::VisualRequests::SetVisible, false);
        GraphCanvas::ConnectionSpliceConfig spliceConfig;
        spliceConfig.m_allowOpportunisticConnections = false;
        if (!GraphCanvas::GraphUtils::SpliceNodeOntoConnection(nodeId, connectionIds.front(), spliceConfig))
        {
            GraphCanvas::GraphUtils::DeleteOutermostNode(graphId, nodeId);
            return false;
        }

        bool allConnectionsCreated = spliceConfig.m_splicedSourceEndpoint.IsValid();
        if (connectionIds.size() > 1)
        {
            AZStd::unordered_set<AZ::EntityId> connectionsToDelete;
            connectionsToDelete.insert(connectionIds.begin() + 1, connectionIds.end());
            GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::Delete, connectionsToDelete);

            for (size_t connectionIndex = 1;
                 allConnectionsCreated && connectionIndex < originalConnections.size();
                 ++connectionIndex)
            {
                GraphCanvas::ConnectionId newConnectionId;
                GraphCanvas::SceneRequestBus::EventResult(
                    newConnectionId,
                    graphId,
                    &GraphCanvas::SceneRequests::CreateConnectionBetween,
                    spliceConfig.m_splicedSourceEndpoint,
                    originalConnections[connectionIndex].m_targetEndpoint);
                allConnectionsCreated = newConnectionId.IsValid();
            }
        }

        if (!allConnectionsCreated)
        {
            GraphCanvas::GraphUtils::DeleteOutermostNode(graphId, nodeId);
            for (const GraphCanvas::ConnectionEndpoints& endpoints : originalConnections)
            {
                GraphCanvas::SceneRequestBus::Event(
                    graphId,
                    &GraphCanvas::SceneRequests::CreateConnectionBetween,
                    endpoints.m_sourceEndpoint,
                    endpoints.m_targetEndpoint);
            }
            return false;
        }

        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::ClearSelection);
        GraphCanvas::VisualRequestBus::Event(nodeId, &GraphCanvas::VisualRequests::SetVisible, true);
        GraphCanvas::SceneMemberUIRequestBus::Event(nodeId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);
        GraphCanvas::SceneNotificationBus::Event(graphId, &GraphCanvas::SceneNotifications::PostCreationEvent);
        return true;
    }

    bool CreateRerouteOnSelectedConnections(const GraphCanvas::GraphId& graphId)
    {
        AZStd::vector<GraphCanvas::ConnectionId> selectedConnections;
        GraphCanvas::SceneRequestBus::EventResult(
            selectedConnections, graphId, &GraphCanvas::SceneRequests::GetSelectedConnections);
        if (!CanCreateRerouteOnConnections(selectedConnections))
        {
            return false;
        }

        QPointF midpointSum;
        for (const GraphCanvas::ConnectionId& connectionId : selectedConnections)
        {
            QPointF sourcePosition;
            QPointF targetPosition;
            GraphCanvas::ConnectionRequestBus::EventResult(
                sourcePosition, connectionId, &GraphCanvas::ConnectionRequests::GetSourcePosition);
            GraphCanvas::ConnectionRequestBus::EventResult(
                targetPosition, connectionId, &GraphCanvas::ConnectionRequests::GetTargetPosition);
            midpointSum += (sourcePosition + targetPosition) * 0.5;
        }

        const QPointF averageMidpoint = midpointSum / aznumeric_cast<qreal>(selectedConnections.size());
        return CreateRerouteOnConnections(
            graphId,
            selectedConnections,
            AZ::Vector2(aznumeric_cast<float>(averageMidpoint.x()), aznumeric_cast<float>(averageMidpoint.y())));
    }

    CreateRerouteConnectionAction::CreateRerouteConnectionAction(QObject* parent)
        : GraphCanvas::ContextMenuAction("Reroute", parent)
    {
        setToolTip("Insert a compact reroute node on this connection.");
    }

    GraphCanvas::ActionGroupId CreateRerouteConnectionAction::GetActionGroupId() const
    {
        return AZ_CRC_CE("LandscapeCanvasConnectionActions");
    }

    void CreateRerouteConnectionAction::RefreshAction(
        const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        setEnabled(CanCreateRerouteOnConnections(GetConnectionsForContextTarget(graphId, targetId)));
    }

    GraphCanvas::ContextMenuAction::SceneReaction CreateRerouteConnectionAction::TriggerAction(
        const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos)
    {
        return CreateRerouteOnConnections(graphId, GetConnectionsForContextTarget(graphId, GetTargetId()), scenePos)
            ? SceneReaction::PostUndo
            : SceneReaction::Nothing;
    }

    ConnectionContextMenu::ConnectionContextMenu(QWidget* parent)
        : GraphCanvas::ConnectionContextMenu(LandscapeCanvas::LANDSCAPE_CANVAS_EDITOR_ID, parent)
    {
        AddMenuAction(aznew CreateRerouteConnectionAction(this));
    }

    DissolveRerouteNodeAction::DissolveRerouteNodeAction(QObject* parent)
        : GraphCanvas::NodeContextMenuAction("Dissolve", parent)
    {
        setToolTip("Remove this reroute and reconnect its incoming and outgoing connections.");
    }

    void DissolveRerouteNodeAction::RefreshAction(
        const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        setVisible(IsRerouteNode(graphId, targetId));
        setEnabled(CanDissolveRerouteNode(graphId, targetId));
    }

    GraphCanvas::ContextMenuAction::SceneReaction DissolveRerouteNodeAction::TriggerAction(
        const GraphCanvas::GraphId& graphId, [[maybe_unused]] const AZ::Vector2& scenePos)
    {
        if (!CanDissolveRerouteNode(graphId, GetTargetId()))
        {
            return SceneReaction::Nothing;
        }

        GraphCanvas::SceneRequestBus::Event(
            graphId, &GraphCanvas::SceneRequests::DeleteNodeAndStitchConnections, GetTargetId());
        return SceneReaction::PostUndo;
    }

    // Select the corresponding Entities in the Editor based on the selected nodes in our scene graph
    QAction* GetNodeSelectInEditorAction(const AZ::EntityId& sceneId, QObject* parent)
    {
        // Retrieve the selected nodes in our scene
        GraphModel::NodePtrList nodeList;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeList, sceneId, &GraphModelIntegration::GraphControllerRequests::GetSelectedNodes);

        // Iterate through the selected nodes to find their corresponding vegetation entities
        AzToolsFramework::EntityIdList vegetationEntityIdsToSelect;
        for (const auto& node : nodeList)
        {
            if (!node)
            {
                continue;
            }

            auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
            if (baseNodePtr->IsVisualOnly())
            {
                continue;
            }
            vegetationEntityIdsToSelect.push_back(baseNodePtr->GetVegetationEntityId());
        }

        QAction* action = new QAction({ vegetationEntityIdsToSelect.size() > 1 ? QObject::tr("Select Entities in Editor") : QObject::tr("Select Entity in Editor") }, parent);

        QString tooltip = QObject::tr("Select the corresponding Entity/Entities in the Editor for the selected node(s) in the graph");
        action->setToolTip(tooltip);
        action->setStatusTip(tooltip);
        action->setEnabled(!vegetationEntityIdsToSelect.empty());

        QObject::connect(action,
            &QAction::triggered,
            [vegetationEntityIdsToSelect](bool)
        {
            // Set the new selection
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::SetSelectedEntities, vegetationEntityIdsToSelect);
        });

        return action;
    }

    NodeContextMenu::NodeContextMenu(const AZ::EntityId& sceneId, QWidget* parent)
        : GraphCanvas::NodeContextMenu(LandscapeCanvas::LANDSCAPE_CANVAS_EDITOR_ID, parent)
    {
        AddMenuAction(GetNodeSelectInEditorAction(sceneId, this));
        AddMenuAction(aznew DissolveRerouteNodeAction(this));
    }

    void NodeContextMenu::OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        GraphCanvas::NodeContextMenu::OnRefreshActions(graphId, targetMemberId);

        // Don't allow cut/copy/paste/duplicate on our area extender nodes because they can't
        // exist without being wrapped on an area (e.g. spawner) node
        GraphModel::NodePtr node;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(node, graphId, &GraphModelIntegration::GraphControllerRequests::GetNodeById, targetMemberId);
        if (node)
        {
            auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
            if (baseNodePtr->IsAreaExtender())
            {
                m_editActionGroup.SetCopyEnabled(false);
                m_editActionGroup.SetCutEnabled(false);
                m_editActionGroup.SetDuplicateEnabled(false);
                m_editActionGroup.SetPasteEnabled(false);
            }
        }
    }
}
