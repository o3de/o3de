/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QToolButton>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/Automation/AutomationIds.h>
#include <GraphCanvas/Editor/Automation/AutomationUtils.h>

#include <GraphCanvas/Widgets/NodePalette/Model/NodePaletteSortFilterProxyModel.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteTreeView.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/CreateElementsActions.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/WidgetActions.h>

namespace ScriptCanvas::Developer
{
    ////////////////////////////////
    // CreateNodeFromPaletteAction
    ////////////////////////////////

    CreateNodeFromPaletteAction::CreateNodeFromPaletteAction(GraphCanvas::NodePaletteWidget* paletteWidget, GraphCanvas::GraphId graphId, QString nodeName, QPointF scenePoint)
        : m_graphId(graphId)
        , m_scenePoint(scenePoint)
        , m_nodeName(nodeName)
        , m_paletteWidget(paletteWidget)
    {
    }

    CreateNodeFromPaletteAction::CreateNodeFromPaletteAction(GraphCanvas::NodePaletteWidget* paletteWidget, GraphCanvas::GraphId graphId, QString nodeName, GraphCanvas::ConnectionId connectionId)
        : m_spliceTarget(connectionId)
        , m_graphId(graphId)
        , m_nodeName(nodeName)
        , m_paletteWidget(paletteWidget)
    {
        if (GraphCanvas::GraphUtils::IsConnection(connectionId))
        {
            QPainterPath outlinePath;
            GraphCanvas::SceneMemberUIRequestBus::EventResult(outlinePath, connectionId, &GraphCanvas::SceneMemberUIRequests::GetOutline);

            m_scenePoint = outlinePath.pointAtPercent(0.5);

            GraphCanvas::ConnectionRequestBus::EventResult(m_sourceEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);
            GraphCanvas::ConnectionRequestBus::EventResult(m_targetEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);
        }
        else
        {
            m_scenePoint = QPointF(0, 0);
        }
    }

