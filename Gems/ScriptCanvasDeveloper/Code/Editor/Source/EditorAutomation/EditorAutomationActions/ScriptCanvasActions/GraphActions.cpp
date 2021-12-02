/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QToolButton>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/Automation/AutomationUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <Editor/GraphCanvas/AutomationIds.h>
#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/GraphActions.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>

namespace ScriptCanvasDeveloper
{
    /////////////////////////
    // CreateNewGraphAction
    /////////////////////////

    CreateNewGraphAction::CreateNewGraphAction()
    {
    }

    void CreateNewGraphAction::SetupAction()
    {
        m_graphId.SetInvalid();

        ClearActionQueue();

        QToolButton* createNewGraphButton = GraphCanvas::AutomationUtils::FindObjectById<QToolButton>(ScriptCanvasEditor::AssetEditorId, ScriptCanvasEditor::AutomationIds::CreateScriptCanvasButton);

        if (createNewGraphButton)
        {
            QPointF point = createNewGraphButton->mapToGlobal(createNewGraphButton->rect().center());

            m_newGraphAction = aznew WaitForNewGraphAction();
            AddAction(aznew MouseClickAction(Qt::MouseButton::LeftButton, point.toPoint()));
            AddAction(m_newGraphAction);
        }

        CompoundAction::SetupAction();
    }

    GraphCanvas::GraphId CreateNewGraphAction::GetGraphId() const
    {
        return m_graphId;
    }

    ActionReport CreateNewGraphAction::GenerateReport() const
    {
        if (!m_graphId.IsValid())
        {
            return AZ::Failure<AZStd::string>("Failed to create New Runtime Graph");
        }
        else
        {
            GraphCanvas::GraphId activeGraphCanvasId;
            ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(activeGraphCanvasId, &ScriptCanvasEditor::GeneralRequests::GetActiveGraphCanvasGraphId);

            if (activeGraphCanvasId != m_graphId)
            {
                return AZ::Failure<AZStd::string>("Active graph is not the newly created graph.");
            }
        }

        return CompoundAction::GenerateReport();
    }

    void CreateNewGraphAction::OnActionsComplete()
    {
        m_graphId = m_newGraphAction->GetGraphId();
    }

    ////////////////////////////
    // CreateNewFunctionAction
    ////////////////////////////

    CreateNewFunctionAction::CreateNewFunctionAction()
        : CreateNewGraphAction()
    {
    }

    void CreateNewFunctionAction::SetupAction()
    {
        m_graphId.SetInvalid();

        ClearActionQueue();

        QToolButton* createNewFunctionButton = GraphCanvas::AutomationUtils::FindObjectById<QToolButton>(ScriptCanvasEditor::AssetEditorId, ScriptCanvasEditor::AutomationIds::CreateScriptCanvasFunctionButton);

        if (createNewFunctionButton)
        {
            QPointF point = createNewFunctionButton->mapToGlobal(createNewFunctionButton->rect().center());

            m_newGraphAction = aznew WaitForNewGraphAction();
            AddAction(aznew MouseClickAction(Qt::MouseButton::LeftButton, point.toPoint()));
            AddAction(m_newGraphAction);
        }

        CompoundAction::SetupAction();
    }

    ActionReport CreateNewFunctionAction::GenerateReport() const
    {
        if (!m_graphId.IsValid())
        {
            return AZ::Failure<AZStd::string>("Failed to create New Function");
        }
        else
        {
            GraphCanvas::GraphId activeGraphCanvasId;
            ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(activeGraphCanvasId, &ScriptCanvasEditor::GeneralRequests::GetActiveGraphCanvasGraphId);

            if (activeGraphCanvasId != m_graphId)
            {
                return AZ::Failure<AZStd::string>("Active graph is not the newly created function.");
            }
        }

        return CompoundAction::GenerateReport();
    }

    void CreateNewFunctionAction::OnActionsComplete()
    {
        m_graphId = m_newGraphAction->GetGraphId();
    }

    ////////////////////////////////
    // ForceCloseActiveGraphAction
    ////////////////////////////////

    ForceCloseActiveGraphAction::ForceCloseActiveGraphAction()
        : ProcessUserEventsAction(AZStd::chrono::milliseconds(500))
    {
    }

    void ForceCloseActiveGraphAction::SetupAction()
    {
        ProcessUserEventsAction::SetupAction();

        m_firstTick = true;
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(m_activeGraphId, &ScriptCanvasEditor::GeneralRequests::GetActiveGraphCanvasGraphId);
    }

    bool ForceCloseActiveGraphAction::Tick()
    {
        if (m_firstTick)
        {
            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::ForceCloseActiveAsset);

            return true;
        }
        else
        {
            return ProcessUserEventsAction::Tick();
        }
    }

    ActionReport ForceCloseActiveGraphAction::GenerateReport() const
    {
        GraphCanvas::GraphId activeGraphCanvasId;
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(activeGraphCanvasId, &ScriptCanvasEditor::GeneralRequests::GetActiveGraphCanvasGraphId);

        if (activeGraphCanvasId == m_activeGraphId
            && m_activeGraphId.IsValid())
        {
            return AZ::Failure<AZStd::string>("Failed to close down currently active graph");
        }

        return ProcessUserEventsAction::GenerateReport();
    }

    ////////////////////////////////
    // WaitForNewGraphAction
    ////////////////////////////////

    WaitForNewGraphAction::WaitForNewGraphAction()
    {
        m_newGraphCreated = false;
        GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);
    }

    WaitForNewGraphAction::~WaitForNewGraphAction()
    {
        GraphCanvas::AssetEditorNotificationBus::Handler::BusDisconnect();
    }

    bool WaitForNewGraphAction::Tick()
    {
        return m_newGraphCreated;
    }

    void WaitForNewGraphAction::OnActiveGraphChanged(const AZ::EntityId& graphId)
    {
        m_graphId = graphId;
        m_newGraphCreated = true;

        GraphCanvas::AssetEditorNotificationBus::Handler::BusDisconnect();
    }

}
