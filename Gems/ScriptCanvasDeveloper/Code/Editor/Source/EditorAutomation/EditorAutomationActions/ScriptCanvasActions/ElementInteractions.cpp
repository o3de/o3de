/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <AzCore/PlatformIncl.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

#include <GraphCanvas/Utils/ConversionUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>

#include <ScriptCanvas/Bus/RequestBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ElementInteractions.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>

namespace ScriptCanvasDeveloper
{
    /////////////////////////////
    // SelectSceneElementAction
    /////////////////////////////

    SelectSceneElementAction::SelectSceneElementAction(AZ::EntityId sceneMemberId)
        : m_sceneMemberId(sceneMemberId)
    {
        GraphCanvas::SceneMemberRequestBus::EventResult(m_graphId, m_sceneMemberId, &GraphCanvas::SceneMemberRequests::GetScene);

        GraphCanvas::SceneRequestBus::EventResult(m_viewId, m_graphId, &GraphCanvas::SceneRequests::GetViewId);

        if (!GraphCanvas::GraphUtils::IsConnection(sceneMemberId))
        {
            QRectF boundingRect;

            QGraphicsItem* graphicsItem = nullptr;
            GraphCanvas::VisualRequestBus::EventResult(graphicsItem, sceneMemberId, &GraphCanvas::VisualRequests::AsGraphicsItem);

            if (graphicsItem)
            {
                boundingRect = graphicsItem->sceneBoundingRect();
            }

            m_scenePoint = boundingRect.topLeft();
            m_scenePoint.setX(boundingRect.center().x());
            m_scenePoint.setY(m_scenePoint.y() + 5);
        }
        else
        {
            QPainterPath outlinePath;
            GraphCanvas::SceneMemberUIRequestBus::EventResult(outlinePath, sceneMemberId, &GraphCanvas::SceneMemberUIRequests::GetOutline);

            m_scenePoint = outlinePath.pointAtPercent(0.5);
        }
    }