    bool CreateNodeFromPaletteAction::IsMissingPrecondition()
    {
        if (!m_centerOnScene && !m_writeToSearchFilter)
        {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_graphId, &GraphCanvas::SceneRequests::GetViewId);

            AZ::Vector2 viewCenter;
            GraphCanvas::ViewRequestBus::EventResult(viewCenter, viewId, &GraphCanvas::ViewRequests::GetViewSceneCenter);

            QRectF viewRect = QRectF(GraphCanvas::ConversionUtils::AZToQPoint(viewCenter), GraphCanvas::ConversionUtils::AZToQPoint(viewCenter));
            viewRect.adjust(-2, -2, 2, 2);

            m_centerOnScene = !viewRect.contains(m_scenePoint);
            m_writeToSearchFilter = (m_paletteWidget->GetSearchFilter()->text().compare(m_nodeName, Qt::CaseInsensitive) != 0);

            return m_centerOnScene || m_writeToSearchFilter;
        }
        else
        {
            return false;
        }
    }

    EditorAutomationAction* CreateNodeFromPaletteAction::GenerateMissingPreconditionAction()
    {
        CompoundAction* compoundAction = aznew CompoundAction();

        if (m_centerOnScene)
        {
            compoundAction->AddAction(aznew CenterOnScenePointAction(m_graphId, m_scenePoint));
        }

        if (m_writeToSearchFilter)
        {
            compoundAction->AddAction(aznew WriteToLineEditAction(m_paletteWidget->GetSearchFilter(), m_nodeName));
#if defined(AZ_COMPILER_MSVC)
            compoundAction->AddAction(aznew TypeCharAction(VK_RETURN));
#endif
            compoundAction->AddAction(aznew DelayAction(AZStd::chrono::milliseconds(400)));
        }

        return compoundAction;
    }

    void CreateNodeFromPaletteAction::SetupAction()
    {
        ClearActionQueue();

        GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphId);
        m_createdNodeId.SetInvalid();

        GraphCanvas::GraphCanvasTreeItem* paletteItem = m_paletteWidget->FindItemWithName(m_nodeName);

        if (paletteItem)
        {
            QModelIndex modelIndex = paletteItem->GetIndexFromModel();

            GraphCanvas::NodePaletteSortFilterProxyModel* proxyModel = m_paletteWidget->GetFilterModel();
            QModelIndex filteredParentIndex = proxyModel->mapFromSource(paletteItem->GetParent()->GetIndexFromModel());
            QModelIndex filteredElementIndex = proxyModel->mapFromSource(modelIndex);

            AddAction(aznew MoveMouseToViewRowAction(m_paletteWidget->GetTreeView(), filteredElementIndex.row(), filteredParentIndex));
            AddAction(aznew PressMouseButtonAction(Qt::MouseButton::LeftButton));
            AddAction(aznew ProcessUserEventsAction());

            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_graphId, &GraphCanvas::SceneRequests::GetViewId);

            AZ::Vector2 screenPoint;
            GraphCanvas::ViewRequestBus::EventResult(screenPoint, viewId, &GraphCanvas::ViewRequests::MapToGlobal, GraphCanvas::ConversionUtils::QPointToVector(m_scenePoint));

            AddAction(aznew MouseMoveAction(GraphCanvas::ConversionUtils::AZToQPoint(screenPoint).toPoint()));
            AddAction(aznew ProcessUserEventsAction());

            // If we are splicing, we need to hold a little bit before we release the button.
            if (m_spliceTarget.IsValid())
            {
                AZStd::chrono::milliseconds connectionDelay;
                GraphCanvas::AssetEditorSettingsRequestBus::EventResult(connectionDelay, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::AssetEditorSettingsRequests::GetDropConnectionSpliceTime);

                // Give it some buffer room on the delay before we release.
                connectionDelay = connectionDelay + connectionDelay / 2;

                AddAction(aznew ProcessUserEventsAction(connectionDelay));
            }

            AddAction(aznew ReleaseMouseButtonAction(Qt::MouseButton::LeftButton));
            AddAction(aznew ProcessUserEventsAction());
            AddAction(aznew MouseMoveAction(GraphCanvas::ConversionUtils::AZToQPoint(screenPoint + AZ::Vector2(1,1)).toPoint()));
            AddAction(aznew ProcessUserEventsAction());
        }

        CompoundAction::SetupAction();
    }

    GraphCanvas::NodeId CreateNodeFromPaletteAction::GetCreatedNodeId() const
    {
        return GraphCanvas::GraphUtils::FindOutermostNode(m_createdNodeId);
    }

    void CreateNodeFromPaletteAction::OnNodeAdded(const AZ::EntityId& nodeId, bool)
    {
        if (!m_createdNodeId.IsValid())
        {
            m_createdNodeId = nodeId;
        }
    }

    ActionReport CreateNodeFromPaletteAction::GenerateReport() const
    {
        if (!m_createdNodeId.IsValid())
        {
            return AZ::Failure<AZStd::string>(AZStd::string::format("Failed to create %s", m_nodeName.toUtf8().data()));
        }
        else if (m_spliceTarget.IsValid())
        {
            GraphCanvas::NodeId nodeId = GetCreatedNodeId();

            GraphCanvas::ConnectionId connectionId;
            GraphCanvas::SlotRequestBus::EventResult(connectionId, m_sourceEndpoint.m_slotId, &GraphCanvas::SlotRequests::GetLastConnection);

            GraphCanvas::Endpoint otherEndpoint;
            GraphCanvas::ConnectionRequestBus::EventResult(otherEndpoint, connectionId, &GraphCanvas::ConnectionRequests::FindOtherEndpoint, m_sourceEndpoint);

            if (!otherEndpoint.IsValid()
                || otherEndpoint.GetNodeId() != nodeId)
            {
                return AZ::Failure<AZStd::string>(AZStd::string::format("Spliced connection failed to create connection from source Endpoint to %s node", m_nodeName.toUtf8().data()));
            }

            connectionId.SetInvalid();
            GraphCanvas::SlotRequestBus::EventResult(connectionId, m_targetEndpoint.m_slotId, &GraphCanvas::SlotRequests::GetLastConnection);

            otherEndpoint = GraphCanvas::Endpoint();
            GraphCanvas::ConnectionRequestBus::EventResult(otherEndpoint, connectionId, &GraphCanvas::ConnectionRequests::FindOtherEndpoint, m_targetEndpoint);

            if (!otherEndpoint.IsValid()
                || otherEndpoint.GetNodeId() != nodeId)
            {
                return AZ::Failure<AZStd::string>(AZStd::string::format("Spliced connection failed to create connection from target Endpoint to %s node", m_nodeName.toUtf8().data()));
            }
        }

        return CompoundAction::GenerateReport();
    }

    void CreateNodeFromPaletteAction::OnActionsComplete()
    {
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();

        m_centerOnScene = false;
        m_writeToSearchFilter = false;
    }

    ////////////////////////////////////////
    // CreateCategoryFromNodePaletteAction
    ////////////////////////////////////////

    CreateCategoryFromNodePaletteAction::CreateCategoryFromNodePaletteAction(GraphCanvas::NodePaletteWidget* paletteWidget, GraphCanvas::GraphId graphId, QString category, QPointF scenePoint)
        : m_graphId(graphId)
        , m_scenePoint(scenePoint)
        , m_categoryName(category)
        , m_paletteWidget(paletteWidget)
    {
    }

    bool CreateCategoryFromNodePaletteAction::IsMissingPrecondition()
    {
        return m_paletteWidget->GetSearchFilter()->text().compare(m_categoryName, Qt::CaseInsensitive) != 0;
    }

    EditorAutomationAction* CreateCategoryFromNodePaletteAction::GenerateMissingPreconditionAction()
    {
        CompoundAction* compoundAction = aznew CompoundAction();

        compoundAction->AddAction(aznew WriteToLineEditAction(m_paletteWidget->GetSearchFilter(), m_categoryName));
#if defined(AZ_COMPILER_MSVC)
        compoundAction->AddAction(aznew TypeCharAction(VK_RETURN));
#endif
        compoundAction->AddAction(aznew ProcessUserEventsAction());

        return compoundAction;
    }

    void CreateCategoryFromNodePaletteAction::SetupAction()
    {
        ClearActionQueue();

        GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphId);

        GraphCanvas::GraphCanvasTreeItem* rootItem = m_paletteWidget->FindItemWithName(m_categoryName);

        AZStd::vector<AZStd::pair<int, QModelIndex>> creationIndexes;

        if (rootItem)
        {
            AZStd::unordered_set< GraphCanvas::GraphCanvasTreeItem* > unexploredItems = { rootItem };

            while (!unexploredItems.empty())
            {
                GraphCanvas::GraphCanvasTreeItem* currentItem = (*unexploredItems.begin());
                unexploredItems.erase(unexploredItems.begin());

                if (currentItem)
                {
                    if (currentItem->GetChildCount() == 0)
                    {
                        QModelIndex currentRow = m_paletteWidget->GetFilterModel()->mapFromSource(currentItem->GetIndexFromModel());
                        QModelIndex parentIndex = m_paletteWidget->GetFilterModel()->mapFromSource(currentItem->GetParent()->GetIndexFromModel());

                        if (currentRow.isValid() && parentIndex.isValid())
                        {
                            creationIndexes.emplace_back(currentRow.row(), parentIndex);
                        }
                    }
                    else
                    {
                        for (int i = 0; i < currentItem->GetChildCount(); ++i)
                        {
                            unexploredItems.insert(currentItem->FindChildByRow(i));
                        }
                    }
                }
            }

            m_expectedCreations = aznumeric_cast<int>(creationIndexes.size());

            for (int i = 0; i < creationIndexes.size(); ++i)
            {
                if (i == 0)
                {
#if defined(AZ_COMPILER_MSVC)
                    AddAction(aznew KeyPressAction(VK_CONTROL));
#endif
                }

                const auto& creationPair = creationIndexes[i];

                AddAction(aznew MoveMouseToViewRowAction(m_paletteWidget->GetTreeView(), creationPair.first, creationPair.second));
                AddAction(aznew ProcessUserEventsAction());

                if (i + 1 >= creationIndexes.size())
                {
                    AddAction(aznew PressMouseButtonAction(Qt::MouseButton::LeftButton));
                }
                else
                {
                    AddAction(aznew MouseClickAction(Qt::MouseButton::LeftButton));
                }

                AddAction(aznew ProcessUserEventsAction());
            }

            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_graphId, &GraphCanvas::SceneRequests::GetViewId);

            AZ::Vector2 screenPoint;
            GraphCanvas::ViewRequestBus::EventResult(screenPoint, viewId, &GraphCanvas::ViewRequests::MapToGlobal, GraphCanvas::ConversionUtils::QPointToVector(m_scenePoint));

            AddAction(aznew MouseMoveAction(GraphCanvas::ConversionUtils::AZToQPoint(screenPoint).toPoint()));

            AddAction(aznew ProcessUserEventsAction());

            AddAction(aznew ReleaseMouseButtonAction(Qt::MouseButton::LeftButton));

            AddAction(aznew ProcessUserEventsAction());

            AddAction(aznew MouseMoveAction(GraphCanvas::ConversionUtils::AZToQPoint(screenPoint + AZ::Vector2(1, 1)).toPoint()));
            AddAction(aznew ProcessUserEventsAction());

