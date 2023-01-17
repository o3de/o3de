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
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/CreateElementsStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/EditorViewStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/ElementInteractionStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/VariableStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/UtilityStates.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorAutmationTest that will create a variable of the specified type using the specified creation type, and optionally give it a name
    */
    class ManuallyCreateVariableTest
        : public EditorAutomationTest
    {
    public:
        ManuallyCreateVariableTest(ScriptCanvas::Data::Type dataType, CreateVariableAction::CreationType creationType, AZStd::string variableName = AZStd::string());
        ~ManuallyCreateVariableTest() override = default;
    };

    /**
        EditorautomationTest that will provide a nicer name for the test.
    */
    class CreateNamedVariableTest
        : public ManuallyCreateVariableTest
    {
    public:
        CreateNamedVariableTest(ScriptCanvas::Data::Type dataType, AZStd::string name, CreateVariableAction::CreationType creationType = CreateVariableAction::CreationType::AutoComplete);
        ~CreateNamedVariableTest() override = default;
    };

    /**
        EditorautomationTest that will create a two variables with a duplicated name
    */
    class DuplicateVariableNameTest
        : public EditorAutomationTest
    {
        class CheckVariableForNameMismatchState
            : public CustomActionState
        {
        public:
            AZ_CLASS_ALLOCATOR(CheckVariableForNameMismatchState, AZ::SystemAllocator)
            CheckVariableForNameMismatchState(AutomationStateModelId nameId, AutomationStateModelId variableId);
            ~CheckVariableForNameMismatchState() override = default;

            void OnCustomAction() override;

        private:

            AutomationStateModelId m_nameId;
            AutomationStateModelId m_variableId;
        };

    public:
        DuplicateVariableNameTest(ScriptCanvas::Data::Type firstType, ScriptCanvas::Data::Type secondType, AZStd::string variableName);
        ~DuplicateVariableNameTest() override = default;
    };

    // General Helper States for the tests in this file.
    namespace EditStringLike
    {
        class VariableInPaletteState
            : public NamedAutomationState
        {
        public:
            AZ_CLASS_ALLOCATOR(VariableInPaletteState, AZ::SystemAllocator)
            typedef std::function< AZ::Outcome<void, AZStd::string>(const ScriptCanvas::GraphVariable*)> VariablePaletteValidator;

            VariableInPaletteState(AZStd::string value, AutomationStateModelId variableId, VariablePaletteValidator validator);
            ~VariableInPaletteState() override = default;

            void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
            void OnStateActionsComplete() override;

        private:            

            AZStd::string          m_stringValue;
            AutomationStateModelId m_variableId;

            VariablePaletteValidator m_validator;

            MouseMoveAction*        m_moveToTableRow = nullptr;
            MouseClickAction        m_clickAction;

            ProcessUserEventsAction       m_processEvents;

            TypeStringAction    m_typeStringAction;
            TypeCharAction      m_typeReturnAction;
        };

        class ValueInNodeState
            : public NamedAutomationState
        {
        public:
            AZ_CLASS_ALLOCATOR(ValueInNodeState, AZ::SystemAllocator)
            typedef std::function< AZ::Outcome<void, AZStd::string>(const ScriptCanvas::Datum*)> DatumValidator;

            ValueInNodeState(AZStd::string value, AutomationStateModelId endpointId, DatumValidator datumValidator);
            ~ValueInNodeState() override = default;

            void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
            void OnStateActionsComplete() override;

        private:

            AZStd::string m_stringValue;
            AutomationStateModelId m_endpointId;

            DatumValidator         m_datumValidator;

            MouseClickAction            m_clickAction;

            ProcessUserEventsAction     m_processEvents;

            TypeStringAction            m_typeStringAction;
            TypeCharAction              m_typeReturnAction;

            MouseToNodePropertyEditorAction* m_moveToPropertyAction = nullptr;
        };
    }

    /**
        EditorautomationTest that will test out a couple of ways of editing a number[GraphPalette modification, and on-node editing]
    */
    class ModifyNumericInputTest
        : public EditorAutomationTest
    {
    public:
        ModifyNumericInputTest(double value);
        ~ModifyNumericInputTest() override = default;
    };

    /**
        EditorautomationTest that will test out a couple of ways of editing a string[GraphPalette modification, an on-node editing]
    */
    class ModifyStringInputTest
        : public EditorAutomationTest
    {
    public:
        ModifyStringInputTest(AZStd::string value);
        ~ModifyStringInputTest() override = default;
    };

    /**
        EditorautomationTest that will test out a couple of ways of editing a bool[GraphPalette modification, an on-node editing]
    */
    class ToggleBoolInputTest
        : public EditorAutomationTest
    {
        enum State
        {
            CreateGraph,
            CreateVariable,
            ToggleBoolInPalette,
            CreateSetNode,
            ToggleBoolInNode,

            CloseGraph
        };

        class ToggleBoolInPaletteState
            : public NamedAutomationState
        {
        public:
            AZ_CLASS_ALLOCATOR(ToggleBoolInPaletteState, AZ::SystemAllocator)
            ToggleBoolInPaletteState(AutomationStateModelId variableId);
            ~ToggleBoolInPaletteState() override = default;

            void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
            void OnStateActionsComplete() override;

        private:

            bool m_originalValue = false;

            AutomationStateModelId  m_variableId;

            MouseClickAction*       m_interactWithTableAction = nullptr;
            ProcessUserEventsAction m_processEvents;
        };

        class ToggleBoolInNodeState
            : public NamedAutomationState
        {
        public:
            AZ_CLASS_ALLOCATOR(ToggleBoolInNodeState, AZ::SystemAllocator)
            ToggleBoolInNodeState(AutomationStateModelId endpointId);
            ~ToggleBoolInNodeState() override = default;

            void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
            void OnStateActionsComplete() override;

        private:

            ScriptCanvas::Endpoint m_scriptCanvasEndpoint;
            bool m_originalValue = false;

            AutomationStateModelId m_endpointId;

            MouseToNodePropertyEditorAction* m_mouseToNodePropertyEditorAction = nullptr;

            ProcessUserEventsAction     m_processEvents;
            MouseClickAction            m_clickAction;
        };

    public:
        ToggleBoolInputTest();
        ~ToggleBoolInputTest() override = default;
    };

    /**
        EditorautomationTest that takes in a set of variable types to create. For each type it will then create the variable, create a get and set node for that variable. Then clean-up the nodes and delete the variable.
    */
    class VariableLifeCycleTest
        : public EditorAutomationTest
    {
    public:
        AZ_CLASS_ALLOCATOR(VariableLifeCycleTest, AZ::SystemAllocator)
        VariableLifeCycleTest(AZStd::string name, AZStd::vector<ScriptCanvas::Data::Type> dataTypes, CreateVariableAction::CreationType creationType = CreateVariableAction::CreationType::AutoComplete);
        ~VariableLifeCycleTest() override = default;

        void OnTestStarting() override;

    protected:

        // Custom Transition
        int EvaluateTransition(int stateId) override;
        ////

    private:

        int SetupNextVariable();

        ScriptCanvas::VariableId             m_activeVariableId;
        AZStd::vector<CreateVariableAction*> m_createVariables;
        AZStd::vector<ScriptCanvas::Data::Type> m_typesToMake;
        
        bool m_createVariablesNodesViaContextMenu = true;
        int m_activeIndex = 0;

        GraphCanvas::ViewId             m_viewId;
        GraphCanvas::GraphId            m_graphId;
        ScriptCanvas::ScriptCanvasId    m_scriptCanvasId;

        AutomationStateModelId m_variableTypeId;
        AutomationStateModelId m_variableId;
        AutomationStateModelId m_viewCenter;
        AutomationStateModelId m_offsetCenter;

        FindViewCenterState* m_findViewCenterState;

        CreateVariableState* m_createVariableState;

        CreateVariableNodeFromGraphPaletteState* m_dragCreateGetNode = nullptr;
        CreateVariableNodeFromGraphPaletteState* m_dragCreateSetNode = nullptr;

        CreateNodeFromContextMenuState* m_createGetNode = nullptr;
        CreateNodeFromContextMenuState* m_createSetNode = nullptr;

        AltClickSceneElementState*      m_destroyGetNode = nullptr;
        AltClickSceneElementState*      m_destroySetNode = nullptr;

        DeleteVariableRowFromPaletteState* m_deleteVariableRowState = nullptr;
    };

    /**
        EditorautomationTest that will create every variable type(including all variations of containers) and then delete them all.
    */
    class RapidVariableCreationDeletionTest
        : public EditorAutomationTest
    {
    public:
        AZ_CLASS_ALLOCATOR(RapidVariableCreationDeletionTest, AZ::SystemAllocator)

        RapidVariableCreationDeletionTest();
        ~RapidVariableCreationDeletionTest() override = default;

        void OnTestStarting() override;

        // Custom Transition
        int EvaluateTransition(int stateId) override;
        ////

    private:

        int SetupNextVariable();

    private:

        QTableView*         m_graphPalette = nullptr;

        int m_activeIndex = 0;
        AZStd::vector< ScriptCanvas::Data::Type > m_variableTypes;

        AutomationStateModelId m_variableType;

        CreateVariableState*                m_createVariableState = nullptr;
        DeleteVariableRowFromPaletteState*  m_deleteVariableRowState = nullptr;

        ShowGraphVariablesAction m_showGraphVariableAction;
    };
}