    bool SelectSceneElementAction::IsMissingPrecondition()
    {
        QRectF viewableArea;
        GraphCanvas::ViewRequestBus::EventResult(viewableArea, m_viewId, &GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

        return m_sceneMemberId.IsValid() && m_graphId.IsValid() && !viewableArea.contains(m_scenePoint);
    }

    EditorAutomationAction* SelectSceneElementAction::GenerateMissingPreconditionAction()
    {
        CompoundAction* compoundAction = aznew CompoundAction();

        QPoint startPoint(m_scenePoint.x() - 5, m_scenePoint.y() - 5);
        QPoint endPoint(m_scenePoint.x() + 5, m_scenePoint.y() + 5);

        QRect sceneRect = QRect(startPoint, endPoint);

        compoundAction->AddAction(aznew EnsureSceneRectVisibleAction(m_graphId, sceneRect));
        compoundAction->AddAction(aznew ProcessUserEventsAction());

        return compoundAction;
    }

    void SelectSceneElementAction::SetupAction()
    {
        ClearActionQueue();

        AZ::Vector2 screenPoint = AZ::Vector2::CreateZero();
        GraphCanvas::ViewRequestBus::EventResult(screenPoint, m_viewId, &GraphCanvas::ViewRequests::MapToGlobal, GraphCanvas::ConversionUtils::QPointToVector(m_scenePoint));

        AddAction(aznew MouseClickAction(Qt::MouseButton::LeftButton, GraphCanvas::ConversionUtils::AZToQPoint(screenPoint).toPoint()));
        AddAction(aznew ProcessUserEventsAction());

        CompoundAction::SetupAction();
    }

    ///////////////////////////////
    // AltClickSceneElementAction
    ///////////////////////////////

    AltClickSceneElementAction::AltClickSceneElementAction(AZ::EntityId sceneMemberId)
        : m_sceneMemberId(sceneMemberId)
    {
        GraphCanvas::SceneMemberRequestBus::EventResult(m_graphId, m_sceneMemberId, &GraphCanvas::SceneMemberRequests::GetScene);

        GraphCanvas::SceneRequestBus::EventResult(m_viewId, m_graphId, &GraphCanvas::SceneRequests::GetViewId);

        if (!GraphCanvas::GraphUtils::IsConnection(sceneMemberId))
        {
            QRectF boundingRect;

            QGraphicsItem* graphicsItem = nullptr;
            GraphCanvas::VisualRequestBus::EventResult(graphicsItem, sceneMemberId, &GraphCanvas::VisualRequests::AsGraphicsItem);

            if (graphicsItem)
            {
                boundingRect = graphicsItem->sceneBoundingRect();
            }

            m_scenePoint = boundingRect.topLeft();
            m_scenePoint.setX(boundingRect.center().x());
            m_scenePoint.setY(m_scenePoint.y() + 5);
        }
        else
        {
            QPainterPath outlinePath;
            GraphCanvas::SceneMemberUIRequestBus::EventResult(outlinePath, sceneMemberId, &GraphCanvas::SceneMemberUIRequests::GetOutline);

            m_scenePoint = outlinePath.pointAtPercent(0.5);
        }
    }

    bool AltClickSceneElementAction::IsMissingPrecondition()
    {
        QRectF viewableArea;
        GraphCanvas::ViewRequestBus::EventResult(viewableArea, m_viewId, &GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

        return !viewableArea.contains(m_scenePoint);
    }

    EditorAutomationAction* AltClickSceneElementAction::GenerateMissingPreconditionAction()
    {
        CompoundAction* compoundAction = aznew CompoundAction();

        QPoint startPoint(m_scenePoint.x() - 5, m_scenePoint.y() - 5);
        QPoint endPoint(m_scenePoint.x() + 5, m_scenePoint.y() + 5);

        QRect sceneRect = QRect(startPoint, endPoint);

        compoundAction->AddAction(aznew EnsureSceneRectVisibleAction(m_graphId, sceneRect));
        compoundAction->AddAction(aznew ProcessUserEventsAction());

        return compoundAction;
    }

    void AltClickSceneElementAction::SetupAction()
    {
        ClearActionQueue();
        m_sceneMemberRemoved = false;
        GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphId);
#if defined(AZ_COMPILER_MSVC)
        AddAction(aznew KeyPressAction(VK_LMENU));

        AZ::Vector2 screenPoint = AZ::Vector2::CreateZero();
        GraphCanvas::ViewRequestBus::EventResult(screenPoint, m_viewId, &GraphCanvas::ViewRequests::MapToGlobal, GraphCanvas::ConversionUtils::QPointToVector(m_scenePoint));

        AddAction(aznew MouseClickAction(Qt::MouseButton::LeftButton, GraphCanvas::ConversionUtils::AZToQPoint(screenPoint).toPoint()));
        AddAction(aznew KeyReleaseAction(VK_LMENU));
        AddAction(aznew ProcessUserEventsAction(AZStd::chrono::milliseconds(750)));
#endif
        CompoundAction::SetupAction();
    }

    ActionReport AltClickSceneElementAction::GenerateReport() const
    {
        if (!m_sceneMemberRemoved)
        {
            return AZ::Failure<AZStd::string>("Failed to delete target scene element with Alt+Click");
        }

        return CompoundAction::GenerateReport();
    }

    void AltClickSceneElementAction::OnNodeRemoved(const AZ::EntityId& nodeId)
    {
        if (m_sceneMemberId == nodeId)
        {
            m_sceneMemberRemoved = true;
        }
    }

    void AltClickSceneElementAction::OnConnectionRemoved(const AZ::EntityId& connectionId)
    {
        if (m_sceneMemberId == connectionId)
        {
            m_sceneMemberRemoved = true;
        }
    }

    void AltClickSceneElementAction::OnActionsComplete()
    {
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////
    // MouseToNodePropertyEditorAction
    ////////////////////////////////////

    MouseToNodePropertyEditorAction::MouseToNodePropertyEditorAction(GraphCanvas::SlotId slotId)
        : m_slotId(slotId)
    {
    }

    void MouseToNodePropertyEditorAction::SetupAction()
    {
        ClearActionQueue();

        GraphCanvas::NodeId nodeId;
        GraphCanvas::SlotRequestBus::EventResult(nodeId, m_slotId, &GraphCanvas::SlotRequests::GetNode);

        GraphCanvas::GraphId graphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphId, nodeId, &GraphCanvas::SceneMemberRequests::GetScene);

        QRectF sceneBoundingRect;
        GraphCanvas::SlotRequestBus::EventResult(nodeId, m_slotId, &GraphCanvas::SlotRequests::GetNode);
        GraphCanvas::DataSlotLayoutRequestBus::EventResult(sceneBoundingRect, m_slotId, &GraphCanvas::DataSlotLayoutRequests::GetWidgetSceneBoundingRect);

        AddAction(aznew SceneMouseMoveAction(graphId, sceneBoundingRect.center()));

        CompoundAction::SetupAction();
    }
}
