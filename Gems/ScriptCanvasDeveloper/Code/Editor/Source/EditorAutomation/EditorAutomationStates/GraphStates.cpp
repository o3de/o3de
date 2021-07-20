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

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/GraphStates.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>

namespace ScriptCanvasDeveloper
{
    ////////////////////////////
    // CreateRuntimeGraphState
    ////////////////////////////

    CreateRuntimeGraphState::CreateRuntimeGraphState()
    {

    }

    void CreateRuntimeGraphState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        actionRunner.AddAction(&m_createNewGraphAction);
    }

    void CreateRuntimeGraphState::OnStateActionsComplete()
    {
        GraphCanvas::GraphId graphId = m_createNewGraphAction.GetGraphId();
        SetModelData(StateModelIds::GraphCanvasId, graphId);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(scriptCanvasId, &ScriptCanvasEditor::GeneralRequests::GetScriptCanvasId, graphId);
        SetModelData(StateModelIds::ScriptCanvasId, scriptCanvasId);

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphId, &GraphCanvas::SceneRequests::GetViewId);
        SetModelData(StateModelIds::ViewId, viewId);

        AZ::Vector2 minorStep = GraphCanvas::GraphUtils::FindMinorStep(graphId);
        SetModelData(StateModelIds::MinorStep, minorStep);
    } 

    /////////////////////////////
    // CreateFunctionGraphState
    /////////////////////////////

    CreateFunctionGraphState::CreateFunctionGraphState()
    {

    }

    void CreateFunctionGraphState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        actionRunner.AddAction(&m_createNewFunctionAction);
    }

    void CreateFunctionGraphState::OnStateActionsComplete()
    {
        GraphCanvas::GraphId graphId = m_createNewFunctionAction.GetGraphId();
        SetModelData(StateModelIds::GraphCanvasId, graphId);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(scriptCanvasId, &ScriptCanvasEditor::GeneralRequests::GetScriptCanvasId, graphId);
        SetModelData(StateModelIds::ScriptCanvasId, scriptCanvasId);

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphId, &GraphCanvas::SceneRequests::GetViewId);
        SetModelData(StateModelIds::ViewId, viewId);

        AZ::Vector2 minorStep = GraphCanvas::GraphUtils::FindMinorStep(graphId);
        SetModelData(StateModelIds::MinorStep, minorStep);
    }

    ///////////////////////////////
    // ForceCloseActiveGraphState
    ///////////////////////////////

    ForceCloseActiveGraphState::ForceCloseActiveGraphState()
    {

    }

    void ForceCloseActiveGraphState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        actionRunner.AddAction(&m_forceCloseActiveGraph);
    }
}
