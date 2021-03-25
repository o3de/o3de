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
#include "precompiled.h"

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>

#include <GraphCanvas/Utils/ConversionUtils.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>

namespace ScriptCanvasDeveloper
{
    /////////////////////////////
    // CenterOnScenePointAction
    /////////////////////////////

    CenterOnScenePointAction::CenterOnScenePointAction(GraphCanvas::GraphId graphId, QPointF scenePoint)
        : m_graphId(graphId)
        , m_scenePoint(scenePoint)
    {

    }

    bool CenterOnScenePointAction::Tick()
    {
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_graphId, &GraphCanvas::SceneRequests::GetViewId);

        if (viewId.IsValid())
        {
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOn, m_scenePoint);
        }

        return true;
    }

    /////////////////////////////////
    // EnsureSceneRectVisibleAction
    /////////////////////////////////

    EnsureSceneRectVisibleAction::EnsureSceneRectVisibleAction(GraphCanvas::GraphId graphId, QRectF sceneRect)
        : DelayAction(AZStd::chrono::milliseconds(250))
        , m_graphId(graphId)
        , m_sceneRect(sceneRect)
    {
    }

    void EnsureSceneRectVisibleAction::SetupAction()
    {
        DelayAction::SetupAction();

        m_firstTick = true;
    }

    bool EnsureSceneRectVisibleAction::Tick()
    {
        if (m_firstTick)
        {
            m_firstTick = false;
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_graphId, &GraphCanvas::SceneRequests::GetViewId);

            if (viewId.IsValid())
            {
                GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnArea, m_sceneRect);
            }

            return false;
        }
        else
        {
            return DelayAction::Tick();
        }
    }
    
    /////////////////////////
    // SceneMouseMoveAction
    /////////////////////////

    SceneMouseMoveAction::SceneMouseMoveAction(GraphCanvas::GraphId graphId, QPointF scenePoint)
        : m_graphId(graphId)
        , m_scenePoint(scenePoint)
    {
        GraphCanvas::SceneRequestBus::EventResult(m_viewId, m_graphId, &GraphCanvas::SceneRequests::GetViewId);
    }

    bool SceneMouseMoveAction::IsMissingPrecondition()
    {
        QRectF viewableBounds = QRectF(m_scenePoint, m_scenePoint);
        viewableBounds.adjust(-10, -10, 10, 10);

        QRectF viewableArea;
        GraphCanvas::ViewRequestBus::EventResult(viewableArea, m_viewId, &GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

        return !viewableArea.isEmpty() && !viewableArea.contains(viewableBounds);
    }

    EditorAutomationAction* SceneMouseMoveAction::GenerateMissingPreconditionAction()
    {
        QRectF viewableBounds = QRectF(m_scenePoint, m_scenePoint);
        viewableBounds.adjust(-10, -10, 10, 10);

        return aznew EnsureSceneRectVisibleAction(m_graphId, viewableBounds);
    }

    void SceneMouseMoveAction::SetupAction()
    {
        ClearActionQueue();

        AZ::Vector2 tempVector = AZ::Vector2::CreateZero();

        QPoint screenPoint;
        GraphCanvas::ViewRequestBus::EventResult(tempVector, m_viewId, &GraphCanvas::ViewRequests::MapToGlobal, GraphCanvas::ConversionUtils::QPointToVector(m_scenePoint));

        screenPoint = GraphCanvas::ConversionUtils::AZToQPoint(tempVector).toPoint();

        AddAction(aznew MouseMoveAction(screenPoint));

        CompoundAction::SetupAction();
    }

    /////////////////////////
    // SceneMouseDragAction
    /////////////////////////

    SceneMouseDragAction::SceneMouseDragAction(GraphCanvas::GraphId graphId, QPointF sceneStart, QPointF sceneEnd, Qt::MouseButton mouseButton)
        : m_graphId(graphId)
        , m_sceneStart(sceneStart)
        , m_sceneEnd(sceneEnd)
        , m_mouseButton(mouseButton)
    {
        GraphCanvas::SceneRequestBus::EventResult(m_viewId, m_graphId, &GraphCanvas::SceneRequests::GetViewId);
    }

    bool SceneMouseDragAction::IsMissingPrecondition()
    {
        QRectF viewableBounds = QRectF(m_sceneStart, m_sceneEnd);
        viewableBounds.adjust(-10, -10, 10, 10);

        QRectF viewableArea;
        GraphCanvas::ViewRequestBus::EventResult(viewableArea, m_viewId, &GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

        return !viewableArea.contains(viewableBounds);
    }

    EditorAutomationAction* SceneMouseDragAction::GenerateMissingPreconditionAction()
    {
        QRectF viewableBounds = QRectF(m_sceneStart, m_sceneEnd);
        viewableBounds.adjust(-10, -10, 10, 10);

        return aznew EnsureSceneRectVisibleAction(m_graphId, viewableBounds);
    }

    void SceneMouseDragAction::SetupAction()
    {
        AZ::Vector2 tempVector = AZ::Vector2::CreateZero();

        QPoint screenStart;
        GraphCanvas::ViewRequestBus::EventResult(tempVector, m_viewId, &GraphCanvas::ViewRequests::MapToGlobal, GraphCanvas::ConversionUtils::QPointToVector(m_sceneStart));

        screenStart = GraphCanvas::ConversionUtils::AZToQPoint(tempVector).toPoint();

        tempVector = AZ::Vector2::CreateZero();

        QPoint screenEnd;
        GraphCanvas::ViewRequestBus::EventResult(tempVector, m_viewId, &GraphCanvas::ViewRequests::MapToGlobal, GraphCanvas::ConversionUtils::QPointToVector(m_sceneEnd));

        screenEnd = GraphCanvas::ConversionUtils::AZToQPoint(tempVector).toPoint();

        AddAction(aznew MouseDragAction(screenStart, screenEnd, m_mouseButton));

        CompoundAction::SetupAction();
    }
}
