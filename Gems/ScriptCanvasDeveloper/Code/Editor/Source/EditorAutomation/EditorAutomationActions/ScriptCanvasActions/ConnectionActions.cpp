/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

#include <GraphCanvas/Utils/ConversionUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ConnectionActions.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>

namespace ScriptCanvasDeveloper
{
    //////////////////////
    // CoupleNodesAction
    //////////////////////

    CoupleNodesAction::CoupleNodesAction(GraphCanvas::NodeId nodeToPickUp, GraphCanvas::ConnectionType connectionType, GraphCanvas::NodeId coupleTarget)
        : m_nodeToPickUp(nodeToPickUp)
        , m_connectionType(connectionType)
        , m_targetNode(coupleTarget)
    {
    }

    bool CoupleNodesAction::IsMissingPrecondition()
    {
        QGraphicsItem* sourceGraphicsItem = nullptr;
        GraphCanvas::VisualRequestBus::EventResult(sourceGraphicsItem, m_nodeToPickUp, &GraphCanvas::VisualRequests::AsGraphicsItem);

        if (sourceGraphicsItem)
        {
            m_pickUpRect = sourceGraphicsItem->sceneBoundingRect();
        }

        QGraphicsItem* targetGraphicsItem = nullptr;
        GraphCanvas::VisualRequestBus::EventResult(targetGraphicsItem, m_targetNode, &GraphCanvas::VisualRequests::AsGraphicsItem);

        if (targetGraphicsItem)
        {
            m_targetRect = targetGraphicsItem->sceneBoundingRect();
        }

        m_sceneRect = m_pickUpRect;
        m_sceneRect |= m_targetRect;

        // Provide some extra spacing just to provide some safety
        m_sceneRect = m_sceneRect.adjusted(-m_sceneRect.width() * 0.25f, -m_sceneRect.height() * 0.25f, m_sceneRect.width() * 0.25f, m_sceneRect.height() * 0.25f);

        GraphCanvas::GraphId graphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphId, m_nodeToPickUp, &GraphCanvas::SceneMemberRequests::GetScene);

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphId, &GraphCanvas::SceneRequests::GetViewId);

        QRectF viewableArea;
        GraphCanvas::ViewRequestBus::EventResult(viewableArea, viewId, &GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

        return !viewableArea.contains(m_sceneRect);
    }

    EditorAutomationAction* CoupleNodesAction::GenerateMissingPreconditionAction()
    {
        GraphCanvas::GraphId graphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphId, m_nodeToPickUp, &GraphCanvas::SceneMemberRequests::GetScene);

        return aznew EnsureSceneRectVisibleAction(graphId, m_sceneRect);
    }

    AZStd::vector< GraphCanvas::ConnectionId > CoupleNodesAction::GetConnectionIds() const
    {
        return m_connections;
    }

    void CoupleNodesAction::OnConnectionAdded(const AZ::EntityId& connectionId)
    {
        m_connections.emplace_back(connectionId);
    }

    void CoupleNodesAction::SetupAction()
    {
        ClearActionQueue();

        QPointF mouseStartPoint = QPointF(m_pickUpRect.center().x(), m_pickUpRect.top() + 5);

        GraphCanvas::GraphId graphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphId, m_nodeToPickUp, &GraphCanvas::SceneMemberRequests::GetScene);

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphId, &GraphCanvas::SceneRequests::GetViewId);

        GraphCanvas::ViewRequests* viewRequests = GraphCanvas::ViewRequestBus::FindFirstHandler(viewId);

        if (viewRequests)
        {
            QPoint initialMousePosition = GraphCanvas::ConversionUtils::AZToQPoint(viewRequests->MapToGlobal(GraphCanvas::ConversionUtils::QPointToVector(mouseStartPoint))).toPoint();
            AddAction(aznew MouseMoveAction(initialMousePosition));
            AddAction(aznew PressMouseButtonAction(Qt::MouseButton::LeftButton));

            QPointF targetPoint = m_targetRect.center();
            QPointF startPoint = m_pickUpRect.center();

            // Depending on which side of the pick-up node we want to connect, we need to target a different edge of the target node.
            if (m_connectionType == GraphCanvas::ConnectionType::CT_Input)
            {
                targetPoint = QPointF(m_targetRect.right(), targetPoint.y());
            }
            else if (m_connectionType == GraphCanvas::ConnectionType::CT_Output)
            {
                targetPoint = QPointF(m_targetRect.left(), targetPoint.y());
            }

            QPointF lineDelta = QPointF(targetPoint.x() - startPoint.x(), targetPoint.y() - startPoint.y());

            QPointF targetMousePosition = QPointF(mouseStartPoint.x() + lineDelta.x(), mouseStartPoint.y() + lineDelta.y());

            AddAction(aznew MouseMoveAction(GraphCanvas::ConversionUtils::AZToQPoint(viewRequests->MapToGlobal(GraphCanvas::ConversionUtils::QPointToVector(targetMousePosition))).toPoint()));

            GraphCanvas::EditorId editorId;
            GraphCanvas::SceneRequestBus::EventResult(editorId, graphId, &GraphCanvas::SceneRequests::GetEditorId);

            AZStd::chrono::milliseconds coupleDuration;
            GraphCanvas::AssetEditorSettingsRequestBus::EventResult(coupleDuration, editorId, &GraphCanvas::AssetEditorSettingsRequests::GetDragCouplingTime);

            // Double the couple duration to be safe.
            coupleDuration = coupleDuration + coupleDuration;

            AddAction(aznew DelayAction(coupleDuration));
            AddAction(aznew MouseMoveAction(initialMousePosition));
            AddAction(aznew ReleaseMouseButtonAction(Qt::MouseButton::LeftButton));
            AddAction(aznew DelayAction(AZStd::chrono::milliseconds(250)));
        }

        m_connections.clear();
        GraphCanvas::SceneNotificationBus::Handler::BusConnect(graphId);

        CompoundAction::SetupAction();
    }

    void CoupleNodesAction::OnActionsComplete()
    {
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
    }

    ///////////////////////////
    // ConnectEndpointsAction
    ///////////////////////////

    ConnectEndpointsAction::ConnectEndpointsAction(GraphCanvas::Endpoint startEndpoint, GraphCanvas::Endpoint targetEndpoint)
        : m_startEndpoint(startEndpoint)
        , m_targetEndpoint(targetEndpoint)
    {
        QPointF startScenePoint;
        GraphCanvas::SlotUIRequestBus::EventResult(startScenePoint, startEndpoint.m_slotId, &GraphCanvas::SlotUIRequests::GetPinCenter);

        QPointF targetScenePoint;
        GraphCanvas::SlotUIRequestBus::EventResult(targetScenePoint, targetEndpoint.m_slotId, &GraphCanvas::SlotUIRequests::GetPinCenter);

        GraphCanvas::GraphId graphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphId, startEndpoint.m_nodeId, &GraphCanvas::SceneMemberRequests::GetScene);

        AddAction(aznew SceneMouseDragAction(graphId, startScenePoint, targetScenePoint, Qt::MouseButton::LeftButton));
    }

    GraphCanvas::ConnectionId ConnectEndpointsAction::GetConnectionId() const
    {
        return m_connectionId;
    }

    void ConnectEndpointsAction::OnActionsComplete()
    {
        GraphCanvas::SlotRequestBus::EventResult(m_connectionId, m_startEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetLastConnection);
    }
}
