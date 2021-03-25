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
#pragma once

#include <QLineEdit>
#include <QMenu>
#include <QString>
#include <QTableView>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

#include <ScriptCanvas/Bus/RequestBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationTest.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/WidgetActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ConnectionActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/CreateElementsActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ElementInteractions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/GraphActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/VariableActions.h>

namespace ScriptCanvasDeveloper
{
    DefineStateId(CreateGraphTest_CreateGraphHotKeyState);

    /**
        EditorautomationTest that will test out the ways of creating a runtime graph
    */
    class CreateGraphTest
        : public EditorAutomationTest
    {
        class CreateGraphHotKeyState
            : public StaticIdAutomationState<CreateGraphTest_CreateGraphHotKeyStateId>
            , public GraphCanvas::AssetEditorNotificationBus::Handler
        {
        public:
            CreateGraphHotKeyState();
            ~CreateGraphHotKeyState() override = default;

            // AssetEditorNotificationBus
            void OnActiveGraphChanged(const AZ::EntityId& graphCanvasId) override;
            ////

        protected:

            void OnSetupStateActions(EditorAutomationActionRunner& actionRunner);
            void OnStateActionsComplete();

        private:

            GraphCanvas::GraphId m_hotKeyGraphId;

            KeyPressAction      m_pressControl;
            KeyReleaseAction    m_releaseControl;
            TypeCharAction    m_typeN;

            ProcessUserEventsAction     m_shortProcessEvents;
            ProcessUserEventsAction     m_longProcessEvents;
        };

    public:
        CreateGraphTest();
        ~CreateGraphTest() override = default;

        void OnTestStarting() override;

    protected:

        int EvaluateTransition(int stateId) override;

    private:

        int m_creationState = EditorAutomationState::EXIT_STATE_ID;
    };

    /**
        EditorautomationTest that will test out the ways of creating a function graph.
    */
    class CreateFunctionTest
        : public EditorAutomationTest
    {
    public:
        CreateFunctionTest();
        ~CreateFunctionTest() override = default;
    };
}
