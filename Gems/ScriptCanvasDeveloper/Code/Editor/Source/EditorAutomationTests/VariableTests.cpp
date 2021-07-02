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

#include <EditorAutomationTests/VariableTests.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ElementInteractions.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/EditorViewStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/ElementInteractionStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/GraphStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/UtilityStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/VariableStates.h>

#if !defined(AZ_COMPILER_MSVC)
#ifndef VK_RETURN
#define VK_RETURN 0x0D
#endif
#endif

namespace ScriptCanvasDeveloper
{
    ///////////////////////////////
    // ManuallyCreateVariableTest
    ///////////////////////////////

    AZStd::string GetModifierDescription(CreateVariableAction::CreationType creationType)
    {
        AZStd::string modifierDescription;

        switch (creationType)
        {
        case CreateVariableAction::CreationType::AutoComplete:
            break;
        case CreateVariableAction::CreationType::Palette:
            modifierDescription = "From Palette";
            break;
        case CreateVariableAction::CreationType::Programmatic:
            modifierDescription = "Programmatically";
            break;
        default:
            break;
        }

        return modifierDescription;
    }
    
    ManuallyCreateVariableTest::ManuallyCreateVariableTest(ScriptCanvas::Data::Type dataType, CreateVariableAction::CreationType creationType, AZStd::string variableName)
        : EditorAutomationTest(QString("Create %1 %2").arg(ScriptCanvas::Data::GetName(dataType).c_str()).arg(GetModifierDescription(creationType).c_str()))
    {
        AutomationStateModelId variableTypeId = "VariableDataType";
        SetStateData(variableTypeId, dataType);

        AutomationStateModelId variableNameId = "VariableName";
        SetStateData(variableNameId, variableName);

        constexpr bool errorOnNameMismatch = true;

        AddState(new CreateRuntimeGraphState());
        AddState(new CreateVariableState(variableTypeId, variableNameId, errorOnNameMismatch, creationType));
        AddState(new ForceCloseActiveGraphState());
    }

    ////////////////////////////
    // CreateNamedVariableTest
    ////////////////////////////

    CreateNamedVariableTest::CreateNamedVariableTest(ScriptCanvas::Data::Type dataType, AZStd::string name, CreateVariableAction::CreationType creationType)
        : ManuallyCreateVariableTest(dataType, creationType, name)
    {
        SetTestName(AZStd::string::format("Create %s with name %s", ScriptCanvas::Data::GetName(dataType).c_str(), name.c_str()).c_str());
    }

    //////////////////////////////
    // DuplicateVariableNameTest
    //////////////////////////////

    DuplicateVariableNameTest::CheckVariableForNameMismatchState::CheckVariableForNameMismatchState(AutomationStateModelId nameId, AutomationStateModelId variableId)
        : CustomActionState("CheckVariableForNameMismatchState")
        , m_nameId(nameId)
        , m_variableId(variableId)
    {
        SetStateName("CheckVariableForNameMismatchState");
    }

