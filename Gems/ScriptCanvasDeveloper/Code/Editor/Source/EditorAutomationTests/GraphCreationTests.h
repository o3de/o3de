/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

namespace ScriptCanvas::Developer
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
            AZ_CLASS_ALLOCATOR(CreateGraphHotKeyState, AZ::SystemAllocator)
            CreateGraphHotKeyState();
            ~CreateGraphHotKeyState() override = default;

            // AssetEditorNotificationBus
            void OnActiveGraphChanged(const AZ::EntityId& graphCanvasId) override;
            ////

        protected:

            void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
            void OnStateActionsComplete() override;

        private:

            GraphCanvas::GraphId m_hotKeyGraphId;

            KeyPressAction      m_pressControl;
            KeyReleaseAction    m_releaseControl;
            TypeCharAction    m_typeN;

            ProcessUserEventsAction     m_shortProcessEvents;
            ProcessUserEventsAction     m_longProcessEvents;
        };

    public:
        AZ_CLASS_ALLOCATOR(CreateGraphTest, AZ::SystemAllocator)
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
