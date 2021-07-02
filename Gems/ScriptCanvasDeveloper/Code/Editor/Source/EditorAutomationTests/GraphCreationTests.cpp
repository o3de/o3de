/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <qpushbutton.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>

#include <Editor/Source/EditorAutomationTests/GraphCreationTests.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/GraphStates.h>

namespace ScriptCanvasDeveloper
{
    ///////////////////////////
    // CreateGraphHotKeyState
    ///////////////////////////
#if !defined(AZ_COMPILER_MSVC)
#ifndef VK_CONTROL
#define VK_CONTROL 0x11
#endif 
#endif

    CreateGraphTest::CreateGraphHotKeyState::CreateGraphHotKeyState()
        : m_pressControl(VK_CONTROL)
        , m_releaseControl(VK_CONTROL)
        , m_typeN(QChar('n'))
        , m_longProcessEvents(AZStd::chrono::milliseconds(1000))
    {
    }

    void CreateGraphTest::CreateGraphHotKeyState::OnActiveGraphChanged(const AZ::EntityId& graphCanvasId)
    {
        m_hotKeyGraphId = graphCanvasId;
    }

    void CreateGraphTest::CreateGraphHotKeyState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);

        actionRunner.AddAction(&m_pressControl);
        actionRunner.AddAction(&m_shortProcessEvents);
        actionRunner.AddAction(&m_typeN);
        actionRunner.AddAction(&m_longProcessEvents);
        actionRunner.AddAction(&m_releaseControl);
        actionRunner.AddAction(&m_shortProcessEvents);

        m_hotKeyGraphId.SetInvalid();
    }

    void CreateGraphTest::CreateGraphHotKeyState::OnStateActionsComplete()
    {
        if (!m_hotKeyGraphId.IsValid())
        {
            ReportError("Failed to create graph using hot key");
        }
        else
        {
            GraphCanvas::GraphId activeGraphCanvasId;
            ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(activeGraphCanvasId, &ScriptCanvasEditor::GeneralRequests::GetActiveGraphCanvasGraphId);

            if (activeGraphCanvasId != m_hotKeyGraphId)
            {
                ReportError("Active graph is not the newly created graph using hotkey.");
            }
            else
            {
                ScriptCanvas::ScriptCanvasId scriptCanvasId;
                ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(scriptCanvasId, &ScriptCanvasEditor::GeneralRequests::GetScriptCanvasId, activeGraphCanvasId);

                bool isRuntimeGraph = false;
                ScriptCanvasEditor::EditorGraphRequestBus::EventResult(isRuntimeGraph, scriptCanvasId, &ScriptCanvasEditor::EditorGraphRequests::IsRuntimeGraph);

                if (!isRuntimeGraph)
                {
                    ReportError("Failed to create Runtime graph using button");
                }
            }
        }

        GraphCanvas::AssetEditorNotificationBus::Handler::BusDisconnect();
    }

    ////////////////////
    // CreateGraphTest
    ////////////////////

    CreateGraphTest::CreateGraphTest()
        : EditorAutomationTest("Create Graph Test")
    {
        SetHasCustomTransitions(true);

        AddState(new CreateRuntimeGraphState());
        AddState(new CreateGraphHotKeyState());
        AddState(new ForceCloseActiveGraphState());

        SetInitialStateId<CreateRuntimeGraphStateId>();
    }

    void CreateGraphTest::OnTestStarting()
    {
        m_creationState = CreateRuntimeGraphStateId::StateID();
    }

    int CreateGraphTest::EvaluateTransition(int stateId)
    {
        if (stateId == CreateRuntimeGraphState::StateID()
            || stateId == CreateGraphHotKeyState::StateID())
        {
            return ForceCloseActiveGraphState::StateID();
        }
        else if (stateId == ForceCloseActiveGraphState::StateID())
        {
            if (m_creationState == CreateRuntimeGraphStateId::StateID())
            {
                m_creationState = CreateGraphHotKeyState::StateID();
                return m_creationState;
            }
        }

        return EditorAutomationState::EXIT_STATE_ID;
    }

    ///////////////////////
    // CreateFunctionTest
    ///////////////////////

    CreateFunctionTest::CreateFunctionTest()
        : EditorAutomationTest("Create Function Test")
    {
        AddState(new CreateFunctionGraphState());
        AddState(new ForceCloseActiveGraphState());
    }
}