    void DuplicateVariableNameTest::CheckVariableForNameMismatchState::OnCustomAction()
    {
        const ScriptCanvas::ScriptCanvasId* scriptCanvasId = GetStateModel()->GetStateDataAs<ScriptCanvas::ScriptCanvasId>(StateModelIds::ScriptCanvasId);
        const ScriptCanvas::VariableId* variableId = GetStateModel()->GetStateDataAs<ScriptCanvas::VariableId>(m_variableId);
        const AZStd::string* variableName = GetStateModel()->GetStateDataAs<AZStd::string>(m_nameId);

        if (variableId && scriptCanvasId && variableName)
        {
            ScriptCanvas::GraphVariable* graphVariable = nullptr;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, (*scriptCanvasId), &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, (*variableId));

            QString testName = QString(variableName->c_str());

            if (testName.compare(graphVariable->GetVariableName().data(), Qt::CaseInsensitive) == 0)
            {
                ReportError(AZStd::string::format("Second Variable has duplicate name %s", testName.toUtf8().data()).c_str());
            }
        }
        else
        {
            if (variableId == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid VariableId", m_variableId.c_str()));
            }

            if (variableName == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid string", m_nameId.c_str()));
            }

            if (scriptCanvasId == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid ScriptCanvas::ScriptCanvasId", StateModelIds::ScriptCanvasId));
            }
        }
    }

    DuplicateVariableNameTest::DuplicateVariableNameTest(ScriptCanvas::Data::Type firstType, ScriptCanvas::Data::Type secondType, AZStd::string variableName)
        : EditorAutomationTest(QString("Duplicate Variable name %1 Test").arg(variableName.c_str()))
    {
        AutomationStateModelId firstVariableTypeId = "VariableDataType::1";
        SetStateData(firstVariableTypeId, firstType);

        AutomationStateModelId secondVariableTypeId = "VariableDataType::2";
        SetStateData(secondVariableTypeId, secondType);

        AutomationStateModelId variableNameId = "VariableName";
        SetStateData(variableNameId, variableName);

        AddState(new CreateRuntimeGraphState());

        constexpr bool errorOnNameMismatch = true;

        AutomationStateModelId firstVariableId = "VariableId::1";
        AddState(new CreateVariableState(firstVariableTypeId, variableNameId, errorOnNameMismatch, CreateVariableAction::CreationType::AutoComplete, firstVariableId));

        AutomationStateModelId secondVariableId = "VariableId::2";
        AddState(new CreateVariableState(secondVariableTypeId, variableNameId, !errorOnNameMismatch, CreateVariableAction::CreationType::AutoComplete, secondVariableId));

        AddState(new CheckVariableForNameMismatchState(variableNameId, secondVariableId));
        AddState(new ForceCloseActiveGraphState());
    }

    namespace EditStringLike
    {
        VariableInPaletteState::VariableInPaletteState(AZStd::string value, AutomationStateModelId variableId, VariablePaletteValidator validator)
            : NamedAutomationState("EditStringLikeVariableInPaletteState")
            , m_stringValue(value)
            , m_variableId(variableId)
            , m_validator(validator)
            , m_clickAction(Qt::MouseButton::LeftButton)
            , m_typeStringAction(value.c_str())
            , m_typeReturnAction(VK_RETURN)
        {
        }

        void VariableInPaletteState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
        {
            QTableView* graphPalette = nullptr;
            ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(graphPalette, &ScriptCanvasEditor::VariableAutomationRequests::GetGraphPaletteTableView);

            if (graphPalette)
            {
                // 0 is the row, we've only created the single variable in this test, so we can assume that.
                // 2 is the column, this is just the column we need to click on in order to enter the editing mode.
                QModelIndex tableIndex = graphPalette->model()->index(0, 2);

                QRectF visualRect = graphPalette->visualRect(tableIndex);
                QPoint targetPoint = graphPalette->mapToGlobal(visualRect.center().toPoint());

                m_moveToTableRow = aznew MouseMoveAction(targetPoint);

                actionRunner.AddAction(m_moveToTableRow);
                actionRunner.AddAction(&m_processEvents);
                actionRunner.AddAction(&m_clickAction);
                actionRunner.AddAction(&m_clickAction);
                actionRunner.AddAction(&m_processEvents);
                actionRunner.AddAction(&m_typeStringAction);
                actionRunner.AddAction(&m_typeReturnAction);
                actionRunner.AddAction(&m_processEvents);
            }
            else
            {
                ReportError("GraphPalette cannot be found in EditNumberInPaletteState");
            }
        }

        void VariableInPaletteState::OnStateActionsComplete()
        {
            const ScriptCanvas::ScriptCanvasId* scriptCanvasId = GetStateModel()->GetStateDataAs<ScriptCanvas::ScriptCanvasId>(StateModelIds::ScriptCanvasId);
            const ScriptCanvas::VariableId* variableId = GetStateModel()->GetStateDataAs<ScriptCanvas::VariableId>(m_variableId);

            if (scriptCanvasId)
            {
                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, (*scriptCanvasId), &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, (*variableId));

                if (graphVariable)
                {
                    AZ::Outcome<void, AZStd::string> validationResult = m_validator(graphVariable);

                    if (!validationResult)
                    {
                        ReportError(validationResult.GetError());
                    }
                }
                else
                {
                    ReportError("Failed to find Created Variable");
                }
            }

            if (m_moveToTableRow)
            {
                delete m_moveToTableRow;
                m_moveToTableRow = nullptr;
            }
        }

        ValueInNodeState::ValueInNodeState(AZStd::string value, AutomationStateModelId endpointId, DatumValidator datumValidator)
            : NamedAutomationState("EditNumberInNodeState")
            , m_stringValue(value)
            , m_endpointId(endpointId)
            , m_datumValidator(datumValidator)            
            , m_clickAction(Qt::MouseButton::LeftButton)
            , m_typeStringAction(value.c_str())
            , m_typeReturnAction(VK_RETURN)
        {
        }

        void ValueInNodeState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
        {
            const GraphCanvas::Endpoint* targetEndpoint = GetStateModel()->GetStateDataAs<GraphCanvas::Endpoint>(m_endpointId);

            if (targetEndpoint)
            {
                m_moveToPropertyAction = aznew MouseToNodePropertyEditorAction(targetEndpoint->GetSlotId());

                actionRunner.AddAction(m_moveToPropertyAction);
                actionRunner.AddAction(&m_processEvents);
                actionRunner.AddAction(&m_clickAction);
                actionRunner.AddAction(&m_processEvents);
                actionRunner.AddAction(&m_typeStringAction);
                actionRunner.AddAction(&m_processEvents);
                actionRunner.AddAction(&m_typeReturnAction);
                actionRunner.AddAction(&m_processEvents);
            }
            else
            {
                ReportError(AZStd::string::format("%s is not a valid GraphCanvas::Endpoint", m_endpointId.c_str()));
            }
        }

        void ValueInNodeState::OnStateActionsComplete()
        {
            const GraphCanvas::Endpoint* targetEndpoint = GetStateModel()->GetStateDataAs<GraphCanvas::Endpoint>(m_endpointId);
            const ScriptCanvas::ScriptCanvasId* scriptCanvasId = GetStateModel()->GetStateDataAs<ScriptCanvas::ScriptCanvasId>(StateModelIds::ScriptCanvasId);

            if (targetEndpoint && scriptCanvasId)
            {
                ScriptCanvas::Endpoint scriptCanvasEndpoint;
                ScriptCanvasEditor::EditorGraphRequestBus::EventResult(scriptCanvasEndpoint, (*scriptCanvasId), &ScriptCanvasEditor::EditorGraphRequests::ConvertToScriptCanvasEndpoint, (*targetEndpoint));

                if (scriptCanvasEndpoint.IsValid())
                {
                    const ScriptCanvas::Datum* datum = nullptr;
                    ScriptCanvas::NodeRequestBus::EventResult(datum, scriptCanvasEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::FindDatum, scriptCanvasEndpoint.GetSlotId());

                    AZ::Outcome<void, AZStd::string> datumValidation = m_datumValidator(datum);

                    if (!datumValidation)
                    {
                        ReportError(datumValidation.GetError());
                    }
                }
                else
                {
                    ReportError("Failed to convert Graph Canvas endpoint to Script Canvas endpoint");
                }
            }
        }
    }

    ///////////////////////////
    // ModifyNumericInputTest
    ///////////////////////////    

    ModifyNumericInputTest::ModifyNumericInputTest(double value)
        : EditorAutomationTest("Numeric Input Test")
    {
        auto paletteDataValidator = [value](const ScriptCanvas::GraphVariable* graphVariable) -> AZ::Outcome<void, AZStd::string>
        {
            const ScriptCanvas::Datum* datum = graphVariable->GetDatum();

            if (datum && datum->GetType() == ScriptCanvas::Data::Type::Number())
            {
                double currentValue = (*datum->GetAs<ScriptCanvas::Data::NumberType>());

                if (!AZ::IsClose(currentValue, value, DBL_EPSILON))
                {
                    return AZ::Failure<AZStd::string>(AZStd::string::format("Expected value %f found %f", value, currentValue));
                }
            }
            else
            {
                return AZ::Failure<AZStd::string>("Datum is missing or incorrect type.");
            }

            return AZ::Success();
        };

        auto datumValidator = [value](const ScriptCanvas::Datum* datum) -> AZ::Outcome<void, AZStd::string>
        {
            if (datum && datum->GetType() == ScriptCanvas::Data::Type::Number())
            {
                const ScriptCanvas::Data::NumberType* numberType = datum->GetAs<ScriptCanvas::Data::NumberType>();

                if (numberType)
                {
                    if (!AZ::IsClose(value, (*numberType), DBL_EPSILON))
                    {
                        return AZ::Failure<AZStd::string>(AZStd::string::format("Expected datum value to be %f, got %f", value, (*numberType)));
                    }
                }
            }
            else
            {
                return AZ::Failure<AZStd::string>("Datum is missing or incorrect type.");
            }

            return AZ::Success();
        };

        AZStd::string inputString = AZStd::string::format("%f", value);
        
        AutomationStateModelId firstVariableTypeId = "VariableDataType";
        SetStateData(firstVariableTypeId, ScriptCanvas::Data::Type::Number());

        AutomationStateModelId variableNameId = "VariableName";
        SetStateData(variableNameId, AZStd::string("Numeric"));

        AddState(new CreateRuntimeGraphState());

        AutomationStateModelId variableId = "VariableId";
        AddState(new CreateVariableState(firstVariableTypeId, variableNameId, false, CreateVariableAction::CreationType::AutoComplete, variableId));

        AddState(new EditStringLike::VariableInPaletteState(inputString, variableId, paletteDataValidator));

        AutomationStateModelId viewCenter = "ViewCenter";
        AddState(new FindViewCenterState(viewCenter));

        AutomationStateModelId variableNodeId = "VariableNodeId";
        AddState(new CreateVariableNodeFromGraphPaletteState(variableNameId, viewCenter, Qt::KeyboardModifier::AltModifier, variableNodeId));

        AutomationStateModelId dataSlotId = "DataSlotId";
        AddState(new FindEndpointOfTypeState(variableNodeId, dataSlotId, GraphCanvas::ConnectionType::CT_Input, GraphCanvas::SlotTypes::DataSlot));

        AddState(new EditStringLike::ValueInNodeState(inputString, dataSlotId, datumValidator));

        AddState(new ForceCloseActiveGraphState());
    }

    //////////////////////////
    // ModifyStringInputTest
    //////////////////////////

    ModifyStringInputTest::ModifyStringInputTest(AZStd::string value)
        : EditorAutomationTest("String Input Test")
    {
        auto paletteDataValidator = [value](const ScriptCanvas::GraphVariable* graphVariable) -> AZ::Outcome<void, AZStd::string>
        {
            const ScriptCanvas::Datum* datum = graphVariable->GetDatum();

            if (datum && datum->GetType() == ScriptCanvas::Data::Type::String())
            {
                AZStd::string currentValue = (*datum->GetAs<ScriptCanvas::Data::StringType>());

                if (currentValue.compare(value) != 0)
                {
                    return AZ::Failure<AZStd::string>(AZStd::string::format("Expected value %s found %s", value.c_str(), currentValue.c_str()));
                }
            }
            else
            {
                return AZ::Failure<AZStd::string>("Datum is missing or incorrect type.");
            }

            return AZ::Success();
        };

        auto datumValidator = [value](const ScriptCanvas::Datum* datum) -> AZ::Outcome<void, AZStd::string>
        {
            if (datum && datum->GetType() == ScriptCanvas::Data::Type::String())
            {
                const ScriptCanvas::Data::StringType* stringType = datum->GetAs<ScriptCanvas::Data::StringType>();

                if (stringType)
                {
                    if (value.compare(stringType->c_str()) != 0)
                    {
                        return AZ::Failure<AZStd::string>(AZStd::string::format("Expected datum value to be %s, got %s", value.c_str(), (*stringType).c_str()));
                    }
                }
            }
            else
            {
                return AZ::Failure<AZStd::string>("Datum is missing or incorrect type.");
            }

            return AZ::Success();
        };

        AutomationStateModelId firstVariableTypeId = "VariableDataType";
        SetStateData(firstVariableTypeId, ScriptCanvas::Data::Type::String());

        AutomationStateModelId variableNameId = "VariableName";
        SetStateData(variableNameId, AZStd::string("String"));

        AddState(new CreateRuntimeGraphState());

        AutomationStateModelId variableId = "VariableId";
        AddState(new CreateVariableState(firstVariableTypeId, variableNameId, false, CreateVariableAction::CreationType::AutoComplete, variableId));

        AddState(new EditStringLike::VariableInPaletteState(value, variableId, paletteDataValidator));

        AutomationStateModelId viewCenter = "ViewCenter";
        AddState(new FindViewCenterState(viewCenter));

        AutomationStateModelId variableNodeId = "VariableNodeId";
        AddState(new CreateVariableNodeFromGraphPaletteState(variableNameId, viewCenter, Qt::KeyboardModifier::AltModifier, variableNodeId));

        AutomationStateModelId dataSlotId = "DataSlotId";
        AddState(new FindEndpointOfTypeState(variableNodeId, dataSlotId, GraphCanvas::ConnectionType::CT_Input, GraphCanvas::SlotTypes::DataSlot));

        AddState(new EditStringLike::ValueInNodeState(value, dataSlotId, datumValidator));

        AddState(new ForceCloseActiveGraphState());
    }

    ////////////////////////
    // ToggleBoolInputTest
    ////////////////////////

    ToggleBoolInputTest::ToggleBoolInPaletteState::ToggleBoolInPaletteState(AutomationStateModelId variableId)
        : NamedAutomationState("ToggleBoolInPaletteState")
        , m_variableId(variableId)
    {
        SetStateName(AZStd::string::format("ToggleBoolInPaletteState::%s", m_variableId.c_str()));
    }

    void ToggleBoolInputTest::ToggleBoolInPaletteState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const ScriptCanvas::VariableId* variableId = GetStateModel()->GetStateDataAs<ScriptCanvas::VariableId>(m_variableId);
        const ScriptCanvas::ScriptCanvasId* scriptCanvasId = GetStateModel()->GetStateDataAs<ScriptCanvas::ScriptCanvasId>(StateModelIds::ScriptCanvasId);

        if (variableId && scriptCanvasId)
        {
            ScriptCanvas::GraphVariable* graphVariable = nullptr;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, (*scriptCanvasId), &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, (*variableId));

            if (graphVariable)
            {
                const ScriptCanvas::Datum* datum = graphVariable->GetDatum();

                if (datum && datum->GetType() == ScriptCanvas::Data::Type::Boolean())
                {
                    m_originalValue = (*datum->GetAs<ScriptCanvas::Data::BooleanType>());
                }

                QTableView* graphPalette = nullptr;
                ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(graphPalette, &ScriptCanvasEditor::VariableAutomationRequests::GetGraphPaletteTableView);

                if (graphPalette)
                {
                    // Assume that the variable will be the only element created in this state. So we can just use row 0.
                    QModelIndex tableIndex = graphPalette->model()->index(0, 2);

                    QRectF visualRect = graphPalette->visualRect(tableIndex);
                    QPointF clickPoint = visualRect.center();
                    clickPoint.setX(visualRect.left() + 15);

                    QPoint targetPoint = graphPalette->mapToGlobal(clickPoint.toPoint());

                    m_interactWithTableAction = aznew MouseClickAction(Qt::MouseButton::LeftButton, targetPoint);

                    actionRunner.AddAction(m_interactWithTableAction);
                }

                actionRunner.AddAction(&m_processEvents);
            }
            else
            {
                ReportError("Failed to find Created Variable");
            }
        }
        else
        {
            if (variableId == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid ScriptCanvas::VariableId", m_variableId.c_str()));
            }

            if (scriptCanvasId == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid ScriptCanvasId", StateModelIds::ScriptCanvasId));
            }
        }
    }

    void ToggleBoolInputTest::ToggleBoolInPaletteState::OnStateActionsComplete()
    {
        const ScriptCanvas::VariableId* variableId = GetStateModel()->GetStateDataAs<ScriptCanvas::VariableId>(m_variableId);
        const ScriptCanvas::ScriptCanvasId* scriptCanvasId = GetStateModel()->GetStateDataAs<ScriptCanvas::ScriptCanvasId>(StateModelIds::ScriptCanvasId);

        if (variableId && scriptCanvasId)
        {
            ScriptCanvas::GraphVariable* graphVariable = nullptr;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, (*scriptCanvasId), &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, (*variableId));

            if (graphVariable)
            {
                const ScriptCanvas::Datum* datum = graphVariable->GetDatum();

                if (datum && datum->GetType() == ScriptCanvas::Data::Type::Boolean())
                {
                    bool currentValue = (*datum->GetAs<ScriptCanvas::Data::BooleanType>());

                    if (currentValue == m_originalValue)
                    {
                        ReportError("Failed to toggle Boolean value");
                    }
                }
            }
        }
    }

    ToggleBoolInputTest::ToggleBoolInNodeState::ToggleBoolInNodeState(AutomationStateModelId endpointId)
        : NamedAutomationState("ToggleBoolInNodeState")
        , m_endpointId(endpointId)
        , m_clickAction(Qt::MouseButton::LeftButton)
    {
        SetStateName(AZStd::string::format("ToggleBoolInNodeState::%s", endpointId.c_str()));
    }

    void ToggleBoolInputTest::ToggleBoolInNodeState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        m_scriptCanvasEndpoint = ScriptCanvas::Endpoint();

        const GraphCanvas::Endpoint* gcEndpoint = GetStateModel()->GetStateDataAs<GraphCanvas::Endpoint>(m_endpointId);
        const ScriptCanvas::ScriptCanvasId* scriptCanvasId = GetStateModel()->GetStateDataAs<ScriptCanvas::ScriptCanvasId>(StateModelIds::ScriptCanvasId);

        if (gcEndpoint && scriptCanvasId)
        {
            ScriptCanvasEditor::EditorGraphRequestBus::EventResult(m_scriptCanvasEndpoint, (*scriptCanvasId), &ScriptCanvasEditor::EditorGraphRequests::ConvertToScriptCanvasEndpoint, (*gcEndpoint));

            if (m_scriptCanvasEndpoint.IsValid())
            {
                const ScriptCanvas::Datum* datum = nullptr;
                ScriptCanvas::NodeRequestBus::EventResult(datum, m_scriptCanvasEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::FindDatum, m_scriptCanvasEndpoint.GetSlotId());

                if (datum && datum->GetType() == ScriptCanvas::Data::Type::Boolean())
                {
                    const ScriptCanvas::Data::BooleanType* booleanType = datum->GetAs<ScriptCanvas::Data::BooleanType>();

                    if (booleanType)
                    {
                        m_originalValue = (*booleanType);
                    }
                }
                else
                {
                    ReportError("Datum is missing or incorrect type.");
                }
            }
            else
            {
                ReportError("Failed to convert Graph Canvas endpoint to Script Canvas endpoint");
            }

            m_mouseToNodePropertyEditorAction = aznew MouseToNodePropertyEditorAction(gcEndpoint->GetSlotId());

            actionRunner.AddAction(m_mouseToNodePropertyEditorAction);
            actionRunner.AddAction(&m_processEvents);
            actionRunner.AddAction(&m_clickAction);
            actionRunner.AddAction(&m_processEvents);
        }
    }

    void ToggleBoolInputTest::ToggleBoolInNodeState::OnStateActionsComplete()
    {
        if (m_scriptCanvasEndpoint.IsValid())
        {
            const ScriptCanvas::Datum* datum = nullptr;
            ScriptCanvas::NodeRequestBus::EventResult(datum, m_scriptCanvasEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::FindDatum, m_scriptCanvasEndpoint.GetSlotId());

            if (datum && datum->GetType() == ScriptCanvas::Data::Type::Boolean())
            {
                const ScriptCanvas::Data::BooleanType* booleanType = datum->GetAs<ScriptCanvas::Data::BooleanType>();

                if (booleanType)
                {
                    if ((*booleanType) == m_originalValue)
                    {
                        ReportError("Boolean datum did not toggle after interaction");
                    }
                }
            }
            else
            {
                ReportError("Datum is missing or incorrect type.");
            }
        }
    }

    ToggleBoolInputTest::ToggleBoolInputTest()
        : EditorAutomationTest("Bool Input Test")
    {
        AutomationStateModelId firstVariableTypeId = "VariableDataType";
        SetStateData(firstVariableTypeId, ScriptCanvas::Data::Type::Boolean());

        AutomationStateModelId variableNameId = "VariableName";
        SetStateData(variableNameId, AZStd::string("Boolean"));

        AddState(new CreateRuntimeGraphState());

        AutomationStateModelId variableId = "VariableId";
        AddState(new CreateVariableState(firstVariableTypeId, variableNameId, false, CreateVariableAction::CreationType::AutoComplete, variableId));

        AddState(new ToggleBoolInPaletteState(variableId));

        AutomationStateModelId viewCenter = "ViewCenter";
        AddState(new FindViewCenterState(viewCenter));

        AutomationStateModelId variableNodeId = "VariableNodeId";
        AddState(new CreateVariableNodeFromGraphPaletteState(variableNameId, viewCenter, Qt::KeyboardModifier::AltModifier, variableNodeId));

        AutomationStateModelId dataSlotId = "DataSlotId";
        AddState(new FindEndpointOfTypeState(variableNodeId, dataSlotId, GraphCanvas::ConnectionType::CT_Input, GraphCanvas::SlotTypes::DataSlot));

        AddState(new ToggleBoolInNodeState(dataSlotId));

        AddState(new ForceCloseActiveGraphState());
    }

    //////////////////////////
    // VariableLifeCycleTest
    //////////////////////////

    VariableLifeCycleTest::VariableLifeCycleTest(AZStd::string name, AZStd::vector<ScriptCanvas::Data::Type> dataTypes, CreateVariableAction::CreationType creationType)
        : EditorAutomationTest(name.c_str())
        , m_creationType(creationType)
        , m_typesToMake(dataTypes)
    {
        m_variableTypeId = "ActiveVariableTypeId";
        m_variableId = "ActiveVariableId";
        m_viewCenter = "ViewCenter";
        m_offsetCenter = "OffsetCenter";

        AutomationStateModelId variableNameId = "VariableName";
        
        AutomationStateModelId setNodeId = "SetNodeId";
        AutomationStateModelId getNodeId = "GetNodeId";

        SetStateData(variableNameId, AZStd::string("LifeCycle"));

        SetHasCustomTransitions(true);

        AddState(new CreateRuntimeGraphState());

        m_findViewCenterState = new FindViewCenterState(m_viewCenter);
        AddState(m_findViewCenterState);

        m_createVariableState = new CreateVariableState(m_variableTypeId, variableNameId, false, creationType, m_variableId);

        AddState(m_createVariableState);

        m_dragCreateGetNode = new CreateVariableNodeFromGraphPaletteState(variableNameId, m_viewCenter, Qt::KeyboardModifier::ShiftModifier, getNodeId);
        m_dragCreateSetNode = new CreateVariableNodeFromGraphPaletteState(variableNameId, m_offsetCenter, Qt::KeyboardModifier::AltModifier, setNodeId);

        AddState(m_dragCreateSetNode);
        AddState(m_dragCreateGetNode);

        m_createGetNode = new CreateNodeFromContextMenuState("Get LifeCycle", CreateNodeFromContextMenuState::CreationType::ScenePosition, m_viewCenter, getNodeId);
        m_createSetNode = new CreateNodeFromContextMenuState("Set LifeCycle", CreateNodeFromContextMenuState::CreationType::ScenePosition, m_offsetCenter, setNodeId);

        AddState(m_createGetNode);
        AddState(m_createSetNode);

        m_destroyGetNode = new AltClickSceneElementState(getNodeId);
        m_destroySetNode = new AltClickSceneElementState(setNodeId);

        AddState(m_destroyGetNode);
        AddState(m_destroySetNode);

        m_deleteVariableRowState = new DeleteVariableRowFromPaletteState(0);
        AddState(m_deleteVariableRowState);

        AddState(new ForceCloseActiveGraphState());

        SetInitialStateId<CreateRuntimeGraphStateId>();
    }

    void VariableLifeCycleTest::OnTestStarting()
    {
        m_activeIndex = -1;
    }

    int VariableLifeCycleTest::EvaluateTransition(int stateId)
    {
        if (stateId == ForceCloseActiveGraphStateId::StateID())
        {
            return EditorAutomationState::EXIT_STATE_ID;
        }
        else if (stateId == CreateRuntimeGraphStateId::StateID())
        {
            return m_findViewCenterState->GetStateId();
        }
        else if (stateId == m_findViewCenterState->GetStateId())
        {
            const AZ::Vector2* viewCenter = GetStateDataAs<AZ::Vector2>(m_viewCenter);

            if (viewCenter)
            {
                AZ::Vector2 offsetCenter = (*viewCenter);

                const AZ::Vector2* minorStep = GetStateDataAs<AZ::Vector2>(StateModelIds::MinorStep);

                if (minorStep)
                {
                    offsetCenter -= (*minorStep);
                }

                SetStateData(m_offsetCenter, offsetCenter);
            }

            return SetupNextVariable();
        }
        else if (stateId == m_createVariableState->GetStateId())
        {
            if (m_createVariablesNodesViaContextMenu)
            {
                return m_createGetNode->GetStateId();
            }
            else
            {
                return m_dragCreateGetNode->GetStateId();
            }
        }
        else if (stateId == m_createGetNode->GetStateId())
        {
            return m_createSetNode->GetStateId();
        }
        else if (stateId == m_dragCreateGetNode->GetStateId())
        {
            return m_dragCreateSetNode->GetStateId();
        }
        else if (stateId == m_createSetNode->GetStateId()
            || stateId == m_dragCreateSetNode->GetStateId())
        {
            return m_destroySetNode->GetStateId();
        }
        else if (stateId == m_destroySetNode->GetStateId())
        {
            return m_destroyGetNode->GetStateId();
        }
        else if (stateId == m_destroyGetNode->GetStateId())
        {
            return m_deleteVariableRowState->GetStateId();
        }
        else if (stateId == m_deleteVariableRowState->GetStateId())
        {
            return SetupNextVariable();
        }

        return EditorAutomationState::EXIT_STATE_ID;
        
    }

    int VariableLifeCycleTest::SetupNextVariable()
    {
        ++m_activeIndex;

        if (m_activeIndex < m_typesToMake.size())
        {
            SetStateData(m_variableTypeId, m_typesToMake[m_activeIndex]);
            SetStateData(m_variableId, ScriptCanvas::VariableId());

            return m_createVariableState->GetStateId();
        }

        return ForceCloseActiveGraphStateId::StateID();
    }

    //////////////////////////////////////
    // RapidVariableCreationDeletionTest
    //////////////////////////////////////

    RapidVariableCreationDeletionTest::RapidVariableCreationDeletionTest()
        : EditorAutomationTest("Mass Create/Destroy Variable Test")
        , m_variableType("Variable Type")
    {
        SetHasCustomTransitions(true);

        AddState(new CreateRuntimeGraphState());

        m_createVariableState = new CreateVariableState(m_variableType, "", false, CreateVariableAction::CreationType::Programmatic);
        AddState(m_createVariableState);

        m_deleteVariableRowState = new DeleteVariableRowFromPaletteState(0);
        AddState(m_deleteVariableRowState);

        AddState(new ForceCloseActiveGraphState());

        SetInitialStateId<CreateRuntimeGraphStateId>();
    }

    void RapidVariableCreationDeletionTest::OnTestStarting()
    {
        m_activeIndex = 0;
        ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(m_variableTypes, &ScriptCanvasEditor::VariableAutomationRequests::GetVariableTypes);
        ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(m_graphPalette, &ScriptCanvasEditor::VariableAutomationRequests::GetGraphPaletteTableView);
    }

    int RapidVariableCreationDeletionTest::EvaluateTransition(int stateId)
    {
        if (stateId == CreateRuntimeGraphStateId::StateID()
            || stateId == m_createVariableState->GetStateId())
        {
            return SetupNextVariable();
        }
        else if (stateId == m_deleteVariableRowState->GetStateId())
        {
            if (m_graphPalette->model()->rowCount() > 0)
            {
                return m_deleteVariableRowState->GetStateId();
            }
            else
            {
                return ForceCloseActiveGraphStateId::StateID();
            }
        }

        return EditorAutomationState::EXIT_STATE_ID;
    }

    int RapidVariableCreationDeletionTest::SetupNextVariable()
    {
        if (m_activeIndex >= m_variableTypes.size())
        {
            m_graphPalette->scrollToTop();
            return m_deleteVariableRowState->GetStateId();
        }
        else
        {
            SetStateData(m_variableType, m_variableTypes[m_activeIndex]);
            ++m_activeIndex;
            return m_createVariableState->GetStateId();
        }
    }
}