#if defined(AZ_COMPILER_MSVC)
            AddAction(aznew KeyReleaseAction(VK_CONTROL));
#endif
            AddAction(aznew ProcessUserEventsAction());
        }

        CompoundAction::SetupAction();
    }

    void CreateCategoryFromNodePaletteAction::OnNodeAdded(const AZ::EntityId& nodeId, bool)
    {
        m_createdNodeIds.emplace_back(nodeId);
    }

    AZStd::vector< GraphCanvas::NodeId > CreateCategoryFromNodePaletteAction::GetCreatedNodes() const
    {
        return m_createdNodeIds;
    }

    void CreateCategoryFromNodePaletteAction::OnActionsComplete()
    {
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();

        AZStd::vector< GraphCanvas::NodeId > nodes = m_createdNodeIds;
        AZStd::unordered_set< GraphCanvas::NodeId > rootNodes;

        m_createdNodeIds.clear();

        for (GraphCanvas::NodeId nodeId : nodes)
        {
            GraphCanvas::NodeId outermostNode = GraphCanvas::GraphUtils::FindOutermostNode(nodeId);
            rootNodes.insert(outermostNode);
        }

        m_createdNodeIds.insert(m_createdNodeIds.begin(), rootNodes.begin(), rootNodes.end());
    }

    ActionReport CreateCategoryFromNodePaletteAction::GenerateReport() const
    {
        if (m_createdNodeIds.size() != m_expectedCreations)
        {
            return AZ::Failure<AZStd::string>(AZStd::string::format("Failed to create all nodes. %i expected, %i created", aznumeric_cast<int>(m_createdNodeIds.size()), m_expectedCreations));
        }

        return CompoundAction::GenerateReport();
    }

    ////////////////////////////////////
    // CreateNodeFromContextMenuAction
    ////////////////////////////////////

    CreateNodeFromContextMenuAction::CreateNodeFromContextMenuAction(GraphCanvas::GraphId graphId, QString nodeName, QPointF scenePoint)
        : m_graphId(graphId)
        , m_scenePoint(scenePoint)
        , m_nodeName(nodeName)
    {
    }
    
    CreateNodeFromContextMenuAction::CreateNodeFromContextMenuAction(GraphCanvas::GraphId graphId, QString nodeName, AZ::EntityId connectionId)
        : m_graphId(graphId)
        , m_nodeName(nodeName)
    {
        if (GraphCanvas::GraphUtils::IsConnection(connectionId))
        {
            QPainterPath outlinePath;
            GraphCanvas::SceneMemberUIRequestBus::EventResult(outlinePath, connectionId, &GraphCanvas::SceneMemberUIRequests::GetOutline);

            m_scenePoint = outlinePath.pointAtPercent(0.5);

            m_spliceTarget = connectionId;

            GraphCanvas::ConnectionRequestBus::EventResult(m_sourceEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);
            GraphCanvas::ConnectionRequestBus::EventResult(m_targetEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);
        }
        else
        {
            m_scenePoint = QPointF(0,0);
        }
    }

    bool CreateNodeFromContextMenuAction::IsMissingPrecondition()
    {
        return m_centerOnScene;
    }

    EditorAutomationAction* CreateNodeFromContextMenuAction::GenerateMissingPreconditionAction()
    {
        m_centerOnScene = false;

        return aznew CenterOnScenePointAction(m_graphId, m_scenePoint);
    }

    void CreateNodeFromContextMenuAction::SetupAction()
    {
        ClearActionQueue();

        m_createdNodeId.SetInvalid();

        GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphId);

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_graphId, &GraphCanvas::SceneRequests::GetViewId);

        AZ::Vector2 screenPoint;
        GraphCanvas::ViewRequestBus::EventResult(screenPoint, viewId, &GraphCanvas::ViewRequests::MapToGlobal, GraphCanvas::ConversionUtils::QPointToVector(m_scenePoint));

        AddAction(aznew MouseMoveAction(GraphCanvas::ConversionUtils::AZToQPoint(screenPoint).toPoint()));

        AddAction(aznew MouseClickAction(Qt::MouseButton::RightButton));
        
        AddAction(aznew ProcessUserEventsAction());

        AddAction(aznew TypeStringAction(m_nodeName));
        AddAction(aznew ProcessUserEventsAction());
#if defined(AZ_COMPILER_MSVC)
        AddAction(aznew TypeCharAction(VK_RETURN));
#endif
        AddAction(aznew ProcessUserEventsAction());

        CompoundAction::SetupAction();
    }

    void CreateNodeFromContextMenuAction::OnNodeAdded(const AZ::EntityId& nodeId, bool)
    {
        m_createdNodeId = nodeId;
    }

    AZ::EntityId CreateNodeFromContextMenuAction::GetCreatedNodeId() const
    {
        return m_createdNodeId;
    }

    void CreateNodeFromContextMenuAction::OnActionsComplete()
    {
        m_createdNodeId = GraphCanvas::GraphUtils::FindOutermostNode(m_createdNodeId);

        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();

        m_centerOnScene = true;
    }

    ActionReport CreateNodeFromContextMenuAction::GenerateReport() const
    {
        if (!m_createdNodeId.IsValid())
        {
            return AZ::Failure<AZStd::string>(AZStd::string::format("Failed to create Node %s.", m_nodeName.toUtf8().data()));
        }
        else if (m_spliceTarget.IsValid())
        {
            GraphCanvas::NodeId nodeId = GetCreatedNodeId();

            GraphCanvas::ConnectionId connectionId;
            GraphCanvas::SlotRequestBus::EventResult(connectionId, m_sourceEndpoint.m_slotId, &GraphCanvas::SlotRequests::GetLastConnection);

            GraphCanvas::Endpoint otherEndpoint;
            GraphCanvas::ConnectionRequestBus::EventResult(otherEndpoint, connectionId, &GraphCanvas::ConnectionRequests::FindOtherEndpoint, m_sourceEndpoint);

            if (!otherEndpoint.IsValid()
                || otherEndpoint.GetNodeId() != nodeId)
            {
                return AZ::Failure<AZStd::string>(AZStd::string::format("Spliced connection failed to create connection from source Endpoint to %s node", m_nodeName.toUtf8().data()));
            }

            connectionId.SetInvalid();
            GraphCanvas::SlotRequestBus::EventResult(connectionId, m_targetEndpoint.m_slotId, &GraphCanvas::SlotRequests::GetLastConnection);

            otherEndpoint = GraphCanvas::Endpoint();
            GraphCanvas::ConnectionRequestBus::EventResult(otherEndpoint, connectionId, &GraphCanvas::ConnectionRequests::FindOtherEndpoint, m_targetEndpoint);

            if (!otherEndpoint.IsValid()
                || otherEndpoint.GetNodeId() != nodeId)
            {
                return AZ::Failure<AZStd::string>(AZStd::string::format("Spliced connection failed to create connection from target Endpoint to %s node", m_nodeName.toUtf8().data()));
            }
        }

        return CompoundAction::GenerateReport();
    }

    /////////////////////////////////
    // CreateNodeFromProposalAction
    /////////////////////////////////

    CreateNodeFromProposalAction::CreateNodeFromProposalAction(GraphCanvas::GraphId graphId, GraphCanvas::Endpoint endpoint, QString nodeName)
        : m_graphId(graphId)
        , m_endpoint(endpoint)
        , m_nodeName(nodeName)
    {
        AZ::Vector2 stepSize = GraphCanvas::GraphUtils::FindMinorStep(graphId);
        
        GraphCanvas::SlotUIRequestBus::EventResult(m_scenePoint, m_endpoint.GetSlotId(), &GraphCanvas::SlotUIRequests::GetConnectionPoint);

        QPointF jutDirection;
        GraphCanvas::SlotUIRequestBus::EventResult(jutDirection, m_endpoint.GetSlotId(), &GraphCanvas::SlotUIRequests::GetJutDirection);

        AZ::Vector2 stepDirection = AZ::Vector2::CreateZero();

        stepDirection.SetX(static_cast<float>(jutDirection.x() * stepSize.GetX()));
        stepDirection.SetY(static_cast<float>(jutDirection.y() * stepSize.GetY()));

        m_scenePoint.setX(m_scenePoint.x() + stepDirection.GetX() * 2);
    }

    CreateNodeFromProposalAction::CreateNodeFromProposalAction(GraphCanvas::GraphId graphId, GraphCanvas::Endpoint endpoint, QString nodeName, QPointF scenePoint)
        : m_graphId(graphId)
        , m_endpoint(endpoint)
        , m_scenePoint(scenePoint)
        , m_nodeName(nodeName)
    {
    }

    bool CreateNodeFromProposalAction::IsMissingPrecondition()
    {
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_graphId, &GraphCanvas::SceneRequests::GetViewId);

        QRectF viewableBounds;
        GraphCanvas::ViewRequestBus::EventResult(viewableBounds, viewId, &GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

        QPointF pinCenter;
        GraphCanvas::SlotUIRequestBus::EventResult(pinCenter, m_endpoint.GetSlotId(), &GraphCanvas::SlotUIRequests::GetPinCenter);

        QRectF sceneRect(pinCenter, m_scenePoint);
        sceneRect.adjust(-10, -10, 10, 10);

        return !viewableBounds.isEmpty() && !viewableBounds.contains(sceneRect);
    }

    EditorAutomationAction* CreateNodeFromProposalAction::GenerateMissingPreconditionAction()
    {
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_graphId, &GraphCanvas::SceneRequests::GetViewId);

        QRectF viewableBounds;
        GraphCanvas::ViewRequestBus::EventResult(viewableBounds, viewId, &GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

        QPointF pinCenter;
        GraphCanvas::SlotUIRequestBus::EventResult(pinCenter, m_endpoint.GetSlotId(), &GraphCanvas::SlotUIRequests::GetPinCenter);

        QRectF sceneRect(pinCenter, m_scenePoint);

        return aznew EnsureSceneRectVisibleAction(m_graphId, sceneRect);
    }

    void CreateNodeFromProposalAction::SetupAction()
    {
        ClearActionQueue();

        m_createdNodeId.SetInvalid();

        QPointF pinCenter;
        GraphCanvas::SlotUIRequestBus::EventResult(pinCenter, m_endpoint.GetSlotId(), &GraphCanvas::SlotUIRequests::GetPinCenter);

        AddAction(aznew SceneMouseDragAction(m_graphId, pinCenter, m_scenePoint));
        AddAction(aznew ProcessUserEventsAction());
        AddAction(aznew TypeStringAction(m_nodeName));
        AddAction(aznew ProcessUserEventsAction());
#if defined(AZ_COMPILER_MSVC)
        AddAction(aznew TypeCharAction(VK_RETURN));
#endif
        AddAction(aznew ProcessUserEventsAction());

        GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphId);

        CompoundAction::SetupAction();
    }

    void CreateNodeFromProposalAction::OnNodeAdded(const AZ::EntityId& nodeId, bool)
    {
        m_createdNodeId = nodeId;
    }

    AZ::EntityId CreateNodeFromProposalAction::GetCreatedNodeId() const
    {
        return GraphCanvas::GraphUtils::FindOutermostNode(m_createdNodeId);
    }

    AZ::EntityId CreateNodeFromProposalAction::GetConnectionId() const
    {
        GraphCanvas::ConnectionId lastConnectionId;
        GraphCanvas::SlotRequestBus::EventResult(lastConnectionId, m_endpoint.m_slotId, &GraphCanvas::SlotRequests::GetLastConnection);

        return lastConnectionId;
    }

    ActionReport CreateNodeFromProposalAction::GenerateReport() const
    {
        if (!m_createdNodeId.IsValid())
        {
            return AZ::Failure<AZStd::string>(AZStd::string::format("Failed to create Node(%s)", m_nodeName.toUtf8().data()));
        }
        else
        {
            GraphCanvas::ConnectionId lastConnectionId;
            GraphCanvas::SlotRequestBus::EventResult(lastConnectionId, m_endpoint.m_slotId, &GraphCanvas::SlotRequests::GetLastConnection);

            GraphCanvas::Endpoint otherEndpoint;
            GraphCanvas::ConnectionRequestBus::EventResult(otherEndpoint, lastConnectionId, &GraphCanvas::ConnectionRequests::FindOtherEndpoint, m_endpoint);

            if (otherEndpoint.IsValid())
            {
                AZ::EntityId nodeId = GetCreatedNodeId();

                if (otherEndpoint.GetNodeId() != nodeId)
                {
                    return AZ::Failure<AZStd::string>(AZStd::string::format("Failed to create connection to Node(%s)", m_nodeName.toUtf8().data()));
                }
            }
            else
            {
                return AZ::Failure<AZStd::string>(AZStd::string::format("Failed to create connection to Node(%s)", m_nodeName.toUtf8().data()));
            }
        }

        return CompoundAction::GenerateReport();
    }

    void CreateNodeFromProposalAction::OnActionsComplete()
    {
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
    }

    //////////////////////
    // CreateGroupAction
    //////////////////////

    CreateGroupAction::CreateGroupAction(GraphCanvas::EditorId editorId, GraphCanvas::GraphId graphId, CreationType creationType)
        : m_editorId(editorId)
        , m_graphId(graphId)
        , m_creationType(creationType)
    {
        if (m_creationType == CreationType::Hotkey)
        {
            SetupHotkeyAction();
        }
    }

    void CreateGroupAction::SetupAction()
    {
        GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphId);

        m_createdGroup.SetInvalid();

        if (m_creationType == CreationType::Toolbar)
        {
            SetupToolbarAction();
        }

        CompoundAction::SetupAction();
    }

    void CreateGroupAction::OnNodeAdded(const AZ::EntityId& groupId, bool)
    {
        m_createdGroup = groupId;
    }

    AZ::EntityId CreateGroupAction::GetCreatedGroupId() const
    {
        return m_createdGroup;
    }

    ActionReport CreateGroupAction::GenerateReport() const
    {
        if (!m_createdGroup.IsValid())
        {
            if (m_creationType == CreationType::Hotkey)
            {
                return AZ::Failure<AZStd::string>("Failed to create Group using HotKey");
            }
            else if (m_creationType == CreationType::Toolbar)
            {
                return AZ::Failure<AZStd::string>("Failed to create Group using Toolbar");
            }
        }

        return CompoundAction::GenerateReport();
    }

    void CreateGroupAction::SetupToolbarAction()
    {
        QToolButton* createGroupButton = GraphCanvas::AutomationUtils::FindObjectById<QToolButton>(m_editorId, GraphCanvas::AutomationIds::GroupButton);

        if (createGroupButton)
        {
            QPoint clickPoint = createGroupButton->mapToGlobal(createGroupButton->rect().center());

            ClearActionQueue();
            AddAction(aznew MouseClickAction(Qt::MouseButton::LeftButton, clickPoint));
        }

        AddAction(aznew ProcessUserEventsAction());
    }

    void CreateGroupAction::SetupHotkeyAction()
    {
        ClearActionQueue();

#if defined(AZ_COMPILER_MSVC)
        AddAction(aznew KeyPressAction(VK_CONTROL));
        AddAction(aznew KeyPressAction(VK_LSHIFT));
        AddAction(aznew TypeCharAction('G'));
        AddAction(aznew ProcessUserEventsAction());
        AddAction(aznew KeyReleaseAction(VK_LSHIFT));
        AddAction(aznew KeyReleaseAction(VK_CONTROL));
#endif
    }

    void CreateGroupAction::OnActionsComplete()
    {
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
    }
}
